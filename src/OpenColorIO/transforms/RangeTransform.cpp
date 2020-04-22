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

void RangeTransformImpl::deleter(RangeTransform * t)
{
    delete static_cast<RangeTransformImpl *>(t);
}

TransformRcPtr RangeTransformImpl::createEditableCopy() const
{
    RangeTransformRcPtr transform = RangeTransform::Create();
    dynamic_cast<RangeTransformImpl*>(transform.get())->data() = data();
    transform->setStyle(m_style);
    return transform;
}

TransformDirection RangeTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void RangeTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

RangeStyle RangeTransformImpl::getStyle() const noexcept
{
    return m_style;
}

void RangeTransformImpl::setStyle(RangeStyle style) noexcept
{
    m_style = style;
}

void RangeTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
        if (m_style == RANGE_NO_CLAMP)
        {
            if (data().minIsEmpty() || data().maxIsEmpty())
            {
                throw Exception("RangeTransform validation failed: non clamping range must "
                                "have min and max values defined.");
            }
        }
    }
    catch(Exception & ex)
    {
        std::string errMsg("RangeTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & RangeTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & RangeTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

BitDepth RangeTransformImpl::getFileInputBitDepth() const noexcept
{
    return data().getFileInputBitDepth();
}
BitDepth RangeTransformImpl::getFileOutputBitDepth() const noexcept
{
    return data().getFileOutputBitDepth();
}
void RangeTransformImpl::setFileInputBitDepth(BitDepth bitDepth) noexcept
{
    data().setFileInputBitDepth(bitDepth);
}
void RangeTransformImpl::setFileOutputBitDepth(BitDepth bitDepth) noexcept
{
    data().setFileOutputBitDepth(bitDepth);
}

bool RangeTransformImpl::equals(const RangeTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const RangeTransformImpl*>(&other)->data()
        && m_style == other.getStyle();
}

void RangeTransformImpl::setMinInValue(double val) noexcept
{
    data().setMinInValue(val);
}

double RangeTransformImpl::getMinInValue() const noexcept
{
    return data().getMinInValue();
}

bool RangeTransformImpl::hasMinInValue() const noexcept
{
    return data().hasMinInValue();
}

void RangeTransformImpl::unsetMinInValue() noexcept
{
    data().unsetMinInValue();
}


void RangeTransformImpl::setMaxInValue(double val) noexcept
{
    data().setMaxInValue(val);
}

double RangeTransformImpl::getMaxInValue() const noexcept
{
    return data().getMaxInValue();
}

bool RangeTransformImpl::hasMaxInValue() const noexcept
{
    return data().hasMaxInValue();
}

void RangeTransformImpl::unsetMaxInValue() noexcept
{
    data().unsetMaxInValue();
}


void RangeTransformImpl::setMinOutValue(double val) noexcept
{
    data().setMinOutValue(val);
}

double RangeTransformImpl::getMinOutValue() const noexcept
{
    return data().getMinOutValue();
}

bool RangeTransformImpl::hasMinOutValue() const noexcept
{
    return data().hasMinOutValue();
}

void RangeTransformImpl::unsetMinOutValue() noexcept
{
    data().unsetMinOutValue();
}


void RangeTransformImpl::setMaxOutValue(double val) noexcept
{
    data().setMaxOutValue(val);
}

double RangeTransformImpl::getMaxOutValue() const noexcept
{
    return data().getMaxOutValue();
}

bool RangeTransformImpl::hasMaxOutValue() const noexcept
{
    return data().hasMaxOutValue();
}

void RangeTransformImpl::unsetMaxOutValue() noexcept
{
    data().unsetMaxOutValue();
}


std::ostream& operator<< (std::ostream & os, const RangeTransform & t) noexcept
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
