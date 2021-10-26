// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef OPENCOLORIO_OSL_UNITTEST_H
#define OPENCOLORIO_OSL_UNITTEST_H


#include "UnitTestTypes.h"

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include <OSL/wide.h>


struct OSLData;
typedef std::shared_ptr<OSLData> OSLDataRcPtr;

// Defined the unit test data.
struct OSLData
{
    static OSLDataRcPtr Create();

    OCIO::ConfigRcPtr m_config;

    OCIO::ConstTransformRcPtr m_transform;

    // Use these values for the processing instead of the default ones.
    Image m_inputValues;
    
    float m_threshold{ 1e-6f };
    float m_expectedMinimalValue{ 1e-6f };
    bool  m_relativeComparison{ false };

    std::string m_name;

    OSLData() : m_config(OCIO::Config::CreateRaw()->createEditableCopy()) {}
};

// Defines the unit test to be executed.
class UnitTest
{
public:
    UnitTest() : m_data(OSLData::Create()) {}

    // Implement the method to define a specific unit test.
    virtual void create() = 0;

    // The data of the unit test.
    OSLDataRcPtr m_data;
};


// Helper macro.

#ifdef FIELD_STR
    #error Unexpected defined macro 'FIELD_STR'
#endif

// Refer to https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html
// for the macro 'stringizing' and MISRA C++ 2008, 16-3-1 for the potential issue
// of having several # (and/or ##) in the same macro.
#define FIELD_STR(field) #field


#ifdef OCIO_OSL_TEST
    #error Unexpected defined macro 'OCIO_OSL_TEST'
#endif


#define OCIO_OSL_TEST(group, name)                               \
class UnitTest_##group##_##name final : public UnitTest          \
{                                                                \
public:                                                          \
    UnitTest_##group##_##name() : UnitTest()                     \
    {                                                            \
        m_data->m_name  = #group;                                \
        m_data->m_name += " / ";                                 \
        m_data->m_name += #name;                                 \
        create();                                                \
    }                                                            \
    void create() override;                                      \
} g_unitTest##group##_##name;                                    \
void UnitTest_##group##_##name::create()



#endif /* OPENCOLORIO_OSL_UNITTEST_H */
