// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_LUT1D_GPU_H
#define INCLUDED_OCIO_LUT1D_GPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/lut1d/Lut1DOpData.h"

namespace OCIO_NAMESPACE
{

void GetLut1DGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator, ConstLut1DOpDataRcPtr & lutData);

} // namespace OCIO_NAMESPACE


#endif


