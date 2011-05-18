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


OCIODisplay::OCIODisplay(Node *n) : DD::Image::PixelIop(n)
{
    m_layersToProcess = DD::Image::Mask_RGB;
    m_hasLists = false;
    m_colorSpaceIndex = m_displayIndex = m_viewIndex = 0;
    m_displayKnob = m_viewKnob = NULL;
    m_gain = 1.0;
    m_gamma = 1.0;
    //m_channel = 2;
    m_transform = OCIO::DisplayTransform::Create();
    
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        std::string defaultColorSpaceName = config->getColorSpace(OCIO::ROLE_SCENE_LINEAR)->getName();
        
        for(int i=0; i<config->getNumColorSpaces(); ++i)
        {
            std::string csname = config->getColorSpaceNameByIndex(i);
            m_colorSpaceNames.push_back(csname);
            
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
    DD::Image::Enumeration_knob(f,
        &m_colorSpaceIndex, &m_colorSpaceCstrNames[0], "colorspace", "input colorspace");
    DD::Image::SetFlags(f, DD::Image::Knob::SAVE_MENU);
    DD::Image::Tooltip(f, "Input data is taken to be in this colorspace.");

    m_displayKnob = DD::Image::Enumeration_knob(f,
        &m_displayIndex, &m_displayCstrNames[0], "display", "display device");
    DD::Image::SetFlags(f, DD::Image::Knob::SAVE_MENU);
    DD::Image::Tooltip(f, "Display device for output.");

    m_viewKnob = DD::Image::Enumeration_knob(f,
        &m_viewIndex, &m_viewCstrNames[0], "view", "view transform");
    DD::Image::SetFlags(f, DD::Image::Knob::SAVE_MENU);
    DD::Image::Tooltip(f, "Display transform for output.");
    
    DD::Image::Float_knob(f, &m_gain, DD::Image::IRange(1.0 / 64.0f, 64.0f), "gain");
    DD::Image::SetFlags(f, DD::Image::Knob::NO_ANIMATION | DD::Image::Knob::NO_UNDO | DD::Image::Knob::LOG_SLIDER);
    DD::Image::Tooltip(f, "Exposure adjustment, in scene-linear, prior to the display transform.");
    
    DD::Image::Float_knob(f, &m_gamma, DD::Image::IRange(0, 4), "gamma");
    DD::Image::SetFlags(f, DD::Image::Knob::NO_ANIMATION | DD::Image::Knob::NO_UNDO | DD::Image::Knob::LOG_SLIDER);
    DD::Image::Tooltip(f, "Gamma correction applied after the display transform.");
    
    /*
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
    */
    
    DD::Image::BeginClosedGroup(f, "Context");
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
    DD::Image::EndGroup(f);
    
    
    DD::Image::Divider(f);
    
    DD::Image::Input_ChannelSet_knob(f, &m_layersToProcess, 0, "layer", "layer");
    DD::Image::SetFlags(f, DD::Image::Knob::NO_CHECKMARKS | DD::Image::Knob::NO_ALPHA_PULLDOWN);
    DD::Image::Tooltip(f, "Set which layer to process. This should be a layer with rgb data.");
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
        
        const char * csSrcName = m_colorSpaceCstrNames[m_colorSpaceIndex];
        const char * display = m_displayCstrNames[m_displayIndex];
        const char * view = m_viewCstrNames[m_viewIndex];
        const char * csDstName = config->getDisplayColorSpaceName(display, view);

        m_transform->setInputColorSpaceName(csSrcName);
        m_transform->setDisplayColorSpaceName(csDstName);
        
        // Specify an (optional) linear color correction
        {
            const float slope3f[] = { m_gain, m_gain, m_gain };
            OCIO::CDLTransformRcPtr cc =  OCIO::CDLTransform::Create();
            cc->setSlope(slope3f);
            m_transform->setLinearCC(cc);
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
        // TODO: This wont work until OCIO supports alpha swizzling in the
        // sw path
        #if 0
        {
            int channelHot[4] = { 0, 0, 0, 0};
            
            switch(m_channel)
            {
            case 0: // Luma
                channelHot[0] = 1;
                channelHot[1] = 1;
                channelHot[2] = 1;
                break;
            case 1:
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
        #endif
        
        OCIO::ConstContextRcPtr context = getLocalContext();
        m_processor = config->getProcessor(context,
                                           m_transform,
                                           OCIO::TRANSFORM_DIR_FORWARD);
    }
    catch(OCIO::Exception &e)
    {
        error(e.what());
        return;
    }
    
    if(m_processor->isNoOp())
    {
        // TODO or call disable() ?
        set_out_channels(DD::Image::Mask_None); // prevents engine() from being called
        copy_info();
        return;
    }
    
    set_out_channels(DD::Image::Mask_All);

    DD::Image::PixelIop::_validate(for_real);
}

// Note: Same as OCIO ColorSpace::in_channels.
void OCIODisplay::in_channels(int /* n unused */, DD::Image::ChannelSet& mask) const
{
    DD::Image::ChannelSet done;
    foreach(c, mask)
    {
        if ((m_layersToProcess & c) && DD::Image::colourIndex(c) < 3 && !(done & c))
        {
            done.addBrothers(c, 3);
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
        if (!(m_layersToProcess & requestedChannel) || colourIndex(requestedChannel) >= 3)
        {
            out.copy(in, requestedChannel, rowX, rowXBound);
            continue;
        }

        DD::Image::Channel rChannel = DD::Image::brother(requestedChannel, 0);
        DD::Image::Channel gChannel = DD::Image::brother(requestedChannel, 1);
        DD::Image::Channel bChannel = DD::Image::brother(requestedChannel, 2);

        done += rChannel;
        done += gChannel;
        done += bChannel;

        const float *rIn = in[rChannel] + rowX;
        const float *gIn = in[gChannel] + rowX;
        const float *bIn = in[bChannel] + rowX;

        float *rOut = out.writable(rChannel) + rowX;
        float *gOut = out.writable(gChannel) + rowX;
        float *bOut = out.writable(bChannel) + rowX;

        // OCIO modifies in-place
        memcpy(rOut, rIn, sizeof(float)*rowWidth);
        memcpy(gOut, gIn, sizeof(float)*rowWidth);
        memcpy(bOut, bIn, sizeof(float)*rowWidth);

        try
        {
            OCIO::PlanarImageDesc img(rOut, gOut, bOut, rowWidth, /*height*/ 1);
            m_processor->apply(img);
        }
        catch(OCIO::Exception &e)
        {
            error(e.what());
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
        error(err.str().c_str());
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
