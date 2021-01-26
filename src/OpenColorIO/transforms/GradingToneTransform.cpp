// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/GradingToneTransform.h"

namespace OCIO_NAMESPACE
{

GradingToneTransformRcPtr GradingToneTransform::Create(GradingStyle style)
{
    return GradingToneTransformRcPtr(new GradingToneTransformImpl(style),
                                     &GradingToneTransformImpl::deleter);
}

GradingToneTransformImpl::GradingToneTransformImpl(GradingStyle style)
    : m_data(style)
{
}

void GradingToneTransformImpl::deleter(GradingToneTransform * t)
{
    delete static_cast<GradingToneTransformImpl *>(t);
}

TransformRcPtr GradingToneTransformImpl::createEditableCopy() const
{
    GradingToneTransformRcPtr transform = GradingToneTransform::Create(getStyle());
    dynamic_cast<GradingToneTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection GradingToneTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void GradingToneTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void GradingToneTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch(Exception & ex)
    {
        std::string errMsg("GradingToneTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & GradingToneTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & GradingToneTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}


bool GradingToneTransformImpl::equals(const GradingToneTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const GradingToneTransformImpl*>(&other)->data();
}

GradingStyle GradingToneTransformImpl::getStyle() const noexcept
{
    return data().getStyle();
}

void GradingToneTransformImpl::setStyle(GradingStyle style) noexcept
{
    data().setStyle(style);
}

const GradingTone & GradingToneTransformImpl::getValue() const
{
    return data().getValue();
}

void GradingToneTransformImpl::setValue(const GradingTone & values)
{
    data().setValue(values);
}

bool GradingToneTransformImpl::isDynamic() const noexcept
{
    return data().isDynamic();
}

void GradingToneTransformImpl::makeDynamic() noexcept
{
    data().getDynamicPropertyInternal()->makeDynamic();
}

void GradingToneTransformImpl::makeNonDynamic() noexcept
{
    data().getDynamicPropertyInternal()->makeNonDynamic();
}

std::ostream& operator<< (std::ostream & os, const GradingToneTransform & t) noexcept
{
    os << "<GradingToneTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection());
    os << ", style=" << GradingStyleToString(t.getStyle());
    os << ", values=" << t.getValue();
    if (t.isDynamic())
    {
        os << ", dynamic";
    }
    os << ">";
    return os;
}

std::ostream & operator<<(std::ostream & ost, const GradingRGBMSW & rgbmsw)
{
    ost << "<red=" << rgbmsw.m_red << " green=" << rgbmsw.m_green << " blue=" << rgbmsw.m_blue
        << " master=" << rgbmsw.m_master
        << " start=" << rgbmsw.m_start << " width=" << rgbmsw.m_width << ">";
    return ost;
}

std::ostream & operator<<(std::ostream & os, const GradingTone & tone)
{
    os << "<blacks="      << tone.m_blacks;
    os << " shadows="     << tone.m_shadows    ;
    os << " midtones="    << tone.m_midtones   ;
    os << " highlights="  << tone.m_highlights ;
    os << " whites="      << tone.m_whites;
    os << " s_contrast="  << tone.m_scontrast  ;
    os << ">";

    return os;
}

} // namespace OCIO_NAMESPACE
