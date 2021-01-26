// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/GradingRGBCurveTransform.h"

namespace OCIO_NAMESPACE
{
GradingRGBCurveTransformRcPtr GradingRGBCurveTransform::Create(GradingStyle style)
{
    return GradingRGBCurveTransformRcPtr(new GradingRGBCurveTransformImpl(style),
        &GradingRGBCurveTransformImpl::deleter);
}

GradingRGBCurveTransformImpl::GradingRGBCurveTransformImpl(GradingStyle style)
    : m_data(style)
{
}

void GradingRGBCurveTransformImpl::deleter(GradingRGBCurveTransform * t)
{
    delete static_cast<GradingRGBCurveTransformImpl *>(t);
}

TransformRcPtr GradingRGBCurveTransformImpl::createEditableCopy() const
{
    GradingRGBCurveTransformRcPtr transform = GradingRGBCurveTransform::Create(getStyle());
    dynamic_cast<GradingRGBCurveTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection GradingRGBCurveTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void GradingRGBCurveTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void GradingRGBCurveTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch (Exception & ex)
    {
        std::string errMsg("GradingRGBCurveTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & GradingRGBCurveTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & GradingRGBCurveTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}


bool GradingRGBCurveTransformImpl::equals(const GradingRGBCurveTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const GradingRGBCurveTransformImpl*>(&other)->data();
}

GradingStyle GradingRGBCurveTransformImpl::getStyle() const noexcept
{
    return data().getStyle();
}

void GradingRGBCurveTransformImpl::setStyle(GradingStyle style) noexcept
{
    data().setStyle(style);
}

const ConstGradingRGBCurveRcPtr GradingRGBCurveTransformImpl::getValue() const
{
    return data().getValue();
}

void GradingRGBCurveTransformImpl::setValue(const ConstGradingRGBCurveRcPtr & values)
{
    data().setValue(values);
}

float GradingRGBCurveTransformImpl::getSlope(RGBCurveType c, size_t index) const
{
    return data().getSlope(c, index);
}

void GradingRGBCurveTransformImpl::setSlope(RGBCurveType c, size_t index, float slope)
{
    data().setSlope(c, index, slope);
}

bool GradingRGBCurveTransformImpl::slopesAreDefault(RGBCurveType c) const
{
    return data().slopesAreDefault(c);
}

bool GradingRGBCurveTransformImpl::getBypassLinToLog() const
{
    return data().getBypassLinToLog();
}

void GradingRGBCurveTransformImpl::setBypassLinToLog(bool bypass)
{
    data().setBypassLinToLog(bypass);
}

bool GradingRGBCurveTransformImpl::isDynamic() const noexcept
{
    return data().isDynamic();
}

void GradingRGBCurveTransformImpl::makeDynamic() noexcept
{
    data().getDynamicPropertyInternal()->makeDynamic();
}

void GradingRGBCurveTransformImpl::makeNonDynamic() noexcept
{
    data().getDynamicPropertyInternal()->makeNonDynamic();
}

std::ostream& operator<< (std::ostream & os, const GradingRGBCurveTransform & t) noexcept
{
    os << "<GradingRGBCurveTransform ";
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

std::ostream & operator<<(std::ostream & os, const GradingControlPoint & cp)
{
    os << "<x=" << cp.m_x << ", y=" << cp.m_y << ">";
    return os;
}

std::ostream & operator<<(std::ostream & os, const GradingBSplineCurve & bspline)
{
    os << "<control_points=[";
    const auto numPoints = bspline.getNumControlPoints();
    for (size_t i = 0; i < numPoints; ++i)
    {
        os << bspline.getControlPoint(i);
    }
    os << "]>";
    return os;
}

std::ostream & operator<<(std::ostream & os, const GradingRGBCurve & rgbCurve)
{
    os << "<red=" << *rgbCurve.getCurve(RGB_RED);
    os << ", green=" << *rgbCurve.getCurve(RGB_GREEN);
    os << ", blue=" << *rgbCurve.getCurve(RGB_BLUE);
    os << ", master=" << *rgbCurve.getCurve(RGB_MASTER);
    os << ">";

    return os;
}

} // namespace OCIO_NAMESPACE
