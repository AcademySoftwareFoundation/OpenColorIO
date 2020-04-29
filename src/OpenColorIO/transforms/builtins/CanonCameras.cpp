// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/matrix/MatrixOp.h"
#include "transforms/builtins/ACES.h"
#include "transforms/builtins/BuiltinTransformRegistry.h"
#include "transforms/builtins/CanonCameras.h"
#include "transforms/builtins/ColorMatrixHelpers.h"
#include "transforms/builtins/OpHelpers.h"


namespace OCIO_NAMESPACE
{

namespace CANON_CGAMUT
{
static const Chromaticities red_xy(0.7400,  0.2700);
static const Chromaticities grn_xy(0.1700,  1.1400);
static const Chromaticities blu_xy(0.0800, -0.1000);
static const Chromaticities wht_xy(0.3127,  0.3290);

const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);
}


namespace CAMERA
{

namespace CANON
{

void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept
{
    {
        auto CANON_CLOG2_CGAMUT_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            auto GenerateLutValues = [](double in) -> float
            {
                double out = 0.;

                if (in < 0.092864125)
                {
                    out = -(std::pow(10, (0.092864125 - in) / 0.24136077) - 1) / 87.099375;
                }
                else
                {
                    out = (std::pow(10, (in - 0.092864125) / 0.24136077) - 1) / 87.099375;
                }

                return float(out * 0.9);
            };

            CreateLut(ops, 4096, GenerateLutValues);

            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix(CANON_CGAMUT::primaries, ACES_AP0::primaries, ADAPTATION_CAT02);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("CANON_CLOG2-CGAMUT_to_ACES2065-1",
                            "Convert Canon Log 2 Cinema Gamut to ACES2065-1",
                            CANON_CLOG2_CGAMUT_to_ACES2065_1_Functor);
    }
    {
        auto CANON_CLOG3_CGAMUT_to_ACES2065_1_Functor = [](OpRcPtrVec & ops)
        {
            auto GenerateLutValues = [](double in) -> float
            {
                double out = 0.;

                if (in < 0.097465473)
                {
                    out = -(std::pow(10, (0.12783901 - in) / 0.36726845) - 1) / 14.98325;
                }
                else if (in <= 0.15277891)
                {
                    out = (in - 0.12512219) / 1.9754798;
                }
                else
                {
                    out = (std::pow(10, (in - 0.12240537) / 0.36726845) - 1) / 14.98325;
                }

                return float(out * 0.9);
            };

            CreateLut(ops, 4096, GenerateLutValues);

            MatrixOpData::MatrixArrayPtr matrix
                = build_conversion_matrix(CANON_CGAMUT::primaries, ACES_AP0::primaries, ADAPTATION_CAT02);
            CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
        };

        registry.addBuiltin("CANON_CLOG3-CGAMUT_to_ACES2065-1",
                            "Convert Canon Log 3 Cinema Gamut to ACES2065-1",
                            CANON_CLOG3_CGAMUT_to_ACES2065_1_Functor);
    }
}

} // namespace CANON

} // namespace CAMERA

} // namespace OCIO_NAMESPACE
