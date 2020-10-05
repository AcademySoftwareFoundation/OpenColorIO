// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <limits>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "ops/range/RangeOpGPU.h"


namespace OCIO_NAMESPACE
{

void GetRangeGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator, ConstRangeOpDataRcPtr & range)
{
    GpuShaderText ss(shaderCreator->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add a Range processing";
    ss.newLine() << "";

    if(range->scales())
    {
        const double scale[3]
            = { range->getScale(),
                range->getScale(),
                range->getScale() };

        const double offset[3]
            = { range->getOffset(),
                range->getOffset(),
                range->getOffset() };

        ss.newLine() << shaderCreator->getPixelName() << ".rgb = "
                     << shaderCreator->getPixelName() << ".rgb * "
                     << ss.float3Const(scale[0], scale[1], scale[2])
                     << " + "
                     << ss.float3Const(offset[0], offset[1], offset[2])
                     << ";";
    }

    if(!range->minIsEmpty())
    {
        const double lowerBound[3]
            = { range->getMinOutValue(),
                range->getMinOutValue(),
                range->getMinOutValue() };

        ss.newLine() << shaderCreator->getPixelName() << ".rgb = "
                     << "max(" << ss.float3Const(lowerBound[0],
                                                 lowerBound[1],
                                                 lowerBound[2]) << ", "
                     << shaderCreator->getPixelName()
                     << ".rgb);";
    }

    if (!range->maxIsEmpty())
    {
        const double upperBound[3]
            = { range->getMaxOutValue(),
                range->getMaxOutValue(),
                range->getMaxOutValue() };

        ss.newLine() << shaderCreator->getPixelName() << ".rgb = "
            << "min(" << ss.float3Const(upperBound[0],
                                        upperBound[1],
                                        upperBound[2]) << ", "
            << shaderCreator->getPixelName()
            << ".rgb);";
    }

    shaderCreator->addToFunctionShaderCode(ss.string().c_str());
}


} // namespace OCIO_NAMESPACE
