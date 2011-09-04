/**
 * OpenColorIO ColorSpace Iop.
 */

#include "OCIOColorSpace.h"

namespace OCIO = OCIO_NAMESPACE;

#include <string>
#include <sstream>
#include <stdexcept>

#include <DDImage/Channel.h>
#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>
#include <DDImage/Row.h>
#include <DDImage/Knobs.h>
#include <DDImage/ddImageVersionNumbers.h>

// Should we use cascasing ColorSpace menus
#if defined kDDImageVersionInteger && (kDDImageVersionInteger>=62300)
#define OCIO_CASCASE
#endif

OCIOColorSpace::OCIOColorSpace(Node *n) : DD::Image::PixelIop(n)
{
    m_hasColorSpaces = false;

    m_inputColorSpaceIndex = 0;
    m_outputColorSpaceIndex = 0;

    m_layersToProcess = DD::Image::Mask_RGB;

    
    // Query the colorspace names from the current config
    // TODO (when to) re-grab the list of available colorspaces? How to save/load?
    
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        
        OCIO::ConstColorSpaceRcPtr defaultcs = config->getColorSpace(OCIO::ROLE_SCENE_LINEAR);
        if(!defaultcs) throw std::runtime_error("ROLE_SCENE_LINEAR not defined.");
        std::string defaultColorSpaceName = defaultcs->getName();
        
        for(int i = 0; i < config->getNumColorSpaces(); i++)
        {
            std::string csname = config->getColorSpaceNameByIndex(i);
            
#ifdef OCIO_CASCASE
            std::string family = config->getColorSpace(csname.c_str())->getFamily();
            if(family.empty())
                m_colorSpaceNames.push_back(csname.c_str());
            else
                m_colorSpaceNames.push_back(family + "/" + csname);
#else
            m_colorSpaceNames.push_back(csname);
#endif
            
            if(csname == defaultColorSpaceName)
            {
                m_inputColorSpaceIndex = i;
                m_outputColorSpaceIndex = i;
            }
        }
    }
    catch (OCIO::Exception& e)
    {
        std::cerr << "OCIOColorSpace: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "OCIOColorSpace: Unknown exception during OCIO setup." << std::endl;
    }
    
    // Then, create a cstr array for passing to Nuke
    // This must be done in a second pass, lest the original m_colorSpaceNames
    // std::string be reallocated in the interim
    for(unsigned int i=0; i<m_colorSpaceNames.size(); ++i)
    {
        m_inputColorSpaceCstrNames.push_back(m_colorSpaceNames[i].c_str());
        m_outputColorSpaceCstrNames.push_back(m_colorSpaceNames[i].c_str());
    }
    
    m_inputColorSpaceCstrNames.push_back(NULL);
    m_outputColorSpaceCstrNames.push_back(NULL);
    
    m_hasColorSpaces = (!m_colorSpaceNames.empty());
    
    if(!m_hasColorSpaces)
    {
        std::cerr << "OCIOColorSpace: No ColorSpaces available for input and/or output." << std::endl;
    }
}

OCIOColorSpace::~OCIOColorSpace()
{

}

void OCIOColorSpace::knobs(DD::Image::Knob_Callback f)
{
#ifdef OCIO_CASCASE
    DD::Image::CascadingEnumeration_knob(f,
        &m_inputColorSpaceIndex, &m_inputColorSpaceCstrNames[0], "in_colorspace", "in");
    DD::Image::Tooltip(f, "Input data is taken to be in this colorspace.");

    DD::Image::CascadingEnumeration_knob(f,
        &m_outputColorSpaceIndex, &m_outputColorSpaceCstrNames[0], "out_colorspace", "out");
    DD::Image::Tooltip(f, "Image data is converted to this colorspace for output.");
#else
    DD::Image::Enumeration_knob(f,
        &m_inputColorSpaceIndex, &m_inputColorSpaceCstrNames[0], "in_colorspace", "in");
    DD::Image::Tooltip(f, "Input data is taken to be in this colorspace.");

    DD::Image::Enumeration_knob(f,
        &m_outputColorSpaceIndex, &m_outputColorSpaceCstrNames[0], "out_colorspace", "out");
    DD::Image::Tooltip(f, "Image data is converted to this colorspace for output.");
#endif

    DD::Image::Divider(f);

    DD::Image::Input_ChannelSet_knob(f, &m_layersToProcess, 0, "layer", "layer");
    DD::Image::SetFlags(f, DD::Image::Knob::NO_CHECKMARKS | DD::Image::Knob::NO_ALPHA_PULLDOWN);
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

OCIO::ConstContextRcPtr OCIOColorSpace::getLocalContext()
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

void OCIOColorSpace::append(DD::Image::Hash& localhash)
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

void OCIOColorSpace::_validate(bool for_real)
{
    input0().validate(for_real);

    if(!m_hasColorSpaces)
    {
        error("No colorspaces available for input and/or output.");
        return;
    }

    int inputColorSpaceCount = static_cast<int>(m_inputColorSpaceCstrNames.size()) - 1;
    if(m_inputColorSpaceIndex < 0 || m_inputColorSpaceIndex >= inputColorSpaceCount)
    {
        std::ostringstream err;
        err << "Input colorspace index (" << m_inputColorSpaceIndex << ") out of range.";
        error(err.str().c_str());
        return;
    }

    int outputColorSpaceCount = static_cast<int>(m_outputColorSpaceCstrNames.size()) - 1;
    if(m_outputColorSpaceIndex < 0 || m_outputColorSpaceIndex >= outputColorSpaceCount)
    {
        std::ostringstream err;
        err << "Output colorspace index (" << m_outputColorSpaceIndex << ") out of range.";
        error(err.str().c_str());
        return;
    }

    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        config->sanityCheck();
        
        const char * inputName = config->getColorSpaceNameByIndex(m_inputColorSpaceIndex);
        const char * outputName = config->getColorSpaceNameByIndex(m_outputColorSpaceIndex);
        
        OCIO::ConstContextRcPtr context = getLocalContext();
        m_processor = config->getProcessor(context, inputName, outputName);
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

// Note that this is copied by others (OCIODisplay)
void OCIOColorSpace::in_channels(int /* n unused */, DD::Image::ChannelSet& mask) const
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

// See Saturation::pixel_engine for a well-commented example.
// Note that this is copied by others (OCIODisplay)
void OCIOColorSpace::pixel_engine(
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

const DD::Image::Op::Description OCIOColorSpace::description("OCIOColorSpace", build);

const char* OCIOColorSpace::Class() const
{
    return description.name;
}

const char* OCIOColorSpace::displayName() const
{
    return description.name;
}

const char* OCIOColorSpace::node_help() const
{
    // TODO more detailed help text
    return "Use OpenColorIO to convert from one ColorSpace to another.";
}


DD::Image::Op* build(Node *node)
{
    DD::Image::NukeWrapper *op = new DD::Image::NukeWrapper(new OCIOColorSpace(node));
    op->noMix();
    op->noMask();
    op->noChannels(); // prefer our own channels control without checkboxes / alpha
    op->noUnpremult();
    return op;
}
