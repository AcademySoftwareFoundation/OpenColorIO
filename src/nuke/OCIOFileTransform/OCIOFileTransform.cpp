/**
 * OpenColorIO FileTransform Iop.
 */

#include "OCIOFileTransform.h"

namespace OCIO = OCIO_NAMESPACE;

#include <string>
#include <sstream>
#include <stdexcept>

#include <DDImage/Channel.h>
#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>
#include <DDImage/Row.h>
#include <DDImage/Knobs.h>

OCIOFileTransform::OCIOFileTransform(Node *n) : DD::Image::PixelIop(n)
{
    m_file = NULL;
    m_dirindex = 0;
    m_interpindex = 1;
    m_reload_version = 1;
}

OCIOFileTransform::~OCIOFileTransform()
{

}

const char* OCIOFileTransform::dirs[] = { "forward", "inverse", 0 };

const char* OCIOFileTransform::interp[] = { "nearest", "linear", "tetrahedral", "best", 0 };

void OCIOFileTransform::knobs(DD::Image::Knob_Callback f)
{
    File_knob(f, &m_file, "file", "file");
    DD::Image::Tooltip(f, "Specify the file, on disk, to use for this transform. See the node help for the list of supported formats.");

    // Reload button, and hidden "version" knob to invalidate cache on reload
    Button(f, "reload", "reload");
    DD::Image::Tooltip(f, "Reloads specified files");
    Int_knob(f, &m_reload_version, "version");
    DD::Image::SetFlags(f, DD::Image::Knob::HIDDEN);
    
    String_knob(f, &m_cccid, "cccid");
    const char * srchelp2 = "If the source file is an ASC CDL CCC (color correction collection), "
    "this specifys the id to lookup. OpenColorIO::Contexts (envvars) are obeyed.";
    DD::Image::Tooltip(f, srchelp2);
    
    DD::Image::PyScript_knob(f, "import ocionuke.cdl; ocionuke.cdl.select_cccid_for_filetransform(fileknob='file', cccidknob = 'cccid')", "select_cccid", "select cccid");
    
    Enumeration_knob(f, &m_dirindex, dirs, "direction", "direction");
    DD::Image::Tooltip(f, "Specify the transform direction.");
    
    Enumeration_knob(f, &m_interpindex, interp, "interpolation", "interpolation");
    DD::Image::Tooltip(f, "Specify the interpolation method. For files that are not LUTs (mtx, etc) this is ignored.");
}

void OCIOFileTransform::_validate(bool for_real)
{
    if(!m_file)
    {
        error("The source file must be specified.");
        return;
    }
    
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        
        OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
        transform->setSrc(m_file);
        
        transform->setCCCId(m_cccid.c_str());
        
        if(m_dirindex == 0) transform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
        else transform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
        
        if(m_interpindex == 0) transform->setInterpolation(OCIO::INTERP_NEAREST);
        else if(m_interpindex == 1) transform->setInterpolation(OCIO::INTERP_LINEAR);
        else if(m_interpindex == 2) transform->setInterpolation(OCIO::INTERP_TETRAHEDRAL);
        else if(m_interpindex == 3) transform->setInterpolation(OCIO::INTERP_BEST);
        else
        {
            // Should never happen
            error("Interpolation value out of bounds");
            return;
        }
        
        m_processor = config->getProcessor(transform, OCIO::TRANSFORM_DIR_FORWARD);
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
void OCIOFileTransform::in_channels(int /* n unused */, DD::Image::ChannelSet& mask) const
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

void OCIOFileTransform::append(DD::Image::Hash& nodehash)
{
    // There is a bug where in Nuke <6.3 the String_knob (used for
    // cccid) is not included in the node's hash. Include it manually
    // so the node correctly redraws. Appears fixed in in 6.3
    nodehash.append(m_cccid.c_str());

    // Incremented to force reloading after rereading the LUT file
    nodehash.append(m_reload_version);
}

int OCIOFileTransform::knob_changed(DD::Image::Knob* k)
{
    // Only show the cccid knob when loading a .cc/.ccc file. Set
    // hidden state when the src is changed, or the node properties
    // are shown
    if(k->is("file") | k->is("showPanel"))
    {
        // Convoluted equiv to pysting::endswith(m_file, ".ccc")
        // TODO: Could this be queried from the processor?
        const std::string srcstring = m_file;
        const std::string cccext = "ccc";
        const std::string ccext = "cc";
        if(std::equal(cccext.rbegin(), cccext.rend(), srcstring.rbegin()) ||
           std::equal(ccext.rbegin(), ccext.rend(), srcstring.rbegin()))
        {
            knob("cccid")->show();
            knob("select_cccid")->show();
        }
        else
        {
            knob("cccid")->hide();
            knob("select_cccid")->hide();
        }

        // Ensure this callback is always triggered (for src knob)
        return true;
    }

    if(k->is("reload"))
    {
        knob("version")->set_value(m_reload_version+1);
        OCIO::ClearAllCaches();

        return true; // ensure callback is triggered again
    }

    // Return zero to avoid callbacks for other knobs
    return false;
}

// See Saturation::pixel_engine for a well-commented example.
// Note that this is copied by others (OCIODisplay)
void OCIOFileTransform::pixel_engine(
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

const DD::Image::Op::Description OCIOFileTransform::description("OCIOFileTransform", build);

const char* OCIOFileTransform::Class() const
{
    return description.name;
}

const char* OCIOFileTransform::displayName() const
{
    return description.name;
}

const char* OCIOFileTransform::node_help() const
{
    if(m_nodehelp.empty())
    {
        const char * helptext =
        "Use OpenColorIO to apply a transform loaded from the given "
        "file.\n\n"
        "This is usually a 1D or 3D LUT file, but can be other file-based "
        "transform, for example an ASC ColorCorrection XML file.\n\n"
        "Note that the file's transform is applied with no special "
        "input/output colorspace handling - so if the file expects "
        "log-encoded pixels, but you apply the node to a linear "
        "image, you will get incorrect results.\n\n";
        
        std::ostringstream os;
        os << helptext;
        
        os << "Supported formats:\n";
        for(int i=0; i<OCIO::FileTransform::getNumFormats(); ++i)
        {
            const char* name = OCIO::FileTransform::getFormatNameByIndex(i);
            const char* exten = OCIO::FileTransform::getFormatExtensionByIndex(i);
            os << "\n." << exten << " (" << name << ")";
        }
        
        m_nodehelp = os.str();
    }
    
    return m_nodehelp.c_str();
}


DD::Image::Op* build(Node *node)
{
    DD::Image::NukeWrapper *op = new DD::Image::NukeWrapper(new OCIOFileTransform(node));
    op->channels(DD::Image::Mask_RGB);
    return op;
}
