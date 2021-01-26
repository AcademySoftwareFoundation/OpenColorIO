// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "OpBuilders.h"

#include "ops/gradingprimary/GradingPrimaryOp.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(GradingPrimaryOp, create)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::GradingPrimaryOpDataRcPtr data =
        std::make_shared<OCIO::GradingPrimaryOpData>(OCIO::GRADING_LOG);
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateGradingPrimaryOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<GradingPrimaryOp>");
    OCIO_CHECK_ASSERT(ops[0]->isIdentity());
    OCIO_CHECK_ASSERT(ops[0]->isNoOp());

    data->getDynamicPropertyInternal()->makeDynamic();
    OCIO_CHECK_NO_THROW(OCIO::CreateGradingPrimaryOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_REQUIRE_ASSERT(ops[1]);
    OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<GradingPrimaryOp>");
    OCIO_CHECK_ASSERT(!ops[1]->isIdentity());
    OCIO_CHECK_ASSERT(!ops[1]->isNoOp());
}

OCIO_ADD_TEST(GradingPrimaryOp, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::GradingPrimaryOpDataRcPtr data =
        std::make_shared<OCIO::GradingPrimaryOpData>(OCIO::GRADING_LOG);
    data->getDynamicPropertyInternal()->makeDynamic();
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateGradingPrimaryOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateGradingPrimaryTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto gpTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::GradingPrimaryTransform>(transform);
    OCIO_REQUIRE_ASSERT(gpTransform);
    OCIO_CHECK_EQUAL(gpTransform->getStyle(), OCIO::GRADING_LOG);
    OCIO_CHECK_ASSERT(gpTransform->isDynamic());
}

OCIO_ADD_TEST(GradingPrimaryOp, build_ops)
{
    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateRaw();

    auto gpTransform = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LOG);

    // Identity does create an op.
    OCIO::OpRcPtrVec ops;
    OCIO::BuildOps(ops, *(config.get()), config->getCurrentContext(), gpTransform,
                   OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_ASSERT(ops[0]->isIdentity());
    OCIO_CHECK_ASSERT(ops[0]->isNoOp());
    ops.clear();

    // Make it dynamic and keep default values.
    gpTransform->makeDynamic();

    OCIO::BuildOps(ops, *(config.get()), config->getCurrentContext(), gpTransform,
                   OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<GradingPrimaryOp>");
    OCIO::ConstGradingPrimaryOpRcPtr gpo = OCIO_DYNAMIC_POINTER_CAST<OCIO::GradingPrimaryOp>(ops[0]);
    auto data = gpo->data();
    OCIO_REQUIRE_ASSERT(data);
    auto gpd = OCIO_DYNAMIC_POINTER_CAST<const OCIO::GradingPrimaryOpData>(data);
    OCIO_REQUIRE_ASSERT(gpd);
    OCIO_CHECK_ASSERT(gpd->isDynamic());
    
    auto valsOp = gpd->getValue();
    OCIO_CHECK_EQUAL(valsOp.m_pivotBlack, 0.);

    // Sharing of dynamic properties is done through processor, changing the source will not
    // change the op.
    OCIO::GradingPrimary vals{ OCIO::GRADING_LOG };
    vals.m_pivotBlack = 0.1;
    gpTransform->setValue(vals);

    valsOp = gpd->getValue();
    OCIO_CHECK_EQUAL(valsOp.m_pivotBlack, 0.);

    auto proc = config->getProcessor(gpTransform);
    OCIO_CHECK_ASSERT(proc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY));
    OCIO_CHECK_ASSERT(!proc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));
    OCIO_CHECK_THROW_WHAT(proc->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE),
                          OCIO::Exception, "Cannot find dynamic property");

    auto cpu = proc->getDefaultCPUProcessor();

    // Get dynamic property from the CPU proc.
    OCIO::DynamicPropertyRcPtr dp;
    OCIO_CHECK_NO_THROW(dp = cpu->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY));
    OCIO_REQUIRE_ASSERT(dp);
    // Get typed value accessor.
    auto dpgp = OCIO_DYNAMIC_POINTER_CAST<OCIO::DynamicPropertyGradingPrimary>(dp);
    OCIO_REQUIRE_ASSERT(dpgp);

    OCIO_CHECK_THROW_WHAT(cpu->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE),
                          OCIO::Exception, "Cannot find dynamic property");

    float pixel[]{ 0.0f, 0.2f, 2.0f };
    cpu->applyRGB(pixel);
    // Default values are identity.
    static constexpr float error = 1e-5f;
    OCIO_CHECK_CLOSE(pixel[0], 0.0f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.2f, error);
    OCIO_CHECK_CLOSE(pixel[2], 2.0f, 5.f * error);

    // Add clamping and update dynamic property.
    vals.m_clampBlack = 0.1;
    vals.m_clampWhite = 1.;
    dpgp->setValue(vals);

    // Values are now clamped.
    cpu->applyRGB(pixel);
    OCIO_CHECK_CLOSE(pixel[0], 0.1f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.2f, error);
    OCIO_CHECK_CLOSE(pixel[2], 1.0f, error);
}
