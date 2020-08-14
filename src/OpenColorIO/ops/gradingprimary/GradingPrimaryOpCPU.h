// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_GRADINGPRIMARY_CPU_H
#define INCLUDED_OCIO_GRADINGPRIMARY_CPU_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/gradingprimary/GradingPrimaryOpData.h"

namespace OCIO_NAMESPACE
{

ConstOpCPURcPtr GetGradingPrimaryCPURenderer(ConstGradingPrimaryOpDataRcPtr & prim);

} // namespace OCIO_NAMESPACE

#endif
