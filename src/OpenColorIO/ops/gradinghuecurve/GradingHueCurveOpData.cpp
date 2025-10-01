// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradinghuecurve/GradingHueCurve.h"
#include "ops/gradinghuecurve/GradingHueCurveOpData.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

namespace DefaultValues
{
const std::streamsize FLOAT_DECIMALS = 7;
}

GradingHueCurveOpData::GradingHueCurveOpData(GradingStyle style)
    : OpData()
    , m_style(style)
{
    ConstGradingHueCurveRcPtr hueCurve = GradingHueCurve::Create(style);
    m_value = std::make_shared<DynamicPropertyGradingHueCurveImpl>(hueCurve, false);
}

GradingHueCurveOpData::GradingHueCurveOpData(const GradingHueCurveOpData & rhs)
    : OpData(rhs)
    , m_style(rhs.m_style)
{
    ConstGradingHueCurveRcPtr hueCurve = GradingHueCurve::Create(rhs.m_style);
    m_value = std::make_shared<DynamicPropertyGradingHueCurveImpl>(hueCurve, false);

    *this = rhs;
}

GradingHueCurveOpData::GradingHueCurveOpData(GradingStyle style,                     
   ConstGradingBSplineCurveRcPtr hueHue,
   ConstGradingBSplineCurveRcPtr hueSat,
   ConstGradingBSplineCurveRcPtr hueLum,
   ConstGradingBSplineCurveRcPtr lumSat,
   ConstGradingBSplineCurveRcPtr satSat,
   ConstGradingBSplineCurveRcPtr lumLum,
   ConstGradingBSplineCurveRcPtr satLum,
   ConstGradingBSplineCurveRcPtr hueFx)
    : OpData()
    , m_style(style)
{
    ConstGradingHueCurveRcPtr hueCurve = GradingHueCurve::Create(
       hueHue,
       hueSat,
       hueLum,
       lumSat,
       satSat,
       lumLum,
       satLum,
       hueFx);
    m_value = std::make_shared<DynamicPropertyGradingHueCurveImpl>(hueCurve, false);
}

GradingHueCurveOpData & GradingHueCurveOpData::operator=(const GradingHueCurveOpData & rhs)
{
    if (this == &rhs) return *this;

    OpData::operator=(rhs);

    m_direction = rhs.m_direction;
    m_style = rhs.m_style;
    m_RGBToHSY = rhs.m_RGBToHSY;

    // Copy dynamic properties. Sharing happens when needed, with CPUOp for instance.
    m_value->setValue(rhs.m_value->getValue());
    if (rhs.m_value->isDynamic())
    {
        m_value->makeDynamic();
    }

    return *this;
}

GradingHueCurveOpData::~GradingHueCurveOpData()
{
}

GradingHueCurveOpDataRcPtr GradingHueCurveOpData::clone() const
{
    return std::make_shared<GradingHueCurveOpData>(*this);
}

void GradingHueCurveOpData::validate() const
{
    // This should already be valid.
    m_value->getValue()->validate();
    return;
}

bool GradingHueCurveOpData::isNoOp() const
{
    return isIdentity();
}

bool GradingHueCurveOpData::isIdentity() const
{
    if (isDynamic()) return false;
    
    return m_value->getValue()->isIdentity();
}

bool GradingHueCurveOpData::isInverse(ConstGradingHueCurveOpDataRcPtr & r) const
{
    if (isDynamic() || r->isDynamic())
    {
        return false;
    }

    if (m_style == r->m_style &&
        (m_style != GRADING_LIN || m_RGBToHSY == r->m_RGBToHSY) &&
        m_value->equals(*r->m_value))
    {
        if (CombineTransformDirections(getDirection(), r->getDirection()) == TRANSFORM_DIR_INVERSE)
        {
            return true;
        }
    }
    return false;
}

GradingHueCurveOpDataRcPtr GradingHueCurveOpData::inverse() const
{
    auto res = clone();
    res->m_direction = GetInverseTransformDirection(m_direction);
    return res;
}

std::string GradingHueCurveOpData::getCacheID() const
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
    if (m_RGBToHSY != HSY_TRANSFORM_1)
    {
        cacheIDStream << " bypassRGBToHSY ";
    }
    if (!isDynamic())
    {
        cacheIDStream << *(m_value->getValue());
    }
    return cacheIDStream.str();
}

void GradingHueCurveOpData::setStyle(GradingStyle style) noexcept
{
    if (style != m_style)
    {
        m_style = style;
        // Reset value to default when style is changing.
        ConstGradingHueCurveRcPtr reset = GradingHueCurve::Create(style);
        m_value->setValue(reset);
    }
}

float GradingHueCurveOpData::getSlope(HueCurveType c, size_t index) const
{
    ConstGradingBSplineCurveRcPtr curve = m_value->getValue()->getCurve(c);
    return curve->getSlope(index);
}

void GradingHueCurveOpData::setSlope(HueCurveType c, size_t index, float slope)
{
    GradingHueCurveRcPtr hueCurve( m_value->getValue()->createEditableCopy() );
    GradingBSplineCurveRcPtr curve = hueCurve->getCurve(c);
    curve->setSlope(index, slope);
    m_value->setValue(hueCurve);
}

bool GradingHueCurveOpData::slopesAreDefault(HueCurveType c) const
{
    ConstGradingBSplineCurveRcPtr curve = m_value->getValue()->getCurve(c);
    return curve->slopesAreDefault();
}

TransformDirection GradingHueCurveOpData::getDirection() const noexcept
{
    return m_direction;
}

void GradingHueCurveOpData::setDirection(TransformDirection dir) noexcept
{
    m_direction = dir;
}

bool GradingHueCurveOpData::isDynamic() const noexcept
{
    return m_value->isDynamic();
}

DynamicPropertyRcPtr GradingHueCurveOpData::getDynamicProperty() const noexcept
{
    return m_value;
}

void GradingHueCurveOpData::replaceDynamicProperty(DynamicPropertyGradingHueCurveImplRcPtr prop) noexcept
{
    m_value = prop;
}

void GradingHueCurveOpData::removeDynamicProperty() noexcept
{
    m_value->makeNonDynamic();
}

bool GradingHueCurveOpData::equals(const OpData & other) const
{
    if (!OpData::equals(other)) return false;

    const GradingHueCurveOpData* rop = static_cast<const GradingHueCurveOpData*>(&other);

    if (m_direction      != rop->m_direction ||
        m_style          != rop->m_style ||
        m_RGBToHSY       != rop->m_RGBToHSY ||
       !m_value->equals(  *(rop->m_value)  ))
    {
        return false;
    }

    return true;
}

bool operator==(const GradingHueCurveOpData & lhs, const GradingHueCurveOpData & rhs)
{
    return lhs.equals(rhs);
}

} // namespace OCIO_NAMESPACE
