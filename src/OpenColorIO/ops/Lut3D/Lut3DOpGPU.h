// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LUT3D_GPU
#define INCLUDED_OCIO_LUT3D_GPU


#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/Lut3D/Lut3DOpData.h"


OCIO_NAMESPACE_ENTER
{

void GetLut3DGPUShaderProgram(GpuShaderDescRcPtr & shaderDesc,
                              ConstLut3DOpDataRcPtr & lutData);

}
OCIO_NAMESPACE_EXIT


#endif