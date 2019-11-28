// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_LOG_GPU_H
#define INCLUDED_OCIO_LOG_GPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/log/LogOpData.h"

namespace OCIO_NAMESPACE
{

void GetLogGPUShaderProgram(GpuShaderDescRcPtr & shaderDesc,
                            ConstLogOpDataRcPtr & logData);

} // namespace OCIO_NAMESPACE

#endif


