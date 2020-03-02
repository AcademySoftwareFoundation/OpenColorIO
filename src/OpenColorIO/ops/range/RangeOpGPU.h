// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_RANGEOP_GPU_H
#define INCLUDED_OCIO_RANGEOP_GPU_H


#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/range/RangeOpData.h"


namespace OCIO_NAMESPACE
{

void GetRangeGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator, ConstRangeOpDataRcPtr & range);

} // namespace OCIO_NAMESPACE


#endif


