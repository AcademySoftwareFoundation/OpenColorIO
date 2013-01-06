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
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
#if (OIIO_VERSION < 10100)
namespace OIIO = OIIO_NAMESPACE;
#endif


#include "argparse.h"

// array of non openimageIO arguments
static std::vector<std::string> args;


// fill 'args' array with openimageIO arguments
static int
parse_end_args(int argc, const char *argv[])
{
  while(argc>0)
  {
    args.push_back(argv[0]);
    argc--;
    argv++;
  }
  
  return 0;
}


bool ParseNameValuePair(std::string& name, std::string& value,
                        const std::string& input);

bool StringToFloat(float * fval, const char * str);

bool StringToInt(int * ival, const char * str);

bool StringToVector(std::vector<int> * ivector, const char * str);

int main(int argc, const char **argv)
{
    ArgParse ap;
    
    std::vector<std::string> floatAttrs;
    std::vector<std::string> intAttrs;
    std::vector<std::string> stringAttrs;
    std::string keepChannels;
    bool croptofull = false;
     
    ap.options("ocioconvert -- apply colorspace transform to an image \n\n"
               "usage: ocioconvert [options]  inputimage inputcolorspace outputimage outputcolorspace\n\n",
               "%*", parse_end_args, "",
               "<SEPARATOR>", "OpenImageIO options",
               "--float-attribute %L", &floatAttrs, "name=float pair defining OIIO float attribute",
               "--int-attribute %L", &intAttrs, "name=int pair defining OIIO int attribute",
               "--string-attribute %L", &stringAttrs, "name=string pair defining OIIO string attribute",
               "--croptofull", &croptofull, "name=Crop or pad to make pixel data region match the \"full\" region",
               "--ch %s", &keepChannels, "name=Select channels (e.g., \"2,3,4\")",
               NULL
               );
    if (ap.parse (argc, argv) < 0) {
        std::cerr << ap.geterror() << std::endl;
        ap.usage ();
        exit(1);
    }

    if(args.size()!=4)
    {
      ap.usage();
      exit(1);
    }
    
    const char * inputimage = args[0].c_str();
    const char * inputcolorspace = args[1].c_str();
    const char * outputimage = args[2].c_str();
    const char * outputcolorspace = args[3].c_str();
    
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
        
        std::vector<int> kchannels;
        //parse --ch argument
        if (keepChannels != "" && !StringToVector(&kchannels,keepChannels.c_str()))
        {
            std::cerr << "Error: --ch: '" << keepChannels << "' should be comma-seperated integers\n";
            exit(1);
        }
        
        //if kchannels not specified, then keep all channels
        if (kchannels.size() == 0)
        {
            kchannels.resize(components);
            for (int channel=0; channel < components; channel++)
            {
                kchannels[channel] = channel;
            }
        }
        
        if (croptofull)
        {
            imgwidth = spec.full_width;
            imgheight = spec.full_height;
            std::cerr << "cropping to " << imgwidth;
            std::cerr << "x" << imgheight << std::endl;
        }
        
        if (croptofull || (int)kchannels.size() < spec.nchannels)
        {
            // crop down bounding box and ditch all but n channels
            // img is a flattened 3 dimensional matrix heightxwidthxchannels
            // fill croppedimg with only the needed pixels
            std::vector<float> croppedimg;
            croppedimg.resize(imgwidth*imgheight*kchannels.size());
            for (int y=0 ; y < spec.height ; y++)
            {
                for (int x=0 ; x < spec.width; x++)
                {
                    for (int k=0; k < (int)kchannels.size(); k++)
                    {
                        int channel = kchannels[k];
                        int current_pixel_y = y + spec.y;
                        int current_pixel_x = x + spec.x;
                        
                        if (current_pixel_y >= 0 &&
                            current_pixel_x >= 0 &&
                            current_pixel_y < imgheight &&
                            current_pixel_x < imgwidth)
                        {
                            // get the value at the desired pixel
                            float current_pixel = img[(y*spec.width*components)
                                                      + (x*components)+channel];
                            // put in croppedimg.
                            croppedimg[(current_pixel_y*imgwidth*kchannels.size())
                                       + (current_pixel_x*kchannels.size())
                                       + channel] = current_pixel;
                        }
                    }
                }
            }
            // redefine the spec so it matches the new bounding box
            spec.x = 0;
            spec.y = 0;
            spec.height = imgheight;
            spec.width = imgwidth;
            spec.nchannels = (int)(kchannels.size());
            components = (int)(kchannels.size());
            img = croppedimg;
        }
    
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
        exit(1);
    }
    catch(...)
    {
        std::cerr << "Unknown OCIO error encountered." << std::endl;
        exit(1);
    }
    
    
        
    //
    // set the provided OpenImageIO attributes
    //
    bool parseerror = false;
    for(unsigned int i=0; i<floatAttrs.size(); ++i)
    {
        std::string name, value;
        float fval = 0.0f;
        
        if(!ParseNameValuePair(name, value, floatAttrs[i]) ||
           !StringToFloat(&fval,value.c_str()))
        {
            std::cerr << "Error: attribute string '" << floatAttrs[i] << "' should be in the form name=floatvalue\n";
            parseerror = true;
            continue;
        }
        
        spec.attribute(name, fval);
    }
    
    for(unsigned int i=0; i<intAttrs.size(); ++i)
    {
        std::string name, value;
        int ival = 0;
        if(!ParseNameValuePair(name, value, intAttrs[i]) ||
           !StringToInt(&ival,value.c_str()))
        {
            std::cerr << "Error: attribute string '" << intAttrs[i] << "' should be in the form name=intvalue\n";
            parseerror = true;
            continue;
        }
        
        spec.attribute(name, ival);
    }
    
    for(unsigned int i=0; i<stringAttrs.size(); ++i)
    {
        std::string name, value;
        if(!ParseNameValuePair(name, value, stringAttrs[i]))
        {
            std::cerr << "Error: attribute string '" << stringAttrs[i] << "' should be in the form name=value\n";
            parseerror = true;
            continue;
        }
        
        spec.attribute(name, value);
    }
   
    if(parseerror)
    {
        exit(1);
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


// Parse name=value parts
// return true on success

bool ParseNameValuePair(std::string& name,
                        std::string& value,
                        const std::string& input)
{
    // split string into name=value 
    size_t pos = input.find('=');
    if(pos==std::string::npos) return false;
    
    name = input.substr(0,pos);
    value = input.substr(pos+1);
    return true;
}

// return true on success
bool StringToFloat(float * fval, const char * str)
{
    if(!str) return false;
    
    std::istringstream inputStringstream(str);
    float x;
    if(!(inputStringstream >> x))
    {
        return false;
    }
    
    if(fval) *fval = x;
    return true;
}

bool StringToInt(int * ival, const char * str)
{
    if(!str) return false;
    
    std::istringstream inputStringstream(str);
    int x;
    if(!(inputStringstream >> x))
    {
        return false;
    }
    
    if(ival) *ival = x;
    return true;
}

bool StringToVector(std::vector<int> * ivector, const char * str)
{
    std::stringstream ss(str);
    int i;
    while (ss >> i)
    {
        ivector->push_back(i);
        if (ss.peek() == ',')
        {
          ss.ignore();
        }
    }
    return ivector->size() != 0;
}




