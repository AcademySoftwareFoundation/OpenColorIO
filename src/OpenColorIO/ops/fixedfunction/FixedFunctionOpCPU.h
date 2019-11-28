// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_FIXEDFUNCTION_CPU_H
#define INCLUDED_OCIO_FIXEDFUNCTION_CPU_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/fixedfunction/FixedFunctionOpData.h"

namespace OCIO_NAMESPACE
{

ConstOpCPURcPtr GetFixedFunctionCPURenderer(ConstFixedFunctionOpDataRcPtr & func);

} // namespace OCIO_NAMESPACE

#endif
