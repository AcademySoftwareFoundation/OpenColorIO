// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <stdio.h>
#include <sstream>
#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "GPUUnitTest.h"
#include "GPUHelpers.h"


namespace OCIO = OCIO_NAMESPACE;


// TODO: Add ctf file unit tests when the CLF reader is in.


namespace CDL_Data_1
{
constexpr double slope[3]  = { 1.35,  1.10, 0.71 };
constexpr double offset[3] = { 0.05, -0.23, 0.11 };
constexpr double power[3]  = { 0.93,  0.81, 1.27 };
};

// Use the legacy shader description with the CDL from OCIO v1 implementation.
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v1_legacy_shader)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();
    config->setMajorVersion(1);

    test.setProcessor(config, cdl);

    test.setLegacyShader(true);
    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-6f);
    // TODO: Would like to be able to remove the setTestNaN(false)
    // from all of these tests.
    test.setTestNaN(false);
}

// Use the generic shader description with the CDL from OCIO v1 implementation.
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v1)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();
    config->setMajorVersion(1);
    test.setProcessor(config, cdl);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-6f);
    test.setTestNaN(false);
}

// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op with the ASC/clamping style and a forward direction).
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_ASC);
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();

    test.setProcessor(config, cdl);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-5f);
}

OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_no_clamp_v2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_NO_CLAMP);
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();

    test.setProcessor(config, cdl);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(5e-5f);
    test.setTestNaN(false);
    test.setTestInfinity(false);
}

// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op with the ASC/clamping style and a inverse direction).
OCIO_ADD_GPU_TEST(CDLOp, clamp_inv_v2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_ASC);
    cdl->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();

    test.setProcessor(config, cdl);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-4f);
}

OCIO_ADD_GPU_TEST(CDLOp, clamp_inv_no_clamp_v2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_NO_CLAMP);
    cdl->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();

    test.setProcessor(config, cdl);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-4f);
}

namespace CDL_Data_2
{
constexpr double slope[3]  = { 1.15,  1.10, 0.90 };
constexpr double offset[3] = { 0.05, -0.02, 0.07 };
constexpr double power[3]  = { 1.20,  0.95, 1.13 };
constexpr double saturation = 0.9;
};

// Use the legacy shader description with the CDL from OCIO v1 implementation.
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v1_legacy_shader_Data_2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_2::slope);
    cdl->setOffset(CDL_Data_2::offset);
    cdl->setPower(CDL_Data_2::power);
    cdl->setSat(CDL_Data_2::saturation);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();
    config->setMajorVersion(1);
    test.setProcessor(config, cdl);

    test.setLegacyShader(true);
    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-6f);
    test.setTestNaN(false);
}

// Use the generic shader description with the CDL from OCIO v1 implementation.
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v1_Data_2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_2::slope);
    cdl->setOffset(CDL_Data_2::offset);
    cdl->setPower(CDL_Data_2::power);
    cdl->setSat(CDL_Data_2::saturation);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();
    config->setMajorVersion(1);
    test.setProcessor(config, cdl);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-6f);
    test.setTestNaN(false);
}

// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op with the ASC/clamping style and a forward direction).
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v2_Data_2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_ASC);
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_2::slope);
    cdl->setOffset(CDL_Data_2::offset);
    cdl->setPower(CDL_Data_2::power);
    cdl->setSat(CDL_Data_2::saturation);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();

    test.setProcessor(config, cdl);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(2e-5f);
}

OCIO_ADD_GPU_TEST(CDLOp, clamp_inv_v2_Data_2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_ASC);
    cdl->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    cdl->setSlope(CDL_Data_2::slope);
    cdl->setOffset(CDL_Data_2::offset);
    cdl->setPower(CDL_Data_2::power);
    cdl->setSat(CDL_Data_2::saturation);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();

    test.setProcessor(config, cdl);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(2e-5f);
}

OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_no_clamp_v2_Data_2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_NO_CLAMP);
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_2::slope);
    cdl->setOffset(CDL_Data_2::offset);
    cdl->setPower(CDL_Data_2::power);
    cdl->setSat(CDL_Data_2::saturation);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();

    test.setProcessor(config, cdl);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(5e-5f);
    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(CDLOp, clamp_inv_no_clamp_v2_Data_2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_NO_CLAMP);
    cdl->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    cdl->setSlope(CDL_Data_2::slope);
    cdl->setOffset(CDL_Data_2::offset);
    cdl->setPower(CDL_Data_2::power);
    cdl->setSat(CDL_Data_2::saturation);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();

    test.setProcessor(config, cdl);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(5e-5f);
    test.setTestNaN(false);
    test.setTestInfinity(false);
}

namespace CDL_Data_3
{
constexpr double slope[3]  = {  3.405,  1.0,    1.0   };
constexpr double offset[3] = { -0.178, -0.178, -0.178 };
constexpr double power[3]  = {  1.095,  1.095,  1.0 };
constexpr double saturation = 1.2;
};


// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op with the ASC/clamping style and a forward direction).
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v2_Data_3)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_ASC);
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_3::slope);
    cdl->setOffset(CDL_Data_3::offset);
    cdl->setPower(CDL_Data_3::power);
    cdl->setSat(CDL_Data_3::saturation);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();

    test.setProcessor(config, cdl);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(5e-5f);
}

OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_no_clamp_v2_Data_3)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_NO_CLAMP);
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_3::slope);
    cdl->setOffset(CDL_Data_3::offset);
    cdl->setPower(CDL_Data_3::power);
    cdl->setSat(CDL_Data_3::saturation);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();

    test.setProcessor(config, cdl);

    test.setTestWideRange(false);
    test.setRelativeComparison(false);
    test.setErrorThreshold(5e-5f);
    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(CDLOp, clamp_inv_no_clamp_v2_Data_3)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_NO_CLAMP);
    cdl->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    cdl->setSlope(CDL_Data_3::slope);
    cdl->setOffset(CDL_Data_3::offset);
    cdl->setPower(CDL_Data_3::power);
    cdl->setSat(CDL_Data_3::saturation);

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();

    test.setProcessor(config, cdl);

    test.setTestWideRange(false);
    test.setRelativeComparison(false);
    test.setErrorThreshold(5e-5f);
    test.setTestNaN(false);
    test.setTestInfinity(false);
}
