// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


// NOTE:
// The file is a copy and paste from the corresponding GPU unitest i.e. []/tests/gpu/CDLOp_test.cpp.
// Keep the two files in sync. to increase the test coverage.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


namespace CDL_Data_1
{
constexpr double slope[3]  = { 1.35,  1.10, 0.71 };
constexpr double offset[3] = { 0.05, -0.23, 0.11 };
constexpr double power[3]  = { 0.93,  0.81, 1.27 };
};

// Use the generic shader description with the CDL from OCIO v1 implementation.
OCIO_OSL_TEST(CDLOp, clamp_fwd_v1)
{
    auto cdl = OCIO::CDLTransform::Create();
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    m_data->m_transform = cdl;
    
    m_data->m_config->setMajorVersion(1);

    m_data->m_threshold = 1e-5f; // Slight difference with the GLSL unit test i.e. 1e-6f
}

// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op with the ASC/clamping style and a forward direction).
OCIO_OSL_TEST(CDLOp, clamp_fwd_v2)
{
    auto cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_ASC);
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    m_data->m_transform = cdl;

    m_data->m_threshold = 1e-5f;
}

OCIO_OSL_TEST(CDLOp, clamp_fwd_no_clamp_v2)
{
    auto cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_NO_CLAMP);
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    m_data->m_transform = cdl;

    m_data->m_threshold = 5e-5f;
}

// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op with the ASC/clamping style and a inverse direction).
OCIO_OSL_TEST(CDLOp, clamp_inv_v2)
{
    auto cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_ASC);
    cdl->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    m_data->m_transform = cdl;

    m_data->m_threshold = 1e-4f;
}

OCIO_OSL_TEST(CDLOp, clamp_inv_no_clamp_v2)
{
    auto cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_NO_CLAMP);
    cdl->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    m_data->m_transform = cdl;

    m_data->m_threshold = 1e-4f;
}

namespace CDL_Data_2
{
constexpr double slope[3]  = { 1.15,  1.10, 0.90 };
constexpr double offset[3] = { 0.05, -0.02, 0.07 };
constexpr double power[3]  = { 1.20,  0.95, 1.13 };
constexpr double saturation = 0.9;
};

// Use the generic shader description with the CDL from OCIO v1 implementation.
OCIO_OSL_TEST(CDLOp, clamp_fwd_v1_Data_2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_2::slope);
    cdl->setOffset(CDL_Data_2::offset);
    cdl->setPower(CDL_Data_2::power);
    cdl->setSat(CDL_Data_2::saturation);

    m_data->m_transform = cdl;

    m_data->m_config->setMajorVersion(1);

    m_data->m_threshold = 1e-5f; // Slight difference with the GLSL unit test i.e. 1e-6f
}

// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op with the ASC/clamping style and a forward direction).
OCIO_OSL_TEST(CDLOp, clamp_fwd_v2_Data_2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_ASC);
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_2::slope);
    cdl->setOffset(CDL_Data_2::offset);
    cdl->setPower(CDL_Data_2::power);
    cdl->setSat(CDL_Data_2::saturation);

    m_data->m_transform = cdl;

    m_data->m_threshold = 2e-5f;
}

OCIO_OSL_TEST(CDLOp, clamp_inv_v2_Data_2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_ASC);
    cdl->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    cdl->setSlope(CDL_Data_2::slope);
    cdl->setOffset(CDL_Data_2::offset);
    cdl->setPower(CDL_Data_2::power);
    cdl->setSat(CDL_Data_2::saturation);

    m_data->m_transform = cdl;

    m_data->m_threshold = 2e-5f;
}

OCIO_OSL_TEST(CDLOp, clamp_fwd_no_clamp_v2_Data_2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_NO_CLAMP);
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_2::slope);
    cdl->setOffset(CDL_Data_2::offset);
    cdl->setPower(CDL_Data_2::power);
    cdl->setSat(CDL_Data_2::saturation);

    m_data->m_transform = cdl;

    m_data->m_threshold = 5e-5f;
}

OCIO_OSL_TEST(CDLOp, clamp_inv_no_clamp_v2_Data_2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_NO_CLAMP);
    cdl->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    cdl->setSlope(CDL_Data_2::slope);
    cdl->setOffset(CDL_Data_2::offset);
    cdl->setPower(CDL_Data_2::power);
    cdl->setSat(CDL_Data_2::saturation);

    m_data->m_transform = cdl;

    m_data->m_threshold = 5e-5f;
}

namespace CDL_Data_3
{
constexpr double slope[3]  = {  3.405,  1.0,    1.0   };
constexpr double offset[3] = { -0.178, -0.178, -0.178 };
constexpr double power[3]  = {  1.095,  1.095,  1.0   };
constexpr double saturation = 1.2;
};


// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op with the ASC/clamping style and a forward direction).
OCIO_OSL_TEST(CDLOp, clamp_fwd_v2_Data_3)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_ASC);
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_3::slope);
    cdl->setOffset(CDL_Data_3::offset);
    cdl->setPower(CDL_Data_3::power);
    cdl->setSat(CDL_Data_3::saturation);

    m_data->m_transform = cdl;

    m_data->m_threshold = 5e-5f;
}

OCIO_OSL_TEST(CDLOp, clamp_fwd_no_clamp_v2_Data_3)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_NO_CLAMP);
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_3::slope);
    cdl->setOffset(CDL_Data_3::offset);
    cdl->setPower(CDL_Data_3::power);
    cdl->setSat(CDL_Data_3::saturation);

    m_data->m_transform = cdl;

    m_data->m_threshold = 5e-5f;
}

OCIO_OSL_TEST(CDLOp, clamp_inv_no_clamp_v2_Data_3)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_NO_CLAMP);
    cdl->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    cdl->setSlope(CDL_Data_3::slope);
    cdl->setOffset(CDL_Data_3::offset);
    cdl->setPower(CDL_Data_3::power);
    cdl->setSat(CDL_Data_3::saturation);

    m_data->m_transform = cdl;

    m_data->m_threshold = 5e-5f;
}
