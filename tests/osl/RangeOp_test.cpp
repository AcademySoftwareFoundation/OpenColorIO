// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


OCIO_OSL_TEST(Range, identity)
{
    m_data->m_inValue   = OSL::Vec4(0.1, 0.2, 0.6, 0.0);
    m_data->m_outValue  = OSL::Vec4(0.1, 0.2, 0.6, 0.0);

    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.);
    range->setMinOutValue(0.);

    m_data->m_transform = range;
}

OCIO_OSL_TEST(Range, scale)
{
    m_data->m_inValue   = OSL::Vec4(0.1, 0.2, 0.6, 0.0);
    m_data->m_outValue  = OSL::Vec4(0.3, 0.4, 0.8, 0.0);

    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.);
    range->setMaxInValue(1.);
    range->setMinOutValue(0.2);
    range->setMaxOutValue(1.2);

    m_data->m_transform = range;
}

OCIO_OSL_TEST(Range, scale_and_clamp)
{
    m_data->m_inValue   = OSL::Vec4(-0.1, 0.2, 1.6, 0.0);
    m_data->m_outValue  = OSL::Vec4( 0.2, 0.4, 1.2, 0.0);

    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.);
    range->setMaxInValue(1.);
    range->setMinOutValue(0.2);
    range->setMaxOutValue(1.2);

    m_data->m_transform = range;
}
