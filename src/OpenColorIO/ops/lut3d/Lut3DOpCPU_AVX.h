// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_LUT3DOP_CPU_AVX_H
#define INCLUDED_OCIO_LUT3DOP_CPU_AVX_H

#include <OpenColorIO/OpenColorIO.h>

#include "CPUInfo.h"

#if OCIO_USE_AVX
namespace OCIO_NAMESPACE
{

void applyTetrahedralAVX(const float *lut3d, int dim, const float *src, float *dst, int total_pixel_count);

} // namespace OCIO_NAMESPACE

#endif // OCIO_USE_AVX

#endif /* INCLUDED_OCIO_LUT3DOP_CPU_AVX_H */