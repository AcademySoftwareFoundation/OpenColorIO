#include "Display.h"

#include <algorithm>
#include <string>
#include <sstream>
#include <stdexcept>

#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>
#include <DDImage/Row.h>
#include <DDImage/Knobs.h>
#include <DDImage/Enumeration_KnobI.h>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

Display::Display(Node *n) : DD::Image::PixelIop(n)
{
    layersToProcess = DD::Image::Mask_RGB;
    hasLists = false;
    colorSpaceIndex = displayDeviceIndex = displayTransformIndex = 0;
    displayDeviceKnob = displayTransformKnob = NULL;
    exposure = 0.0;
    display_gamma = 1.0;
    
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        std::string defaultColorSpaceName = config->getColorSpace(OCIO::ROLE_SCENE_LINEAR)->getName();
        std::string defaultDeviceName = config->getDefaultDisplayDeviceName();
        
        int nColorSpaces = config->getNumColorSpaces();
        int nDeviceNames = config->getNumDisplayDeviceNames();

        for(int i = 0; i < nColorSpaces; i++)
        {
            std::string csname = config->getColorSpaceNameByIndex(i);
            colorSpaceNames.push_back(csname);
            
            colorSpaceCstrNames.push_back(colorSpaceNames.back().c_str());
            
            if (defaultColorSpaceName == csname)
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
        
        refreshDisplayTransforms();
        
        transformPtr = OCIO::DisplayTransform::Create();
    }
    catch (OCIO::Exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown exception during OCIO setup." << std::endl;
    }

    colorSpaceCstrNames.push_back(NULL);
    displayDeviceCstrNames.push_back(NULL);

    hasLists = !(colorSpaceNames.empty() || displayDeviceNames.empty() || displayTransformNames.empty());

    if(!hasLists)
    {
        std::cerr << "Missing one or more of colorspaces, display devices, or display transforms." << std::endl;
    }
}

Display::~Display()
{

}

void Display::knobs(DD::Image::Knob_Callback f)
{
    DD::Image::Enumeration_knob(f,
        &colorSpaceIndex, &colorSpaceCstrNames[0], "colorspace", "input colorspace");
    DD::Image::SetFlags(f, DD::Image::Knob::SAVE_MENU);
    DD::Image::Tooltip(f, "Input data is taken to be in this colorspace.");

    displayDeviceKnob = DD::Image::Enumeration_knob(f,
        &displayDeviceIndex, &displayDeviceCstrNames[0], "device", "display device");
    DD::Image::SetFlags(f, DD::Image::Knob::SAVE_MENU);
    DD::Image::Tooltip(f, "Display device for output.");

    displayTransformKnob = DD::Image::Enumeration_knob(f,
        &displayTransformIndex, &displayTransformCstrNames[0], "transform", "display transform");
    DD::Image::SetFlags(f, DD::Image::Knob::SAVE_MENU);
    DD::Image::Tooltip(f, "Display transform for output.");

    DD::Image::Double_knob(f, &exposure, DD::Image::IRange(-4, 4), "exposure", "exposure");
    DD::Image::Tooltip(f, "Adjust the exposure, in f-stops, of the image in scene-linear.");

    DD::Image::Double_knob(f, &display_gamma, DD::Image::IRange(0.0, 4.0), "display_gamma", "display gamma");
    DD::Image::Tooltip(f, "Gamma correction, applied after the display transform.");

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
        config->sanityCheck();

        const char *csSrcName = colorSpaceCstrNames[colorSpaceIndex];

        const char *deviceName = displayDeviceCstrNames[displayDeviceIndex];
        const char *transformName = displayTransformCstrNames[displayTransformIndex];
        const char *csDstName = config->getDisplayColorSpaceName(deviceName, transformName);

        transformPtr->setInputColorSpaceName(csSrcName);
        transformPtr->setDisplayColorSpaceName(csDstName);
        
        // Specify an (optional) linear color correction
        {
            float e = static_cast<float>(exposure);
            float gain = powf(2.0f,e);
            const float slope3f[] = { gain, gain, gain };
            OCIO::CDLTransformRcPtr cc =  OCIO::CDLTransform::Create();
            cc->setSlope(slope3f);
            transformPtr->setLinearCC(cc);
        }
        
        // Specify an (optional) post-display transform.
        {
            float exponent = 1.0f/std::max(1e-6f, static_cast<float>(display_gamma));
            const float exponent4f[] = { exponent, exponent, exponent, exponent };
            OCIO::ExponentTransformRcPtr cc =  OCIO::ExponentTransform::Create();
            cc->setValue(exponent4f);
            transformPtr->setDisplayCC(cc);
        }
        
        processorPtr = config->getProcessor(transformPtr, OCIO::TRANSFORM_DIR_FORWARD);
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

// Note: Same as OCIO ColorSpace::in_channels.
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

// Note: Same as OCIO ColorSpace::pixel_engine.
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

void Display::refreshDisplayTransforms()
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

    if (displayDeviceIndex < 0 || displayDeviceIndex >= static_cast<int>(displayDeviceNames.size()))
    {
        std::ostringstream err;
        err << "No or invalid display device specified (index " << displayDeviceIndex << ").";
        error(err.str().c_str());
        return;
    }

    const char *deviceName = displayDeviceCstrNames[displayDeviceIndex];
    int nTransformNames = config->getNumDisplayTransformNames(deviceName);
    std::string defaultTransformName = config->getDefaultDisplayTransformName(deviceName);

    // Try to maintain an old transform name, or use the default.
    bool hasOldTransformName = false;
    std::string oldTransformName = "";
    if (displayTransformIndex >= 0 && displayTransformIndex < static_cast<int>(displayTransformNames.size()))
    {
        hasOldTransformName = true;
        oldTransformName = displayTransformNames[displayTransformIndex];
    }
    int defaultDisplayTransformIndex = 0, newDisplayTransformIndex = -1;

    displayTransformCstrNames.clear();
    displayTransformNames.clear();

    for(int i = 0; i < nTransformNames; i++)
    {
        std::string transformName = config->getDisplayTransformName(deviceName, i);
        displayTransformNames.push_back(transformName);
        displayTransformCstrNames.push_back(displayTransformNames.back().c_str());
        if (hasOldTransformName && transformName == oldTransformName)
        {
            newDisplayTransformIndex = i;
        }
        if (transformName == defaultTransformName)
        {
            defaultDisplayTransformIndex = i;
        }
    }

    if (newDisplayTransformIndex == -1)
    {
        newDisplayTransformIndex = defaultDisplayTransformIndex;
    }

    displayTransformCstrNames.push_back(NULL);

    if (displayTransformKnob == NULL)
    {
        displayTransformIndex = newDisplayTransformIndex;
    }
    else
    {
        DD::Image::Enumeration_KnobI *enumKnob = displayTransformKnob->enumerationKnob();
        enumKnob->menu(displayTransformNames);
        displayTransformKnob->set_value(newDisplayTransformIndex);
    }
}

int Display::knob_changed(DD::Image::Knob *k)
{
    if (k == displayDeviceKnob && displayDeviceKnob != NULL)
    {
        refreshDisplayTransforms();
        return 1;
    }
    else
    {
        return 0;
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
