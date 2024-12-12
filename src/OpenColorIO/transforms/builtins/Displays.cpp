// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/fixedfunction/FixedFunctionOp.h"
#include "ops/gamma/GammaOp.h"
#include "ops/matrix/MatrixOp.h"
#include "ops/range/RangeOp.h"
#include "transforms/builtins/BuiltinTransformRegistry.h"
#include "transforms/builtins/ColorMatrixHelpers.h"
#include "transforms/builtins/Displays.h"
#include "transforms/builtins/OpHelpers.h"

// This is a preparation for OCIO-lite where LUT support may be turned off.
#ifndef OCIO_LUT_SUPPORT
#   define OCIO_LUT_SUPPORT 1
#endif 

namespace OCIO_NAMESPACE
{

namespace DISPLAY
{

namespace ST_2084
{

#if OCIO_LUT_SUPPORT

static constexpr double m1 = 0.25 * 2610. / 4096.;
static constexpr double m2 = 128. * 2523. / 4096.;
static constexpr double c2 = 32. * 2413. / 4096.;
static constexpr double c3 = 32. * 2392. / 4096.;
static constexpr double c1 = c3 - c2 + 1.;

void GeneratePQToLinearOps(OpRcPtrVec& ops)
{
    auto GenerateLutValues = [](double input) -> float
    {
        const double N = std::abs(input);   // mirror about 0
        const double x = std::pow(N, 1. / m2);
        double L = std::pow( std::max(0., x - c1) / (c2 - c3 * x), 1. / m1 );
        // L is in nits/10000, convert to nits/100.
        L *= 100.;

        return float(std::copysign(L, input));
    };

    CreateHalfLut(ops, GenerateLutValues);
}

void GenerateLinearToPQOps(OpRcPtrVec& ops)
{
    auto GenerateLutValues = [](double input) -> float
    {
        // Input is in nits/100, convert to [0,1], where 1 is 10000 nits.
        const double L = std::abs(input * 0.01);
        const double y = std::pow(L, m1);
        const double ratpoly = (c1 + c2 * y) / (1. + c3 * y);
        const double N = std::pow( std::max(0., ratpoly), m2 );

        return float(std::copysign(N, input));
    };

    CreateHalfLut(ops, GenerateLutValues);
}

#else

void GeneratePQToLinearOps(OpRcPtrVec& ops)
{
    CreateFixedFunctionOp(ops, FixedFunctionOpData::PQ_TO_LIN, {});
}

void GenerateLinearToPQOps(OpRcPtrVec& ops)
{
    CreateFixedFunctionOp(ops, FixedFunctionOpData::LIN_TO_PQ, {});
}

#endif // OCIO_LUT_SUPPORT

} // namespace ST_2084

namespace HLG
{

static constexpr double Lw = 1000.;
static constexpr double E_MAX = 3.;

static constexpr double a = 0.17883277;
static constexpr double b = (1. - 4. * a) * E_MAX / 12.;
static const     double c0 = 0.5 - a * std::log(4. * a);
static const     double c = std::log(12. / E_MAX) * a + c0;
static constexpr double E_scale = 3. / E_MAX;
static constexpr double E_break = E_MAX / 12.;

#if OCIO_LUT_SUPPORT

void GenerateHLGToLinearOps(OpRcPtrVec& ops)
{
    auto GenerateLutValues = [](double in) -> float
        {
            double out = 0.0;
            const double Eprime = std::abs(in);   // mirror about 0
            if (Eprime < 0.5)
            {
                out = Eprime * Eprime / E_scale;
            }
            else
            {
                out = b + std::exp( (Eprime - c) / a );
            }
            return float(std::copysign(out, in));
        };

    CreateHalfLut(ops, GenerateLutValues);
}

void GenerateLinearToHLGOps(OpRcPtrVec& ops)
{
    auto GenerateLutValues = [](double in) -> float
        {
            double out = 0.0;
            const double E = std::abs(in);   // mirror about 0
            if (E < E_break)
            {
                out = std::sqrt(E * E_scale);
            }
            else
            {
                out = a * std::log(E - b) + c;
            }
            return float(std::copysign(out, in));
        };

    CreateHalfLut(ops, GenerateLutValues);
}

#else

static const FixedFunctionOpData::Params params
{
    0.0,                // mirror point
    E_break,            // break point

    // Gamma segment.
    0.5,                // gamma power
    std::sqrt(E_scale), // post-power scale
    0.0,                // pre-power offset

    // Log segment.
    std::exp(1.0),      // log base
    a,                  // log-side slope
    c,                  // log-side offset
    1.0,                // lin-side slope
    -b,                 // lin-side offset
};

void GenerateHLGToLinearOps(OpRcPtrVec& ops)
{
    CreateFixedFunctionOp(ops, FixedFunctionOpData::GAMMA_LOG_TO_LIN, params);
}

void GenerateLinearToHLGOps(OpRcPtrVec& ops)
{
    CreateFixedFunctionOp(ops, FixedFunctionOpData::LIN_TO_GAMMA_LOG, params);
}

#endif // OCIO_LUT_SUPPORT

} // namespace HLG

void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept
{
    {
        auto CIE_XYZ_D65_to_REC1886_REC709_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix_from_XYZ_D65(REC709::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

            const GammaOpData::Params rgbParams   = { 2.4 };
            const GammaOpData::Params alphaParams = { 1.0 };
            auto gammaData = std::make_shared<GammaOpData>(GammaOpData::BASIC_REV,
                                                           rgbParams, rgbParams, rgbParams, alphaParams);
            CreateGammaOp(ops, gammaData, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_REC.1886-REC.709", 
                            "Convert CIE XYZ (D65 white) to Rec.1886/Rec.709 (HD video)",
                            CIE_XYZ_D65_to_REC1886_REC709_Functor);
    }

    {
        auto CIE_XYZ_D65_to_REC1886_REC2020_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix_from_XYZ_D65(REC2020::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

            const GammaOpData::Params rgbParams   = { 2.4 };
            const GammaOpData::Params alphaParams = { 1.0 };
            auto gammaData = std::make_shared<GammaOpData>(GammaOpData::BASIC_REV,
                                                           rgbParams, rgbParams, rgbParams, alphaParams);
            CreateGammaOp(ops, gammaData, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_REC.1886-REC.2020", 
                            "Convert CIE XYZ (D65 white) to Rec.1886/Rec.2020 (UHD video)",
                            CIE_XYZ_D65_to_REC1886_REC2020_Functor);
    }

    {
        auto CIE_XYZ_D65_to_G22_REC709_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix_from_XYZ_D65(REC709::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

            const GammaOpData::Params rgbParams   = { 2.2 };
            const GammaOpData::Params alphaParams = { 1.0 };
            auto gammaData = std::make_shared<GammaOpData>(GammaOpData::BASIC_REV,
                                                           rgbParams, rgbParams, rgbParams, alphaParams);
            CreateGammaOp(ops, gammaData, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_G2.2-REC.709", 
                            "Convert CIE XYZ (D65 white) to Gamma2.2, Rec.709",
                            CIE_XYZ_D65_to_G22_REC709_Functor);
    }

    {
        auto CIE_XYZ_D65_to_SRGB_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix_from_XYZ_D65(REC709::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

            const GammaOpData::Params rgbParams   = { 2.4, 0.055 };
            const GammaOpData::Params alphaParams = { 1.0, 0.0 };
            auto gammaData = std::make_shared<GammaOpData>(GammaOpData::MONCURVE_REV,
                                                           rgbParams, rgbParams, rgbParams, alphaParams);
            CreateGammaOp(ops, gammaData, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_sRGB", 
                            "Convert CIE XYZ (D65 white) to sRGB (piecewise EOTF)",
                            CIE_XYZ_D65_to_SRGB_Functor);
    }

    {
        auto CIE_XYZ_D65_to_P3_DCI_BFD_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix_from_XYZ_D65(P3_DCI::primaries, ADAPTATION_BRADFORD);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

            const GammaOpData::Params rgbParams   = { 2.6 };
            const GammaOpData::Params alphaParams = { 1.0 };
            auto gammaData = std::make_shared<GammaOpData>(GammaOpData::BASIC_REV,
                                                           rgbParams, rgbParams, rgbParams, alphaParams);
            CreateGammaOp(ops, gammaData, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_G2.6-P3-DCI-BFD", 
                            "Convert CIE XYZ (D65 white) to Gamma 2.6, P3-DCI (DCI white with Bradford adaptation)",
                            CIE_XYZ_D65_to_P3_DCI_BFD_Functor);
    }

    {
        auto CIE_XYZ_D65_to_P3_D65_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix_from_XYZ_D65(P3_D65::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

            const GammaOpData::Params rgbParams   = { 2.6 };
            const GammaOpData::Params alphaParams = { 1.0 };
            auto gammaData = std::make_shared<GammaOpData>(GammaOpData::BASIC_REV,
                                                           rgbParams, rgbParams, rgbParams, alphaParams);
            CreateGammaOp(ops, gammaData, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_G2.6-P3-D65", 
                            "Convert CIE XYZ (D65 white) to Gamma 2.6, P3-D65",
                            CIE_XYZ_D65_to_P3_D65_Functor);
    }

    {
        auto CIE_XYZ_D65_to_P3_D60_BFD_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix_from_XYZ_D65(P3_D60::primaries, ADAPTATION_BRADFORD);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

            const GammaOpData::Params rgbParams   = { 2.6 };
            const GammaOpData::Params alphaParams = { 1.0 };
            auto gammaData = std::make_shared<GammaOpData>(GammaOpData::BASIC_REV,
                                                           rgbParams, rgbParams, rgbParams, alphaParams);
            CreateGammaOp(ops, gammaData, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_G2.6-P3-D60-BFD", 
                            "Convert CIE XYZ (D65 white) to Gamma 2.6, P3-D60 (Bradford adaptation)",
                            CIE_XYZ_D65_to_P3_D60_BFD_Functor);
    }

    {
        auto CIE_XYZ_D65_to_DCDM_D65_Functor = [](OpRcPtrVec & ops)
        {
            const double scale     = 48.0 / 52.37;
            const double scale4[4] = { scale, scale, scale, 1. };
            CreateScaleOp(ops, scale4, TRANSFORM_DIR_FORWARD);

            const GammaOpData::Params rgbParams   = { 2.6 };
            const GammaOpData::Params alphaParams = { 1.0 };
            auto gammaData = std::make_shared<GammaOpData>(GammaOpData::BASIC_REV,
                                                           rgbParams, rgbParams, rgbParams, alphaParams);
            CreateGammaOp(ops, gammaData, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_DCDM-D65",
                            "Convert CIE XYZ (D65 white) to Gamma 2.6 (D65 white in XYZ-E encoding)",
                            CIE_XYZ_D65_to_DCDM_D65_Functor);
    }

    {
        auto CIE_XYZ_D65_to_DisplayP3_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix_from_XYZ_D65(P3_D65::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

            // This color space is intended to be useful for macOS color spaces
            // kCGColorSpaceDisplayP3 and kCGColorSpaceExtendedDisplayP3.
            // The developer documentation does not seem to detail how the
            // transfer function is extended below 0 for the "extended" version.
            // However it does say that it uses the sRGB transfer function and
            // the kCGColorSpaceExtendedSRGB color space extends by reflecting
            // the curve around 0.  Hence the use here of MONCURVE_MIRROR_REV.
            // As with the other displays here, this built-in should be used
            // with a RangeTransform to limit the results to [0,1], if necessary.
            //
            // https://developer.apple.com/documentation/coregraphics/kcgcolorspacedisplayp3
            // https://developer.apple.com/documentation/appkit/nscolorspace/1644175-extendedsrgbcolorspace

            const GammaOpData::Params rgbParams   = { 2.4, 0.055 };
            const GammaOpData::Params alphaParams = { 1.0, 0.0 };
            auto gammaData = std::make_shared<GammaOpData>(GammaOpData::MONCURVE_MIRROR_REV,
                                                           rgbParams, rgbParams, rgbParams, alphaParams);
            CreateGammaOp(ops, gammaData, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_DisplayP3", 
                            "Convert CIE XYZ (D65 white) to Apple Display P3",
                            CIE_XYZ_D65_to_DisplayP3_Functor);

        // NOTE: This builtin is defined to be able to partition SDR and HDR view transforms under two separate
        // displays rather than a single one.
        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_DisplayP3-HDR",
                            "Convert CIE XYZ (D65 white) to Apple Display P3 (HDR)",
                            CIE_XYZ_D65_to_DisplayP3_Functor);
    }

    {
        auto ST2084_to_Linear_Functor = [](OpRcPtrVec & ops)
        {
            ST_2084::GeneratePQToLinearOps(ops);
        };

        registry.addBuiltin("CURVE - ST-2084_to_LINEAR",
                            "Convert SMPTE ST-2084 (PQ) full-range to linear nits/100",
                            ST2084_to_Linear_Functor);
    }

    {
        auto Linear_to_ST2084_Functor = [](OpRcPtrVec & ops)
        {
            ST_2084::GenerateLinearToPQOps(ops);
        };

        registry.addBuiltin("CURVE - LINEAR_to_ST-2084",
                            "Convert linear nits/100 to SMPTE ST-2084 (PQ) full-range",
                            Linear_to_ST2084_Functor);
    }

    {
        auto CIE_XYZ_D65_to_REC2100_PQ_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix_from_XYZ_D65(REC2020::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

            ST_2084::GenerateLinearToPQOps(ops);
        };

        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_REC.2100-PQ", 
                            "Convert CIE XYZ (D65 white) to Rec.2100-PQ",
                            CIE_XYZ_D65_to_REC2100_PQ_Functor);
    }

    {
        auto CIE_XYZ_D65_to_ST2084_P3_D65_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix_from_XYZ_D65(P3_D65::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

            ST_2084::GenerateLinearToPQOps(ops);
        };

        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_ST2084-P3-D65", 
                            "Convert CIE XYZ (D65 white) to ST-2084 (PQ), P3-D65 primaries",
                            CIE_XYZ_D65_to_ST2084_P3_D65_Functor);
    }

    {
        auto CIE_XYZ_D65_to_ST2084_DCDM_D65_Functor = [](OpRcPtrVec & ops)
        {
            ST_2084::GenerateLinearToPQOps(ops);
        };

        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_ST2084-DCDM-D65", 
                            "Convert CIE XYZ (D65 white) to ST-2084 (PQ) (D65 white in XYZ-E encoding)",
                            CIE_XYZ_D65_to_ST2084_DCDM_D65_Functor);
    }

    {
        auto HLG_to_Linear_Functor = [](OpRcPtrVec & ops)
        {
            HLG::GenerateHLGToLinearOps(ops);
        };

        registry.addBuiltin("CURVE - HLG-OETF-INVERSE",
                            "Apply ITU-R BT.2100 (HLG) OETF inverse, scaled with HLG 0.42 at 18% grey",
                            HLG_to_Linear_Functor);
    }

    {
        auto Linear_to_HLG_Functor = [](OpRcPtrVec & ops)
        {
            HLG::GenerateLinearToHLGOps(ops);
        };

        registry.addBuiltin("CURVE - HLG-OETF",
                            "Apply ITU-R BT.2100 (HLG) OETF, scaled with 18% grey at HLG 0.42",
                            Linear_to_HLG_Functor);
    }

    {
        auto CIE_XYZ_D65_to_REC2100_HLG_1000nit_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix_from_XYZ_D65(REC2020::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

            const double gamma            = 1.2 + 0.42 * std::log10(HLG::Lw / 1000.);
            {
                static constexpr double scale     = 100.;
                static constexpr double scale4[4] = { scale, scale, scale, 1. };
                CreateScaleOp(ops, scale4, TRANSFORM_DIR_FORWARD);
            }
            {
                const double scale     = std::pow(HLG::E_MAX, gamma) / HLG::Lw;
                const double scale4[4] = { scale, scale, scale, 1. };
                CreateScaleOp(ops, scale4, TRANSFORM_DIR_FORWARD);
            }

            CreateFixedFunctionOp(ops, FixedFunctionOpData::REC2100_SURROUND_FWD, {1. / gamma});

            HLG::GenerateLinearToHLGOps(ops);
        };

        registry.addBuiltin("DISPLAY - CIE-XYZ-D65_to_REC.2100-HLG-1000nit", 
                            "Convert CIE XYZ (D65 white) to Rec.2100-HLG, 1000 nit",
                            CIE_XYZ_D65_to_REC2100_HLG_1000nit_Functor);
    }

}

} // namespace DISPLAY

} // namespace OCIO_NAMESPACE
