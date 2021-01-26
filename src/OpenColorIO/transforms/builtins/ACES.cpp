// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "OpenEXR/half.h"
#include "ops/fixedfunction/FixedFunctionOp.h"
#include "ops/gradingrgbcurve/GradingRGBCurveOp.h"
#include "ops/log/LogOp.h"
#include "ops/matrix/MatrixOp.h"
#include "ops/range/RangeOp.h"
#include "transforms/builtins/ACES.h"
#include "transforms/builtins/BuiltinTransformRegistry.h"
#include "transforms/builtins/OpHelpers.h"


namespace OCIO_NAMESPACE
{

//
// Define component functions for reuse in multiple built-ins.
//

namespace AP1_to_CIE_XYZ_D65
{

void GenerateOps(OpRcPtrVec & ops)
{
    MatrixOpData::MatrixArrayPtr matrix
        = build_conversion_matrix_to_XYZ_D65(ACES_AP1::primaries, ADAPTATION_BRADFORD);
    CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
}

}  // namespace AP1_to_CIE_XYZ_D65

namespace ACEScct_to_LINEAR
{

static constexpr double linSideSlope  = 1.;
static constexpr double linSideOffset = 0.;
static constexpr double logSideSlope  = 1. / 17.52;
static constexpr double logSideOffset = 9.72 / 17.52;
static constexpr double linSideBreak  = 0.0078125;
static constexpr double base          = 2.;

static const LogOpData::Params 
    params { logSideSlope, logSideOffset, linSideSlope, linSideOffset, linSideBreak };

static const LogOpData log(base, params, params, params, TRANSFORM_DIR_INVERSE);

}  // namespace ACEScct_to_LINEAR

namespace ADX_to_ACES
{

static constexpr unsigned lutSize = 11;
static constexpr double nonuniform_LUT[lutSize * 2]
{
    -0.190000000000000, -6.000000000000000,
     0.010000000000000, -2.721718645000000,
     0.028000000000000, -2.521718645000000,
     0.054000000000000, -2.321718645000000,
     0.095000000000000, -2.121718645000000,
     0.145000000000000, -1.921718645000000,
     0.220000000000000, -1.721718645000000,
     0.300000000000000, -1.521718645000000,
     0.400000000000000, -1.321718645000000,
     0.500000000000000, -1.121718645000000,
     0.600000000000000, -0.926545676714876
};

void GenerateOps(OpRcPtrVec & ops)
{
    // Note that in CTL, the matrices are stored transposed.
    static constexpr double CDD_TO_CID[4 * 4]
    {
        0.75573,  0.22197,  0.02230,  0.,
        0.05901,  0.96928, -0.02829,  0.,
        0.16134,  0.07406,  0.76460,  0.,
        0.,       0.,       0.,       1.
    };

    // Convert Channel Dependent Density values into Channel Independent Density values.
    CreateMatrixOp(ops, &CDD_TO_CID[0], TRANSFORM_DIR_FORWARD);

    auto GenerateLutValues = [](double in) -> float
    {
        double out = 0.;

        if (in < nonuniform_LUT[0]) // Lower bound i.e. in < nonuniform_LUT[0, 0].
        {
            // Extrapolate to ease conversion to LUT1D.
            const double slope = (nonuniform_LUT[3] - nonuniform_LUT[1])
                                    / (nonuniform_LUT[2] - nonuniform_LUT[0]);

            out = nonuniform_LUT[1] - slope * (nonuniform_LUT[0] - in);

            if (out < -10.) out = -10.;
        }
        else if (in <= nonuniform_LUT[(lutSize-1) * 2])
        {
            out = Interpolate1D(lutSize, &nonuniform_LUT[0], in);
        }
        else // Upper bound i.e. in > nonuniform_LUT[lutSize-1, 0].
        {
            const double REF_PT
                = (7120. - 1520.) / 8000. * (100. / 55.) - std::log10(0.18);

            out = (100. / 55.) * in - REF_PT;

            if (out > 4.8162678) out = 4.8162678;  // log10(HALF_MAX)
        }

        return float(out);
    };

    // Convert Channel Independent Density values to Relative Log Exposure values.
    CreateHalfLut(ops, GenerateLutValues);

    // Convert Relative Log Exposure values to Relative Exposure values.
    CreateLogOp(ops, 10., TRANSFORM_DIR_INVERSE);

    static constexpr double EXP_TO_ACES[4 * 4]
    {
        0.72286,  0.12630,  0.15084,  0.,
        0.11923,  0.76418,  0.11659,  0.,
        0.01427,  0.08213,  0.90359,  0.,
        0.,       0.,       0.,       1.
    };

    // Convert Relative Exposure values to ACES values.
    CreateMatrixOp(ops, &EXP_TO_ACES[0], TRANSFORM_DIR_FORWARD);            
}

}  // namespace ADX_to_ACES

namespace ACES_OUTPUT
{

void Generate_RRT_preamble_ops(OpRcPtrVec & ops)
{
    CreateFixedFunctionOp(ops, FixedFunctionOpData::ACES_GLOW_10_FWD, {});

    CreateFixedFunctionOp(ops, FixedFunctionOpData::ACES_RED_MOD_10_FWD, {});

    CreateRangeOp(ops, 
                  0., RangeOpData::EmptyValue(),  // don't clamp high end
                  0., RangeOpData::EmptyValue(),  // don't clamp high end
                  TRANSFORM_DIR_FORWARD);

    MatrixOpData::MatrixArrayPtr matrix
        = build_conversion_matrix(ACES_AP0::primaries, ACES_AP1::primaries, ADAPTATION_NONE);
    CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

    CreateRangeOp(ops, 
                  0., RangeOpData::EmptyValue(),  // don't clamp high end
                  0., RangeOpData::EmptyValue(),  // don't clamp high end
                  TRANSFORM_DIR_FORWARD);

    static constexpr double RRT_SAT_MAT[4 * 4]
    {
        0.970889148671, 0.026963270632, 0.002147580696, 0., 
        0.010889148671, 0.986963270632, 0.002147580696, 0., 
        0.010889148671, 0.026963270632, 0.962147580696, 0., 
        0., 0., 0., 1.
    };
    CreateMatrixOp(ops, &RRT_SAT_MAT[0], TRANSFORM_DIR_FORWARD);            
}

void Generate_tonecurve_ops(OpRcPtrVec & ops)
{
    // Convert to Log space.
    CreateLogOp(ops, 10., TRANSFORM_DIR_FORWARD);

    // Apply RRT shaper using the same quadratic B-spline as the CTL.
    {
        auto curve = GradingBSplineCurve::Create({
                {-5.26017743f, -4.f},
                {-3.75502745f, -3.57868829f},
                {-2.24987747f, -1.82131329f},
                {-0.74472749f,  0.68124124f},
                { 1.06145248f,  2.87457742f},
                { 2.86763245f,  3.83406206f},
                { 4.67381243f,  4.f}
            });
        float slopes[] = { 0.f,  0.55982688f,  1.77532247f,  1.55f,  0.8787017f,  0.18374463f,  0.f };
        for (size_t i = 0; i < 7; ++i)
        {
            curve->setSlope( i, slopes[i] );
        }
        ConstGradingBSplineCurveRcPtr m = curve;
        auto identity = GradingBSplineCurve::Create({ { 0.f, 0.f }, { 1.f, 1.f } });
        ConstGradingBSplineCurveRcPtr z = identity;
        auto gc = std::make_shared<GradingRGBCurveOpData>(GRADING_LOG, z, z, z, m);

        CreateGradingRGBCurveOp(ops, gc, TRANSFORM_DIR_FORWARD);
    }

    // Apply SDR ODT shaper using the same quadratic B-spline as the CTL.
    {
        auto curve = GradingBSplineCurve::Create({
                {-2.54062362f,  -1.69897000f},
                {-2.08035721f,  -1.58843500f},
                {-1.62009080f,  -1.35350000f},
                {-1.15982439f,  -1.04695000f},
                {-0.69955799f,  -0.65640000f},
                {-0.23929158f,  -0.22141000f},
                { 0.22097483f,   0.22814402f},
                { 0.68124124f,   0.68124124f},
                { 1.01284632f,   0.99142189f},
                { 1.34445140f,   1.25800000f},
                { 1.67605648f,   1.44995000f},
                { 2.00766156f,   1.55910000f},
                { 2.33926665f,   1.62260000f},
                { 2.67087173f,   1.66065457f},
                { 3.00247681f,   1.68124124f}
            });
        float slopes[] = { 0.f,  0.4803088f,   0.5405565f,   0.79149813f,  0.9055625f,  0.98460368f,
                           0.96884766f,  1.f,  0.87078346f,  0.73702127f,  0.42068113f,  0.23763206f,
                           0.14535362f,  0.08416378f,  0.04f };
        for (size_t i = 0; i < 15; ++i)
        {
            curve->setSlope( i, slopes[i] );
        }
        ConstGradingBSplineCurveRcPtr m = curve;
        auto identity = GradingBSplineCurve::Create({ { 0.f, 0.f }, { 1.f, 1.f } });
        ConstGradingBSplineCurveRcPtr z = identity;
        auto gc = std::make_shared<GradingRGBCurveOpData>(GRADING_LOG, z, z, z, m);

        CreateGradingRGBCurveOp(ops, gc, TRANSFORM_DIR_FORWARD);
    }

    // Undo the logarithm.
    CreateLogOp(ops, 10., TRANSFORM_DIR_INVERSE);

    // Apply Cinema White/Black correction.
    {
        static constexpr double CINEMA_WHITE = 48.;
        // Note: ACESlib.ODT_Common.ctl claims that using pow10(log10(0.02) for black improves performance at 0,
        // but that does not seem to be the case here, 0 input currently gives about 4e-11 XYZ output either way.
        static constexpr double CINEMA_BLACK = 0.02;
        static constexpr double scale        = 1. / (CINEMA_WHITE - CINEMA_BLACK);
        static constexpr double scale4[4]    = { scale, scale, scale, 1. };
        static constexpr double offset       = -CINEMA_BLACK * scale;
        static constexpr double offset4[4]   = { offset, offset, offset, 0. };

        CreateScaleOffsetOp(ops, scale4, offset4, TRANSFORM_DIR_FORWARD);
    }
}

void Generate_video_adjustment_ops(OpRcPtrVec & ops)
{
    // Surround correction for cinema to video.
    CreateFixedFunctionOp(ops, FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD, {});

    // Desat to compensate 48 nit to 100 nit brightness.
    static constexpr double DESAT_100_NITS[4 * 4]
    {
        0.949056010175, 0.047185723607, 0.003758266219, 0., 
        0.019056010175, 0.977185723607, 0.003758266219, 0., 
        0.019056010175, 0.047185723607, 0.933758266219, 0., 
        0., 0., 0., 1.
    };
    CreateMatrixOp(ops, &DESAT_100_NITS[0], TRANSFORM_DIR_FORWARD);
}

void Generate_hdr_tonecurve_ops(OpRcPtrVec & ops, double Y_MAX)
{
    // Convert to Log space.
    CreateLogOp(ops, 10., TRANSFORM_DIR_FORWARD);

    // Apply RRT shaper using the same quadratic B-spline as the CTL.
    if (Y_MAX == 1000.)
    {
        auto curve = GradingBSplineCurve::Create({
                { -5.60050155f,  -4.00000000f },
                { -4.09535157f,  -3.57868829f },
                { -2.59020159f,  -1.82131329f },
                { -1.08505161f,   0.68124124f },
                {  0.22347059f,   2.22673503f },
                {  1.53199279f,   2.87906206f },
                {  2.84051500f,   3.00000000f }
            });
        const float slopes[] = { 0.f,  0.55982688f,  1.77532247f,  1.55f,  0.81219728f,  0.1848466f,  0.f };
        for (size_t i = 0; i < 7; ++i)
        {
            curve->setSlope( i, slopes[i] );
        }
        ConstGradingBSplineCurveRcPtr m = curve;
        auto identity = GradingBSplineCurve::Create({ { 0.f, 0.f }, { 1.f, 1.f } });
        ConstGradingBSplineCurveRcPtr z = identity;
        auto gc = std::make_shared<GradingRGBCurveOpData>(GRADING_LOG, z, z, z, m);

        CreateGradingRGBCurveOp(ops, gc, TRANSFORM_DIR_FORWARD);
    }
    else if (Y_MAX == 2000.)
    {
        auto curve = GradingBSplineCurve::Create({
                { -5.59738488f,  -4.00000000f },
                { -4.09223490f,  -3.57868829f },
                { -2.58708492f,  -1.82131329f },
                { -1.08193494f,   0.68124124f },
                {  0.37639718f,   2.42130131f },
                {  1.83472930f,   3.16609199f },
                {  3.29306142f,   3.30103000f },
            });
        const float slopes[] = { 0.f,  0.55982688f,  1.77532247f,  1.55f,  0.83637009f,  0.18505799f,  0.f };
        for (size_t i = 0; i < 7; ++i)
        {
            curve->setSlope( i, slopes[i] );
        }
        ConstGradingBSplineCurveRcPtr m = curve;
        auto identity = GradingBSplineCurve::Create({ { 0.f, 0.f }, { 1.f, 1.f } });
        ConstGradingBSplineCurveRcPtr z = identity;
        auto gc = std::make_shared<GradingRGBCurveOpData>(GRADING_LOG, z, z, z, m);

        CreateGradingRGBCurveOp(ops, gc, TRANSFORM_DIR_FORWARD);
    }
    else if (Y_MAX == 4000.)
    {
        auto curve = GradingBSplineCurve::Create({
                { -5.59503319f,  -4.00000000f },
                { -4.08988322f,  -3.57868829f },
                { -2.58473324f,  -1.82131329f },
                { -1.07958326f,   0.68124124f },
                {  0.52855878f,   2.61625839f },
                {  2.13670081f,   3.45351273f },
                {  3.74484285f,   3.60205999f },
            });
        const float slopes[] = { 0.f,  0.55982688f,  1.77532247f,  1.55f,  0.85652519f,  0.18474395f,  0.f };
        for (size_t i = 0; i < 7; ++i)
        {
            curve->setSlope( i, slopes[i] );
        }
        ConstGradingBSplineCurveRcPtr m = curve;
        auto identity = GradingBSplineCurve::Create({ { 0.f, 0.f }, { 1.f, 1.f } });
        ConstGradingBSplineCurveRcPtr z = identity;
        auto gc = std::make_shared<GradingRGBCurveOpData>(GRADING_LOG, z, z, z, m);

        CreateGradingRGBCurveOp(ops, gc, TRANSFORM_DIR_FORWARD);
    }
    else if (Y_MAX == 108.)
    {
        auto curve = GradingBSplineCurve::Create({
                { -5.37852506f,  -4.00000000f },
                { -3.87337508f,  -3.57868829f },
                { -2.36822510f,  -1.82131329f },
                { -0.86307513f,   0.68124124f },
                { -0.03557710f,   1.60464482f },
                {  0.79192092f,   1.96008059f },
                {  1.61941895f,   2.03342376f },
            });
        const float slopes[] = { 0.f,  0.55982688f,  1.77532247f,  1.55f,  0.68179646f,  0.17726487f,  0.f };
        for (size_t i = 0; i < 7; ++i)
        {
            curve->setSlope( i, slopes[i] );
        }
        ConstGradingBSplineCurveRcPtr m = curve;
        auto identity = GradingBSplineCurve::Create({ { 0.f, 0.f }, { 1.f, 1.f } });
        ConstGradingBSplineCurveRcPtr z = identity;
        auto gc = std::make_shared<GradingRGBCurveOpData>(GRADING_LOG, z, z, z, m);

        CreateGradingRGBCurveOp(ops, gc, TRANSFORM_DIR_FORWARD);
    }

    // Undo the logarithm.
    CreateLogOp(ops, 10., TRANSFORM_DIR_INVERSE);

    // Apply Cinema White/Black correction.
    {
        const double Y_MIN        = 0.0001;
        const double scale        = 1. / (Y_MAX - Y_MIN);
        const double scale4[4]    = { scale, scale, scale, 1. };
        const double offset       = -Y_MIN * scale;
        const double offset4[4]   = { offset, offset, offset, 0. };

        CreateScaleOffsetOp(ops, scale4, offset4, TRANSFORM_DIR_FORWARD);
    }
}

void Generate_sdr_primary_clamp_ops(OpRcPtrVec & ops, const Primaries & limitPrimaries)
{
    MatrixOpData::MatrixArrayPtr matrix1
        = build_conversion_matrix(ACES_AP1::primaries, limitPrimaries, ADAPTATION_BRADFORD);
    CreateMatrixOp(ops, matrix1, TRANSFORM_DIR_FORWARD);

    CreateRangeOp(ops, 
                  0., 1.,
                  0., 1.,
                  TRANSFORM_DIR_FORWARD);

    MatrixOpData::MatrixArrayPtr matrix2
        = rgb2xyz_from_xy(limitPrimaries);
    CreateMatrixOp(ops, matrix2, TRANSFORM_DIR_FORWARD);
}

void Generate_hdr_primary_clamp_ops(OpRcPtrVec & ops, const Primaries & limitPrimaries)
{
    MatrixOpData::MatrixArrayPtr matrix1
        = build_conversion_matrix(ACES_AP1::primaries, limitPrimaries, ADAPTATION_NONE);
    CreateMatrixOp(ops, matrix1, TRANSFORM_DIR_FORWARD);

    CreateRangeOp(ops, 
                  0., 1.,
                  0., 1.,
                  TRANSFORM_DIR_FORWARD);

    MatrixOpData::MatrixArrayPtr matrix2
        = rgb2xyz_from_xy(limitPrimaries);
    CreateMatrixOp(ops, matrix2, TRANSFORM_DIR_FORWARD);

    MatrixOpData::MatrixArrayPtr matrix3
        = build_vonkries_adapt(WHITEPOINT::D60_XYZ, WHITEPOINT::D65_XYZ, ADAPTATION_BRADFORD);
    CreateMatrixOp(ops, matrix3, TRANSFORM_DIR_FORWARD);
}

void Generate_nit_normalization_ops(OpRcPtrVec & ops, double nit_level)
{
    // The PQ curve expects nits / 100 as input.  Unnormalize 1.0 to the nit level for the transform
    // and then renormalize to put 100 nits at 1.0.
    const double scale     = nit_level * 0.01;
    const double scale4[4] = { scale, scale, scale, 1. };
    CreateScaleOp(ops, scale4, TRANSFORM_DIR_FORWARD);
}

void Generate_roll_white_d60_ops(OpRcPtrVec & ops)
{
    auto GenerateLutValues = [](double in) -> float
    {
        const double new_wht = 0.918;
        const double width = 0.5;
        const double x0 = -1.0;
        const double x1 = x0 + width;
        const double y0 = -new_wht;
        const double y1 = x1;
        const double m1 = (x1 - x0);
        const double a = y0 - y1 + m1;
        const double b = 2. * ( y1 - y0) - m1;
        const double c = y0;
        const double t = (-in - x0) / (x1 - x0);
        double out = 0.0;
        if (t < 0.0)
        {
            out = -(t * b + c);
        }
        else if (t > 1.0)
        {
            out = in;
        }
        else
        {
            out = -(( t * a + b) * t + c);
        }
        return float(out);
    };

    CreateHalfLut(ops, GenerateLutValues);
}

void Generate_roll_white_d65_ops(OpRcPtrVec & ops)
{
    auto GenerateLutValues = [](double in) -> float
    {
        const double new_wht = 0.908;
        const double width = 0.5;
        const double x0 = -1.0;
        const double x1 = x0 + width;
        const double y0 = -new_wht;
        const double y1 = x1;
        const double m1 = (x1 - x0);
        const double a = y0 - y1 + m1;
        const double b = 2. * ( y1 - y0) - m1;
        const double c = y0;
        const double t = (-in - x0) / (x1 - x0);
        double out = 0.0;
        if (t < 0.0)
        {
            out = -(t * b + c);
        }
        else if (t > 1.0)
        {
            out = in;
        }
        else
        {
            out = -(( t * a + b) * t + c);
        }
        return float(out);
    };

    CreateHalfLut(ops, GenerateLutValues);
}

}  // namespace ACES_OUTPUT


//
// Create the built-in transforms.
//

namespace ACES
{

// Register all the ACES related built-in tranforms.
void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept
{
    {
        auto ACES_AP0_to_CIE_XYZ_D65_BFD_Functor = [](OpRcPtrVec & ops)
        {
            // The CIE XYZ space has its conventional normalization (i.e., to illuminant E).
            // A neutral value of [1.,1.,1] in AP0 maps to the XYZ value of D65 ([0.9504..., 1., 1.089...]).
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix_to_XYZ_D65(ACES_AP0::primaries, ADAPTATION_BRADFORD);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("UTILITY - ACES-AP0_to_CIE-XYZ-D65_BFD", 
                            "Convert ACES AP0 primaries to CIE XYZ with a D65 white point with Bradford adaptation",
                            ACES_AP0_to_CIE_XYZ_D65_BFD_Functor);
    }
    {
        auto ACES_AP1_to_CIE_XYZ_D65_BFD_Functor = [](OpRcPtrVec & ops)
        {
            AP1_to_CIE_XYZ_D65::GenerateOps(ops);
        };

        registry.addBuiltin("UTILITY - ACES-AP1_to_CIE-XYZ-D65_BFD", 
                            "Convert ACES AP1 primaries to CIE XYZ with a D65 white point with Bradford adaptation",
                            ACES_AP1_to_CIE_XYZ_D65_BFD_Functor);
    }
    {
        auto ACES_AP1_to_LINEAR_REC709_BFD_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix(ACES_AP1::primaries, REC709::primaries, ADAPTATION_BRADFORD);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("UTILITY - ACES-AP1_to_LINEAR-REC709_BFD", 
                            "Convert ACES AP1 primaries to linear Rec.709 primaries with Bradford adaptation",
                            ACES_AP1_to_LINEAR_REC709_BFD_Functor);
    }
    {
        auto ACEScct_LOG_to_LIN_Functor = [](OpRcPtrVec & ops)
        {
            LogOpDataRcPtr log = ACEScct_to_LINEAR::log.clone();
            CreateLogOp(ops, log, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("CURVE - ACEScct-LOG_to_LINEAR", 
                            "Apply the log-to-lin curve used in ACEScct",
                            ACEScct_LOG_to_LIN_Functor);
    }
    {
        auto ACEScct_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            LogOpDataRcPtr log = ACEScct_to_LINEAR::log.clone();
            CreateLogOp(ops, log, TRANSFORM_DIR_FORWARD);

            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix(ACES_AP1::primaries, ACES_AP0::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("ACEScct_to_ACES2065-1", 
                            "Convert ACEScct to ACES2065-1",
                            ACEScct_to_ACES2065_1_Functor);
    }
    {
        auto ACEScc_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            auto GenerateLutValues = [](double input) -> float
            {
                // The functor input will be [0,1].  Remap this to a wider domain to better capture
                // the full extent of ACEScc.
                static constexpr double IN_MIN = -0.36;
                static constexpr double IN_MAX =  1.50;
                double in = input * (IN_MAX - IN_MIN) + IN_MIN;

                double out = 0.0f;

                if (in < ((9.72 - 15.0) / 17.52))
                {
                    out = (std::pow( 2., in * 17.52 - 9.72) - std::pow( 2., -16.)) * 2.0;
                }
                else
                {
                    out = std::pow( 2., in * 17.52 - 9.72);
                }
                // The CTL clamps at HALF_MAX, but it's better to avoid a slope discontinuity in a LUT.

                return float(out);
            };

            // Allow the LUT to work over a wider input range to better capture the ACEScc extent.
            CreateRangeOp(ops, 
                         -0.36, 1.5, 
                          0.00, 1.0,
                          TRANSFORM_DIR_FORWARD);

            CreateLut(ops, 4096, GenerateLutValues);

            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix(ACES_AP1::primaries, ACES_AP0::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

            // This helps when the transform is inverted to match the CTL, 
            // which clamps incoming ACES2065-1 values.
            CreateRangeOp(ops, 
                          0.00, RangeOpData::EmptyValue(), // don't clamp high end, 
                          0.00, RangeOpData::EmptyValue(), // don't clamp high end
                          TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("ACEScc_to_ACES2065-1", 
                            "Convert ACEScc to ACES2065-1",
                            ACEScc_to_ACES2065_1_Functor);
    }
    {
        auto ACEScg_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix(ACES_AP1::primaries, ACES_AP0::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("ACEScg_to_ACES2065-1",
                            "Convert ACEScg to ACES2065-1",
                            ACEScg_to_ACES2065_1_Functor);
    }
    {
        auto ACESproxy10i_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            CreateRangeOp(ops, 
                          64. / 1023.,                940. / 1023., 
                          (( 64 - 425.) / 50.) - 2.5, ((940 - 425.) / 50.) - 2.5,
                          TRANSFORM_DIR_FORWARD);

            CreateLogOp(ops, 2., TRANSFORM_DIR_INVERSE);

            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix(ACES_AP1::primaries, ACES_AP0::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("ACESproxy10i_to_ACES2065-1", 
                            "Convert ACESproxy 10i to ACES2065-1",
                            ACESproxy10i_to_ACES2065_1_Functor);
    }
    {
        auto ADX10_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            static constexpr double scale     = 1023. / 500.;
            static constexpr double scale4[4] = { scale, scale, scale, 1. };
    
            static constexpr double offset     = -95. / 500.;
            static constexpr double offset4[4] = { offset, offset, offset, 0. };

            // Convert ADX10 values to Channel Dependent Density values.
            CreateScaleOffsetOp(ops, scale4, offset4, TRANSFORM_DIR_FORWARD);

            // Convert to ACES2065-1.
            ADX_to_ACES::GenerateOps(ops);
        };

        registry.addBuiltin("ADX10_to_ACES2065-1",
                            "Convert ADX10 to ACES2065-1",
                            ADX10_to_ACES2065_1_Functor);
    }
    {
        auto ADX16_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            static constexpr double scale     = 65535. / 8000.;
            static constexpr double scale4[4] = { scale, scale, scale, 1. };
    
            static constexpr double offset     = -1520. / 8000.;
            static constexpr double offset4[4] = { offset, offset, offset, 0. };

            // Convert ADX16 values to Channel Dependent Density values.
            CreateScaleOffsetOp(ops, scale4, offset4, TRANSFORM_DIR_FORWARD);

            // Convert to ACES2065-1.
            ADX_to_ACES::GenerateOps(ops);
        };

        registry.addBuiltin("ADX16_to_ACES2065-1",
                            "Convert ADX16 to ACES2065-1",
                            ADX16_to_ACES2065_1_Functor);
    }
    {
        auto BLUE_LIGHT_FIX_Functor = [](OpRcPtrVec & ops)
        {
            // Note that in CTL, the matrices are stored transposed.
            static constexpr double BLUE_LIGHT_FIX[4 * 4]
            {
                0.9404372683, -0.0183068787,  0.0778696104, 0., 
                0.0083786969,  0.8286599939,  0.1629613092, 0., 
                0.0005471261, -0.0008833746,  1.0003362486, 0., 
                0., 0., 0., 1.
            };
            CreateMatrixOp(ops, &BLUE_LIGHT_FIX[0], TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("ACES-LMT - BLUE_LIGHT_ARTIFACT_FIX",
                            "LMT for desaturating blue hues to reduce clipping artifacts",
                            BLUE_LIGHT_FIX_Functor);
    }

    //
    // ACES OUTPUT TRANSFORMS
    //

    {
        auto ACES2065_1_to_CIE_XYZ_cinema_1_0_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_tonecurve_ops(ops);

            AP1_to_CIE_XYZ_D65::GenerateOps(ops);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA_1.0", 
                            "Component of ACES Output Transforms for SDR cinema",
                            ACES2065_1_to_CIE_XYZ_cinema_1_0_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_video_1_0_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_tonecurve_ops(ops);

            ACES_OUTPUT::Generate_video_adjustment_ops(ops);

            AP1_to_CIE_XYZ_D65::GenerateOps(ops);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO_1.0", 
                            "Component of ACES Output Transforms for SDR D65 video",
                            ACES2065_1_to_CIE_XYZ_video_1_0_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_cinema_rec709lim_1_1_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_tonecurve_ops(ops);

            ACES_OUTPUT::Generate_sdr_primary_clamp_ops(ops, REC709::primaries);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA-REC709lim_1.1", 
                            "Component of ACES Output Transforms for SDR cinema",
                            ACES2065_1_to_CIE_XYZ_cinema_rec709lim_1_1_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_video_rec709lim_1_1_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_tonecurve_ops(ops);

            ACES_OUTPUT::Generate_video_adjustment_ops(ops);

            ACES_OUTPUT::Generate_sdr_primary_clamp_ops(ops, REC709::primaries);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-REC709lim_1.1", 
                            "Component of ACES Output Transforms for SDR D65 video",
                            ACES2065_1_to_CIE_XYZ_video_rec709lim_1_1_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_video_p3lim_1_1_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_tonecurve_ops(ops);

            ACES_OUTPUT::Generate_video_adjustment_ops(ops);

            ACES_OUTPUT::Generate_sdr_primary_clamp_ops(ops, P3_D65::primaries);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-P3lim_1.1", 
                            "Component of ACES Output Transforms for SDR D65 video",
                            ACES2065_1_to_CIE_XYZ_video_p3lim_1_1_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_cinema_d60sim_1_1_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_tonecurve_ops(ops);

            CreateRangeOp(ops, 
                          RangeOpData::EmptyValue(), 1.0,  // don't clamp low end
                          RangeOpData::EmptyValue(), 1.0,  // don't clamp low end
                          TRANSFORM_DIR_FORWARD);

            static constexpr double scale     = 0.964;
            static constexpr double scale4[4] = { scale, scale, scale, 1. };
            CreateScaleOp(ops, scale4, TRANSFORM_DIR_FORWARD);

            MatrixOpData::MatrixArrayPtr matrix
                = rgb2xyz_from_xy(ACES_AP1::primaries);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA-D60sim-D65_1.1", 
                            "Component of ACES Output Transforms for SDR D65 cinema simulating D60 white",
                            ACES2065_1_to_CIE_XYZ_cinema_d60sim_1_1_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_video_d60sim_1_0_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_tonecurve_ops(ops);

            CreateRangeOp(ops, 
                          RangeOpData::EmptyValue(), 1.0,  // don't clamp low end
                          RangeOpData::EmptyValue(), 1.0,  // don't clamp low end
                          TRANSFORM_DIR_FORWARD);

            static constexpr double scale     = 0.955;
            static constexpr double scale4[4] = { scale, scale, scale, 1. };
            CreateScaleOp(ops, scale4, TRANSFORM_DIR_FORWARD);

            ACES_OUTPUT::Generate_video_adjustment_ops(ops);

            MatrixOpData::MatrixArrayPtr matrix
                = rgb2xyz_from_xy(ACES_AP1::primaries);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-D60sim-D65_1.0", 
                            "Component of ACES Output Transforms for SDR D65 video simulating D60 white",
                            ACES2065_1_to_CIE_XYZ_video_d60sim_1_0_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_cinema_d60sim_dci_1_0_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_tonecurve_ops(ops);

            ACES_OUTPUT::Generate_roll_white_d60_ops(ops);

            CreateRangeOp(ops, 
                          RangeOpData::EmptyValue(), 0.918,  // don't clamp low end
                          RangeOpData::EmptyValue(), 0.918,  // don't clamp low end
                          TRANSFORM_DIR_FORWARD);

            static constexpr double scale     = 0.96;
            static constexpr double scale4[4] = { scale, scale, scale, 1. };
            CreateScaleOp(ops, scale4, TRANSFORM_DIR_FORWARD);

            MatrixOpData::MatrixArrayPtr matrix
                = rgb2xyz_from_xy(ACES_AP1::primaries);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);

            MatrixOpData::MatrixArrayPtr matrix2
                = build_vonkries_adapt(WHITEPOINT::DCI_XYZ, WHITEPOINT::D65_XYZ, ADAPTATION_BRADFORD);
            CreateMatrixOp(ops, matrix2, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA-D60sim-DCI_1.0", 
                            "Component of ACES Output Transforms for SDR DCI cinema simulating D60 white",
                            ACES2065_1_to_CIE_XYZ_cinema_d60sim_dci_1_0_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_cinema_d65sim_dci_1_1_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_tonecurve_ops(ops);

            ACES_OUTPUT::Generate_roll_white_d65_ops(ops);

            CreateRangeOp(ops, 
                          RangeOpData::EmptyValue(), 0.908,  // don't clamp low end
                          RangeOpData::EmptyValue(), 0.908,  // don't clamp low end
                          TRANSFORM_DIR_FORWARD);

            static constexpr double scale     = 0.9575;
            static constexpr double scale4[4] = { scale, scale, scale, 1. };
            CreateScaleOp(ops, scale4, TRANSFORM_DIR_FORWARD);

            AP1_to_CIE_XYZ_D65::GenerateOps(ops);

            MatrixOpData::MatrixArrayPtr matrix2
                = build_vonkries_adapt(WHITEPOINT::DCI_XYZ, WHITEPOINT::D65_XYZ, ADAPTATION_BRADFORD);
            CreateMatrixOp(ops, matrix2, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-CINEMA-D65sim-DCI_1.1", 
                            "Component of ACES Output Transforms for SDR DCI cinema simulating D65 white",
                            ACES2065_1_to_CIE_XYZ_cinema_d65sim_dci_1_1_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_hdr_video_1000nits_rec2020lim_1_1_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_hdr_tonecurve_ops(ops, 1000.);

            ACES_OUTPUT::Generate_hdr_primary_clamp_ops(ops, REC2020::primaries);

            ACES_OUTPUT::Generate_nit_normalization_ops(ops, 1000.);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-1000nit-15nit-REC2020lim_1.1", 
                            "Component of ACES Output Transforms for 1000 nit HDR D65 video",
                            ACES2065_1_to_CIE_XYZ_hdr_video_1000nits_rec2020lim_1_1_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_hdr_video_1000nits_p3lim_1_1_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_hdr_tonecurve_ops(ops, 1000.);

            ACES_OUTPUT::Generate_hdr_primary_clamp_ops(ops, P3_D65::primaries);

            ACES_OUTPUT::Generate_nit_normalization_ops(ops, 1000.);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-1000nit-15nit-P3lim_1.1", 
                            "Component of ACES Output Transforms for 1000 nit HDR D65 video",
                            ACES2065_1_to_CIE_XYZ_hdr_video_1000nits_p3lim_1_1_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_hdr_video_2000nits_rec2020lim_1_1_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_hdr_tonecurve_ops(ops, 2000.);

            ACES_OUTPUT::Generate_hdr_primary_clamp_ops(ops, REC2020::primaries);

            ACES_OUTPUT::Generate_nit_normalization_ops(ops, 2000.);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-2000nit-15nit-REC2020lim_1.1", 
                            "Component of ACES Output Transforms for 2000 nit HDR D65 video",
                            ACES2065_1_to_CIE_XYZ_hdr_video_2000nits_rec2020lim_1_1_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_hdr_video_2000nits_p3lim_1_1_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_hdr_tonecurve_ops(ops, 2000.);

            ACES_OUTPUT::Generate_hdr_primary_clamp_ops(ops, P3_D65::primaries);

            ACES_OUTPUT::Generate_nit_normalization_ops(ops, 2000.);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-2000nit-15nit-P3lim_1.1", 
                            "Component of ACES Output Transforms for 2000 nit HDR D65 video",
                            ACES2065_1_to_CIE_XYZ_hdr_video_2000nits_p3lim_1_1_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_hdr_video_4000nits_rec2020lim_1_1_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_hdr_tonecurve_ops(ops, 4000.);

            ACES_OUTPUT::Generate_hdr_primary_clamp_ops(ops, REC2020::primaries);

            ACES_OUTPUT::Generate_nit_normalization_ops(ops, 4000.);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-4000nit-15nit-REC2020lim_1.1", 
                            "Component of ACES Output Transforms for 4000 nit HDR D65 video",
                            ACES2065_1_to_CIE_XYZ_hdr_video_4000nits_rec2020lim_1_1_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_hdr_video_4000nits_p3lim_1_1_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_hdr_tonecurve_ops(ops, 4000.);

            ACES_OUTPUT::Generate_hdr_primary_clamp_ops(ops, P3_D65::primaries);

            ACES_OUTPUT::Generate_nit_normalization_ops(ops, 4000.);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-VIDEO-4000nit-15nit-P3lim_1.1", 
                            "Component of ACES Output Transforms for 4000 nit HDR D65 video",
                            ACES2065_1_to_CIE_XYZ_hdr_video_4000nits_p3lim_1_1_Functor);
    }

    {
        auto ACES2065_1_to_CIE_XYZ_hdr_cinema_108nits_p3lim_1_1_Functor = [](OpRcPtrVec & ops)
        {
            ACES_OUTPUT::Generate_RRT_preamble_ops(ops);

            ACES_OUTPUT::Generate_hdr_tonecurve_ops(ops, 108.);

            ACES_OUTPUT::Generate_hdr_primary_clamp_ops(ops, P3_D65::primaries);

            ACES_OUTPUT::Generate_nit_normalization_ops(ops, 108.);
        };

        registry.addBuiltin("ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-CINEMA-108nit-7.2nit-P3lim_1.1", 
                            "Component of ACES Output Transforms for 108 nit HDR D65 cinema",
                            ACES2065_1_to_CIE_XYZ_hdr_cinema_108nits_p3lim_1_1_Functor);
    }
}

} // namespace ACES

} // namespace OCIO_NAMESPACE
