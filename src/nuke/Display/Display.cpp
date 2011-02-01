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
    colorSpaceIndex = displayIndex = viewIndex = 0;
    displayKnob = viewKnob = NULL;
    exposure = 0.0;
    display_gamma = 1.0;
    
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        std::string defaultColorSpaceName = config->getColorSpace(OCIO::ROLE_SCENE_LINEAR)->getName();
        
        int nColorSpaces = config->getNumColorSpaces();
        
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
        
        int numDisplays = config->getNumDisplays();
        std::string defaultDisplay = config->getDefaultDisplay();
        
        for(int i = 0; i < numDisplays; i++)
        {
            std::string display = config->getDisplay(i);
            displayNames.push_back(display);
            displayCstrNames.push_back(displayNames.back().c_str());
            
            if (display == defaultDisplay)
            {
                displayIndex = i;
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
    displayCstrNames.push_back(NULL);

    hasLists = !(colorSpaceNames.empty() || displayNames.empty() || viewNames.empty());

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

    displayKnob = DD::Image::Enumeration_knob(f,
        &displayIndex, &displayCstrNames[0], "device", "display device");
    DD::Image::SetFlags(f, DD::Image::Knob::SAVE_MENU);
    DD::Image::Tooltip(f, "Display device for output.");

    viewKnob = DD::Image::Enumeration_knob(f,
        &viewIndex, &viewCstrNames[0], "transform", "display transform");
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

        const char * display = displayCstrNames[displayIndex];
        const char * transformName = viewCstrNames[viewIndex];
        const char * csDstName = config->getDisplayColorSpaceName(display, transformName);

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

    if (displayIndex < 0 || displayIndex >= static_cast<int>(displayNames.size()))
    {
        std::ostringstream err;
        err << "No or invalid display specified (index " << displayIndex << ").";
        error(err.str().c_str());
        return;
    }

    const char * display = displayCstrNames[displayIndex];
    int numViews = config->getNumViews(display);
    std::string defaultViewName = config->getDefaultView(display);

    // Try to maintain an old transform name, or use the default.
    bool hasOldViewName = false;
    std::string oldViewName = "";
    if (viewIndex >= 0 && viewIndex < static_cast<int>(viewNames.size()))
    {
        hasOldViewName = true;
        oldViewName = viewNames[viewIndex];
    }
    int defaultViewIndex = 0, newViewIndex = -1;

    viewCstrNames.clear();
    viewNames.clear();

    for(int i = 0; i < numViews; i++)
    {
        std::string view = config->getView(display, i);
        viewNames.push_back(view);
        viewCstrNames.push_back(viewNames.back().c_str());
        if (hasOldViewName && view == oldViewName)
        {
            newViewIndex = i;
        }
        if (view == defaultViewName)
        {
            defaultViewIndex = i;
        }
    }

    if (newViewIndex == -1)
    {
        newViewIndex = defaultViewIndex;
    }

    viewCstrNames.push_back(NULL);

    if (viewKnob == NULL)
    {
        viewIndex = newViewIndex;
    }
    else
    {
        DD::Image::Enumeration_KnobI *enumKnob = viewKnob->enumerationKnob();
        enumKnob->menu(viewNames);
        viewKnob->set_value(newViewIndex);
    }
}

int Display::knob_changed(DD::Image::Knob *k)
{
    if (k == displayKnob && displayKnob != NULL)
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
