// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_GRADINGRGBCURVE_CPU_H
#define INCLUDED_OCIO_GRADINGRGBCURVE_CPU_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/gradingrgbcurve/GradingRGBCurveOpData.h"

namespace OCIO_NAMESPACE
{

ConstOpCPURcPtr GetGradingRGBCurveCPURenderer(ConstGradingRGBCurveOpDataRcPtr & rgbCurve);

} // namespace OCIO_NAMESPACE

#endif
