// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/ExponentWithLinearTransform.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{

void CheckValues(const double(&v1)[4], const double(&v2)[4])
{
    static const float errThreshold = 1e-8f;

    OCIO_CHECK_CLOSE(v1[0], v2[0], errThreshold);
    OCIO_CHECK_CLOSE(v1[1], v2[1], errThreshold);
    OCIO_CHECK_CLOSE(v1[2], v2[2], errThreshold);
    OCIO_CHECK_CLOSE(v1[3], v2[3], errThreshold);
}

};

OCIO_ADD_TEST(ExponentWithLinearTransform, basic)
{
    OCIO::ExponentWithLinearTransformRcPtr exp = OCIO::ExponentWithLinearTransform::Create();
    OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    exp->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    double val4[4] = { -1., -1. -1. -1. };

    OCIO_CHECK_NO_THROW(exp->getGamma(val4));
    CheckValues(val4, { 1., 1., 1., 1. });

    val4[1] = 2.1234567;
    OCIO_CHECK_NO_THROW(exp->setGamma(val4));
    val4[1] = -1.;
    OCIO_CHECK_NO_THROW(exp->getGamma(val4));
    CheckValues(val4, {1., 2.1234567, 1., 1.});

    OCIO_CHECK_NO_THROW(exp->getOffset(val4));
    CheckValues(val4, { 0., 0., 0., 0. });

    val4[1] = 0.1234567;
    OCIO_CHECK_NO_THROW(exp->setOffset(val4));
    val4[1] = -1.;
    OCIO_CHECK_NO_THROW(exp->getOffset(val4));
    CheckValues(val4, { 0., 0.1234567, 0., 0. });

    OCIO_CHECK_EQUAL(exp->getNegativeStyle(), OCIO::NEGATIVE_LINEAR);
    OCIO_CHECK_NO_THROW(exp->setNegativeStyle(OCIO::NEGATIVE_MIRROR));
    OCIO_CHECK_EQUAL(exp->getNegativeStyle(), OCIO::NEGATIVE_MIRROR);
    OCIO_CHECK_THROW_WHAT(exp->setNegativeStyle(OCIO::NEGATIVE_PASS_THRU), OCIO::Exception,
                          "Pass thru negative extrapolation is not valid for MonCurve");
    OCIO_CHECK_THROW_WHAT(exp->setNegativeStyle(OCIO::NEGATIVE_CLAMP), OCIO::Exception,
                          "Clamp negative extrapolation is not valid");
    OCIO_CHECK_NO_THROW(exp->setNegativeStyle(OCIO::NEGATIVE_LINEAR));
    OCIO_CHECK_EQUAL(exp->getNegativeStyle(), OCIO::NEGATIVE_LINEAR);
}

