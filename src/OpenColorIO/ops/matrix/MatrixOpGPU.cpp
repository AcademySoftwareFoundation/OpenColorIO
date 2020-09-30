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
    ss.newLine() << "// Add a Matrix processing";
    ss.newLine() << "";

    ArrayDouble::Values values = matrix->getArray().getValues();
    MatrixOpData::Offsets offs(matrix->getOffsets());

    if (!matrix->isUnityDiagonal())
    {
        if (matrix->isDiagonal())
        {
            ss.newLine() << shaderCreator->getPixelName() << " = "
                            << ss.float4Const((float)values[0],
                                              (float)values[5],
                                              (float)values[10],
                                              (float)values[15])
                            << " * " << shaderCreator->getPixelName() << ";";
        }
        else
        {
            ss.newLine() << shaderCreator->getPixelName() << " = "
                            << ss.mat4fMul(&values[0], shaderCreator->getPixelName())
                            << ";";
        }
    }

    if (matrix->hasOffsets())
    {
        ss.newLine() << shaderCreator->getPixelName() << " = "
                        << ss.float4Const((float)offs[0], (float)offs[1], (float)offs[2], (float)offs[3])
                        << " + " << shaderCreator->getPixelName() << ";";
    }

    shaderCreator->addToFunctionShaderCode(ss.string().c_str());
}

} // namespace OCIO_NAMESPACE
