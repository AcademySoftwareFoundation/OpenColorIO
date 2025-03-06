// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_ACES2COLORLIB_H
#define INCLUDED_OCIO_ACES2COLORLIB_H

#include "transforms/builtins/ColorMatrixHelpers.h"
#include "MatrixLib.h"

#include <cmath>

namespace OCIO_NAMESPACE
{

namespace ACES2
{

inline m33f RGBtoXYZ_f33(const Primaries &C)
{
    return m33_from_ocio_matrix_array(
        *build_conversion_matrix(C, CIE_XYZ_ILLUM_E::primaries, ADAPTATION_NONE)
    );
}

inline m33f XYZtoRGB_f33(const Primaries &C)
{
    return m33_from_ocio_matrix_array(
        *build_conversion_matrix(C, CIE_XYZ_ILLUM_E::primaries, ADAPTATION_NONE)->inverse()
    );
}

inline m33f RGBtoRGB_f33(const Primaries &Csrc, const Primaries &Cdst)
{
    return mult_f33_f33(XYZtoRGB_f33(Cdst), RGBtoXYZ_f33(Csrc));
}

constexpr m33f Identity_M33 = {
    1.f, 0.f, 0.f,
    0.f, 1.f, 0.f,
    0.f, 0.f, 1.f
};

} // ACES2 namespace


} // OCIO namespace

#endif