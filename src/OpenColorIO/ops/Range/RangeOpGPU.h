// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_RANGEOP_GPU_H
#define INCLUDED_OCIO_RANGEOP_GPU_H


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


