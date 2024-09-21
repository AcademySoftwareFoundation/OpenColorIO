// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/matrix/MatrixOp.h"
#include "ops/fixedfunction/FixedFunctionOp.h"
#include "ops/range/RangeOp.h"
#include "transforms/builtins/AppleCameras.h"
#include "transforms/builtins/BuiltinTransformRegistry.h"
#include "transforms/builtins/ColorMatrixHelpers.h"
#include "transforms/builtins/OpHelpers.h"


// This is a preparation for OCIO-lite where LUT support may be turned off.
#ifndef OCIO_LUT_SUPPORT
#   define OCIO_LUT_SUPPORT 1
#endif 


namespace OCIO_NAMESPACE
{

namespace APPLE_LOG
{

void GenerateAppleLogToLinearOps(OpRcPtrVec & ops)
{
    static constexpr double R_0   = -0.05641088;
    static constexpr double R_t   = 0.01;
    static constexpr double c     = 47.28711236;
    static constexpr double beta  = 0.00964052;
    static constexpr double gamma = 0.08550479;
    static constexpr double delta = 0.69336945;

#if OCIO_LUT_SUPPORT
    static const double P_t = c * std::pow((R_t - R_0), 2.0);
    auto GenerateLutValues = [](double in) -> float
    {
        if (in >= P_t)
        {
            return float(std::pow(2.0, (in - delta) / gamma) - beta);
        }
        else if (in < P_t && in >= 0.0)
        {
            return float(std::sqrt(in / c) + R_0);
        }
        else
        {
            return float(R_0);
        }
    };

    CreateHalfLut(ops, GenerateLutValues);
#else
    FixedFunctionOpData::Params gamma_log_params
    {
        R_0,           // mirror point
        R_t,           // break point

        // Gamma segment.
        2.0,            // gamma power
        c,              // post-power scale
        -R_0,           // pre-power offset

        // Log segment.
        2.0,            // log base
        gamma,          // log-side slope
        delta,          // log-side offset
        1.0,            // lin-side slope
        beta,           // lin-side offset
    };

    auto range_data = std::make_shared<RangeOpData>(
        0.,
        RangeOpData::EmptyValue(), // don't clamp high end
        0.,
        RangeOpData::EmptyValue());

    CreateRangeOp(ops, range_data, TransformDirection::TRANSFORM_DIR_FORWARD);
    CreateFixedFunctionOp(ops, FixedFunctionOpData::GAMMA_LOG_TO_LIN, gamma_log_params);
#endif // OCIO_LUT_SUPPORT
}

} // namespace APPLE_LOG

namespace CAMERA
{

namespace APPLE
{

void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept
{
    {
        auto APPLE_LOG_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            APPLE_LOG::GenerateAppleLogToLinearOps(ops);
            
            MatrixOpData::MatrixArrayPtr matrix
            = build_conversion_matrix(REC2020::primaries, ACES_AP0::primaries, ADAPTATION_BRADFORD);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };
        
        registry.addBuiltin("APPLE_LOG_to_ACES2065-1",
                            "Convert Apple Log to ACES2065-1",
                            APPLE_LOG_to_ACES2065_1_Functor);
    }
    {
        auto APPLE_LOG_to_Linear_Functor = [](OpRcPtrVec & ops)
        {
            APPLE_LOG::GenerateAppleLogToLinearOps(ops);
        };
        
        registry.addBuiltin("CURVE - APPLE_LOG_to_LINEAR",
                            "Convert Apple Log to linear",
                            APPLE_LOG_to_Linear_Functor);
    }
}

} // namespace APPLE

} // namespace CAMERA

} // namespace OCIO_NAMESPACE
