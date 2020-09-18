// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradingrgbcurve/GradingRGBCurve.h"

namespace OCIO_NAMESPACE
{
namespace
{
static const std::vector<GradingControlPoint> DefaultCtrl{ { 0.f, 0.f },{ 0.5f, 0.5f },{ 1.0f, 1.0f } };
static const std::vector<GradingControlPoint> DefaultCtrlLin{ { -7.0f, -7.0f },{ 0.f, 0.f },{ 7.0f, 7.0f } };
}

const GradingBSplineCurveImpl GradingRGBCurveImpl::Default(DefaultCtrl);
const GradingBSplineCurveImpl GradingRGBCurveImpl::DefaultLin(DefaultCtrlLin);

GradingRGBCurveImpl::GradingRGBCurveImpl(GradingStyle style)
{
    m_curves[RGB_RED] = (style == GRADING_LIN ? GradingRGBCurveImpl::DefaultLin.createEditableCopy() :
                                                GradingRGBCurveImpl::Default.createEditableCopy());
    m_curves[RGB_GREEN]  = m_curves[RGB_RED]->createEditableCopy();
    m_curves[RGB_BLUE]   = m_curves[RGB_RED]->createEditableCopy();
    m_curves[RGB_MASTER] = m_curves[RGB_RED]->createEditableCopy();
}

GradingRGBCurveImpl::GradingRGBCurveImpl(const ConstGradingBSplineCurveRcPtr & red,
                                         const ConstGradingBSplineCurveRcPtr & green,
                                         const ConstGradingBSplineCurveRcPtr & blue,
                                         const ConstGradingBSplineCurveRcPtr & master)
{
    if (!red || !green || !blue || !master)
    {
        throw Exception("All curves have to be defined");
    }
    m_curves[RGB_RED]    = red->createEditableCopy();
    m_curves[RGB_GREEN]  = green->createEditableCopy();
    m_curves[RGB_BLUE]   = blue->createEditableCopy();
    m_curves[RGB_MASTER] = master->createEditableCopy();
}

GradingRGBCurveImpl::GradingRGBCurveImpl(const ConstGradingRGBCurveRcPtr & rhs)
{
    auto impl = dynamic_cast<const GradingRGBCurveImpl *>(rhs.get());
    if (impl)
    {
        for (int c = 0; c < RGB_NUM_CURVES; ++c)
        {
            m_curves[c] = impl->m_curves[c]->createEditableCopy();
        }
    }
}

GradingRGBCurveRcPtr GradingRGBCurveImpl::createEditableCopy() const
{
    auto newCurve = std::make_shared<GradingRGBCurveImpl>(m_curves[RGB_RED],
                                                          m_curves[RGB_GREEN],
                                                          m_curves[RGB_BLUE],
                                                          m_curves[RGB_MASTER]);
    GradingRGBCurveRcPtr res = newCurve;
    return res;
}

namespace
{
const char * CurveType(int c)
{
    const RGBCurveType curve = static_cast<RGBCurveType>(c);
    switch (curve)
    {
    case RGB_RED:
        return "red";
    case RGB_GREEN:
        return "green";
    case RGB_BLUE:
        return "blue";
    case RGB_MASTER:
        return "master";

    case RGB_NUM_CURVES:
    default:
        break;
    }
    return "invalid";
}
}

void GradingRGBCurveImpl::validate() const
{
    for (int c = 0; c < RGB_NUM_CURVES; ++c)
    {
        try
        {
            m_curves[c]->validate();
        }
        catch (Exception & e)
        {
            std::ostringstream oss;
            oss << "GradingRGBCurve validation failed for '" << CurveType(c) << "' curve "
                << "with: " << e.what();
            throw Exception(oss.str().c_str());
        }
    }
}

bool GradingRGBCurveImpl::isIdentity() const
{
    for (int c = 0; c < RGB_NUM_CURVES; ++c)
    {
        if (!IsGradingCurveIdentity(m_curves[c]))
        {
            return false;
        }
    }
    return true;
}

ConstGradingBSplineCurveRcPtr GradingRGBCurveImpl::getCurve(RGBCurveType c) const
{
    if (c < 0 || c >= RGB_NUM_CURVES)
    {
        throw Exception("Invalid curve.");
    }
    return m_curves[c];
}

GradingBSplineCurveRcPtr GradingRGBCurveImpl::getCurve(RGBCurveType c)
{
    if (c < 0 || c >= RGB_NUM_CURVES)
    {
        throw Exception("Invalid curve.");
    }
    return m_curves[c];
}

GradingRGBCurveRcPtr GradingRGBCurve::Create(GradingStyle style)
{
    auto newCurve = std::make_shared<GradingRGBCurveImpl>(style);
    GradingRGBCurveRcPtr res = newCurve;
    return res;
}

GradingRGBCurveRcPtr GradingRGBCurve::Create(const ConstGradingRGBCurveRcPtr & rhs)
{
    auto newCurve = std::make_shared<GradingRGBCurveImpl>(rhs);
    GradingRGBCurveRcPtr res = newCurve;
    return res;
}

GradingRGBCurveRcPtr GradingRGBCurve::Create(const ConstGradingBSplineCurveRcPtr & red,
                                             const ConstGradingBSplineCurveRcPtr & green,
                                             const ConstGradingBSplineCurveRcPtr & blue,
                                             const ConstGradingBSplineCurveRcPtr & master)
{
    auto newCurve = std::make_shared<GradingRGBCurveImpl>(red, green, blue, master);
    GradingRGBCurveRcPtr res = newCurve;
    return res;
}

bool operator==(const GradingRGBCurve & lhs, const GradingRGBCurve & rhs)
{
    if (*(lhs.getCurve(RGB_RED)) == *(rhs.getCurve(RGB_RED)) &&
        *(lhs.getCurve(RGB_GREEN)) == *(rhs.getCurve(RGB_GREEN)) &&
        *(lhs.getCurve(RGB_BLUE)) == *(rhs.getCurve(RGB_BLUE)) &&
        *(lhs.getCurve(RGB_MASTER)) == *(rhs.getCurve(RGB_MASTER)))
    {
        return true;
    }
    return false;
}

bool operator!=(const GradingRGBCurve & lhs, const GradingRGBCurve & rhs)
{
    return !(lhs == rhs);
}


} // namespace OCIO_NAMESPACE

