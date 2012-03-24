/**
 * OpenColorIO LogConvert Iop.
 */

#include "OCIOLogConvert.h"

namespace OCIO = OCIO_NAMESPACE;

#include <string>
#include <sstream>
#include <stdexcept>

#include <DDImage/Channel.h>
#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>
#include <DDImage/Row.h>
#include <DDImage/Knobs.h>

const char* OCIOLogConvert::modes[] = {
    "log to lin", "lin to log", 0
};

OCIOLogConvert::OCIOLogConvert(Node *n) : DD::Image::PixelIop(n)
{
    modeindex = 0;
}

OCIOLogConvert::~OCIOLogConvert()
{

}

void OCIOLogConvert::knobs(DD::Image::Knob_Callback f)
{

    Enumeration_knob(f, &modeindex, modes, "operation", "operation");
    DD::Image::SetFlags(f, DD::Image::Knob::ALWAYS_SAVE);
}

void OCIOLogConvert::_validate(bool for_real)
{
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        
        const char * src = 0;
        const char * dst = 0;
        
        if(modeindex == 0)
        {
            src = OCIO::ROLE_COMPOSITING_LOG;
            dst = OCIO::ROLE_SCENE_LINEAR;
        }
        else
        {
            src = OCIO::ROLE_SCENE_LINEAR;
            dst = OCIO::ROLE_COMPOSITING_LOG;
        }
        
        processor = config->getProcessor(src, dst);
    }
    catch(OCIO::Exception &e)
    {
        error(e.what());
        return;
    }
    
    if(processor->isNoOp())
    {
        set_out_channels(DD::Image::Mask_None); // prevents engine() from being called
    } else {    
        set_out_channels(DD::Image::Mask_All);
    }

    DD::Image::PixelIop::_validate(for_real);
}

// Note that this is copied by others (OCIODisplay)
void OCIOLogConvert::in_channels(int /* n unused */, DD::Image::ChannelSet& mask) const
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
void OCIOLogConvert::pixel_engine(
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
            processor->apply(img);
        }
        catch(OCIO::Exception &e)
        {
            error(e.what());
        }
    }
}

const DD::Image::Op::Description OCIOLogConvert::description("OCIOLogConvert", build);

const char* OCIOLogConvert::Class() const
{
    return description.name;
}

const char* OCIOLogConvert::displayName() const
{
    return description.name;
}

const char* OCIOLogConvert::node_help() const
{
    // TODO more detailed help text
    return "Use OpenColorIO to convert from SCENE_LINEAR to COMPOSITING_LOG (or back).";
}


DD::Image::Op* build(Node *node)
{
    DD::Image::NukeWrapper *op = new DD::Image::NukeWrapper(new OCIOLogConvert(node));
    op->channels(DD::Image::Mask_RGB);
    return op;
}
