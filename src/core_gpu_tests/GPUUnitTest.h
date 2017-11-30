/*
    Made by Autodesk Inc. under the terms of the OpenColorIO BSD 3 Clause License
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
