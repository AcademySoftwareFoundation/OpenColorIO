// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "transforms/RangeTransform.h"

namespace OCIO_NAMESPACE
{

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



} // namespace OCIO_NAMESPACE
