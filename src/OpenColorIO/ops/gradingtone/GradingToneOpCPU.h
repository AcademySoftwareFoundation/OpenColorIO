// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_GRADINGTONE_CPU_H
#define INCLUDED_OCIO_GRADINGTONE_CPU_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/gradingtone/GradingToneOpData.h"

namespace OCIO_NAMESPACE
{

ConstOpCPURcPtr GetGradingToneCPURenderer(ConstGradingToneOpDataRcPtr & prim);

} // namespace OCIO_NAMESPACE

#endif
