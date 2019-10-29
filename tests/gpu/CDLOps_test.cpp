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
const double slope[3]  = { 1.35,  1.10, 0.71 };
const double offset[3] = { 0.05, -0.23, 0.11 };
const double power[3]  = { 0.93,  0.81, 1.27 };
};

// Use the legacy shader description with the CDL from OCIO v1 implementation
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v1_legacy_shader)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    OCIO::GpuShaderDescRcPtr shaderDesc
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(32);

    test.setContext(cdl, shaderDesc);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-6f);
    // TODO: Would like to be able to remove the setTestNaN(false)
    // from all of these tests.
    test.setTestNaN(false);
}

// Use the generic shader description with the CDL from OCIO v1 implementation
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v1)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(cdl, shaderDesc);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-6f);
    test.setTestNaN(false);
}

// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op (with the fwd clamp style) and a forward direction)
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();
    config->setMajorVersion(2);

    test.setContext(config, cdl, shaderDesc);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-5f);
}

// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op (with the fwd clamp style) and an inverse direction)
OCIO_ADD_GPU_TEST(CDLOp, clamp_inv_v2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    cdl->setSlope(CDL_Data_1::slope);
    cdl->setOffset(CDL_Data_1::offset);
    cdl->setPower(CDL_Data_1::power);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();
    config->setMajorVersion(2);

    test.setContext(config, cdl, shaderDesc);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-4f);
}

namespace CDL_Data_2
{
const double slope[3]  = { 1.15, 1.10, 0.90 };
const double offset[3] = { 0.05, 0.02, 0.07 };
const double power[3]  = { 1.20, 0.95, 1.13 };
};


// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op (with the fwd clamp style) and a forward direction)
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v2_Data_2)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_2::slope);
    cdl->setOffset(CDL_Data_2::offset);
    cdl->setPower(CDL_Data_2::power);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();
    config->setMajorVersion(2);

    test.setContext(config, cdl, shaderDesc);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(2e-5f);
}


namespace CDL_Data_3
{
const double slope[3]  = {  3.405,  1.0,    1.0   };
const double offset[3] = { -0.178, -0.178, -0.178 };
const double power[3]  = {  1.095,  1.095,  1.095 };
};


// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op (with the fwd clamp style) and a forward direction)
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v2_Data_3)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    cdl->setSlope(CDL_Data_3::slope);
    cdl->setOffset(CDL_Data_3::offset);
    cdl->setPower(CDL_Data_3::power);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    OCIO::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();
    config->setMajorVersion(2);

    test.setContext(config, cdl, shaderDesc);

    test.setTestWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-5f);
}
