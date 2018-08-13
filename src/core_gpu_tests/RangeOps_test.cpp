/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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

const float g_epsilon = 1e-5f;

OCIO_ADD_GPU_TEST(RangeOp, Identity)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setErrorThreshold(g_epsilon);
    test.setContext(range->createEditableCopy(), shaderDesc);
}

OCIO_ADD_GPU_TEST(RangeOp, scale_with_low_and_high_clippings)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.1f);
    range->setMaxInValue(1.1f);
    range->setMinOutValue(0.5f);
    range->setMaxOutValue(1.5f);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setErrorThreshold(g_epsilon);
    test.setContext(range->createEditableCopy(), shaderDesc);
}

OCIO_ADD_GPU_TEST(RangeOp, scale_with_low_clipping)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.1f);
    range->setMinOutValue(0.5f);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setErrorThreshold(g_epsilon);
    test.setContext(range->createEditableCopy(), shaderDesc);
}

OCIO_ADD_GPU_TEST(RangeOp, scale_with_high_clipping)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMaxInValue(0.9f);
    range->setMaxOutValue(1.05f);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setErrorThreshold(g_epsilon);
    test.setContext(range->createEditableCopy(), shaderDesc);
}

OCIO_ADD_GPU_TEST(RangeOp, scale_with_low_and_high_clippings_2)
{
    // The test has the scale + offset parts compare the tests above

    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.1f);
    range->setMaxInValue(1.1f);
    range->setMinOutValue(-0.5f);
    range->setMaxOutValue(1.5f);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setErrorThreshold(g_epsilon);
    test.setContext(range->createEditableCopy(), shaderDesc);
}

OCIO_ADD_GPU_TEST(RangeOp, Arbitrary_1)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.4f);
    range->setMaxInValue(0.6f);
    range->setMinOutValue(0.4f);
    range->setMaxOutValue(0.6f);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setErrorThreshold(g_epsilon);
    test.setContext(range->createEditableCopy(), shaderDesc);
}

OCIO_ADD_GPU_TEST(RangeOp, Arbitrary_2)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(-0.4f);
    range->setMaxInValue(0.6f);
    range->setMinOutValue(0.2f);
    range->setMaxOutValue(1.6f);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setErrorThreshold(g_epsilon);
    test.setContext(range->createEditableCopy(), shaderDesc);
}
