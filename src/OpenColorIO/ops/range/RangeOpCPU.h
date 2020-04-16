// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_RANGEOP_CPU_H
#define INCLUDED_OCIO_RANGEOP_CPU_H


#include <OpenColorIO/OpenColorIO.h>

#include "ops/range/RangeOpData.h"


namespace OCIO_NAMESPACE
{

ConstOpCPURcPtr GetRangeRenderer(ConstRangeOpDataRcPtr & range);

} // namespace OCIO_NAMESPACE


#endif