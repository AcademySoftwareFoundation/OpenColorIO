/**
 * OpenColorIO conversion Iop.
 */

#include "OCIOCDLTransform.h"

namespace OCIO = OCIO_NAMESPACE;

#include <string>
#include <sstream>
#include <stdexcept>

#include <DDImage/Channel.h>
#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>
#include <DDImage/Row.h>
#include <DDImage/Knobs.h>


const char* OCIOCDLTransform::dirs[] = { "forward", "inverse", 0 };

OCIOCDLTransform::OCIOCDLTransform(Node *n) : DD::Image::PixelIop(n)
{
    for (int i = 0; i < 3; i++){
        m_slope[i] = 1.0;
        m_offset[i] = 0.0;
        m_power[i] = 1.0;
    }

    m_saturation = 1.0;
    m_readFromFile = false;
    m_dirindex = 0;
    m_file = NULL;
    m_reload_version = 1;
    
    m_slopeKnob = NULL;
    m_offsetKnob = NULL;
    m_powerKnob = NULL;
    m_saturationKnob = NULL;
    m_fileKnob = NULL;
    m_cccidKnob = NULL;
    
    m_firstLoad = true;
}

OCIOCDLTransform::~OCIOCDLTransform()
{

}

void OCIOCDLTransform::knobs(DD::Image::Knob_Callback f)
{
    // ASC CDL grade numbers
    m_slopeKnob = DD::Image::Color_knob(f, m_slope, DD::Image::IRange(0, 4.0), "slope");
    m_offsetKnob = DD::Image::Color_knob(f, m_offset, DD::Image::IRange(-0.2, 0.2), "offset");
    m_powerKnob = DD::Image::Color_knob(f, m_power, DD::Image::IRange(0.0, 4.0), "power");
    m_saturationKnob = DD::Image::Float_knob(f, &m_saturation, DD::Image::IRange(0, 4.0), "saturation");
    
    Enumeration_knob(f, &m_dirindex, dirs, "direction", "direction");
    DD::Image::Tooltip(f, "Specify the transform direction.");
    
    DD::Image::Divider(f);
    
    DD::Image::Bool_knob(f, &m_readFromFile, "read_from_file", "read from file");
    DD::Image::SetFlags(f, DD::Image::Knob::EARLY_STORE);
    DD::Image::Tooltip(f, "Load color correction information from the .cc or .ccc file.");
    
    m_fileKnob = File_knob(f, &m_file, "file", "file");
    const char * filehelp = "Specify the src ASC CDL file, on disk, to use for this transform. "
    "This can be either a .cc or .ccc file. If .ccc is specified, the cccid is required.";
    DD::Image::Tooltip(f, filehelp);

    // Reload button, and hidden "version" knob to invalidate cache on reload
    Button(f, "reload", "reload");
    DD::Image::Tooltip(f, "Reloads specified files");
    Int_knob(f, &m_reload_version, "version");
    DD::Image::SetFlags(f, DD::Image::Knob::HIDDEN);

    DD::Image::SetFlags(f, DD::Image::Knob::ENDLINE);
    
    m_cccidKnob = String_knob(f, &m_cccid, "cccid");
    const char * ccchelp = "If the source file is an ASC CDL CCC (color correction collection), "
    "this specifies the id to lookup. OpenColorIO::Contexts (envvars) are obeyed.";
    DD::Image::Tooltip(f, ccchelp);
    
    /* TODO:
    These scripts should be updated to make use of native OCIO APIs, rather than convenience functions
    exposed in ocionuke.  Ideally, ocionuke would be a minimal module (essentially only ui code) that makes
    use of OCIO for all heavy lifting.
    */
    
    DD::Image::PyScript_knob(f, "import ocionuke.cdl; ocionuke.cdl.select_cccid_for_filetransform()", "select_cccid", "select cccid");
    
    // Import/export buttons
    DD::Image::PyScript_knob(f, "import ocionuke.cdl; ocionuke.cdl.export_as_cc()", "export_cc", "export grade as .cc");
    DD::Image::Tooltip(f, "Export this grade as a ColorCorrection XML file, which can be loaded with the OCIOFileTransform, or using a FileTransform in an OCIO config");

    DD::Image::PyScript_knob(f, "import ocionuke.cdl; ocionuke.cdl.import_cc_from_xml()", "import_cc", "import from .cc");
    DD::Image::Tooltip(f, "Import grade from a ColorCorrection XML file");
    
    
    DD::Image::Divider(f);
    
    /*
    TODO: One thing thats sucks is that we dont apparently have a mechansism to call refreshKnobEnabledState
    after the knobs have been loaded, but before scripts have a chance to run.  I'd love to have a post-knob
    finalize callback opportunity to reload the cdl from file, if needed.  The current system will only do the
    initial file refresh when either the ui is loaded, or a render is triggered.
    */
    
    if(!f.makeKnobs() && m_firstLoad)
    {
        m_firstLoad = false;
        refreshKnobEnabledState();
        if(m_readFromFile) loadCDLFromFile();
    }
}

void OCIOCDLTransform::refreshKnobEnabledState()
{
    if(m_readFromFile)
    {
        m_slopeKnob->disable();
        m_offsetKnob->disable();
        m_powerKnob->disable();
        m_saturationKnob->disable();
        
        // We leave these active to allow knob re-use with the import/export buttons
        //m_fileKnob->enable();
        //m_cccidKnob->enable();
        
        loadCDLFromFile();
    }
    else
    {
        m_slopeKnob->enable();
        m_offsetKnob->enable();
        m_powerKnob->enable();
        m_saturationKnob->enable();
        
        // We leave these active to allow knob re-use with the import/export buttons
        //m_fileKnob->disable();
        //m_cccidKnob->disable();
    }
}

void OCIOCDLTransform::append(DD::Image::Hash& nodehash)
{
    // There is a bug where in Nuke <6.3 the String_knob (used for
    // cccid) is not included in the node's hash. Include it manually
    // so the node correctly redraws. Appears fixed in in 6.3
    nodehash.append(m_cccid.c_str());

    // Incremented to force reloading after rereading the LUT file
    nodehash.append(m_reload_version);
}

int OCIOCDLTransform::knob_changed(DD::Image::Knob* k)
{
    // return true if you want to continue to receive changes for this knob
    std::string knobname = k->name();
    
    if(knobname == "read_from_file" || knobname == "file" || knobname == "cccid")
    {
        refreshKnobEnabledState();
        
        if(m_readFromFile)
        {
            loadCDLFromFile();
        }
        return true;
    }

    else if(knobname == "reload")
    {
        knob("version")->set_value(m_reload_version+1);
        OCIO::ClearAllCaches();
        m_firstLoad = true;

        return true; // ensure callback is triggered again
    }

    return false;
}

// Check to see if file exists
// read from file
// set knob values
// throw an error if anything goes wrong

void OCIOCDLTransform::loadCDLFromFile()
{
    OCIO::CDLTransformRcPtr transform;
    
    try
    {
        // This is inexpensive to call multiple times, as OCIO caches results
        // internally.
        transform = OCIO::CDLTransform::CreateFromFile(m_file, m_cccid.c_str());
    }
    catch(OCIO::Exception &e)
    {
        error(e.what());
        return;
    }
    
    
    float sop[9];
    transform->getSOP(sop);
    
    m_slopeKnob->clear_animated(-1);
    m_slopeKnob->set_value(sop[0], 0);
    m_slopeKnob->set_value(sop[1], 1);
    m_slopeKnob->set_value(sop[2], 2);
    
    m_offsetKnob->clear_animated(-1);
    m_offsetKnob->set_value(sop[3], 0);
    m_offsetKnob->set_value(sop[4], 1);
    m_offsetKnob->set_value(sop[5], 2);
    
    m_powerKnob->clear_animated(-1);
    m_powerKnob->set_value(sop[6], 0);
    m_powerKnob->set_value(sop[7], 1);
    m_powerKnob->set_value(sop[8], 2);
    
    m_saturationKnob->clear_animated(-1);
    m_saturationKnob->set_value(transform->getSat());
}

void OCIOCDLTransform::_validate(bool for_real)
{
    if(m_firstLoad)
    {
        m_firstLoad = false;
        if(m_readFromFile) loadCDLFromFile();
    }
    
    // We must explicitly refresh the enable state here as well,
    // as there are some changes (such as expressioned knob updates)
    // that wont trigger the knob_changed callback. This allows
    // us to catch these here.
    
    refreshKnobEnabledState();
    
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

        OCIO::CDLTransformRcPtr cc = OCIO::CDLTransform::Create();
        cc->setSlope(m_slope);
        cc->setOffset(m_offset);
        cc->setPower(m_power);
        cc->setSat(m_saturation);

        if(m_dirindex == 0) cc->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
        else cc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

        m_processor = config->getProcessor(cc);
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
void OCIOCDLTransform::in_channels(int /* n unused */, DD::Image::ChannelSet& mask) const
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
void OCIOCDLTransform::pixel_engine(
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

const DD::Image::Op::Description OCIOCDLTransform::description("OCIOCDLTransform", build);

const char* OCIOCDLTransform::Class() const
{
    return description.name;
}

const char* OCIOCDLTransform::displayName() const
{
    return description.name;
}

const char* OCIOCDLTransform::node_help() const
{
    // TODO more detailed help text
    return "Use OpenColorIO to apply an ASC CDL grade. Applied using:\n\n"\
        "out = (i * s + o)^p\n\nWhere i is the input value, s is slope, "\
        "o is offset and p is power";
}


DD::Image::Op* build(Node *node)
{
    DD::Image::NukeWrapper *op = new DD::Image::NukeWrapper(new OCIOCDLTransform(node));
    op->channels(DD::Image::Mask_RGB);
    return op;
}
