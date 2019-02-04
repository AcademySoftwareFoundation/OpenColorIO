/*
Copyright (c) 2019 Autodesk Inc., et al.
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

#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "MathUtils.h"
#include "ops/Lut3D/Lut3DOpGPU.h"

OCIO_NAMESPACE_ENTER
{

void GetLut3DGPUShaderProgram(GpuShaderDescRcPtr & shaderDesc,
                              ConstLut3DOpDataRcPtr & lutData)
{

    std::ostringstream resName;
    resName << shaderDesc->getResourcePrefix()
            << std::string("lut3d_")
            << shaderDesc->getNum3DTextures();

    const std::string name(resName.str());

    shaderDesc->add3DTexture(GpuShaderText::getSamplerName(name).c_str(),
        lutData->getCacheID().c_str(), lutData->getGridSize(),
        lutData->getConcreteInterpolation(), &lutData->getArray()[0]);

    {
        GpuShaderText ss(shaderDesc->getLanguage());
        ss.declareTex3D(name);
        shaderDesc->addToDeclareShaderCode(ss.string().c_str());
    }


    const float dim = (float)lutData->getGridSize();

    // incr = 1/dim (amount needed to increment one index in the grid)
    const float incr = 1.0f / dim;

    {
        GpuShaderText ss(shaderDesc->getLanguage());
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

            ss.newLine() << ss.vec3fDecl("coords") << " = "
                         << shaderDesc->getPixelName() << ".rgb * "
                         << ss.vec3fConst(dim - 1) << "; ";

            // baseInd is on [0,dim-1]
            ss.newLine() << ss.vec3fDecl("baseInd") << " = floor(coords);";

            // frac is on [0,1]
            ss.newLine() << ss.vec3fDecl("frac") << " = coords - baseInd;";

            // scale/offset baseInd onto [0,1] as usual for doing texture lookups
            // we use zyx to flip the order since blue varies most rapidly
            // in the grid array ordering
            ss.newLine() << ss.vec3fDecl("f1, f4") << ";";

            ss.newLine() << "baseInd = ( baseInd.zyx + " << ss.vec3fConst(0.5f) << " ) / " << ss.vec3fConst(dim) << ";";
            ss.newLine() << ss.vec3fDecl("v1") << " = " << ss.sampleTex3D(name, "baseInd") << ".rgb;";

            ss.newLine() << ss.vec3fDecl("nextInd") << " = baseInd + " << ss.vec3fConst(incr) << ";";
            ss.newLine() << ss.vec3fDecl("v4") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "if (frac.r >= frac.g)";
            ss.newLine() << "{";
            ss.indent();
            ss.newLine() << "if (frac.g >= frac.b)";  // R > G > B
            ss.newLine() << "{";
            ss.indent();
            // Note that compared to the CPU version of the algorithm,
            // we increment in inverted order since baseInd & nextInd
            // are essentially BGR rather than RGB.
            ss.newLine() << "nextInd = baseInd + " << ss.vec3fConst(0.0f, 0.0f, incr) << ";";
            ss.newLine() << ss.vec3fDecl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "nextInd = baseInd + " << ss.vec3fConst(0.0f, incr, incr) << ";";
            ss.newLine() << ss.vec3fDecl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "f1 = " << ss.vec3fConst("1. - frac.r") << ";";
            ss.newLine() << "f4 = " << ss.vec3fConst("frac.b") << ";";
            ss.newLine() << ss.vec3fDecl("f2") << " = " << ss.vec3fConst("frac.r - frac.g") << ";";
            ss.newLine() << ss.vec3fDecl("f3") << " = " << ss.vec3fConst("frac.g - frac.b") << ";";

            ss.newLine() << shaderDesc->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
            ss.dedent();
            ss.newLine() << "}";
            ss.newLine() << "else if (frac.r >= frac.b)";  // R > B > G
            ss.newLine() << "{";
            ss.indent();
            ss.newLine() << "nextInd = baseInd + " << ss.vec3fConst(0.0f, 0.0f, incr) << ";";
            ss.newLine() << ss.vec3fDecl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "nextInd = baseInd + " << ss.vec3fConst(incr, 0.0f, incr) << ";";
            ss.newLine() << ss.vec3fDecl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "f1 = " << ss.vec3fConst("1. - frac.r") << ";";
            ss.newLine() << "f4 = " << ss.vec3fConst("frac.g") << ";";
            ss.newLine() << ss.vec3fDecl("f2") << " = " << ss.vec3fConst("frac.r - frac.b") << ";";
            ss.newLine() << ss.vec3fDecl("f3") << " = " << ss.vec3fConst("frac.b - frac.g") << ";";

            ss.newLine() << shaderDesc->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
            ss.dedent();
            ss.newLine() << "}";
            ss.newLine() << "else";  // B > R > G
            ss.newLine() << "{";
            ss.indent();
            ss.newLine() << "nextInd = baseInd + " << ss.vec3fConst(incr, 0.0f, 0.0f) << ";";
            ss.newLine() << ss.vec3fDecl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "nextInd = baseInd + " << ss.vec3fConst(incr, 0.0f, incr) << ";";
            ss.newLine() << ss.vec3fDecl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "f1 = " << ss.vec3fConst("1. - frac.b") << ";";
            ss.newLine() << "f4 = " << ss.vec3fConst("frac.g") << ";";
            ss.newLine() << ss.vec3fDecl("f2") << " = " << ss.vec3fConst("frac.b - frac.r") << ";";
            ss.newLine() << ss.vec3fDecl("f3") << " = " << ss.vec3fConst("frac.r - frac.g") << ";";

            ss.newLine() << shaderDesc->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
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
            ss.newLine() << "nextInd = baseInd + " << ss.vec3fConst(incr, 0.0f, 0.0f) << ";";
            ss.newLine() << ss.vec3fDecl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "nextInd = baseInd + " << ss.vec3fConst(incr, incr, 0.0f) << ";";
            ss.newLine() << ss.vec3fDecl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "f1 = " << ss.vec3fConst("1. - frac.b") << ";";
            ss.newLine() << "f4 = " << ss.vec3fConst("frac.r") << ";";
            ss.newLine() << ss.vec3fDecl("f2") << " = " << ss.vec3fConst("frac.b - frac.g") << ";";
            ss.newLine() << ss.vec3fDecl("f3") << " = " << ss.vec3fConst("frac.g - frac.r") << ";";

            ss.newLine() << shaderDesc->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
            ss.dedent();
            ss.newLine() << "}";
            ss.newLine() << "else if (frac.r >= frac.b)";  // G > R > B
            ss.newLine() << "{";
            ss.indent();
            ss.newLine() << "nextInd = baseInd + " << ss.vec3fConst(0.0f, incr, 0.0f) << ";";
            ss.newLine() << ss.vec3fDecl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "nextInd = baseInd + " << ss.vec3fConst(0.0f, incr, incr) << ";";
            ss.newLine() << ss.vec3fDecl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "f1 = " << ss.vec3fConst("1. - frac.g") << ";";
            ss.newLine() << "f4 = " << ss.vec3fConst("frac.b") << ";";
            ss.newLine() << ss.vec3fDecl("f2") << " = " << ss.vec3fConst("frac.g - frac.r") << ";";
            ss.newLine() << ss.vec3fDecl("f3") << " = " << ss.vec3fConst("frac.r - frac.b") << ";";

            ss.newLine() << shaderDesc->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
            ss.dedent();
            ss.newLine() << "}";
            ss.newLine() << "else";  // G > B > R
            ss.newLine() << "{";
            ss.indent();
            ss.newLine() << "nextInd = baseInd + " << ss.vec3fConst(0.0f, incr, 0.0f) << ";";
            ss.newLine() << ss.vec3fDecl("v2") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "nextInd = baseInd + " << ss.vec3fConst(incr, incr, 0.0f) << ";";
            ss.newLine() << ss.vec3fDecl("v3") << " = " << ss.sampleTex3D(name, "nextInd") << ".rgb;";

            ss.newLine() << "f1 = " << ss.vec3fConst("1. - frac.g") << ";";
            ss.newLine() << "f4 = " << ss.vec3fConst("frac.r") << ";";
            ss.newLine() << ss.vec3fDecl("f2") << " = " << ss.vec3fConst("frac.g - frac.b") << ";";
            ss.newLine() << ss.vec3fDecl("f3") << " = " << ss.vec3fConst("frac.b - frac.r") << ";";

            ss.newLine() << shaderDesc->getPixelName() << ".rgb = (f2 * v2) + (f3 * v3);";
            ss.dedent();
            ss.newLine() << "}";
            ss.dedent();
            ss.newLine() << "}";

            ss.newLine() << shaderDesc->getPixelName()
                         << ".rgb = "
                         << shaderDesc->getPixelName()
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

            ss.newLine() << ss.vec3fDecl(name + "_coords")
                         << " = (" << shaderDesc->getPixelName() << ".zyx * "
                         << ss.vec3fConst(dim - 1) << " + "
                         << ss.vec3fConst(0.5f) + ") / "
                         << ss.vec3fConst(dim) << ";";

            ss.newLine() << shaderDesc->getPixelName() << ".rgb = "
                         << ss.sampleTex3D(name, name + "_coords") << ".rgb;";
        }

        shaderDesc->addToFunctionShaderCode(ss.string().c_str());
    }
}


}
OCIO_NAMESPACE_EXIT

