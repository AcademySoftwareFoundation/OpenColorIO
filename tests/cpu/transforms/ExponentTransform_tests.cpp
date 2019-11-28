// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "transforms/ExponentTransform.cpp"

#include "UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(ExponentTransform, basic)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();
    OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    exp->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    double val4[4]{ 0., 0., 0., 0. };
    OCIO_CHECK_NO_THROW(exp->getValue(val4));
    OCIO_CHECK_EQUAL(val4[0], 1.);
    OCIO_CHECK_EQUAL(val4[1], 1.);
    OCIO_CHECK_EQUAL(val4[2], 1.);
    OCIO_CHECK_EQUAL(val4[3], 1.);

    val4[1] = 2.;
    OCIO_CHECK_NO_THROW(exp->setValue(val4));
    OCIO_CHECK_NO_THROW(exp->getValue(val4));
    OCIO_CHECK_EQUAL(val4[0], 1.);
    OCIO_CHECK_EQUAL(val4[1], 2.);
    OCIO_CHECK_EQUAL(val4[2], 1.);
    OCIO_CHECK_EQUAL(val4[3], 1.);
}

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

OCIO_ADD_TEST(ExponentTransform, double)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();
    OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    double val4[4] = { -1., -2., -3., -4. };
    OCIO_CHECK_NO_THROW(exp->getValue(val4));
    CheckValues(val4, { 1., 1., 1., 1. });

    val4[1] = 2.1234567;
    OCIO_CHECK_NO_THROW(exp->setValue(val4));
    val4[1] = -2.;
    OCIO_CHECK_NO_THROW(exp->getValue(val4));
    CheckValues(val4, {1., 2.1234567, 1., 1.});
}

