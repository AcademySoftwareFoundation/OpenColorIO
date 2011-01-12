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

#include <iostream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
namespace OIIO = OIIO_NAMESPACE;

const char * USAGE_TEXT = "\n"
"ocioconvert inputimage inputcolorspace outputimage outputcolorspace\n";

int main(int argc, char **argv)
{
    if(argc<5)
    {
        std::cerr << USAGE_TEXT << std::endl;
        exit(0);
    }
    
    const char * inputimage = argv[1];
    const char * inputcolorspace = argv[2];
    const char * outputimage = argv[3];
    const char * outputcolorspace = argv[4];
    
    OIIO::ImageSpec spec;
    std::vector<float> img;
    int imgwidth = 0;
    int imgheight = 0;
    int components = 0;
    
    // Load the image
    std::cerr << "Loading " << inputimage << std::endl;
    try
    {
        OIIO::ImageInput* f = OIIO::ImageInput::create(inputimage);
        if(!f)
        {
            std::cerr << "Could not create image input." << std::endl;
            exit(1);
        }
        
        f->open(inputimage, spec);
        
        std::string error = f->geterror();
        if(!error.empty())
        {
            std::cerr << "Error loading image " << error << std::endl;
            exit(1);
        }
        
        imgwidth = spec.width;
        imgheight = spec.height;
        components = spec.nchannels;
        
        img.resize(imgwidth*imgheight*components);
        memset(&img[0], 0, imgwidth*imgheight*components*sizeof(float));
        
        f->read_image(OIIO::TypeDesc::TypeFloat, &img[0]);
        delete f;
    
    }
    catch(...)
    {
        std::cerr << "Error loading file.";
        exit(1);
    }
    
    // Process the image
    try
    {
        // Load the current config.
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        
        // Get the processor
        OCIO::ConstProcessorRcPtr processor = config->getProcessor(inputcolorspace, outputcolorspace);
        
        // Wrap the image in a light-weight ImageDescription
        OCIO::PackedImageDesc imageDesc(&img[0], imgwidth, imgheight, components);
        
        // Apply the color transformation (in place)
        processor->apply(imageDesc);
    }
    catch(OCIO::Exception & exception)
    {
        std::cerr << "OCIO Error: " << exception.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Unknown OCIO error encountered." << std::endl;
    }
    
    // Write out the result
    try
    {
        OIIO::ImageOutput* f = OIIO::ImageOutput::create(outputimage);
        if(!f)
        {
            std::cerr << "Could not create output input." << std::endl;
            exit(1);
        }
        
        f->open(outputimage, spec);
        f->write_image(OIIO::TypeDesc::FLOAT, &img[0]);
        f->close();
        delete f;
    }
    catch(...)
    {
        std::cerr << "Error loading file.";
        exit(1);
    }
    
    std::cerr << "Wrote " << outputimage << std::endl;
    
    return 0;
}
