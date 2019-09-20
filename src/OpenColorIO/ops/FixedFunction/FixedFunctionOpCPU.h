// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_FIXEDFUNCTION_CPU
#define INCLUDED_OCIO_FIXEDFUNCTION_CPU


#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/FixedFunction/FixedFunctionOpData.h"


OCIO_NAMESPACE_ENTER
{

ConstOpCPURcPtr GetFixedFunctionCPURenderer(ConstFixedFunctionOpDataRcPtr & func);

}
OCIO_NAMESPACE_EXIT

#endif
