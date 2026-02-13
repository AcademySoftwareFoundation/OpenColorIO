// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_LUT1DOP_CPU_SSE2_H
#define INCLUDED_OCIO_LUT1DOP_CPU_SSE2_H


#include "CPUInfoConfig.h"

#if OCIO_USE_SSE2

#include <OpenColorIO/OpenColorIO.h>

typedef void (Lut1DOpCPUApplyFunc)(const float *, const float *, const float *, int, const void *, void *, long);

namespace OCIO_NAMESPACE
{

Lut1DOpCPUApplyFunc * SSE2GetLut1DApplyFunc(BitDepth inBD, BitDepth outBD);

} // namespace OCIO_NAMESPACE

#endif // OCIO_USE_SSE2

#endif /* INCLUDED_OCIO_LUT1DOP_CPU_SSE2_H */