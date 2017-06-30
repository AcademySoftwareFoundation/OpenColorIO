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
#include <GL/glext.h>
#include <GL/glut.h>
#endif

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
#if (OIIO_VERSION < 10100)
namespace OIIO = OIIO_NAMESPACE;
#endif

GLint g_win = 0;
int g_winWidth = 0;
int g_winHeight = 0;

GLuint g_fragShader = 0;
GLuint g_program = 0;

GLuint g_imageTexID;
float g_imageAspect;

GLuint g_lut3dTexID;
const int LUT3D_EDGE_SIZE = 32;
std::vector<float> g_lut3d;
std::string g_lut3dcacheid;
std::string g_shadercacheid;

std::string g_inputColorSpace;
std::string g_display;
std::string g_transformName;

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
    
    if(filename)
    {
        std::cout << "loading: " << filename << std::endl;
        try
        {
            OIIO::ImageInput* f = OIIO::ImageInput::create(filename);
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
            
            f->read_image(OIIO::TypeDesc::TypeFloat, &img[0]);
            delete f;
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
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_imageTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, texWidth, texHeight, 0,
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
    
    g_inputColorSpace = OCIO::ROLE_SCENE_LINEAR;
    if(filename)
    {
        std::string cs = config->parseColorSpaceFromString(filename);
        if(!cs.empty())
        {
            g_inputColorSpace = cs;
            std::cout << "colorspace: " << cs << std::endl;
        }
        else
        {
            std::cout << "colorspace: " << g_inputColorSpace << " \t(could not determine from filename, using default)" << std::endl;
        }
    }
}

static void AllocateLut3D()
{
    glGenTextures(1, &g_lut3dTexID);
    
    int num3Dentries = 3*LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE;
    g_lut3d.resize(num3Dentries);
    memset(&g_lut3d[0], 0, sizeof(float)*num3Dentries);
    
    glActiveTexture(GL_TEXTURE2);
    
    glBindTexture(GL_TEXTURE_3D, g_lut3dTexID);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB,
                 LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE,
                 0, GL_RGB,GL_FLOAT, &g_lut3d[0]);
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
    glDeleteShader(g_fragShader);
    glDeleteProgram(g_program);
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

GLuint
CompileShaderText(GLenum shaderType, const char *text)
{
    GLuint shader;
    GLint stat;
    
    shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, (const GLchar **) &text, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &stat);
    
    if (!stat)
    {
        GLchar log[1000];
        GLsizei len;
        glGetShaderInfoLog(shader, 1000, &len, log);
        fprintf(stderr, "Error: problem compiling shader: %s\n", log);
        return 0;
    }
    
    return shader;
}

GLuint
LinkShaders(GLuint fragShader)
{
    if (!fragShader) return 0;
    
    GLuint program = glCreateProgram();
    
    if (fragShader)
        glAttachShader(program, fragShader);
    
    glLinkProgram(program);
    
    /* check link */
    {
        GLint stat;
        glGetProgramiv(program, GL_LINK_STATUS, &stat);
        if (!stat) {
            GLchar log[1000];
            GLsizei len;
            glGetProgramInfoLog(program, 1000, &len, log);
            fprintf(stderr, "Shader link error:\n%s\n", log);
            return 0;
        }
    }
    
    return program;
}

const char * g_fragShaderText = ""
"\n"
"uniform sampler2D tex1;\n"
"uniform sampler3D tex2;\n"
"\n"
"void main()\n"
"{\n"
"    vec4 col = texture2D(tex1, gl_TexCoord[0].st);\n"
"    gl_FragColor = OCIODisplay(col, tex2);\n"
"}\n";


void UpdateOCIOGLState()
{
    // Step 0: Get the processor using any of the pipelines mentioned above.
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    
    OCIO::DisplayTransformRcPtr transform = OCIO::DisplayTransform::Create();
    transform->setInputColorSpaceName( g_inputColorSpace.c_str() );
    transform->setDisplay( g_display.c_str() );
    transform->setView( g_transformName.c_str() );
    
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
    
    // Step 1: Create a GPU Shader Description
    OCIO::GpuShaderDesc shaderDesc;
    shaderDesc.setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_0);
    shaderDesc.setFunctionName("OCIODisplay");
    shaderDesc.setLut3DEdgeLen(LUT3D_EDGE_SIZE);
    
    // Step 2: Compute the 3D LUT
    std::string lut3dCacheID = processor->getGpuLut3DCacheID(shaderDesc);
    if(lut3dCacheID != g_lut3dcacheid)
    {
        //std::cerr << "Computing 3DLut " << g_lut3dcacheid << std::endl;
        
        g_lut3dcacheid = lut3dCacheID;
        processor->getGpuLut3D(&g_lut3d[0], shaderDesc);
        
        glBindTexture(GL_TEXTURE_3D, g_lut3dTexID);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        0, 0, 0, 
                        LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE,
                        GL_RGB,GL_FLOAT, &g_lut3d[0]);
    }
    
    // Step 3: Compute the Shader
    std::string shaderCacheID = processor->getGpuShaderTextCacheID(shaderDesc);
    if(g_program == 0 || shaderCacheID != g_shadercacheid)
    {
        //std::cerr << "Computing Shader " << g_shadercacheid << std::endl;
        
        g_shadercacheid = shaderCacheID;
        
        std::ostringstream os;
        os << processor->getGpuShaderText(shaderDesc) << "\n";
        os << g_fragShaderText;
        //std::cerr << os.str() << std::endl;
        
        if(g_fragShader) glDeleteShader(g_fragShader);
        g_fragShader = CompileShaderText(GL_FRAGMENT_SHADER, os.str().c_str());
        if(g_program) glDeleteProgram(g_program);
        g_program = LinkShaders(g_fragShader);
    }
    
    glUseProgram(g_program);
    glUniform1i(glGetUniformLocation(g_program, "tex1"), 1);
    glUniform1i(glGetUniformLocation(g_program, "tex2"), 2);
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
    
    UpdateOCIOGLState();
    glutPostRedisplay();
}

void transform_CB(int id)
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    
    const char * transform = config->getView(g_display.c_str(), id);
    if(!transform) return;
    
    g_transformName = transform;
    
    UpdateOCIOGLState();
    glutPostRedisplay();
}

static void PopulateOCIOMenus()
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    
    
    int csMenuID = glutCreateMenu(imageColorSpace_CB);
    for(int i=0; i<config->getNumColorSpaces(); ++i)
    {
        glutAddMenuEntry(config->getColorSpaceNameByIndex(i), i);
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
    
    glutCreateMenu(menuCallback);
    glutAddSubMenu("Image ColorSpace", csMenuID);
    glutAddSubMenu("Transform", transformMenuID);
    glutAddSubMenu("Device", deviceMenuID);
    
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
"\tRight-Mouse Button:   Configure Display / Transform / ColorSpace\n"
"\n"
"\tEsc: Quit\n";

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(512, 512);
    glutInitWindowPosition (100, 100);
    
    g_win = glutCreateWindow(argv[0]);
    
#ifndef __APPLE__
    glewInit();
    if (!glewIsSupported("GL_VERSION_2_0"))
    {
        printf("OpenGL 2.0 not supported\n");
        exit(1);
    }
#endif
    
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutSpecialFunc(SpecialKey);
    glutDisplayFunc(Redisplay);
    
    const char * filename = 0;
    if(argc>1) filename = argv[1];
    
    std::cout << USAGE_TEXT << std::endl;
    
    // TODO: switch profiles based on shading language
    // std::cout << "GL_SHADING_LANGUAGE_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    AllocateLut3D();
    
    InitImageTexture(filename);
    try
    {
        InitOCIO(filename);
    }
    catch(OCIO::Exception & e)
    {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    
    PopulateOCIOMenus();
    
    Reshape(1024, 512);
    
    UpdateOCIOGLState();
    
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
