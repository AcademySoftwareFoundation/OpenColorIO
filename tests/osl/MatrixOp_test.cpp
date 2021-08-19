// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"

// TODO: OSL: Add macros to ease the unit test writing.

struct MatrixTest1
{
    MatrixTest1()
    {
        GPUTestRcPtr test = GPUTest::Create();

        test->m_inValue   = OSL::Vec4(0.1, 0.2, 0.6, 0.0);
        test->m_outValue  = OSL::Vec4(0.2, 0.4, 1.0, 0.0);

        OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();

        static constexpr double offsets[4]{0.1, 0.2, 0.4, 0.0};
        matrix->setOffset(offsets);
        test->m_transform = matrix;
    
        test->m_name = "Matrix";
    }
} matrix1;

struct MatrixTest2
{
    MatrixTest2()
    {
        GPUTestRcPtr test = GPUTest::Create();

        test->m_inValue   = OSL::Vec4(0.1, 0.2, 0.6, 1.0);
        test->m_outValue  = OSL::Vec4(0.1, 0.4, 1.8, 1.0);

        OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();

        static constexpr double values[16]{1.0, 0.0, 0.0, 0.0,
                                           0.0, 2.0, 0.0, 0.0,
                                           0.0, 0.0, 3.0, 0.0,
                                           0.0, 0.0, 0.0, 1.0};
        matrix->setMatrix(values);
        test->m_transform = matrix;
    
        test->m_name = "Matrix";
    }
} matrix2;
