/*
Copyright (c) 2019 Autodesk Inc., et al.
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

#include <functional>
#include <string>
#include <vector>

class OCIOGPUTest;

typedef void (*OCIOTestFunc)(OCIOGPUTest & test);

// Test harness for comparing GPU results to CPU results.
//
// By default, the framework compares the GPU results to the CPU results
// using an automatically generated neutral ramp for the standard range [0, 1].
// 
class OCIOGPUTest
{
    public:
        // The structure holds color values to check.
        struct CustomValues
        {
            typedef std::vector<float> Values;
            Values m_inputValues;

            // Keeping the original input value size allows 
            // to avoid manipulating padded values added to fit
            // the predefined GPU texture size.
            size_t m_originalInputValueSize;

            CustomValues() 
                : m_originalInputValueSize(0)
            {}
        };

    public:
        OCIOGPUTest(const std::string& testgroup, const std::string& testname, OCIOTestFunc test);

        ~OCIOGPUTest();

        inline const std::string& group() const  { return m_group; }
        inline const std::string& name() const  { return m_name; }

        void setContext(OCIO_NAMESPACE::ConstConfigRcPtr config,
                        OCIO_NAMESPACE::TransformRcPtr transform, 
                        OCIO_NAMESPACE::GpuShaderDescRcPtr shaderDesc);

        void setContext(OCIO_NAMESPACE::TransformRcPtr transform, 
                        OCIO_NAMESPACE::GpuShaderDescRcPtr shaderDesc);

        void setContextProcessor(OCIO_NAMESPACE::ConstProcessorRcPtr processor, 
                                 OCIO_NAMESPACE::GpuShaderDescRcPtr shaderDesc);

        inline OCIO_NAMESPACE::ConstProcessorRcPtr & getProcessor() { return m_processor; }
        inline OCIO_NAMESPACE::GpuShaderDescRcPtr & getShaderDesc() { return m_shaderDesc; }

        // Set TestWideRange to true to use test values on [-1,2] rather than [0,1].
        inline bool getTestWideRange() const { return m_testWideRange; }
        inline void setTestWideRange(bool use) { m_testWideRange = use; }

        // Set TestNaN to true to include NaNs in each channel of the test values.
        inline bool getTestNaN() const { return m_testNaN; }
        inline void setTestNaN(bool use) { m_testNaN = use; }

        // Set TestInfinity to true to include positive and negative infinity
        // in each channel of the test values.
        inline bool getTestInfinity() const { return m_testInfinity; }
        inline void setTestInfinity(bool use) { m_testInfinity = use; }

        // Provide a set of RGBA values to test (otherwise a neutral ramp will be used).
        // TestWideRange, TestNaN & TestInfinity are used when m_inputValues is empty. 
        inline void setCustomValues(CustomValues & values) { m_values = values; }
        inline CustomValues & getCustomValues() { return m_values; }

        inline float getErrorThreshold() const { return m_errorThreshold; }
        inline void setErrorThreshold(float error) { m_errorThreshold = error; }

        inline bool getRelativeComparison() const { return m_performRelativeComparison; }
        inline void setRelativeComparison(bool relCompare) { m_performRelativeComparison = relCompare; }

        // This is the lower bound for the value that is divided into the absolute error 
        // to obtain the relative error.
        inline float getExpectedMinimalValue() const { return m_expectedMinimalValue; }
        inline void setExpectedMinimalValue(float minValue) { m_expectedMinimalValue = minValue; }

        // Dump or not the gpu shader program
        inline void setVerbose(bool verbose) { m_verbose = verbose; }
        inline bool isVerbose() const { return m_verbose; }

        inline void setup() { m_function(*this); }

        inline bool isValid() { return m_processor && m_shaderDesc; }

        inline void disable() { m_enabled = false; }
        inline bool isEnabled() const { return m_enabled; }

        // Testing dynamic properties requires to run several times the same
        // unit test with the same shader code with some dynamic property
        // value changes. Registering n callbacks (i.e. each callback changing
        // a value for example) would run 1+n times the unit test.
        // Test runs, then for each callback, it is called and test runs again.
        // See ECOps_test.cpp for an example.
        typedef std::function<void()> RetestSetupCallback;
        inline void addRetest(RetestSetupCallback & retest)
        {
            m_retests.push_back(retest);
        }

        inline size_t getNumRetests() const
        {
            return m_retests.size();
        }

        void retestSetup(size_t idx);

        inline void updateMaxDiff(float maxDiff, size_t idxDiff)
        {
            if (maxDiff > m_maxDiff)
            {
                m_maxDiff = maxDiff;
                m_idxDiff = idxDiff;
            }
        }
        inline float getMaxDiff() const
        {
            return m_maxDiff;
        }
        inline size_t getMaxDiffIndex() const
        {
            return m_idxDiff;
        }

    private:
        const std::string m_group, m_name;
        OCIOTestFunc m_function;          
        OCIO_NAMESPACE::ConstProcessorRcPtr m_processor;
        OCIO_NAMESPACE::GpuShaderDescRcPtr m_shaderDesc;
        float m_errorThreshold;
        float m_expectedMinimalValue = 1e-6f;
        float m_maxDiff = 0.f;
        size_t m_idxDiff = 0;
        bool m_testWideRange = true;
        bool m_testNaN = true;
        bool m_testInfinity = true;
        bool m_performRelativeComparison = false;
        bool m_verbose = false;
        bool m_enabled = true;
        CustomValues m_values;

        std::vector<RetestSetupCallback> m_retests;

};

typedef std::shared_ptr<OCIOGPUTest> OCIOGPUTestRcPtr;
typedef std::vector<OCIOGPUTestRcPtr> UnitTests;

UnitTests& GetUnitTests();

struct AddTest { AddTest(OCIOGPUTestRcPtr test); };


// Use this macro to declare a test and provide a setup function for the test.
#define OCIO_ADD_GPU_TEST(group, name)                                    \
    static void ocio_gputest_##group##_##name(OCIOGPUTest & test);        \
    AddTest ocio_##group##_##name(                                        \
        std::make_shared<OCIOGPUTest>(#group,                             \
                                      #name,                              \
                                      ocio_gputest_##group##_##name));    \
    static void ocio_gputest_##group##_##name(OCIOGPUTest & test)

// Use this macro inside OCIO_ADD_GPU_TEST function to disable the test.
// The remaining of the function implementation will be skipped.
#define OCIO_DISABLE_GPU_TEST() \
    test.disable();             \
    if (!test.isEnabled()) return; // Test to avoid unreacheable code warning.

#endif /* OPENCOLORIO_GPU_UNITTEST_H */
