// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_COLOR_MATRIX_HELPERS_H
#define INCLUDED_OCIO_COLOR_MATRIX_HELPERS_H


#include <OpenColorIO/OpenColorIO.h>

#include "ops/matrix/MatrixOpData.h"


namespace OCIO_NAMESPACE
{

struct Chromaticities
{
    Chromaticities() = delete;
    ~Chromaticities() = default;

    Chromaticities(double x, double y)
    {
        m_xy[0] = x;
        m_xy[1] = y;
    }

    Chromaticities(const Chromaticities & xy)
    {
        m_xy[0] = xy.m_xy[0];
        m_xy[1] = xy.m_xy[1];
    }

    Chromaticities & operator=(const Chromaticities & xy)
    {
        if (this == &xy) return *this;

        m_xy[0] = xy.m_xy[0];
        m_xy[1] = xy.m_xy[1];

        return *this;
    }

    double m_xy[2] { 0., 0. };
};

struct Primaries
{
    Primaries() = delete;
    ~Primaries() = default;

    Primaries(const Chromaticities & red, const Chromaticities & grn, const Chromaticities & blu,
              const Chromaticities & wht)
        :   m_red(red)
        ,   m_grn(grn)
        ,   m_blu(blu)
        ,   m_wht(wht)
    {}

    Primaries(const Primaries & primaries)
        :   m_red(primaries.m_red)
        ,   m_grn(primaries.m_grn)
        ,   m_blu(primaries.m_blu)
        ,   m_wht(primaries.m_wht)
    {}

    Primaries & operator=(const Primaries & primaries)
    {
        if (this == &primaries) return *this;

        m_red = primaries.m_red;
        m_grn = primaries.m_grn;
        m_blu = primaries.m_blu;
        m_wht = primaries.m_wht;

        return *this;
    }

    Chromaticities m_red; // CIE xy chromaticity coordinates for red primary.
    Chromaticities m_grn; // CIE xy chromaticity coordinates for green primary.
    Chromaticities m_blu; // CIE xy chromaticity coordinates for blue primary.
    Chromaticities m_wht; // CIE xy chromaticities for white (or gray).
};

namespace CIE_XYZ_ILLUM_E
{
extern const Primaries primaries;
}

namespace ACES_AP0
{
extern const Primaries primaries;
}

namespace ACES_AP1
{
extern const Primaries primaries;
}

namespace REC709
{
extern const Primaries primaries;
}

namespace REC2020
{
extern const Primaries primaries;
}

namespace P3_DCI
{
extern const Primaries primaries;
}

namespace P3_D65
{
extern const Primaries primaries;
}

namespace P3_D60
{
extern const Primaries primaries;
}

namespace WHITEPOINT
{
extern const MatrixOpData::Offsets D60_XYZ;
extern const MatrixOpData::Offsets D65_XYZ;
extern const MatrixOpData::Offsets DCI_XYZ;
}

// Calculate a matrix to convert arbitrary RGB primary tristimulus values
// to CIE XYZ tristimulus values using the CIE xy chromaticity coordinates
// of the RGB primaries and white.  The matrix is scaled to take RGB values
// on [0,1] and produce XYZ values on [0,1].  Apply the matrix as follows:
//        X = rgb2xyz[0][0] * R + rgb2xyz[0][1] * G + rgb2xyz[0][2] * B;
//        Y = rgb2xyz[1][0] * R + rgb2xyz[1][1] * G + rgb2xyz[1][2] * B;
//        Z = rgb2xyz[2][0] * R + rgb2xyz[2][1] * G + rgb2xyz[2][2] * B;

MatrixOpData::MatrixArrayPtr rgb2xyz_from_xy(const Primaries & chromaticities);

enum AdaptationMethod
{
    ADAPTATION_NONE = 0,
    ADAPTATION_BRADFORD,
    ADAPTATION_CAT02
};

// Build a von Kries type chromatic adaptation matrix from source white point src_XYZ to 
// destination white point dst_XYZ, using the chosen cone primary matrix.

MatrixOpData::MatrixArrayPtr build_vonkries_adapt(const MatrixOpData::Offsets & src_XYZ,
                                                  const MatrixOpData::Offsets & dst_XYZ,
                                                  AdaptationMethod method);

// Build a conversion matrix from source primaries to destination primaries.
// The resulting matrix will map [1,1,1] input RGB to [1,1,1] output RGB.

MatrixOpData::MatrixArrayPtr build_conversion_matrix(const Primaries & src_prims,
                                                     const Primaries & dst_prims,
                                                     AdaptationMethod method);

// Build a conversion matrix from source primaries to destination primaries with the option of
// setting the adaptation source and destination manually.  If you pass zeros for either of the
// white points, that corresponding white point will be taken from the primaries.

MatrixOpData::MatrixArrayPtr build_conversion_matrix(const Primaries & src_prims,
                                                     const Primaries & dst_prims,
                                                     const MatrixOpData::Offsets & src_wht_XYZ,
                                                     const MatrixOpData::Offsets & dst_wht_XYZ,
                                                     AdaptationMethod method);

// Build a conversion matrix to CIE XYZ D65 from the source primaries.

MatrixOpData::MatrixArrayPtr build_conversion_matrix_to_XYZ_D65(const Primaries & src_prims,
                                                                AdaptationMethod method);

// Build a conversion matrix from CIE XYZ D65 to the destination primaries.

MatrixOpData::MatrixArrayPtr build_conversion_matrix_from_XYZ_D65(const Primaries & dst_prims,
                                                                  AdaptationMethod method);
} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_COLOR_MATRIX_HELPERS_H
