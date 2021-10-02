// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/FixedFunctionTransform.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FixedFunctionTransform, basic)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    OCIO_CHECK_EQUAL(func->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(func->getStyle(), OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    OCIO_CHECK_EQUAL(func->getNumParams(), 0);
    OCIO_CHECK_NO_THROW(func->validate());

    OCIO_CHECK_NO_THROW(func->setDirection(OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(func->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(func->getStyle(), OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    OCIO_CHECK_EQUAL(func->getNumParams(), 0);
    OCIO_CHECK_NO_THROW(func->validate());

    OCIO_CHECK_NO_THROW(func->setStyle(OCIO::FIXED_FUNCTION_ACES_RED_MOD_10));
    OCIO_CHECK_EQUAL(func->getStyle(), OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);
    OCIO_CHECK_EQUAL(func->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(func->getNumParams(), 0);
    OCIO_CHECK_NO_THROW(func->validate());

    OCIO_CHECK_NO_THROW(func->setStyle(OCIO::FIXED_FUNCTION_ACES_GAMUT_COMP_13));
    OCIO_CHECK_EQUAL(func->getStyle(), OCIO::FIXED_FUNCTION_ACES_GAMUT_COMP_13);
    OCIO_CHECK_EQUAL(func->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(func->getNumParams(), 0);
    OCIO_CHECK_THROW_WHAT(func->validate(), OCIO::Exception,
                          "The style 'ACES_GamutComp13 (Inverse)' must have "
                          "seven parameters but 0 found.");
    OCIO::FixedFunctionOpData::Params values_7 = { 1.147, 1.264, 1.312, 0.815, 0.803, 0.880, 1.2 };
    OCIO_CHECK_NO_THROW(func->setParams(&values_7[0], 7));
    OCIO_CHECK_EQUAL(func->getNumParams(), 7);
    OCIO_CHECK_NO_THROW(func->validate());

    OCIO_CHECK_NO_THROW(func->setParams(nullptr, 0));
    OCIO_CHECK_NO_THROW(func->setStyle(OCIO::FIXED_FUNCTION_REC2100_SURROUND));
    OCIO_CHECK_THROW_WHAT(func->validate(), OCIO::Exception,
                          "The style 'REC2100_Surround (Inverse)' must have "
                          "one parameter but 0 found.");

    OCIO_CHECK_EQUAL(func->getNumParams(), 0);
    const double values_1[1] = { 1. };
    OCIO_CHECK_NO_THROW(func->setParams(&values_1[0], 1));
    OCIO_CHECK_EQUAL(func->getNumParams(), 1);
    double results[1] = { 0. };
    OCIO_CHECK_NO_THROW(func->getParams(&results[0]));
    OCIO_CHECK_EQUAL(results[0], values_1[0]);

    OCIO_CHECK_NO_THROW(func->validate());

    OCIO_CHECK_NO_THROW(func->setStyle(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10));
    OCIO_CHECK_THROW_WHAT(func->validate(), OCIO::Exception,
                          "The style 'ACES_DarkToDim10 (Inverse)' must have "
                          "zero parameters but 1 found.");

    OCIO_CHECK_NO_THROW(func->setStyle(OCIO::FIXED_FUNCTION_RGB_TO_HSV));
    OCIO_CHECK_THROW_WHAT(func->validate(), OCIO::Exception,
                          "The style 'RGB_TO_HSV' must have "
                          "zero parameters but 1 found.");

    OCIO_CHECK_THROW_WHAT(func->setStyle(OCIO::FIXED_FUNCTION_ACES_GAMUTMAP_02), OCIO::Exception,
                          "Unimplemented fixed function types: "
                          "FIXED_FUNCTION_ACES_GAMUTMAP_02, "
                          "FIXED_FUNCTION_ACES_GAMUTMAP_07.");

    OCIO_CHECK_THROW_WHAT(
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GAMUTMAP_07),
        OCIO::Exception, "Unimplemented fixed function types: "
                         "FIXED_FUNCTION_ACES_GAMUTMAP_02, "
                         "FIXED_FUNCTION_ACES_GAMUTMAP_07.");
}

OCIO_ADD_TEST(FixedFunctionTransform, createEditableCopy)
{
    OCIO::FixedFunctionTransformRcPtr func;

    // Create an editable copy for fixed transforms without params.

    OCIO_CHECK_NO_THROW(func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03));
    OCIO_CHECK_NO_THROW(func->createEditableCopy());

    // Create an editable copy for fixed transforms with params.

    constexpr double values[1] = { 1. };
    OCIO_CHECK_NO_THROW(func
        = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_REC2100_SURROUND, &values[0], 1));
    OCIO_CHECK_NO_THROW(func->createEditableCopy());
}
