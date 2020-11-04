// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/LogTransform.cpp"

#include "OpBuilders.h"
#include "ops/log/LogOp.h"
#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(LogTransform, basic)
{
    const OCIO::LogTransformRcPtr log = OCIO::LogTransform::Create();

    OCIO_CHECK_EQUAL(log->getBase(), 2.0f);
    OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    log->setBase(10.0f);
    OCIO_CHECK_EQUAL(log->getBase(), 10.0f);

    OCIO::OpRcPtrVec ops;

    // Convert to op.
    OCIO::BuildLogOp(ops, *log, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<LogOp>");

    // Convert back to transform.
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    OCIO::ConstOpRcPtr op(ops[0]);
    OCIO::CreateLogTransform(group, op);

    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto lTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::LogTransform>(transform);
    OCIO_REQUIRE_ASSERT(lTransform);

}

