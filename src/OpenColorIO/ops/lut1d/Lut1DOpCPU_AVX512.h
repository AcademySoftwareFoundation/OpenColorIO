// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_LUT1DOP_CPU_AVX512_H
#define INCLUDED_OCIO_LUT1DOP_CPU_AVX512_H

#include <OpenColorIO/OpenColorIO.h>

#include "CPUInfo.h"

typedef void (Lut1DOpCPUApplyFunc)(const float *, const float *, const float *, int, const void *, void *, long);

#if OCIO_USE_AVX512
namespace OCIO_NAMESPACE
{

Lut1DOpCPUApplyFunc * AVX512GetLut1DApplyFunc(BitDepth inBD, BitDepth outBD);

} // namespace OCIO_NAMESPACE

#endif // OCIO_USE_AVX512

#endif /* INCLUDED_OCIO_LUT1DOP_CPU_AVX512_H */