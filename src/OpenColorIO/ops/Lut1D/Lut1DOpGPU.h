// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LUT1D_GPU
#define INCLUDED_OCIO_LUT1D_GPU


#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/Lut1D/Lut1DOpData.h"


OCIO_NAMESPACE_ENTER
{

void GetLut1DGPUShaderProgram(GpuShaderDescRcPtr & shaderDesc,
                              ConstLut1DOpDataRcPtr & lutData);

}
OCIO_NAMESPACE_EXIT


#endif


