// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef OPENCOLORIO_OSL_UNITTEST_H
#define OPENCOLORIO_OSL_UNITTEST_H

#include <OSL/wide.h>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include <string>


struct OSLData;
typedef std::shared_ptr<OSLData> OSLDataRcPtr;

struct OSLData
{
    static OSLDataRcPtr Create();

    // It contains the color to process.
    OSL::Vec4 m_inValue;
    // It contains the expected color.
    OSL::Vec4 m_outValue;

    OCIO::ConstTransformRcPtr m_transform;

    std::string m_name;
};

class UnitTest
{
public:
    UnitTest() : m_data(OSLData::Create()) {}
    virtual void create() = 0;

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
class UnitTest_##group##_##name : public UnitTest                \
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
