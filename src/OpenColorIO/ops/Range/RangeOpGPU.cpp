// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <limits>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "ops/Range/RangeOpGPU.h"


OCIO_NAMESPACE_ENTER
{

void GetRangeGPUShaderProgram(GpuShaderDescRcPtr & shaderDesc,
                              ConstRangeOpDataRcPtr & range)
{
    GpuShaderText ss(shaderDesc->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add a Range processing";
    ss.newLine() << "";

    if(range->scales(true))
    {
        const double scale[3]
            = { range->getScale(),
                range->getScale(),
                range->getScale() };

        const double offset[3] 
            = { range->getOffset(), 
                range->getOffset(), 
                range->getOffset() };

        ss.newLine() << shaderDesc->getPixelName() << ".rgb = "
                     << shaderDesc->getPixelName() << ".rgb * "
                     << ss.vec3fConst(scale[0], scale[1], scale[2])
                     << " + "
                     << ss.vec3fConst(offset[0], offset[1], offset[2])
                     << ";";
    
        const double alphaScale = range->getAlphaScale();
        if (alphaScale != 1.0)
        {
            ss.newLine() << shaderDesc->getPixelName() << ".w = "
                         << shaderDesc->getPixelName() << ".w * " << (float)alphaScale
                         << ";";
        }
    }

    if(range->minClips())
    {
        const double lowerBound[3] 
            = { range->getLowBound(), 
                range->getLowBound(), 
                range->getLowBound() };

        ss.newLine() << shaderDesc->getPixelName() << ".rgb = "
                     << "max(" << ss.vec3fConst(lowerBound[0],
                                                lowerBound[1],
                                                lowerBound[2]) << ", "
                     << shaderDesc->getPixelName()
                     << ".rgb);";
    }

    if (range->maxClips())
    {
        const double upperBound[3]
            = { range->getHighBound(),
                range->getHighBound(),
                range->getHighBound() };

        ss.newLine() << shaderDesc->getPixelName() << ".rgb = "
            << "min(" << ss.vec3fConst(upperBound[0],
                                       upperBound[1],
                                       upperBound[2]) << ", "
            << shaderDesc->getPixelName()
            << ".rgb);";
    }

    shaderDesc->addToFunctionShaderCode(ss.string().c_str());
}


}
OCIO_NAMESPACE_EXIT
