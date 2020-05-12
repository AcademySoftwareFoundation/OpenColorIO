// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/log/LogOp.h"
#include "ops/matrix/MatrixOp.h"
#include "transforms/builtins/ACES.h"
#include "transforms/builtins/BuiltinTransformRegistry.h"
#include "transforms/builtins/ColorMatrixHelpers.h"
#include "transforms/builtins/RedCameras.h"


namespace OCIO_NAMESPACE
{

namespace RED_WIDE_GAMUT_RGB
{
static const Chromaticities red_xy(0.780308,  0.304253);
static const Chromaticities grn_xy(0.121595,  1.493994);
static const Chromaticities blu_xy(0.095612, -0.084589);
static const Chromaticities wht_xy(0.3127,    0.3290);

const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);
}

namespace RED_REDLOGFILM_RWG_to_LINEAR
{
static constexpr double refWhite    = 685. / 1023.;
static constexpr double refBlack    = 95. / 1023.;
static constexpr double range       = 0.002 * 1023.;
static constexpr double gamma       = 0.6;
static constexpr double highlight   = 1.;
static constexpr double shadow      = 0.;
static constexpr double multiFactor = range / gamma;
static const     double gain        = (highlight - shadow)
                                        / (1. - std::pow(10., multiFactor * (refBlack - refWhite)));
static const     double offset      = gain - (highlight - shadow);

static constexpr double logSideSlope  = 1. / multiFactor;
static constexpr double logSideOffset = refWhite;
static const     double linSideSlope  = 1. / gain;
static const     double linSideOffset = (offset - shadow) / gain;
static constexpr double base          = 10.;

static const LogOpData::Params 
    params { logSideSlope, logSideOffset, linSideSlope, linSideOffset };

static const LogOpData log(base, params, params, params, TRANSFORM_DIR_INVERSE);
}

namespace RED_LOG3G10_RWG_to_LINEAR
{
static constexpr double linSideSlope  = 155.975327;
static constexpr double linSideOffset = 0.01 * linSideSlope + 1.0;
static constexpr double logSideSlope  = 0.224282;
static constexpr double logSideOffset = 0.0;
static constexpr double linSideBreak  = -0.01;
static constexpr double base          = 10.;

static const LogOpData::Params 
    params { logSideSlope, logSideOffset, linSideSlope, linSideOffset, linSideBreak };

static const LogOpData log(base, params, params, params, TRANSFORM_DIR_INVERSE);
}


namespace CAMERA
{

namespace RED
{

void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept
{
    {
        auto RED_REDLOGFILM_RWG_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            LogOpDataRcPtr log = RED_REDLOGFILM_RWG_to_LINEAR::log.clone();
            CreateLogOp(ops, log, TRANSFORM_DIR_FORWARD);

            MatrixOpData::MatrixArrayPtr
                rwg = build_conversion_matrix(RED_WIDE_GAMUT_RGB::primaries,
                                              ACES_AP0::primaries,
                                              ADAPTATION_BRADFORD); 
            CreateMatrixOp(ops, rwg, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("RED_REDLOGFILM-RWG_to_ACES2065-1",
                            "Convert RED LogFilm RED Wide Gamut to ACES2065-1",
                            RED_REDLOGFILM_RWG_to_ACES2065_1_Functor);
    }
    {
        auto RED_LOG3G10_RWG_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            LogOpDataRcPtr log = RED_LOG3G10_RWG_to_LINEAR::log.clone();
            CreateLogOp(ops, log, TRANSFORM_DIR_FORWARD);

            MatrixOpData::MatrixArrayPtr
                rwg = build_conversion_matrix(RED_WIDE_GAMUT_RGB::primaries,
                                              ACES_AP0::primaries,
                                              ADAPTATION_BRADFORD); 
            CreateMatrixOp(ops, rwg, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("RED_LOG3G10-RWG_to_ACES2065-1", 
                            "Convert RED Log3G10 RED Wide Gamut to ACES2065-1",
                            RED_LOG3G10_RWG_to_ACES2065_1_Functor);
    }
}

} // namespace RED

} // namespace CAMERA

} // namespace OCIO_NAMESPACE
