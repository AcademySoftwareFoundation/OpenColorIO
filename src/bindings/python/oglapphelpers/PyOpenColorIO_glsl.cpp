// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "glsl.h"

#include "PyOpenColorIO_oglapphelpers.h"

namespace OCIO_NAMESPACE
{
void bindPyGLSL(py::module & m)
{
    auto cls = py::class_<OpenGLBuilder, OpenGLBuilderRcPtr /* holder */>(m, "OpenGLBuilder")
            .def(py::init([](const GpuShaderDescRcPtr & gpuShader)
                {
                    return OpenGLBuilderRcPtr::Create(gpuShader);
                }),
                "gpuShader"_a)

            .def("setVerbose", &OpenGLBuilder::setVerbose, "verbose"_a)
            .def("isVerbose", &OpenGLBuilder::isVerbose)
            .def("allocateAllTextures", &OpenGLBuilder::allocateAllTextures, "startIndex"_a)
            .def("useAllTextures", &OpenGLBuilder::useAllTextures)
            .def("useAllUniforms", &OpenGLBuilder::useAllUniforms)
            .def("buildProgram", &OpenGLBuilder::buildProgram, "clientShaderProgram"_a)
            .def("useProgram", &OpenGLBuilder::useProgram)
            .def("getProgramHandle", &OpenGLBuilder::getProgramHandle)
            .def_static("GetTextureMaxWidth", &OpenGLBuilder::GetTextureMaxWidth)
            .def("linkAllUniforms", &OpenGLBuilder::linkAllUniforms)
            .def("deleteAllTextures", &OpenGLBuilder::deleteAllTextures)
            .def("deleteAllUniforms", &OpenGLBuilder::deleteAllUniforms);

    py::class_<OpenGLBuilder::TextureId>(cls, "TextureID")
            .def(py::init([](unsigned uid,
                             const std::string & textureName,
                             const std::string & samplerName,
                             unsigned type)
    {

    }),
                "uid"_a,
                "textureName"_a,
                "samplerName"_a,
                "type"_a);

    py::class_<OpenGLBuilder::Uniform>(oglbuilder, "Uniform")
            .def(py::init([](const std::string & name,
                          DynamicProperty v),
                          "name"_a,
                          "v"_a))

            .def("setUp", &OpenGLBuilder::Uniform::setUp, "program"_a)
            .def("use", &OpenGLBuilder::Uniform::use)
            .def("getValue", &OpenGLBuilder::Uniform::getValue);


}

} // namespace OCIO_NAMESPACE
