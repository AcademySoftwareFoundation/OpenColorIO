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
#include "ilmbase/half.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "ops/Range/RangeOpCPU.h"
#include "ops/Range/RangeOpGPU.h"
#include "ops/Range/RangeOps.h"


OCIO_NAMESPACE_ENTER
{

namespace
{

class RangeOp;
typedef OCIO_SHARED_PTR<RangeOp> RangeOpRcPtr;
typedef OCIO_SHARED_PTR<const RangeOp> ConstRangeOpRcPtr;

class RangeOp : public Op
{
public:
    RangeOp() = delete;

    RangeOp(RangeOpDataRcPtr & range, TransformDirection direction);

    RangeOp(double minInValue, double maxInValue,
            double minOutValue, double maxOutValue,
            TransformDirection direction);

    virtual ~RangeOp();

    TransformDirection getDirection() const noexcept override { return m_direction; }

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    void finalize(FinalizationFlags fFlags) override;

    ConstOpCPURcPtr getCPUOp() const override;

    void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;

protected:
    ConstRangeOpDataRcPtr rangeData() const { return DynamicPtrCast<const RangeOpData>(data()); }
    RangeOpDataRcPtr rangeData() { return DynamicPtrCast<RangeOpData>(data()); }

private:            
    // The range direction
    TransformDirection m_direction;
};


RangeOp::RangeOp(RangeOpDataRcPtr & range, TransformDirection direction)
    :   Op()
    ,   m_direction(direction)
{
    if(m_direction == TRANSFORM_DIR_UNKNOWN)
    {
        throw Exception(
            "Cannot create RangeOp with unspecified transform direction.");
    }

    data() = range;
}

RangeOp::RangeOp(double minInValue, double maxInValue,
                 double minOutValue, double maxOutValue,
                 TransformDirection direction)
    :   Op()
    ,   m_direction(direction)
{
    if(m_direction == TRANSFORM_DIR_UNKNOWN)
    {
        throw Exception(
            "Cannot create RangeOp with unspecified transform direction.");
    }

    data().reset(new RangeOpData(BIT_DEPTH_F32, BIT_DEPTH_F32,
                                 minInValue, maxInValue,
                                 minOutValue, maxOutValue));
}

OpRcPtr RangeOp::clone() const
{
    RangeOpDataRcPtr clonedData = rangeData()->clone();
    return std::make_shared<RangeOp>(clonedData, m_direction);
}

RangeOp::~RangeOp()
{
}

std::string RangeOp::getInfo() const
{
    return "<RangeOp>";
}

bool RangeOp::isSameType(ConstOpRcPtr & op) const
{
    ConstRangeOpRcPtr typedRcPtr = DynamicPtrCast<const RangeOp>(op);
    return (bool)typedRcPtr;
}

bool RangeOp::isInverse(ConstOpRcPtr & op) const
{
    ConstRangeOpRcPtr typedRcPtr = DynamicPtrCast<const RangeOp>(op);
    if(!typedRcPtr) return false;

    if(GetInverseTransformDirection(m_direction)==typedRcPtr->m_direction)
    {
        return *rangeData()==*(typedRcPtr->rangeData());
    }

    ConstRangeOpDataRcPtr rangeOpData = typedRcPtr->rangeData();
    return rangeData()->isInverse(rangeOpData);
}

bool RangeOp::canCombineWith(ConstOpRcPtr & /*op*/) const
{
    // TODO: Implement RangeOp::canCombineWith()
    return false;
}

void RangeOp::combineWith(OpRcPtrVec & /*ops*/, ConstOpRcPtr & secondOp) const
{
    if(!canCombineWith(secondOp))
    {
        throw Exception("TODO: Range can't be combined.");
    }

    // TODO: Implement RangeOp::combineWith()
}

void RangeOp::finalize(FinalizationFlags /*fFlags*/)
{
    const RangeOp & constThis = *this;
    if(m_direction == TRANSFORM_DIR_INVERSE)
    {
        data() = constThis.rangeData()->inverse();
        m_direction = TRANSFORM_DIR_FORWARD;
    }

    // Only 32f processing is natively supported.
    rangeData()->setInputBitDepth(BIT_DEPTH_F32);
    rangeData()->setOutputBitDepth(BIT_DEPTH_F32);

    rangeData()->finalize();

    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<RangeOp ";
    cacheIDStream << constThis.rangeData()->getCacheID() << " ";
    cacheIDStream << TransformDirectionToString(m_direction) << " ";
    cacheIDStream << ">";
    
    m_cacheID = cacheIDStream.str();
}

ConstOpCPURcPtr RangeOp::getCPUOp() const
{
    ConstRangeOpDataRcPtr data = rangeData();
    return GetRangeRenderer(data);
}

void RangeOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
{
    if (m_direction != TRANSFORM_DIR_FORWARD)
    {
        throw Exception(
            "RangeOp direction should have been set to forward by finalize");
    }

    if(getInputBitDepth()!=BIT_DEPTH_F32 || getOutputBitDepth()!=BIT_DEPTH_F32)
    {
        throw Exception(
            "Only 32F bit depth is supported for the GPU shader");
    }

    ConstRangeOpDataRcPtr data = rangeData();
    GetRangeGPUShaderProgram(shaderDesc, data);
}

}  // Anon namespace




///////////////////////////////////////////////////////////////////////////




void CreateRangeOp(OpRcPtrVec & ops, 
                   double minInValue, double maxInValue,
                   double minOutValue, double maxOutValue)
{
    CreateRangeOp(ops, 
                  minInValue, maxInValue,
                  minOutValue, maxOutValue,
                  TRANSFORM_DIR_FORWARD);
}

void CreateRangeOp(OpRcPtrVec & ops, 
                   double minInValue, double maxInValue,
                   double minOutValue, double maxOutValue,
                   TransformDirection direction)
{
    RangeOpDataRcPtr rangeData =
        std::make_shared<RangeOpData>(BIT_DEPTH_F32, BIT_DEPTH_F32,
                                      minInValue, maxInValue, minOutValue, maxOutValue);

    CreateRangeOp(ops, rangeData, direction);
}

void CreateRangeOp(OpRcPtrVec & ops, RangeOpDataRcPtr & rangeData, TransformDirection direction)
{
    if (rangeData->isNoOp()) return;

    ops.push_back(std::make_shared<RangeOp>(rangeData, direction));
}

}
OCIO_NAMESPACE_EXIT



////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "ops/Matrix/MatrixOps.h"
#include "UnitTest.h"


OCIO_NAMESPACE_USING

const float g_error = 1e-7f;

OCIO_ADD_TEST(RangeOps, apply_arbitrary)
{
    OCIO::RangeOp r(-0.101, 0.95, 0.194, 1.001, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_NO_THROW(r.finalize(OCIO::FINALIZATION_EXACT));

    float image[4*3] = { -0.50f,  0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OCIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OCIO_CHECK_CLOSE(image[0],  0.194f,        g_error);
    OCIO_CHECK_CLOSE(image[1],  0.4635119438f, g_error);
    OCIO_CHECK_CLOSE(image[2],  0.6554719806f, g_error);
    OCIO_CHECK_CLOSE(image[3],  0.0f,          g_error);
    OCIO_CHECK_CLOSE(image[4],  0.8474320173f, g_error);
    OCIO_CHECK_CLOSE(image[5],  1.001f,        g_error);
    OCIO_CHECK_CLOSE(image[6],  1.001f,        g_error);
    OCIO_CHECK_CLOSE(image[7],  1.0f,          g_error);
    OCIO_CHECK_CLOSE(image[8],  1.001f,        g_error);
    OCIO_CHECK_CLOSE(image[9],  1.001f,        g_error);
    OCIO_CHECK_CLOSE(image[10], 1.001f,        g_error);
    OCIO_CHECK_CLOSE(image[11], 0.0f,          g_error);
}

OCIO_ADD_TEST(RangeOps, combining)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1.0);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
    OCIO::CreateRangeOp(ops, 0., 1., 0.5, 1.5);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

    OCIO::ConstOpRcPtr op1 = ops[1];

    // TODO: implement Range combine
    OCIO_CHECK_THROW_WHAT(ops[0]->combineWith(ops, op1), 
                          OCIO::Exception, "TODO: Range can't be combined");
    OCIO_CHECK_EQUAL(ops.size(), 2);

}

OCIO_ADD_TEST(RangeOps, combining_with_inverse)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 1., 0.5, 1.5);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
    OCIO::CreateRangeOp(ops, 0., 1., 0.5, 1.5, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

    OCIO::ConstOpRcPtr op1 = ops[1];

    // TODO: implement Range combine
    OCIO_CHECK_THROW_WHAT(ops[0]->combineWith(ops, op1), 
                          OCIO::Exception, "TODO: Range can't be combined");
    OCIO_CHECK_EQUAL(ops.size(), 2);

}

OCIO_ADD_TEST(RangeOps, is_inverse)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1.);
    OCIO_CHECK_EQUAL(ops.size(), 1);
    // Skip finalize so that inverse direction is kept
    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1., OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(ops.size(), 2);

    const float offset[] = { 1.1f, -1.3f, 0.3f, 0.0f };
    OCIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];

    OCIO_CHECK_ASSERT(ops[0]->isSameType(op1));
    OCIO_CHECK_ASSERT(!ops[0]->isSameType(op2));

    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op2));
    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op0));

    // Inverse based on Op direction

    OCIO_CHECK_ASSERT(ops[0]->isInverse(op1));

    OCIO::CreateRangeOp(ops, 0.000002, 0.5, 0.5, 1., OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    OCIO::ConstOpRcPtr op3 = ops[3];

    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op3));
    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op3));

    OCIO::CreateRangeOp(ops, 0.000002, 0.5, 0.5, 1.);
    OCIO_REQUIRE_EQUAL(ops.size(), 5);
    OCIO::ConstOpRcPtr op4 = ops[4];

    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op4));
    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op4));
    OCIO_CHECK_ASSERT(ops[3]->isInverse(op4));

    OCIO::CreateRangeOp(ops, 0.5, 1., 0.000002, 0.5, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_REQUIRE_EQUAL(ops.size(), 6);
    OCIO::ConstOpRcPtr op5 = ops[5];

    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op5));
    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op5));
    OCIO_CHECK_ASSERT(ops[3]->isInverse(op5));
    OCIO_CHECK_ASSERT(!ops[4]->isInverse(op5));
}

OCIO_ADD_TEST(RangeOps, computed_identifier)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1.0);
    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1.0);
    OCIO::CreateRangeOp(ops, 0.1, 1., 0.3, 1.9);
    OCIO::CreateRangeOp(ops, 0.1, 1., 0.3, 1.9, OCIO::TRANSFORM_DIR_INVERSE);
    for(OCIO::OpRcPtrVec::reference op : ops) { op->finalize(OCIO::FINALIZATION_EXACT); }

    OCIO_REQUIRE_EQUAL(ops.size(), 4);

    OCIO_CHECK_ASSERT(ops[0]->getCacheID() == ops[1]->getCacheID());
    OCIO_CHECK_ASSERT(ops[0]->getCacheID() != ops[2]->getCacheID());
    OCIO_CHECK_ASSERT(ops[1]->getCacheID() != ops[2]->getCacheID());
    OCIO_CHECK_ASSERT(ops[2]->getCacheID() != ops[3]->getCacheID());

    OCIO::CreateRangeOp(ops, 0.1, 1., 0.3, 1.90001);
    for(OCIO::OpRcPtrVec::reference op : ops) { op->finalize(OCIO::FINALIZATION_EXACT); }

    OCIO_REQUIRE_EQUAL(ops.size(), 5);
    OCIO_CHECK_ASSERT(ops[2]->getCacheID() != ops[4]->getCacheID());
    OCIO_CHECK_ASSERT(ops[3]->getCacheID() != ops[4]->getCacheID());
}

OCIO_ADD_TEST(RangeOps, bit_depth)
{
    // Test bit depths.

    OCIO::RangeOpDataRcPtr range
        = std::make_shared<OCIO::RangeOpData>(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT16,
                                              0., 255., -1., 65540.);

    OCIO_CHECK_EQUAL(range->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(range->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT16);

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO_CHECK_EQUAL(ops[0]->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(ops[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT16);

    // Test bit depths after a clone.

    OCIO::ConstOpRcPtr o = ops[0]->clone();
    OCIO_CHECK_EQUAL(o->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(o->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT16);

    OCIO::ConstRangeOpDataRcPtr r = DynamicPtrCast<const OCIO::RangeOpData>(o->data());
    OCIO_CHECK_EQUAL(r->getMinOutValue(), -1.);
}
#endif
