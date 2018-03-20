/*
    Made by Autodesk Inc. under the terms of the OpenColorIO BSD 3 Clause License
*/


#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#elif _WIN32
#include <GL/glew.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#endif


#include "glsl.h"

#include <sstream>


namespace
{
    void AllocateTexture3D(unsigned index, unsigned& texId, unsigned edgelen, const float* values)
    {
        glGenTextures(1, &texId);
        
        const unsigned num3Dentries = 3*edgelen*edgelen*edgelen;

        glActiveTexture(GL_TEXTURE0 + index);
        
        glBindTexture(GL_TEXTURE_3D, texId);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB,
                     edgelen, edgelen, edgelen, 0, GL_RGB, GL_FLOAT, values);
    }

    void AllocateTexture2D(unsigned index, unsigned& texId, unsigned width, unsigned height,
        OCIO::Interpolation interpolation, const float* values)
    {
        glGenTextures(1, &texId);

        glActiveTexture(GL_TEXTURE0 + index);

        if(height>1)
        {
            glBindTexture(GL_TEXTURE_2D, texId);

            if(interpolation==OCIO::INTERP_LINEAR || interpolation==OCIO::INTERP_BEST)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, values);
        }
        else
        {
            glBindTexture(GL_TEXTURE_1D, texId);

            if(interpolation==OCIO::INTERP_LINEAR || interpolation==OCIO::INTERP_BEST)
            {
                glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            else
            {
                glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            glTexImage1D(GL_TEXTURE_1D, 0, GL_R16F, width, 0, GL_RED, GL_FLOAT, values);
        }
    }

    GLuint CompileShaderText(GLenum shaderType, const char *text)
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

    GLuint LinkShaders(GLuint fragShader)
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
}


OpenGLBuilderRcPtr OpenGLBuilder::Create(OCIO::ConstGpuShaderRcPtr& gpuShader)
{
    return OpenGLBuilderRcPtr(new OpenGLBuilder(gpuShader));
}

OpenGLBuilder::OpenGLBuilder(OCIO::ConstGpuShaderRcPtr& gpuShader)
    :   m_gpuShader(gpuShader)
    ,   m_startIndex(0)
    ,   m_fragShader(0)
    ,   m_program(0)
{
}

OpenGLBuilder::~OpenGLBuilder()
{
    if(m_fragShader)
    {
        glDeleteShader(m_fragShader);
        m_fragShader = 0;
    }

    if(m_program)
    {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

void OpenGLBuilder::allocateAllTextures(unsigned startIndex)
{
    deleteAllTextures();

    m_startIndex = startIndex + 1;
    unsigned currIndex = m_startIndex;

    const unsigned maxTexture3D = m_gpuShader->getNum3DTextures();
    for(unsigned idx=0; idx<maxTexture3D; ++idx)
    {
        const char* name = 0x0;
        const char* uid  = 0x0;
        unsigned edgelen = 0;
        m_gpuShader->get3DTexture(idx, name, uid, edgelen);

        const float* values = 0x0;
        m_gpuShader->get3DTextureValues(idx, values);

        unsigned texId = 0;
        AllocateTexture3D(currIndex, texId, edgelen, values);

        m_textureIds.push_back(std::make_pair(texId, name));

        currIndex++;
    }

    const unsigned maxTexture2D = m_gpuShader->getNumTextures();
    for(unsigned idx=0; idx<maxTexture2D; ++idx)
    {
        const char* name = 0x0;
        const char* uid  = 0x0;
        unsigned width = 0;
        unsigned height = 0;
        OCIO::GpuShader::TextureType channel = OCIO::GpuShader::TEXTURE_RGB_CHANNEL;
        OCIO::Interpolation interpolation = OCIO::INTERP_LINEAR;
        m_gpuShader->getTexture(idx, name, uid, width, height, channel, interpolation);

        const float* red   = 0x0;
        const float* green = 0x0;
        const float* blue  = 0x0;
        m_gpuShader->getTextureValues(idx, red, green, blue);

        unsigned texId = 0;
        AllocateTexture2D(currIndex, texId, width, height, interpolation, red);
        m_textureIds.push_back(std::make_pair(texId, name));
        currIndex++;

        if(channel==OCIO::GpuShader::TEXTURE_RGB_CHANNEL)
        {
            AllocateTexture2D(currIndex, texId, width, height, interpolation, green);
            m_textureIds.push_back(std::make_pair(texId, name));
            currIndex++;

            AllocateTexture2D(currIndex, texId, width, height, interpolation, blue);
            m_textureIds.push_back(std::make_pair(texId, name));
            currIndex++;
        }
    }
}

void OpenGLBuilder::deleteAllTextures()
{
    const size_t max = m_textureIds.size();
    for(size_t idx=0; idx<max; ++idx)
    {
        const std::pair<unsigned, std::string> data = m_textureIds[idx];
        glDeleteTextures(1, &data.first);
    }

    m_textureIds.clear();
}

void OpenGLBuilder::useAllTextures()
{
    const size_t max = m_textureIds.size();
    for(size_t idx=0; idx<max; ++idx)
    {
        const std::pair<unsigned, std::string> data = m_textureIds[idx];
        glUniform1i(glGetUniformLocation(m_program, data.second.c_str()), m_startIndex + idx);
    }
}

unsigned OpenGLBuilder::buildProgram(const std::string& fragMainMethod)
{
    std::ostringstream os;
    os << m_gpuShader->getShaderText() << "\n";
    os << fragMainMethod;

    if(m_fragShader) glDeleteShader(m_fragShader);
    m_fragShader = CompileShaderText(GL_FRAGMENT_SHADER, os.str().c_str());
    if(m_program) glDeleteProgram(m_program);
    m_program = LinkShaders(m_fragShader);   

    return m_program;
}

void OpenGLBuilder::useProgram()
{
    glUseProgram(m_program);
}