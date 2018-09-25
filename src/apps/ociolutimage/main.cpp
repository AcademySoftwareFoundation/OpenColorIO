/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;
OCIO_NAMESPACE_USING;

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
#if (OIIO_VERSION < 10100)
namespace OIIO = OIIO_NAMESPACE;
#endif

#include "argparse.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <sstream>

#include "pystring.h"

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
        else if(getenv("OCIO"))
        {
            config = OCIO::Config::CreateFromEnv();
        }
        else
        {
            std::ostringstream os;
            os << "You must specify an ocio configuration ";
            os << "(either with --config or $OCIO).";
            throw Exception(os.str().c_str());
        }
        
        OCIO::ConstProcessorRcPtr processor = 
            config->getProcessor(incolorspace.c_str(), outcolorspace.c_str());
       
        OCIO::PackedImageDesc imgdesc(&img[0], width, height, 3);
        processor->apply(imgdesc);
    }

    OIIO::ImageOutput* f = OIIO::ImageOutput::create(outputfile);
    if(!f)
    {
        throw Exception( "Could not create output image.");
    }
    
    OIIO::ImageSpec spec(width, height, numchannels, OIIO::TypeDesc::TypeFloat);
    
    // TODO: If DPX, force 16-bit output?
    f->open(outputfile, spec);
    f->write_image(OIIO::TypeDesc::FLOAT, &img[0]);
    f->close();
    delete f;
}


void Extract(int cubesize, int maxwidth,
             const std::string & inputfile,
             const std::string & outputfile)
{
    // Read the image
    OIIO::ImageInput* f = OIIO::ImageInput::create(inputfile);
    if(!f)
    {
        throw Exception("Could not create input image.");
    }
    
    OIIO::ImageSpec spec;
    f->open(inputfile, spec);
    
    std::string error = f->geterror();
    if(!error.empty())
    {
        std::ostringstream os;
        os << "Error loading image " << error;
        throw Exception(os.str().c_str());
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
        throw Exception(os.str().c_str());
    }
    
    if(spec.nchannels<3)
    {
        throw Exception("Image must have 3 or more channels.");
    }
    
    int lut3DNumPixels = cubesize*cubesize*cubesize;
    
    if(spec.width*spec.height<lut3DNumPixels)
    {
        throw Exception("Image is not large enough to contain expected 3dlut.");
    }
    
    // TODO: confirm no data window?
    std::vector<float> img;
    img.resize(spec.width*spec.height*spec.nchannels, 0);
    f->read_image(OIIO::TypeDesc::TypeFloat, &img[0]);
    delete f;
    
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
    
    // Write the output lut
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
    ap.options("ociolutimage -- Convert a 3dlut to or from an image\n\n"
               "usage:  ociolutimage [options] <OUTPUTFILE.LUT>\n\n"
               "example:  ociolutimage --generate --output lut.exr\n"
               "example:  ociolutimage --extract --input lut.exr --output output.spi3d\n",
               "<SEPARATOR>", "",
               "--generate", &generate, "Generate a lattice image",
               "--extract", &extract, "Extract a 3dlut from an input image",
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
            std::cerr << "Error extracting lut: " << e.what() << std::endl;
            exit(1);
        }
        catch(...)
        {
            std::cerr << "Error extracting lut. An unknown error occurred.\n";
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
        throw Exception("Cannot generate idenitity 3d lut with less than 3 channels.");
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
        throw Exception("Unknown Lut3DOrder.");
    }
}

void WriteLut3D(const std::string & filename, const float* lutdata, int edgeLen)
{
    if(!pystring::endswith(filename,".spi3d"))
    {
        std::ostringstream os;
        os << "Only .spi3d writing is currently supported. ";
        os << "As a work around, please write a .spi3d file, and then use ";
        os << "ociobakelut for transcoding.";
        throw Exception(os.str().c_str());
    }
    
    std::ofstream output;
    output.open(filename.c_str());
    if(!output.is_open())
    {
        std::ostringstream os;
        os <<  "Error opening " << filename << " for writing.";
        throw Exception(os.str().c_str());
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
