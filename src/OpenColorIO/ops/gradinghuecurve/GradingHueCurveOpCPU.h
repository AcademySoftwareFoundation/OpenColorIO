// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_GRADINGHUECURVE_CPU_H
#define INCLUDED_OCIO_GRADINGHUECURVE_CPU_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/gradinghuecurve/GradingHueCurveOpData.h"

namespace OCIO_NAMESPACE
{

ConstOpCPURcPtr GetGradingHueCurveCPURenderer(ConstGradingHueCurveOpDataRcPtr & hueCurve);

} // namespace OCIO_NAMESPACE

#endif
