// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_LUT3DOP_CPU_AVX2_H
#define INCLUDED_OCIO_LUT3DOP_CPU_AVX2_H

#include <OpenColorIO/OpenColorIO.h>

#include "CPUInfo.h"

#if OCIO_USE_AVX2
namespace OCIO_NAMESPACE
{

void applyTetrahedralAVX2(const float *lut3d, int dim, const float *src, float *dst, int total_pixel_count);

} // namespace OCIO_NAMESPACE

#endif // OCIO_USE_AVX2

#endif /* INCLUDED_OCIO_LUT3DOP_CPU_AVX2_H */