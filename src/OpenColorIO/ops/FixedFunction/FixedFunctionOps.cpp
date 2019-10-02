// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/FixedFunction/FixedFunctionOpCPU.h"
#include "ops/FixedFunction/FixedFunctionOpGPU.h"
#include "ops/FixedFunction/FixedFunctionOps.h"


OCIO_NAMESPACE_ENTER
{

namespace
{

class FixedFunctionOp;
typedef OCIO_SHARED_PTR<FixedFunctionOp> FixedFunctionOpRcPtr;
typedef OCIO_SHARED_PTR<const FixedFunctionOp> ConstFixedFunctionOpRcPtr;

class FixedFunctionOp : public Op
{
public:
    FixedFunctionOp() = delete;
    FixedFunctionOp(const FixedFunctionOp &) = delete;
    explicit FixedFunctionOp(FixedFunctionOpDataRcPtr & func);

    virtual ~FixedFunctionOp();

    TransformDirection getDirection() const noexcept override { return TRANSFORM_DIR_FORWARD; }

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isIdentity() const override;
    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    void finalize(FinalizationFlags fFlags) override;

    ConstOpCPURcPtr getCPUOp() const override;

    void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;

protected:
    ConstFixedFunctionOpDataRcPtr fnData() const { return DynamicPtrCast<const FixedFunctionOpData>(data()); }
    FixedFunctionOpDataRcPtr fnData() { return DynamicPtrCast<FixedFunctionOpData>(data()); }
};


FixedFunctionOp::FixedFunctionOp(FixedFunctionOpDataRcPtr & func)
    :   Op()
{
    data() = func;
}

OpRcPtr FixedFunctionOp::clone() const
{
    FixedFunctionOpDataRcPtr f = fnData()->clone();
    return std::make_shared<FixedFunctionOp>(f);
}

FixedFunctionOp::~FixedFunctionOp()
{
}

std::string FixedFunctionOp::getInfo() const
{
    return "<FixedFunctionOp>";
}

bool FixedFunctionOp::isIdentity() const
{
    return fnData()->isIdentity();
}

bool FixedFunctionOp::isSameType(ConstOpRcPtr & op) const
{
    ConstFixedFunctionOpRcPtr typedRcPtr = DynamicPtrCast<const FixedFunctionOp>(op);
    return (bool)typedRcPtr;
}

bool FixedFunctionOp::isInverse(ConstOpRcPtr & op) const
{
    ConstFixedFunctionOpRcPtr typedRcPtr = DynamicPtrCast<const FixedFunctionOp>(op);
    if(!typedRcPtr) return false;

    ConstFixedFunctionOpDataRcPtr fnOpData = typedRcPtr->fnData();
    return fnData()->isInverse(fnOpData);
}

bool FixedFunctionOp::canCombineWith(ConstOpRcPtr & /*op*/) const
{
    return false;
}

void FixedFunctionOp::combineWith(OpRcPtrVec & /*ops*/, ConstOpRcPtr & secondOp) const
{
    if(!canCombineWith(secondOp))
    {
        throw Exception("FixedFunction can't be combined.");
    }
}

void FixedFunctionOp::finalize(FinalizationFlags /*fFlags*/)
{
    fnData()->finalize();

    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<FixedFunctionOp ";
    cacheIDStream << fnData()->getCacheID() << " ";
    cacheIDStream << ">";
    
    m_cacheID = cacheIDStream.str();
}

ConstOpCPURcPtr FixedFunctionOp::getCPUOp() const
{
    ConstFixedFunctionOpDataRcPtr data = fnData();
    return GetFixedFunctionCPURenderer(data);
}

void FixedFunctionOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
{
    if(getInputBitDepth()!=BIT_DEPTH_F32 || getOutputBitDepth()!=BIT_DEPTH_F32)
    {
        throw Exception("Only 32F bit depth is supported for the GPU shader");
    }

    GpuShaderText ss(shaderDesc->getLanguage());
    ss.indent();

    ConstFixedFunctionOpDataRcPtr fnOpData = fnData();
    GetFixedFunctionGPUShaderProgram(ss, fnOpData);

    ss.dedent();

    shaderDesc->addToFunctionShaderCode(ss.string().c_str());
}


}  // Anon namespace




///////////////////////////////////////////////////////////////////////////


void CreateFixedFunctionOp(OpRcPtrVec & ops, 
                           const FixedFunctionOpData::Params & params,
                           FixedFunctionOpData::Style style)
{
    FixedFunctionOpDataRcPtr funcData 
        = std::make_shared<FixedFunctionOpData>(BIT_DEPTH_F32, BIT_DEPTH_F32,
                                                params, style);
    CreateFixedFunctionOp(ops, funcData, TRANSFORM_DIR_FORWARD);
}

void CreateFixedFunctionOp(OpRcPtrVec & ops, 
                           FixedFunctionOpDataRcPtr & funcData,
                           TransformDirection direction)
{
    auto func = funcData;
    if (direction == TRANSFORM_DIR_INVERSE)
    {
        func = func->inverse();
    }

    ops.push_back(std::make_shared<FixedFunctionOp>(func));
}

///////////////////////////////////////////////////////////////////////////

void CreateFixedFunctionTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto ff = DynamicPtrCast<const FixedFunctionOp>(op);
    if (!ff)
    {
        throw Exception("CreateFixedFunctionTransform: op has to be a FixedFunctionOp");
    }
    auto ffData = DynamicPtrCast<const FixedFunctionOpData>(op->data());
    auto ffTransform = FixedFunctionTransform::Create();

    const auto style = ffData->getStyle();

    if (style == FixedFunctionOpData::ACES_RED_MOD_03_INV ||
        style == FixedFunctionOpData::ACES_RED_MOD_10_INV ||
        style == FixedFunctionOpData::ACES_GLOW_03_INV ||
        style == FixedFunctionOpData::ACES_GLOW_10_INV ||
        style == FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV)
    {
        ffTransform->setDirection(TRANSFORM_DIR_INVERSE);
    }
    const auto transformStyle = FixedFunctionOpData::ConvertStyle(style);
    ffTransform->setStyle(transformStyle);

    auto & formatMetadata = ffTransform->getFormatMetadata();
    auto & metadata = dynamic_cast<FormatMetadataImpl &>(formatMetadata);
    metadata = ffData->getFormatMetadata();

    auto & params = ffData->getParams();
    ffTransform->setParams(params.data(), params.size());

    group->push_back(ffTransform);
}

void BuildFixedFunctionOps(OpRcPtrVec & ops,
                           const Config & /*config*/,
                           const ConstContextRcPtr & /*context*/,
                           const FixedFunctionTransform & transform,
                           TransformDirection dir)
{
    const TransformDirection combinedDir 
        = CombineTransformDirections(dir, transform.getDirection());

    const size_t numParams = transform.getNumParams();
    FixedFunctionOpData::Params params(numParams, 0.);
    if(numParams>0) transform.getParams(&params[0]);

    const auto style = FixedFunctionOpData::ConvertStyle(transform.getStyle());

    auto funcData = std::make_shared<FixedFunctionOpData>(
        BIT_DEPTH_F32, BIT_DEPTH_F32,
        params, style);

    CreateFixedFunctionOp(ops, funcData, combinedDir);
}


}
OCIO_NAMESPACE_EXIT



////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "MathUtils.h"
#include "pystring/pystring.h"
#include "UnitTest.h"

OCIO_ADD_TEST(FixedFunctionOps, basic)
{
    OCIO::OpRcPtrVec ops;
    OCIO::FixedFunctionOpData::Params params;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, params, 
                                                    OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD));

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstFixedFunctionOpRcPtr func
        = OCIO::DynamicPtrCast<OCIO::FixedFunctionOp>(ops[0]);

    OCIO_REQUIRE_EQUAL(func->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_REQUIRE_EQUAL(func->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OCIO_CHECK_ASSERT(!func->isNoOp());
    OCIO_CHECK_ASSERT(!func->isIdentity());

    OCIO::ConstFixedFunctionOpDataRcPtr funcData 
        = OCIO::DynamicPtrCast<const OCIO::FixedFunctionOpData>(func->data());
    OCIO_REQUIRE_EQUAL(funcData->getStyle(), OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD);
    OCIO_REQUIRE_ASSERT(funcData->getParams() == params);
}

OCIO_ADD_TEST(FixedFunctionOps, glow03_cpu_engine)
{
    // Validate that the right CPU OP is created.

    const OCIO::FixedFunctionOpData::Style style = OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD;
    const OCIO::FixedFunctionOpData::Params data;

    OCIO::FixedFunctionOpDataRcPtr funcData 
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                                data, style);

    OCIO::FixedFunctionOp func(funcData);
    OCIO_CHECK_NO_THROW(func.finalize(OCIO::FINALIZATION_EXACT));

    OCIO::ConstOpCPURcPtr cpuOp = func.getCPUOp();
    const OCIO::OpCPU & c = *cpuOp;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(-1, pystring::find(typeName, "Renderer_ACES_Glow03_Fwd"));
}

OCIO_ADD_TEST(FixedFunctionOps, darktodim10_cpu_engine)
{
    // Validate that the right CPU OP is created.

    const OCIO::FixedFunctionOpData::Style style = OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD;
    const OCIO::FixedFunctionOpData::Params data;

    OCIO::FixedFunctionOpDataRcPtr funcData 
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                                      data, style);

    OCIO::FixedFunctionOp func(funcData);
    OCIO_CHECK_NO_THROW(func.finalize(OCIO::FINALIZATION_EXACT));

    OCIO::ConstOpCPURcPtr cpuOp = func.getCPUOp();
    const OCIO::OpCPU & c = *cpuOp;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(-1, pystring::find(typeName, "Renderer_ACES_DarkToDim10_Fwd"));
}

OCIO_ADD_TEST(FixedFunctionOps, aces_red_mod_inv)
{
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_RED_MOD_03_INV));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_RED_MOD_03_FWD));

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(!op0->isIdentity());
    OCIO_CHECK_ASSERT(!op1->isIdentity());

    OCIO_CHECK_ASSERT(op0->isSameType(op1));
    OCIO_CHECK_ASSERT(op0->isInverse(op1));
    OCIO_CHECK_ASSERT(op1->isInverse(op0));
}

OCIO_ADD_TEST(FixedFunctionOps, aces_glow_inv)
{
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_GLOW_03_INV));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD));

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(!op0->isIdentity());
    OCIO_CHECK_ASSERT(!op1->isIdentity());

    OCIO_CHECK_ASSERT(op0->isSameType(op1));
    OCIO_CHECK_ASSERT(op0->isInverse(op1));
    OCIO_CHECK_ASSERT(op1->isInverse(op0));
}

OCIO_ADD_TEST(FixedFunctionOps, aces_darktodim10_inv)
{
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD));

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(!op0->isIdentity());
    OCIO_CHECK_ASSERT(!op1->isIdentity());

    OCIO_CHECK_ASSERT(op0->isSameType(op1));
    OCIO_CHECK_ASSERT(op0->isInverse(op1));
    OCIO_CHECK_ASSERT(op1->isInverse(op0));
}

OCIO_ADD_TEST(FixedFunctionOps, rec2100_surround_inv)
{
    OCIO::OpRcPtrVec ops;

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, { 2. }, 
                                                    OCIO::FixedFunctionOpData::REC2100_SURROUND));

    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, { 1. / 2. }, 
                                                    OCIO::FixedFunctionOpData::REC2100_SURROUND));

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
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

    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    {
        OCIO::ConstOpRcPtr op0 = ops[0];
        OCIO::ConstOpRcPtr op1 = ops[1];
        OCIO::ConstOpRcPtr op2 = ops[2];

        OCIO_CHECK_ASSERT(!op0->isInverse(op2));
        OCIO_CHECK_ASSERT(!op1->isInverse(op2));
    }
}

OCIO_ADD_TEST(FixedFunctionOps, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    const OCIO::FixedFunctionOpData::Params data{ 2.01 };
    const OCIO::FixedFunctionOpData::Style style = OCIO::FixedFunctionOpData::REC2100_SURROUND;

    OCIO::FixedFunctionOpDataRcPtr funcData
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::BIT_DEPTH_UINT10,
                                                      OCIO::BIT_DEPTH_F32,
                                                      data, style);
    funcData->getFormatMetadata().addAttribute("name", "test");

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, funcData, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateFixedFunctionTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->size(), 1);
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

#endif
