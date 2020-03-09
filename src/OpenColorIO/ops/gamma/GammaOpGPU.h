// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GAMMAOP_GPU_H
#define INCLUDED_OCIO_GAMMAOP_GPU_H


#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/gamma/GammaOpData.h"

namespace OCIO_NAMESPACE
{

void GetGammaGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                              ConstGammaOpDataRcPtr & gammaData);


} // namespace OCIO_NAMESPACE


#endif // INCLUDED_OCIO_GAMMAOP_GPU_H
