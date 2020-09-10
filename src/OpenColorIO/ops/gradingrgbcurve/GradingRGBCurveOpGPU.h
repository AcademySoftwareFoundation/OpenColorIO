// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_GRADINGRGBCURVE_GPU_H
#define INCLUDED_OCIO_GRADINGRGBCURVE_GPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/gradingrgbcurve/GradingRGBCurveOpData.h"

namespace OCIO_NAMESPACE
{

void GetGradingRGBCurveGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                                        ConstGradingRGBCurveOpDataRcPtr & gpData);

} // namespace OCIO_NAMESPACE

#endif


