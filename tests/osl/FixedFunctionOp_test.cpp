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

OCIO_OSL_TEST(FixedFunction, aces_red_mod_03_inv)
{
    m_data->m_inValue   = OSL::Vec4(0.79670035, 0.05, 0.19934007, 0.5);
    m_data->m_outValue  = OSL::Vec4(0.9,        0.05, 0.22,       0.5);

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;
}

OCIO_OSL_TEST(FixedFunction, aces_red_mod_10)
{
    m_data->m_inValue   = OSL::Vec4(0.90,        0.05,   0.22,   0.5);
    m_data->m_outValue  = OSL::Vec4(0.77148211,  0.05,   0.22,   0.5);

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);

    m_data->m_transform = func;
}

OCIO_OSL_TEST(FixedFunction, aces_red_mod_10_inv)
{
    m_data->m_inValue   = OSL::Vec4(0.77148211,  0.05,   0.22,   0.5);
    m_data->m_outValue  = OSL::Vec4(0.90,        0.05,   0.22,   0.5);

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;
}

OCIO_OSL_TEST(FixedFunction, aces_glow_03)
{
    m_data->m_inValue   = OSL::Vec4(0.11,       0.02,        0.0,  0.5); // YC = 0.10
    m_data->m_outValue  = OSL::Vec4(0.11392101, 0.02071291f, 0.0,  0.5);

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_03);

    m_data->m_transform = func;
}

OCIO_OSL_TEST(FixedFunction, aces_glow_03_inv)
{
    m_data->m_inValue   = OSL::Vec4(0.11392101, 0.02071291f, 0.0,  0.5);
    m_data->m_outValue  = OSL::Vec4(0.11,       0.02,        0.0,  0.5);

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;
}

OCIO_OSL_TEST(FixedFunction, aces_glow_10)
{
    m_data->m_inValue   = OSL::Vec4(0.11,       0.02,       0.0, 0.5); // YC = 0.10
    m_data->m_outValue  = OSL::Vec4(0.11154121, 0.02028021, 0.0, 0.5);

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_10);

    m_data->m_transform = func;
}

OCIO_OSL_TEST(FixedFunction, aces_glow_10_inv)
{
    m_data->m_inValue   = OSL::Vec4(0.11154121, 0.02028021, 0.0, 0.5);
    m_data->m_outValue  = OSL::Vec4(0.11,       0.02,       0.0, 0.5);

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;
}

OCIO_OSL_TEST(FixedFunction, aces_dark_to_dim_10)
{
    m_data->m_inValue   = OSL::Vec4(0.11,       0.02,       0.04,       0.5);
    m_data->m_outValue  = OSL::Vec4(0.11661188, 0.02120216, 0.04240432, 0.5);

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10);

    m_data->m_transform = func;
}

OCIO_OSL_TEST(FixedFunction, aces_dark_to_dim_10_inv)
{
    m_data->m_inValue   = OSL::Vec4(0.11661188, 0.02120216, 0.04240432, 0.5);
    m_data->m_outValue  = OSL::Vec4(0.11,       0.02,       0.04,       0.5);

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;
}

OCIO_OSL_TEST(FixedFunction, aces_gamut_map_13)
{
    m_data->m_inValue   = OSL::Vec4(0.96663409472, 0.04819045216, 0.00719300006, 0.0); // ALEXA Wide Gamut
    m_data->m_outValue  = OSL::Vec4(0.96663409472, 0.08610087633, 0.04698687792, 0.0);

    static constexpr double params[] { 1.147, 1.264, 1.312, 0.815, 0.803, 0.880, 1.2 };

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GAMUT_COMP_13, params, 7);

    m_data->m_transform = func;
}

OCIO_OSL_TEST(FixedFunction, aces_gamut_map_13_inv)
{
    m_data->m_inValue   = OSL::Vec4(0.96663409472, 0.08610087633, 0.04698687792, 0.0);
    m_data->m_outValue  = OSL::Vec4(0.96663409472, 0.04819045216, 0.00719300006, 0.0);

    static constexpr double params[] { 1.147, 1.264, 1.312, 0.815, 0.803, 0.880, 1.2 };

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GAMUT_COMP_13, params, 7);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;
}

OCIO_OSL_TEST(FixedFunction, rec2100_surround)
{
    m_data->m_inValue   = OSL::Vec4(0.11,       0.02,       0.04,       0.5);
    m_data->m_outValue  = OSL::Vec4(0.21779590, 0.03959925, 0.07919850, 0.5);

    static constexpr double params[] { 0.78 };

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_REC2100_SURROUND, params, 1);

    m_data->m_transform = func;
}

OCIO_OSL_TEST(FixedFunction, rec2100_surround_inv)
{
    m_data->m_inValue   = OSL::Vec4(0.21779590, 0.03959925, 0.07919850, 0.5);
    m_data->m_outValue  = OSL::Vec4(0.11,       0.02,       0.04,       0.5);

    static constexpr double params[] { 1 / 0.78 };

    OCIO::FixedFunctionTransformRcPtr func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_REC2100_SURROUND, params, 1);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;
}
