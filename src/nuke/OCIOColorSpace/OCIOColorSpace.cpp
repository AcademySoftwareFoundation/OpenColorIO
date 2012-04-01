/**
 * OpenColourIO ColourSpace Iop.
 */

#include "OCIOColourSpace.h"

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

// Should we use cascasing ColourSpace menus
#if defined kDDImageVersionInteger && (kDDImageVersionInteger>=62300)
#define OCIO_CASCADE
#endif

OCIOColourSpace::OCIOColourSpace(Node *n) : DD::Image::PixelIop(n)
{
    m_hasColourSpaces = false;

    m_inputColourSpaceIndex = 0;
    m_outputColourSpaceIndex = 0;
    
    // Query the colour space names from the current config
    // TODO (when to) re-grab the list of available colour spaces? How to save/load?
    
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        
        OCIO::ConstColourSpaceRcPtr defaultcs = config->getColourSpace(OCIO::ROLE_SCENE_LINEAR);
        if(!defaultcs) throw std::runtime_error("ROLE_SCENE_LINEAR not defined.");
        std::string defaultColourSpaceName = defaultcs->getName();
        
        for(int i = 0; i < config->getNumColourSpaces(); i++)
        {
            std::string csname = config->getColourSpaceNameByIndex(i);
            
#ifdef OCIO_CASCADE
            std::string family = config->getColourSpace(csname.c_str())->getFamily();
            if(family.empty())
                m_colourSpaceNames.push_back(csname.c_str());
            else
                m_colourSpaceNames.push_back(family + "/" + csname);
#else
            m_colourSpaceNames.push_back(csname);
#endif
            
            if(csname == defaultColourSpaceName)
            {
                m_inputColourSpaceIndex = i;
                m_outputColourSpaceIndex = i;
            }
        }
    }
    catch (OCIO::Exception& e)
    {
        std::cerr << "OCIOColourSpace: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "OCIOColourSpace: Unknown exception during OCIO setup." << std::endl;
    }
    
    // Then, create a cstr array for passing to Nuke
    // This must be done in a second pass, lest the original m_colourSpaceNames
    // std::string be reallocated in the interim
    for(unsigned int i=0; i<m_colourSpaceNames.size(); ++i)
    {
        m_inputColourSpaceCstrNames.push_back(m_colourSpaceNames[i].c_str());
        m_outputColourSpaceCstrNames.push_back(m_colourSpaceNames[i].c_str());
    }
    
    m_inputColourSpaceCstrNames.push_back(NULL);
    m_outputColourSpaceCstrNames.push_back(NULL);
    
    m_hasColourSpaces = (!m_colourSpaceNames.empty());
    
    if(!m_hasColourSpaces)
    {
        std::cerr << "OCIOColourSpace: No colour spaces available for input and/or output." << std::endl;
    }
}

OCIOColourSpace::~OCIOColourSpace()
{

}

void OCIOColourSpace::knobs(DD::Image::Knob_Callback f)
{
#ifdef OCIO_CASCADE
    DD::Image::CascadingEnumeration_knob(f,
        &m_inputColourSpaceIndex, &m_inputColourSpaceCstrNames[0], "in_colourspace", "in");
    DD::Image::Tooltip(f, "Input data is taken to be in this colour space.");
    DD::Image::SetFlags(f, DD::Image::Knob::ALWAYS_SAVE);

    DD::Image::CascadingEnumeration_knob(f,
        &m_outputColourSpaceIndex, &m_outputColourSpaceCstrNames[0], "out_colourspace", "out");
    DD::Image::Tooltip(f, "Image data is converted to this colour space for output.");
    DD::Image::SetFlags(f, DD::Image::Knob::ALWAYS_SAVE);
#else
    DD::Image::Enumeration_knob(f,
        &m_inputColourSpaceIndex, &m_inputColourSpaceCstrNames[0], "in_colourspace", "in");
    DD::Image::Tooltip(f, "Input data is taken to be in this colour space.");
    DD::Image::SetFlags(f, DD::Image::Knob::ALWAYS_SAVE);

    DD::Image::Enumeration_knob(f,
        &m_outputColourSpaceIndex, &m_outputColourSpaceCstrNames[0], "out_colourspace", "out");
    DD::Image::Tooltip(f, "Image data is converted to this colour space for output.");
    DD::Image::SetFlags(f, DD::Image::Knob::ALWAYS_SAVE);
#endif
    
}

OCIO::ConstContextRcPtr OCIOColourSpace::getLocalContext()
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

void OCIOColourSpace::append(DD::Image::Hash& localhash)
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

void OCIOColourSpace::_validate(bool for_real)
{
    if(!m_hasColourSpaces)
    {
        error("No colour spaces available for input and/or output.");
        return;
    }

    int inputColourSpaceCount = static_cast<int>(m_inputColourSpaceCstrNames.size()) - 1;
    if(m_inputColourSpaceIndex < 0 || m_inputColourSpaceIndex >= inputColourSpaceCount)
    {
        std::ostringstream err;
        err << "Input colour space index (" << m_inputColourSpaceIndex << ") out of range.";
        error(err.str().c_str());
        return;
    }

    int outputColourSpaceCount = static_cast<int>(m_outputColourSpaceCstrNames.size()) - 1;
    if(m_outputColourSpaceIndex < 0 || m_outputColourSpaceIndex >= outputColourSpaceCount)
    {
        std::ostringstream err;
        err << "Output colour space index (" << m_outputColourSpaceIndex << ") out of range.";
        error(err.str().c_str());
        return;
    }

    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        
        const char * inputName = config->getColourSpaceNameByIndex(m_inputColourSpaceIndex);
        const char * outputName = config->getColourSpaceNameByIndex(m_outputColourSpaceIndex);
        
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
        set_out_channels(DD::Image::Mask_None); // prevents engine() from being called
    } else {    
        set_out_channels(DD::Image::Mask_All);
    }

    DD::Image::PixelIop::_validate(for_real);
}

// Note that this is copied by others (OCIODisplay)
void OCIOColourSpace::in_channels(int /* n unused */, DD::Image::ChannelSet& mask) const
{
    DD::Image::ChannelSet done;
    foreach(c, mask)
    {
        if (DD::Image::colourIndex(c) < 3 && !(done & c))
        {
            done.addBrothers(c, 3);
        }
    }
    mask += done;
}

// See Saturation::pixel_engine for a well-commented example.
// Note that this is copied by others (OCIODisplay)
void OCIOColourSpace::pixel_engine(
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
        if (colourIndex(requestedChannel) >= 3)
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
        // Note: xOut can equal xIn in some circumstances, such as when the
        // 'Black' (throwaway) scanline is uses. We thus must guard memcpy,
        // which does not allow for overlapping regions.
        if (rOut != rIn) memcpy(rOut, rIn, sizeof(float)*rowWidth);
        if (gOut != gIn) memcpy(gOut, gIn, sizeof(float)*rowWidth);
        if (bOut != bIn) memcpy(bOut, bIn, sizeof(float)*rowWidth);

        try
        {
            OCIO::PlanarImageDesc img(rOut, gOut, bOut, NULL, rowWidth, /*height*/ 1);
            m_processor->apply(img);
        }
        catch(OCIO::Exception &e)
        {
            error(e.what());
        }
    }
}

const DD::Image::Op::Description OCIOColourSpace::description("OCIOColourSpace", build);

const char* OCIOColourSpace::Class() const
{
    return description.name;
}

const char* OCIOColourSpace::displayName() const
{
    return description.name;
}

const char* OCIOColourSpace::node_help() const
{
    // TODO more detailed help text
    return "Use OpenColourIO to convert from one colour space to another.";
}

// This class is necessary in order to call knobsAtTheEnd(). Otherwise, the NukeWrapper knobs 
// will be added to the Context tab instead of the primary tab.
class OCIOColourSpaceNukeWrapper : public DD::Image::NukeWrapper
{
public:
    OCIOColourSpaceNukeWrapper(DD::Image::PixelIop* op) : DD::Image::NukeWrapper(op)
    {
    }
    
    virtual void attach()
    {
        wrapped_iop()->attach();
    }
    
    virtual void detach()
    {
        wrapped_iop()->detach();
    }
    
    virtual void knobs(DD::Image::Knob_Callback f)
    {
        OCIOColourSpace* csIop = dynamic_cast<OCIOColourSpace*>(wrapped_iop());
        if(!csIop) return;
        
        DD::Image::NukeWrapper::knobs(f);
        
        DD::Image::Tab_knob(f, "Context");
        {
            DD::Image::String_knob(f, &csIop->m_contextKey1, "key1");
            DD::Image::Spacer(f, 10);
            DD::Image::String_knob(f, &csIop->m_contextValue1, "value1");
            DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE);

            DD::Image::String_knob(f, &csIop->m_contextKey2, "key2");
            DD::Image::Spacer(f, 10);
            DD::Image::String_knob(f, &csIop->m_contextValue2, "value2");
            DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE);

            DD::Image::String_knob(f, &csIop->m_contextKey3, "key3");
            DD::Image::Spacer(f, 10);
            DD::Image::String_knob(f, &csIop->m_contextValue3, "value3");
            DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE);

            DD::Image::String_knob(f, &csIop->m_contextKey4, "key4");
            DD::Image::Spacer(f, 10);
            DD::Image::String_knob(f, &csIop->m_contextValue4, "value4");
            DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE);
        }
    }
};

static DD::Image::Op* build(Node *node)
{
    DD::Image::NukeWrapper *op = (new OCIOColourSpaceNukeWrapper(new OCIOColourSpace(node)));
    op->channels(DD::Image::Mask_RGB);
    return op;
}
