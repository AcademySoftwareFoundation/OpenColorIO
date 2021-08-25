// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


OCIO_OSL_TEST(FixedFunction, aces_red_mod_03)
{
    m_data->m_inValue   = OSL::Vec4(0.9,        0.05, 0.22,       0.5);
    m_data->m_outValue  = OSL::Vec4(0.79670035, 0.05, 0.19934007, 0.5);

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);

    m_data->m_transform = func;
}
