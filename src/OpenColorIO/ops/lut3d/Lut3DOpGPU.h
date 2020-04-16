// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_LUT3D_GPU_H
#define INCLUDED_OCIO_LUT3D_GPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/lut3d/Lut3DOpData.h"

namespace OCIO_NAMESPACE
{

void GetLut3DGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator, ConstLut3DOpDataRcPtr & lutData);

} // namespace OCIO_NAMESPACE


#endif
