// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_EXPOSURECONTRAST_GPU_H
#define INCLUDED_OCIO_EXPOSURECONTRAST_GPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/exposurecontrast/ExposureContrastOpData.h"


namespace OCIO_NAMESPACE
{

void GetExposureContrastGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                                         ConstExposureContrastOpDataRcPtr & ec);

} // namespace OCIO_NAMESPACE

#endif
