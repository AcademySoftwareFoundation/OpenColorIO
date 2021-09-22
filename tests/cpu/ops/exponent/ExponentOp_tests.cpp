// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/exponent/ExponentOp.cpp"

#include "ops/noop/NoOps.h"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(ExponentOp, value)
{
    const double exp1[4] = { 1.2, 1.3, 1.4, 1.5 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 2);

    OCIO_CHECK_NO_THROW(ops.finalize());

    float error = 1e-6f;

    const float source[] = {  0.1f, 0.3f, 0.9f, 0.5f };

    const float result1[] = { 0.0630957261f, 0.209053621f,
                              0.862858355f, 0.353553385f };

    float tmp[4];
    memcpy(tmp, source, 4*sizeof(float));
    ops[0]->apply(tmp, tmp, 1);

    for(unsigned int i=0; i<4; ++i)
    {
        OCIO_CHECK_CLOSE(tmp[i], result1[i], error);
    }

    ops[1]->apply(tmp, tmp, 1);
    for(unsigned int i=0; i<4; ++i)
    {
        OCIO_CHECK_CLOSE(tmp[i], source[i], error);
    }
}

void ValidateOp(const float * source, const OCIO::OpRcPtr op, const float * result, const float error)
{
    float tmp[4];
    memcpy(tmp, source, 4 * sizeof(float));
    op->apply(tmp, tmp, 1);

    for (unsigned int i = 0; i<4; ++i)
    {
        OCIO_CHECK_CLOSE(tmp[i], result[i], error);
    }
}

OCIO_ADD_TEST(ExponentOp, value_limits)
{
    const double exp1[4] = { 0., 2., -2., 1.5 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());

    float error = 1e-6f;

    const float source1[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    const float result1[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ValidateOp(source1, ops[0], result1, error);

    const float source2[] = { 2.0f, 2.0f, 2.0f, 2.0f };
    const float result2[] = { 1.0f, 4.0f, 0.25f, 2.82842708f };
    ValidateOp(source2, ops[0], result2, error);

    const float source3[] = { -2.0f, -2.0f, 1.0f, -2.0f };
    const float result3[] = { 1.0f, 0.0f, 1.0f, 0.0f };
    ValidateOp(source3, ops[0], result3, error);

    const float source4[] = { 0.0f, 0.0f, 1.0f, 0.0f };
    const float result4[] = { 1.0f, 0.0f, 1.0f, 0.0f };
    ValidateOp(source4, ops[0], result4, error);
}

OCIO_ADD_TEST(ExponentOp, combining)
{
    const float error = 1e-6f;
    {
    const double exp1[4] = { 2.0, 2.0, 2.0, 1.0 };
    const double exp2[4] = { 1.2, 1.2, 1.2, 1.0 };

    auto expData1 = std::make_shared<OCIO::ExponentOpData>(exp1);
    auto expData2 = std::make_shared<OCIO::ExponentOpData>(exp2);
    expData1->setName("Exp1");
    expData1->setID("ID1");
    expData1->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "First exponent");
    expData2->setName("Exp2");
    expData2->setID("ID2");
    expData2->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "Second exponent");
    expData2->getFormatMetadata().addAttribute("Attrib", "value");

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, expData1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, expData2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO_CHECK_NO_THROW(ops.finalize());

    OCIO::ConstOpRcPtr op1 = ops[1];

    const float source[] = {  0.9f, 0.4f, 0.1f, 0.5f, };
    const float result[] = { 0.776572466f, 0.110903174f,
                             0.00398107106f, 0.5f };

    float tmp[4];
    memcpy(tmp, source, 4*sizeof(float));
    ops[0]->apply(tmp, tmp, 1);
    ops[1]->apply(tmp, tmp, 1);

    for(unsigned int i=0; i<4; ++i)
    {
        OCIO_CHECK_CLOSE(tmp[i], result[i], error);
    }

    OCIO::OpRcPtrVec combined;
    OCIO_CHECK_NO_THROW(ops[0]->combineWith(combined, op1));
    OCIO_CHECK_EQUAL(combined.size(), 1);

    auto combinedData = OCIO::DynamicPtrCast<const OCIO::Op>(combined[0])->data();

    // Check metadata of combined op.
    OCIO_CHECK_EQUAL(combinedData->getName(), "Exp1 + Exp2");
    OCIO_CHECK_EQUAL(combinedData->getID(), "ID1 + ID2");
    OCIO_REQUIRE_EQUAL(combinedData->getFormatMetadata().getNumChildrenElements(), 2);
    const auto & child0 = combinedData->getFormatMetadata().getChildElement(0);
    OCIO_CHECK_EQUAL(std::string(child0.getElementName()), OCIO::METADATA_DESCRIPTION);
    OCIO_CHECK_EQUAL(std::string(child0.getElementValue()), "First exponent");
    const auto & child1 = combinedData->getFormatMetadata().getChildElement(1);
    OCIO_CHECK_EQUAL(std::string(child1.getElementName()), OCIO::METADATA_DESCRIPTION);
    OCIO_CHECK_EQUAL(std::string(child1.getElementValue()), "Second exponent");
    // 3 attributes: name, id and Attrib.
    OCIO_CHECK_EQUAL(combinedData->getFormatMetadata().getNumAttributes(), 3);
    auto & attribs = combinedData->getFormatMetadata().getAttributes();
    OCIO_CHECK_EQUAL(attribs[2].first, "Attrib");
    OCIO_CHECK_EQUAL(attribs[2].second, "value");

    OCIO_CHECK_NO_THROW(combined.finalize());

    float tmp2[4];
    memcpy(tmp2, source, 4*sizeof(float));
    combined[0]->apply(tmp2, tmp2, 1);

    for(unsigned int i=0; i<4; ++i)
    {
        OCIO_CHECK_CLOSE(tmp2[i], result[i], error);
    }
    }

    {

    const double exp1[4] = {1.037289, 1.019015, 0.966082, 1.0};

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_INVERSE));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO::OpRcPtrVec combined;
    OCIO_CHECK_NO_THROW(ops[0]->combineWith(combined, op1));
    OCIO_CHECK_EQUAL(combined.empty(), true);

    }

    {
    const double exp1[4] = { 1.037289, 1.019015, 0.966082, 1.0 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_EQUAL(ops.size(), 3);

    const float source[] = { 0.1f, 0.5f, 0.9f, 0.5f, };
    const float result[] = { 0.0765437484f, 0.480251998f,
        0.909373641f, 0.5f };

    float tmp[4];
    memcpy(tmp, source, 4 * sizeof(float));
    ops[0]->apply(tmp, tmp, 1);
    ops[1]->apply(tmp, tmp, 1);
    ops[2]->apply(tmp, tmp, 1);

    for (unsigned int i = 0; i<4; ++i)
    {
        OCIO_CHECK_CLOSE(tmp[i], result[i], error);
    }

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 1);

    memcpy(tmp, source, 4 * sizeof(float));
    ops[0]->apply(tmp, tmp, 1);

    for (unsigned int i = 0; i<4; ++i)
    {
        OCIO_CHECK_CLOSE(tmp[i], result[i], error);
    }
    }
}

OCIO_ADD_TEST(ExponentOp, throw_create)
{
    const double exp1[4] = { 0.0, 1.3, 1.4, 1.5 };

    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_THROW_WHAT(
        OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_INVERSE),
        OCIO::Exception, "Cannot apply 0.0 exponent in the inverse");
}

OCIO_ADD_TEST(ExponentOp, can_combine_with)
{
    const double exp1[4] = { 0.0, 1.3, 1.4, 1.5 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateFileNoOp(ops, "NoOp"));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(!ops[0]->canCombineWith(op1));
    OCIO_CHECK_THROW_WHAT(ops[0]->combineWith(ops, op1), OCIO::Exception,
                          "ExponentOp: canCombineWith must be checked")
}

OCIO_ADD_TEST(ExponentOp, noop)
{
    const double exp1[4] = { 1.0, 1.0, 1.0, 1.0 };

    // CreateExponentOp will create a NoOp
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_INVERSE));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_ASSERT(ops[0]->isNoOp());
    OCIO_CHECK_ASSERT(ops[1]->isNoOp());

    // Optimize it.
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);
}

OCIO_ADD_TEST(ExponentOp, cache_id)
{
    const double exp1[4] = { 2.0, 2.1, 3.0, 3.1 };
    const double exp2[4] = { 4.0, 4.1, 5.0, 5.1 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateExponentOp(ops, exp1, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_EQUAL(ops.size(), 3);

    OCIO_CHECK_NO_THROW(ops.validate());

    std::string opCacheID0;
    OCIO_CHECK_NO_THROW(opCacheID0 = ops[0]->getCacheID());
    std::string opCacheID1;
    OCIO_CHECK_NO_THROW(opCacheID1 = ops[1]->getCacheID());
    std::string opCacheID2;
    OCIO_CHECK_NO_THROW(opCacheID2 = ops[2]->getCacheID());

    OCIO_CHECK_EQUAL(opCacheID0, opCacheID2);
    OCIO_CHECK_NE(opCacheID0, opCacheID1);
}

OCIO_ADD_TEST(ExponentOp, create_transform)
{
    const double exp[4] = { 2.0, 2.1, 3.0, 3.1 };
    OCIO::ConstOpRcPtr op(new OCIO::ExponentOp(exp));

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    OCIO::CreateExponentTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto expTransform = OCIO::DynamicPtrCast<OCIO::ExponentTransform>(transform);
    OCIO_REQUIRE_ASSERT(expTransform);

    OCIO_CHECK_EQUAL(expTransform->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    double expVal[4];
    expTransform->getValue(expVal);
    OCIO_CHECK_EQUAL(expVal[0], exp[0]);
    OCIO_CHECK_EQUAL(expVal[1], exp[1]);
    OCIO_CHECK_EQUAL(expVal[2], exp[2]);
    OCIO_CHECK_EQUAL(expVal[3], exp[3]);
}

