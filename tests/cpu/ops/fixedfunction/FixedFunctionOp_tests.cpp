// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/fixedfunction/FixedFunctionOp.cpp"

#include "MathUtils.h"
#include "utils/StringUtils.h"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FixedFunctionOp, basic)
{
    OCIO::OpRcPtrVec ops;
    OCIO::FixedFunctionOpData::Params params;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, 
                                                    OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD,
                                                    params));

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
        = std::make_shared<OCIO::FixedFunctionOpData>(style, data);

    OCIO::FixedFunctionOp func(funcData);
    OCIO_CHECK_NO_THROW(func.validate());

    OCIO::ConstOpCPURcPtr cpuOp = func.getCPUOp(false);
    const OCIO::OpCPU & c = *cpuOp;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "Renderer_ACES_Glow03_Fwd"));
}

OCIO_ADD_TEST(FixedFunctionOp, darktodim10_cpu_engine)
{
    // Validate that the right CPU OP is created.

    const OCIO::FixedFunctionOpData::Style style = OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD;
    const OCIO::FixedFunctionOpData::Params data;

    OCIO::FixedFunctionOpDataRcPtr funcData 
        = std::make_shared<OCIO::FixedFunctionOpData>(style, data);

    OCIO::FixedFunctionOp func(funcData);
    OCIO_CHECK_NO_THROW(func.validate());

    OCIO::ConstOpCPURcPtr cpuOp = func.getCPUOp(false);
    const OCIO::OpCPU & c = *cpuOp;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "Renderer_ACES_DarkToDim10_Fwd"));
}

OCIO_ADD_TEST(FixedFunctionOp, aces_red_mod_inv)
{
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops,
                                                    OCIO::FixedFunctionOpData::ACES_RED_MOD_03_INV,
                                                    {}));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops,
                                                    OCIO::FixedFunctionOpData::ACES_RED_MOD_03_FWD,
                                                    {}));

    OCIO_CHECK_NO_THROW(ops.finalize());
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

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops,
                                                    OCIO::FixedFunctionOpData::ACES_GLOW_03_INV,
                                                    {}));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops,
                                                    OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD,
                                                    {}));

    OCIO_CHECK_NO_THROW(ops.finalize());
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

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops,
                                                    OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV,
                                                    {}));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops,
                                                    OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD,
                                                    {}));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(!op0->isIdentity());
    OCIO_CHECK_ASSERT(!op1->isIdentity());

    OCIO_CHECK_ASSERT(op0->isSameType(op1));
    OCIO_CHECK_ASSERT(op0->isInverse(op1));
    OCIO_CHECK_ASSERT(op1->isInverse(op0));
}

OCIO_ADD_TEST(FixedFunctionOp, aces_gamutmap13_inv)
{
    OCIO::OpRcPtrVec ops;

    OCIO::FixedFunctionOpData::Params params = { 1.147, 1.264, 1.312, 0.815, 0.803, 0.880, 1.2 };

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops,
                                                    OCIO::FixedFunctionOpData::ACES_GAMUT_COMP_13_INV,
                                                    params));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops,
                                                    OCIO::FixedFunctionOpData::ACES_GAMUT_COMP_13_FWD,
                                                    params));

    OCIO_CHECK_NO_THROW(ops.finalize());
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

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops,
                                                    OCIO::FixedFunctionOpData::REC2100_SURROUND_FWD,
                                                    { 2. }));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops,
                                                    OCIO::FixedFunctionOpData::REC2100_SURROUND_FWD,
                                                    { 1. / 2. }));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops,
                                                    OCIO::FixedFunctionOpData::REC2100_SURROUND_INV,
                                                    { 2. }));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    {
        OCIO::ConstOpRcPtr op0 = ops[0];
        OCIO::ConstOpRcPtr op1 = ops[1];
        OCIO::ConstOpRcPtr op2 = ops[2];

        OCIO_CHECK_ASSERT(!op0->isIdentity());
        OCIO_CHECK_ASSERT(!op1->isIdentity());
        OCIO_CHECK_ASSERT(!op2->isIdentity());

        OCIO_CHECK_ASSERT(op0->isSameType(op1));
        OCIO_CHECK_ASSERT(op0->isInverse(op1));
        OCIO_CHECK_ASSERT(op1->isInverse(op0));
        OCIO_CHECK_ASSERT(op0->isInverse(op2));
        OCIO_CHECK_ASSERT(op2->isInverse(op0));
    }
    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops,
                                                    OCIO::FixedFunctionOpData::REC2100_SURROUND_FWD,
                                                    { 2.01 }));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    {
        OCIO::ConstOpRcPtr op0 = ops[0];
        OCIO::ConstOpRcPtr op1 = ops[1];
        OCIO::ConstOpRcPtr op3 = ops[3];

        OCIO_CHECK_ASSERT(!op0->isInverse(op3));
        OCIO_CHECK_ASSERT(!op1->isInverse(op3));
    }
}

OCIO_ADD_TEST(FixedFunctionOp, create_transform)
{
    const OCIO::FixedFunctionOpData::Params data{ 0.5 };
    const OCIO::FixedFunctionOpData::Style style = OCIO::FixedFunctionOpData::REC2100_SURROUND_INV;

    OCIO::FixedFunctionOpDataRcPtr funcData
        = std::make_shared<OCIO::FixedFunctionOpData>(style, data);

    OCIO_CHECK_EQUAL(OCIO::FixedFunctionOpData::REC2100_SURROUND_INV, funcData->getStyle());
    // Direction is already inverse, this does nothing.
    funcData->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(OCIO::FixedFunctionOpData::REC2100_SURROUND_INV, funcData->getStyle());
    // Changing the direction is changing the style.
    funcData->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(OCIO::FixedFunctionOpData::REC2100_SURROUND_FWD, funcData->getStyle());
    funcData->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(OCIO::FixedFunctionOpData::REC2100_SURROUND_INV, funcData->getStyle());

    funcData->getFormatMetadata().addAttribute("name", "test");

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, funcData, OCIO::TRANSFORM_DIR_FORWARD));
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

    OCIO_CHECK_EQUAL(ffTransform->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(ffTransform->getStyle(), OCIO::FIXED_FUNCTION_REC2100_SURROUND);
    OCIO_CHECK_EQUAL(ffTransform->getNumParams(), 1);
    double param[1];
    ffTransform->getParams(param);
    OCIO_CHECK_EQUAL(param[0], 0.5);
}

OCIO_ADD_TEST(FixedFunctionOps, RGB_TO_HSV)
{
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, OCIO::FixedFunctionOpData::RGB_TO_HSV, {}));
    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, OCIO::FixedFunctionOpData::HSV_TO_RGB, {}));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(!op0->isIdentity());
    OCIO_CHECK_ASSERT(!op1->isIdentity());

    OCIO_CHECK_ASSERT(op0->isSameType(op1));
    OCIO_CHECK_ASSERT(op0->isInverse(op1));
    OCIO_CHECK_ASSERT(op1->isInverse(op0));

    OCIO::ConstOpCPURcPtr cpuOp = op0->getCPUOp(false);
    const OCIO::OpCPU & c = *cpuOp;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "Renderer_RGB_TO_HSV"));
}

OCIO_ADD_TEST(FixedFunctionOps, XYZ_TO_xyY)
{
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, OCIO::FixedFunctionOpData::XYZ_TO_xyY, {}));
    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, OCIO::FixedFunctionOpData::xyY_TO_XYZ, {}));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(!op0->isIdentity());
    OCIO_CHECK_ASSERT(!op1->isIdentity());

    OCIO_CHECK_ASSERT(op0->isSameType(op1));
    OCIO_CHECK_ASSERT(op0->isInverse(op1));
    OCIO_CHECK_ASSERT(op1->isInverse(op0));

    OCIO::ConstOpCPURcPtr cpuOp = op0->getCPUOp(false);
    const OCIO::OpCPU & c = *cpuOp;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "Renderer_XYZ_TO_xyY"));
}

OCIO_ADD_TEST(FixedFunctionOps, XYZ_TO_uvY)
{
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, OCIO::FixedFunctionOpData::XYZ_TO_uvY, {}));
    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, OCIO::FixedFunctionOpData::uvY_TO_XYZ, {}));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(!op0->isIdentity());
    OCIO_CHECK_ASSERT(!op1->isIdentity());

    OCIO_CHECK_ASSERT(op0->isSameType(op1));
    OCIO_CHECK_ASSERT(op0->isInverse(op1));
    OCIO_CHECK_ASSERT(op1->isInverse(op0));

    OCIO::ConstOpCPURcPtr cpuOp = op0->getCPUOp(false);
    const OCIO::OpCPU & c = *cpuOp;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "Renderer_XYZ_TO_uvY"));
}

OCIO_ADD_TEST(FixedFunctionOps, XYZ_TO_LUV)
{
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, OCIO::FixedFunctionOpData::XYZ_TO_LUV, {}));
    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, OCIO::FixedFunctionOpData::LUV_TO_XYZ, {}));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(!op0->isIdentity());
    OCIO_CHECK_ASSERT(!op1->isIdentity());

    OCIO_CHECK_ASSERT(op0->isSameType(op1));
    OCIO_CHECK_ASSERT(op0->isInverse(op1));
    OCIO_CHECK_ASSERT(op1->isInverse(op0));

    OCIO::ConstOpCPURcPtr cpuOp = op0->getCPUOp(false);
    const OCIO::OpCPU & c = *cpuOp;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "Renderer_XYZ_TO_LUV"));
}
