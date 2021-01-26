// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "OpBuilders.h"

#include "ops/gradingtone/GradingToneOp.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(GradingToneOp, create)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::GradingToneOpDataRcPtr data =
        std::make_shared<OCIO::GradingToneOpData>(OCIO::GRADING_LOG);
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateGradingToneOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<GradingToneOp>");
    OCIO_CHECK_ASSERT(ops[0]->isIdentity());
    OCIO_CHECK_ASSERT(ops[0]->isNoOp());

    data->getDynamicPropertyInternal()->makeDynamic();
    OCIO_CHECK_NO_THROW(OCIO::CreateGradingToneOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_REQUIRE_ASSERT(ops[1]);
    OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<GradingToneOp>");
    OCIO_CHECK_ASSERT(!ops[1]->isIdentity());
    OCIO_CHECK_ASSERT(!ops[1]->isNoOp());
}

OCIO_ADD_TEST(GradingToneOp, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::GradingToneOpDataRcPtr data =
        std::make_shared<OCIO::GradingToneOpData>(OCIO::GRADING_LOG);
    data->getDynamicPropertyInternal()->makeDynamic();
    OCIO::OpRcPtrVec ops;

    data->getDynamicPropertyInternal()->makeDynamic();
    OCIO_CHECK_NO_THROW(OCIO::CreateGradingToneOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateGradingToneTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto gtTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::GradingToneTransform>(transform);
    OCIO_REQUIRE_ASSERT(gtTransform);
    OCIO_CHECK_EQUAL(gtTransform->getStyle(), OCIO::GRADING_LOG);
    OCIO_CHECK_ASSERT(gtTransform->isDynamic());
}

OCIO_ADD_TEST(GradingToneOp, build_ops)
{
    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateRaw();

    auto gtTransform = OCIO::GradingToneTransform::Create(OCIO::GRADING_LOG);

    // Identity does create an op.
    OCIO::OpRcPtrVec ops;
    OCIO::BuildOps(ops, *(config.get()), config->getCurrentContext(), gtTransform,
                   OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_ASSERT(ops[0]->isIdentity());
    OCIO_CHECK_ASSERT(ops[0]->isNoOp());
    ops.clear();

    // Make it dynamic and keep default values.
    gtTransform->makeDynamic();

    OCIO::BuildOps(ops, *(config.get()), config->getCurrentContext(), gtTransform,
                   OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<GradingToneOp>");
    OCIO::ConstGradingToneOpRcPtr gto = OCIO_DYNAMIC_POINTER_CAST<OCIO::GradingToneOp>(ops[0]);
    auto data = gto->data();
    OCIO_REQUIRE_ASSERT(data);
    auto gtd = OCIO_DYNAMIC_POINTER_CAST<const OCIO::GradingToneOpData>(data);
    OCIO_REQUIRE_ASSERT(gtd);
    OCIO_CHECK_ASSERT(gtd->isDynamic());
    
    auto valsOp = gtd->getValue();
    OCIO_CHECK_EQUAL(valsOp.m_scontrast, 1.);

    // Sharing of dynamic properties is done through processor, changing the source will not
    // change the op.
    OCIO::GradingTone vals(OCIO::GRADING_LOG);
    vals.m_scontrast = 1.1;
    gtTransform->setValue(vals);

    valsOp = gtd->getValue();
    OCIO_CHECK_EQUAL(valsOp.m_scontrast, 1.);

    auto proc = config->getProcessor(gtTransform);
    OCIO_CHECK_ASSERT(proc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_TONE));
    OCIO_CHECK_ASSERT(!proc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));

    auto cpu = proc->getDefaultCPUProcessor();

    // Get dynamic property from the CPU proc.
    OCIO::DynamicPropertyRcPtr dp;
    OCIO_CHECK_NO_THROW(dp = cpu->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_TONE));
    OCIO_REQUIRE_ASSERT(dp);
    // Get typed value accessor.
    auto dpgt = OCIO_DYNAMIC_POINTER_CAST<OCIO::DynamicPropertyGradingTone>(dp);
    OCIO_REQUIRE_ASSERT(dpgt);

    // Set identity value in dynamic property.
    vals.m_scontrast = 1.0;
    dpgt->setValue(vals);

    float pixel[]{ 0.0f, 0.2f, 2.0f };
    cpu->applyRGB(pixel);
    // Default values are identity.
    OCIO_CHECK_EQUAL(pixel[0], 0.0f);
    OCIO_CHECK_EQUAL(pixel[1], 0.2f);
    OCIO_CHECK_EQUAL(pixel[2], 2.0f);

    // Change values and update dynamic property.
    vals.m_scontrast = 1.1;
    vals.m_midtones.m_red = 1.1;
    dpgt->setValue(vals);

    // No longer identity.
    cpu->applyRGB(pixel);

    static constexpr float error = 1e-5f;
    OCIO_CHECK_CLOSE(pixel[0], 0.f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.18729f, error);
    OCIO_CHECK_CLOSE(pixel[2], 1.91875f, error);
}
