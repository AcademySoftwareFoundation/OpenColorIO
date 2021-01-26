// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_GRADINGTONE_GPU_H
#define INCLUDED_OCIO_GRADINGTONE_GPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/gradingtone/GradingToneOpData.h"

namespace OCIO_NAMESPACE
{

void GetGradingToneGPUShaderProgram(GpuShaderCreatorRcPtr & shaderCreator,
                                    ConstGradingToneOpDataRcPtr & gtData);

} // namespace OCIO_NAMESPACE

#endif


