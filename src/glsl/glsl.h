/*
    Made by Autodesk Inc. under the terms of the OpenColorIO BSD 3 Clause License
*/


#ifndef INCLUDED_OCIO_GLSL_H_
#define INCLUDED_OCIO_GLSL_H_

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;


#include <vector>


class OpenGLBuilder;
typedef OCIO_SHARED_PTR<OpenGLBuilder> OpenGLBuilderRcPtr;



class OpenGLBuilder
{
    typedef std::vector<std::pair<unsigned, std::string>> TextureIds;

public:
    // Create an OpenGL builder using the GPU shader information from a specific processor
    static OpenGLBuilderRcPtr Create(OCIO::ConstGpuShaderRcPtr& gpuShader);

    ~OpenGLBuilder();

    // Allocate all the needed textures starting at the (zero based) index 1 by default
    //  (e.g. the first index is reserved for the input image to process)
    void allocateAllTextures(unsigned startIndex = 1);
    void useAllTextures();

    // Build the shader fragment program
    unsigned buildProgram(const std::string& fragMainMethod);
    void useProgram();

protected:
    OpenGLBuilder(OCIO::ConstGpuShaderRcPtr& gpuShader);

    void deleteAllTextures();

private:
    OCIO::ConstGpuShaderRcPtr m_gpuShader;
    unsigned m_startIndex;
    TextureIds m_textureIds;
    unsigned m_fragShader;
    unsigned m_program;
};


#endif // INCLUDED_OCIO_GLSL_H_

