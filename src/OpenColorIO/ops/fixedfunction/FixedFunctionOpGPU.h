// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_FIXEDFUNCTION_GPU_H
#define INCLUDED_OCIO_FIXEDFUNCTION_GPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/fixedfunction/FixedFunctionOpData.h"

namespace OCIO_NAMESPACE
{

void GetFixedFunctionGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                                      ConstFixedFunctionOpDataRcPtr & func);

} // namespace OCIO_NAMESPACE

#endif


