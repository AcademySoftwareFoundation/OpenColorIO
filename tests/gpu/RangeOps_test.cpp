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

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO = OCIO_NAMESPACE;
#include "GPUUnitTest.h"
#include "GPUHelpers.h"

#include <stdio.h>
#include <sstream>
#include <string>

OCIO_NAMESPACE_USING

const float g_epsilon = 1e-6f;

OCIO_ADD_GPU_TEST(RangeOp, identity)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(range->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, scale_with_low_and_high_clippings)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.1f);
    range->setMaxInValue(1.1f);
    range->setMinOutValue(0.5f);
    range->setMaxOutValue(1.5f);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(range->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, scale_with_low_clipping)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.1f);
    range->setMinOutValue(0.5f);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(range->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, scale_with_high_clipping)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMaxInValue(0.9f);
    range->setMaxOutValue(1.05f);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(range->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, scale_with_low_and_high_clippings_2)
{
    // The similar test above is only an offset, this test is a scale & offset.
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.1f);
    range->setMaxInValue(1.1f);
    range->setMinOutValue(-0.5f);
    range->setMaxOutValue(1.5f);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(range->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, arbitrary_1)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue (0.4000202f);
    range->setMaxInValue (0.6000502f);
    range->setMinOutValue(0.4000601f);
    range->setMaxOutValue(0.6000801f);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(range->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, arbitrary_1_no_clamp)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setStyle(OCIO::RANGE_NO_CLAMP);
    range->setMinInValue (0.4000202f);
    range->setMaxInValue (0.6000502f);
    range->setMinOutValue(0.4000601f);
    range->setMaxOutValue(0.6000801f);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(range->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, arbitrary_2)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(-0.010201f);
    range->setMaxInValue (0.601102f);
    range->setMinOutValue(0.209803f);
    range->setMaxOutValue(1.600208f);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(range->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, arbitrary_2_no_clamp)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setStyle(OCIO::RANGE_NO_CLAMP);
    range->setMinInValue(-0.010201f);
    range->setMaxInValue (0.601102f);
    range->setMinOutValue(0.209803f);
    range->setMaxOutValue(1.600208f);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(range->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);
}
