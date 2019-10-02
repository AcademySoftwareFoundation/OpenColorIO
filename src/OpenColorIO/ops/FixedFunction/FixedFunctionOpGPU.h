// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_FIXEDFUNCTION_GPU
#define INCLUDED_OCIO_FIXEDFUNCTION_GPU


#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/FixedFunction/FixedFunctionOpData.h"


OCIO_NAMESPACE_ENTER
{

void GetFixedFunctionGPUShaderProgram(GpuShaderText & ss, 
                                      ConstFixedFunctionOpDataRcPtr & func);

}
OCIO_NAMESPACE_EXIT


#endif


