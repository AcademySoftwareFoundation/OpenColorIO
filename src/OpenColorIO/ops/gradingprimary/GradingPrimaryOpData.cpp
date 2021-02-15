// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradingprimary/GradingPrimaryOpData.h"
#include "ops/range/RangeOpData.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

namespace DefaultValues
{
const std::streamsize FLOAT_DECIMALS = 7;
}

GradingPrimaryOpData::GradingPrimaryOpData(GradingStyle style)
    : m_style(style)
    , m_value(std::make_shared<DynamicPropertyGradingPrimaryImpl>(style,
                                                                  TRANSFORM_DIR_FORWARD,
                                                                  GradingPrimary(style),
                                                                  false))
{
}

GradingPrimaryOpData::GradingPrimaryOpData(const GradingPrimaryOpData & rhs)
    : m_style(rhs.m_style)
    , m_value(std::make_shared<DynamicPropertyGradingPrimaryImpl>(rhs.m_style,
                                                                  TRANSFORM_DIR_FORWARD,
                                                                  GradingPrimary(rhs.m_style),
                                                                  false))
{
    *this = rhs;
}

GradingPrimaryOpData & GradingPrimaryOpData::operator=(const GradingPrimaryOpData & rhs)
{
    if (this == &rhs) return *this;

    OpData::operator=(rhs);

    m_style     = rhs.m_style;

    // Copy dynamic properties. Sharing happens when needed, with CPUop for instance.
    m_value->setDirection(rhs.m_value->getDirection());
    m_value->setValue(rhs.m_value->getValue());
    if (rhs.m_value->isDynamic())
    {
        m_value->makeDynamic();
    }

    return *this;
}

GradingPrimaryOpData::~GradingPrimaryOpData()
{
}

GradingPrimaryOpDataRcPtr GradingPrimaryOpData::clone() const
{
    return std::make_shared<GradingPrimaryOpData>(*this);
}

void GradingPrimaryOpData::validate() const
{
    // This should already be valid.
    m_value->getValue().validate(m_style);
}

bool GradingPrimaryOpData::isNoOp() const
{
    return isIdentity();
}

bool GradingPrimaryOpData::isIdentity() const
{
    if (isDynamic()) return false;

    const GradingPrimary defaultValues{ m_style };
    auto & values = m_value->getValue();

    if (defaultValues.m_saturation == values.m_saturation  &&
        defaultValues.m_clampBlack == values.m_clampBlack &&
        defaultValues.m_clampWhite == values.m_clampWhite)
    {
        switch (m_style)
        {
        case GRADING_LOG:
            if (defaultValues.m_pivotBlack == values.m_pivotBlack &&
                defaultValues.m_pivotWhite == values.m_pivotWhite &&
                defaultValues.m_brightness == values.m_brightness   &&
                defaultValues.m_contrast   == values.m_contrast     &&
                defaultValues.m_gamma      == values.m_gamma)
            {
                // Pivot value can be ignored if other values are identity.
                return true;
            }
            break;
        case GRADING_LIN:
            if (defaultValues.m_contrast == values.m_contrast &&
                defaultValues.m_offset   == values.m_offset   &&
                defaultValues.m_exposure == values.m_exposure)
            {
                // Pivot value can be ignored if other values are identity.
                return true;
            }
            break;
        case GRADING_VIDEO:
            if (defaultValues.m_gamma      == values.m_gamma      &&
                defaultValues.m_offset     == values.m_offset     &&
                defaultValues.m_lift       == values.m_lift       &&
                defaultValues.m_gain       == values.m_gain)
            {
                // PivotBlack/White value can be ignored if other values are identity.
                return true;
            }
            break;
        }
    }
    return false;
}

OpDataRcPtr GradingPrimaryOpData::getIdentityReplacement() const
{
    auto & values = m_value->getValue();
    double clampLow = values.m_clampBlack;
    bool lowEmpty = false;
    bool highEmpty = false;
    if (clampLow == GradingPrimary::NoClampBlack())
    {
        clampLow = RangeOpData::EmptyValue();
        lowEmpty = true;
    }
    double clampHigh = values.m_clampWhite;
    if (clampHigh == GradingPrimary::NoClampWhite())
    {
        clampHigh = RangeOpData::EmptyValue();
        highEmpty = true;
    }

    if (lowEmpty && highEmpty)
    {
        return std::make_shared<MatrixOpData>();
    }
    return std::make_shared<RangeOpData>(clampLow,
                                         clampHigh,
                                         clampLow,
                                         clampHigh);
}

bool GradingPrimaryOpData::hasChannelCrosstalk() const
{
    auto & values = m_value->getValue();
    return values.m_saturation != 1.;
}

bool GradingPrimaryOpData::isInverse(ConstGradingPrimaryOpDataRcPtr & r) const
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

GradingPrimaryOpDataRcPtr GradingPrimaryOpData::inverse() const
{
    auto res = clone();
    const auto newDir = GetInverseTransformDirection(getDirection());
    res->m_value->setDirection(newDir);
    return res;
}

std::string GradingPrimaryOpData::getCacheID() const
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

void GradingPrimaryOpData::setStyle(GradingStyle style) noexcept
{
    if (style != m_style)
    {
        m_style = style;
        // Reset value to default when style is changing.
        m_value->setStyle(style);
    }
}

TransformDirection GradingPrimaryOpData::getDirection() const noexcept
{
    return m_value->getDirection();
}

void GradingPrimaryOpData::setDirection(TransformDirection dir) noexcept
{
    m_value->setDirection(dir);
}

bool GradingPrimaryOpData::isDynamic() const noexcept
{
    return m_value->isDynamic();
}

DynamicPropertyRcPtr GradingPrimaryOpData::getDynamicProperty() const noexcept
{
    return m_value;
}

void GradingPrimaryOpData::replaceDynamicProperty(DynamicPropertyGradingPrimaryImplRcPtr prop) noexcept
{
    m_value = prop;
}

void GradingPrimaryOpData::removeDynamicProperty() noexcept
{
    m_value->makeNonDynamic();
}

bool GradingPrimaryOpData::operator==(const OpData & other) const
{
    if (!OpData::operator==(other)) return false;

    const GradingPrimaryOpData* rop = static_cast<const GradingPrimaryOpData*>(&other);

    if (m_style                 != rop->m_style ||
        m_value->getDirection() != rop->getDirection() ||
       !m_value->equals(         *(rop->m_value) ))
    {
        return false;
    }

    return true;
}


} // namespace OCIO_NAMESPACE
