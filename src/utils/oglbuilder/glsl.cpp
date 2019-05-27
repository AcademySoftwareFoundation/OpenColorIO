/*
Copyright (c) 2018 Autodesk Inc., et al.
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
#endif

#include <sstream>
#include <iostream>

#include <OpenColorIO/OpenColorIO.h>

#include "glsl.h"



OCIO_NAMESPACE_ENTER
{

namespace
{
    bool GetGLError(std::string & error)
    {
        const GLenum glErr = glGetError();
        if(glErr!=GL_NO_ERROR)
        {
#ifdef __APPLE__
            // Unfortunately no gluErrorString equivalent on Mac.
            error = "OpenGL Error";
#else
            error = (const char*)gluErrorString(glErr);
#endif
            return true;
        }
        return false;
    }

    void CheckStatus()
    {
        std::string error;
        if (GetGLError(error))
        {
            throw Exception(error.c_str());
        }
    }

    void SetTextureParameters(GLenum textureType, Interpolation interpolation)
    {
        if(interpolation==INTERP_NEAREST)
        {
            glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        else
        {
            glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        glTexParameteri(textureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(textureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(textureType, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    void AllocateTexture3D(unsigned index, unsigned & texId, 
                           Interpolation interpolation,
                           unsigned edgelen, const float * values)
    {
        if(values==0x0)
        {
            throw Exception("Missing texture data");
        }

        glGenTextures(1, &texId);
        
        glActiveTexture(GL_TEXTURE0 + index);
        
        glBindTexture(GL_TEXTURE_3D, texId);

        SetTextureParameters(GL_TEXTURE_3D, interpolation);

        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F_ARB,
                     edgelen, edgelen, edgelen, 0, GL_RGB, GL_FLOAT, values);
    }

    void AllocateTexture2D(unsigned index, unsigned & texId, unsigned width, unsigned height,
                           Interpolation interpolation, const float * values)
    {
        if(values==0x0)
        {
            throw Exception("Missing texture data");
        }

        glGenTextures(1, &texId);

        glActiveTexture(GL_TEXTURE0 + index);

        if(height>1)
        {
            glBindTexture(GL_TEXTURE_2D, texId);

            SetTextureParameters(GL_TEXTURE_2D, interpolation);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F_ARB, width, height, 0, GL_RGB, GL_FLOAT, values);
        }
        else
        {
            glBindTexture(GL_TEXTURE_1D, texId);

            SetTextureParameters(GL_TEXTURE_1D, interpolation);

            glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F_ARB, width, 0, GL_RGB, GL_FLOAT, values);
        }
    }

    GLuint CompileShaderText(GLenum shaderType, const char * text)
    {
        CheckStatus();

        if(!text || !*text)
        {
            throw Exception("Invalid fragment shader program");
        }

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

            std::string err("OCIO Shader program compilation failed: ");
            err += log;
            err += "\n";
            err += text;

            throw Exception(err.c_str());
        }
        
        return shader;
    }

    void LinkShaders(GLuint program, GLuint fragShader)
    {
        CheckStatus();

        if (!fragShader)
        {
            throw Exception("Missing shader program");
        }
        else        
        {
            glAttachShader(program, fragShader);
        }
        
        glLinkProgram(program);
        
        GLint stat;
        glGetProgramiv(program, GL_LINK_STATUS, &stat);
        if (!stat) 
        {
            GLchar log[1000];
            GLsizei len;
            glGetProgramInfoLog(program, 1000, &len, log);

            std::string err("Shader link error:\n");
            err += log;
            throw Exception(err.c_str());
        }
    }
}


//////////////////////////////////////////////////////////

void OpenGLBuilder::Uniform::setUp(unsigned program)
{
    m_handle = glGetUniformLocation(program, m_name.c_str());

    std::string error;
    if (GetGLError(error))
    {
        std::string err("Shader parameter ");
        err += m_name;
        err += " not found: ";
        throw Exception(err.c_str());
    }
}

void OpenGLBuilder::Uniform::use()
{
    // Update value.
    glUniform1f(m_handle, (GLfloat)m_value->getDoubleValue());
}



//////////////////////////////////////////////////////////

OpenGLBuilderRcPtr OpenGLBuilder::Create(const GpuShaderDescRcPtr & shaderDesc)
{
    return OpenGLBuilderRcPtr(new OpenGLBuilder(shaderDesc));
}

OpenGLBuilder::OpenGLBuilder(const GpuShaderDescRcPtr & shaderDesc)
    :   m_shaderDesc(shaderDesc)
    ,   m_startIndex(0)
    ,   m_fragShader(0)
    ,   m_program(glCreateProgram())
    ,   m_verbose(false)
{
}

OpenGLBuilder::~OpenGLBuilder()
{
    deleteAllTextures();

    if(m_fragShader)
    {
        glDetachShader(m_program, m_fragShader);
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

    // This is the first available index for the textures.
    m_startIndex = startIndex;
    unsigned currIndex = m_startIndex;

    // Process the 3D LUT first.

    const unsigned maxTexture3D = m_shaderDesc->getNum3DTextures();
    for(unsigned idx=0; idx<maxTexture3D; ++idx)
    {
        // 1. Get the information of the 3D LUT.

        const char* name = 0x0;
        const char* uid  = 0x0;
        unsigned edgelen = 0;
        Interpolation interpolation = INTERP_LINEAR;
        m_shaderDesc->get3DTexture(idx, name, uid, edgelen, interpolation);

        if(!name || !*name || !uid || !*uid || edgelen==0)
        {
            throw Exception("The texture data is corrupted");
        }

        const float* values = 0x0;
        m_shaderDesc->get3DTextureValues(idx, values);
        if(!values)
        {
            throw Exception("The texture values are missing");
        }

        // 2. Allocate the 3D LUT.

        unsigned texId = 0;
        AllocateTexture3D(currIndex, texId, interpolation, edgelen, values);

        // 3. Keep the texture id & name for the later enabling.

        m_textureIds.push_back(TextureId(texId, name, GL_TEXTURE_3D));

        currIndex++;
    }

    // Process the 1D LUTs.

    const unsigned maxTexture2D = m_shaderDesc->getNumTextures();
    for(unsigned idx=0; idx<maxTexture2D; ++idx)
    {
        // 1. Get the information of the 1D LUT.

        const char* name = 0x0;
        const char* uid  = 0x0;
        unsigned width = 0;
        unsigned height = 0;
        GpuShaderDesc::TextureType channel = GpuShaderDesc::TEXTURE_RGB_CHANNEL;
        Interpolation interpolation = INTERP_LINEAR;
        m_shaderDesc->getTexture(idx, name, uid, width, height, channel, interpolation);

        if(!name || !*name || !uid || !*uid || width==0)
        {
            throw Exception("The texture data is corrupted");
        }

        const float * values = 0x0;
        m_shaderDesc->getTextureValues(idx, values);
        if(!values)
        {
            throw Exception("The texture values are missing");
        }

        // 2. Allocate the 1D LUT (a 2D texture is needed to hold large LUTs).

        unsigned texId = 0;
        AllocateTexture2D(currIndex, texId, width, height, interpolation, values);

        // 3. Keep the texture id & name for the later enabling.

        unsigned type = (height > 1) ? GL_TEXTURE_2D : GL_TEXTURE_1D;
        m_textureIds.push_back(TextureId(texId, name, type));
        currIndex++;
    }
}

void OpenGLBuilder::deleteAllTextures()
{
    const size_t max = m_textureIds.size();
    for(size_t idx=0; idx<max; ++idx)
    {
        const TextureId& data = m_textureIds[idx];
        glDeleteTextures(1, &data.m_id);
    }

    m_textureIds.clear();
}

void OpenGLBuilder::useAllTextures()
{
    const size_t max = m_textureIds.size();
    for(size_t idx=0; idx<max; ++idx)
    {
        const TextureId& data = m_textureIds[idx];
        glActiveTexture((GLenum)(GL_TEXTURE0 + m_startIndex + idx));
        glBindTexture(data.m_type, data.m_id);
        glUniform1i(
            glGetUniformLocation(m_program, 
                                 data.m_name.c_str()), 
                                 GLint(m_startIndex + idx) );
    }
}

void OpenGLBuilder::linkAllUniforms()
{
    deleteAllUniforms();

    const unsigned maxUniforms = m_shaderDesc->getNumUniforms();
    for (unsigned idx = 0; idx < maxUniforms; ++idx)
    {
        const char * name;
        DynamicPropertyRcPtr value;
        m_shaderDesc->getUniform(idx, name, value);
        // Transfer uniform.
        m_uniforms.emplace_back(name, value);
        // Connect uniform with program.
        m_uniforms.back().setUp(m_program);
    }
}

void OpenGLBuilder::deleteAllUniforms()
{
    m_uniforms.clear();
}

void OpenGLBuilder::useAllUniforms()
{
    for (auto uniform : m_uniforms)
    {
        uniform.use();
    }
}

unsigned OpenGLBuilder::buildProgram(const std::string & clientShaderProgram)
{
    const std::string shaderCacheID = m_shaderDesc->getCacheID();
    if(shaderCacheID!=m_shaderCacheID)
    {
        if(m_fragShader)
        {
            glDetachShader(m_program, m_fragShader);
            glDeleteShader(m_fragShader);
        }

        std::ostringstream os;
        os  << m_shaderDesc->getShaderText() << std::endl
            << clientShaderProgram << std::endl;

        if(m_verbose)
        {
            std::cout << std::endl;
            std::cout << "GPU Shader Program:" << std::endl;
            std::cout << std::endl;
            std::cout << os.str() << std::endl;
            std::cout << std::endl;
        }

        m_fragShader = CompileShaderText(GL_FRAGMENT_SHADER, os.str().c_str());

        LinkShaders(m_program, m_fragShader);
        m_shaderCacheID = shaderCacheID;

        linkAllUniforms();
    }

    return m_program;
}

void OpenGLBuilder::useProgram()
{
    glUseProgram(m_program);
}

unsigned OpenGLBuilder::getProgramHandle()
{
    return m_program;
}

unsigned OpenGLBuilder::GetTextureMaxWidth()
{
    // Arbitrary huge number only to find the limit.
    static unsigned maxTextureSize = 256 * 1024;

    CheckStatus();

    unsigned w = maxTextureSize;
    unsigned h = 1;

    while(w>1)
    {
        glTexImage2D(GL_PROXY_TEXTURE_2D, 0, 
                     GL_RGB32F_ARB, 
                     w, h, 0, 
                     GL_RGB, GL_FLOAT, NULL);

        bool texValid = true;
        GLenum glErr = GL_NO_ERROR;

        while((glErr=glGetError()) != GL_NO_ERROR)
        {
            if(glErr==GL_INVALID_VALUE)
            {
                texValid = false;
            }
        }

#ifndef __APPLE__
        //
        // In case of Linux, if glTexImage2D() succeeds
        //  glGetTexLevelParameteriv() could fail.
        //
        // In case of OSX, glTexImage2D() will provide the right result,
        //  and glGetTexLevelParameteriv() always fails.
        //  So do not try glGetTexLevelParameteriv().
        //
        if(texValid)
        {
            GLint format = 0;
            glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0,
                                     GL_TEXTURE_COMPONENTS, &format);

            texValid = texValid && (GL_RGB32F_ARB==format);

            while((glErr=glGetError()) != GL_NO_ERROR);
        }
#endif

        if(texValid) break;

        w = w >> 1;
        h = h << 1;
    }

    if(w==1)
    {
        throw Exception("Maximum texture size unknown");
    }

    CheckStatus();

    return w;
}

}
OCIO_NAMESPACE_EXIT
