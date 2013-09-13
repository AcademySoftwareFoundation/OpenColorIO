/**
 * OpenColorIO Display Iop.
 */

#include "OCIODisplay.h"

namespace OCIO = OCIO_NAMESPACE;

#include <algorithm>
#include <string>
#include <sstream>
#include <stdexcept>

#include <DDImage/Channel.h>
#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>
#include <DDImage/Row.h>
#include <DDImage/Knobs.h>
#include <DDImage/Enumeration_KnobI.h>
#include <DDImage/ddImageVersionNumbers.h>

// Should we use cascasing ColorSpace menus
#if defined kDDImageVersionInteger && (kDDImageVersionInteger>=62300)
#define OCIO_CASCADE
#endif

OCIODisplay::OCIODisplay(Node *n) : DD::Image::PixelIop(n)
{
    m_layersToProcess = DD::Image::Mask_RGBA;
    m_hasLists = false;
    m_colorSpaceIndex = m_displayIndex = m_viewIndex = 0;
    m_displayKnob = m_viewKnob = NULL;
    m_gain = 1.0;
    m_gamma = 1.0;
    m_channel = 2;
    m_transform = OCIO::DisplayTransform::Create();
    
#ifdef OCIO_NUKE_GPU_ENABLE
    m_textureUnit = -1;
    m_textureHandle = 0;
#endif
    
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        
        OCIO::ConstColorSpaceRcPtr defaultcs = config->getColorSpace(OCIO::ROLE_SCENE_LINEAR);
        if(!defaultcs) throw std::runtime_error("ROLE_SCENE_LINEAR not defined.");
        std::string defaultColorSpaceName = defaultcs->getName();
        
        for(int i=0; i<config->getNumColorSpaces(); ++i)
        {
            std::string csname = config->getColorSpaceNameByIndex(i);
            
#ifdef OCIO_CASCADE
            std::string family = config->getColorSpace(csname.c_str())->getFamily();
            if(family.empty())
                m_colorSpaceNames.push_back(csname.c_str());
            else
                m_colorSpaceNames.push_back(family + "/" + csname);
#else
            m_colorSpaceNames.push_back(csname);
#endif
            
            if(defaultColorSpaceName == csname)
            {
                m_colorSpaceIndex = i;
            }
        }
        
        std::string defaultDisplay = config->getDefaultDisplay();
        
        for(int i=0; i<config->getNumDisplays(); ++i)
        {
            std::string display = config->getDisplay(i);
            m_displayNames.push_back(display);
            
            if(display == defaultDisplay)
            {
                m_displayIndex = i;
            }
        }
    }
    catch(OCIO::Exception& e)
    {
        std::cerr << "OCIODisplay: " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "OCIODisplay: Unknown exception during OCIO setup." << std::endl;
    }
    
    // Build the cstr vectors on our second pass
    for(unsigned int i=0; i<m_colorSpaceNames.size(); ++i)
    {
        m_colorSpaceCstrNames.push_back(m_colorSpaceNames[i].c_str());
    }
    m_colorSpaceCstrNames.push_back(NULL);
    
    for(unsigned int i=0; i<m_displayNames.size(); ++i)
    {
        m_displayCstrNames.push_back(m_displayNames[i].c_str());
    }
    m_displayCstrNames.push_back(NULL);
    
    refreshDisplayTransforms();
    
    m_hasLists = !(m_colorSpaceNames.empty() || m_displayNames.empty() || m_viewNames.empty());
    
    if(!m_hasLists)
    {
        std::cerr << "OCIODisplay: Missing one or more of colorspaces, display devices, or display transforms." << std::endl;
    }
}

OCIODisplay::~OCIODisplay()
{

}

void OCIODisplay::knobs(DD::Image::Knob_Callback f)
{
#ifdef OCIO_CASCADE
    DD::Image::CascadingEnumeration_knob(f,
        &m_colorSpaceIndex, &m_colorSpaceCstrNames[0], "colorspace", "input colorspace");
#else
    DD::Image::Enumeration_knob(f,
        &m_colorSpaceIndex, &m_colorSpaceCstrNames[0], "colorspace", "input colorspace");
#endif
    DD::Image::SetFlags(f, DD::Image::Knob::ALWAYS_SAVE);
    DD::Image::Tooltip(f, "Input data is taken to be in this colorspace.");

    m_displayKnob = DD::Image::Enumeration_knob(f,
        &m_displayIndex, &m_displayCstrNames[0], "display", "display device");
    DD::Image::SetFlags(f, DD::Image::Knob::ALWAYS_SAVE);
    DD::Image::Tooltip(f, "Display device for output.");

    m_viewKnob = DD::Image::Enumeration_knob(f,
        &m_viewIndex, &m_viewCstrNames[0], "view", "view transform");
    DD::Image::SetFlags(f, DD::Image::Knob::ALWAYS_SAVE);
    DD::Image::Tooltip(f, "Display transform for output.");
    
    DD::Image::Float_knob(f, &m_gain, DD::Image::IRange(1.0 / 64.0f, 64.0f), "gain");
    DD::Image::SetFlags(f, DD::Image::Knob::NO_ANIMATION | DD::Image::Knob::NO_UNDO | DD::Image::Knob::LOG_SLIDER);
    DD::Image::Tooltip(f, "Exposure adjustment, in scene-linear, prior to the display transform.");
    
    DD::Image::Float_knob(f, &m_gamma, DD::Image::IRange(0, 4), "gamma");
    DD::Image::SetFlags(f, DD::Image::Knob::NO_ANIMATION | DD::Image::Knob::NO_UNDO | DD::Image::Knob::LOG_SLIDER);
    DD::Image::Tooltip(f, "Gamma correction applied after the display transform.");
    
    static const char* const channelvalues[] = {
      "Luminance",
      "Matte overlay",
      "RGB",
      "R",
      "G",
      "B",
      "A",
      0
    };
    DD::Image::Enumeration_knob(f, &m_channel, channelvalues, "channel_selector", "channel view");
    DD::Image::SetFlags(f, DD::Image::Knob::NO_ANIMATION | DD::Image::Knob::NO_UNDO);
    DD::Image::Tooltip(f, "Specify which channels to view (prior to the display transform).");
    
    DD::Image::Divider(f);
    
    DD::Image::Input_ChannelSet_knob(f, &m_layersToProcess, 0, "layer", "layer");
    // DD::Image::SetFlags(f, DD::Image::Knob::NO_CHECKMARKS | DD::Image::Knob::NO_ALPHA_PULLDOWN);
    DD::Image::Tooltip(f, "Set which layer to process. This should be a layer with rgb data.");
    
    DD::Image::Tab_knob(f, "Context");
    {
        DD::Image::String_knob(f, &m_contextKey1, "key1");
        DD::Image::Spacer(f, 10);
        DD::Image::String_knob(f, &m_contextValue1, "value1");
        DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE);
        
        DD::Image::String_knob(f, &m_contextKey2, "key2");
        DD::Image::Spacer(f, 10);
        DD::Image::String_knob(f, &m_contextValue2, "value2");
        DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE);
        
        DD::Image::String_knob(f, &m_contextKey3, "key3");
        DD::Image::Spacer(f, 10);
        DD::Image::String_knob(f, &m_contextValue3, "value3");
        DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE);
        
        DD::Image::String_knob(f, &m_contextKey4, "key4");
        DD::Image::Spacer(f, 10);
        DD::Image::String_knob(f, &m_contextValue4, "value4");
        DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE);
    }
}

OCIO::ConstContextRcPtr OCIODisplay::getLocalContext()
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    OCIO::ConstContextRcPtr context = config->getCurrentContext();
    OCIO::ContextRcPtr mutableContext;
    
    if(!m_contextKey1.empty())
    {
        if(!mutableContext) mutableContext = context->createEditableCopy();
        mutableContext->setStringVar(m_contextKey1.c_str(), m_contextValue1.c_str());
    }
    if(!m_contextKey2.empty())
    {
        if(!mutableContext) mutableContext = context->createEditableCopy();
        mutableContext->setStringVar(m_contextKey2.c_str(), m_contextValue2.c_str());
    }
    if(!m_contextKey3.empty())
    {
        if(!mutableContext) mutableContext = context->createEditableCopy();
        mutableContext->setStringVar(m_contextKey3.c_str(), m_contextValue3.c_str());
    }
    if(!m_contextKey4.empty())
    {
        if(!mutableContext) mutableContext = context->createEditableCopy();
        mutableContext->setStringVar(m_contextKey4.c_str(), m_contextValue4.c_str());
    }
    
    if(mutableContext) context = mutableContext;
    
    return context;
}

void OCIODisplay::append(DD::Image::Hash& localhash)
{
    // TODO: Hang onto the context, what if getting it
    // (and querying getCacheID) is expensive?
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        OCIO::ConstContextRcPtr context = getLocalContext();
        std::string configCacheID = config->getCacheID(context);
        localhash.append(configCacheID);
        
        // This is required due to our custom channel overlay mode post-processing
        localhash.append(m_channel);
    }
    catch(OCIO::Exception &e)
    {
        error(e.what());
        return;
    }
}

void OCIODisplay::_validate(bool for_real)
{
    input0().validate(for_real);

    if(!m_hasLists)
    {
        error("Missing one or more of colorspaces, display devices, or display transforms.");
        return;
    }

    int nColorSpaces = static_cast<int>(m_colorSpaceNames.size());
    if(m_colorSpaceIndex < 0 || m_colorSpaceIndex >= nColorSpaces)
    {
        std::ostringstream err;
        err << "ColorSpace index (" << m_colorSpaceIndex << ") out of range.";
        error(err.str().c_str());
        return;
    }

    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        
        m_transform->setInputColorSpaceName(config->getColorSpaceNameByIndex(m_colorSpaceIndex));
        m_transform->setDisplay(m_displayCstrNames[m_displayIndex]);
        m_transform->setView(m_viewCstrNames[m_viewIndex]);
        
        // Specify an (optional) linear color correction
        {
            float m44[16];
            float offset4[4];
            
            const float slope4f[] = { m_gain, m_gain, m_gain, m_gain };
            OCIO::MatrixTransform::Scale(m44, offset4, slope4f);
            
            OCIO::MatrixTransformRcPtr mtx =  OCIO::MatrixTransform::Create();
            mtx->setValue(m44, offset4);
            
            m_transform->setLinearCC(mtx);
        }
        
        // Specify an (optional) post-display transform.
        {
            float exponent = 1.0f/std::max(1e-6f, m_gamma);
            const float exponent4f[] = { exponent, exponent, exponent, exponent };
            OCIO::ExponentTransformRcPtr cc =  OCIO::ExponentTransform::Create();
            cc->setValue(exponent4f);
            m_transform->setDisplayCC(cc);
        }
        
        // Add Channel swizzling
        {
            int channelHot[4] = { 0, 0, 0, 0};
            
            switch(m_channel)
            {
            case 0: // Luma
                channelHot[0] = 1;
                channelHot[1] = 1;
                channelHot[2] = 1;
                break;
            case 1: //  Channel overlay mode. Do rgb, and then swizzle later
                channelHot[0] = 1;
                channelHot[1] = 1;
                channelHot[2] = 1;
                channelHot[3] = 1;
                break;
            case 2: // RGB
                channelHot[0] = 1;
                channelHot[1] = 1;
                channelHot[2] = 1;
                channelHot[3] = 1;
                break;
            case 3: // R
                channelHot[0] = 1;
                break;
            case 4: // G
                channelHot[1] = 1;
                break;
            case 5: // B
                channelHot[2] = 1;
                break;
            case 6: // A
                channelHot[3] = 1;
                break;
            default:
                break;
            }
            
            float lumacoef[3];
            config->getDefaultLumaCoefs(lumacoef);
            float m44[16];
            float offset[4];
            OCIO::MatrixTransform::View(m44, offset, channelHot, lumacoef);
            OCIO::MatrixTransformRcPtr swizzle = OCIO::MatrixTransform::Create();
            swizzle->setValue(m44, offset);
            m_transform->setChannelView(swizzle);
        }
        
        OCIO::ConstContextRcPtr context = getLocalContext();
        m_processor = config->getProcessor(context,
                                           m_transform,
                                           OCIO::TRANSFORM_DIR_FORWARD);

#ifdef OCIO_NUKE_GPU_ENABLE
        // TODO: Make this optional? Only computed before needed?
        // Generate the GLSL code
        OCIO::GpuShaderDesc shaderDesc;
        shaderDesc.setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3);
        shaderDesc.setFunctionName("OCIODisplay$$");
        shaderDesc.setLut3DEdgeLen(LUT3D_EDGE_SIZE);
        
        int num3Dentries = 3*LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE;
        m_lut3d.resize(num3Dentries);
        
        m_processor->getGpuLut3D(&m_lut3d[0], shaderDesc);

        // Store a variable name for the sampler3D object that is unique to this Op instance
        m_textureName = uniqueGPUShaderId("$$lut");
        
        // GPU path shader declaration
        std::stringstream gpuDecl;
        gpuDecl << "uniform sampler3D " << m_textureName << ";\n";
        gpuDecl << m_processor->getGpuShaderText(shaderDesc);
        m_gpuEngineDecl = gpuDecl.str();
                
        // GPU path shader body
        std::stringstream gpuBody;
        gpuBody << "OUT.rgb = OCIODisplay$$(OUT, " << m_textureName << ").rgb;\n";
        m_gpuEngineBody = gpuBody.str();
#endif
    }
    catch(OCIO::Exception &e)
    {
        error(e.what());
        return;
    }
    
    if(m_processor->isNoOp())
    {
        set_out_channels(DD::Image::Mask_None); // prevents engine() from being called
    } else {    
        set_out_channels(DD::Image::Mask_All);
    }

    DD::Image::PixelIop::_validate(for_real);
}

// Note: Same as OCIO ColorSpace::in_channels.
void OCIODisplay::in_channels(int /* n unused */, DD::Image::ChannelSet& mask) const
{
    DD::Image::ChannelSet done;
    foreach(c, mask)
    {
        if ((m_layersToProcess & c) && DD::Image::colourIndex(c) < 4 && !(done & c))
        {
            done.addBrothers(c, 4);
        }
    }
    mask += done;
}

// Note: Same as OCIO ColorSpace::pixel_engine.
void OCIODisplay::pixel_engine(
    const DD::Image::Row& in,
    int /* rowY unused */, int rowX, int rowXBound,
    DD::Image::ChannelMask outputChannels,
    DD::Image::Row& out)
{
    int rowWidth = rowXBound - rowX;

    DD::Image::ChannelSet done;
    foreach (requestedChannel, outputChannels)
    {
        // Skip channels which had their trios processed already,
        if (done & requestedChannel)
        {
            continue;
        }

        // Pass through channels which are not selected for processing
        // and non-rgb channels.
        if (!(m_layersToProcess & requestedChannel))
        {
            out.copy(in, requestedChannel, rowX, rowXBound);
            continue;
        }

        DD::Image::Channel rChannel = DD::Image::brother(requestedChannel, 0);
        DD::Image::Channel gChannel = DD::Image::brother(requestedChannel, 1);
        DD::Image::Channel bChannel = DD::Image::brother(requestedChannel, 2);
        DD::Image::Channel aChannel = outputChannels.next(bChannel);

        done += rChannel;
        done += gChannel;
        done += bChannel;
        done += aChannel;

        const float *rIn = in[rChannel] + rowX;
        const float *gIn = in[gChannel] + rowX;
        const float *bIn = in[bChannel] + rowX;
        const float *aIn = in[aChannel] + rowX;

        float *rOut = out.writable(rChannel) + rowX;
        float *gOut = out.writable(gChannel) + rowX;
        float *bOut = out.writable(bChannel) + rowX;
        float *aOut = out.writable(aChannel) + rowX;

        // OCIO modifies in-place
        // Note: xOut can equal xIn in some circumstances, such as when the
        // 'Black' (throwaway) scanline is uses. We thus must guard memcpy,
        // which does not allow for overlapping regions.
        if (rOut != rIn) memcpy(rOut, rIn, sizeof(float)*rowWidth);
        if (gOut != gIn) memcpy(gOut, gIn, sizeof(float)*rowWidth);
        if (bOut != bIn) memcpy(bOut, bIn, sizeof(float)*rowWidth);
        if (aOut != aIn) memcpy(aOut, aIn, sizeof(float)*rowWidth);

        try
        {
            OCIO::PlanarImageDesc img(rOut, gOut, bOut, aOut, rowWidth, /*height*/ 1);
            m_processor->apply(img);
        }
        catch(OCIO::Exception &e)
        {
            error(e.what());
        }
        
        // Hack to emulate Channel overlay mode
        if(m_channel == 1)
        {
            for(int i=0; i<rowWidth; ++i)
            {
                rOut[i] = rOut[i] + (1.0f - rOut[i]) * (0.5f * aOut[i]);
                gOut[i] = gOut[i] - gOut[i] * (0.5f * aOut[i]);
                bOut[i] = bOut[i] - bOut[i] * (0.5f * aOut[i]);
            }
        }
    }
}

void OCIODisplay::refreshDisplayTransforms()
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

    if (m_displayIndex < 0 || m_displayIndex >= static_cast<int>(m_displayNames.size()))
    {
        std::ostringstream err;
        err << "No or invalid display specified (index " << m_displayIndex << ").";
        std::cerr << err.str(); // This can't set an error, as it's called in the constructor
        return;
    }

    const char * display = m_displayCstrNames[m_displayIndex];
    int numViews = config->getNumViews(display);
    std::string defaultViewName = config->getDefaultView(display);

    // Try to maintain an old transform name, or use the default.
    bool hasOldViewName = false;
    std::string oldViewName = "";
    if (m_viewIndex >= 0 && m_viewIndex < static_cast<int>(m_viewNames.size()))
    {
        hasOldViewName = true;
        oldViewName = m_viewNames[m_viewIndex];
    }
    int defaultViewIndex = 0, newViewIndex = -1;

    m_viewCstrNames.clear();
    m_viewNames.clear();

    for(int i = 0; i < numViews; i++)
    {
        std::string view = config->getView(display, i);
        m_viewNames.push_back(view);
        if (hasOldViewName && view == oldViewName)
        {
            newViewIndex = i;
        }
        if (view == defaultViewName)
        {
            defaultViewIndex = i;
        }
    }
    
    for(unsigned int i=0; i<m_viewNames.size(); ++i)
    {
        m_viewCstrNames.push_back(m_viewNames[i].c_str());
    }
    m_viewCstrNames.push_back(NULL);
    
    if (newViewIndex == -1)
    {
        newViewIndex = defaultViewIndex;
    }

    if (m_viewKnob == NULL)
    {
        m_viewIndex = newViewIndex;
    }
    else
    {
        DD::Image::Enumeration_KnobI *enumKnob = m_viewKnob->enumerationKnob();
        enumKnob->menu(m_viewNames);
        m_viewKnob->set_value(newViewIndex);
    }
}

int OCIODisplay::knob_changed(DD::Image::Knob *k)
{
    if (k == m_displayKnob && m_displayKnob != NULL)
    {
        refreshDisplayTransforms();
        return 1;
    }
    else
    {
        return 0;
    }
}

#ifdef OCIO_NUKE_GPU_ENABLE
void OCIODisplay::checkGLError(const char* scope)
{
    GLenum glError = glGetError();
    if (glError != GL_NO_ERROR)
    {
        std::ostringstream msg;
        msg << scope << ": " << gluErrorString(glError) << std::endl;
        warning(msg.str().c_str());
    }
}

const char* OCIODisplay::gpuEngine_decl() const
{
    return m_gpuEngineDecl.c_str();
}

const char* OCIODisplay::gpuEngine_body() const
{
    return m_gpuEngineBody.c_str();
}

int OCIODisplay::gpuEngine_getNumRequiredTexUnits() const
{
    return 1;
}

void OCIODisplay::gpuEngine_GL_begin(DD::Image::GPUContext* context)
{
    // Get the texture unit from Nuke based on it's internal texture tracking
    m_textureUnit = context->acquireTextureUnit();
    
    // If we've switched targets or if this is the first time creating our texture, let's do that now
    bool updateTex = true;
    if (m_textureHandle == 0)
    {
        glGenTextures(1, &m_textureHandle);
        checkGLError("Generating texture");
        updateTex = false;
        debug("Processing in GPU mode.");
    }
    
    glActiveTextureARB(GL_TEXTURE0_ARB + int(m_textureUnit));
    checkGLError("Activating texture");

    // Enable 3d texturing
    glEnable(GL_TEXTURE_3D);
    checkGLError("Enabling GL_TEXTURE_3D");
    glBindTexture(GL_TEXTURE_3D, m_textureHandle);
    checkGLError("Binding texture");
    glActiveTextureARB(GL_TEXTURE0_ARB);
    checkGLError("Activating GL_TEXTURE0_ARB"); 
    
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    // Bind the texture handle to the GL thingy
    glBindTexture(GL_TEXTURE_3D, m_textureHandle);
    checkGLError("Binding texture");
    
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    checkGLError("Setting wrap parameters");
    
    // Either update the existing texture or make a new one
    if (updateTex)
    {
        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0,
                LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE, GL_RGB, GL_FLOAT, &m_lut3d[0]);
        checkGLError("Updating texture");
    }
    else
    {
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        checkGLError("Setting filter parameters");
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB,
            LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE,
            0, GL_RGB, GL_FLOAT, &m_lut3d[0]);
        checkGLError("Creating texture");
    }
    
    // enable
    glActiveTextureARB(GL_TEXTURE0_ARB + m_textureUnit);
    glEnable(GL_TEXTURE_3D);
    checkGLError("Enabling 3D textures");
    glBindTexture(GL_TEXTURE_3D, m_textureHandle);
    checkGLError("Binding texture");

    glActiveTextureARB(GL_TEXTURE0_ARB);
    
    bool bindResult = context->bind(m_textureName.c_str(), m_textureUnit);
    checkGLError("Binding texture via context");
}

void OCIODisplay::gpuEngine_GL_end(DD::Image::GPUContext* context)
{
    checkGLError("Compiling and executing shader");
    
    if (m_textureUnit >= 0)
    {
        glActiveTextureARB(GL_TEXTURE0_ARB + int(m_textureUnit));
        checkGLError("Activating texture");
        glDisable(GL_TEXTURE_3D);
        checkGLError("Disabling texture");
        glBindTexture(GL_TEXTURE_3D, 0);
        checkGLError("Binding texture");
        glActiveTextureARB(GL_TEXTURE0_ARB);
    }
    
    context->releaseTextureUnit(m_textureUnit);
}
#endif

const DD::Image::Op::Description OCIODisplay::description("OCIODisplay", build);

const char* OCIODisplay::Class() const
{
    return description.name;
}

const char* OCIODisplay::displayName() const
{
    return description.name;
}

const char* OCIODisplay::node_help() const
{
    // TODO more detailed help text
    return "Use OpenColorIO to convert for a display device.";
}


DD::Image::Op* build(Node *node)
{
    DD::Image::NukeWrapper *op = new DD::Image::NukeWrapper(new OCIODisplay(node));
    op->noMix();
    op->noMask();
    op->noChannels(); // prefer our own channels control without checkboxes / alpha
    op->noUnpremult();
    return op;
}
