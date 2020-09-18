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
    , m_value(std::make_shared<DynamicPropertyGradingPrimaryImpl>(GradingPrimary(style), false))
{
}

GradingPrimaryOpData::GradingPrimaryOpData(const GradingPrimaryOpData & rhs)
    : m_style(rhs.m_style)
    , m_value(std::make_shared<DynamicPropertyGradingPrimaryImpl>(GradingPrimary(rhs.m_style), false))
{
    *this = rhs;
}

GradingPrimaryOpData & GradingPrimaryOpData::operator=(const GradingPrimaryOpData & rhs)
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
    m_value->getValue().validate();
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
    // This function is used to optimimize ops in a processor, if both are dynamic their values
    // will be the same after dynamic properties are unified. Equals compares dynamic properties.
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
    res->m_direction = GetInverseTransformDirection(m_direction);
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
        GradingPrimary reset{ style };
        m_value->setValue(reset);
    }
}

TransformDirection GradingPrimaryOpData::getDirection() const noexcept
{
    return m_direction;
}

void GradingPrimaryOpData::setDirection(TransformDirection dir) noexcept
{
    m_direction = dir;
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

    if (m_direction != rop->m_direction ||
        m_style     != rop->m_style ||
        !(*m_value == *(rop->m_value)))
    {
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool operator==(const GradingRGBM & lhs, const GradingRGBM & rhs)
{
    return lhs.m_red == rhs.m_red && lhs.m_green == rhs.m_green &&
           lhs.m_blue == rhs.m_blue && lhs.m_master == rhs.m_master;
}

bool operator!=(const GradingRGBM & lhs, const GradingRGBM & rhs)
{
    return !(lhs == rhs);
}

bool operator==(const GradingPrimary & lhs, const GradingPrimary & rhs)
{
    return lhs.m_brightness  == rhs.m_brightness  &&
           lhs.m_contrast    == rhs.m_contrast    &&
           lhs.m_gamma       == rhs.m_gamma       &&
           lhs.m_offset      == rhs.m_offset      &&
           lhs.m_exposure    == rhs.m_exposure    &&
           lhs.m_lift        == rhs.m_lift        &&
           lhs.m_gain        == rhs.m_gain        &&
           lhs.m_pivot       == rhs.m_pivot       &&
           lhs.m_saturation  == rhs.m_saturation  &&
           lhs.m_clampWhite == rhs.m_clampWhite &&
           lhs.m_clampBlack == rhs.m_clampBlack &&
           lhs.m_pivotWhite == rhs.m_pivotWhite &&
           lhs.m_pivotBlack == rhs.m_pivotBlack;
}

bool operator!=(const GradingPrimary & lhs, const GradingPrimary & rhs)
{
    return !(lhs == rhs);
}

double GradingPrimary::NoClampBlack()
{
    // Note that this is not a magic number, renderers do rely on this value.
    return -std::numeric_limits<double>::max();
}

double GradingPrimary::NoClampWhite()
{
    // Note that this is not a magic number, renderers do rely on this value.
    return std::numeric_limits<double>::max();
}

void GradingPrimary::validate() const
{
    // Validating all values, even if some or not used for a given style.

    static constexpr double GradingPrimaryGammaLowerBound = 0.01;
    static constexpr double GradingPrimaryBoundError = 0.000001;
    static constexpr double GradingPrimaryGammaMin = GradingPrimaryGammaLowerBound -
                                                     GradingPrimaryBoundError;

    if (m_gamma.m_red < GradingPrimaryGammaMin   ||
        m_gamma.m_green < GradingPrimaryGammaMin ||
        m_gamma.m_blue < GradingPrimaryGammaMin  ||
        m_gamma.m_master < GradingPrimaryGammaMin)
    {
        std::ostringstream oss;
        oss << "GradingTone gamma '" << m_gamma << "' are below lower bound ("
            << GradingPrimaryGammaLowerBound << ").";
        throw Exception(oss.str().c_str());
    }

    if (m_pivotBlack > m_pivotWhite)
    {
        throw Exception("GradingPrimary black pivot should be smaller than white pivot.");
    }

    if (m_clampBlack > m_clampWhite)
    {
        throw Exception("GradingPrimary black clamp should be smaller than white clamp.");
    }
}

} // namespace OCIO_NAMESPACE
