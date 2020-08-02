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
                return OpenGLBuilder::Create(gpuShader);
            }),
             "gpuShader"_a)

        .def("setVerbose", &OpenGLBuilder::setVerbose, "verbose"_a)
        .def("isVerbose", &OpenGLBuilder::isVerbose)
        .def("allocateAllTextures", &OpenGLBuilder::allocateAllTextures, "startIndex"_a)
        .def("useAllTextures", &OpenGLBuilder::useAllTextures)
        .def("useAllUniforms", &OpenGLBuilder::useAllUniforms)
        .def("buildProgram", &OpenGLBuilder::buildProgram, "clientShaderProgram"_a)
        .def("useProgram", &OpenGLBuilder::useProgram)
        .def("getProgramHandle", &OpenGLBuilder::getProgramHandle);
}

} // namespace OCIO_NAMESPACE
