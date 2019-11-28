// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_LUT1DOP_CPU_H
#define INCLUDED_OCIO_LUT1DOP_CPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/lut1d/Lut1DOpData.h"

namespace OCIO_NAMESPACE
{

ConstOpCPURcPtr GetLut1DRenderer(ConstLut1DOpDataRcPtr & lut, BitDepth in, BitDepth out);

} // namespace OCIO_NAMESPACE

#endif