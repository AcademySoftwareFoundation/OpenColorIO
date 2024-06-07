// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/HueCurveTransform.h"

namespace OCIO_NAMESPACE
{

HueCurveTransformRcPtr HueCurveTransform::Create(GradingStyle style)
{
    return HueCurveTransformRcPtr(new HueCurveTransformImpl(style),
                                      &HueCurveTransformImpl::deleter);
}

HueCurveTransformImpl::HueCurveTransformImpl(GradingStyle style) :
   m_data(style)
{
}

void HueCurveTransformImpl::deleter(HueCurveTransform* t)
{
    delete static_cast<HueCurveTransformImpl*>(t);
}

TransformRcPtr HueCurveTransformImpl::createEditableCopy() const
{
    HueCurveTransformRcPtr transform = HueCurveTransform::Create(getStyle());
    dynamic_cast<HueCurveTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection HueCurveTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void HueCurveTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void HueCurveTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch(Exception & ex)
    {
        std::string errMsg("HueCurveTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }

    return;
}

FormatMetadata & HueCurveTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & HueCurveTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

bool HueCurveTransformImpl::equals(const HueCurveTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const HueCurveTransformImpl*>(&other)->data();
}

GradingStyle HueCurveTransformImpl::getStyle() const noexcept
{
    return data().getStyle();
}

void HueCurveTransformImpl::setStyle(GradingStyle style) noexcept
{
    data().setStyle(style);
}

const ConstGradingHueCurveRcPtr HueCurveTransformImpl::getValue() const
{
    return data().getValue();
}

void HueCurveTransformImpl::setValue(const ConstGradingHueCurveRcPtr & values)
{
    data().setValue(values);
}

float HueCurveTransformImpl::getSlope(HueCurveType c, size_t index) const
{
    return data().getSlope(c, index);
}

void HueCurveTransformImpl::setSlope(HueCurveType c, size_t index, float slope)
{
    data().setSlope(c, index, slope);
}

bool HueCurveTransformImpl::slopesAreDefault(HueCurveType c) const
{
    return data().slopesAreDefault(c);
}

bool HueCurveTransformImpl::getBypassLinToLog() const noexcept
{
    return data().getBypassLinToLog();
}

void HueCurveTransformImpl::setBypassLinToLog(bool bypass) noexcept
{
    data().setBypassLinToLog(bypass);
}

bool HueCurveTransformImpl::isDynamic() const noexcept
{
    return data().isDynamic();
}

void HueCurveTransformImpl::makeDynamic() noexcept
{
    data().getDynamicPropertyInternal()->makeDynamic();
}

void HueCurveTransformImpl::makeNonDynamic() noexcept
{
    data().getDynamicPropertyInternal()->makeNonDynamic();
}

std::ostream& operator<< (std::ostream & os, const HueCurveTransform & t) noexcept
{
    os << "<HueCurveTransform ";
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
