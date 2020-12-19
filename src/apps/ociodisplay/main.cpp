// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <array>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

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
#if __APPLE__
#include "metalapp.h"
#endif

#include "glsl.h"
#include "oglapp.h"
#include "imageio.h"

bool g_verbose   = false;
bool g_gpulegacy = false;
bool g_gpuinfo   = false;
#if __APPLE__
bool g_useMetal  = false;
#endif

std::string g_filename;

float g_imageAspect;

std::string g_inputColorSpace;
std::string g_display;
std::string g_transformName;
std::string g_look;
OCIO::OptimizationFlags g_optimization{ OCIO::OPTIMIZATION_DEFAULT };

static const std::array<std::pair<const char*, OCIO::OptimizationFlags>, 5> OptmizationMenu = { {
    { "None",      OCIO::OPTIMIZATION_NONE },
    { "Lossless",  OCIO::OPTIMIZATION_LOSSLESS },
    { "Very good", OCIO::OPTIMIZATION_VERY_GOOD },
    { "Good",      OCIO::OPTIMIZATION_GOOD },
    { "Draft",     OCIO::OPTIMIZATION_DRAFT } } };

float g_exposure_fstop{ 0.0f };
float g_display_gamma{ 1.0f };
int g_channelHot[4]{ 1, 1, 1, 1 };  // show rgb

OCIO::OglAppRcPtr g_oglApp;


void UpdateOCIOGLState();

static void InitImageTexture(const char * filename)
{
    OCIO::ImageIO img;

    if (filename && *filename)
    {
        std::cout << "Loading: " << filename << std::endl;

        try
        {
            img.read(filename, OCIO::BIT_DEPTH_F32);
        }
        catch (const std::exception & e)
        {
            std::cerr << "ERROR: Loading file failed: " << e.what() << std::endl;
            exit(1);
        }
        catch (...)
        {
            std::cerr << "ERROR: Loading file failed." << std::endl;
            exit(1);
        }
    }
    // If no file is provided, use a default gradient texture
    else
    {
        std::cout << "No image specified, loading gradient." << std::endl;

        img.init(512, 512, OCIO::CHANNEL_ORDERING_RGBA, OCIO::BIT_DEPTH_F32);

        float * pixels = (float *) img.getData();
        const long width = img.getWidth();
        const long channels = img.getNumChannels();

        for (int y=0; y<img.getHeight(); ++y)
        {
            for (int x=0; x<img.getWidth(); ++x)
            {
                float c = (float)x / ((float)width-1.0f);
                pixels[channels*(width*y+x) + 0] = c;
                pixels[channels*(width*y+x) + 1] = c;
                pixels[channels*(width*y+x) + 2] = c;
                pixels[channels*(width*y+x) + 3] = 1.0f;
            }
        }
    }

    OCIO::OglApp::Components comp = OCIO::OglApp::COMPONENTS_RGBA;
    if (img.getNumChannels() == 4)
    {
        comp = OCIO::OglApp::COMPONENTS_RGBA;
    }
    else if (img.getNumChannels() == 3)
    {
        comp = OCIO::OglApp::COMPONENTS_RGB;
    }
    else
    {
        std::cerr << "Cannot load image with " << img.getNumChannels()
                  << " components." << std::endl;
        exit(1);
    }

    g_imageAspect = 1.0;
    if (img.getHeight()!=0)
    {
        g_imageAspect = (float) img.getWidth() / (float) img.getHeight();
    }

    if (g_oglApp)
    {
        g_oglApp->initImage(img.getWidth(),
                            img.getHeight(),
                            comp,
                            (float*) img.getData());
    }

}

void InitOCIO(const char * filename)
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    g_display = config->getDefaultDisplay();
    g_transformName = config->getDefaultView(g_display.c_str());
    g_look = config->getDisplayViewLooks(g_display.c_str(), g_transformName.c_str());
    g_inputColorSpace = OCIO::ROLE_SCENE_LINEAR;

    if (filename && *filename)
    {
        std::string cs = config->getColorSpaceFromFilepath(filename);
        if (!cs.empty())
        {
            g_inputColorSpace = cs;
            std::cout << "colorspace: " << cs << std::endl;
        }
        else
        {
            std::cout << "colorspace: " << g_inputColorSpace 
                      << " \t(could not determine from filename, using default)"
                      << std::endl;
        }
    }
}

void Redisplay(void)
{
    if (g_oglApp)
    {
        g_oglApp->redisplay();
    }
}

static void Reshape(int width, int height)
{
    if (g_oglApp)
    {
        g_oglApp->reshape(width, height);
    }
}

static void CleanUp(void)
{
    g_oglApp.reset();
}

static void Key(unsigned char key, int /*x*/, int /*y*/)
{
    if (key == 'c' || key == 'C')
    {
        g_channelHot[0] = 1;
        g_channelHot[1] = 1;
        g_channelHot[2] = 1;
        g_channelHot[3] = 1;
    }
    else if (key == 'r' || key == 'R')
    {
        g_channelHot[0] = 1;
        g_channelHot[1] = 0;
        g_channelHot[2] = 0;
        g_channelHot[3] = 0;
    }
    else if (key == 'g' || key == 'G')
    {
        g_channelHot[0] = 0;
        g_channelHot[1] = 1;
        g_channelHot[2] = 0;
        g_channelHot[3] = 0;
    }
    else if (key == 'b' || key == 'B')
    {
        g_channelHot[0] = 0;
        g_channelHot[1] = 0;
        g_channelHot[2] = 1;
        g_channelHot[3] = 0;
    }
    else if (key == 'a' || key == 'A')
    {
        g_channelHot[0] = 0;
        g_channelHot[1] = 0;
        g_channelHot[2] = 0;
        g_channelHot[3] = 1;
    }
    else if (key == 'l' || key == 'L')
    {
        g_channelHot[0] = 1;
        g_channelHot[1] = 1;
        g_channelHot[2] = 1;
        g_channelHot[3] = 0;
    }
    else if (key == 27)
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

void UpdateOCIOGLState()
{
    if (!g_oglApp)
    {
        return;
    }

    // Step 0: Get the processor using any of the pipelines mentioned above.
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

    OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
    transform->setSrc( g_inputColorSpace.c_str() );
    transform->setDisplay( g_display.c_str() );
    transform->setView( g_transformName.c_str() );

    OCIO::LegacyViewingPipelineRcPtr vp = OCIO::LegacyViewingPipeline::Create();
    vp->setDisplayViewTransform(transform);
    vp->setLooksOverrideEnabled(true);
    vp->setLooksOverride(g_look.c_str());

    if (g_verbose)
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

        for (const auto & opt : OptmizationMenu)
        {
            if (opt.second == g_optimization)
            {
                std::cout << std::endl << "Optimization: " << opt.first << std::endl;
            }
        }

    }

    // Add optional transforms to create a full-featured, "canonical" display pipeline
    // Fstop exposure control (in SCENE_LINEAR)
    {
        double gain = powf(2.0f, g_exposure_fstop);
        const double slope4f[] = { gain, gain, gain, gain };
        double m44[16];
        double offset4[4];
        OCIO::MatrixTransform::Scale(m44, offset4, slope4f);
        OCIO::MatrixTransformRcPtr mtx =  OCIO::MatrixTransform::Create();
        mtx->setMatrix(m44);
        mtx->setOffset(offset4);
        vp->setLinearCC(mtx);
    }

    // Channel swizzling
    {
        double lumacoef[3];
        config->getDefaultLumaCoefs(lumacoef);
        double m44[16];
        double offset[4];
        OCIO::MatrixTransform::View(m44, offset, g_channelHot, lumacoef);
        OCIO::MatrixTransformRcPtr swizzle = OCIO::MatrixTransform::Create();
        swizzle->setMatrix(m44);
        swizzle->setOffset(offset);
        vp->setChannelView(swizzle);
    }

    // Post-display transform gamma
    {
        double exponent = 1.0/std::max(1e-6, static_cast<double>(g_display_gamma));
        const double exponent4f[4] = { exponent, exponent, exponent, exponent };
        OCIO::ExponentTransformRcPtr expTransform =  OCIO::ExponentTransform::Create();
        expTransform->setValue(exponent4f);
        vp->setDisplayCC(expTransform);
    }

    OCIO::ConstProcessorRcPtr processor;
    try
    {
        processor = vp->getProcessor(config, config->getCurrentContext());
    }
    catch (const OCIO::Exception & e)
    {
        std::cerr << e.what() << std::endl;
        return;
    }
    catch (...)
    {
        return;
    }

    // Set the shader context.
    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    shaderDesc->setLanguage(
#if __APPLE__
                            g_useMetal ?
                            OCIO::GPU_LANGUAGE_MSL_2_0 :
#endif
                            OCIO::GPU_LANGUAGE_GLSL_1_2);
    shaderDesc->setFunctionName("OCIODisplay");
    shaderDesc->setResourcePrefix("ocio_");

    // Extract the shader information.
    OCIO::ConstGPUProcessorRcPtr gpu
        = g_gpulegacy ? processor->getOptimizedLegacyGPUProcessor(g_optimization, 32)
                      : processor->getOptimizedGPUProcessor(g_optimization);
    gpu->extractGpuShaderInfo(shaderDesc);

    g_oglApp->setShader(shaderDesc);
}

void menuCallback(int /*id*/)
{
    glutPostRedisplay();
}

void imageColorSpace_CB(int id)
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    const char * name = config->getColorSpaceNameByIndex(id);
    if (!name)
    {
        return;
    }

    g_inputColorSpace = name;

    UpdateOCIOGLState();
    glutPostRedisplay();
}

void displayDevice_CB(int id)
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    const char * display = config->getDisplay(id);
    if (!display)
    {
        return;
    }

    g_display = display;

    const char * csname = config->getDisplayViewColorSpaceName(g_display.c_str(), g_transformName.c_str());
    if (!csname || !*csname)
    {
        g_transformName = config->getDefaultView(g_display.c_str());
    }

    g_look = config->getDisplayViewLooks(g_display.c_str(), g_transformName.c_str());

    UpdateOCIOGLState();
    glutPostRedisplay();
}

void transform_CB(int id)
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

    const char * transform = config->getView(g_display.c_str(), id);
    if (!transform)
    {
        return;
    }

    g_transformName = transform;

    g_look = config->getDisplayViewLooks(g_display.c_str(), g_transformName.c_str());

    UpdateOCIOGLState();
    glutPostRedisplay();
}

void look_CB(int id)
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

    const char * look = config->getLookNameByIndex(id);
    if (!look || !*look)
    {
        return;
    }

    g_look = look;

    UpdateOCIOGLState();
    glutPostRedisplay();
}

void optimization_CB(int id)
{
    g_optimization = OptmizationMenu[id].second;

    UpdateOCIOGLState();
    glutPostRedisplay();
}

static void PopulateOCIOMenus()
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

    int csMenuID = glutCreateMenu(imageColorSpace_CB);

    std::map<std::string, int> families;
    for (int i=0; i<config->getNumColorSpaces(); ++i)
    {
        const char * csName = config->getColorSpaceNameByIndex(i);
        if (csName && *csName)
        {
            OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(csName);
            if (cs)
            {
                const char * family = cs->getFamily();
                if (family && *family)
                {
                    if (families.find(family)==families.end())
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
    for (int i=0; i<config->getNumDisplays(); ++i)
    {
        glutAddMenuEntry(config->getDisplay(i), i);
    }

    int transformMenuID = glutCreateMenu(transform_CB);
    const char * defaultDisplay = config->getDefaultDisplay();
    for (int i=0; i<config->getNumViews(defaultDisplay); ++i)
    {
        glutAddMenuEntry(config->getView(defaultDisplay, i), i);
    }

    int lookMenuID = glutCreateMenu(look_CB);
    for (int i=0; i<config->getNumLooks(); ++i)
    {
        glutAddMenuEntry(config->getLookNameByIndex(i), i);
    }

    int optimizationMenuID = glutCreateMenu(optimization_CB);
    for (size_t i = 0; i<OptmizationMenu.size(); ++i)
    {
        glutAddMenuEntry(OptmizationMenu[i].first, static_cast<int>(i));
    }

    glutCreateMenu(menuCallback);
    glutAddSubMenu("Image ColorSpace", csMenuID);
    glutAddSubMenu("Transform", transformMenuID);
    glutAddSubMenu("Device", deviceMenuID);
    glutAddSubMenu("Looks Override", lookMenuID);
    glutAddSubMenu("Optimization", optimizationMenuID);

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
"\tR:   View Red\n"
"\tG:   View Green\n"
"\tB:   View Blue\n"
"\tA:   View Alpha\n"
"\tL:   View Luma\n"
"\n"
"\tRight-Mouse Button:   Configure Display / Transform / ColorSpace / Looks / Optimization\n"
"\n"
"\tEsc: Quit\n";

void parseArguments(int argc, char **argv)
{
    for (int i=1; i<argc; ++i)
    {
        if (0==strcmp(argv[i], "-v"))
        {
            g_verbose = true;
        }
        else if (0==strcmp(argv[i], "-gpulegacy"))
        {
            g_gpulegacy = true;
        }
        else if (0==strcmp(argv[i], "-gpuinfo"))
        {
            g_gpuinfo = true;
        }
#if __APPLE__
        else if (0==strcmp(argv[i], "-metal"))
        {
            g_useMetal = true;
        }
#endif
        else if (0==strcmp(argv[i], "-h"))
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
#if __APPLE__
            std::cout << "     -metal     :  use metal OCIO shader backend " << std::endl;
#endif
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
    parseArguments(argc, argv);

    try
    {
#if __APPLE__
        if (g_useMetal)
        {
            g_oglApp = std::make_shared<OCIO::MetalApp>("ociodisplay", 512, 512);
        }
        else
#endif
        {
            g_oglApp = std::make_shared<OCIO::ScreenApp>("ociodisplay", 512, 512);
        }
    }
    catch (const OCIO::Exception & e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    if (g_verbose)
    {
        g_oglApp->printGLInfo();
    }

    g_oglApp->setYMirror();
    g_oglApp->setPrintShader(g_gpuinfo);

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutSpecialFunc(SpecialKey);
    glutDisplayFunc(Redisplay);

    if (g_verbose)
    {
        if (!g_filename.empty())
        {
            std::cout << std::endl;
            std::cout << "Image: " << g_filename << std::endl;
        }
        std::cout << std::endl;
        std::cout << OCIO::ImageIO::GetVersion() << std::endl;
        std::cout << "OCIO Version: " << OCIO::GetVersion() << std::endl;
    }

    OCIO::ConstConfigRcPtr config;
    try
    {
        config = OCIO::GetCurrentConfig();
    }
    catch (...)
    {
        const char * env = OCIO::GetEnvVariable("OCIO");
        std::cerr << "Error loading the config file: '" << (env ? env : "") << "'";
        exit(1);
    }

    if (g_verbose)
    {
        const char * env = OCIO::GetEnvVariable("OCIO");

        if (env && *env)
        {
            std::cout << std::endl;
            std::cout << "OCIO Config. file   : '" << env << "'" << std::endl;
            std::cout << "OCIO Config. version: " << config->getMajorVersion() << "." 
                                                  << config->getMinorVersion() << std::endl;
            std::cout << "OCIO search_path    : " << config->getSearchPath() << std::endl;
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

    try
    {
        UpdateOCIOGLState();
    }
    catch (const OCIO::Exception & e)
    {
        std::cerr << e.what() << std::endl;
        exit(1);
    }

    Redisplay();

    glutMainLoop();

    return 0;
}
