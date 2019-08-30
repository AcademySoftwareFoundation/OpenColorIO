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

class RangeTransformImpl : public RangeTransform
{
public:
    RangeTransformImpl() = default;
    RangeTransformImpl(const RangeTransformImpl &) = delete;
    RangeTransformImpl& operator=(const RangeTransformImpl &) = delete;
    virtual ~RangeTransformImpl() = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const override;
    void setDirection(TransformDirection dir) override;

    RangeStyle getStyle() const override;
    void setStyle(RangeStyle style) override;

    void validate() const override;

    bool equals(const RangeTransform & other) const override;

    void setMinInValue(double val) override;
    double getMinInValue() const override;
    bool hasMinInValue() const override;
    void unsetMinInValue() override;

    void setMaxInValue(double val) override;
    double getMaxInValue() const override;
    bool hasMaxInValue() const override;
    void unsetMaxInValue() override;

    void setMinOutValue(double val) override;
    double getMinOutValue() const override;
    bool hasMinOutValue() const override;
    void unsetMinOutValue() override;

    void setMaxOutValue(double val) override;
    double getMaxOutValue() const override;
    bool hasMaxOutValue() const override;
    void unsetMaxOutValue() override;

    RangeOpData & data() { return m_data; }
    const RangeOpData & data() const { return m_data; }

    static void deleter(RangeTransformImpl * t)
    {
        delete t;
    }

private:
    TransformDirection m_direction = TRANSFORM_DIR_FORWARD;
    RangeStyle  m_style = RANGE_CLAMP;
    RangeOpData m_data;
};



///////////////////////////////////////////////////////////////////////////



RangeTransformRcPtr RangeTransform::Create()
{
    return RangeTransformRcPtr(new RangeTransformImpl(), &RangeTransformImpl::deleter);
}

TransformRcPtr RangeTransformImpl::createEditableCopy() const
{
    RangeTransformRcPtr transform = RangeTransform::Create();
    dynamic_cast<RangeTransformImpl*>(transform.get())->data() = data();
    transform->setDirection(m_direction);
    transform->setStyle(m_style);
    return transform;
}

TransformDirection RangeTransformImpl::getDirection() const
{
    return m_direction;
}

void RangeTransformImpl::setDirection(TransformDirection dir)
{
    m_direction = dir;
}

RangeStyle RangeTransformImpl::getStyle() const
{
    return m_style;
}

void RangeTransformImpl::setStyle(RangeStyle style)
{
    m_style = style;
}

void RangeTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch(Exception & ex)
    {
        std::string errMsg("RangeTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

bool RangeTransformImpl::equals(const RangeTransform & other) const
{
    return data() == dynamic_cast<const RangeTransformImpl*>(&other)->data()
        && m_style == other.getStyle()
        && m_direction == other.getDirection();
}

void RangeTransformImpl::setMinInValue(double val)
{
    data().setMinInValue(val);
}

double RangeTransformImpl::getMinInValue() const
{
    return data().getMinInValue();
}

bool RangeTransformImpl::hasMinInValue() const
{
    return data().hasMinInValue();
}

void RangeTransformImpl::unsetMinInValue()
{
    data().unsetMinInValue();
}


void RangeTransformImpl::setMaxInValue(double val)
{
    data().setMaxInValue(val);
}

double RangeTransformImpl::getMaxInValue() const
{
    return data().getMaxInValue();
}

bool RangeTransformImpl::hasMaxInValue() const
{
    return data().hasMaxInValue();
}

void RangeTransformImpl::unsetMaxInValue()
{
    data().unsetMaxInValue();
}


void RangeTransformImpl::setMinOutValue(double val)
{
    data().setMinOutValue(val);
}

double RangeTransformImpl::getMinOutValue() const
{
    return data().getMinOutValue();
}

bool RangeTransformImpl::hasMinOutValue() const
{
    return data().hasMinOutValue();
}

void RangeTransformImpl::unsetMinOutValue()
{
    data().unsetMinOutValue();
}


void RangeTransformImpl::setMaxOutValue(double val)
{
    data().setMaxOutValue(val);
}

double RangeTransformImpl::getMaxOutValue() const
{
    return data().getMaxOutValue();
}

bool RangeTransformImpl::hasMaxOutValue() const
{
    return data().hasMaxOutValue();
}

void RangeTransformImpl::unsetMaxOutValue()
{
    data().unsetMaxOutValue();
}


std::ostream& operator<< (std::ostream & os, const RangeTransform & t)
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
                   const Config & /*config*/,
                   const RangeTransform & transform,
                   TransformDirection dir)
{
    const TransformDirection combinedDir
        = CombineTransformDirections(dir, transform.getDirection());

    const RangeOpData & data
        = dynamic_cast<const RangeTransformImpl*>(&transform)->data();

    data.validate();

    if(transform.getStyle()==RANGE_CLAMP)
    {
        CreateRangeOp(ops, data.clone(), combinedDir);
    }
    else
    {
        MatrixOpDataRcPtr m = data.convertToMatrix();
        CreateMatrixOp(ops, m, combinedDir);
    }
}

}
OCIO_NAMESPACE_EXIT


////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"


OCIO_ADD_TEST(RangeTransform, basic)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    OCIO_CHECK_EQUAL(range->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(range->getStyle(), OCIO::RANGE_CLAMP);
    OCIO_CHECK_ASSERT(!range->hasMinInValue());
    OCIO_CHECK_ASSERT(!range->hasMaxInValue());
    OCIO_CHECK_ASSERT(!range->hasMinOutValue());
    OCIO_CHECK_ASSERT(!range->hasMaxOutValue());

    range->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(range->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    range->setStyle(OCIO::RANGE_NO_CLAMP);
    OCIO_CHECK_EQUAL(range->getStyle(), OCIO::RANGE_NO_CLAMP);

    range->setMinInValue(-0.5f);
    OCIO_CHECK_EQUAL(range->getMinInValue(), -0.5f);
    OCIO_CHECK_ASSERT(range->hasMinInValue());

    OCIO::RangeTransformRcPtr range2 = OCIO::RangeTransform::Create();
    range2->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    range2->setMinInValue(-0.5f);
    range2->setStyle(OCIO::RANGE_NO_CLAMP);
    OCIO_CHECK_ASSERT(range2->equals(*range));

    range2->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    range2->unsetMinInValue();
    range2->setMaxInValue(-0.5f);
    range2->setMinOutValue(1.5f);
    range2->setMaxOutValue(4.5f);

    // (Note that the transform would not validate at this point.)

    OCIO_CHECK_ASSERT(!range2->hasMinInValue());
    OCIO_CHECK_EQUAL(range2->getMaxInValue(), -0.5f);
    OCIO_CHECK_EQUAL(range2->getMinOutValue(), 1.5f);
    OCIO_CHECK_EQUAL(range2->getMaxOutValue(), 4.5f);

    range2->setMinInValue(-1.5f);
    OCIO_CHECK_EQUAL(range2->getMinInValue(), -1.5f);
    OCIO_CHECK_EQUAL(range2->getMaxInValue(), -0.5f);
    OCIO_CHECK_EQUAL(range2->getMinOutValue(), 1.5f);
    OCIO_CHECK_EQUAL(range2->getMaxOutValue(), 4.5f);

    OCIO_CHECK_ASSERT(range2->hasMinInValue());
    OCIO_CHECK_ASSERT(range2->hasMaxInValue());
    OCIO_CHECK_ASSERT(range2->hasMinOutValue());
    OCIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMinInValue();
    OCIO_CHECK_ASSERT(!range2->hasMinInValue());
    OCIO_CHECK_ASSERT(range2->hasMaxInValue());
    OCIO_CHECK_ASSERT(range2->hasMinOutValue());
    OCIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMaxInValue();
    OCIO_CHECK_ASSERT(!range2->hasMinInValue());
    OCIO_CHECK_ASSERT(!range2->hasMaxInValue());
    OCIO_CHECK_ASSERT(range2->hasMinOutValue());
    OCIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMinOutValue();
    OCIO_CHECK_ASSERT(!range2->hasMinInValue());
    OCIO_CHECK_ASSERT(!range2->hasMaxInValue());
    OCIO_CHECK_ASSERT(!range2->hasMinOutValue());
    OCIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMaxOutValue();
    OCIO_CHECK_ASSERT(!range2->hasMinInValue());
    OCIO_CHECK_ASSERT(!range2->hasMaxInValue());
    OCIO_CHECK_ASSERT(!range2->hasMinOutValue());
    OCIO_CHECK_ASSERT(!range2->hasMaxOutValue());
}


OCIO_ADD_TEST(RangeTransform, no_clamp_converts_to_matrix)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    OCIO::OpRcPtrVec ops;

    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    OCIO_CHECK_EQUAL(range->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(range->getStyle(), OCIO::RANGE_CLAMP);
    OCIO_CHECK_ASSERT(!range->hasMinInValue());
    OCIO_CHECK_ASSERT(!range->hasMaxInValue());
    OCIO_CHECK_ASSERT(!range->hasMinOutValue());
    OCIO_CHECK_ASSERT(!range->hasMaxOutValue());

    range->setMinInValue(0.0);
    range->setMaxInValue(0.5);
    range->setMinOutValue(0.5);
    range->setMaxOutValue(1.5);

    // Test the resulting Range Op

    OCIO_CHECK_NO_THROW(
        OCIO::BuildRangeOps(ops, *config, *range, OCIO::TRANSFORM_DIR_FORWARD) );

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO_REQUIRE_EQUAL(op0->data()->getType(), OCIO::OpData::RangeType);

    OCIO::ConstRangeOpDataRcPtr rangeData
        = OCIO::DynamicPtrCast<const OCIO::RangeOpData>(op0->data());

    OCIO_CHECK_EQUAL(rangeData->getMinInValue(), range->getMinInValue());
    OCIO_CHECK_EQUAL(rangeData->getMaxInValue(), range->getMaxInValue());
    OCIO_CHECK_EQUAL(rangeData->getMinOutValue(), range->getMinOutValue());
    OCIO_CHECK_EQUAL(rangeData->getMaxOutValue(), range->getMaxOutValue());

    // Test the resulting Matrix Op

    range->setStyle(OCIO::RANGE_NO_CLAMP);

    OCIO_CHECK_NO_THROW(
        OCIO::BuildRangeOps(ops, *config, *range, OCIO::TRANSFORM_DIR_FORWARD) );

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO_REQUIRE_EQUAL(op1->data()->getType(), OCIO::OpData::MatrixType);

    OCIO::ConstMatrixOpDataRcPtr matrixData
        = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op1->data());

    OCIO_CHECK_EQUAL(matrixData->getOffsetValue(0), rangeData->getOffset());

    OCIO_CHECK_EQUAL(matrixData->getOffsetValue(0), 0.5);
    OCIO_CHECK_EQUAL(matrixData->getOffsetValue(1), 0.5);
    OCIO_CHECK_EQUAL(matrixData->getOffsetValue(2), 0.5);
    OCIO_CHECK_EQUAL(matrixData->getOffsetValue(3), 0.0);

    OCIO_CHECK_ASSERT(matrixData->isDiagonal());

    OCIO_CHECK_EQUAL(matrixData->getArray()[0], rangeData->getScale());

    OCIO_CHECK_EQUAL(matrixData->getArray()[ 0], 2.0);
    OCIO_CHECK_EQUAL(matrixData->getArray()[ 5], 2.0);
    OCIO_CHECK_EQUAL(matrixData->getArray()[10], 2.0);
    OCIO_CHECK_EQUAL(matrixData->getArray()[15], 1.0);
}

#endif
