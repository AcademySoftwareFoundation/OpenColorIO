// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/Lut3DTransform.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Lut3DTransform, basic)
{
    const OCIO::Lut3DTransformRcPtr lut = OCIO::Lut3DTransform::Create();

    OCIO_CHECK_EQUAL(lut->getGridSize(), 2);
    OCIO_CHECK_EQUAL(lut->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    lut->getValue(0, 0, 0, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 0.f);
    OCIO_CHECK_EQUAL(b, 0.f);
    lut->getValue(0, 1, 1, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 1.f);
    OCIO_CHECK_EQUAL(b, 1.f);
    lut->getValue(1, 0, 0, r, g, b);
    OCIO_CHECK_EQUAL(r, 1.f);
    OCIO_CHECK_EQUAL(g, 0.f);
    OCIO_CHECK_EQUAL(b, 0.f);

    lut->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(lut->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    lut->setGridSize(3);
    OCIO_CHECK_EQUAL(lut->getGridSize(), 3);
    lut->getValue(0, 0, 0, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 0.f);
    OCIO_CHECK_EQUAL(b, 0.f);
    lut->getValue(0, 1, 1, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 0.5f);
    OCIO_CHECK_EQUAL(b, 0.5f);
    lut->getValue(2, 0, 2, r, g, b);
    OCIO_CHECK_EQUAL(r, 1.f);
    OCIO_CHECK_EQUAL(g, 0.f);
    OCIO_CHECK_EQUAL(b, 1.f);

    lut->getValue(0, 1, 2, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 0.5f);
    OCIO_CHECK_EQUAL(b, 1.f);

    r = 0.1f;
    g = 0.52f;
    b = 0.93f;
    lut->setValue(0, 1, 2, r, g, b);

    r = 0.f;
    g = 0.f;
    b = 0.f;
    lut->getValue(0, 1, 2, r, g, b);

    OCIO_CHECK_EQUAL(r, 0.1f);
    OCIO_CHECK_EQUAL(g, 0.52f);
    OCIO_CHECK_EQUAL(b, 0.93f);

    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);

    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    // File out bit-depth does not affect values.
    r = 0.f;
    g = 0.f;
    b = 0.f;
    lut->getValue(0, 1, 2, r, g, b);

    OCIO_CHECK_EQUAL(r, 0.1f);
    OCIO_CHECK_EQUAL(g, 0.52f);
    OCIO_CHECK_EQUAL(b, 0.93f);

    OCIO_CHECK_THROW_WHAT(lut->setValue(3, 1, 1, 0.f, 0.f, 0.f), OCIO::Exception,
                          "should be less than the grid size");
    OCIO_CHECK_THROW_WHAT(lut->getValue(0, 0, 4, r, g, b), OCIO::Exception,
                          "should be less than the grid size");

    OCIO_CHECK_THROW_WHAT(lut->setGridSize(200), OCIO::Exception,
                          "must not be greater than '129'");

    OCIO_CHECK_NO_THROW(lut->validate());

    lut->setValue(0, 0, 0, -0.2f, -0.1f, -0.3f);
    lut->setValue(2, 2, 2, 1.2f, 1.3f, 1.8f);

    std::ostringstream oss;
    oss << *lut;
    OCIO_CHECK_EQUAL(oss.str(), "<Lut3DTransform direction=inverse, fileoutdepth=8ui,"
        " interpolation=default, gridSize=3, minrgb=[-0.2 -0.1 -0.3], maxrgb=[1.2 1.3 1.8]>");
}

OCIO_ADD_TEST(Lut3DTransform, create_with_parameters)
{
    const auto lut = OCIO::Lut3DTransform::Create(8);

    OCIO_CHECK_EQUAL(lut->getGridSize(), 8);
    OCIO_CHECK_EQUAL(lut->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(lut->getInterpolation(), OCIO::INTERP_DEFAULT);

    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    lut->getValue(7, 7, 7, r, g, b);
    OCIO_CHECK_EQUAL(r, 1.0f);
    OCIO_CHECK_EQUAL(g, 1.0f);
    OCIO_CHECK_EQUAL(b, 1.0f);
}

