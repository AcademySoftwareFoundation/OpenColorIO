// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "OpBuilders.h"
#include "ops/log/LogOp.h"
#include "transforms/LogAffineTransform.cpp"

#include "testutils/UnitTest.h"

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

    OCIO::OpRcPtrVec ops;

    // Convert to op.
    OCIO::BuildLogOp(ops, *log, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<LogOp>");

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    OCIO::ConstOpRcPtr op(ops[0]);
    // Convert back to transform.
    OCIO::CreateLogTransform(group, op);

    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    // Affine parameters are identity, so it comes back as a simple log.
    auto lTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::LogTransform>(transform);
    OCIO_REQUIRE_ASSERT(lTransform);

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

    log->setLogSideOffsetValue({ 0.4, 0.5, 0.6 });
    log->getLogSideOffsetValue(values);
    OCIO_CHECK_EQUAL(values[0], 0.4);
    OCIO_CHECK_EQUAL(values[1], 0.5);
    OCIO_CHECK_EQUAL(values[2], 0.6);

    log->setLogSideSlopeValue({ 1.4, 1.5, 1.6 });
    log->getLogSideSlopeValue(values);
    OCIO_CHECK_EQUAL(values[0], 1.4);
    OCIO_CHECK_EQUAL(values[1], 1.5);
    OCIO_CHECK_EQUAL(values[2], 1.6);

    // Convert to op and back to transform.
    OCIO::BuildLogOp(ops, *log, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<LogOp>");

    OCIO::ConstOpRcPtr op1(ops[1]);
    OCIO::CreateLogTransform(group, op1);

    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 2);
    auto transform2 = group->getTransform(1);
    OCIO_REQUIRE_ASSERT(transform2);
    auto lTransform2 = OCIO_DYNAMIC_POINTER_CAST<OCIO::LogAffineTransform>(transform2);
    OCIO_REQUIRE_ASSERT(lTransform2);
    OCIO_CHECK_ASSERT(lTransform2->equals(*log));
}

