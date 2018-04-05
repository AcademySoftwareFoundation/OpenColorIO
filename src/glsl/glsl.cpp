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
    void SetTextureParameters(GLenum textureType, OCIO::Interpolation interpolation)
    {
        if(interpolation==OCIO::INTERP_LINEAR || interpolation==OCIO::INTERP_BEST)
        {
            glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else
        {
            glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        glTexParameteri(textureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(textureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(textureType, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    void AllocateTexture3D(unsigned index, unsigned& texId, unsigned edgelen, const float* values)
    {
        glGenTextures(1, &texId);
        
        const unsigned num3Dentries = 3*edgelen*edgelen*edgelen;

        glActiveTexture(GL_TEXTURE0 + index);
        
        glBindTexture(GL_TEXTURE_3D, texId);

        SetTextureParameters(GL_TEXTURE_3D, OCIO::INTERP_LINEAR);

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

            SetTextureParameters(GL_TEXTURE_2D, interpolation);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, values);
        }
        else
        {
            glBindTexture(GL_TEXTURE_1D, texId);

            SetTextureParameters(GL_TEXTURE_1D, interpolation);

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
        
        GLint stat;
        glGetProgramiv(program, GL_LINK_STATUS, &stat);
        if (!stat) 
        {
            GLchar log[1000];
            GLsizei len;
            glGetProgramInfoLog(program, 1000, &len, log);

            fprintf(stderr, "Shader link error:\n%s\n", log);
            return 0;
        }
        
        return program;
    }
}


OpenGLBuilderRcPtr OpenGLBuilder::Create(const OCIO::GpuShaderDescRcPtr & shaderDesc)
{
    return OpenGLBuilderRcPtr(new OpenGLBuilder(shaderDesc));
}

OpenGLBuilder::OpenGLBuilder(const OCIO::GpuShaderDescRcPtr & shaderDesc)
    :   m_shaderDesc(shaderDesc)
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

    // Process the 3D Luts first

    const unsigned maxTexture3D = m_shaderDesc->getNum3DTextures();
    for(unsigned idx=0; idx<maxTexture3D; ++idx)
    {
        // 1. Get the information of the 3D lut

        const char* name = 0x0;
        const char* uid  = 0x0;
        unsigned edgelen = 0;
        m_shaderDesc->get3DTexture(idx, name, uid, edgelen);

        const float* values = 0x0;
        m_shaderDesc->get3DTextureValues(idx, values);

        // 2. Allocate the 3D lut

        unsigned texId = 0;
        AllocateTexture3D(currIndex, texId, edgelen, values);

        // 3. Keep the texture id & name for the later enabling

        m_textureIds.push_back(std::make_pair(texId, name));

        currIndex++;
    }

    // Process the 1D luts

    const unsigned maxTexture2D = m_shaderDesc->getNumTextures();
    for(unsigned idx=0; idx<maxTexture2D; ++idx)
    {
        // 1. Get the information of the 1D lut

        const char* name = 0x0;
        const char* uid  = 0x0;
        unsigned width = 0;
        unsigned height = 0;
        OCIO::GpuShaderDesc::TextureType channel = OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL;
        OCIO::Interpolation interpolation = OCIO::INTERP_LINEAR;
        m_shaderDesc->getTexture(idx, name, uid, width, height, channel, interpolation);

        const float* red   = 0x0;
        const float* green = 0x0;
        const float* blue  = 0x0;
        m_shaderDesc->getTextureValues(idx, red, green, blue);

        // 2. Allocate the 1D lut (a 2D texture is needed to hold large luts)

        unsigned texId = 0;
        AllocateTexture2D(currIndex, texId, width, height, interpolation, red);

        // 3. Keep the texture id & name for the later enabling

        m_textureIds.push_back(std::make_pair(texId, name));
        currIndex++;

        // 4. Repeat for the green and blue if needed

        if(channel==OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL)
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
        glUniform1i(
            glGetUniformLocation(m_program, 
                                 data.second.c_str()), 
                                 GLint(m_startIndex + idx) );
    }
}

unsigned OpenGLBuilder::buildProgram(const std::string & clientShaderProgram)
{
    std::ostringstream os;
    os << m_shaderDesc->getShaderText() << "\n";
    os << clientShaderProgram;

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