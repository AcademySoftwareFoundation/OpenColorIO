// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "OpBuilders.h"

#include "ops/gradinghuecurve/GradingHueCurveOp.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingHueCurveOp, create)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::GradingHueCurveOpDataRcPtr data =
       std::make_shared<OCIO::GradingHueCurveOpData>(OCIO::GRADING_LOG);
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateGradingHueCurveOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<GradingHueCurveOp>");
    OCIO_CHECK_ASSERT(ops[0]->isIdentity());
    OCIO_CHECK_ASSERT(ops[0]->isNoOp());

    data->getDynamicPropertyInternal()->makeDynamic();
    OCIO_CHECK_NO_THROW(OCIO::CreateGradingHueCurveOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_REQUIRE_ASSERT(ops[1]);
    OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<GradingHueCurveOp>");
    OCIO_CHECK_ASSERT(!ops[1]->isIdentity());
    OCIO_CHECK_ASSERT(!ops[1]->isNoOp());
}

OCIO_ADD_TEST(GradingHueCurveOp, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::GradingHueCurveOpDataRcPtr data =
       std::make_shared<OCIO::GradingHueCurveOpData>(OCIO::GRADING_LOG);
    data->getDynamicPropertyInternal()->makeDynamic();
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateGradingHueCurveOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateGradingHueCurveTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto gcTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::GradingHueCurveTransform>(transform);
    OCIO_REQUIRE_ASSERT(gcTransform);
    OCIO_CHECK_EQUAL(gcTransform->getStyle(), OCIO::GRADING_LOG);
    OCIO_CHECK_ASSERT(gcTransform->isDynamic());
}

OCIO_ADD_TEST(GradingHueCurveOp, build_ops)
{
    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateRaw();

    auto gcTransform = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_LOG);
    OCIO_CHECK_ASSERT(gcTransform.get());

    // Identity does create an op.
    OCIO::OpRcPtrVec ops;
    OCIO::BuildOps(ops, *(config.get()), config->getCurrentContext(), gcTransform,
                   OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_ASSERT(ops[0]->isIdentity());
    OCIO_CHECK_ASSERT(ops[0]->isNoOp());
    ops.clear();

    // Make it dynamic and keep default values.
    gcTransform->makeDynamic();

    OCIO::BuildOps(ops, *(config.get()), config->getCurrentContext(), gcTransform,
       OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<GradingHueCurveOp>");
    OCIO::ConstGradingHueCurveOpRcPtr gco = OCIO_DYNAMIC_POINTER_CAST<OCIO::GradingHueCurveOp>(ops[0]);
    auto data = gco->data();
    OCIO_REQUIRE_ASSERT(data);
    auto gcd = OCIO_DYNAMIC_POINTER_CAST<const OCIO::GradingHueCurveOpData>(data);
    OCIO_REQUIRE_ASSERT(gcd);
    OCIO_CHECK_ASSERT(gcd->isDynamic());

    auto valsOp = gcd->getValue();
    OCIO_CHECK_EQUAL(6, valsOp->getCurve(OCIO::HUE_HUE)->getNumControlPoints());

    // Create processor with dynamic identity before changing the transform.
    auto proc = config->getProcessor(gcTransform);
    OCIO_CHECK_ASSERT(proc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_HUECURVE));
    OCIO_CHECK_ASSERT(!proc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));

    auto cpu = proc->getDefaultCPUProcessor();

    // Create a non-identity curve.
    auto hueCurve = OCIO::GradingHueCurve::Create(OCIO::GRADING_LOG);
    OCIO::GradingBSplineCurveRcPtr spline = hueCurve->getCurve(OCIO::HUE_LUM);
    // Add a constant offset to all hues.
    spline->setNumControlPoints(2);
    spline->getControlPoint(0) = OCIO::GradingControlPoint(0.f, 2.f);
    spline->getControlPoint(1) = OCIO::GradingControlPoint(0.9f, 2.f);
    OCIO_CHECK_ASSERT(!hueCurve->isIdentity());

    // Sharing of dynamic properties is done through processor, changing the source transform
    // does not change the op that was created from it.
    gcTransform->setValue(hueCurve);
    valsOp = gcd->getValue();
    // (Still has its original value.)
    OCIO_CHECK_EQUAL(1.f, valsOp->getCurve(OCIO::HUE_LUM)->getControlPoint(0).m_y);

    // Get dynamic property from the CPU processor.
    OCIO::DynamicPropertyRcPtr dp;
    OCIO_CHECK_NO_THROW(dp = cpu->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_HUECURVE));
    OCIO_REQUIRE_ASSERT(dp);
    // Get typed value accessor.
    auto dpgc = OCIO_DYNAMIC_POINTER_CAST<OCIO::DynamicPropertyGradingHueCurve>(dp);
    OCIO_REQUIRE_ASSERT(dpgc);

    float pixel[]{ 0.0f, 0.2f, 2.0f };
    cpu->applyRGB(pixel);
    // Default values are identity.
    static constexpr float error = 1e-5f;
    OCIO_CHECK_CLOSE(pixel[0], 0.0f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.2f, error);
    OCIO_CHECK_CLOSE(pixel[2], 2.0f, error);

    // Set the modified curve.
    dpgc->setValue(hueCurve);

    // The pixel value is different.
    cpu->applyRGB(pixel);
    OCIO_CHECK_CLOSE(pixel[0], 0.1f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.3f, error);
    OCIO_CHECK_CLOSE(pixel[2], 2.1f, error);
}
