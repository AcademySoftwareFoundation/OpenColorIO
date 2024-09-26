// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_ACES2COLORLIB_H
#define INCLUDED_OCIO_ACES2COLORLIB_H

#include "transforms/builtins/ColorMatrixHelpers.h"
#include "MatrixLib.h"


namespace OCIO_NAMESPACE
{

namespace ACES2
{

inline f3 HSV_to_RGB(const f3 &HSV)
{
    const float C = HSV[2] * HSV[1];
    const float X = C * (1.f - std::abs(std::fmod(HSV[0] * 6.f, 2.f) - 1.f));
    const float m = HSV[2] - C;

    f3 RGB{};
    if (HSV[0] < 1.f/6.f) {
        RGB = {C, X, 0.f};
    } else if (HSV[0] < 2./6.) {
        RGB = {X, C, 0.f};
    } else if (HSV[0] < 3./6.) {
        RGB = {0.f, C, X};
    } else if (HSV[0] < 4./6.) {
        RGB = {0.f, X, C};
    } else if (HSV[0] < 5./6.) {
        RGB = {X, 0.f, C};
    } else {
        RGB = {C, 0.f, X};
    }
    RGB = add_f_f3(m, RGB);
    return RGB;
}

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