// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "ops/Range/RangeOpData.h"


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

    FormatMetadata & getFormatMetadata() override;
    const FormatMetadata & getFormatMetadata() const override;

    BitDepth getFileInputBitDepth() const override;
    BitDepth getFileOutputBitDepth() const override;
    void setFileInputBitDepth(BitDepth bitDepth) override;
    void setFileOutputBitDepth(BitDepth bitDepth) override;

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

FormatMetadata & RangeTransformImpl::getFormatMetadata()
{
	return data().getFormatMetadata();
}

const FormatMetadata & RangeTransformImpl::getFormatMetadata() const
{
	return data().getFormatMetadata();
}

BitDepth RangeTransformImpl::getFileInputBitDepth() const
{
    return data().getFileInputBitDepth();
}
BitDepth RangeTransformImpl::getFileOutputBitDepth() const
{
    return data().getFileOutputBitDepth();
}
void RangeTransformImpl::setFileInputBitDepth(BitDepth bitDepth)
{
    data().setFileInputBitDepth(bitDepth);
}
void RangeTransformImpl::setFileOutputBitDepth(BitDepth bitDepth)
{
    data().setFileOutputBitDepth(bitDepth);
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
    os << ", fileindepth=" << BitDepthToString(t.getFileInputBitDepth());
    os << ", fileoutdepth=" << BitDepthToString(t.getFileOutputBitDepth());
    if(t.getStyle()!=RANGE_CLAMP) os << ", style="       << RangeStyleToString(t.getStyle());
    if(t.hasMinInValue())         os << ", minInValue="  << t.getMinInValue();
    if(t.hasMaxInValue())         os << ", maxInValue="  << t.getMaxInValue();
    if(t.hasMinOutValue())        os << ", minOutValue=" << t.getMinOutValue();
    if(t.hasMaxOutValue())        os << ", maxOutValue=" << t.getMaxOutValue();
    os << ">";
    return os;
}



}
OCIO_NAMESPACE_EXIT


////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

#include "UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


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

    range->setMinInValue(-0.5);
    OCIO_CHECK_EQUAL(range->getMinInValue(), -0.5);
    OCIO_CHECK_ASSERT(range->hasMinInValue());

    OCIO::RangeTransformRcPtr range2 = OCIO::RangeTransform::Create();
    range2->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    range2->setMinInValue(-0.5);
    range2->setStyle(OCIO::RANGE_NO_CLAMP);
    OCIO_CHECK_ASSERT(range2->equals(*range));

    range2->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    range2->setMinInValue(-1.5);
    range2->setMaxInValue(-0.5);
    range2->setMinOutValue(1.5);
    range2->setMaxOutValue(4.5);

    OCIO_CHECK_EQUAL(range2->getFileInputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);
    OCIO_CHECK_EQUAL(range2->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);

    range2->setFileInputBitDepth(OCIO::BIT_DEPTH_UINT8);
    range2->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    OCIO_CHECK_EQUAL(range2->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(range2->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    OCIO_CHECK_EQUAL(range2->getMinInValue(), -1.5);
    OCIO_CHECK_EQUAL(range2->getMaxInValue(), -0.5);
    OCIO_CHECK_EQUAL(range2->getMinOutValue(), 1.5);
    OCIO_CHECK_EQUAL(range2->getMaxOutValue(), 4.5);

    range2->unsetMinInValue();

    // (Note that the transform would not validate at this point.)

    OCIO_CHECK_ASSERT(!range2->hasMinInValue());
    OCIO_CHECK_EQUAL(range2->getMaxInValue(), -0.5);
    OCIO_CHECK_EQUAL(range2->getMinOutValue(), 1.5);
    OCIO_CHECK_EQUAL(range2->getMaxOutValue(), 4.5);

    range2->setMinInValue(-1.5f);
    OCIO_CHECK_EQUAL(range2->getMinInValue(), -1.5);
    OCIO_CHECK_EQUAL(range2->getMaxInValue(), -0.5);
    OCIO_CHECK_EQUAL(range2->getMinOutValue(), 1.5);
    OCIO_CHECK_EQUAL(range2->getMaxOutValue(), 4.5);

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

#endif
