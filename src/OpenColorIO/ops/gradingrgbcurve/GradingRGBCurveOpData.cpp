// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradingrgbcurve/GradingRGBCurve.h"
#include "ops/gradingrgbcurve/GradingRGBCurveOpData.h"
#include "ops/range/RangeOpData.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

namespace DefaultValues
{
const std::streamsize FLOAT_DECIMALS = 7;
ConstGradingBSplineCurveRcPtr Curve(GradingStyle style)
{
    return style == GRADING_LIN ? GradingRGBCurveImpl::DefaultLin.createEditableCopy() :
                                  GradingRGBCurveImpl::Default.createEditableCopy();
}
}

GradingRGBCurveOpData::GradingRGBCurveOpData(GradingStyle style)
    : GradingRGBCurveOpData(style, DefaultValues::Curve(style), DefaultValues::Curve(style),
                                   DefaultValues::Curve(style), DefaultValues::Curve(style))
{
}

GradingRGBCurveOpData::GradingRGBCurveOpData(GradingStyle style,
                                             ConstGradingBSplineCurveRcPtr red,
                                             ConstGradingBSplineCurveRcPtr green,
                                             ConstGradingBSplineCurveRcPtr blue,
                                             ConstGradingBSplineCurveRcPtr master)
    : m_style(style)
{
    ConstGradingRGBCurveRcPtr rgbCurve = GradingRGBCurve::Create(red, green, blue, master);
    m_value = std::make_shared<DynamicPropertyGradingRGBCurveImpl>(rgbCurve, false);
}

GradingRGBCurveOpData::GradingRGBCurveOpData(const GradingRGBCurveOpData & rhs)
    : m_style(rhs.m_style)
{
    ConstGradingRGBCurveRcPtr rgbCurve = GradingRGBCurve::Create(rhs.m_style);
    m_value = std::make_shared<DynamicPropertyGradingRGBCurveImpl>(rgbCurve, false);

    *this = rhs;
}

GradingRGBCurveOpData & GradingRGBCurveOpData::operator=(const GradingRGBCurveOpData & rhs)
{
    if (this == &rhs) return *this;

    OpData::operator=(rhs);

    m_style          = rhs.m_style;
    m_direction      = rhs.m_direction;
    m_bypassLinToLog = rhs.m_bypassLinToLog;

    // Copy dynamic properties. Sharing happens when needed, with CPUOp for instance.
    m_value->setValue(rhs.m_value->getValue());
    if (rhs.m_value->isDynamic())
    {
        m_value->makeDynamic();
    }

    return *this;
}

GradingRGBCurveOpData::~GradingRGBCurveOpData()
{
}

GradingRGBCurveOpDataRcPtr GradingRGBCurveOpData::clone() const
{
    return std::make_shared<GradingRGBCurveOpData>(*this);
}

void GradingRGBCurveOpData::validate() const
{
    // This should already be valid.
    m_value->getValue()->validate();
}

bool GradingRGBCurveOpData::isNoOp() const
{
    return isIdentity();
}

bool GradingRGBCurveOpData::isIdentity() const
{
    if (isDynamic()) return false;

    return m_value->getValue()->isIdentity();
}

bool GradingRGBCurveOpData::isInverse(ConstGradingRGBCurveOpDataRcPtr & r) const
{
    if (isDynamic() || r->isDynamic())
    {
        return false;
    }

    if (m_style == r->m_style &&
        (m_style != GRADING_LIN || m_bypassLinToLog == r->m_bypassLinToLog) &&
        m_value->equals(*r->m_value))
    {
        if (CombineTransformDirections(getDirection(), r->getDirection()) == TRANSFORM_DIR_INVERSE)
        {
            return true;
        }
    }
    return false;
}

GradingRGBCurveOpDataRcPtr GradingRGBCurveOpData::inverse() const
{
    auto res = clone();
    res->m_direction = GetInverseTransformDirection(m_direction);
    return res;
}

std::string GradingRGBCurveOpData::getCacheID() const
{
    AutoMutex lock(m_mutex);

    std::ostringstream cacheIDStream;
    if (!getID().empty())
    {
        cacheIDStream << getID() << " ";
    }

    cacheIDStream.precision(DefaultValues::FLOAT_DECIMALS);

    cacheIDStream << GradingStyleToString(getStyle()) << " ";
    cacheIDStream << TransformDirectionToString(getDirection()) << " ";
    if (m_bypassLinToLog)
    {
        cacheIDStream << " bypassLinToLog";
    }
    if (!isDynamic())
    {
        cacheIDStream << *(m_value->getValue());
    }
    return cacheIDStream.str();
}

void GradingRGBCurveOpData::setStyle(GradingStyle style) noexcept
{
    if (style != m_style)
    {
        m_style = style;
        // Reset value to default when style is changing.
        ConstGradingRGBCurveRcPtr reset = GradingRGBCurve::Create(style);
        m_value->setValue(reset);
    }
}

float GradingRGBCurveOpData::getSlope(RGBCurveType c, size_t index) const
{
    ConstGradingBSplineCurveRcPtr curve = m_value->getValue()->getCurve(c);
    return curve->getSlope(index);
}

void GradingRGBCurveOpData::setSlope(RGBCurveType c, size_t index, float slope)
{
    GradingRGBCurveRcPtr rgbcurve( m_value->getValue()->createEditableCopy() );
    GradingBSplineCurveRcPtr curve = rgbcurve->getCurve(c);
    curve->setSlope(index, slope);
    m_value->setValue(rgbcurve);
}

bool GradingRGBCurveOpData::slopesAreDefault(RGBCurveType c) const
{
    ConstGradingBSplineCurveRcPtr curve = m_value->getValue()->getCurve(c);
    return curve->slopesAreDefault();
}

TransformDirection GradingRGBCurveOpData::getDirection() const noexcept
{
    return m_direction;
}

void GradingRGBCurveOpData::setDirection(TransformDirection dir) noexcept
{
    m_direction = dir;
}

bool GradingRGBCurveOpData::isDynamic() const noexcept
{
    return m_value->isDynamic();
}

DynamicPropertyRcPtr GradingRGBCurveOpData::getDynamicProperty() const noexcept
{
    return m_value;
}

void GradingRGBCurveOpData::replaceDynamicProperty(DynamicPropertyGradingRGBCurveImplRcPtr prop) noexcept
{
    m_value = prop;
}

void GradingRGBCurveOpData::removeDynamicProperty() noexcept
{
    m_value->makeNonDynamic();
}

bool GradingRGBCurveOpData::operator==(const OpData & other) const
{
    if (!OpData::operator==(other)) return false;

    const GradingRGBCurveOpData* rop = static_cast<const GradingRGBCurveOpData*>(&other);

    if (m_direction      != rop->m_direction ||
        m_style          != rop->m_style ||
        m_bypassLinToLog != rop->m_bypassLinToLog ||
       !m_value->equals(  *(rop->m_value)  ))
    {
        return false;
    }

    return true;
}

} // namespace OCIO_NAMESPACE

