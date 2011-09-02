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
    src = NULL;
    dirindex = 0;
    interpindex = 1;
    
    layersToProcess = DD::Image::Mask_RGB;
}

OCIOFileTransform::~OCIOFileTransform()
{

}

const char* OCIOFileTransform::dirs[] = { "forward", "inverse", 0 };

const char* OCIOFileTransform::interp[] = { "nearest", "linear", 0 };

void OCIOFileTransform::knobs(DD::Image::Knob_Callback f)
{

    File_knob(f, &src, "src", "src");
    const char * srchelp = "Specify the src file, on disk, to use for this transform. "
    "This can be any file format that OpenColorIO supports: "
    ".3dl, .cc, .ccc, .csp, .cub, .cube, .lut (houdini), .mga, .m3d, .spi1d, .spi3d, .spimtx, .vf";
    DD::Image::Tooltip(f, srchelp);
    
    String_knob(f, &cccid, "cccid");
    const char * srchelp2 = "If the source file is an ASC CDL CCC (color correction collection), "
    "this specifys the id to lookup. OpenColorIO::Contexts (envvars) are obeyed.";
    DD::Image::Tooltip(f, srchelp2);
    
    DD::Image::PyScript_knob(f, "import ocionuke.cdl; ocionuke.cdl.select_cccid_for_filetransform()", "select_cccid", "select cccid");
    
    Enumeration_knob(f, &dirindex, dirs, "direction", "direction");
    DD::Image::Tooltip(f, "Specify the transform direction.");
    
    Enumeration_knob(f, &interpindex, interp, "interpolation", "interpolation");
    DD::Image::Tooltip(f, "Specify the interpolation method. For files that are not LUTs (mtx, etc) this is ignored.");
    
    DD::Image::Divider(f);
    
    DD::Image::Input_ChannelSet_knob(f, &layersToProcess, 0, "layer", "layer");
    DD::Image::SetFlags(f, DD::Image::Knob::NO_CHECKMARKS | DD::Image::Knob::NO_ALPHA_PULLDOWN);
    DD::Image::Tooltip(f, "Set which layer to process. This should be a layer with rgb data.");
}

void OCIOFileTransform::_validate(bool for_real)
{
    input0().validate(for_real);
    
    if(!src)
    {
        error("The source file must be specified.");
        return;
    }
    
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        config->sanityCheck();
        
        OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
        transform->setSrc(src);
        
        transform->setCCCId(cccid.c_str());
        
        if(dirindex == 0) transform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
        else transform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
        
        if(interpindex == 0) transform->setInterpolation(OCIO::INTERP_NEAREST);
        else transform->setInterpolation(OCIO::INTERP_LINEAR);
        
        processor = config->getProcessor(transform, OCIO::TRANSFORM_DIR_FORWARD);
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
void OCIOFileTransform::in_channels(int /* n unused */, DD::Image::ChannelSet& mask) const
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

void OCIOFileTransform::append(DD::Image::Hash& nodehash)
{
    // There is a bug where in Nuke <6.3 the String_knob (used for
    // cccid) is not included in the node's hash. Include it manually
    // so the node correctly redraws. Appears fixed in in 6.3
    nodehash.append(cccid.c_str());
}

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
    // TODO more detailed help text
    return "Use OpenColorIO to apply the specified LUT file transform.";
}


DD::Image::Op* build(Node *node)
{
    DD::Image::NukeWrapper *op = new DD::Image::NukeWrapper(new OCIOFileTransform(node));
    op->noMix();
    op->noMask();
    op->noChannels(); // prefer our own channels control without checkboxes / alpha
    op->noUnpremult();
    return op;
}
