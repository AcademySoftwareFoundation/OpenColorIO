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


#ifndef INCLUDED_OCIO_GLSL_H_
#define INCLUDED_OCIO_GLSL_H_

#include <vector>

#include <OpenColorIO/OpenColorIO.h>


OCIO_NAMESPACE_ENTER
{

class OpenGLBuilder;
typedef OCIO_SHARED_PTR<OpenGLBuilder> OpenGLBuilderRcPtr;


// This is a reference implementation showing how to do the texture upload & allocation, 
// and the program compilation for the GLSL shader language.

class OpenGLBuilder
{
    struct TextureId
    {
        unsigned m_id = -1;
        std::string m_name;
        unsigned m_type = -1;

        TextureId(unsigned id, const std::string& name, unsigned type)
            : m_id(id)
            , m_name(name)
            , m_type(type)
        {}
    };

    typedef std::vector<TextureId> TextureIds;

    // Uniform are used for dynamic parameters.
    class Uniform
    {
    public:
        Uniform(const std::string & name,
                DynamicPropertyRcPtr v)
            : m_name(name)
            , m_value(v)
            , m_handle(0)
        {
        }

        void setUp(unsigned program);

        void use();

    private:
        Uniform() = delete;
        std::string m_name;
        DynamicPropertyRcPtr m_value;
        GLuint m_handle;
    };
    typedef std::vector<Uniform> Uniforms;

public:
    // Create an OpenGL builder using the GPU shader information from a specific processor
    static OpenGLBuilderRcPtr Create(const GpuShaderDescRcPtr & gpuShader);

    ~OpenGLBuilder();

    inline void setVerbose(bool verbose) { m_verbose = verbose; }
    inline bool isVerbose() const { return m_verbose; }

    // Allocate & upload all the needed textures
    //  (i.e. the index is the first available index for any kind of textures).
    void allocateAllTextures(unsigned startIndex);
    void useAllTextures();

    // Update all uniforms.
    void useAllUniforms();

    // Build the complete shader program which includes the OCIO shader program 
    // and the client shader program.
    unsigned buildProgram(const std::string & clientShaderProgram);
    void useProgram();
    unsigned getProgramHandle();

    // Determine the maximum width value of a texture
    // depending of the graphic card and its driver.
    static unsigned GetTextureMaxWidth();

protected:
    OpenGLBuilder(const GpuShaderDescRcPtr & gpuShader);

    // Prepare all the needed uniforms.
    void linkAllUniforms();

    void deleteAllTextures();
    void deleteAllUniforms();

private:
    OpenGLBuilder();
    OpenGLBuilder(const OpenGLBuilder&);
    OpenGLBuilder& operator=(const OpenGLBuilder&);

    const GpuShaderDescRcPtr m_shaderDesc; // Description of the fragment shader to create
    unsigned m_startIndex;                 // Starting index for texture allocations
    TextureIds m_textureIds;               // Texture ids of all needed textures
    Uniforms m_uniforms;                   // Vector of dynamic parameters
    unsigned m_fragShader;                 // Fragment shader identifier
    unsigned m_program;                    // Program identifier
    std::string m_shaderCacheID;           // Current shader program key
    bool m_verbose;                        // Print shader code to std::cout for debugging purposes
};

}
OCIO_NAMESPACE_EXIT

#endif // INCLUDED_OCIO_GLSL_H_

