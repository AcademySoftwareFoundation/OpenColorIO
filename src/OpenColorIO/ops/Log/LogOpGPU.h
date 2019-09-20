// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LOG_GPU
#define INCLUDED_OCIO_LOG_GPU


#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/Log/LogOpData.h"


OCIO_NAMESPACE_ENTER
{

void GetLogGPUShaderProgram(GpuShaderDescRcPtr & shaderDesc,
                            ConstLogOpDataRcPtr & logData);

}
OCIO_NAMESPACE_EXIT


#endif


