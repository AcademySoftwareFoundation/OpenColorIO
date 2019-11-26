// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "ops/fixedfunction/FixedFunctionOp.cpp"

#include "MathUtils.h"
#include "pystring/pystring.h"
#include "UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(FixedFunctionOp, basic)
{
    OCIO::OpRcPtrVec ops;
    OCIO::FixedFunctionOpData::Params params;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, params, 
                                                    OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD));

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstFixedFunctionOpRcPtr func
        = OCIO::DynamicPtrCast<OCIO::FixedFunctionOp>(ops[0]);

    OCIO_CHECK_ASSERT(!func->isNoOp());
    OCIO_CHECK_ASSERT(!func->isIdentity());

    OCIO::ConstFixedFunctionOpDataRcPtr funcData 
        = OCIO::DynamicPtrCast<const OCIO::FixedFunctionOpData>(func->data());
    OCIO_REQUIRE_EQUAL(funcData->getStyle(), OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD);
    OCIO_REQUIRE_ASSERT(funcData->getParams() == params);
}

OCIO_ADD_TEST(FixedFunctionOp, glow03_cpu_engine)
{
    // Validate that the right CPU OP is created.

    const OCIO::FixedFunctionOpData::Style style = OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD;
    const OCIO::FixedFunctionOpData::Params data;

    OCIO::FixedFunctionOpDataRcPtr funcData 
        = std::make_shared<OCIO::FixedFunctionOpData>(data, style);

    OCIO::FixedFunctionOp func(funcData);
    OCIO_CHECK_NO_THROW(func.finalize(OCIO::OPTIMIZATION_NONE));

    OCIO::ConstOpCPURcPtr cpuOp = func.getCPUOp();
    const OCIO::OpCPU & c = *cpuOp;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(-1, pystring::find(typeName, "Renderer_ACES_Glow03_Fwd"));
}

OCIO_ADD_TEST(FixedFunctionOp, darktodim10_cpu_engine)
{
    // Validate that the right CPU OP is created.

    const OCIO::FixedFunctionOpData::Style style = OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD;
    const OCIO::FixedFunctionOpData::Params data;

    OCIO::FixedFunctionOpDataRcPtr funcData 
        = std::make_shared<OCIO::FixedFunctionOpData>(data, style);

    OCIO::FixedFunctionOp func(funcData);
    OCIO_CHECK_NO_THROW(func.finalize(OCIO::OPTIMIZATION_NONE));

    OCIO::ConstOpCPURcPtr cpuOp = func.getCPUOp();
    const OCIO::OpCPU & c = *cpuOp;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(-1, pystring::find(typeName, "Renderer_ACES_DarkToDim10_Fwd"));
}

OCIO_ADD_TEST(FixedFunctionOp, aces_red_mod_inv)
{
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_RED_MOD_03_INV));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_RED_MOD_03_FWD));

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(!op0->isIdentity());
    OCIO_CHECK_ASSERT(!op1->isIdentity());

    OCIO_CHECK_ASSERT(op0->isSameType(op1));
    OCIO_CHECK_ASSERT(op0->isInverse(op1));
    OCIO_CHECK_ASSERT(op1->isInverse(op0));
}

OCIO_ADD_TEST(FixedFunctionOp, aces_glow_inv)
{
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_GLOW_03_INV));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD));

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(!op0->isIdentity());
    OCIO_CHECK_ASSERT(!op1->isIdentity());

    OCIO_CHECK_ASSERT(op0->isSameType(op1));
    OCIO_CHECK_ASSERT(op0->isInverse(op1));
    OCIO_CHECK_ASSERT(op1->isInverse(op0));
}

OCIO_ADD_TEST(FixedFunctionOp, aces_darktodim10_inv)
{
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD));

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(!op0->isIdentity());
    OCIO_CHECK_ASSERT(!op1->isIdentity());

    OCIO_CHECK_ASSERT(op0->isSameType(op1));
    OCIO_CHECK_ASSERT(op0->isInverse(op1));
    OCIO_CHECK_ASSERT(op1->isInverse(op0));
}

OCIO_ADD_TEST(FixedFunctionOp, rec2100_surround_inv)
{
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, { 2. }, 
                                                    OCIO::FixedFunctionOpData::REC2100_SURROUND));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, { 1. / 2. }, 
                                                    OCIO::FixedFunctionOpData::REC2100_SURROUND));

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    {
        OCIO::ConstOpRcPtr op0 = ops[0];
        OCIO::ConstOpRcPtr op1 = ops[1];

        OCIO_CHECK_ASSERT(!op0->isIdentity());
        OCIO_CHECK_ASSERT(!op1->isIdentity());

        OCIO_CHECK_ASSERT(op0->isSameType(op1));
        OCIO_CHECK_ASSERT(op0->isInverse(op1));
        OCIO_CHECK_ASSERT(op1->isInverse(op0));
    }
    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, { 2.01 }, 
                                                    OCIO::FixedFunctionOpData::REC2100_SURROUND));

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    {
        OCIO::ConstOpRcPtr op0 = ops[0];
        OCIO::ConstOpRcPtr op1 = ops[1];
        OCIO::ConstOpRcPtr op2 = ops[2];

        OCIO_CHECK_ASSERT(!op0->isInverse(op2));
        OCIO_CHECK_ASSERT(!op1->isInverse(op2));
    }
}

OCIO_ADD_TEST(FixedFunctionOp, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    const OCIO::FixedFunctionOpData::Params data{ 2.01 };
    const OCIO::FixedFunctionOpData::Style style = OCIO::FixedFunctionOpData::REC2100_SURROUND;

    OCIO::FixedFunctionOpDataRcPtr funcData
        = std::make_shared<OCIO::FixedFunctionOpData>(data, style);
    funcData->getFormatMetadata().addAttribute("name", "test");

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, funcData, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateFixedFunctionTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto ffTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::FixedFunctionTransform>(transform);
    OCIO_REQUIRE_ASSERT(ffTransform);

    const auto & metadata = ffTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeName(0)), "name");
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeValue(0)), "test");

    OCIO_CHECK_EQUAL(ffTransform->getDirection(), direction);
    OCIO_CHECK_EQUAL(ffTransform->getStyle(), OCIO::FIXED_FUNCTION_REC2100_SURROUND);
    OCIO_CHECK_EQUAL(ffTransform->getNumParams(), 1);
    double param[1];
    ffTransform->getParams(param);
    OCIO_CHECK_EQUAL(param[0], 2.01);
}

