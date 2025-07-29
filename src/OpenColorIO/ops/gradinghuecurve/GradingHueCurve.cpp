// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradinghuecurve/GradingHueCurve.h"

namespace OCIO_NAMESPACE
{

namespace
{

static const std::vector<GradingControlPoint> DefaultHueHueCtrl{ {0.0f, 0.0f}, 
                                                                 {1.f/6.f, 1.f/6.f}, 
                                                                 {2.f/6.f, 2.f/6.f}, 
                                                                 {0.5f, 0.5f},
                                                                 {4.f/6.f, 4.f/6.f}, 
                                                                 {5.f/6.f, 5.f/6.f}  };

static const std::vector<GradingControlPoint> DefaultHueSatCtrl{ {0.0f, 1.0f}, 
                                                                 {1.f/6.f, 1.0f}, 
                                                                 {2.f/6.f, 1.0f}, 
                                                                 {0.5f, 1.0f},
                                                                 {4.f/6.f, 1.0f}, 
                                                                 {5.f/6.f, 1.0f} };

static const std::vector<GradingControlPoint> DefaultHueFxCtrl{ {0.0f, 0.0f}, 
                                                                {1.f/6.f, 0.f}, 
                                                                {2.f/6.f, 0.f}, 
                                                                {0.5f, 0.f},
                                                                {4.f/6.f, 0.f}, 
                                                                {5.f/6.f, 0.f} };

static const std::vector<GradingControlPoint> DefaultLumSatCtrl{ {0.0f, 1.0f}, {0.5f, 1.0f}, {1.0f, 1.0f} };
static const std::vector<GradingControlPoint> DefaultLumSatLinCtrl{ {-7.0f, 1.0f}, {0.f, 1.0f}, {7.0f, 1.0f} };

static const std::vector<GradingControlPoint> DefaultSatSatCtrl{ {0.0f, 0.0f}, {0.5f, 0.5f}, {1.0f, 1.0f} };
static const std::vector<GradingControlPoint> DefaultSatLumCtrl{ {0.0f, 1.0f}, {0.5f, 1.0f}, {1.0f, 1.0f} };

static const std::vector<GradingControlPoint> DefaultLumLumCtrl{ { 0.f, 0.f },{ 0.5f, 0.5f },{ 1.0f, 1.0f } };
static const std::vector<GradingControlPoint> DefaultLumLumLinCtrl{ { -7.0f, -7.0f },{ 0.f, 0.f },{ 7.0f, 7.0f } };

} // anon

const GradingBSplineCurveImpl GradingHueCurveImpl::DefaultHueHue(DefaultHueHueCtrl, BSplineType::HUE_HUE_B_SPLINE );
const GradingBSplineCurveImpl GradingHueCurveImpl::DefaultHueSat(DefaultHueSatCtrl, BSplineType::PERIODIC_1_B_SPLINE );
const GradingBSplineCurveImpl GradingHueCurveImpl::DefaultHueLum(DefaultHueSatCtrl, BSplineType::PERIODIC_1_B_SPLINE ); // HUE_LUM use the same as HUE_SAT
const GradingBSplineCurveImpl GradingHueCurveImpl::DefaultHueFx(DefaultHueFxCtrl, BSplineType::PERIODIC_0_B_SPLINE );
const GradingBSplineCurveImpl GradingHueCurveImpl::DefaultLumSat(DefaultLumSatCtrl, BSplineType::HORIZONTAL1_B_SPLINE);
const GradingBSplineCurveImpl GradingHueCurveImpl::DefaultLumSatLin(DefaultLumSatLinCtrl, BSplineType::HORIZONTAL1_B_SPLINE);
const GradingBSplineCurveImpl GradingHueCurveImpl::DefaultSatSat(DefaultSatSatCtrl, BSplineType::DIAGONAL_B_SPLINE);
const GradingBSplineCurveImpl GradingHueCurveImpl::DefaultSatLum(DefaultSatLumCtrl, BSplineType::HORIZONTAL1_B_SPLINE);
const GradingBSplineCurveImpl GradingHueCurveImpl::DefaultLumLum(DefaultLumLumCtrl, BSplineType::DIAGONAL_B_SPLINE);
const GradingBSplineCurveImpl GradingHueCurveImpl::DefaultLumLumLin(DefaultLumLumLinCtrl, BSplineType::DIAGONAL_B_SPLINE);

const std::array<std::reference_wrapper<const GradingBSplineCurveImpl>, static_cast<size_t>(HUE_NUM_CURVES)> 
   GradingHueCurveImpl::DefaultCurvesLin( {  std::ref(DefaultHueHue),
                                             std::ref(DefaultHueSat),
                                             std::ref(DefaultHueLum),
                                             std::ref(DefaultLumSatLin),
                                             std::ref(DefaultSatSat),
                                             std::ref(DefaultLumLumLin),
                                             std::ref(DefaultSatLum),
                                             std::ref(DefaultHueFx) });


const std::array<std::reference_wrapper<const GradingBSplineCurveImpl>, static_cast<size_t>(HUE_NUM_CURVES)> 
   GradingHueCurveImpl::DefaultCurves(  {  std::ref(DefaultHueHue),
                                           std::ref(DefaultHueSat),
                                           std::ref(DefaultHueLum),
                                           std::ref(DefaultLumSat),
                                           std::ref(DefaultSatSat),
                                           std::ref(DefaultLumLum),
                                           std::ref(DefaultSatLum),
                                           std::ref(DefaultHueFx) });

GradingHueCurveImpl::GradingHueCurveImpl() : 
    GradingHueCurveImpl(GRADING_LOG)
{}

GradingHueCurveImpl::GradingHueCurveImpl(GradingStyle style)
{  
   auto & curves = (style == GRADING_LIN) ? GradingHueCurveImpl::DefaultCurvesLin:
                                            GradingHueCurveImpl::DefaultCurves;

   for (int c = 0; c < HUE_NUM_CURVES; ++c)
   {
       m_curves[c] = curves[c].get().createEditableCopy();
   }
}

GradingHueCurveImpl::GradingHueCurveImpl(
   ConstGradingBSplineCurveRcPtr hueHue,
   ConstGradingBSplineCurveRcPtr hueSat,
   ConstGradingBSplineCurveRcPtr hueLum,
   ConstGradingBSplineCurveRcPtr lumSat,
   ConstGradingBSplineCurveRcPtr satSat,
   ConstGradingBSplineCurveRcPtr lumLum,
   ConstGradingBSplineCurveRcPtr satLum,
   ConstGradingBSplineCurveRcPtr hueFx)
{
    m_curves[HUE_HUE] = hueHue->createEditableCopy();
    m_curves[HUE_SAT] = hueSat->createEditableCopy();
    m_curves[HUE_LUM] = hueLum->createEditableCopy();
    m_curves[LUM_SAT] = lumSat->createEditableCopy();
    m_curves[SAT_SAT] = satSat->createEditableCopy();
    m_curves[LUM_LUM] = lumLum->createEditableCopy();
    m_curves[SAT_LUM] = satLum->createEditableCopy();
    m_curves[HUE_FX] = hueFx->createEditableCopy();
}

GradingHueCurveImpl::GradingHueCurveImpl(const ConstGradingHueCurveRcPtr & rhs)
{
    auto impl = dynamic_cast<const GradingHueCurveImpl *>(rhs.get());
    if (impl)
    {
        for (int c = 0; c < HUE_NUM_CURVES; ++c)
        {
            m_curves[c] = impl->m_curves[c]->createEditableCopy();
        }
        m_drawCurveOnly = rhs->getDrawCurveOnly();
    }
}

GradingHueCurveRcPtr GradingHueCurveImpl::createEditableCopy() const
{
    auto newCurve = std::make_shared<GradingHueCurveImpl>();
    for (int c = 0; c < HUE_NUM_CURVES; ++c)
    {
        newCurve->m_curves[c] = m_curves[c]->createEditableCopy();
    }
    newCurve->setDrawCurveOnly(m_drawCurveOnly);

    GradingHueCurveRcPtr res = newCurve;
    return res;
}

namespace
{
const char * CurveTypeName(int c)
{
    const HueCurveType curveType = static_cast<HueCurveType>(c);
    switch (curveType)
    {
    case HUE_HUE:
        return "hue_hue";
    case HUE_SAT:
        return "hue_sat";
    case HUE_LUM:
        return "hue_lum";
    case LUM_SAT:
        return "lum_sat";
    case SAT_SAT:
        return "sat_sat";
    case LUM_LUM:
        return "lum_lum";
    case SAT_LUM:
        return "sat_lum";
    case HUE_FX:
        return "hue_fx";
    case HUE_NUM_CURVES:
    default:
        break;
    }
    return "illegal";
}
}

void GradingHueCurveImpl::validate() const
{
    for (int c = 0; c < HUE_NUM_CURVES; ++c)
    {
        try
        {
            m_curves[c]->validate();
        }
        catch (Exception & e)
        {
            std::ostringstream oss;
            oss << "GradingHueCurve validation failed for '" << CurveTypeName(c) << "' curve "
                << "with: " << e.what();
            throw Exception(oss.str().c_str());
        }

        // Unless drawCurveOnly is enabled, check that the spline type is correct for
        // the given hue curve type.
        if (!getDrawCurveOnly())
        {
            const BSplineType splineType = m_curves[c]->getSplineType();
            const HueCurveType hueType = static_cast<HueCurveType>(c);
            if (splineType != GetBSplineTypeForHueCurveType(hueType))
            {
                std::ostringstream oss;
                oss << "GradingHueCurve validation failed: '" << CurveTypeName(c) << "' curve "
                    << "is of the wrong BSplineType.";
                throw Exception(oss.str().c_str());
            }
        }
    }
}

bool GradingHueCurveImpl::isIdentity() const
{
    for (int c = 0; c < HUE_NUM_CURVES; ++c)
    {
        if (!IsGradingCurveIdentity(m_curves[c]))
        {
            return false;
        }
    }
    return true;
}

bool GradingHueCurveImpl::getDrawCurveOnly() const
{
    return m_drawCurveOnly;
}

void GradingHueCurveImpl::setDrawCurveOnly(bool drawCurveOnly)
{
    m_drawCurveOnly = drawCurveOnly;
}

bool GradingHueCurveImpl::isHueCurveTypeValid(HueCurveType c) const
{
    return ( c >= HUE_HUE && 
             c < HUE_NUM_CURVES );
}

ConstGradingBSplineCurveRcPtr GradingHueCurveImpl::getCurve(HueCurveType c) const
{
    if(!isHueCurveTypeValid(c))
    {
        throw Exception("The HueCurveType provided is illegal");
    }

    return m_curves[c];
}

GradingBSplineCurveRcPtr GradingHueCurveImpl::getCurve(HueCurveType c)
{
    if(!isHueCurveTypeValid(c))
    {
        throw Exception("The HueCurveType provided is illegal");
    }

    return m_curves[c];
}

BSplineType GradingHueCurve::GetBSplineTypeForHueCurveType(HueCurveType curveType)
{
    switch (curveType)
    {
    case HUE_HUE:
        return BSplineType::HUE_HUE_B_SPLINE;
    case HUE_SAT:
        return BSplineType::PERIODIC_1_B_SPLINE;
    case HUE_LUM:
        return BSplineType::PERIODIC_1_B_SPLINE;
    case LUM_SAT:
        return BSplineType::HORIZONTAL1_B_SPLINE;
    case SAT_SAT:
        return BSplineType::DIAGONAL_B_SPLINE;
    case LUM_LUM:
        return BSplineType::DIAGONAL_B_SPLINE;
    case SAT_LUM:
        return BSplineType::HORIZONTAL1_B_SPLINE;
    case HUE_FX:
        return BSplineType::PERIODIC_0_B_SPLINE;
    case HUE_NUM_CURVES:
    default:
        return BSplineType::B_SPLINE;
    }
}

GradingHueCurveRcPtr GradingHueCurve::Create(GradingStyle style)
{
    auto newCurve = std::make_shared<GradingHueCurveImpl>(style);
    GradingHueCurveRcPtr res = newCurve;
    return res;
}

GradingHueCurveRcPtr GradingHueCurve::Create(const ConstGradingHueCurveRcPtr & rhs)
{
    auto newCurve = std::make_shared<GradingHueCurveImpl>(rhs);
    GradingHueCurveRcPtr res = newCurve;
    return res;
}

GradingHueCurveRcPtr GradingHueCurve::Create(
   ConstGradingBSplineCurveRcPtr hueHue,
   ConstGradingBSplineCurveRcPtr hueSat,
   ConstGradingBSplineCurveRcPtr hueLum,
   ConstGradingBSplineCurveRcPtr lumSat,
   ConstGradingBSplineCurveRcPtr satSat,
   ConstGradingBSplineCurveRcPtr lumLum,
   ConstGradingBSplineCurveRcPtr satLum,
   ConstGradingBSplineCurveRcPtr hueFx)
{
    auto newCurve = std::make_shared<GradingHueCurveImpl>(
       hueHue,
       hueSat,
       hueLum,
       lumSat,
       satSat,
       lumLum,
       satLum,
       hueFx);

    newCurve->validate();

    GradingHueCurveRcPtr res = newCurve;
    return res;
}

bool operator==(const GradingHueCurve & lhs, const GradingHueCurve & rhs)
{

    for (int c = 0; c < HUE_NUM_CURVES; ++c)
    {
        if (*(lhs.getCurve(static_cast<HueCurveType>(c))) != *(rhs.getCurve(static_cast<HueCurveType>(c))))
        {
            return false;
        }
    }
    if (lhs.getDrawCurveOnly() != rhs.getDrawCurveOnly())
    {
        return false;
    }
    return true;
}

bool operator!=(const GradingHueCurve & lhs, const GradingHueCurve & rhs)
{
    return !(lhs == rhs);
}

} // namespace OCIO_NAMESPACE

