// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/exponent/ExponentOp.h"
#include "ops/gamma/GammaOp.h"
#include "transforms/ExponentTransform.cpp"

#include "testutils/UnitTest.h"

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

OCIO_ADD_TEST(ExponentTransform, build_ops)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();
    const std::string id{ "sample exponent" };
    exp->getFormatMetadata().addAttribute(OCIO::METADATA_ID, id.c_str());
    OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(exp->getNegativeStyle(), OCIO::NEGATIVE_CLAMP);

    // With v1 config, exponent transform is converted to ExponentOp that does not handle
    // negative styles.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(1);
    {
        OCIO::OpRcPtrVec ops;
        OCIO::BuildExponentOp(ops, *config, *exp, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        auto op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
        auto data = OCIO::DynamicPtrCast<const OCIO::ExponentOpData>(op->data());
        OCIO_REQUIRE_ASSERT(data);
        // In v1 identity exponent is considered a No-op and will be removed (losing the clamp).
        OCIO_CHECK_ASSERT(data->isNoOp());
        OCIO_CHECK_EQUAL(id, data->getID());
    }
    exp->setNegativeStyle(OCIO::NEGATIVE_MIRROR);
    {
        OCIO::OpRcPtrVec ops;
        OCIO::BuildExponentOp(ops, *config, *exp, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        auto op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
        // Style is ignored, still getting a simple ExponentOpData.
        auto data = OCIO::DynamicPtrCast<const OCIO::ExponentOpData>(op->data());
        OCIO_REQUIRE_ASSERT(data);
        OCIO_CHECK_ASSERT(data->isNoOp());
    }

    // With v2 config, exponent transform is converted to GammaOp that handles negative styles.
    config->setMajorVersion(2);
    exp->setNegativeStyle(OCIO::NEGATIVE_CLAMP);
    {
        OCIO::OpRcPtrVec ops;
        OCIO::BuildExponentOp(ops, *config, *exp, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        auto op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
        auto data = OCIO::DynamicPtrCast<const OCIO::GammaOpData>(op->data());
        OCIO_REQUIRE_ASSERT(data);
        OCIO_CHECK_EQUAL(data->getStyle(), OCIO::GammaOpData::BASIC_FWD);
        OCIO_CHECK_ASSERT(data->isIdentity());
        // With v2 config clamping is preserved.
        OCIO_CHECK_ASSERT(!data->isNoOp());
        OCIO_CHECK_EQUAL(id, data->getID());
    }
    exp->setNegativeStyle(OCIO::NEGATIVE_MIRROR);
    {
        OCIO::OpRcPtrVec ops;
        OCIO::BuildExponentOp(ops, *config, *exp, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        auto op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
        auto data = OCIO::DynamicPtrCast<const OCIO::GammaOpData>(op->data());
        OCIO_REQUIRE_ASSERT(data);
        OCIO_CHECK_EQUAL(data->getStyle(), OCIO::GammaOpData::BASIC_MIRROR_FWD);
        OCIO_CHECK_ASSERT(data->isIdentity());
        OCIO_CHECK_ASSERT(data->isNoOp());
    }
    exp->setNegativeStyle(OCIO::NEGATIVE_PASS_THRU);
    {
        OCIO::OpRcPtrVec ops;
        OCIO::BuildExponentOp(ops, *config, *exp, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        auto op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
        auto data = OCIO::DynamicPtrCast<const OCIO::GammaOpData>(op->data());
        OCIO_REQUIRE_ASSERT(data);
        OCIO_CHECK_EQUAL(data->getStyle(), OCIO::GammaOpData::BASIC_PASS_THRU_FWD);
        OCIO_CHECK_ASSERT(data->isIdentity());
        OCIO_CHECK_ASSERT(data->isNoOp());
    }

    OCIO_CHECK_THROW_WHAT(exp->setNegativeStyle(OCIO::NEGATIVE_LINEAR), OCIO::Exception,
                          "Linear negative extrapolation is not valid for basic exponent style");
}
