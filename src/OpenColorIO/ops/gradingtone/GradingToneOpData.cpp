// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "ops/gradingtone/GradingTone.h"
#include "ops/gradingtone/GradingToneOpData.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

namespace DefaultValues
{
const std::streamsize FLOAT_DECIMALS = 7;
}

GradingToneOpData::GradingToneOpData(GradingStyle style)
    : m_style(style)
    , m_value(std::make_shared<DynamicPropertyGradingToneImpl>(GradingTone(style), style, false))
{
}

GradingToneOpData::GradingToneOpData(const GradingToneOpData & other)
    : m_style(other.m_style)
    , m_value(std::make_shared<DynamicPropertyGradingToneImpl>(GradingTone(other.m_style), other.m_style, false))
{
    *this = other;
}

GradingToneOpData & GradingToneOpData::operator=(const GradingToneOpData & rhs)
{
    if (this == &rhs) return *this;

    OpData::operator=(rhs);

    m_style     = rhs.m_style;
    m_direction = rhs.m_direction;

    // Copy dynamic properties. Sharing happens when needed, with CPUop for instance.
    m_value->setValue(rhs.m_value->getValue());
    if (rhs.m_value->isDynamic())
    {
        m_value->makeDynamic();
    }

    return *this;
}

GradingToneOpData::~GradingToneOpData()
{
}

GradingToneOpDataRcPtr GradingToneOpData::clone() const
{
    return std::make_shared<GradingToneOpData>(*this);
}

void GradingToneOpData::validate() const
{
    // This should already be valid.
    m_value->getValue().validate();
}

bool GradingToneOpData::isNoOp() const
{
    return isIdentity();
}

bool GradingToneOpData::isIdentity() const
{
    if (isDynamic()) return false;

    auto & value = m_value->getValue();

    return IsIdentity(value);
}

bool GradingToneOpData::isInverse(ConstGradingToneOpDataRcPtr & r) const
{
    if (isDynamic() || r->isDynamic())
    {
        return false;
    }

    if (m_style == r->m_style && m_value->equals(*r->m_value))
    {
        if (CombineTransformDirections(getDirection(), r->getDirection()) == TRANSFORM_DIR_INVERSE)
        {
            return true;
        }
    }
    return false;
}

GradingToneOpDataRcPtr GradingToneOpData::inverse() const
{
    auto res = clone();
    res->m_direction = GetInverseTransformDirection(m_direction);
    return res;
}

std::string GradingToneOpData::getCacheID() const
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
    if (!isDynamic())
    {
        cacheIDStream << m_value->getValue();
    }

    return cacheIDStream.str();
}

void GradingToneOpData::setStyle(GradingStyle style) noexcept
{
    if (m_style != style)
    {
        m_style = style;
        // Reset value to default when style is changing.
        m_value->setStyle(style);
    }
}

TransformDirection GradingToneOpData::getDirection() const noexcept
{
    return m_direction;
}

void GradingToneOpData::setDirection(TransformDirection dir) noexcept
{
    m_direction = dir;
}

bool GradingToneOpData::isDynamic() const noexcept
{
    return m_value->isDynamic();
}

DynamicPropertyRcPtr GradingToneOpData::getDynamicProperty() const noexcept
{
    return m_value;
}

void GradingToneOpData::replaceDynamicProperty(DynamicPropertyGradingToneImplRcPtr prop) noexcept
{
    m_value = prop;
}

void GradingToneOpData::removeDynamicProperty() noexcept
{
    m_value->makeNonDynamic();
}

bool GradingToneOpData::operator==(const OpData & other) const
{
    if (!OpData::operator==(other)) return false;

    const GradingToneOpData* rop = static_cast<const GradingToneOpData*>(&other);

    if (m_direction         != rop->m_direction ||
        m_style             != rop->m_style ||
       !m_value->equals(     *(rop->m_value) ))
    {
        return false;
    }

    return true;
}


} // namespace OCIO_NAMESPACE

