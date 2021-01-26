// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "OpBuilders.h"
#include "ops/log/LogOp.h"
#include "transforms/LogCameraTransform.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
bool AllEqual(double (&values)[3])
{
    return values[0] == values[1] && values[0] == values[2];
}
}

OCIO_ADD_TEST(LogCameraTransform, camera)
{
    const OCIO::LogCameraTransformRcPtr log = OCIO::LogCameraTransform::Create({ 0.2, 0.2, 0.2 });

    double values[3]{ -1., -1., -1. };

    log->getLinSideBreakValue(values);
    OCIO_CHECK_ASSERT(AllEqual(values));
    OCIO_CHECK_EQUAL(values[0], 0.2);

    OCIO_CHECK_ASSERT(!log->getLinearSlopeValue(values));

    OCIO_CHECK_NO_THROW(log->setLinearSlopeValue({ 1, 1, 1 }));
    OCIO_CHECK_ASSERT(log->getLinearSlopeValue(values));
    OCIO_CHECK_ASSERT(AllEqual(values));
    OCIO_CHECK_EQUAL(values[0], 1);

    log->unsetLinearSlopeValue();
    OCIO_CHECK_ASSERT(!log->getLinearSlopeValue(values));

    log->setLinSideBreakValue({ 0.01, 0.02, 0.03 });
    log->getLinSideBreakValue(values);
    OCIO_CHECK_EQUAL(values[0], 0.01);
    OCIO_CHECK_EQUAL(values[1], 0.02);
    OCIO_CHECK_EQUAL(values[2], 0.03);
    OCIO_CHECK_ASSERT(!log->getLinearSlopeValue(values));

    OCIO_CHECK_NO_THROW(log->setLinearSlopeValue({ 1, 1.1, 1.2 }));
    OCIO_CHECK_ASSERT(log->getLinearSlopeValue(values));
    OCIO_CHECK_EQUAL(values[0], 1);
    OCIO_CHECK_EQUAL(values[1], 1.1);
    OCIO_CHECK_EQUAL(values[2], 1.2);

    OCIO::OpRcPtrVec ops;

    // Convert to op and back to transform.
    OCIO::BuildLogOp(ops, *log, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<LogOp>");

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    OCIO::ConstOpRcPtr op(ops[0]);
    OCIO::CreateLogTransform(group, op);

    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto lTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::LogCameraTransform>(transform);
    OCIO_REQUIRE_ASSERT(lTransform);
    OCIO_CHECK_ASSERT(lTransform->equals(*log));
}
