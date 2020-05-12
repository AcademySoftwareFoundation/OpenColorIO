// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "ops/log/LogOp.h"
#include "ops/matrix/MatrixOp.h"
#include "transforms/builtins/ACES.h"
#include "transforms/builtins/ArriCameras.h"
#include "transforms/builtins/BuiltinTransformRegistry.h"
#include "transforms/builtins/ColorMatrixHelpers.h"


namespace OCIO_NAMESPACE
{

namespace ARRI_ALEXA_WIDE_GAMUT
{
static const Chromaticities red_xy(0.68400,  0.31300);
static const Chromaticities grn_xy(0.22100,  0.84800);
static const Chromaticities blu_xy(0.08610, -0.10200);
static const Chromaticities wht_xy(0.31270,  0.32900);

const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);
}

namespace ARRI_ALEXA_LOGC_EI800_to_LINEAR
{
static constexpr double linSideSlope  = 1. / (0.18 * 0.005 * (800. / 400.) / 0.01);
static constexpr double linSideOffset = 0.0522722750;
static constexpr double logSideSlope  = 0.2471896383;
static constexpr double logSideOffset = 0.3855369987;
static constexpr double linSideBreak  = ((1. / 9.) - linSideOffset) / linSideSlope;
static constexpr double base          = 10.;

static const LogOpData::Params 
    params { logSideSlope, logSideOffset, linSideSlope, linSideOffset, linSideBreak };

static const LogOpData log(base, params, params, params, TRANSFORM_DIR_INVERSE);
}


namespace CAMERA
{

namespace ARRI
{

void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept
{
    {
        auto ARRI_ALEXA_LOGC_EI800_AWG_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            LogOpDataRcPtr log = ARRI_ALEXA_LOGC_EI800_to_LINEAR::log.clone();
            CreateLogOp(ops, log, TRANSFORM_DIR_FORWARD);

            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix(ARRI_ALEXA_WIDE_GAMUT::primaries, ACES_AP0::primaries, ADAPTATION_CAT02);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("ARRI_ALEXA-LOGC-EI800-AWG_to_ACES2065-1",
                            "Convert ARRI ALEXA LogC (EI800) ALEXA Wide Gamut to ACES2065-1",
                            ARRI_ALEXA_LOGC_EI800_AWG_to_ACES2065_1_Functor);
    }
}

} // namespace ARRI

} // namespace CAMERA

} // namespace OCIO_NAMESPACE
