// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "ops/matrix/MatrixOpGPU.h"


namespace OCIO_NAMESPACE
{

void GetMatrixGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator, ConstMatrixOpDataRcPtr & matrix)
{
    GpuShaderText ss(shaderCreator->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add Matrix processing";
    ss.newLine() << "";

    ss.newLine() << "{";
    ss.indent();

    ArrayDouble::Values values = matrix->getArray().getValues();
    MatrixOpData::Offsets offs(matrix->getOffsets());

    const std::string pxl(shaderCreator->getPixelName());

    ss.newLine() << ss.float4Decl("res") 
                 << " = " << ss.float4Const(pxl + ".rgb.r",
                                            pxl + ".rgb.g",
                                            pxl + ".rgb.b",
                                            pxl + ".a") << ";";

    if (!matrix->isUnityDiagonal())
    {
        if (matrix->isDiagonal())
        {
            ss.newLine() << "res = "
                         << ss.float4Const((float)values[0],
                                           (float)values[5],
                                           (float)values[10],
                                           (float)values[15])
                         << " * res;";
        }
        else
        {
            // NOTE: The in-place matrix computation is not supported by OSL so,
            // a temporary variable is needed.
            ss.newLine() << ss.float4Decl("tmp") << " = res;"; 
            ss.newLine() << "res = " << ss.mat4fMul(&values[0], "tmp") << ";";
        }
    }

    if (matrix->hasOffsets())
    {
        ss.newLine() << "res = "
                     << ss.float4Const((float)offs[0], (float)offs[1], (float)offs[2], (float)offs[3])
                     << " + res;";
    }

    ss.newLine() << pxl << ".rgb = " << ss.float3Const("res.x", "res.y", "res.z") << ";";
    ss.newLine() << pxl << ".a = res.w;";

    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToFunctionShaderCode(ss.string().c_str());
}

} // namespace OCIO_NAMESPACE
