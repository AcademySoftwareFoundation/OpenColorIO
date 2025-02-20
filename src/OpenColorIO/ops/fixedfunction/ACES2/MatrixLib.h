// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_ACES2_MATRIXLIB_H
#define INCLUDED_OCIO_ACES2_MATRIXLIB_H

#include "ops/matrix/MatrixOpData.h"

#include <array>

namespace OCIO_NAMESPACE
{

namespace ACES2
{

using f2 = std::array<float, 2>;
using f3 = std::array<float, 3>;
using f4 = std::array<float, 4>;
using m33f = std::array<float, 9>;


inline f3 f3_from_f(float v)
{
    return f3 {v, v, v};
}

inline f3 add_f_f3(float v, const f3 &f3)
{
    return {
        v + f3[0],
        v + f3[1],
        v + f3[2]
    };
}

inline f3 mult_f_f3(float v, const f3 &f3)
{
    return {
        v * f3[0],
        v * f3[1],
        v * f3[2]
    };
}

inline f3 mult_f3_f33(const f3 &f3, const m33f &mat33)
{
    return {
        f3[0] * mat33[0] + f3[1] * mat33[1] + f3[2] * mat33[2],
        f3[0] * mat33[3] + f3[1] * mat33[4] + f3[2] * mat33[5],
        f3[0] * mat33[6] + f3[1] * mat33[7] + f3[2] * mat33[8]
    };
}

inline m33f mult_f33_f33(const m33f &a, const m33f &b)
{
    return {
        a[0] * b[0] + a[1] * b[3] + a[2] * b[6],
        a[0] * b[1] + a[1] * b[4] + a[2] * b[7],
        a[0] * b[2] + a[1] * b[5] + a[2] * b[8],

        a[3] * b[0] + a[4] * b[3] + a[5] * b[6],
        a[3] * b[1] + a[4] * b[4] + a[5] * b[7],
        a[3] * b[2] + a[4] * b[5] + a[5] * b[8],

        a[6] * b[0] + a[7] * b[3] + a[8] * b[6],
        a[6] * b[1] + a[7] * b[4] + a[8] * b[7],
        a[6] * b[2] + a[7] * b[5] + a[8] * b[8]
    };
}

inline m33f scale_f33(const m33f &mat33, const f3 &scale)
{
    return {
        mat33[0] * scale[0], mat33[3], mat33[6],
        mat33[1], mat33[4] * scale[1], mat33[7],
        mat33[2], mat33[5], mat33[8] * scale[2]
    };
}

inline m33f m33_from_ocio_matrix_array(const MatrixOpData::MatrixArray &array)
{
    const auto& v = array.getValues();
    return {
        (float) v[0], (float) v[1], (float) v[2],
        (float) v[4], (float) v[5], (float) v[6],
        (float) v[8], (float) v[9], (float) v[10],
    };
}

inline m33f invert_f33(const m33f &mat33)
{
    MatrixOpData::MatrixArray array;
    MatrixOpData::MatrixArray::Values & v = array.getValues();

    v[0] = mat33[0];
    v[1] = mat33[1];
    v[2] = mat33[2];

    v[4] = mat33[3];
    v[5] = mat33[4];
    v[6] = mat33[5];

    v[8] = mat33[6];
    v[9] = mat33[7];
    v[10] = mat33[8];

    MatrixOpData::MatrixArray inverse = *(array.inverse());
    return m33_from_ocio_matrix_array(inverse);
}

} // ACES2 namespace

} // OCIO namespace

#endif
