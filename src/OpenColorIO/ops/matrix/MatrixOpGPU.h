// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_GPU_MATRIXOP_H
#define INCLUDED_OCIO_GPU_MATRIXOP_H

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/matrix/MatrixOpData.h"

namespace OCIO_NAMESPACE
{

void GetMatrixGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                               ConstMatrixOpDataRcPtr & matrix);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_GPU_MATRIXOP_H


