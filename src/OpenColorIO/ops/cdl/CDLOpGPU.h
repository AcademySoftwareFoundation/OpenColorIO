// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_CDL_GPU_H
#define INCLUDED_OCIO_CDL_GPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/cdl/CDLOpData.h"


namespace OCIO_NAMESPACE
{

void GetCDLGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator, ConstCDLOpDataRcPtr & cdl);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_CDL_GPU_H
