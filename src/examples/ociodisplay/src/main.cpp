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

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glut.h>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
namespace OIIO = OpenImageIO;



GLint g_win = 0;
GLuint g_fragShader = 0;
GLuint g_program = 0;

GLuint g_imageTexID;

GLuint g_lut3dTexID;
const int LUT3D_EDGE_SIZE = 32;
std::vector<float> g_lut3d;
std::string g_lut3dcacheid;

std::string g_inputColorSpace;
std::string g_device;
std::string g_transformName;

float g_exposure_fstop = 0.0f;
int g_channelHot[4] = { 1, 1, 1, 1 };  // show rgb


void UpdateDrawState();

static void
InitImageTexture(const char * filename)
{
    glGenTextures(1, &g_imageTexID);
    
    std::vector<float> img;
    int texWidth = 1024;
    int texHeight = 512;
    int components = 4;
    
    if(filename)
    {
        std::cerr << "Loading " << filename << std::endl;
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
            
            f->read_image(TypeDesc::TypeFloat, &img[0]);
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
        std::cerr << "No image specified, loading gradient." << std::endl;
        
        img.resize(texWidth*texHeight*components);
        memset(&img[0], 0, texWidth*texHeight*components*sizeof(float));
        
        for(int y=0; y<texHeight; ++y)
        {
            for(int x=0; x<texWidth; ++x)
            {
                float c = x/(texWidth-1.0);
                img[components*(texWidth*y+x) + 0] = c;
                img[components*(texWidth*y+x) + 1] = c;
                img[components*(texWidth*y+x) + 2] = c;
                img[components*(texWidth*y+x) + 3] = 1.0;
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
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_imageTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0,
        format, GL_FLOAT, &img[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

void InitOCIO(const char * filename)
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    g_device = config->getDefaultDisplayDeviceName();
    g_transformName = config->getDefaultDisplayTransformName(g_device.c_str());
    
    const char * cs = config->parseColorSpaceFromString(filename);
    if(!cs) cs = OCIO::ROLE_SCENE_LINEAR;
    g_inputColorSpace = std::string(cs);
    
    std::cerr << "inputColorSpace " << g_inputColorSpace << std::endl;
}


static void
AllocateLut3D()
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
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB,
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

static void
Redisplay(void)
{
    glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glColor3f(1, 1, 1);
    
    glPushMatrix();

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(0.0f, 0.0f);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0.0f, 1.0f);

    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(1.0f, 1.0f);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1.0f, 0.0f);

    glEnd();

    glPopMatrix();
    
    glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -100.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


static void
CleanUp(void)
{
    glDeleteShader(g_fragShader);
    glDeleteProgram(g_program);
    glutDestroyWindow(g_win);
}


static void
Key(unsigned char key, int x, int y)
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
    
    UpdateDrawState();
    
    glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
    (void) x;
    (void) y;
    
    int mod = glutGetModifiers();
    
    if(key == GLUT_KEY_UP && (mod & GLUT_ACTIVE_CTRL))
    {
        g_exposure_fstop += 0.25;
    }
    else if(key == GLUT_KEY_DOWN && (mod & GLUT_ACTIVE_CTRL))
    {
        g_exposure_fstop -= 0.25;
    }
    else if(key == GLUT_KEY_HOME && (mod & GLUT_ACTIVE_CTRL))
    {
        g_exposure_fstop = 0.0;
    }
    
    UpdateDrawState();
    
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
"#extension GL_EXT_gpu_shader4 : enable\n"
"#extension GL_ARB_texture_rectangle : enable\n"
"\n"
"uniform sampler2D tex1;\n"
"uniform sampler3D tex2;\n"
"\n"
"void main()\n"
"{\n"
"    vec4 col = texture2D(tex1, gl_TexCoord[0].st);\n"
"    gl_FragColor = OCIODisplay(col, tex2);\n"
"}\n";


void UpdateDrawState()
{
    std::string ocioShaderText = "";
    
    // Step 0: Get the processor using any of the pipelines mentioned above.
    
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    const char * displayColorSpace = config->getDisplayColorSpaceName(g_device.c_str(), g_transformName.c_str());
    
    OCIO::DisplayTransformRcPtr transform = OCIO::DisplayTransform::Create();
    transform->setInputColorSpaceName( g_inputColorSpace.c_str() );
    transform->setDisplayColorSpaceName( displayColorSpace );
    
    // Add custom (optional) transforms for our 'canonical' display pipeline
    {
        // Add an fstop exposure control (in SCENE_LINEAR)
        float gain = powf(2.0f, g_exposure_fstop);
        const float slope3f[] = { gain, gain, gain };
        OCIO::CDLTransformRcPtr cc =  OCIO::CDLTransform::Create();
        cc->setSlope(slope3f);
        transform->setLinearCC(cc);
        
        // Add Channel swizzling
        float lumacoef[3];
        config->getDefaultLumaCoefs(lumacoef);
        
        float m44[16];
        float offset[4];
        OCIO::MatrixTransform::View(m44, offset, g_channelHot, lumacoef);
        
        OCIO::MatrixTransformRcPtr swizzle = OCIO::MatrixTransform::Create();
        swizzle->setValue(m44, offset);
        transform->setChannelView(swizzle);
    }
    
    OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform);
    
    // Step 1: Create a GPU Shader Description
    OCIO::GpuShaderDesc shaderDesc;
    shaderDesc.setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_0);
    shaderDesc.setFunctionName("OCIODisplay");
    shaderDesc.setLut3DEdgeLen(LUT3D_EDGE_SIZE);
    
    // Step 2: Compute the 3D LUT
    
    std::string lut3dCacheID = processor->getGpuLut3DCacheID(shaderDesc);
    if(lut3dCacheID != g_lut3dcacheid)
    {
        g_lut3dcacheid = lut3dCacheID;
        //std::cerr << "Computing 3DLut " << g_lut3dcacheid << std::endl;
        
        processor->getGpuLut3D(&g_lut3d[0], shaderDesc);
        
        glBindTexture(GL_TEXTURE_3D, g_lut3dTexID);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        0, 0, 0, 
                        LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE,
                        GL_RGB,GL_FLOAT, &g_lut3d[0]);
    }
    
    std::ostringstream os;
    os << processor->getGpuShaderText(shaderDesc) << "\n";
    os << g_fragShaderText;
    
    /*
    // Print the shader text
    std::cerr << std::endl;
    std::cerr << os.str() << std::endl;
    */
    
    // TODO: Cleanup shader text?
    g_fragShader = CompileShaderText(GL_FRAGMENT_SHADER, os.str().c_str());
    g_program = LinkShaders(g_fragShader);
    
    glUseProgram(g_program);
    
    glUniform1i(glGetUniformLocation(g_program, "tex1"), 1);
    glUniform1i(glGetUniformLocation(g_program, "tex2"), 2);
}

const char * USAGE_TEXT = "\n"
"Keys:\n"
"\tCtrl+Up:   Exposure +1/4 stop (in scene linear)\n"
"\tCtrl+Down: Exposure -1/4 stop (in scene linear)\n"
"\tCtrl+Home: Reset Exposure\n"
"\n"
"\tC:   View Color\n"
"\tR:   View Red  \n"
"\tG:   View Green\n"
"\tB:   View Blue\n"
"\tA:   View Alpha\n"
"\tL:   View Luma\n"
"\n"
"\tEsc: Quit\n";

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    
    glutInitWindowSize(1024, 512);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    g_win = glutCreateWindow(argv[0]);
    
    glewInit();
    if (!glewIsSupported("GL_VERSION_2_0"))
    {
        printf("OpenGL 2.0 not supported\n");
        exit(1);
    }
    
    if (!GLEW_ARB_texture_rectangle)
    {
        printf("No ARB_texture_rectangle\n");
        exit(1);
    }
    
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutSpecialFunc(SpecialKey);
    glutDisplayFunc(Redisplay);
    
    const char * filename = 0;
    if(argc>1) filename = argv[1];
    
    std::cerr << USAGE_TEXT << std::endl;
    
    AllocateLut3D();
    
    InitImageTexture(filename);
    InitOCIO(filename);
    
    Reshape(1024, 512);
    
    UpdateDrawState();
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
