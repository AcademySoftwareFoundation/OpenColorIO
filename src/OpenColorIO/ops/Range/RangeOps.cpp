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
#include "HashUtils.h"
#include "MathUtils.h"
#include "RangeOpCPU.h"
#include "RangeOps.h"


// TODO: To be removed when ilmbase library will be in
#define HALF_MAX    65504.0

OCIO_NAMESPACE_ENTER
{

namespace
{

class RangeOp;
typedef OCIO_SHARED_PTR<RangeOp> RangeOpRcPtr;

class RangeOp : public Op
{
public:
    RangeOp();

    RangeOp(RangeOpDataRcPtr & range, TransformDirection direction);

    RangeOp(double minInValue, double maxInValue,
            double minOutValue, double maxOutValue,
            TransformDirection direction);

    virtual ~RangeOp();
    
    virtual OpRcPtr clone() const;
    
    virtual std::string getInfo() const;
    
    virtual bool isIdentity() const;
    virtual bool isSameType(const OpRcPtr & op) const;
    virtual bool isInverse(const OpRcPtr & op) const;
    virtual bool canCombineWith(const OpRcPtr & op) const;
    virtual void combineWith(OpRcPtrVec & ops, const OpRcPtr & secondOp) const;
    
    virtual void finalize();
    virtual void apply(float * rgbaBuffer, long numPixels) const;
    
    virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;

protected:
    const RangeOpDataRcPtr rangeData() const { return DynamicPtrCast<RangeOpData>(const_data()); }

private:            
    // The range direction
    TransformDirection m_direction;
    // The CPU processor
    OpCPURcPtr m_cpu;
};


RangeOp::RangeOp()
    :   Op()
    ,   m_direction(TRANSFORM_DIR_FORWARD)
    ,   m_cpu(new NoOpCPU)
{           
    data().reset(new RangeOpData());
}

RangeOp::RangeOp(RangeOpDataRcPtr & range, TransformDirection direction)
    :   Op()
    ,   m_direction(direction)
    ,   m_cpu(new NoOpCPU)
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
    ,   m_cpu(new NoOpCPU)
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
    return OpRcPtr(new RangeOp(rangeData()->getMinInValue(), 
                               rangeData()->getMaxInValue(),
                               rangeData()->getMinOutValue(), 
                               rangeData()->getMaxOutValue(),
                               m_direction));
}

RangeOp::~RangeOp()
{
}

std::string RangeOp::getInfo() const
{
    return "<RangeOp>";
}

bool RangeOp::isIdentity() const
{
    return rangeData()->isIdentity();
}

bool RangeOp::isSameType(const OpRcPtr & op) const
{
    const RangeOpRcPtr typedRcPtr = DynamicPtrCast<RangeOp>(op);
    return (bool)typedRcPtr;
}

bool RangeOp::isInverse(const OpRcPtr & op) const
{
    RangeOpRcPtr typedRcPtr = DynamicPtrCast<RangeOp>(op);
    if(!typedRcPtr) return false;

    if(GetInverseTransformDirection(m_direction)==typedRcPtr->m_direction)
    {
        return *rangeData()==*(typedRcPtr->rangeData());
    }

    return rangeData()->isInverse(typedRcPtr->rangeData());
}

bool RangeOp::canCombineWith(const OpRcPtr & /*op*/) const
{
    // TODO: Implement RangeOp::canCombineWith()
    return false;
}

void RangeOp::combineWith(OpRcPtrVec & /*ops*/, const OpRcPtr & secondOp) const
{
    if(!canCombineWith(secondOp))
    {
        throw Exception("TODO: Range can't be combined.");
    }

    // TODO: Implement RangeOp::combineWith()
}

void RangeOp::finalize()
{
    if(m_direction == TRANSFORM_DIR_INVERSE)
    {
        data() = rangeData()->inverse();
        m_direction = TRANSFORM_DIR_FORWARD;
    }

    // In this initial implementation, only 32f processing is
    // natively supported.
    rangeData()->setInputBitDepth(BIT_DEPTH_F32);
    rangeData()->setOutputBitDepth(BIT_DEPTH_F32);

    rangeData()->validate();
    rangeData()->finalize();

    m_cpu = RangeOpCPU::GetRenderer(rangeData());

    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<RangeOp ";
    cacheIDStream << rangeData()->getCacheID() << " ";
    cacheIDStream << TransformDirectionToString(m_direction) << " ";
    cacheIDStream << ">";
    
    m_cacheID = cacheIDStream.str();
}

void RangeOp::apply(float * rgbaBuffer, long numPixels) const
{
    m_cpu->apply(rgbaBuffer, numPixels);
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

    GpuShaderText ss(shaderDesc->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add Range processing";
    ss.newLine() << "";

    if(rangeData()->scales(true))
    {
        const double scale[4] 
            = { rangeData()->getScale(), 
                rangeData()->getScale(), 
                rangeData()->getScale(), 
                rangeData()->getAlphaScale() };

        const double offset[4] 
            = { rangeData()->getOffset(), 
                rangeData()->getOffset(), 
                rangeData()->getOffset(), 
                0. };

        ss.newLine() << shaderDesc->getPixelName() << " = "
                     << shaderDesc->getPixelName()
                     << " * "
                     << ss.vec4fConst(scale[0], scale[1], scale[2], scale[3])
                     << " + "
                     << ss.vec4fConst(offset[0], offset[1], offset[2], offset[3])
                     << ";";
    }

    if(rangeData()->minClips())
    {
        const double lowerBound[4] 
            = { rangeData()->getLowBound(), 
                rangeData()->getLowBound(), 
                rangeData()->getLowBound(), 
                -1. * HALF_MAX };

        ss.newLine() << shaderDesc->getPixelName() << " = "
                     << "max(" << shaderDesc->getPixelName() << ", "
                     << ss.vec4fConst(lowerBound[0], lowerBound[1], lowerBound[2], lowerBound[3])
                     << ");";
    }

    if(rangeData()->maxClips())
    {
        const double upperBound[4] 
            = { rangeData()->getHighBound(), 
                rangeData()->getHighBound(), 
                rangeData()->getHighBound(), 
                HALF_MAX };

        ss.newLine() << shaderDesc->getPixelName() << " = "
                     << "min(" << shaderDesc->getPixelName() << ", "
                     << ss.vec4fConst(upperBound[0], upperBound[1], upperBound[2], upperBound[3])
                     << ");";
    }

    shaderDesc->addToFunctionShaderCode(ss.string().c_str());
}

}  // Anon namespace




///////////////////////////////////////////////////////////////////////////




void CreateRangeOp(OpRcPtrVec & ops, RangeOpDataRcPtr & rangeData, TransformDirection direction)
{
    if(rangeData->isNoOp()) return;

    ops.push_back(RangeOpRcPtr(new RangeOp(rangeData, direction)));
}


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
    RangeOpDataRcPtr rangeData(
        new RangeOpData(BIT_DEPTH_F32, BIT_DEPTH_F32,
                        minInValue, maxInValue, minOutValue, maxOutValue));

    CreateRangeOp(ops, rangeData, direction);
}

}
OCIO_NAMESPACE_EXIT



////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "ops/Matrix/MatrixOps.h"
#include "unittest.h"


OCIO_NAMESPACE_USING

const float g_error = 1e-7f;


OIIO_ADD_TEST(RangeOps, identity)
{
    OCIO::RangeOp r;

    float image[4*3] = { -0.50f, -0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(r.finalize());
    OIIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OIIO_CHECK_CLOSE(image[0],  -0.50f, g_error);
    OIIO_CHECK_CLOSE(image[1],  -0.25f, g_error);
    OIIO_CHECK_CLOSE(image[2],   0.50f, g_error);
    OIIO_CHECK_CLOSE(image[3],   0.00f, g_error);
    OIIO_CHECK_CLOSE(image[4],   0.75f, g_error);
    OIIO_CHECK_CLOSE(image[5],   1.00f, g_error);
    OIIO_CHECK_CLOSE(image[6],   1.25f, g_error);
    OIIO_CHECK_CLOSE(image[7],   1.00f, g_error);
    OIIO_CHECK_CLOSE(image[8],   1.25f, g_error);
    OIIO_CHECK_CLOSE(image[9],   1.50f, g_error);
    OIIO_CHECK_CLOSE(image[10],  1.75f, g_error);
    OIIO_CHECK_CLOSE(image[11],  0.00f, g_error);
}

OIIO_ADD_TEST(RangeOps, scale_with_low_and_high_clippings)
{
    OCIO::RangeOp r(0.0f, 1.0f, 0.5f, 1.5f, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_NO_THROW(r.finalize());

    float image[4*3] = { -0.50f, -0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OIIO_CHECK_CLOSE(image[0],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[1],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[2],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.00f, g_error);
    OIIO_CHECK_CLOSE(image[4],  1.25f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[8],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[9],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[10], 1.50f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OIIO_ADD_TEST(RangeOps, scale_with_low_clipping)
{
    OCIO::RangeOp r(0.0f, OCIO::RangeOpData::EmptyValue(), 
                    0.5f, OCIO::RangeOpData::EmptyValue(), 
                    OCIO::TRANSFORM_DIR_FORWARD);

    OIIO_CHECK_NO_THROW(r.finalize());

    float image[4*3] = { -0.50f, -0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OIIO_CHECK_CLOSE(image[0],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[1],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[2],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.00f, g_error);
    OIIO_CHECK_CLOSE(image[4],  1.25f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.75f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[8],  1.75f, g_error);
    OIIO_CHECK_CLOSE(image[9],  2.00f, g_error);
    OIIO_CHECK_CLOSE(image[10], 2.25f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OIIO_ADD_TEST(RangeOps, scale_with_high_clipping)
{
    OCIO::RangeOp r(OCIO::RangeOpData::EmptyValue(), 1.0f, 
                    OCIO::RangeOpData::EmptyValue(), 1.5f, 
                    OCIO::TRANSFORM_DIR_FORWARD);

    OIIO_CHECK_NO_THROW(r.finalize());

    float image[4*3] = { -0.50f, -0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OIIO_CHECK_CLOSE(image[0],  0.00f, g_error);
    OIIO_CHECK_CLOSE(image[1],  0.25f, g_error);
    OIIO_CHECK_CLOSE(image[2],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.00f, g_error);
    OIIO_CHECK_CLOSE(image[4],  1.25f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[8],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[9],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[10], 1.50f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OIIO_ADD_TEST(RangeOps, scale_with_low_and_high_clippings_2)
{
    OCIO::RangeOp r(0.0f, 1.0f, 0.0f, 1.5f, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_NO_THROW(r.finalize());

    float image[4*3] = { -0.50f, -0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OIIO_CHECK_CLOSE(image[0],  0.000f, g_error);
    OIIO_CHECK_CLOSE(image[1],  0.000f, g_error);
    OIIO_CHECK_CLOSE(image[2],  0.750f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.000f, g_error);
    OIIO_CHECK_CLOSE(image[4],  1.125f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.500f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.500f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.000f, g_error);
    OIIO_CHECK_CLOSE(image[8],  1.500f, g_error);
    OIIO_CHECK_CLOSE(image[9],  1.500f, g_error);
    OIIO_CHECK_CLOSE(image[10], 1.500f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.000f, g_error);
}

OIIO_ADD_TEST(RangeOps, apply_arbitrary)
{
    OCIO::RangeOp r(-0.101f, 0.950f, 0.194f, 1.001f, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_NO_THROW(r.finalize());

    float image[4*3] = { -0.50f,  0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OIIO_CHECK_CLOSE(image[0],  0.194f,        g_error);
    OIIO_CHECK_CLOSE(image[1],  0.4635119438f, g_error);
    OIIO_CHECK_CLOSE(image[2],  0.6554719806f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.0f,          g_error);
    OIIO_CHECK_CLOSE(image[4],  0.8474320173f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.001f,        g_error);
    OIIO_CHECK_CLOSE(image[6],  1.001f,        g_error);
    OIIO_CHECK_CLOSE(image[7],  1.0f,          g_error);
    OIIO_CHECK_CLOSE(image[8],  1.001f,        g_error);
    OIIO_CHECK_CLOSE(image[9],  1.001f,        g_error);
    OIIO_CHECK_CLOSE(image[10], 1.001f,        g_error);
    OIIO_CHECK_CLOSE(image[11], 0.0f,          g_error);
}

OIIO_ADD_TEST(RangeOps, combining)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0.0f, 0.5f, 0.5f, 1.0f);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_NO_THROW(ops[0]->finalize());
    OCIO::CreateRangeOp(ops, 0.0f, 1.0f, 0.5f, 1.5f);
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    // TODO: implement Range combine
    OIIO_CHECK_THROW_WHAT(ops[0]->combineWith(ops, ops[1]), 
                          OCIO::Exception, "TODO: Range can't be combined");
    OIIO_CHECK_EQUAL(ops.size(), 2);

}

OIIO_ADD_TEST(RangeOps, combining_with_inverse)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 1., 0.5, 1.5);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_NO_THROW(ops[0]->finalize());
    OCIO::CreateRangeOp(ops, 0., 1., 0.5, 1.5, OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    // TODO: implement Range combine
    OIIO_CHECK_THROW_WHAT(ops[0]->combineWith(ops, ops[1]), 
                          OCIO::Exception, "TODO: Range can't be combined");
    OIIO_CHECK_EQUAL(ops.size(), 2);

}

OIIO_ADD_TEST(RangeOps, is_inverse)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1., OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    // Skip finalize so that inverse direction is kept
    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1., OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 2);

    const float offset[] = { 1.1f, -1.3f, 0.3f, 0.0f };
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 3);

    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[1]));
    OIIO_CHECK_ASSERT(!ops[0]->isSameType(ops[2]));

    OIIO_CHECK_ASSERT(!ops[0]->isInverse(ops[2]));
    OIIO_CHECK_ASSERT(!ops[0]->isInverse(ops[0]));

    // Inverse based on Op direction

    OIIO_CHECK_ASSERT(ops[0]->isInverse(ops[1]));

    OCIO::CreateRangeOp(ops, 0.000002, 0.5, 0.5, 1.0, OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 4);

    OIIO_CHECK_ASSERT(!ops[0]->isInverse(ops[3]));
    OIIO_CHECK_ASSERT(!ops[2]->isInverse(ops[3]));

    OCIO::CreateRangeOp(ops, 0.000002, 0.5, 0.5, 1.0, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 5);

    OIIO_CHECK_ASSERT(!ops[0]->isInverse(ops[4]));
    OIIO_CHECK_ASSERT(!ops[2]->isInverse(ops[4]));
    OIIO_CHECK_ASSERT(ops[3]->isInverse(ops[4]));

    OCIO::CreateRangeOp(ops, 0.5, 1., 0.000002, 0.5, OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 6);

    OIIO_CHECK_ASSERT(!ops[0]->isInverse(ops[5]));
    OIIO_CHECK_ASSERT(!ops[2]->isInverse(ops[5]));
    OIIO_CHECK_ASSERT(ops[3]->isInverse(ops[5]));
    OIIO_CHECK_ASSERT(!ops[4]->isInverse(ops[5]));
}

OIIO_ADD_TEST(RangeOps, computed_identifier)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1.0, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1.0, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateRangeOp(ops, 0.1, 1., 0.3, 1.9, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateRangeOp(ops, 0.1, 1., 0.3, 1.9, OCIO::TRANSFORM_DIR_INVERSE);
    for(OCIO::OpRcPtrVec::reference op : ops) { op->finalize(); }

    OIIO_REQUIRE_EQUAL(ops.size(), 4);

    OIIO_CHECK_ASSERT(ops[0]->getCacheID() == ops[1]->getCacheID());
    OIIO_CHECK_ASSERT(ops[0]->getCacheID() != ops[2]->getCacheID());
    OIIO_CHECK_ASSERT(ops[1]->getCacheID() != ops[2]->getCacheID());
    OIIO_CHECK_ASSERT(ops[2]->getCacheID() != ops[3]->getCacheID());

    OCIO::CreateRangeOp(ops, 0.1f, 1.0f, 0.3f, 1.90001f, OCIO::TRANSFORM_DIR_FORWARD);
    for(OCIO::OpRcPtrVec::reference op : ops) { op->finalize(); }

    OIIO_REQUIRE_EQUAL(ops.size(), 5);
    OIIO_CHECK_ASSERT(ops[2]->getCacheID() != ops[4]->getCacheID());
    OIIO_CHECK_ASSERT(ops[3]->getCacheID() != ops[4]->getCacheID());
}

#endif
