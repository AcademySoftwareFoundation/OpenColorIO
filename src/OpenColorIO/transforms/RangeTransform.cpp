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

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "OpBuilders.h"
#include "ops/Range/RangeOpData.h"
#include "ops/Range/RangeOps.h"


OCIO_NAMESPACE_ENTER
{

RangeTransformRcPtr RangeTransform::Create()
{
    return RangeTransformRcPtr(new RangeTransform(), &deleter);
}

void RangeTransform::deleter(RangeTransform* t)
{
    delete t;
}

class RangeTransform::Impl : public RangeOpData
{
public:
    Impl() 
        :   RangeOpData()
        ,   m_direction(TRANSFORM_DIR_FORWARD)
        ,   m_style(RANGE_CLAMP)
    {
    }

    Impl(const Impl &) = delete;

    ~Impl() {}

    Impl& operator=(const Impl & rhs)
    {
        if(this!=&rhs)
        {
            RangeOpData::operator=(rhs);
            m_direction  = rhs.m_direction;
            m_style      = rhs.m_style;
        }
        return *this;
    }

    bool equals(const Impl & rhs) const
    {
        if(this==&rhs) return true;

        return RangeOpData::operator==(rhs)
            && m_direction == rhs.m_direction
            && m_style     == rhs.m_style;
    }

    TransformDirection m_direction;
    RangeStyle         m_style;
};

///////////////////////////////////////////////////////////////////////////



RangeTransform::RangeTransform()
    : m_impl(new RangeTransform::Impl)
{
}

TransformRcPtr RangeTransform::createEditableCopy() const
{
    RangeTransformRcPtr transform = RangeTransform::Create();
    *transform->m_impl = *m_impl;
    return transform;
}

RangeTransform::~RangeTransform()
{
    delete m_impl;
    m_impl = NULL;
}

RangeTransform& RangeTransform::operator= (const RangeTransform & rhs)
{
    if (this != &rhs)
    {
        *m_impl = *rhs.m_impl;
    }
    return *this;
}

TransformDirection RangeTransform::getDirection() const
{
    return getImpl()->m_direction;
}

void RangeTransform::setDirection(TransformDirection dir)
{
    getImpl()->m_direction = dir;
}

RangeStyle RangeTransform::getStyle() const
{
    return getImpl()->m_style;
}

void RangeTransform::setStyle(RangeStyle style)
{
    getImpl()->m_style = style;
}

void RangeTransform::validate() const
{
        try
        {
            Transform::validate();
            getImpl()->validate();
        }
        catch(Exception & ex)
        {
            std::string errMsg("RangeTransform validation failed: ");
            errMsg += ex.what();
            throw Exception(errMsg.c_str());
        }
}

bool RangeTransform::equals(const RangeTransform & other) const
{
    return getImpl()->equals(*other.getImpl());
}

void RangeTransform::setMinInValue(double val)
{
    getImpl()->setMinInValue(val);
}

double RangeTransform::getMinInValue() const
{
    return getImpl()->getMinInValue();
}

bool RangeTransform::hasMinInValue() const
{
    return getImpl()->hasMinInValue();
}

void RangeTransform::unsetMinInValue()
{
    getImpl()->unsetMinInValue();
}


void RangeTransform::setMaxInValue(double val)
{
    getImpl()->setMaxInValue(val);
}

double RangeTransform::getMaxInValue() const
{
    return getImpl()->getMaxInValue();
}

bool RangeTransform::hasMaxInValue() const
{
    return getImpl()->hasMaxInValue();
}

void RangeTransform::unsetMaxInValue()
{
    getImpl()->unsetMaxInValue();
}


void RangeTransform::setMinOutValue(double val)
{
    getImpl()->setMinOutValue(val);
}

double RangeTransform::getMinOutValue() const
{
    return getImpl()->getMinOutValue();
}

bool RangeTransform::hasMinOutValue() const
{
    return getImpl()->hasMinOutValue();
}

void RangeTransform::unsetMinOutValue()
{
    getImpl()->unsetMinOutValue();
}


void RangeTransform::setMaxOutValue(double val)
{
    getImpl()->setMaxOutValue(val);
}

double RangeTransform::getMaxOutValue() const
{
    return getImpl()->getMaxOutValue();
}

bool RangeTransform::hasMaxOutValue() const
{
    return getImpl()->hasMaxOutValue();
}

void RangeTransform::unsetMaxOutValue()
{
    getImpl()->unsetMaxOutValue();
}


std::ostream& operator<< (std::ostream& os, const RangeTransform& t)
{
    os << "<RangeTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection());
    if(t.getStyle()!=RANGE_CLAMP) os << ", style="       << RangeStyleToString(t.getStyle());
    if(t.hasMinInValue())         os << ", minInValue="  << t.getMinInValue();
    if(t.hasMaxInValue())         os << ", maxInValue="  << t.getMaxInValue();
    if(t.hasMinOutValue())        os << ", minOutValue=" << t.getMinOutValue();
    if(t.hasMaxOutValue())        os << ", maxOutValue=" << t.getMaxOutValue();
    os << ">";
    return os;
}



///////////////////////////////////////////////////////////////////////////

void BuildRangeOps(OpRcPtrVec & ops,
                   const Config& /*config*/,
                   const RangeTransform & transform,
                   TransformDirection dir)
{
    const TransformDirection combinedDir 
        = CombineTransformDirections(dir, transform.getDirection());

    if(transform.getStyle()==RANGE_CLAMP)
    {
        CreateRangeOp(ops, 
                      transform.getMinInValue(), transform.getMaxInValue(), 
                      transform.getMinOutValue(), transform.getMaxOutValue(),
                      combinedDir);
    }
    else
    {
        const RangeOpData r(BIT_DEPTH_F32, BIT_DEPTH_F32, 
                            transform.getMinInValue(), transform.getMaxInValue(), 
                            transform.getMinOutValue(), transform.getMaxOutValue());
        MatrixOpDataRcPtr m = r.convertToMatrix();

        CreateMatrixOp(ops, m, combinedDir);
    }
}
    
}
OCIO_NAMESPACE_EXIT


////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"


OCIO_NAMESPACE_USING


OIIO_ADD_TEST(RangeTransform, basic)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    OIIO_CHECK_EQUAL(range->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(range->getStyle(), OCIO::RANGE_CLAMP);
    OIIO_CHECK_ASSERT(!range->hasMinInValue());
    OIIO_CHECK_ASSERT(!range->hasMaxInValue());
    OIIO_CHECK_ASSERT(!range->hasMinOutValue());
    OIIO_CHECK_ASSERT(!range->hasMaxOutValue());

    range->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(range->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    range->setStyle(OCIO::RANGE_NO_CLAMP);
    OIIO_CHECK_EQUAL(range->getStyle(), OCIO::RANGE_NO_CLAMP);

    range->setMinInValue(-0.5f);
    OIIO_CHECK_EQUAL(range->getMinInValue(), -0.5f);
    OIIO_CHECK_ASSERT(range->hasMinInValue());

    OCIO::RangeTransformRcPtr range2 = OCIO::RangeTransform::Create();
    range2->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    range2->setMinInValue(-0.5f);
    range2->setStyle(OCIO::RANGE_NO_CLAMP);
    OIIO_CHECK_ASSERT(range2->equals(*range));

    range2->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    range2->unsetMinInValue();
    range2->setMaxInValue(-0.5f);
    range2->setMinOutValue(1.5f);
    range2->setMaxOutValue(4.5f);

    // (Note that the transform would not validate at this point.)

    OIIO_CHECK_ASSERT(!range2->hasMinInValue());
    OIIO_CHECK_EQUAL(range2->getMaxInValue(), -0.5f);
    OIIO_CHECK_EQUAL(range2->getMinOutValue(), 1.5f);
    OIIO_CHECK_EQUAL(range2->getMaxOutValue(), 4.5f);

    range2->setMinInValue(-1.5f);
    OIIO_CHECK_EQUAL(range2->getMinInValue(), -1.5f);
    OIIO_CHECK_EQUAL(range2->getMaxInValue(), -0.5f);
    OIIO_CHECK_EQUAL(range2->getMinOutValue(), 1.5f);
    OIIO_CHECK_EQUAL(range2->getMaxOutValue(), 4.5f);

    OIIO_CHECK_ASSERT(range2->hasMinInValue());
    OIIO_CHECK_ASSERT(range2->hasMaxInValue());
    OIIO_CHECK_ASSERT(range2->hasMinOutValue());
    OIIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMinInValue();
    OIIO_CHECK_ASSERT(!range2->hasMinInValue());
    OIIO_CHECK_ASSERT(range2->hasMaxInValue());
    OIIO_CHECK_ASSERT(range2->hasMinOutValue());
    OIIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMaxInValue();
    OIIO_CHECK_ASSERT(!range2->hasMinInValue());
    OIIO_CHECK_ASSERT(!range2->hasMaxInValue());
    OIIO_CHECK_ASSERT(range2->hasMinOutValue());
    OIIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMinOutValue();
    OIIO_CHECK_ASSERT(!range2->hasMinInValue());
    OIIO_CHECK_ASSERT(!range2->hasMaxInValue());
    OIIO_CHECK_ASSERT(!range2->hasMinOutValue());
    OIIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMaxOutValue();
    OIIO_CHECK_ASSERT(!range2->hasMinInValue());
    OIIO_CHECK_ASSERT(!range2->hasMaxInValue());
    OIIO_CHECK_ASSERT(!range2->hasMinOutValue());
    OIIO_CHECK_ASSERT(!range2->hasMaxOutValue());
}


OIIO_ADD_TEST(RangeTransform, no_clamp_converts_to_matrix)
{
    ConfigRcPtr config = Config::Create();
    OCIO::OpRcPtrVec ops;

    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    OIIO_CHECK_EQUAL(range->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(range->getStyle(), OCIO::RANGE_CLAMP);
    OIIO_CHECK_ASSERT(!range->hasMinInValue());
    OIIO_CHECK_ASSERT(!range->hasMaxInValue());
    OIIO_CHECK_ASSERT(!range->hasMinOutValue());
    OIIO_CHECK_ASSERT(!range->hasMaxOutValue());

    range->setMinInValue(0.0f);
    range->setMaxInValue(0.5f);
    range->setMinOutValue(0.5f);
    range->setMaxOutValue(1.5f);

    // Test the resulting Range Op

    OIIO_CHECK_NO_THROW(
        BuildRangeOps(ops, *config, *range, OCIO::TRANSFORM_DIR_FORWARD) );

    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OIIO_REQUIRE_EQUAL(op0->data()->getType(), OCIO::OpData::RangeType);

    OCIO::ConstRangeOpDataRcPtr rangeData
        = DynamicPtrCast<const OCIO::RangeOpData>(op0->data());

    OIIO_CHECK_EQUAL(rangeData->getMinInValue(), range->getMinInValue());
    OIIO_CHECK_EQUAL(rangeData->getMaxInValue(), range->getMaxInValue());
    OIIO_CHECK_EQUAL(rangeData->getMinOutValue(), range->getMinOutValue());
    OIIO_CHECK_EQUAL(rangeData->getMaxOutValue(), range->getMaxOutValue());

    // Test the resulting Matrix Op

    range->setStyle(OCIO::RANGE_NO_CLAMP);

    OIIO_CHECK_NO_THROW(
        BuildRangeOps(ops, *config, *range, OCIO::TRANSFORM_DIR_FORWARD) );

    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op1 = ops[1];
    OIIO_REQUIRE_EQUAL(op1->data()->getType(), OCIO::OpData::MatrixType);

    OCIO::ConstMatrixOpDataRcPtr matrixData
        = DynamicPtrCast<const OCIO::MatrixOpData>(op1->data());

    OIIO_CHECK_EQUAL(matrixData->getOffsetValue(0), rangeData->getOffset());

    OIIO_CHECK_EQUAL(matrixData->getOffsetValue(0), 0.5);
    OIIO_CHECK_EQUAL(matrixData->getOffsetValue(1), 0.5);
    OIIO_CHECK_EQUAL(matrixData->getOffsetValue(2), 0.5);
    OIIO_CHECK_EQUAL(matrixData->getOffsetValue(3), 0.0);

    OIIO_CHECK_ASSERT(matrixData->isDiagonal());

    OIIO_CHECK_EQUAL(matrixData->getArray()[0], rangeData->getScale());

    OIIO_CHECK_EQUAL(matrixData->getArray()[ 0], 2.0);
    OIIO_CHECK_EQUAL(matrixData->getArray()[ 5], 2.0);
    OIIO_CHECK_EQUAL(matrixData->getArray()[10], 2.0);
    OIIO_CHECK_EQUAL(matrixData->getArray()[15], 1.0);
}

#endif
