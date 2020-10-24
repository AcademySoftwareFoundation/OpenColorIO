// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "MathUtils.h"
#include "ops/lut3d/Lut3DOpGPU.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

void GetLut3DGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator, ConstLut3DOpDataRcPtr & lutData)
{

    std::ostringstream resName;
    resName << shaderCreator->getResourcePrefix()
            << std::string("_")
            << std::string("lut3d_")
            << shaderCreator->getNextResourceIndex();

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    std::string name(resName.str());
    StringUtils::ReplaceInPlace(name, "__", "_");

    // (Using CacheID here to potentially allow reuse of existing textures.)
    shaderCreator->add3DTexture(name.c_str(),
                                GpuShaderText::getSamplerName(name).c_str(),
                                lutData->getGridSize(),
                                lutData->getConcreteInterpolation(),
                                &lutData->getArray()[0]);

    {
        GpuShaderText ss(shaderCreator->getLanguage());
        ss.declareTex3D(name);
        shaderCreator->addToDeclareShaderCode(ss.string().c_str());
    }


    const float dim = (float)lutData->getGridSize();

    // incr = 1/dim (amount needed to increment one index in the grid)
    const float incr = 1.0f / dim;

    {
        GpuShaderText ss(shaderCreator->getLanguage());
        ss.indent();

        ss.newLine() << "";
        ss.newLine() << "// Add a LUT 3D processing for " << name;
        ss.newLine() << "";


        // Tetrahedral interpolation
        // The strategy is to use texture3d lookups with GL_NEAREST to fetch the
        // 4 corners of the cube (v1,v2,v3,v4), compute the 4 barycentric weights
        // (f1,f2,f3,f4), and then perform the interpolation manually.
        // One side benefit of this is that we are not subject to the 8-bit
        // quantization of the fractional weights that happens using GL_LINEAR.
        if (lutData->getConcreteInterpolation() == INTERP_TETRAHEDRAL)
        {
            ss.newLine() << "{";
            ss.indent();

            ss.newLine() << ss.float3Decl("coords") << " = "
                         << shaderCreator->getPixelName() << ".rgb * "
                         << ss.float3Const(dim - 1) << "; ";

            // baseInd is on [0,dim-1]
            ss.newLine() << ss.float3Decl("baseInd") << " = floor(coords);";

            // frac is on [0,1]
            ss.newLine() << ss.float3Decl("frac") << " = coords - baseInd;";

            // scale/offset baseInd onto [0,1] as usual for doing texture lookups
            // we use zyx to flip the order since blue varies most rapidly
            // in the grid array ordering
            ss.newLine() << ss.float3Decl("f1, f4") << ";";

            ss.newLine() << "baseInd = ( baseInd.zyx + " << ss.float3Const(0.5f) << " ) / " << ss.float3Const(dim) << ";";
            ss.newLine() << ss.float3Decl("v1") << " = " << ss.sampleTex3D(name, "baseInd") << ".rgb;";

            ss.newLine() << ss.float3Decl("nextInd") << " = baseInd + " << ss.float3Const(incr) << ";";
            ss.newLine() << ss.float3Decl("v4") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "if (frac.r >= frac.g)";
            ss.newLine() << "{";
            ss.indent();
            ss.newLine() << "if (frac.g >= frac.b)";  // R > G > B
            ss.newLine() << "{";
            ss.indent();
            // Note that compared to the CPU version of the algorithm,
            // we increment in inverted order since baseInd & nextInd
            // are essentially BGR rather than RGB.
            ss.newLine() << "nextInd = baseInd + " << ss.float3Const(0.0f, 0.0f, incr) << ";";
            ss.newLine() << ss.float3Decl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "nextInd = baseInd + " << ss.float3Const(0.0f, incr, incr) << ";";
            ss.newLine() << ss.float3Decl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "f1 = " << ss.float3Const("1. - frac.r") << ";";
            ss.newLine() << "f4 = " << ss.float3Const("frac.b") << ";";
            ss.newLine() << ss.float3Decl("f2") << " = " << ss.float3Const("frac.r - frac.g") << ";";
            ss.newLine() << ss.float3Decl("f3") << " = " << ss.float3Const("frac.g - frac.b") << ";";

            ss.newLine() << shaderCreator->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
            ss.dedent();
            ss.newLine() << "}";
            ss.newLine() << "else if (frac.r >= frac.b)";  // R > B > G
            ss.newLine() << "{";
            ss.indent();
            ss.newLine() << "nextInd = baseInd + " << ss.float3Const(0.0f, 0.0f, incr) << ";";
            ss.newLine() << ss.float3Decl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "nextInd = baseInd + " << ss.float3Const(incr, 0.0f, incr) << ";";
            ss.newLine() << ss.float3Decl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "f1 = " << ss.float3Const("1. - frac.r") << ";";
            ss.newLine() << "f4 = " << ss.float3Const("frac.g") << ";";
            ss.newLine() << ss.float3Decl("f2") << " = " << ss.float3Const("frac.r - frac.b") << ";";
            ss.newLine() << ss.float3Decl("f3") << " = " << ss.float3Const("frac.b - frac.g") << ";";

            ss.newLine() << shaderCreator->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
            ss.dedent();
            ss.newLine() << "}";
            ss.newLine() << "else";  // B > R > G
            ss.newLine() << "{";
            ss.indent();
            ss.newLine() << "nextInd = baseInd + " << ss.float3Const(incr, 0.0f, 0.0f) << ";";
            ss.newLine() << ss.float3Decl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "nextInd = baseInd + " << ss.float3Const(incr, 0.0f, incr) << ";";
            ss.newLine() << ss.float3Decl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "f1 = " << ss.float3Const("1. - frac.b") << ";";
            ss.newLine() << "f4 = " << ss.float3Const("frac.g") << ";";
            ss.newLine() << ss.float3Decl("f2") << " = " << ss.float3Const("frac.b - frac.r") << ";";
            ss.newLine() << ss.float3Decl("f3") << " = " << ss.float3Const("frac.r - frac.g") << ";";

            ss.newLine() << shaderCreator->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
            ss.dedent();
            ss.newLine() << "}";
            ss.dedent();
            ss.newLine() << "}";
            ss.newLine() << "else";
            ss.newLine() << "{";
            ss.indent();
            ss.newLine() << "if (frac.g <= frac.b)";  // B > G > R
            ss.newLine() << "{";
            ss.indent();
            ss.newLine() << "nextInd = baseInd + " << ss.float3Const(incr, 0.0f, 0.0f) << ";";
            ss.newLine() << ss.float3Decl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "nextInd = baseInd + " << ss.float3Const(incr, incr, 0.0f) << ";";
            ss.newLine() << ss.float3Decl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "f1 = " << ss.float3Const("1. - frac.b") << ";";
            ss.newLine() << "f4 = " << ss.float3Const("frac.r") << ";";
            ss.newLine() << ss.float3Decl("f2") << " = " << ss.float3Const("frac.b - frac.g") << ";";
            ss.newLine() << ss.float3Decl("f3") << " = " << ss.float3Const("frac.g - frac.r") << ";";

            ss.newLine() << shaderCreator->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
            ss.dedent();
            ss.newLine() << "}";
            ss.newLine() << "else if (frac.r >= frac.b)";  // G > R > B
            ss.newLine() << "{";
            ss.indent();
            ss.newLine() << "nextInd = baseInd + " << ss.float3Const(0.0f, incr, 0.0f) << ";";
            ss.newLine() << ss.float3Decl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "nextInd = baseInd + " << ss.float3Const(0.0f, incr, incr) << ";";
            ss.newLine() << ss.float3Decl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "f1 = " << ss.float3Const("1. - frac.g") << ";";
            ss.newLine() << "f4 = " << ss.float3Const("frac.b") << ";";
            ss.newLine() << ss.float3Decl("f2") << " = " << ss.float3Const("frac.g - frac.r") << ";";
            ss.newLine() << ss.float3Decl("f3") << " = " << ss.float3Const("frac.r - frac.b") << ";";

            ss.newLine() << shaderCreator->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
            ss.dedent();
            ss.newLine() << "}";
            ss.newLine() << "else";  // G > B > R
            ss.newLine() << "{";
            ss.indent();
            ss.newLine() << "nextInd = baseInd + " << ss.float3Const(0.0f, incr, 0.0f) << ";";
            ss.newLine() << ss.float3Decl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "nextInd = baseInd + " << ss.float3Const(incr, incr, 0.0f) << ";";
            ss.newLine() << ss.float3Decl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "f1 = " << ss.float3Const("1. - frac.g") << ";";
            ss.newLine() << "f4 = " << ss.float3Const("frac.r") << ";";
            ss.newLine() << ss.float3Decl("f2") << " = " << ss.float3Const("frac.g - frac.b") << ";";
            ss.newLine() << ss.float3Decl("f3") << " = " << ss.float3Const("frac.b - frac.r") << ";";

            ss.newLine() << shaderCreator->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
            ss.dedent();
            ss.newLine() << "}";
            ss.dedent();
            ss.newLine() << "}";

            ss.newLine() << shaderCreator->getPixelName()
                         << ".rgb = "
                         << shaderCreator->getPixelName()
                         << ".rgb + (f1 * v1) + (f4 * v4);";

            ss.dedent();
            ss.newLine() << "}";
        }
        else
        {
            // Trilinear interpolation
            // Use texture3d and GL_LINEAR and the GPU's built-in trilinear algorithm.
            // Note that the fractional components are quantized to 8-bits on some
            // hardware, which introduces significant error with small grid sizes.

            ss.newLine() << ss.float3Decl(name + "_coords")
                         << " = (" << shaderCreator->getPixelName() << ".zyx * "
                         << ss.float3Const(dim - 1) << " + "
                         << ss.float3Const(0.5f) + ") / "
                         << ss.float3Const(dim) << ";";

            ss.newLine() << shaderCreator->getPixelName() << ".rgb = "
                         << ss.sampleTex3D(name, name + "_coords") << ".rgb;";
        }

        shaderCreator->addToFunctionShaderCode(ss.string().c_str());
    }
}


} // namespace OCIO_NAMESPACE
