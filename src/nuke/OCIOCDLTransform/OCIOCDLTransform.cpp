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
    layersToProcess = DD::Image::Mask_RGB;

    for (int i = 0; i < 3; i++){
        m_slope[i] = 1.0;
        m_offset[i] = 0.0;
        m_power[i] = 1.0;
    }

    m_saturation = 1.0;

    m_dirindex = 0;

    m_cccid = "";
}

OCIOCDLTransform::~OCIOCDLTransform()
{

}

void OCIOCDLTransform::knobs(DD::Image::Knob_Callback f)
{

    // ASC CDL grade numbers
    DD::Image::Color_knob(f, m_slope, DD::Image::IRange(0, 4.0), "slope");
    DD::Image::Color_knob(f, m_offset, DD::Image::IRange(-0.2, 0.2), "offset");
    DD::Image::Color_knob(f, m_power, DD::Image::IRange(0.0, 4.0), "power");
    DD::Image::Float_knob(f, &m_saturation, DD::Image::IRange(0, 4.0), "saturation");

    Enumeration_knob(f, &m_dirindex, dirs, "direction", "direction");
    DD::Image::Tooltip(f, "Specify the transform direction.");

    DD::Image::Divider(f);

    // Cache ID
    DD::Image::String_knob(f, &m_cccid, "cccid", "cccid");
    DD::Image::SetFlags(f, DD::Image::Knob::ENDLINE);

    // Import/export buttons
    DD::Image::PyScript_knob(f, "import ocionuke.cdl; ocionuke.cdl.export_as_cc()", "export_cc", "export grade as .cc");
    DD::Image::Tooltip(f, "Export this grade as a ColorCorrection XML file, which can be loaded with the OCIOFileTransform, or using a FileTransform in an OCIO config");

    DD::Image::PyScript_knob(f, "import ocionuke.cdl; ocionuke.cdl.import_cc_from_xml()", "import_cc", "import from .cc");
    DD::Image::Tooltip(f, "Import grade from a ColorCorrection XML file");

    DD::Image::Divider(f);

    // Layer selection
    DD::Image::Input_ChannelSet_knob(f, &layersToProcess, 0, "layer", "layer");
    DD::Image::SetFlags(f, DD::Image::Knob::NO_CHECKMARKS | DD::Image::Knob::NO_ALPHA_PULLDOWN);
    DD::Image::Tooltip(f, "Set which layer to process. This should be a layer with rgb data.");
}

void OCIOCDLTransform::_validate(bool for_real)
{
    input0().validate(for_real);
    
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        config->sanityCheck();

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
        // TODO or call disable() ?
        set_out_channels(DD::Image::Mask_None); // prevents engine() from being called
        copy_info();
        return;
    }
    
    set_out_channels(DD::Image::Mask_All);

    DD::Image::PixelIop::_validate(for_real);
}

// Note that this is copied by others (OCIODisplay)
void OCIOCDLTransform::in_channels(int /* n unused */, DD::Image::ChannelSet& mask) const
{
    DD::Image::ChannelSet done;
    foreach(c, mask)
    {
        if ((layersToProcess & c) && DD::Image::colourIndex(c) < 3 && !(done & c))
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
        if (!(layersToProcess & requestedChannel) || colourIndex(requestedChannel) >= 3)
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
    op->noMix();
    op->noMask();
    op->noChannels(); // prefer our own channels control without checkboxes / alpha
    op->noUnpremult();
    return op;
}
