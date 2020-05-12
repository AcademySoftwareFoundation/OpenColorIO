// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "OpenEXR/half.h"
#include "ops/log/LogOp.h"
#include "ops/matrix/MatrixOp.h"
#include "ops/range/RangeOp.h"
#include "transforms/builtins/ACES.h"
#include "transforms/builtins/BuiltinTransformRegistry.h"
#include "transforms/builtins/OpHelpers.h"


namespace OCIO_NAMESPACE
{

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
}

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

}  // namespace ADX


namespace ACES
{

// Register all the ACES related built-in tranforms.
void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept
{
    {
        auto ACES_AP0_to_CIE_XYZ_D65_BFD_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix(ACES_AP0::primaries, CIE_XYZ_D65::primaries, ADAPTATION_BRADFORD);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("ACES-AP0_to_CIE-XYZ-D65_BFD", 
                            "Convert ACES AP0 primaries to CIE XYZ with a D65 white point with Bradford adaptation",
                            ACES_AP0_to_CIE_XYZ_D65_BFD_Functor);
    }
    {
        auto ACES_AP1_to_CIE_XYZ_D65_BFD_Functor = [](OpRcPtrVec & ops)
        {
            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix(ACES_AP1::primaries, CIE_XYZ_D65::primaries, ADAPTATION_BRADFORD);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("ACES-AP1_to_CIE-XYZ-D65_BFD", 
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

        registry.addBuiltin("ACES-AP1_to_LINEAR-REC709_BFD", 
                            "Convert ACES AP1 primaries to linear Rec.709 primaries with Bradford adaptation",
                            ACES_AP1_to_LINEAR_REC709_BFD_Functor);
    }
    {
        auto ACEScct_LOG_to_LIN_Functor = [](OpRcPtrVec & ops)
        {
            LogOpDataRcPtr log = ACEScct_to_LINEAR::log.clone();
            CreateLogOp(ops, log, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("ACEScct-LOG_to_LIN", 
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
            auto GenerateLutValues = [](double in) -> float
            {
                double out = 0.0f;

                if (in < ((9.72 - 15.0) / 17.52))
                {
                    out = (std::pow( 2., in * 17.52 - 9.72) - std::pow( 2., -16.)) * 2.0;
                }
                else if (in < (log2(HALF_MAX) + 9.72) / 17.52)
                {
                    out = std::pow( 2., in * 17.52 - 9.72);
                }
                else // (in >= (log2(HALF_MAX) + 9.72) / 17.52)
                {
                    out = HALF_MAX;
                }

                return float(out);
            };

            CreateLut(ops, 4096, GenerateLutValues);

            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix(ACES_AP1::primaries, ACES_AP0::primaries, ADAPTATION_NONE);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
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
}

} // namespace ACES

} // namespace OCIO_NAMESPACE
