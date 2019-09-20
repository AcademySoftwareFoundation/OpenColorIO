// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_LUT1DOPCPU
#define INCLUDED_OCIO_LUT1DOPCPU

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Lut1D/Lut1DOpData.h"

OCIO_NAMESPACE_ENTER
{

ConstOpCPURcPtr GetLut1DRenderer(ConstLut1DOpDataRcPtr & lut, BitDepth in, BitDepth out);

}
OCIO_NAMESPACE_EXIT


#endif