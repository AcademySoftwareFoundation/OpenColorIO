// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_RANGE_GPU
#define INCLUDED_OCIO_RANGE_GPU


#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/Range/RangeOpData.h"


OCIO_NAMESPACE_ENTER
{

void GetRangeGPUShaderProgram(GpuShaderDescRcPtr & shaderDesc,
                              ConstRangeOpDataRcPtr & range);

}
OCIO_NAMESPACE_EXIT


#endif


