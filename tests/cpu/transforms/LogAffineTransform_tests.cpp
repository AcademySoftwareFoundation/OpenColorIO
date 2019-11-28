// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "transforms/LogAffineTransform.cpp"

#include "UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{
bool AllEqual(double (&values)[3])
{
    return values[0] == values[1] && values[0] == values[2];
}
}

OCIO_ADD_TEST(LogAffineTransform, basic)
{
    const OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();

    const double base = log->getBase();
    OCIO_CHECK_EQUAL(base, 2.0);
    double values[3];
    log->getLinSideOffsetValue(values);
    OCIO_CHECK_ASSERT(AllEqual(values));
    OCIO_CHECK_EQUAL(values[0], 0.0);
    log->getLinSideSlopeValue(values);
    OCIO_CHECK_ASSERT(AllEqual(values));
    OCIO_CHECK_EQUAL(values[0], 1.0);
    log->getLogSideOffsetValue(values);
    OCIO_CHECK_ASSERT(AllEqual(values));
    OCIO_CHECK_EQUAL(values[0], 0.0);
    log->getLogSideSlopeValue(values);
    OCIO_CHECK_ASSERT(AllEqual(values));
    OCIO_CHECK_EQUAL(values[0], 1.0);
    OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    log->setBase(3.0);
    log->getBase();
    OCIO_CHECK_EQUAL(log->getBase(), 3.0);

    log->setLinSideOffsetValue({ 0.1, 0.2, 0.3 });
    log->getLinSideOffsetValue(values);
    OCIO_CHECK_EQUAL(values[0], 0.1);
    OCIO_CHECK_EQUAL(values[1], 0.2);
    OCIO_CHECK_EQUAL(values[2], 0.3);

    log->setLinSideSlopeValue({ 1.1, 1.2, 1.3 });
    log->getLinSideSlopeValue(values);
    OCIO_CHECK_EQUAL(values[0], 1.1);
    OCIO_CHECK_EQUAL(values[1], 1.2);
    OCIO_CHECK_EQUAL(values[2], 1.3);

    log->setLogSideOffsetValue({ 0.1, 0.2, 0.3 });
    log->getLogSideOffsetValue(values);
    OCIO_CHECK_EQUAL(values[0], 0.1);
    OCIO_CHECK_EQUAL(values[1], 0.2);
    OCIO_CHECK_EQUAL(values[2], 0.3);

    log->setLogSideSlopeValue({ 1.1, 1.2, 1.3 });
    log->getLogSideSlopeValue(values);
    OCIO_CHECK_EQUAL(values[0], 1.1);
    OCIO_CHECK_EQUAL(values[1], 1.2);
    OCIO_CHECK_EQUAL(values[2], 1.3);
}

