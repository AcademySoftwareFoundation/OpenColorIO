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
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>

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

bool g_verbose   = false;
bool g_gpulegacy = false;
bool g_gpuinfo   = false;

std::string g_filename;


GLint g_win = 0;
int g_winWidth = 0;
int g_winHeight = 0;

OCIO::OpenGLBuilderRcPtr g_oglBuilder;

GLuint g_imageTexID;
float g_imageAspect;

const int LUT3D_EDGE_SIZE = 32;

std::string g_inputColorSpace;
std::string g_display;
std::string g_transformName;
std::string g_look;

float g_exposure_fstop = 0.0f;
float g_display_gamma = 1.0f;
int g_channelHot[4] = { 1, 1, 1, 1 };  // show rgb


void UpdateOCIOGLState();

static void InitImageTexture(const char * filename)
{
    glGenTextures(1, &g_imageTexID);
    
    std::vector<float> img;
    int texWidth = 512;
    int texHeight = 512;
    int components = 4;
    
    if(filename && *filename)
    {
        std::cout << "loading: " << filename << std::endl;
        try
        {
#if OIIO_VERSION < 10903
            OIIO::ImageInput* f = OIIO::ImageInput::create(filename);
#else
            auto f = OIIO::ImageInput::create(filename);
#endif
            if(!f)
            {
                std::cerr << "Could not create image input." << std::endl;
                exit(1);
            }
            
            OIIO::ImageSpec spec;
            f->open(filename, spec);
            
            std::string error = f->geterror();
            if(!error.empty())
            {
                std::cerr << "Error loading image " << error << std::endl;
                exit(1);
            }
            
            texWidth = spec.width;
            texHeight = spec.height;
            components = spec.nchannels;

            img.resize(texWidth*texHeight*components);
            memset(&img[0], 0, texWidth*texHeight*components*sizeof(float));

            const bool ok = f->read_image(OIIO::TypeDesc::FLOAT, &img[0]);
            if(!ok)
            {
                std::cerr << "Error reading \"" << filename << "\" : " << f->geterror() << "\n";
                exit(1);
            }

#if OIIO_VERSION < 10903
            OIIO::ImageInput::destroy(f);
#endif
        }
        catch(...)
        {
            std::cerr << "Error loading file.";
            exit(1);
        }
    }
    // If no file is provided, use a default gradient texture
    else
    {
        std::cout << "No image specified, loading gradient." << std::endl;
        
        img.resize(texWidth*texHeight*components);
        memset(&img[0], 0, texWidth*texHeight*components*sizeof(float));
        
        for(int y=0; y<texHeight; ++y)
        {
            for(int x=0; x<texWidth; ++x)
            {
                float c = (float)x/((float)texWidth-1.0f);
                img[components*(texWidth*y+x) + 0] = c;
                img[components*(texWidth*y+x) + 1] = c;
                img[components*(texWidth*y+x) + 2] = c;
                img[components*(texWidth*y+x) + 3] = 1.0f;
            }
        }
    }
    
    
    GLenum format = 0;
    if(components == 4) format = GL_RGBA;
    else if(components == 3) format = GL_RGB;
    else
    {
        std::cerr << "Cannot load image with " << components << " components." << std::endl;
        exit(1);
    }
    
    g_imageAspect = 1.0;
    if(texHeight!=0)
    {
        g_imageAspect = (float) texWidth / (float) texHeight;
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_imageTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, texWidth, texHeight, 0,
        format, GL_FLOAT, &img[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

void InitOCIO(const char * filename)
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    g_display = config->getDefaultDisplay();
    g_transformName = config->getDefaultView(g_display.c_str());
    g_look = config->getDisplayLooks(g_display.c_str(), g_transformName.c_str());
    
    g_inputColorSpace = OCIO::ROLE_SCENE_LINEAR;
    if(filename && *filename)
    {
        std::string cs = config->parseColorSpaceFromString(filename);
        if(!cs.empty())
        {
            g_inputColorSpace = cs;
            std::cout << "colorspace: " << cs << std::endl;
        }
        else
        {
            std::cout << "colorspace: " << g_inputColorSpace 
                << " \t(could not determine from filename, using default)" << std::endl;
        }
    }
}

/*
static void
Idle(void)
{
   // + Do Work
   glutPostRedisplay();
}
*/

void Redisplay(void)
{
    float windowAspect = 1.0;
    if(g_winHeight != 0)
    {
        windowAspect = (float)g_winWidth/(float)g_winHeight;
    }
    
    float pts[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // x0,y0,x1,y1
    if(windowAspect>g_imageAspect)
    {
        float imgWidthScreenSpace = g_imageAspect * (float)g_winHeight;
        pts[0] = (float)g_winWidth * 0.5f - (float)imgWidthScreenSpace * 0.5f;
        pts[2] = (float)g_winWidth * 0.5f + (float)imgWidthScreenSpace * 0.5f;
        pts[1] = 0.0f;
        pts[3] = (float)g_winHeight;
    }
    else
    {
        float imgHeightScreenSpace = (float)g_winWidth / g_imageAspect;
        pts[0] = 0.0f;
        pts[2] = (float)g_winWidth;
        pts[1] = (float)g_winHeight * 0.5f - imgHeightScreenSpace * 0.5f;
        pts[3] = (float)g_winHeight * 0.5f + imgHeightScreenSpace * 0.5f;
    }
    
    glEnable(GL_TEXTURE_2D);
    glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(1, 1, 1);
    
    glPushMatrix();
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(pts[0], pts[1]);
    
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(pts[0], pts[3]);
    
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(pts[2], pts[3]);
    
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(pts[2], pts[1]);
    
    glEnd();
    glPopMatrix();
    
    glDisable(GL_TEXTURE_2D);
    
    glutSwapBuffers();
}


static void Reshape(int width, int height)
{
    g_winWidth = width;
    g_winHeight = height;
    
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, g_winWidth, 0.0, g_winHeight, -100.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


static void CleanUp(void)
{
    g_oglBuilder.reset();
    glutDestroyWindow(g_win);
}


static void Key(unsigned char key, int /*x*/, int /*y*/)
{
    if(key == 'c' || key == 'C')
    {
        g_channelHot[0] = 1;
        g_channelHot[1] = 1;
        g_channelHot[2] = 1;
        g_channelHot[3] = 1;
    }
    else if(key == 'r' || key == 'R')
    {
        g_channelHot[0] = 1;
        g_channelHot[1] = 0;
        g_channelHot[2] = 0;
        g_channelHot[3] = 0;
    }
    else if(key == 'g' || key == 'G')
    {
        g_channelHot[0] = 0;
        g_channelHot[1] = 1;
        g_channelHot[2] = 0;
        g_channelHot[3] = 0;
    }
    else if(key == 'b' || key == 'B')
    {
        g_channelHot[0] = 0;
        g_channelHot[1] = 0;
        g_channelHot[2] = 1;
        g_channelHot[3] = 0;
    }
    else if(key == 'a' || key == 'A')
    {
        g_channelHot[0] = 0;
        g_channelHot[1] = 0;
        g_channelHot[2] = 0;
        g_channelHot[3] = 1;
    }
    else if(key == 'l' || key == 'L')
    {
        g_channelHot[0] = 1;
        g_channelHot[1] = 1;
        g_channelHot[2] = 1;
        g_channelHot[3] = 0;
    }
    else if(key == 27)
    {
        CleanUp();
        exit(0);
    }
    
    UpdateOCIOGLState();
    glutPostRedisplay();
}


static void SpecialKey(int key, int x, int y)
{
    (void) x;
    (void) y;
    
    int mod = glutGetModifiers();
    
    if(key == GLUT_KEY_UP && (mod & GLUT_ACTIVE_CTRL))
    {
        g_exposure_fstop += 0.25f;
    }
    else if(key == GLUT_KEY_DOWN && (mod & GLUT_ACTIVE_CTRL))
    {
        g_exposure_fstop -= 0.25f;
    }
    else if(key == GLUT_KEY_HOME && (mod & GLUT_ACTIVE_CTRL))
    {
        g_exposure_fstop = 0.0f;
        g_display_gamma = 1.0f;
    }
    
    else if(key == GLUT_KEY_UP && (mod & GLUT_ACTIVE_ALT))
    {
        g_display_gamma *= 1.1f;
    }
    else if(key == GLUT_KEY_DOWN && (mod & GLUT_ACTIVE_ALT))
    {
        g_display_gamma /= 1.1f;
    }
    else if(key == GLUT_KEY_HOME && (mod & GLUT_ACTIVE_ALT))
    {
        g_exposure_fstop = 0.0f;
        g_display_gamma = 1.0f;
    }
    
    UpdateOCIOGLState();
    
    glutPostRedisplay();
}

const char * g_fragShaderText = ""
"\n"
"uniform sampler2D tex1;\n"
"\n"
"void main()\n"
"{\n"
"    vec4 col = texture2D(tex1, gl_TexCoord[0].st);\n"
"    gl_FragColor = OCIODisplay(col);\n"
"}\n";


void UpdateOCIOGLState()
{
    // Step 0: Get the processor using any of the pipelines mentioned above.
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    
    OCIO::DisplayTransformRcPtr transform = OCIO::DisplayTransform::Create();
    transform->setInputColorSpaceName( g_inputColorSpace.c_str() );
    transform->setDisplay( g_display.c_str() );
    transform->setView( g_transformName.c_str() );
    transform->setLooksOverride( g_look.c_str() );

    if(g_verbose)
    {
        std::cout << std::endl;
        std::cout << "Color transformation composed of:" << std::endl;
        std::cout << "      Image ColorSpace is:\t" << g_inputColorSpace << std::endl;
        std::cout << "      Transform is:\t\t" << g_transformName << std::endl;
        std::cout << "      Device is:\t\t" << g_display << std::endl;
        std::cout << "      Looks Override is:\t'" << g_look << "'" << std::endl;
        std::cout << "  with:" << std::endl;
        std::cout << "    exposure_fstop = " << g_exposure_fstop << std::endl;
        std::cout << "    display_gamma  = " << g_display_gamma << std::endl;
        std::cout << "    channels       = " 
                  << (g_channelHot[0] ? "R" : "")
                  << (g_channelHot[1] ? "G" : "")
                  << (g_channelHot[2] ? "B" : "")
                  << (g_channelHot[3] ? "A" : "") << std::endl;

    }
    
    // Add optional transforms to create a full-featured, "canonical" display pipeline
    // Fstop exposure control (in SCENE_LINEAR)
    {
        float gain = powf(2.0f, g_exposure_fstop);
        const float slope4f[] = { gain, gain, gain, gain };
        float m44[16];
        float offset4[4];
        OCIO::MatrixTransform::Scale(m44, offset4, slope4f);
        OCIO::MatrixTransformRcPtr mtx =  OCIO::MatrixTransform::Create();
        mtx->setValue(m44, offset4);
        transform->setLinearCC(mtx);
    }
    
    // Channel swizzling
    {
        float lumacoef[3];
        config->getDefaultLumaCoefs(lumacoef);
        float m44[16];
        float offset[4];
        OCIO::MatrixTransform::View(m44, offset, g_channelHot, lumacoef);
        OCIO::MatrixTransformRcPtr swizzle = OCIO::MatrixTransform::Create();
        swizzle->setValue(m44, offset);
        transform->setChannelView(swizzle);
    }
    
    // Post-display transform gamma
    {
        float exponent = 1.0f/std::max(1e-6f, static_cast<float>(g_display_gamma));
        const float exponent4f[] = { exponent, exponent, exponent, exponent };
        OCIO::ExponentTransformRcPtr expTransform =  OCIO::ExponentTransform::Create();
        expTransform->setValue(exponent4f);
        transform->setDisplayCC(expTransform);
    }
    
    OCIO::ConstProcessorRcPtr processor;
    try
    {
        processor = config->getProcessor(transform);
    }
    catch(OCIO::Exception & e)
    {
        std::cerr << e.what() << std::endl;
        return;
    }
    catch(...)
    {
        return;
    }
    
    // Step 1: Create the appropriate GPU shader description
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = g_gpulegacy ? OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE)
                      : OCIO::GpuShaderDesc::CreateShaderDesc();
    shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_0);
    shaderDesc->setFunctionName("OCIODisplay");
    shaderDesc->setResourcePrefix("ocio_");

    // Step 2: Collect the shader program information for a specific processor    
    processor->extractGpuShaderInfo(shaderDesc);

    // Step 3: Use the helper OpenGL builder
    g_oglBuilder = OCIO::OpenGLBuilder::Create(shaderDesc);
    g_oglBuilder->setVerbose(g_gpuinfo);

    // Step 4: Allocate & upload all the LUTs
    // 
    // NB: The start index for the texture indices is 1 as one texture
    //     was already created for the input image.
    //     
    g_oglBuilder->allocateAllTextures(1);
    
    // Step 5: Build the fragment shader program
    g_oglBuilder->buildProgram(g_fragShaderText);

    // Step 6: Enable the fragment shader program, and all needed textures
    g_oglBuilder->useProgram();
    // The image texture
    glUniform1i(glGetUniformLocation(g_oglBuilder->getProgramHandle(), "tex1"), 0);
    // The LUT textures
    g_oglBuilder->useAllTextures();
    // Enable uniforms for dynamic properties
    g_oglBuilder->useAllUniforms();
}

void menuCallback(int /*id*/)
{
    glutPostRedisplay();
}

void imageColorSpace_CB(int id)
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    const char * name = config->getColorSpaceNameByIndex(id);
    if(!name) return;
    
    g_inputColorSpace = name;
    
    UpdateOCIOGLState();
    glutPostRedisplay();
}

void displayDevice_CB(int id)
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    const char * display = config->getDisplay(id);
    if(!display) return;
    
    g_display = display;
    
    const char * csname = config->getDisplayColorSpaceName(g_display.c_str(), g_transformName.c_str());
    if(!csname)
    {
        g_transformName = config->getDefaultView(g_display.c_str());
    }

    g_look = config->getDisplayLooks(g_display.c_str(), g_transformName.c_str());

    UpdateOCIOGLState();
    glutPostRedisplay();
}

void transform_CB(int id)
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    
    const char * transform = config->getView(g_display.c_str(), id);
    if(!transform) return;
    
    g_transformName = transform;

    g_look = config->getDisplayLooks(g_display.c_str(), g_transformName.c_str());
    
    UpdateOCIOGLState();
    glutPostRedisplay();
}

void look_CB(int id)
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    
    const char * look = config->getLookNameByIndex(id);
    if(!look || !*look) return;
    
    g_look = look;
    
    UpdateOCIOGLState();
    glutPostRedisplay();
}

static void PopulateOCIOMenus()
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    
    int csMenuID = glutCreateMenu(imageColorSpace_CB);

    std::map<std::string, int> families;
    for(int i=0; i<config->getNumColorSpaces(); ++i)
    {
        const char * csName = config->getColorSpaceNameByIndex(i);
        if(csName && *csName)
        {
            OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(csName);
            if(cs)
            {
                const char * family = cs->getFamily();
                if(family && *family)
                {
                    if(families.find(family)==families.end())
                    {
                        families[family] = glutCreateMenu(imageColorSpace_CB);
                        glutAddMenuEntry(csName, i);

                        glutSetMenu(csMenuID);
                        glutAddSubMenu(family, families[family]);
                    }
                    else
                    {
                        glutSetMenu(families[family]);
                        glutAddMenuEntry(csName, i);
                    }
                }
                else
                {
                    glutSetMenu(csMenuID);
                    glutAddMenuEntry(csName, i);
                }
            }
        }
    }
    
    int deviceMenuID = glutCreateMenu(displayDevice_CB);
    for(int i=0; i<config->getNumDisplays(); ++i)
    {
        glutAddMenuEntry(config->getDisplay(i), i);
    }
    
    int transformMenuID = glutCreateMenu(transform_CB);
    const char * defaultDisplay = config->getDefaultDisplay();
    for(int i=0; i<config->getNumViews(defaultDisplay); ++i)
    {
        glutAddMenuEntry(config->getView(defaultDisplay, i), i);
    }
    
    int lookMenuID = glutCreateMenu(look_CB);
    for(int i=0; i<config->getNumLooks(); ++i)
    {
        glutAddMenuEntry(config->getLookNameByIndex(i), i);
    }
    
    glutCreateMenu(menuCallback);
    glutAddSubMenu("Image ColorSpace", csMenuID);
    glutAddSubMenu("Transform", transformMenuID);
    glutAddSubMenu("Device", deviceMenuID);
    glutAddSubMenu("Looks Override", lookMenuID);
    
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

const char * USAGE_TEXT = "\n"
"Keys:\n"
"\tCtrl+Up:   Exposure +1/4 stop (in scene linear)\n"
"\tCtrl+Down: Exposure -1/4 stop (in scene linear)\n"
"\tCtrl+Home: Reset Exposure + Gamma\n"
"\n"
"\tAlt+Up:    Gamma up (post display transform)\n"
"\tAlt+Down:  Gamma down (post display transform)\n"
"\tAlt+Home:  Reset Exposure + Gamma\n"
"\n"
"\tC:   View Color\n"
"\tR:   View Red  \n"
"\tG:   View Green\n"
"\tB:   View Blue\n"
"\tA:   View Alpha\n"
"\tL:   View Luma\n"
"\n"
"\tRight-Mouse Button:   Configure Display / Transform / ColorSpace / Looks\n"
"\n"
"\tEsc: Quit\n";

void parseArguments(int argc, char **argv)
{
    for(int i=1; i<argc; ++i)
    {
        if(0==strcmp(argv[i], "-v"))
        {
            g_verbose = true;
        }
        else if(0==strcmp(argv[i], "-gpulegacy"))
        {
            g_gpulegacy = true;
        }
        else if(0==strcmp(argv[i], "-gpuinfo"))
        {
            g_gpuinfo = true;
        }
        else if(0==strcmp(argv[i], "-h"))
        {
            std::cout << std::endl;
            std::cout << "help:" << std::endl;
            std::cout << "  ociodisplay [OPTIONS] [image]  where" << std::endl;
            std::cout << std::endl;
            std::cout << "  OPTIONS:" << std::endl;
            std::cout << "     -h         :  displays the help and exit" << std::endl;
            std::cout << "     -v         :  displays the color space information" << std::endl;
            std::cout << "     -gpulegacy :  use the legacy (i.e. baked) GPU color processing" << std::endl;
            std::cout << "     -gpuinfo   :  output the OCIO shader program" << std::endl;
             std::cout << std::endl;
            exit(0);
        }
        else
        {
            g_filename = argv[i];
        }
    }
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition (100, 100);

    parseArguments(argc, argv);
    
    g_win = glutCreateWindow(argv[0]);
    
#ifndef __APPLE__
    glewInit();
    if (!glewIsSupported("GL_VERSION_2_0"))
    {
        std::cerr << "OpenGL 2.0 not supported" << std::endl;
        exit(1);
    }
#endif
    
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutSpecialFunc(SpecialKey);
    glutDisplayFunc(Redisplay);

    if(g_verbose)
    {
        std::cout << std::endl
                  << "GL Vendor:    " << glGetString(GL_VENDOR) << std::endl
                  << "GL Renderer:  " << glGetString(GL_RENDERER) << std::endl
                  << "GL Version:   " << glGetString(GL_VERSION) << std::endl
                  << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

        if(!g_filename.empty())
        {
            std::cout << std::endl;
            std::cout << "Image: " << g_filename << std::endl;
        }
        std::cout << std::endl;
        std::cout << "OIIO Version: " << OIIO_VERSION_STRING << std::endl;
        std::cout << "OCIO Version: " << OCIO::GetVersion() << std::endl;
    }

    OCIO::ConstConfigRcPtr config;
    try
    {
        config = OCIO::GetCurrentConfig();
    }
    catch(...)
    {
        const char * env = getenv("OCIO");
        std::cerr << "Error loading the config file: '" << (env ? env : "") << "'";
        exit(1);
    }

    if(g_verbose)
    {
        const char * env = getenv("OCIO");

        if(env && *env)
        {
            std::cout << std::endl;
            std::cout << "OCIO Configuration: '" << env << "'" << std::endl;
            std::cout << "OCIO search_path:    " << config->getSearchPath() << std::endl;
        }
    }
    
    std::cout << std::endl;
    std::cout << USAGE_TEXT << std::endl;
    
    InitImageTexture(g_filename.c_str());
    try
    {
        InitOCIO(g_filename.c_str());
    }
    catch(OCIO::Exception & e)
    {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    
    PopulateOCIOMenus();
    
    Reshape(1024, 512);
    
    try
    {
        UpdateOCIOGLState();
    }
    catch(OCIO::Exception & e)
    {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    
    Redisplay();
    
    /*
    if (Anim)
    {
        glutIdleFunc(Idle);
    }
    */

    glutMainLoop();

    return 0;
}
