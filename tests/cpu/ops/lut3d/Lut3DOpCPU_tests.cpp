// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <limits>

#include "ops/lut3d/Lut3DOpCPU.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


void Lut3DRendererNaNTest(OCIO::Interpolation interpol)
{
    OCIO::Lut3DOpDataRcPtr lut = std::make_shared<OCIO::Lut3DOpData>(interpol, 4);

    float * values = &lut->getArray().getValues()[0];
    // Change LUT so that it is not identity.
    values[65] += 0.001f;

    OCIO::ConstLut3DOpDataRcPtr lutConst = lut;
    OCIO::ConstOpCPURcPtr renderer = OCIO::GetLut3DRenderer(lutConst);

    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float pixels[16] = { qnan, qnan, qnan, 0.5f,
                         0.5f, 0.3f, 0.2f, qnan,
                          inf,  inf,  inf,  inf,
                         -inf, -inf, -inf, -inf };

    renderer->apply(pixels, pixels, 4);

    OCIO_CHECK_CLOSE(pixels[0], values[0], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[1], values[1], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[2], values[2], 1e-7f);
    OCIO_CHECK_ASSERT(OCIO::IsNan(pixels[7]));
    OCIO_CHECK_CLOSE(pixels[8], 1.0f, 1e-7f);
    OCIO_CHECK_CLOSE(pixels[9], 1.0f, 1e-7f);
    OCIO_CHECK_CLOSE(pixels[10], 1.0f, 1e-7f);
    OCIO_CHECK_EQUAL(pixels[11], inf);
    OCIO_CHECK_CLOSE(pixels[12], 0.0f, 1e-7f);
    OCIO_CHECK_CLOSE(pixels[13], 0.0f, 1e-7f);
    OCIO_CHECK_CLOSE(pixels[14], 0.0f, 1e-7f);
    OCIO_CHECK_EQUAL(pixels[15], -inf);
}

OCIO_ADD_TEST(Lut3DRenderer, nan_linear_test)
{
    Lut3DRendererNaNTest(OCIO::INTERP_LINEAR);
}

OCIO_ADD_TEST(Lut3DRenderer, nan_tetra_test)
{
    Lut3DRendererNaNTest(OCIO::INTERP_TETRAHEDRAL);
}

