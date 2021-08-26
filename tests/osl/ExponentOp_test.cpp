// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


OCIO_OSL_TEST(Exponent, value)
{
    m_data->m_inValue   = OSL::Vec4(0.1,          0.3,         0.9,         0.5);
    m_data->m_outValue  = OSL::Vec4(0.0630957261, 0.209053621, 0.862858355, 0.353553385);

    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();

    static constexpr double expVal[4] = { 1.2, 1.3, 1.4, 1.5 };
    exp->setValue(expVal);
    m_data->m_transform = exp;
}
