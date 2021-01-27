// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "transforms/builtins/ColorMatrixHelpers.h"


namespace OCIO_NAMESPACE
{

namespace CIE_XYZ_ILLUM_E
{
static const Chromaticities red_xy(1.,     0.    );
static const Chromaticities grn_xy(0.,     1.    );
static const Chromaticities blu_xy(0.,     0.    );
static const Chromaticities wht_xy(1./3.,  1./3. );

const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);
}

// ACES Primaries from SMPTE ST2065-1.
namespace ACES_AP0
{
static const Chromaticities red_xy(0.7347,  0.2653 );
static const Chromaticities grn_xy(0.0000,  1.0000 );
static const Chromaticities blu_xy(0.0001, -0.0770 );
static const Chromaticities wht_xy(0.32168, 0.33767);

const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);
}

namespace ACES_AP1
{
static const Chromaticities red_xy(0.713,   0.293  );
static const Chromaticities grn_xy(0.165,   0.830  );
static const Chromaticities blu_xy(0.128,   0.044  );
static const Chromaticities wht_xy(0.32168, 0.33767);

const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);
}

namespace REC709
{
static const Chromaticities red_xy(0.64,   0.33  );
static const Chromaticities grn_xy(0.30,   0.60  );
static const Chromaticities blu_xy(0.15,   0.06  );
static const Chromaticities wht_xy(0.3127, 0.3290);

const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);
}

namespace REC2020
{
static const Chromaticities red_xy(0.708,  0.292 );
static const Chromaticities grn_xy(0.170,  0.797 );
static const Chromaticities blu_xy(0.131,  0.046 );
static const Chromaticities wht_xy(0.3127, 0.3290);

const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);
}

namespace P3_DCI
{
static const Chromaticities red_xy(0.680, 0.320);
static const Chromaticities grn_xy(0.265, 0.690);
static const Chromaticities blu_xy(0.150, 0.060);
static const Chromaticities wht_xy(0.314, 0.351);

const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);
}

namespace P3_D65
{
static const Chromaticities red_xy(0.680,  0.320 );
static const Chromaticities grn_xy(0.265,  0.690 );
static const Chromaticities blu_xy(0.150,  0.060 );
static const Chromaticities wht_xy(0.3127, 0.3290);

const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);
}

namespace P3_D60
{
static const Chromaticities red_xy(0.680,  0.320 );
static const Chromaticities grn_xy(0.265,  0.690 );
static const Chromaticities blu_xy(0.150,  0.060 );
static const Chromaticities wht_xy(0.32168, 0.33767);

const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);
}

namespace WHITEPOINT
{
const MatrixOpData::Offsets D60_XYZ(0.95264607456985, 1., 1.00882518435159, 0.);
const MatrixOpData::Offsets D65_XYZ(0.95045592705167, 1., 1.08905775075988, 0.);
const MatrixOpData::Offsets DCI_XYZ(0.89458689458689, 1., 0.95441595441595, 0.);
}

// Here are some notes on how one derives the color space conversion
// matrix starting with chromaticity coordinates.
// 
// We're looking for a 3x3 matrix M that converts tristimulus values to
// standard CIE XYZ primaries from some other set of RGB primaries.
// I.e.:
// 
//    [ X Y Z ] = [ R G B ] * M
//
// (This note uses row * matrix rather than matrix * column notation, since
// it's easier to type, but keep in mind that OCIO uses RGB as columns.)
// 
// If the red primary occurs at [ R G B] = [ 1 0 0] and similarly for
// green and blue, then we have:
// 
//  [ rX rY rZ ]   [  1  0  0 ]   
//  | gX gY gZ | = |  0  1  0 | *  M  =  I * M
//  [ bX bY bZ ]   [  0  0  1 ]   
//  
// where [ rX rY rZ ] are the tristimulus values of the red primary
// (likewise for green and blue) and the rows of M are simply the
// measurements of the RGB primaries.  The problem then is how to
// find the matrix M.
// 
// We may proceed as follows:
// 
// Recall the definition of chromaticity coordinates:
// 
//  x = X / ( X + Y + Z),  y = Y / ( X + Y + Z),  z = Z / ( X + Y + Z),
// 
// implying z = 1 - x - y, and also X = Y * x / y, and Z = Y * z / y.
// 
// For the red primary, call the chromaticity coordinates [ rx ry rz ]
// and let rS = ( rX + rY + rZ) be the sum of its tristimulus values
// (and likewise for green and blue).  Then, by definition:
// 
//  [ rX rY rZ ]   [ rS  0  0 ]   [ rx ry rz ]
//  | gX gY gZ | = |  0 gS  0 | * | gx gy gz | =  I * M         ( eq 1)
//  [ bX bY bZ ]   [  0  0 bS ]   [ bx by bz ]
// 
// The left-most two matrices are unknowns.  However, we can add the
// constraint that the sum of the tristimulus values of the primaries
// should give the tristimulus values of white, i.e.:
// 
//  [ wX wY wZ ] = [ 1 1 1 ] * M                                ( eq 2)
// 
// We then arbitrarily choose the luminance of white based on how we
// want to scale the CIE tristimulus values.  E.g., to scale Y from
// [0,1], we would set the luminance of white wY = 1.  Then calculate
// wX and wZ as:
// 
//    wX = wY * wx / wy,  wZ = wY * ( 1 - wx - wy) / wy.
// 
// Then, combining equations 1 and 2, we have:
// 
//                               [ rx ry rz ]
// [ wX wY wZ ] = [ rS gS bS ] * | gx gy gz |                   ( eq 3)
//                               [ bx by bz ]
// 
// where only rS, gS, and bS are unknown and may be solved for by
// factoring (or inverting) the matrix on the right.  Plugging these
// into equation 1 gives the desired matrix M.

MatrixOpData::MatrixArrayPtr rgb2xyz_from_xy(const Primaries & primaries)
{
    double wht_XYZ[3] { 0., 0., 0. };
    double gains[3]   { 0., 0., 0. };

    // Create a 4x4 identity matrix.
    MatrixOpData::MatrixArrayPtr matrix = std::make_shared<MatrixOpData::MatrixArray>();

    matrix->setDoubleValue( 0, primaries.m_red.m_xy[0]);
    matrix->setDoubleValue( 4, primaries.m_red.m_xy[1]);
    matrix->setDoubleValue( 8, 1. - primaries.m_red.m_xy[0] - primaries.m_red.m_xy[1]);

    matrix->setDoubleValue( 1, primaries.m_grn.m_xy[0]);
    matrix->setDoubleValue( 5, primaries.m_grn.m_xy[1]);
    matrix->setDoubleValue( 9, 1. - primaries.m_grn.m_xy[0] - primaries.m_grn.m_xy[1]);

    matrix->setDoubleValue( 2, primaries.m_blu.m_xy[0]);
    matrix->setDoubleValue( 6, primaries.m_blu.m_xy[1]);
    matrix->setDoubleValue(10, 1. - primaries.m_blu.m_xy[0] - primaries.m_blu.m_xy[1]);

    // 'matrix' is always well-conditioned, forming inverse is okay.
    MatrixOpData::MatrixArrayPtr invMatrix = matrix->inverse();

    wht_XYZ[0] = primaries.m_wht.m_xy[0] / primaries.m_wht.m_xy[1];
    wht_XYZ[1] = 1.0;  // Set scaling of XYZ values to [0, 1].
    wht_XYZ[2] = (1.0 - primaries.m_wht.m_xy[0] - primaries.m_wht.m_xy[1]) / primaries.m_wht.m_xy[1];

    // Tristimulus value conversion matrix, initialized to a 4x4 identity matrix for now.
    MatrixOpData::MatrixArrayPtr rgb2xyz = std::make_shared<MatrixOpData::MatrixArray>();

    for (unsigned i = 0; i < 3; i++)
    {
        gains[i] =    wht_XYZ[0] * invMatrix->getDoubleValue(i * 4 + 0)
                    + wht_XYZ[1] * invMatrix->getDoubleValue(i * 4 + 1)
                    + wht_XYZ[2] * invMatrix->getDoubleValue(i * 4 + 2);

        for (unsigned j = 0; j < 3; j++)
        {
            rgb2xyz->setDoubleValue(j * 4 + i, gains[i] * matrix->getDoubleValue(j * 4 + i));
        }
    }

    return rgb2xyz;
}

MatrixOpData::MatrixArrayPtr build_vonkries_adapt(const MatrixOpData::Offsets & src_XYZ, 
                                                  const MatrixOpData::Offsets & dst_XYZ,
                                                  AdaptationMethod method)
{
    static constexpr double CONE_RESP_MAT_BRADFORD[16]{
             0.8951,  0.2664, -0.1614,  0.,
            -0.7502,  1.7135,  0.0367,  0.,
             0.0389, -0.0685,  1.0296,  0.,
             0.,      0.,      0.,      1. };

    static constexpr double CONE_RESP_MAT_CAT02[16]{
             0.7328,  0.4296, -0.1624,  0.,
            -0.7036,  1.6975,  0.0061,  0.,
             0.0030,  0.0136,  0.9834,  0.,
             0.,      0.,      0.,      1. };

    MatrixOpData::MatrixArrayPtr xyz2rgb = std::make_shared<MatrixOpData::MatrixArray>();
    if (method == ADAPTATION_CAT02)
    {
        xyz2rgb->setRGBA(&CONE_RESP_MAT_CAT02[0]);
    }
    else
    {
        xyz2rgb->setRGBA(&CONE_RESP_MAT_BRADFORD[0]);
    }

    MatrixOpData::MatrixArrayPtr rgb2xyz = xyz2rgb->inverse();

    // Convert white point XYZ values to cone primary RGBs.
    const MatrixOpData::Offsets src_RGB(xyz2rgb->inner(src_XYZ));
    const MatrixOpData::Offsets dst_RGB(xyz2rgb->inner(dst_XYZ));
    const double scaleFactor[4] { dst_RGB[0] / src_RGB[0], 
                                  dst_RGB[1] / src_RGB[1],
                                  dst_RGB[2] / src_RGB[2],
                                  1. };

    // Make a diagonal matrix with the scale factors.
    MatrixOpData::MatrixArrayPtr scaleMat = std::make_shared<MatrixOpData::MatrixArray>();
    scaleMat->setDoubleValue( 0, scaleFactor[0]);
    scaleMat->setDoubleValue( 5, scaleFactor[1]);
    scaleMat->setDoubleValue(10, scaleFactor[2]);
    scaleMat->setDoubleValue(15, scaleFactor[3]);

    // Compose into the adaptation matrix.
    return rgb2xyz->inner(scaleMat->inner(xyz2rgb));
}

MatrixOpData::MatrixArrayPtr build_conversion_matrix(const Primaries & src_prims,
                                                     const Primaries & dst_prims,
                                                     const MatrixOpData::Offsets & src_wht_XYZ,
                                                     const MatrixOpData::Offsets & dst_wht_XYZ,
                                                     AdaptationMethod method)
{
    static const MatrixOpData::Offsets ones(1., 1., 1., 0.);

    // Calculate the primary conversion matrices.
    MatrixOpData::MatrixArrayPtr src_rgb2xyz = rgb2xyz_from_xy(src_prims);
    MatrixOpData::MatrixArrayPtr dst_rgb2xyz = rgb2xyz_from_xy(dst_prims);
    MatrixOpData::MatrixArrayPtr dst_xyz2rgb = dst_rgb2xyz->inverse();

    // Return the composed matrix if no white point adaptation is needed.
    if ( !src_wht_XYZ.isNotNull() && !dst_wht_XYZ.isNotNull() )
    {
        // If the white points are equal, don't need to adapt.
        if ( src_prims.m_wht.m_xy[0] == dst_prims.m_wht.m_xy[0] &&
             src_prims.m_wht.m_xy[1] == dst_prims.m_wht.m_xy[1] )
        {
            return dst_xyz2rgb->inner(src_rgb2xyz);
        }
    }
    if ( method == ADAPTATION_NONE )
    {
        return dst_xyz2rgb->inner(src_rgb2xyz);
    }

    // Calculate src and dst white XYZ.
    MatrixOpData::Offsets src_wht, dst_wht;
    if ( dst_wht_XYZ.isNotNull() )
    {
        dst_wht = dst_wht_XYZ;
    }
    else
    {
        dst_wht = dst_rgb2xyz->inner(ones);
    }
    if ( src_wht_XYZ.isNotNull() )
    {
        src_wht = src_wht_XYZ;
    }
    else
    {
        src_wht = src_rgb2xyz->inner(ones);
    }

    // Build the adaptation matrix (may be an identity).
    MatrixOpData::MatrixArrayPtr vkmat = build_vonkries_adapt(src_wht, dst_wht, method);

    // Compose the adaptation into the conversion matrix.
    return dst_xyz2rgb->inner(vkmat->inner(src_rgb2xyz));
}

MatrixOpData::MatrixArrayPtr build_conversion_matrix(const Primaries & src_prims,
                                                     const Primaries & dst_prims,
                                                     AdaptationMethod method)
{
    static const MatrixOpData::Offsets zero(0., 0., 0., 0.);
    return build_conversion_matrix( src_prims, dst_prims, zero, zero, method );
}

MatrixOpData::MatrixArrayPtr build_conversion_matrix_to_XYZ_D65(const Primaries & src_prims,
                                                                AdaptationMethod method)
{
    static const MatrixOpData::Offsets zero(0., 0., 0., 0.);
    return build_conversion_matrix( src_prims, CIE_XYZ_ILLUM_E::primaries, zero, WHITEPOINT::D65_XYZ, method );
}

MatrixOpData::MatrixArrayPtr build_conversion_matrix_from_XYZ_D65(const Primaries & dst_prims,
                                                                  AdaptationMethod method)
{
    static const MatrixOpData::Offsets zero(0., 0., 0., 0.);
    return build_conversion_matrix( CIE_XYZ_ILLUM_E::primaries, dst_prims, WHITEPOINT::D65_XYZ, zero, method );
}

} // namespace OCIO_NAMESPACE
