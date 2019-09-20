// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_RANGEOP_CPU
#define INCLUDED_OCIO_RANGEOP_CPU


#include <OpenColorIO/OpenColorIO.h>

#include "ops/Range/RangeOpData.h"


OCIO_NAMESPACE_ENTER
{

ConstOpCPURcPtr GetRangeRenderer(ConstRangeOpDataRcPtr & range);

}
OCIO_NAMESPACE_EXIT


#endif