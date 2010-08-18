#include "Display.h"

namespace OCIO = OCIO_NAMESPACE;

#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>
#include <DDImage/Row.h>
#include <DDImage/Knobs.h>

#include <string>
#include <sstream>
#include <stdexcept>



    /*
    Knobs should be:
        input colorspace (string)
        display device (string) - CRT etc
        transform (string) - film, log, etc
        exposure (float or float4) - master or master+rgb
    Object for display transform will be persistent
    */
Display::Display(Node *n) : DD::Image::PixelIop(n)
{
    layersToProcess = DD::Image::Mask_RGB;
    hasLists = false;
    colorSpaceIndex = displayDeviceIndex = displayTransformIndex = 0;

    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

        OCIO::ConstColorSpaceRcPtr defaultColorSpace = \
            config->getColorSpaceForRole(OCIO::ROLE_SCENE_LINEAR);
        std::string defaultDeviceName = config->getDefaultDisplayDeviceName();
        std::string defaultTransformName = config->getDefaultDisplayTransformName(defaultDeviceName.c_str());

        int nColorSpaces = config->getNumColorSpaces();
        int nDeviceNames = config->getNumDisplayDeviceNames();
        int nTransformNames = config->getNumDisplayTransformNames(defaultDeviceName.c_str()); // TODO update on knob change

        for(int i = 0; i < nColorSpaces; i++)
        {
            OCIO::ConstColorSpaceRcPtr colorSpace = config->getColorSpaceByIndex(i);
            colorSpaceNames.push_back(colorSpace->getName());
            colorSpaceCstrNames.push_back(colorSpaceNames.back().c_str());
            if (colorSpace->equals(defaultColorSpace))
            {
                colorSpaceIndex = i;
            }
        }

        for(int i = 0; i < nDeviceNames; i++)
        {
            std::string deviceName = config->getDisplayDeviceName(i);
            displayDeviceNames.push_back(deviceName);
            displayDeviceCstrNames.push_back(displayDeviceNames.back().c_str());
            if (deviceName == defaultDeviceName)
            {
                displayDeviceIndex = i;
            }
        }

        for(int i = 0; i < nTransformNames; i++)
        {
            std::string transformName = config->getDisplayTransformName(defaultDeviceName.c_str(), i); // TODO
            displayTransformNames.push_back(transformName);
            displayTransformCstrNames.push_back(displayTransformNames.back().c_str());
            if (transformName == defaultTransformName)
            {
                displayTransformIndex = i;
            }
        }

        transformPtr = OCIO::DisplayTransform::Create();
    }
    catch (OCIO::Exception& e)
    {
        error(e.what());
    }
    catch (...)
    {
        error("Unknown exception during OCIO setup.");
    }

    colorSpaceCstrNames.push_back(NULL);
    displayDeviceCstrNames.push_back(NULL);
    displayTransformCstrNames.push_back(NULL);

    hasLists = !(colorSpaceNames.empty() || displayDeviceNames.empty() || displayTransformNames.empty());

    if(!hasLists)
    {
        error("Missing one or more of colorspaces, display devices, or display transforms.");
    }
}

Display::~Display()
{

}

void Display::knobs(DD::Image::Knob_Callback f)
{
    DD::Image::Enumeration_knob(f, &colorSpaceIndex, &colorSpaceCstrNames[0], "colorspace", "input colorspace");
    DD::Image::Tooltip(f, "Input data is taken to be in this colorspace.");

    DD::Image::Enumeration_knob(f, &displayDeviceIndex, &displayDeviceCstrNames[0], "device", "display device");
    DD::Image::Tooltip(f, "Display device for output.");

    DD::Image::Enumeration_knob(f, &displayTransformIndex, &displayTransformCstrNames[0], "transform", "display transform");
    DD::Image::Tooltip(f, "Display transform for output.");

    DD::Image::Divider(f);

    DD::Image::Input_ChannelSet_knob(f, &layersToProcess, 0, "layer", "layer");
    DD::Image::SetFlags(f, DD::Image::Knob::NO_CHECKMARKS | DD::Image::Knob::NO_ALPHA_PULLDOWN);
    DD::Image::Tooltip(f, "Set which layer to process. This should be a layer with rgb data.");
}

void Display::_validate(bool for_real)
{
    input0().validate(for_real);

    if(!hasLists)
    {
        error("Missing one or more of colorspaces, display devices, or display transforms.");
        return;
    }

    int nColorSpaces = static_cast<int>(colorSpaceNames.size());
    if(colorSpaceIndex < 0 || colorSpaceIndex >= nColorSpaces)
    {
        std::ostringstream err;
        err << "ColorSpace index (" << colorSpaceIndex << ") out of range.";
        error(err.str().c_str());
        return;
    }

    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

        const char *csSrcName = colorSpaceCstrNames[colorSpaceIndex];

        const char *deviceName = displayDeviceCstrNames[displayDeviceIndex];
        const char *transformName = displayTransformCstrNames[displayTransformIndex];
        const char *csDstName = config->getDisplayColorSpaceName(deviceName, transformName);

        transformPtr->setInputColorSpace(config->getColorSpaceByName(csSrcName));
        transformPtr->setDisplayColorSpace(config->getColorSpaceByName(csDstName));

        processorPtr = config->getProcessor(transformPtr);
    }
    catch(OCIO::Exception &e)
    {
        error(e.what());
        return;
    }
    
    if(processorPtr->isNoOp())
    {
        // TODO or call disable() ?
        set_out_channels(DD::Image::Mask_None); // prevents engine() from being called
        copy_info();
        return;
    }
    
    set_out_channels(DD::Image::Mask_All);

    DD::Image::PixelIop::_validate(for_real);
}

// TODO Same as OCIO ColorSpace::in_channels.
void Display::in_channels(int /* n unused */, DD::Image::ChannelSet& mask) const
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

// TODO Same as OCIO ColorSpace::pixel_engine.
void Display::pixel_engine(
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
            processorPtr->apply(img);
        }
        catch(OCIO::Exception &e)
        {
            error(e.what());
        }
    }
}

const DD::Image::Op::Description Display::description("OCIODisplay", build);

const char* Display::Class() const
{
    return description.name;
}

const char* Display::displayName() const
{
    return description.name;
}

const char* Display::node_help() const
{
    // TODO more detailed help text
    return "Use OpenColorIO to convert for a display device.";
}


DD::Image::Op* build(Node *node)
{
    DD::Image::NukeWrapper *op = new DD::Image::NukeWrapper(new Display(node));
    op->noMix();
    op->noMask();
    op->noChannels(); // prefer our own channels control without checkboxes / alpha
    op->noUnpremult();
    return op;
}
