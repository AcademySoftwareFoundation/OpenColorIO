/**
 * OpenColorIO conversion Iop.
 */

#include "LogConvert.h"

namespace OCIO = OCIO_NAMESPACE;

#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>
#include <DDImage/Row.h>
#include <DDImage/Knobs.h>

#include <string>
#include <sstream>
#include <stdexcept>


const char* LogConvert::modes[] = {
    "log to lin", "lin to log", 0
};

LogConvert::LogConvert(Node *n) : DD::Image::PixelIop(n)
{
    modeindex = 0;
    layersToProcess = DD::Image::Mask_RGB;

}

LogConvert::~LogConvert()
{

}

void LogConvert::knobs(DD::Image::Knob_Callback f)
{

    Enumeration_knob(f, &modeindex, modes, "operation", "operation");
    //DD::Image::Tooltip(f, "Input data is taken to be in this colorspace.");
    
    DD::Image::Divider(f);
    
    DD::Image::Input_ChannelSet_knob(f, &layersToProcess, 0, "layer", "layer");
    DD::Image::SetFlags(f, DD::Image::Knob::NO_CHECKMARKS | DD::Image::Knob::NO_ALPHA_PULLDOWN);
    DD::Image::Tooltip(f, "Set which layer to process. This should be a layer with rgb data.");
}

void LogConvert::_validate(bool for_real)
{
    input0().validate(for_real);
    
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        
        if(modeindex == 0)
        {
            processor = config->getProcessor(OCIO::ROLE_SCENE_LINEAR,
                                             OCIO::ROLE_COMPOSITING_LOG);
        }
        else
        {
            processor = config->getProcessor(OCIO::ROLE_COMPOSITING_LOG,
                                             OCIO::ROLE_SCENE_LINEAR);
        }
    }
    catch(OCIO::Exception &e)
    {
        error(e.what());
        return;
    }
    
    if(processor->isNoOp())
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
void LogConvert::in_channels(int /* n unused */, DD::Image::ChannelSet& mask) const
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
void LogConvert::pixel_engine(
    const DD::Image::Row& in,
    int /* rowY unused */, int rowX, int rowXBound,
    const DD::Image::ChannelMask outputChannels,
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
            processor->apply(img);
        }
        catch(OCIO::Exception &e)
        {
            error(e.what());
        }
    }
}

const DD::Image::Op::Description LogConvert::description("OCIOLogConvert", build);

const char* LogConvert::Class() const
{
    return description.name;
}

const char* LogConvert::displayName() const
{
    return description.name;
}

const char* LogConvert::node_help() const
{
    // TODO more detailed help text
    return "Use OpenColorIO to convert from SCENE_LINEAR to COMPOSITING_LOG (or back).";
}


DD::Image::Op* build(Node *node)
{
    DD::Image::NukeWrapper *op = new DD::Image::NukeWrapper(new LogConvert(node));
    op->noMix();
    op->noMask();
    op->noChannels(); // prefer our own channels control without checkboxes / alpha
    op->noUnpremult();
    return op;
}
