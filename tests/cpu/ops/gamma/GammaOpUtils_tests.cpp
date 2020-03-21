// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gamma/GammaOpUtils.cpp"

#include "MathUtils.h"
#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(GammaOpUtils, compute_params_forward)
{
    const OCIO::GammaOpData::Params gParams = { 2.0f, 0.1f };
    OCIO::RendererParams rParams;

    ComputeParamsFwd(gParams, rParams);

    OCIO_CHECK_EQUAL(rParams.gamma,    2.0f);
    OCIO_CHECK_EQUAL(rParams.offset,   float(0.1 / (1. + 0.1)));
    OCIO_CHECK_EQUAL(rParams.breakPnt, float(0.1 / (2. - 1. )));
    OCIO_CHECK_EQUAL(rParams.scale,    float(1.  / (1. + 0.1)));

    OCIO_CHECK_ASSERT(OCIO::EqualWithAbsError(rParams.slope, 0.33057851f, 1e-7f));
}

OCIO_ADD_TEST(GammaOpUtils, compute_params_reverse)
{
    const OCIO::GammaOpData::Params gParams = { 2.0f, 0.1f };
    OCIO::RendererParams rParams;

    ComputeParamsRev(gParams, rParams);

    OCIO_CHECK_EQUAL(rParams.gamma,    0.5f );
    OCIO_CHECK_EQUAL(rParams.offset,   0.1f);
    OCIO_CHECK_EQUAL(rParams.scale,    1.0f + 0.1f);

    OCIO_CHECK_ASSERT(OCIO::EqualWithAbsError(rParams.breakPnt, 0.03305785f, 1e-7f));
    OCIO_CHECK_ASSERT(OCIO::EqualWithAbsError(rParams.slope,    3.02499986f, 1e-7f));
}

