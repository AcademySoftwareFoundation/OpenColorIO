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

#ifndef OPENCOLORIO_GPU_UNITTEST_H
#define OPENCOLORIO_GPU_UNITTEST_H

#include <iostream>
#include <cmath>
#include <vector>
#include <string>


class OCIOGPUTest;

typedef void (*OCIOTestFunc)(OCIOGPUTest & test);

// The class holds the information around a specific GPU unit test
class OCIOGPUTest
{
    public:
        OCIOGPUTest(const std::string& testgroup, const std::string& testname, OCIOTestFunc test);

        inline const std::string& group() const  { return _group; }
        inline const std::string& name() const  { return _name; }

        void setContext(OCIO_NAMESPACE::TransformRcPtr transform, float errorThreshold);

        inline OCIO_NAMESPACE::ConstProcessorRcPtr& getProcessor() { return _processor; }
        inline float getErrorThreshold() { return _errorThreshold; }

        inline void setup() { _function(*this); }

    private:
        const std::string _group, _name;
        OCIOTestFunc _function;          
        OCIO_NAMESPACE::ConstProcessorRcPtr _processor;
        float _errorThreshold;
};

typedef std::vector<OCIOGPUTest*> UnitTests;

UnitTests& GetUnitTests();

struct AddTest { AddTest(OCIOGPUTest* test); };



#define OCIO_ADD_GPU_TEST(group, name)                                     \
    static void ocio_gputest_##group##_##name(OCIOGPUTest & test);         \
    AddTest ocio_##group##_##name(                                         \
        new OCIOGPUTest(#group, #name, ocio_gputest_##group##_##name));    \
    static void ocio_gputest_##group##_##name(OCIOGPUTest & test)


#endif /* OPENCOLORIO_GPU_UNITTEST_H */
