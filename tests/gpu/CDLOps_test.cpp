/*
Copyright (c) 2018 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdio.h>
#include <sstream>
#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "GPUUnitTest.h"
#include "GPUHelpers.h"


namespace OCIO = OCIO_NAMESPACE;

OCIO_NAMESPACE_USING


// TODO: Add ctf file unit tests when the CLF reader is in.


namespace CDL_Data_1
{
const float slope[3]  = { 1.35f,  1.10f, 0.71f };
const float offset[3] = { 0.05f, -0.23f, 0.11f };
const float power[3]  = { 0.93f,  0.81f, 1.27f };
};

// Use the legacy shader description with the CDL from OCIO v1 implementation
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v1_legacy_shader)
{
    CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
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
    CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
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
    CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
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
    // TODO: How to explain the threshold difference compare to the previous test
    //       (i.e. CDL v1 versus v2)
    test.setErrorThreshold(1e-5f);
}

// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op (with the fwd clamp style) and an inverse direction)
OCIO_ADD_GPU_TEST(CDLOp, clamp_inv_v2)
{
    CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
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
    test.setErrorThreshold(1e-5f);
}

namespace CDL_Data_2
{
const float slope[3]  = { 1.15f, 1.10f, 0.90f };
const float offset[3] = { 0.05f, 0.02f, 0.07f };
const float power[3]  = { 1.20f, 0.95f, 1.13f };
};


// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op (with the fwd clamp style) and a forward direction)
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v2_Data_2)
{
    CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
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
const float slope[3]  = {  3.405f,  1.0f,    1.0f   };
const float offset[3] = { -0.178f, -0.178f, -0.178f };
const float power[3]  = {  1.095f,  1.095f,  1.095f };
};


// Use the generic shader description with the CDL from OCIO v2 implementation
// (i.e. use the CDL Op (with the fwd clamp style) and a forward direction)
OCIO_ADD_GPU_TEST(CDLOp, clamp_fwd_v2_Data_3)
{
    CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
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

