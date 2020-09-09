// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "DynamicProperty.h"
#include "ops/gradingprimary/GradingPrimaryOpData.h"
#include "ops/gradingrgbcurve/GradingRGBCurve.h"
#include "ops/gradingtone/GradingToneOpData.h"

namespace OCIO_NAMESPACE
{

namespace DynamicPropertyValue
{
DynamicPropertyDoubleRcPtr AsDouble(DynamicPropertyRcPtr & prop)
{
    auto res = OCIO_DYNAMIC_POINTER_CAST<DynamicPropertyDouble>(prop);
    if (res) return res;
    throw Exception("Dynamic property value is not a double.");
}
DynamicPropertyGradingPrimaryRcPtr AsGradingPrimary(DynamicPropertyRcPtr & prop)
{
    auto res = OCIO_DYNAMIC_POINTER_CAST<DynamicPropertyGradingPrimary>(prop);
    if (res) return res;
    throw Exception("Dynamic property value is not a grading primary.");
}
DynamicPropertyGradingRGBCurveRcPtr AsGradingRGBCurve(DynamicPropertyRcPtr & prop)
{
    auto res = OCIO_DYNAMIC_POINTER_CAST<DynamicPropertyGradingRGBCurve>(prop);
    if (res) return res;
    throw Exception("Dynamic property value is not a grading RGB curve.");
}
DynamicPropertyGradingToneRcPtr AsGradingTone(DynamicPropertyRcPtr & prop)
{
    auto res = OCIO_DYNAMIC_POINTER_CAST<DynamicPropertyGradingTone>(prop);
    if (res) return res;
    throw Exception("Dynamic property value is not a grading tone.");
}
}

bool operator==(const DynamicProperty &lhs, const DynamicProperty &rhs)
{
    if (lhs.getType() != rhs.getType()) return false;

    DynamicPropertyImpl const * plhs = dynamic_cast<DynamicPropertyImpl const*>(&lhs);
    DynamicPropertyImpl const * prhs = dynamic_cast<DynamicPropertyImpl const*>(&rhs);
    if (plhs && prhs)
    {
        return plhs->equals(*prhs);
    }
    throw Exception("Unknown DynamicProperty implementation.");
}

DynamicPropertyImpl::DynamicPropertyImpl(DynamicPropertyType type, bool dynamic)
    : m_type(type)
    , m_isDynamic(dynamic)
{
}

bool DynamicPropertyImpl::equals(const DynamicPropertyImpl & rhs) const
{
    if (this == &rhs) return true;

    if (m_isDynamic == rhs.m_isDynamic && m_type == rhs.m_type)
    {
        if (!m_isDynamic)
        {
            // Both not dynamic, same value or not.
            switch (getType())
            {
            case DYNAMIC_PROPERTY_CONTRAST:
            case DYNAMIC_PROPERTY_EXPOSURE:
            case DYNAMIC_PROPERTY_GAMMA:
            {
                auto lhst = dynamic_cast<const DynamicPropertyDouble *>(this);
                auto rhst = dynamic_cast<const DynamicPropertyDouble *>(&rhs);
                return (lhst->getValue() == rhst->getValue());
            }
            case DYNAMIC_PROPERTY_GRADING_PRIMARY:
            {
                auto lhst = dynamic_cast<const DynamicPropertyGradingPrimary *>(this);
                auto rhst = dynamic_cast<const DynamicPropertyGradingPrimary *>(&rhs);
                return (lhst->getValue() == rhst->getValue());
            }
            case DYNAMIC_PROPERTY_GRADING_RGBCURVE:
            {
                auto lhst = dynamic_cast<const DynamicPropertyGradingRGBCurve *>(this);
                auto rhst = dynamic_cast<const DynamicPropertyGradingRGBCurve *>(&rhs);
                return (*lhst->getValue() == *rhst->getValue());
            }
            case DYNAMIC_PROPERTY_GRADING_TONE:
            {
                auto lhst = dynamic_cast<const DynamicPropertyGradingTone *>(this);
                auto rhst = dynamic_cast<const DynamicPropertyGradingTone *>(&rhs);
                return (lhst->getValue() == rhst->getValue());
            }
            }
            // Different values.
            return false;
        }
        else
        {
            // Both dynamic, will be same.
            return true;
        }
    }

    // One dynamic, not the other or different types.
    return false;
}

DynamicPropertyDoubleImpl::DynamicPropertyDoubleImpl(DynamicPropertyType type, double value,
                                                     bool dynamic)
    : DynamicPropertyImpl(type, dynamic)
    , m_value(value)
{
}

DynamicPropertyDoubleImplRcPtr DynamicPropertyDoubleImpl::createEditableCopy() const
{
    return std::make_shared<DynamicPropertyDoubleImpl>(getType(), getValue(), isDynamic());
}

DynamicPropertyGradingPrimaryImpl::DynamicPropertyGradingPrimaryImpl(const GradingPrimary & value,
                                                                     bool dynamic)
    : DynamicPropertyImpl(DYNAMIC_PROPERTY_GRADING_PRIMARY, dynamic)
    , m_value(value)
{
    precompute();
}

DynamicPropertyGradingPrimaryImplRcPtr DynamicPropertyGradingPrimaryImpl::createEditableCopy() const
{
    return std::make_shared<DynamicPropertyGradingPrimaryImpl>(getValue(), isDynamic());
}

void DynamicPropertyGradingPrimaryImpl::setValue(const GradingPrimary & value)
{
    value.validate();
    m_value = value;
    precompute();
}

void DynamicPropertyGradingPrimaryImpl::precompute()
{
    // Identity does not depend of the style (style only affects pivot).
    static const GradingPrimary identity{ GRADING_LOG };
    // Ignore pivots. Could ignore some values based on style, but style is not know here.
    m_localBypass = m_value.m_brightness == identity.m_brightness &&
                    m_value.m_clampBlack == identity.m_clampBlack &&
                    m_value.m_clampWhite == identity.m_clampWhite &&
                    m_value.m_contrast == identity.m_contrast &&
                    m_value.m_exposure == identity.m_exposure &&
                    m_value.m_gain == identity.m_gain &&
                    m_value.m_gamma == identity.m_gamma &&
                    m_value.m_lift == identity.m_lift &&
                    m_value.m_offset == identity.m_offset;

}

DynamicPropertyGradingRGBCurveImpl::DynamicPropertyGradingRGBCurveImpl(
    const ConstGradingRGBCurveRcPtr & value, bool dynamic)
    : DynamicPropertyImpl(DYNAMIC_PROPERTY_GRADING_RGBCURVE, dynamic)
{
    m_gradingRGBCurve = GradingRGBCurve::Create(value);
    // Convert control points from the UI into knots and coefficients for the apply.
    precompute();
}

const ConstGradingRGBCurveRcPtr & DynamicPropertyGradingRGBCurveImpl::getValue() const
{
    return m_gradingRGBCurve;
}

void DynamicPropertyGradingRGBCurveImpl::setValue(const ConstGradingRGBCurveRcPtr & value)
{
    value->validate();

    m_gradingRGBCurve = value->createEditableCopy();
    // Convert control points from the UI into knots and coefficients for the apply.
    precompute();
}

bool DynamicPropertyGradingRGBCurveImpl::getLocalBypass() const
{
    return m_knotsCoefs.m_localBypass;
}

int DynamicPropertyGradingRGBCurveImpl::getNumKnots() const
{
    return static_cast<int>(m_knotsCoefs.m_knotsArray.size());
}

int DynamicPropertyGradingRGBCurveImpl::getNumCoefs() const
{
    return static_cast<int>(m_knotsCoefs.m_coefsArray.size());
}

const int * DynamicPropertyGradingRGBCurveImpl::getKnotsOffsetsArray() const
{
    return m_knotsCoefs.m_knotsOffsetsArray.data();
}

const int * DynamicPropertyGradingRGBCurveImpl::getCoefsOffsetsArray() const
{
    return m_knotsCoefs.m_coefsOffsetsArray.data();
}

const float * DynamicPropertyGradingRGBCurveImpl::getKnotsArray() const
{
    return m_knotsCoefs.m_knotsArray.data();
}

const float * DynamicPropertyGradingRGBCurveImpl::getCoefsArray() const
{
    return m_knotsCoefs.m_coefsArray.data();
}

unsigned int DynamicPropertyGradingRGBCurveImpl::GetMaxKnots()
{
    return GradingBSplineCurveImpl::KnotsCoefs::MAX_NUM_KNOTS;
}

unsigned int DynamicPropertyGradingRGBCurveImpl::GetMaxCoefs()
{
    return GradingBSplineCurveImpl::KnotsCoefs::MAX_NUM_COEFS;
}

void DynamicPropertyGradingRGBCurveImpl::precompute()
{
    m_knotsCoefs.m_localBypass = false;
    m_knotsCoefs.m_knotsArray.resize(0);
    m_knotsCoefs.m_coefsArray.resize(0);

    // Compute knots and coefficients for each control point and pack all knots and coefs of
    // all curves in one knots array and one coef array, using an offset array to find specific
    // curve data.
    for (const auto c : { RGB_RED, RGB_GREEN, RGB_BLUE, RGB_MASTER })
    {
        ConstGradingBSplineCurveRcPtr curve = m_gradingRGBCurve->getCurve(c);
        auto curveImpl = dynamic_cast<const GradingBSplineCurveImpl *>(curve.get());
        curveImpl->computeKnotsAndCoefs(m_knotsCoefs, static_cast<int>(c));
    }
    if (m_knotsCoefs.m_knotsArray.empty()) m_knotsCoefs.m_localBypass = true;
}

DynamicPropertyGradingRGBCurveImplRcPtr DynamicPropertyGradingRGBCurveImpl::createEditableCopy() const
{
    auto res = std::make_shared<DynamicPropertyGradingRGBCurveImpl>(getValue(), isDynamic());
    res->m_knotsCoefs = m_knotsCoefs;
    return res;
}

DynamicPropertyGradingToneImpl::DynamicPropertyGradingToneImpl(const GradingTone & value,
                                                               GradingStyle style,
                                                               bool dynamic)
    : DynamicPropertyImpl(DYNAMIC_PROPERTY_GRADING_TONE, dynamic)
    , m_value(value)
    , m_computed(style)
{
    m_computed.computeValues(m_value);
}

DynamicPropertyGradingToneImpl::DynamicPropertyGradingToneImpl(const GradingTone & value,
                                                               const GradingTonePreRender & comp,
                                                               bool dynamic)
    : DynamicPropertyImpl(DYNAMIC_PROPERTY_GRADING_TONE, dynamic)
    , m_value(value)
    , m_computed(comp)
{
}

DynamicPropertyGradingToneImplRcPtr DynamicPropertyGradingToneImpl::createEditableCopy() const
{
    return std::make_shared<DynamicPropertyGradingToneImpl>(m_value, m_computed, isDynamic());
}

void DynamicPropertyGradingToneImpl::setValue(const GradingTone & value)
{
    value.validate();

    m_value = value;
    m_computed.computeValues(m_value);
}

void DynamicPropertyGradingToneImpl::useStyle(GradingStyle style)
{
    // Reset values to style defaults.
    m_value = GradingTone(style);
    m_computed.updateForStyle(style, m_value);
}

} // namespace OCIO_NAMESPACE
