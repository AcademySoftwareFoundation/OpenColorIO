// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef OPENCOLORIO_OSL_UNITTEST_H
#define OPENCOLORIO_OSL_UNITTEST_H

#include <OSL/wide.h>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include <string>


struct GPUTest;
typedef std::shared_ptr<GPUTest> GPUTestRcPtr;

struct GPUTest
{
    static GPUTestRcPtr Create();

    // It contains the color to process.
    OSL::Vec4 m_inValue;
    // It contains the processed color.
    OSL::Vec4 m_outValue;

    OCIO::ConstTransformRcPtr m_transform;

    std::string m_name;
};

#endif /* OPENCOLORIO_OSL_UNITTEST_H */
