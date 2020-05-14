// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/matrix/MatrixOp.h"
#include "ops/log/LogOp.h"
#include "transforms/builtins/ACES.h"
#include "transforms/builtins/BuiltinTransformRegistry.h"
#include "transforms/builtins/ColorMatrixHelpers.h"
#include "transforms/builtins/OpHelpers.h"
#include "transforms/builtins/PanasonicCameras.h"


namespace OCIO_NAMESPACE
{

namespace PANASONIC_VLOG_VGAMUT
{
static const Chromaticities red_xy(0.730,   0.280);
static const Chromaticities grn_xy(0.165,   0.840);
static const Chromaticities blu_xy(0.100,  -0.030);
static const Chromaticities wht_xy(0.3127,  0.3290);

const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);
}

namespace PANASONIC_VLOG_VGAMUT_to_LINEAR
{
static constexpr double cut1 = 0.01;
static constexpr double b    = 0.00873;
static constexpr double c    = 0.241514;
static constexpr double d    = 0.598206;

static constexpr double linSideSlope  = 1.;
static constexpr double linSideOffset = b;
static constexpr double logSideSlope  = c;
static constexpr double logSideOffset = d;
static constexpr double linSideBreak  = cut1;
static constexpr double base          = 10.;

static const LogOpData::Params 
    params { logSideSlope, logSideOffset, linSideSlope, linSideOffset, linSideBreak };

static const LogOpData log(base, params, params, params, TRANSFORM_DIR_INVERSE);
}


namespace CAMERA
{

namespace PANASONIC
{

void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept
{
    {
        auto PANASONIC_VLOG_VGAMUT_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            LogOpDataRcPtr log = PANASONIC_VLOG_VGAMUT_to_LINEAR::log.clone();
            CreateLogOp(ops, log, TRANSFORM_DIR_FORWARD);

            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix(PANASONIC_VLOG_VGAMUT::primaries, ACES_AP0::primaries, ADAPTATION_BRADFORD);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("PANASONIC_VLOG-VGAMUT_to_ACES2065-1",
                            "Convert Panasonic Varicam V-Log V-Gamut to ACES2065-1",
                            PANASONIC_VLOG_VGAMUT_to_ACES2065_1_Functor);
    }
}

} // namespace PANASONIC

} // namespace CAMERA

} // namespace OCIO_NAMESPACE
