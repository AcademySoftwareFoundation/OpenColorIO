/**
 * OpenColourIO ColourSpace Iop.
 */

#include "OCIOLookTransform.h"

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

OCIOLookTransform::OCIOLookTransform(Node *n) : DD::Image::PixelIop(n)
{
    m_hasColourSpaces = false;

    m_inputColourSpaceIndex = 0;
    m_outputColourSpaceIndex = 0;
    m_dirIndex = 0;
    m_ignoreErrors = false;
    m_reload_version = 1;
    
    // Query the colourspace names from the current config
    // TODO (when to) re-grab the list of available colourspaces? How to save/load?
    
    OCIO::ConstConfigRcPtr config;
    std::string linear;
    
    try
    {
        config = OCIO::GetCurrentConfig();
        
        OCIO::ConstColourSpaceRcPtr linearcs = config->getColourSpace(OCIO::ROLE_SCENE_LINEAR);
        if(!linearcs) throw std::runtime_error("ROLE_SCENE_LINEAR not defined.");
        linear = linearcs->getName();
        
        if(config->getNumLooks()>0)
        {
            m_look = config->getLookNameByIndex(0);
        }
        
        std::ostringstream os;
        os << "Specify the look(s) to apply, as predefined in the OpenColourIO ";
        os << "configuration. This may be the name of a single look, or a ";
        os << "combination of looks using the 'look syntax' (outlined below)\n\n";
        
        std::string firstlook = "a";
        std::string secondlook = "b";
        if(config->getNumLooks()>0)
        {
            os << "Looks: ";
            for(int i = 0; i<config->getNumLooks(); ++i)
            {
                if(i!=0) os << ", ";
                const char * lookname = config->getLookNameByIndex(i);
                os << lookname;
                if(i==0) firstlook = lookname;
                if(i==1) secondlook = lookname;
            }
            os << "\n\n";
        }
        else
        {
            os << "NO LOOKS DEFINED -- ";
            os << "This node cannot be used until looks are added to the OCIO Configuration. ";
            os << "See opencolourio.org for examples.\n\n";
        }
        
        os << "Look Syntax:\n";
        os << "Multiple looks are combined with commas: '";
        os << firstlook << ", " << secondlook << "'\n";
        os << "Direction is specified with +/- prefixes: '";
        os << "+" << firstlook << ", -" << secondlook << "'\n";
        os << "Missing look 'fallbacks' specified with |: '";
        os << firstlook << ", -" << secondlook << " | -" << secondlook << "'";
        m_lookhelp = os.str();
    }
    catch (const OCIO::Exception& e)
    {
        std::cerr << "OCIOLookTransform: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "OCIOLookTransform: Unknown exception during OCIO setup." << std::endl;
    }
    
    if(!config)
    {
        m_hasColourSpaces = false;
        return;
    }
    
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
        if(csname == linear)
        {
            m_inputColourSpaceIndex = i;
            m_outputColourSpaceIndex = i;
        }
    }
    
    
    // Step 2: Create a cstr array for passing to Nuke
    // (This must be done in a second pass, lest the original strings be reallocated)
    
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
        std::cerr << "OCIOLookTransform: No ColourSpaces available for input and/or output." << std::endl;
    }
}

OCIOLookTransform::~OCIOLookTransform()
{

}

namespace
{
    static const char * directions[] = { "forward", "inverse", 0 };
}

void OCIOLookTransform::knobs(DD::Image::Knob_Callback f)
{
#ifdef OCIO_CASCADE
    DD::Image::CascadingEnumeration_knob(f,
        &m_inputColourSpaceIndex, &m_inputColourSpaceCstrNames[0], "in_colourspace", "in");
#else
    DD::Image::Enumeration_knob(f,
        &m_inputColourSpaceIndex, &m_inputColourSpaceCstrNames[0], "in_colourspace", "in");
#endif
    DD::Image::SetFlags(f, DD::Image::Knob::ALWAYS_SAVE);
    DD::Image::Tooltip(f, "Input data is taken to be in this colourspace.");
    
    DD::Image::String_knob(f, &m_look, "look");
    DD::Image::Tooltip(f, m_lookhelp.c_str());
    DD::Image::SetFlags(f, DD::Image::Knob::ALWAYS_SAVE);
    
    DD::Image::Spacer(f, 8);
    
    Enumeration_knob(f, &m_dirIndex, directions, "direction", "direction");
    DD::Image::Tooltip(f, "Specify the look transform direction. in/out colourspace handling is not affected.");
    DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE );
    
    // Reload button, and hidden "version" knob to invalidate cache on reload
    DD::Image::Spacer(f, 8);
    
    Button(f, "reload", "reload");
    DD::Image::Tooltip(f, "Reload all files used in the underlying Look(s).");
    Int_knob(f, &m_reload_version, "version");
    DD::Image::SetFlags(f, DD::Image::Knob::HIDDEN);
    
#ifdef OCIO_CASCADE
    DD::Image::CascadingEnumeration_knob(f,
        &m_outputColourSpaceIndex, &m_outputColourSpaceCstrNames[0], "out_colourspace", "out");
#else
    DD::Image::Enumeration_knob(f,
        &m_outputColourSpaceIndex, &m_outputColourSpaceCstrNames[0], "out_colourspace", "out");
#endif
    DD::Image::SetFlags(f, DD::Image::Knob::ALWAYS_SAVE);
    DD::Image::Tooltip(f, "Image data is converted to this colourspace for output.");
    
    
    DD::Image::Bool_knob(f, &m_ignoreErrors, "ignore_errors", "ignore errors");
    DD::Image::Tooltip(f, "If enabled, looks that cannot find the specified correction"
                          " are treated as a normal ColourSpace conversion instead of triggering a render error.");
    DD::Image::SetFlags(f, DD::Image::Knob::STARTLINE );
    
}

OCIO::ConstContextRcPtr OCIOLookTransform::getLocalContext()
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

void OCIOLookTransform::append(DD::Image::Hash& nodehash)
{
    // Incremented to force reloading after rereading the LUT file
    nodehash.append(m_reload_version);
    
    // TODO: Hang onto the context, what if getting it
    // (and querying getCacheID) is expensive?
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        OCIO::ConstContextRcPtr context = getLocalContext();
        std::string configCacheID = config->getCacheID(context);
        nodehash.append(configCacheID);
    }
    catch(const OCIO::Exception &e)
    {
        error(e.what());
    }
    catch (...)
    {
        error("OCIOLookTransform: Unknown exception during hash generation.");
    }
}


int OCIOLookTransform::knob_changed(DD::Image::Knob* k)
{
    if(k->is("reload"))
    {
        knob("version")->set_value(m_reload_version+1);
        OCIO::ClearAllCaches();

        return true; // ensure callback is triggered again
    }

    // Return zero to avoid callbacks for other knobs
    return false;
}


void OCIOLookTransform::_validate(bool for_real)
{
    if(!m_hasColourSpaces)
    {
        error("No colourspaces available for input and/or output.");
        return;
    }

    int inputColourSpaceCount = static_cast<int>(m_inputColourSpaceCstrNames.size()) - 1;
    if(m_inputColourSpaceIndex < 0 || m_inputColourSpaceIndex >= inputColourSpaceCount)
    {
        std::ostringstream err;
        err << "Input colourspace index (" << m_inputColourSpaceIndex << ") out of range.";
        error(err.str().c_str());
        return;
    }

    int outputColourSpaceCount = static_cast<int>(m_outputColourSpaceCstrNames.size()) - 1;
    if(m_outputColourSpaceIndex < 0 || m_outputColourSpaceIndex >= outputColourSpaceCount)
    {
        std::ostringstream err;
        err << "Output colourspace index (" << m_outputColourSpaceIndex << ") out of range.";
        error(err.str().c_str());
        return;
    }

    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        
        const char * inputName = config->getColourSpaceNameByIndex(m_inputColourSpaceIndex);
        const char * outputName = config->getColourSpaceNameByIndex(m_outputColourSpaceIndex);
        
        OCIO::LookTransformRcPtr transform = OCIO::LookTransform::Create();
        transform->setLooks(m_look.c_str());
        
        OCIO::ConstContextRcPtr context = getLocalContext();
        OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_UNKNOWN;
        bool invertTransform = (m_dirIndex == 0) ? false : true;
        
        // Forward
        if(!invertTransform)
        {
            transform->setSrc(inputName);
            transform->setDst(outputName);
            direction = OCIO::TRANSFORM_DIR_FORWARD;
        }
        else
        {
            // The TRANSFORM_DIR_INVERSE applies an inverse for the end-to-end transform,
            // which would otherwise do dst->inv look -> src.
            // This is an unintuitive result for the artist (who would expect in, out to
            // remain unchanged), so we account for that here by flipping src/dst
            
            transform->setSrc(outputName);
            transform->setDst(inputName);
            direction = OCIO::TRANSFORM_DIR_INVERSE;
        }
        
        try
        {
            m_processor = config->getProcessor(context, transform, direction);
        }
        // We only catch the exceptions for missing files, and try to succeed
        // in this case. All other errors represent more serious problems and
        // should fail through.
        catch(const OCIO::ExceptionMissingFile &e)
        {
            if(!m_ignoreErrors) throw;
            m_processor = config->getProcessor(context, inputName, outputName);
        }
    }
    catch(const OCIO::Exception &e)
    {
        error(e.what());
        return;
    }
    catch (...)
    {
        error("OCIOLookTransform: Unknown exception during _validate.");
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
void OCIOLookTransform::in_channels(int /* n unused */, DD::Image::ChannelSet& mask) const
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
void OCIOLookTransform::pixel_engine(
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
        catch(const OCIO::Exception &e)
        {
            error(e.what());
        }
        catch (...)
        {
            error("OCIOLookTransform: Unknown exception during pixel_engine.");
        }
    }
}

const DD::Image::Op::Description OCIOLookTransform::description("OCIOLookTransform", build);

const char* OCIOLookTransform::Class() const
{
    return description.name;
}

const char* OCIOLookTransform::displayName() const
{
    return description.name;
}

const char* OCIOLookTransform::node_help() const
{
    static const char * help = "OpenColourIO LookTransform\n\n"
    "A 'look' is a named colour transform, intended to modify the look of an "
    "image in a 'creative' manner (as opposed to a colourspace definion which "
    "tends to be technically/mathematically defined).\n\n"
    "Examples of looks may be a neutral grade, to be applied to film scans "
    "prior to VFX work, or a per-shot DI grade decided on by the director, "
    "to be applied just before the viewing transform.\n\n"
    "OCIOLooks must be predefined in the OpenColourIO configuration before usage, "
    "and often reference per-shot/sequence LUTs/CCs.\n\n"
    "See the look knob for further syntax details.\n\n"
    "See opencolourio.org for look configuration customization examples.";
    return help;
}

// This class is necessary in order to call knobsAtTheEnd(). Otherwise, the NukeWrapper knobs 
// will be added to the Context tab instead of the primary tab.
class OCIOLookTransformNukeWrapper : public DD::Image::NukeWrapper
{
public:
    OCIOLookTransformNukeWrapper(DD::Image::PixelIop* op) : DD::Image::NukeWrapper(op)
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
        OCIOLookTransform* lookIop = dynamic_cast<OCIOLookTransform*>(wrapped_iop());
        if(!lookIop) return;
        
        DD::Image::NukeWrapper::knobs(f);
        
        DD::Image::Tab_knob(f, "Context");
        {
            DD::Image::String_knob(f, &lookIop->m_contextKey1, "key1");
            DD::Image::Spacer(f, 10);
            DD::Image::String_knob(f, &lookIop->m_contextValue1, "value1");
            DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE);

            DD::Image::String_knob(f, &lookIop->m_contextKey2, "key2");
            DD::Image::Spacer(f, 10);
            DD::Image::String_knob(f, &lookIop->m_contextValue2, "value2");
            DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE);

            DD::Image::String_knob(f, &lookIop->m_contextKey3, "key3");
            DD::Image::Spacer(f, 10);
            DD::Image::String_knob(f, &lookIop->m_contextValue3, "value3");
            DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE);

            DD::Image::String_knob(f, &lookIop->m_contextKey4, "key4");
            DD::Image::Spacer(f, 10);
            DD::Image::String_knob(f, &lookIop->m_contextValue4, "value4");
            DD::Image::ClearFlags(f, DD::Image::Knob::STARTLINE);
        }
    }
};

static DD::Image::Op* build(Node *node)
{
    DD::Image::NukeWrapper *op = (new OCIOLookTransformNukeWrapper(new OCIOLookTransform(node)));
    op->channels(DD::Image::Mask_RGB);
    return op;
}
