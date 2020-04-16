// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;


#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>

#include "apputils/argparse.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <sstream>

#include "utils/StringUtils.h"

enum Lut3DOrder
{
    LUT3DORDER_FAST_RED = 0,
    LUT3DORDER_FAST_BLUE
};

void WriteLut3D(const std::string & filename, const float* lutdata, int edgeLen);
void GenerateIdentityLut3D(float* img, int edgeLen, int numChannels, Lut3DOrder lut3DOrder);


void GetLutImageSize(int & width, int & height,
                     int cubesize, int maxwidth)
{
    // Compute the image width / height
    width = cubesize*cubesize;
    if(maxwidth>0 && width>=maxwidth)
    {
        // TODO: Do something smarter here to find a better multiple,
        // to create a more pleasing gradient rendition.
        // Use prime divisors / lowest common denominator, if possible?
        width = std::min(maxwidth, width);
    }

    int numpixels = cubesize*cubesize*cubesize;
    height = (int)(ceilf((float)numpixels/(float)width));
}


void Generate(int cubesize, int maxwidth,
              const std::string & outputfile,
              const std::string & configfile,
              const std::string & incolorspace,
              const std::string & outcolorspace)
{
    int width = 0;
    int height = 0;
    int numchannels = 3;
    GetLutImageSize(width, height, cubesize, maxwidth);

    std::vector<float> img;
    img.resize(width*height*numchannels, 0);

    GenerateIdentityLut3D(&img[0], cubesize, numchannels, LUT3DORDER_FAST_RED);

    if(!incolorspace.empty() || !outcolorspace.empty())
    {
        OCIO::ConstConfigRcPtr config = OCIO::Config::Create();
        if(!configfile.empty())
        {
            config = OCIO::Config::CreateFromFile(configfile.c_str());
        }
        else if(OCIO::GetEnvVariable("OCIO"))
        {
            config = OCIO::Config::CreateFromEnv();
        }
        else
        {
            std::ostringstream os;
            os << "You must specify an OCIO configuration ";
            os << "(either with --config or $OCIO).";
            throw OCIO::Exception(os.str().c_str());
        }

        OCIO::ConstCPUProcessorRcPtr processor =
            config->getProcessor(incolorspace.c_str(),
                                 outcolorspace.c_str())->getDefaultCPUProcessor();

        OCIO::PackedImageDesc imgdesc(&img[0], width, height, 3);
        processor->apply(imgdesc);
    }

#if OIIO_VERSION < 10903
    OIIO::ImageOutput* f = OIIO::ImageOutput::create(outputfile);
#else
    auto f = OIIO::ImageOutput::create(outputfile);
#endif
    if(!f)
    {
        throw OCIO::Exception( "Could not create output image.");
    }

    OIIO::ImageSpec spec(width, height, numchannels, OIIO::TypeDesc::TypeFloat);

    // TODO: If DPX, force 16-bit output?
    f->open(outputfile, spec);
    const bool ok = f->write_image(OIIO::TypeDesc::FLOAT, &img[0]);
    if(!ok)
    {
        std::stringstream ss;
        ss << "Error writing \"" << outputfile << "\" : " << f->geterror() << "\n";
        throw OCIO::Exception(ss.str().c_str());
    }

    f->close();
#if OIIO_VERSION < 10903
    OIIO::ImageOutput::destroy(f);
#endif
}


void Extract(int cubesize, int maxwidth,
             const std::string & inputfile,
             const std::string & outputfile)
{
    // Read the image
#if OIIO_VERSION < 10903
    OIIO::ImageInput* f = OIIO::ImageInput::create(inputfile);
#else
    auto f = OIIO::ImageInput::create(inputfile);
#endif
    if(!f)
    {
        throw OCIO::Exception("Could not create input image.");
    }

    OIIO::ImageSpec spec;
    f->open(inputfile, spec);

    std::string error = f->geterror();
    if(!error.empty())
    {
        std::ostringstream os;
        os << "Error loading image " << error;
        throw OCIO::Exception(os.str().c_str());
    }

    int width = 0;
    int height = 0;
    GetLutImageSize(width, height, cubesize, maxwidth);

    if(spec.width != width || spec.height != height)
    {
        std::ostringstream os;
        os << "Image does not have expected dimensions. ";
        os << "Expected " << width << "x" << height << ", ";
        os << "Found " << spec.width << "x" << spec.height;
        throw OCIO::Exception(os.str().c_str());
    }

    if(spec.nchannels<3)
    {
        throw OCIO::Exception("Image must have 3 or more channels.");
    }

    int lut3DNumPixels = cubesize*cubesize*cubesize;

    if(spec.width*spec.height<lut3DNumPixels)
    {
        throw OCIO::Exception("Image is not large enough to contain expected 3D LUT.");
    }

    // TODO: confirm no data window?
    std::vector<float> img;
    img.resize(spec.width*spec.height*spec.nchannels, 0);
    const bool ok = f->read_image(OIIO::TypeDesc::FLOAT, &img[0]);
    if(!ok)
    {
        std::stringstream ss;
        ss << "Error reading \"" << inputfile << "\" : " << f->geterror() << "\n";
        throw OCIO::Exception(ss.str().c_str());
    }

#if OIIO_VERSION < 10903
    OIIO::ImageInput::destroy(f);
#endif

    // Repack into rgb
    // Convert the RGB[...] image to an RGB image, in place.
    // Of course, this only works because we're doing it from left to right
    // so old pixels are read before they're written over

    if(spec.nchannels > 3)
    {
        for(int i=0; i<lut3DNumPixels; ++i)
        {
            img[3*i+0] = img[spec.nchannels*i+0];
            img[3*i+1] = img[spec.nchannels*i+1];
            img[3*i+2] = img[spec.nchannels*i+2];
        }
    }

    img.resize(lut3DNumPixels*3);

    // Write the output LUT
    WriteLut3D(outputfile, &img[0], cubesize);
}



int main (int argc, const char* argv[])
{
    bool generate = false;
    bool extract = false;
    int cubesize = 32;
    int maxwidth = 2048;
    std::string inputfile;
    std::string outputfile;
    std::string config;
    std::string incolorspace;
    std::string outcolorspace;

    // TODO: Add optional allocation transform instead of colorconvert
    ArgParse ap;
    ap.options("ociolutimage -- Convert a 3D LUT to or from an image\n\n"
               "usage:  ociolutimage [options] <OUTPUTFILE.LUT>\n\n"
               "example:  ociolutimage --generate --output lut.exr\n"
               "example:  ociolutimage --extract --input lut.exr --output output.spi3d\n",
               "<SEPARATOR>", "",
               "--generate", &generate, "Generate a lattice image",
               "--extract", &extract, "Extract a 3D LUT from an input image",
               "<SEPARATOR>", "",
               "--cubesize %d", &cubesize, "Size of the cube (default: 32)",
               "--maxwidth %d", &maxwidth, "Specify maximum width of the image (default: 2048)",
               "--input %s", &inputfile, "Specify the input filename",
               "--output %s", &outputfile, "Specify the output filename",
               "<SEPARATOR>", "",
               "--config %s", &config, ".ocio configuration file (default: $OCIO)",
               "--colorconvert %s %s", &incolorspace, &outcolorspace, "Apply a color space conversion to the image.",
               NULL);

    if (ap.parse(argc, argv) < 0)
    {
        std::cout << ap.geterror() << std::endl;
        ap.usage();
        std::cout << "\n";
        return 1;
    }

    if (argc == 1 )
    {
        ap.usage();
        std::cout << "\n";
        return 1;
    }

    if(generate)
    {
        try
        {
            Generate(cubesize, maxwidth,
                     outputfile,
                     config, incolorspace, outcolorspace);
        }
        catch(std::exception & e)
        {
            std::cerr << "Error generating image: " << e.what() << std::endl;
            exit(1);
        }
        catch(...)
        {
            std::cerr << "Error generating image. An unknown error occurred.\n";
            exit(1);
        }
    }
    else if(extract)
    {
        try
        {
            Extract(cubesize, maxwidth,
                    inputfile, outputfile);
        }
        catch(std::exception & e)
        {
            std::cerr << "Error extracting LUT: " << e.what() << std::endl;
            exit(1);
        }
        catch(...)
        {
            std::cerr << "Error extracting LUT. An unknown error occurred.\n";
            exit(1);
        }
    }
    else
    {
        std::cerr << "Must specify either --generate or --extract.\n";
        exit(1);
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// TODO: These should be exposed from inside OCIO, in appropriate time.
//

inline int GetLut3DIndex_RedFast(int indexR, int indexG, int indexB,
                                 int sizeR,  int sizeG,  int /*sizeB*/)
{
    return 3 * (indexR + sizeR * (indexG + sizeG * indexB));
}

void GenerateIdentityLut3D(float* img, int edgeLen, int numChannels,
                           Lut3DOrder lut3DOrder)
{
    if(!img) return;
    if(numChannels < 3)
    {
        throw OCIO::Exception("Cannot generate identity 3D LUT with less than 3 channels.");
    }

    float c = 1.0f / ((float)edgeLen - 1.0f);

    if(lut3DOrder == LUT3DORDER_FAST_RED)
    {
        for(int i=0; i<edgeLen*edgeLen*edgeLen; i++)
        {
            img[numChannels*i+0] = (float)(i%edgeLen) * c;
            img[numChannels*i+1] = (float)((i/edgeLen)%edgeLen) * c;
            img[numChannels*i+2] = (float)((i/edgeLen/edgeLen)%edgeLen) * c;
        }
    }
    else if(lut3DOrder == LUT3DORDER_FAST_BLUE)
    {
        for(int i=0; i<edgeLen*edgeLen*edgeLen; i++)
        {
            img[numChannels*i+0] = (float)((i/edgeLen/edgeLen)%edgeLen) * c;
            img[numChannels*i+1] = (float)((i/edgeLen)%edgeLen) * c;
            img[numChannels*i+2] = (float)(i%edgeLen) * c;
        }
    }
    else
    {
        throw OCIO::Exception("Unknown Lut3DOrder.");
    }
}

void WriteLut3D(const std::string & filename, const float* lutdata, int edgeLen)
{
    if(!StringUtils::EndsWith(filename, ".spi3d"))
    {
        std::ostringstream os;
        os << "Only .spi3d writing is currently supported. ";
        os << "As a work around, please write a .spi3d file, and then use ";
        os << "ociobakelut for transcoding.";
        throw OCIO::Exception(os.str().c_str());
    }

    std::ofstream output;
    output.open(filename.c_str());
    if(!output.is_open())
    {
        std::ostringstream os;
        os <<  "Error opening " << filename << " for writing.";
        throw OCIO::Exception(os.str().c_str());
    }

    output << "SPILUT 1.0\n";
    output << "3 3\n";
    output << edgeLen << " " << edgeLen << " " << edgeLen << "\n";

    int index = 0;
    for(int rindex=0; rindex<edgeLen; ++rindex)
    {
        for(int gindex=0; gindex<edgeLen; ++gindex)
        {
            for(int bindex=0; bindex<edgeLen; ++bindex)
            {
                index = GetLut3DIndex_RedFast(rindex, gindex, bindex,
                                              edgeLen, edgeLen, edgeLen);

                output << rindex << " " << gindex << " " << bindex << " ";
                output << lutdata[index+0] << " ";
                output << lutdata[index+1] << " ";
                output << lutdata[index+2] << "\n";
            }
        }
    }

    output.close();
}
