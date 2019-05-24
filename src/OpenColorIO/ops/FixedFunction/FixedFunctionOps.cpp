/*
Copyright (c) 2018 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


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
    FixedFunctionOp(FixedFunctionOpDataRcPtr & func);

    virtual ~FixedFunctionOp();

    virtual OpRcPtr clone() const override;

    virtual std::string getInfo() const override;

    virtual bool isIdentity() const override;
    virtual bool isSameType(ConstOpRcPtr & op) const override;
    virtual bool isInverse(ConstOpRcPtr & op) const override;
    virtual bool canCombineWith(ConstOpRcPtr & op) const override;
    virtual void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    virtual void finalize() override;

    virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;

protected:
    ConstFixedFunctionOpDataRcPtr fnData() const { return DynamicPtrCast<const FixedFunctionOpData>(data()); }
    FixedFunctionOpDataRcPtr fnData() { return DynamicPtrCast<FixedFunctionOpData>(data()); }

    FixedFunctionOp() = delete;
    FixedFunctionOp(const FixedFunctionOp &) = delete;
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

void FixedFunctionOp::finalize()
{
    // In this initial implementation, only 32f processing is natively supported.
    fnData()->setInputBitDepth(BIT_DEPTH_F32);
    fnData()->setOutputBitDepth(BIT_DEPTH_F32);

    fnData()->validate();
    fnData()->finalize();

    const FixedFunctionOp & constThis = *this;
    ConstFixedFunctionOpDataRcPtr fnOpData = constThis.fnData();
    m_cpuOp = GetFixedFunctionCPURenderer(fnOpData);

    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<FixedFunctionOp ";
    cacheIDStream << fnData()->getCacheID() << " ";
    cacheIDStream << ">";
    
    m_cacheID = cacheIDStream.str();
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
    if(funcData->isNoOp()) return;

    auto func = funcData;
    if (direction == TRANSFORM_DIR_INVERSE)
    {
        func = func->inverse();
    }

    ops.push_back(std::make_shared<FixedFunctionOp>(func));
}



}
OCIO_NAMESPACE_EXIT



////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "MathUtils.h"
#include "pystring/pystring.h"
#include "unittest.h"


OIIO_ADD_TEST(FixedFunctionOps, basic)
{
    OCIO::OpRcPtrVec ops;
    OCIO::FixedFunctionOpData::Params params;

    OIIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, params, 
                                                    OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD));

    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstFixedFunctionOpRcPtr func
        = OCIO::DynamicPtrCast<OCIO::FixedFunctionOp>(ops[0]);

    OIIO_REQUIRE_EQUAL(func->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_REQUIRE_EQUAL(func->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_ASSERT(!func->isNoOp());
    OIIO_CHECK_ASSERT(!func->isIdentity());

    OCIO::ConstFixedFunctionOpDataRcPtr funcData 
        = OCIO::DynamicPtrCast<const OCIO::FixedFunctionOpData>(func->data());
    OIIO_REQUIRE_EQUAL(funcData->getStyle(), OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD);
    OIIO_REQUIRE_ASSERT(funcData->getParams() == params);
}

OIIO_ADD_TEST(FixedFunctionOps, glow03_cpu_engine)
{
    // Validate that the right CPU OP is created.

    const OCIO::FixedFunctionOpData::Style style = OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD;
    const OCIO::FixedFunctionOpData::Params data;

    OCIO::FixedFunctionOpDataRcPtr funcData 
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                                data, style);

    OCIO::FixedFunctionOp func(funcData);
    OIIO_CHECK_NO_THROW(func.finalize());

    OCIO::ConstOpCPURcPtr cpuOp = func.getCPUOp();
    const OCIO::OpCPU & c = *cpuOp;
    const std::string typeName(typeid(c).name());
    OIIO_CHECK_NE(-1, OCIO::pystring::find(typeName, "Renderer_ACES_Glow03_Fwd"));
}

OIIO_ADD_TEST(FixedFunctionOps, darktodim10_cpu_engine)
{
    // Validate that the right CPU OP is created.

    const OCIO::FixedFunctionOpData::Style style = OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD;
    const OCIO::FixedFunctionOpData::Params data;

    OCIO::FixedFunctionOpDataRcPtr funcData 
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                                      data, style);

    OCIO::FixedFunctionOp func(funcData);
    OIIO_CHECK_NO_THROW(func.finalize());

    OCIO::ConstOpCPURcPtr cpuOp = func.getCPUOp();
    const OCIO::OpCPU & c = *cpuOp;
    const std::string typeName(typeid(c).name());
    OIIO_CHECK_NE(-1, OCIO::pystring::find(typeName, "Renderer_ACES_DarkToDim10_Fwd"));
}

OIIO_ADD_TEST(FixedFunctionOps, aces_red_mod_inv)
{
    OCIO::OpRcPtrVec ops;

    OIIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_RED_MOD_03_INV));

    OIIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_RED_MOD_03_FWD));

    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops, false));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OIIO_CHECK_ASSERT(!op0->isIdentity());
    OIIO_CHECK_ASSERT(!op1->isIdentity());

    OIIO_CHECK_ASSERT(op0->isSameType(op1));
    OIIO_CHECK_ASSERT(op0->isInverse(op1));
    OIIO_CHECK_ASSERT(op1->isInverse(op0));
}

OIIO_ADD_TEST(FixedFunctionOps, aces_glow_inv)
{
    OCIO::OpRcPtrVec ops;

    OIIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_GLOW_03_INV));

    OIIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD));

    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops, false));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OIIO_CHECK_ASSERT(!op0->isIdentity());
    OIIO_CHECK_ASSERT(!op1->isIdentity());

    OIIO_CHECK_ASSERT(op0->isSameType(op1));
    OIIO_CHECK_ASSERT(op0->isInverse(op1));
    OIIO_CHECK_ASSERT(op1->isInverse(op0));
}

OIIO_ADD_TEST(FixedFunctionOps, aces_darktodim10_inv)
{
    OCIO::OpRcPtrVec ops;

    OIIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV));

    OIIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, {}, 
                                                    OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD));

    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops, false));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OIIO_CHECK_ASSERT(!op0->isIdentity());
    OIIO_CHECK_ASSERT(!op1->isIdentity());

    OIIO_CHECK_ASSERT(op0->isSameType(op1));
    OIIO_CHECK_ASSERT(op0->isInverse(op1));
    OIIO_CHECK_ASSERT(op1->isInverse(op0));
}

OIIO_ADD_TEST(FixedFunctionOps, rec2100_surround_inv)
{
    OCIO::OpRcPtrVec ops;

    OIIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, { 2. }, 
                                                    OCIO::FixedFunctionOpData::REC2100_SURROUND));

    OIIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, { 1. / 2. }, 
                                                    OCIO::FixedFunctionOpData::REC2100_SURROUND));

    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops, false));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    {
        OCIO::ConstOpRcPtr op0 = ops[0];
        OCIO::ConstOpRcPtr op1 = ops[1];

        OIIO_CHECK_ASSERT(!op0->isIdentity());
        OIIO_CHECK_ASSERT(!op1->isIdentity());

        OIIO_CHECK_ASSERT(op0->isSameType(op1));
        OIIO_CHECK_ASSERT(op0->isInverse(op1));
        OIIO_CHECK_ASSERT(op1->isInverse(op0));
    }
    OIIO_CHECK_NO_THROW(OCIO::CreateFixedFunctionOp(ops, { 2.01 }, 
                                                    OCIO::FixedFunctionOpData::REC2100_SURROUND));

    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops, false));
    OIIO_REQUIRE_EQUAL(ops.size(), 3);
    {
        OCIO::ConstOpRcPtr op0 = ops[0];
        OCIO::ConstOpRcPtr op1 = ops[1];
        OCIO::ConstOpRcPtr op2 = ops[2];

        OIIO_CHECK_ASSERT(!op0->isInverse(op2));
        OIIO_CHECK_ASSERT(!op1->isInverse(op2));
    }
}

#endif
