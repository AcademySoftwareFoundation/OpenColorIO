// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_EXPOSURECONTRAST_GPU
#define INCLUDED_OCIO_EXPOSURECONTRAST_GPU


#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/exposurecontrast/ExposureContrastOpData.h"


OCIO_NAMESPACE_ENTER
{

void GetExposureContrastGPUShaderProgram(GpuShaderDescRcPtr & shaderDesc,
                                         ConstExposureContrastOpDataRcPtr & ec);

}
OCIO_NAMESPACE_EXIT

#endif
