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

OCIO_NAMESPACE_USING


// Helper method to build unit tests
void AddLogTest(OCIOGPUTest & test, TransformDirection direction,
    const float base, float epsilon)
{
    OCIO::LogTransformRcPtr log = OCIO::LogTransform::Create();
    log->setDirection(direction);
    log->setBase(base);

    test.setContext(log->createEditableCopy(), epsilon);
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase)
{
    const float base10 = 10.0f;
    AddLogTest(test, TRANSFORM_DIR_FORWARD, base10, 1e-5f);
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase_inverse)
{
    const float base10 = 10.0f;
    AddLogTest(test, TRANSFORM_DIR_INVERSE, base10, 1e-4f);
}


OCIO_ADD_GPU_TEST(LogTransform, euler_constant)
{
    const float eulerConstant = std::exp(1.0f);
    AddLogTest(test, TRANSFORM_DIR_FORWARD, eulerConstant, 1e-5f);
}


OCIO_ADD_GPU_TEST(LogTransform, euler_constant_inverse)
{
    const float eulerConstant = std::exp(1.0f);
    AddLogTest(test, TRANSFORM_DIR_INVERSE, eulerConstant, 1e-5f);
}


OCIO_ADD_GPU_TEST(LogTransform, base1_inverse)
{
    const float base1 = 1.0f;
    AddLogTest(test, TRANSFORM_DIR_INVERSE, base1, 1e-5f);
}
