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


#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <GLUT/glut.h>
#elif _WIN32
#include <GL/glew.h>
#include <GL/glut.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#endif
#include "glsl.h"


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

class GPUManagement
{
private:
    GPUManagement()
        : m_glwin(0)
        , m_initState(STATE_CREATED)
        , m_imageTexID(0)
        , m_format(0)
        , m_width(0)
        , m_height(0)
    {
    }

    GPUManagement(const GPUManagement &) = delete;
    GPUManagement & operator=(const GPUManagement &) = delete;

    ~GPUManagement()
    {
        if (m_initState != STATE_CREATED)
        {
            cleanUp();
        }
    }

public:

    static GPUManagement & Instance()
    {
        static GPUManagement inst;
        return inst;
    }

    void init(bool verbose)
    {
        if (m_initState != STATE_CREATED) return;
            
        int argcgl = 2;
        const char* argvgl[] = { "main", "-glDebug" };
        glutInit(&argcgl, const_cast<char**>(&argvgl[0]));

        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
        glutInitWindowSize(10, 10);
        glutInitWindowPosition(0, 0);

        m_glwin = glutCreateWindow(argvgl[0]);

#ifndef __APPLE__
        glewInit();
        if (!glewIsSupported("GL_VERSION_2_0"))
        {
            std::cerr << "OpenGL 2.0 not supported" << std::endl;
            exit(1);
        }
#endif

        if(verbose)
        {
            std::cout << std::endl
                      << "GL Vendor:    " << glGetString(GL_VENDOR) << std::endl
                      << "GL Renderer:  " << glGetString(GL_RENDERER) << std::endl
                      << "GL Version:   " << glGetString(GL_VERSION) << std::endl
                      << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        }

        // Initilize the OpenGL engine
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);           // 4-byte pixel alignment
#ifndef __APPLE__
        glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);     //
        glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);   // avoid any kind of clamping
        glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE); //
#endif

        glEnable(GL_TEXTURE_2D);
        glClearColor(0, 0, 0, 0);                        // background color
        glClearStencil(0);                               // clear stencil buffer

        m_initState = STATE_INITIALIZED;
    }

    void prepareImage(float * data, long width, long height, long numChannels)
    {
        if (m_initState != STATE_INITIALIZED)
        {
            std::cerr << "The GPU engine is not initialized." << std::endl;
            exit(1);
        }

        m_width = width;
        m_height = height;

        if (numChannels == 4) m_format = GL_RGBA;
        else if (numChannels == 3) m_format = GL_RGB;
        else
        {
            std::cerr << "Cannot process with GPU image with "
                << numChannels << " components." << std::endl;
            exit(1);
        }

        glGenTextures(1, &m_imageTexID);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_imageTexID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, width, height, 0,
            m_format, GL_FLOAT, &data[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        // Create the frame buffer and render buffer
        GLuint fboId;

        // create a framebuffer object, you need to delete them when program exits.
        glGenFramebuffers(1, &fboId);
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);

        GLuint rboId;
        // create a renderbuffer object to store depth info
        glGenRenderbuffers(1, &rboId);
        glBindRenderbuffer(GL_RENDERBUFFER, rboId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F_ARB, m_width, m_height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // attach a texture to FBO color attachement point
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_imageTexID, 0);

        // attach a renderbuffer to depth attachment point
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rboId);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Set the rendering destination to FBO
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);

        // Clear buffer
        glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_initState = STATE_IMAGE_PREPARED;
    }

    void updateGPUShader(
        const OCIO::ConstProcessorRcPtr & processor, bool legacyShader, bool gpuinfo)
    {
        if (m_initState != STATE_IMAGE_PREPARED)
        {
            std::cerr << "GPU image not prepared." << std::endl;
            exit(1);
        }

        OCIO::GpuShaderDescRcPtr shaderDesc
            = legacyShader ? OCIO::GpuShaderDesc::CreateLegacyShaderDesc(32)
                           : OCIO::GpuShaderDesc::CreateShaderDesc();

        // Create the GPU shader description
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3);

        // Collect the shader program information for a specific processor    
        processor->extractGpuShaderInfo(shaderDesc);

        // Use the helper OpenGL builder
        m_oglBuilder = OCIO::OpenGLBuilder::Create(shaderDesc);
        m_oglBuilder->setVerbose(gpuinfo);

        // Allocate & upload all the LUTs
        m_oglBuilder->allocateAllTextures(1);

        std::ostringstream main;
        main << std::endl
            << "uniform sampler2D img;" << std::endl
            << std::endl
            << "void main()" << std::endl
            << "{" << std::endl
            << "    vec4 col = texture2D(img, gl_TexCoord[0].st);" << std::endl
            << "    gl_FragColor = " << shaderDesc->getFunctionName() << "(col);" << std::endl
            << "}" << std::endl;

        // Build the fragment shader program
        m_oglBuilder->buildProgram(main.str().c_str());

        // Enable the fragment shader program, and all needed textures
        m_oglBuilder->useProgram();
        // The image texture
        glUniform1i(glGetUniformLocation(m_oglBuilder->getProgramHandle(), "img"), 0);
        // The LUT textures
        m_oglBuilder->useAllTextures();
        // Enable uniforms for dynamic properties
        m_oglBuilder->useAllUniforms();

        m_initState = STATE_SHADER_UPDATED;
    }

    void processImage()
    {
        if (m_initState != STATE_SHADER_UPDATED)
        {
            std::cerr << "GPU shader has not been updated." << std::endl;
            exit(1);
        }

        glViewport(0, 0, m_width, m_height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, m_width, 0.0, m_height, -100.0, 100.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glEnable(GL_TEXTURE_2D);
        glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glColor3f(1, 1, 1);
        glPushMatrix();
            glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 1.0f);
                glVertex2f(0.0f, (float)m_height);

                glTexCoord2f(0.0f, 0.0f);
                glVertex2f(0.0f, 0.0f);

                glTexCoord2f(1.0f, 0.0f);
                glVertex2f((float)m_width, 0.0f);

                glTexCoord2f(1.0f, 1.0f);
                glVertex2f((float)m_width, (float)m_height);
            glEnd();
        glPopMatrix();
        glDisable(GL_TEXTURE_2D);

        glutSwapBuffers();

        m_initState = STATE_IMAGE_PROCESSED;
    }

    void readImage(std::vector<float>& image)
    {
        if (m_initState != STATE_IMAGE_PROCESSED)
        {
            std::cerr << "Image has not been processed by GPU shader." << std::endl;
            exit(1);
        }

        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, m_width, m_height, m_format, GL_FLOAT, (GLvoid*)&image[0]);

        // Current implementation only has to process 1 image.
        // To handle more images we could go back to STATE_INITIALIZED
        m_initState = STATE_IMAGE_READ;
    }

private:
    void cleanUp()
    {
        m_oglBuilder.reset();
        glutDestroyWindow(m_glwin);
        m_initState = STATE_CREATED;
    }

    enum State
    {
        STATE_CREATED,
        STATE_INITIALIZED,
        STATE_IMAGE_PREPARED,
        STATE_SHADER_UPDATED,
        STATE_IMAGE_PROCESSED,
        STATE_IMAGE_READ
    };

    GLint m_glwin;
    State m_initState;
    OCIO::OpenGLBuilderRcPtr m_oglBuilder;
    GLuint m_imageTexID;
    GLenum m_format;
    long m_width;
    long m_height;
};

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
    bool usegpu = false;
    bool usegpuLegacy = false;
    bool outputgpuInfo = false;
    bool verbose = false;

    ap.options("ocioconvert -- apply colorspace transform to an image \n\n"
               "usage: ocioconvert [options]  inputimage inputcolorspace outputimage outputcolorspace\n\n",
               "%*", parse_end_args, "",
               "<SEPARATOR>", "OpenImageIO options",
               "--float-attribute %L", &floatAttrs, "name=float pair defining OIIO float attribute",
               "--int-attribute %L", &intAttrs, "name=int pair defining OIIO int attribute",
               "--string-attribute %L", &stringAttrs, "name=string pair defining OIIO string attribute",
               "--croptofull", &croptofull, "name=Crop or pad to make pixel data region match the \"full\" region",
               "--ch %s", &keepChannels, "name=Select channels (e.g., \"2,3,4\")",
               "--gpu", &usegpu, "Use GPU color processing instead of CPU (CPU is the default)",
               "--gpulegacy", &usegpuLegacy, "Use the legacy (i.e. baked) GPU color processing "
                                             "instead of the CPU one (--gpu is ignored)",
               "--gpuinfo", &outputgpuInfo, "Output the OCIO shader program",
               "--v", &verbose, "Display general information",
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

    if(verbose)
    {
        std::cout << std::endl;
        std::cout << "OIIO Version: " << OIIO_VERSION_STRING << std::endl;
        std::cout << "OCIO Version: " << OCIO::GetVersion() << std::endl;
        const char * env = getenv("OCIO");
        if(env && *env)
        {
            try
            {
                std::cout << std::endl;
                std::cout << "OCIO Configuration: '" << env << "'" << std::endl;
                OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
                std::cout << "OCIO search_path:    " << config->getSearchPath() << std::endl;
            }
            catch(...)
            {

                std::cerr << "Error loading the config file: '" << env << "'";
                exit(1);
            }
        }
    }
    
    if (usegpuLegacy)
    {
        std::cout << std::endl;
        std::cout << "Using legacy OCIO v1 GPU color processing." << std::endl;
    }
    else if (usegpu)
    {
        std::cout << std::endl;
        std::cout << "Using GPU color processing." << std::endl;
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
    std::cout << std::endl;
    std::cout << "Loading " << inputimage << std::endl;
    try
    {
#if OIIO_VERSION < 10903
        OIIO::ImageInput* f = OIIO::ImageInput::create(inputimage);
#else
        auto f = OIIO::ImageInput::create(inputimage);
#endif
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
        
        const bool ok = f->read_image(OIIO::TypeDesc::FLOAT, &img[0]);
        if(!ok)
        {
            std::cerr << "Error reading \"" << inputimage << "\" : " << f->geterror() << "\n";
            exit(1);
        }

#if OIIO_VERSION < 10903
        OIIO::ImageInput::destroy(f);
#endif
        
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

    // Initialize GPU
    if (usegpu || usegpuLegacy)
    {
        GPUManagement & gpuMgmt = GPUManagement::Instance();
        gpuMgmt.init(verbose);
        gpuMgmt.prepareImage(&img[0], imgwidth, imgheight, components);
    }

    // Process the image
    try
    {
        // Load the current config.
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        
        // Get the processor
        OCIO::ConstProcessorRcPtr processor = config->getProcessor(inputcolorspace, outputcolorspace);

        if (usegpu || usegpuLegacy)
        {
            GPUManagement & gpuMgmt = GPUManagement::Instance();
            // Get the GPU shader program from the processor and set GPU to use it
            gpuMgmt.updateGPUShader(processor, usegpuLegacy, outputgpuInfo);

            // Run the GPU shader on the image
            gpuMgmt.processImage();

            // Read the result
            gpuMgmt.readImage(img);
        }
        else
        {
            // Wrap the image in a light-weight ImageDescription
            OCIO::PackedImageDesc imageDesc(&img[0], imgwidth, imgheight, components);
            
            // Apply the color transformation (in place)
            processor->apply(imageDesc);
        }
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
#if OIIO_VERSION < 10903
        OIIO::ImageOutput* f = OIIO::ImageOutput::create(outputimage);
#else
        auto f = OIIO::ImageOutput::create(outputimage);
#endif
        if(!f)
        {
            std::cerr << "Could not create output input." << std::endl;
            exit(1);
        }
        
        f->open(outputimage, spec);
        const bool ok = f->write_image(OIIO::TypeDesc::FLOAT, &img[0]);
        if(!ok)
        {
            std::cerr << "Error writing \"" << outputimage << "\" : " << f->geterror() << "\n";
            exit(1);
        }

        f->close();
#if OIIO_VERSION < 10903
        OIIO::ImageOutput::destroy(f);
#endif
    }
    catch(...)
    {
        std::cerr << "Error writing file.";
        exit(1);
    }
    
    std::cout << std::endl;
    std::cout << "Wrote " << outputimage << std::endl;
    
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




