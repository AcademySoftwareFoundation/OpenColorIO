// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "OpBuilders.h"

#include "ops/gradingrgbcurve/GradingRGBCurveOp.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingRGBCurveOp, create)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::GradingRGBCurveOpDataRcPtr data =
        std::make_shared<OCIO::GradingRGBCurveOpData>(OCIO::GRADING_LOG);
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateGradingRGBCurveOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<GradingRGBCurveOp>");
    OCIO_CHECK_ASSERT(ops[0]->isIdentity());
    OCIO_CHECK_ASSERT(ops[0]->isNoOp());

    data->getDynamicPropertyInternal()->makeDynamic();
    OCIO_CHECK_NO_THROW(OCIO::CreateGradingRGBCurveOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_REQUIRE_ASSERT(ops[1]);
    OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<GradingRGBCurveOp>");
    OCIO_CHECK_ASSERT(!ops[1]->isIdentity());
    OCIO_CHECK_ASSERT(!ops[1]->isNoOp());
}

OCIO_ADD_TEST(GradingRGBCurveOp, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::GradingRGBCurveOpDataRcPtr data =
        std::make_shared<OCIO::GradingRGBCurveOpData>(OCIO::GRADING_LOG);
    data->getDynamicPropertyInternal()->makeDynamic();
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateGradingRGBCurveOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateGradingRGBCurveTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto gcTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::GradingRGBCurveTransform>(transform);
    OCIO_REQUIRE_ASSERT(gcTransform);
    OCIO_CHECK_EQUAL(gcTransform->getStyle(), OCIO::GRADING_LOG);
    OCIO_CHECK_ASSERT(gcTransform->isDynamic());
}

OCIO_ADD_TEST(GradingRGBCurveOp, build_ops)
{
    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateRaw();

    auto gcTransform = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);

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
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<GradingRGBCurveOp>");
    OCIO::ConstGradingRGBCurveOpRcPtr gco = OCIO_DYNAMIC_POINTER_CAST<OCIO::GradingRGBCurveOp>(ops[0]);
    auto data = gco->data();
    OCIO_REQUIRE_ASSERT(data);
    auto gcd = OCIO_DYNAMIC_POINTER_CAST<const OCIO::GradingRGBCurveOpData>(data);
    OCIO_REQUIRE_ASSERT(gcd);
    OCIO_CHECK_ASSERT(gcd->isDynamic());

    auto valsOp = gcd->getValue();
    OCIO_CHECK_EQUAL(3, valsOp->getCurve(OCIO::RGB_GREEN)->getNumControlPoints());

    // Create processor with dynamic identity before changing the transform.
    auto proc = config->getProcessor(gcTransform);
    OCIO_CHECK_ASSERT(proc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_RGBCURVE));
    OCIO_CHECK_ASSERT(!proc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));

    auto cpu = proc->getDefaultCPUProcessor();

    // Sharing of dynamic properties is done through processor, changing the source will not
    // change the op.
    auto curve = OCIO::GradingBSplineCurve::Create({ { 0.f,1.f },{ 0.2f,0.3f },
                                                     { 0.5f,0.8f },{ 2.f,1.5f } });
    auto rgbCurve = OCIO::GradingRGBCurve::Create(curve, curve, curve, curve);
    gcTransform->setValue(rgbCurve);

    // Still use the default identity curves.
    valsOp = gcd->getValue();
    OCIO_CHECK_EQUAL(3, valsOp->getCurve(OCIO::RGB_GREEN)->getNumControlPoints());

    // Get dynamic property from the CPU proc.
    OCIO::DynamicPropertyRcPtr dp;
    OCIO_CHECK_NO_THROW(dp = cpu->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_RGBCURVE));
    OCIO_REQUIRE_ASSERT(dp);
    // Get typed value accessor.
    auto dpgc = OCIO_DYNAMIC_POINTER_CAST<OCIO::DynamicPropertyGradingRGBCurve>(dp);
    OCIO_REQUIRE_ASSERT(dpgc);

    float pixel[]{ 0.0f, 0.2f, 2.0f };
    cpu->applyRGB(pixel);
    // Default values are identity.
    static constexpr float error = 1e-5f;
    OCIO_CHECK_CLOSE(pixel[0], 0.0f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.2f, error);
    OCIO_CHECK_CLOSE(pixel[2], 2.0f, error);

    // Use other curve that has 4 control points.
    dpgc->setValue(rgbCurve);

    // Control point has moved.
    cpu->applyRGB(pixel);
    OCIO_CHECK_CLOSE(pixel[0], 1.11148262f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.04518771f, error);
    OCIO_CHECK_CLOSE(pixel[2], 1.32527864f, error);
}
