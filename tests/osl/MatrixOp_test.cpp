// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


OCIO_OSL_TEST(Matrix, offset_only)
{
    m_data->m_inValue   = OSL::Vec4(0.1, 0.2, 0.6, 0.0);
    m_data->m_outValue  = OSL::Vec4(0.2, 0.4, 1.0, 0.0);

    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();

    static constexpr double offsets[4]{0.1, 0.2, 0.4, 0.0};
    matrix->setOffset(offsets);
    m_data->m_transform = matrix;
}

OCIO_OSL_TEST(Matrix, diagonal)
{
    m_data->m_inValue   = OSL::Vec4(0.1, 0.2, 0.6, 1.0);
    m_data->m_outValue  = OSL::Vec4(0.1, 0.4, 1.8, 1.0);

    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();

    static constexpr double values[16]{1.0, 0.0, 0.0, 0.0,
                                       0.0, 2.0, 0.0, 0.0,
                                       0.0, 0.0, 3.0, 0.0,
                                       0.0, 0.0, 0.0, 1.0};
    matrix->setMatrix(values);
    m_data->m_transform = matrix;
}
