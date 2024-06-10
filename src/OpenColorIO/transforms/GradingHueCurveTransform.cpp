// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/GradingHueCurveTransform.h"

namespace OCIO_NAMESPACE
{

GradingHueCurveTransformRcPtr GradingHueCurveTransform::Create(GradingStyle style)
{
    return GradingHueCurveTransformRcPtr(new GradingHueCurveTransformImpl(style),
                                      &GradingHueCurveTransformImpl::deleter);
}

GradingHueCurveTransformImpl::GradingHueCurveTransformImpl(GradingStyle style) :
   m_data(style)
{
}

void GradingHueCurveTransformImpl::deleter(GradingHueCurveTransform* t)
{
    delete static_cast<GradingHueCurveTransformImpl*>(t);
}

TransformRcPtr GradingHueCurveTransformImpl::createEditableCopy() const
{
    GradingHueCurveTransformRcPtr transform = GradingHueCurveTransform::Create(getStyle());
    dynamic_cast<GradingHueCurveTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection GradingHueCurveTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void GradingHueCurveTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void GradingHueCurveTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch(Exception & ex)
    {
        std::string errMsg("GradingHueCurveTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }

    return;
}

FormatMetadata & GradingHueCurveTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & GradingHueCurveTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

bool GradingHueCurveTransformImpl::equals(const GradingHueCurveTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const GradingHueCurveTransformImpl*>(&other)->data();
}

GradingStyle GradingHueCurveTransformImpl::getStyle() const noexcept
{
    return data().getStyle();
}

void GradingHueCurveTransformImpl::setStyle(GradingStyle style) noexcept
{
    data().setStyle(style);
}

const ConstGradingHueCurveRcPtr GradingHueCurveTransformImpl::getValue() const
{
    return data().getValue();
}

void GradingHueCurveTransformImpl::setValue(const ConstGradingHueCurveRcPtr & values)
{
    data().setValue(values);
}

float GradingHueCurveTransformImpl::getSlope(HueCurveType c, size_t index) const
{
    return data().getSlope(c, index);
}

void GradingHueCurveTransformImpl::setSlope(HueCurveType c, size_t index, float slope)
{
    data().setSlope(c, index, slope);
}

bool GradingHueCurveTransformImpl::slopesAreDefault(HueCurveType c) const
{
    return data().slopesAreDefault(c);
}

bool GradingHueCurveTransformImpl::getBypassLinToLog() const noexcept
{
    return data().getBypassLinToLog();
}

void GradingHueCurveTransformImpl::setBypassLinToLog(bool bypass) noexcept
{
    data().setBypassLinToLog(bypass);
}

bool GradingHueCurveTransformImpl::isDynamic() const noexcept
{
    return data().isDynamic();
}

void GradingHueCurveTransformImpl::makeDynamic() noexcept
{
    data().getDynamicPropertyInternal()->makeDynamic();
}

void GradingHueCurveTransformImpl::makeNonDynamic() noexcept
{
    data().getDynamicPropertyInternal()->makeNonDynamic();
}

std::ostream& operator<< (std::ostream & os, const GradingHueCurveTransform & t) noexcept
{
    os << "<GradingHueCurveTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection());
    os << ", style=" << GradingStyleToString(t.getStyle());
    os << ", values=" << *t.getValue();
    if (t.isDynamic())
    {
        os << ", dynamic";
    }
    os << ">";
    return os;
}

std::ostream & operator<<(std::ostream & os, const GradingHueCurve & hueCurve)
{
    os << "<hue_hue=" << *hueCurve.getCurve(HUE_HUE);
    os << ", hue_sat=" << *hueCurve.getCurve(HUE_SAT);
    os << ", hue_lum=" << *hueCurve.getCurve(HUE_LUM);
    os << ", lum_sat=" << *hueCurve.getCurve(LUM_SAT);
    os << ", sat_sat=" << *hueCurve.getCurve(SAT_SAT);
    os << ", lum_lum=" << *hueCurve.getCurve(LUM_LUM);
    os << ", sat_lum=" << *hueCurve.getCurve(SAT_LUM);
    os << ", hue_fx=" << *hueCurve.getCurve(HUE_FX);
    os << ">";

    return os;
}

} // namespace OCIO_NAMESPACE
