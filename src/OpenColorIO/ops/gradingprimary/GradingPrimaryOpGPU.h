// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_GRADINGPRIMARY_GPU_H
#define INCLUDED_OCIO_GRADINGPRIMARY_GPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/gradingprimary/GradingPrimaryOpData.h"

namespace OCIO_NAMESPACE
{

void GetGradingPrimaryGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                                       ConstGradingPrimaryOpDataRcPtr & gpData);

} // namespace OCIO_NAMESPACE

#endif


