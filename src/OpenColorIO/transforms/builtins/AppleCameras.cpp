// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>


#include <cmath>


#include "ops/matrix/MatrixOp.h"
#include "transforms/builtins/AppleCameras.h"
#include "transforms/builtins/BuiltinTransformRegistry.h"
#include "transforms/builtins/ColorMatrixHelpers.h"
#include "transforms/builtins/OpHelpers.h"


namespace OCIO_NAMESPACE
{

namespace APPLE_LOG
{
#if OCIO_LUT_AND_FILETRANSFORM_SUPPORT
void GenerateAppleLogToLinearOps(OpRcPtrVec & ops)
{
    auto GenerateLutValues = [](double in) -> float
    {
        constexpr double R_0   = -0.05641088;
        constexpr double R_t   = 0.01;
        constexpr double c     = 47.28711236;
        constexpr double beta  = 0.00964052;
        constexpr double gamma = 0.08550479;
        constexpr double delta = 0.69336945;
        const double P_t       = c * std::pow((R_t - R_0), 2.0);
        
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

}
#endif
} // namespace APPLE_LOG

namespace CAMERA
{

namespace APPLE
{

void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept
{
    {
        // FIXME: needs LUT-free implementation
        std::function<void(OpRcPtrVec& ops)> APPLE_LOG_to_ACES2065_1_Functor;
#if OCIO_LUT_AND_FILETRANSFORM_SUPPORT
        APPLE_LOG_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            APPLE_LOG::GenerateAppleLogToLinearOps(ops);
            
            MatrixOpData::MatrixArrayPtr matrix
            = build_conversion_matrix(REC2020::primaries, ACES_AP0::primaries, ADAPTATION_BRADFORD);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };
#endif
        
        registry.addBuiltin("APPLE_LOG_to_ACES2065-1",
                            "Convert Apple Log to ACES2065-1",
                            APPLE_LOG_to_ACES2065_1_Functor);
    }
    {
        // FIXME: needs LUT-free implementation
        std::function<void(OpRcPtrVec& ops)> APPLE_LOG_to_Linear_Functor;
#if OCIO_LUT_AND_FILETRANSFORM_SUPPORT
        APPLE_LOG_to_Linear_Functor = [](OpRcPtrVec & ops)
        {
            APPLE_LOG::GenerateAppleLogToLinearOps(ops);
        };
#endif
        
        registry.addBuiltin("CURVE - APPLE_LOG_to_LINEAR",
                            "Convert Apple Log to linear",
                            APPLE_LOG_to_Linear_Functor);
    }
}

} // namespace APPLE

} // namespace CAMERA

} // namespace OCIO_NAMESPACE
