// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_HUECURVE_GPU_H
#define INCLUDED_OCIO_HUECURVE_GPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/gradinghuecurve/GradingHueCurveOpData.h"

namespace OCIO_NAMESPACE
{

void GetGradingHueCurveGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                                        ConstGradingHueCurveOpDataRcPtr & gpData);

} // namespace OCIO_NAMESPACE

#endif
