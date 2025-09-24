// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>
#include "OpenColorIO/DynamicProperty.h"

#include "GpuShaderUtils.h"
#include "ops/gradingrgbcurve/GradingBSplineCurve.h"

namespace OCIO_NAMESPACE
{

namespace
{

//------------------------------------------------------------------------------------------------
//
void PrepHueCurveData(const std::vector<GradingControlPoint>& ctrlPnts,
                      std::vector<GradingControlPoint>& outCtrlPnts,
                      bool isPeriodic,
                      bool isHorizontal)
 {
     size_t numCtrlPnts = ctrlPnts.size();     
     for (unsigned i = 0; i < numCtrlPnts; ++i)
     {
         const float xval = ctrlPnts[ i ].m_x;
         const float yval = ctrlPnts[ i ].m_y;
         // Wrap periodic x values into [0,1).
         if (isPeriodic && (xval < 0.f))
         {
             outCtrlPnts.push_back(GradingControlPoint(xval + 1.f, isHorizontal ? yval : yval + 1.f));
         }
         else if (isPeriodic && (xval >= 1.f))
         {
             outCtrlPnts.push_back(GradingControlPoint(xval - 1.f, isHorizontal ? yval : yval - 1.f));
         }
         else
         {
             outCtrlPnts.push_back(GradingControlPoint(xval, yval));
         }
     }
  
     // Sort x and y based on x order.
     for (unsigned i = 0; i < numCtrlPnts; ++i)
     {
         unsigned min_index = i;
         float min_val = outCtrlPnts[i].m_x;
         for (unsigned j = i + 1; j < numCtrlPnts; ++j)
         {
           if (outCtrlPnts[j].m_x < min_val)
           {
               min_val = outCtrlPnts[j].m_x;
               min_index = j;
           }
         }
    
         std::swap( outCtrlPnts[i], outCtrlPnts[min_index] );
     }
  
     // Ensure that there is a minimum space between the x values.
     const float tol = 2e-3f;
     const float x_span = outCtrlPnts[numCtrlPnts - 1].m_x - outCtrlPnts[0].m_x;
     for (unsigned i = 1; i < outCtrlPnts.size(); ++i)
     {
       if ( (outCtrlPnts[i].m_x - outCtrlPnts[i - 1].m_x) < x_span * tol )
       {
           outCtrlPnts[i].m_x = outCtrlPnts[i - 1].m_x + x_span * tol;
       }
     }
     if (!isHorizontal)
     {
         // Ensure that there is a minimum space between the y values.
         const float y_span = outCtrlPnts[numCtrlPnts - 1].m_y - outCtrlPnts[0].m_y;
         for (unsigned i = 1; i < outCtrlPnts.size(); ++i)
         {
             if ( (outCtrlPnts[i].m_y - outCtrlPnts[i - 1].m_y) < y_span * tol )
             {
                 outCtrlPnts[i].m_y = outCtrlPnts[i - 1].m_y + y_span * tol;
             }
         }
     }

     if (isPeriodic)
     {
         // Copy a value from each side and wrap it around to the other side.
         GradingControlPoint firstCtrlPnt = outCtrlPnts[numCtrlPnts - 1];
         firstCtrlPnt.m_x -= 1.f;
         firstCtrlPnt.m_y = isHorizontal ? firstCtrlPnt.m_y : firstCtrlPnt.m_y - 1.f;
         outCtrlPnts.insert(outCtrlPnts.begin(), firstCtrlPnt);
         
         GradingControlPoint lastCtrlPnt = outCtrlPnts[1];
         lastCtrlPnt.m_x += 1.f;
         lastCtrlPnt.m_y = isHorizontal ? lastCtrlPnt.m_y : lastCtrlPnt.m_y + 1.f;
         outCtrlPnts.push_back(lastCtrlPnt);
     }
 }

//------------------------------------------------------------------------------------------------
//
float CalcKsi(unsigned i,
              const std::vector<GradingControlPoint>& outCtrlPnts,
              const std::vector<float>& slopes)
{
    const GradingControlPoint& p0 = outCtrlPnts[i];
    const GradingControlPoint& p1 = outCtrlPnts[i + 1];

    const float k = 0.2f;
    const float dx = p1.m_x - p0.m_x;
    const float secantSlope = (p1.m_y - p0.m_y) / dx;
    float secant = secantSlope;
    float m0 = slopes[i];
    float m1 = slopes[i + 1];
    if (secant < 0.f)
    {
        m0 = -slopes[i];  m1 = -slopes[i + 1];
        secant = -secant;
    }

    const float x_mid = p0.m_x + 0.5f * dx;
    const float left_bnd = p0.m_x + dx * k;
    const float right_bnd = p1.m_x - dx * k;
    float top_bnd = left_bnd;
    float bottom_bnd = right_bnd;
    float m_min = m0;
    float m_max = m1;
    if (m0 > m1)
    {
        m_max = m0;  m_min = m1;
        top_bnd = right_bnd;  bottom_bnd = left_bnd;
    }

    const float dm = m_max - m_min;
    const float b = 1.f - 0.5f * k;
    const float b_high = m_min + b * dm;
    const float b_low = m_min + (1.f - b) * dm;
    const float bbb = m_max * 4.f;
    const float bb = m_max * 1.1f;
    const float m_rel_diff = dm / std::max(0.01f, m_max);
    const float alpha = std::max( 0.f, std::min( (m_rel_diff - 0.05f) / (0.75f - 0.05f), 1.f ) );
    top_bnd = x_mid + alpha * (top_bnd - x_mid);
    bottom_bnd = x_mid + alpha * (bottom_bnd - x_mid);

    // Calculate the middle knot.
    float ksi = 0.f;
    if (secant >= bbb)
    {
        ksi = x_mid;
    }
    else if (secant > bb)
    {
        const float blend = (secant - bb) / (bbb - bb);
        ksi = top_bnd + blend * (x_mid - top_bnd);
    }
    else if (secant >= b_high)
    {
        ksi = top_bnd;
    }
    else if ((secant > b_low) && (b_high != b_low))
    {
        const float blend = (secant - b_low) / (b_high - b_low);
        ksi = bottom_bnd + blend * (top_bnd - bottom_bnd);
    }
    else
    {
        ksi = bottom_bnd;
    }
    return ksi;
}

//------------------------------------------------------------------------------------------------
//
void FitHueSpline(const std::vector<GradingControlPoint>& ctrlPnts,
                  const std::vector<float>& slopes,
                  std::vector<float>& knots,
                  std::vector<float>& coefsA,
                  std::vector<float>& coefsB,
                  std::vector<float>& coefsC)
{
    knots.push_back( ctrlPnts[0].m_x );
    size_t numCtrlPnts = ctrlPnts.size();
    for (unsigned i = 0; i < numCtrlPnts - 1; ++i)
    {
        const GradingControlPoint& p0 = ctrlPnts[i];
        const GradingControlPoint& p1 = ctrlPnts[i + 1];
    
        const float dx = p1.m_x - p0.m_x;
        const float secantSlope = (p1.m_y - p0.m_y) / dx;
    
        if ( fabsf( (slopes[i] + slopes[i + 1]) - 2.f * secantSlope ) <= 1e-5f )
        {
            coefsC.push_back( p0.m_y );
            coefsB.push_back( slopes[i] );
            coefsA.push_back( 0.5f * (slopes[i + 1] - slopes[i]) / dx );
        }
        else
        {
            // Calculate the middle knot.
            const float ksi = CalcKsi(i, ctrlPnts, slopes);
      
            // Calculate the coefficients.
            const float m_bar = (2.f * secantSlope - slopes[i + 1]) +
                                (slopes[i + 1] - slopes[i]) * (ksi - p0.m_x) / (p1.m_x - p0.m_x);
            const float eta = (m_bar - slopes[i]) / (ksi - p0.m_x);
            coefsC.push_back( p0.m_y );
            coefsB.push_back( slopes[i] );
            coefsA.push_back( 0.5f * eta );
            coefsC.push_back( p0.m_y + slopes[i] * (ksi - p0.m_x) + 0.5f * eta * (ksi - p0.m_x) * (ksi - p0.m_x) );
            coefsB.push_back( m_bar );
            coefsA.push_back( 0.5f * (slopes[i + 1] - m_bar) / (p1.m_x - ksi) );
            knots.push_back( ksi );
        }
    
        knots.push_back( p1.m_x );
    }
}
 
//------------------------------------------------------------------------------------------------
//
void EstimateHueSlopes(const std::vector<GradingControlPoint>& ctrlPnts,
                       std::vector<float>& slopes,
                       bool isPeriodic,
                       bool isHorizontal)
{
    slopes.clear();
    size_t numCtrlPnts = ctrlPnts.size();
    std::vector<float> secantSlope;
    std::vector<float> secantLen;
    for (unsigned i = 0; i < numCtrlPnts - 1; ++i)
    {
        const GradingControlPoint& p0 = ctrlPnts[i];
        const GradingControlPoint& p1 = ctrlPnts[i + 1];
    
        const float del_x = p1.m_x - p0.m_x;  // PrepHueCurveData ensures this is > 0
        const float del_y = p1.m_y - p0.m_y;
        secantSlope.push_back( del_y / del_x );
        secantLen.push_back( sqrt( del_x * del_x + del_y * del_y ) );
    }

    if (numCtrlPnts == 2)
    {
        slopes.push_back( secantSlope[0] );
        slopes.push_back( secantSlope[0] );
        return;
    }

    slopes.push_back(0.f);

    if (isHorizontal) // All horizontal curves and diagonal hue-hue.
    {
        for (unsigned i = 1; i < numCtrlPnts - 1; ++i)
        {
            float s = 0.f;
            float denom = secantSlope[i] + secantSlope[i - 1];
            if (fabsf(denom) < 1e-3f)
            {
                const float minval = denom < 0.f ? -1e-3f : 1e-3f;
                s = 2.f * secantSlope[i] * secantSlope[i - 1] / minval;
            }
            else
            {
                s = 2.f * secantSlope[i] * secantSlope[i - 1] / denom;
            }
            // Set slope to zero at flat areas or extrema.
            if ( secantSlope[i] * secantSlope[i - 1] <= 0.f )
            {
                s = 0.f;
            }
            slopes.push_back( s );
        }
        slopes.push_back( 0.5f * ( 3.f * secantSlope[numCtrlPnts - 2] - slopes[numCtrlPnts - 2] ) );
        slopes[0] = 0.5f * ( 3.f * secantSlope[0] - slopes[1] );
    }
    else // Diagonal curves except hue-hue (LvL and SvS).
    {
        unsigned i = 0;
        while (true)
        {
            unsigned j = i;
            float DL = secantLen[i];
            while ( ( j < numCtrlPnts - 2 ) && ( fabsf( secantSlope[j + 1] - secantSlope[j] ) < 1e-6f ) )
            {
                DL += secantLen[ j + 1 ];
                j++;
            }
            for (unsigned k = i; k <= j; ++k)
                secantLen[k] = DL;
            if (j >= numCtrlPnts - 3)
                break;
            i = j + 1;
        }
    
        for (unsigned k = 1; k < numCtrlPnts - 1; ++k)
        {
            const float s = ( secantLen[k] * secantSlope[k] + secantLen[k - 1] * secantSlope[k - 1] ) / 
                            ( secantLen[k] + secantLen[k - 1] );
            slopes.push_back( s );
        }
    
        const float minSlope = 0.01f;
        slopes.push_back( std::max(minSlope, 0.5f * ( 3.f * secantSlope[numCtrlPnts - 2] - slopes[numCtrlPnts - 2] )) );
        slopes[0] = std::max(minSlope, 0.5f * ( 3.f * secantSlope[0] - slopes[1] ));
    }

  // Adjust slopes that are not shape-preserving.
  for (unsigned i = 0; i < numCtrlPnts - 1; ++i)
  {
      float k = 0.2f;
      if (fabsf(slopes[i]) > fabsf(slopes[i+1]))
          k = 1.f - k;
      const float m_near_min = slopes[i] + k * (slopes[i + 1] - slopes[i]);
      float scale = 1.f;
      if (m_near_min != 0.f)
          scale = 0.75f * 2.f * secantSlope[i] / m_near_min;
      if (scale < 1.f)
      {
          slopes[i] = scale * slopes[i];
          slopes[i + 1] = scale * slopes[i + 1];
      }
  }

  // Copy end slopes from the opposite side.
  if (isPeriodic)
  {
      slopes[0] = slopes[numCtrlPnts - 2];
      slopes[numCtrlPnts - 1] = slopes[1];
  }
}

//------------------------------------------------------------------------------------------------
//
void EstimateRGBSlopes(const std::vector<GradingControlPoint> & ctrlPnts, 
                       std::vector<float> & slopes)
{
    std::vector<float> secantSlope;
    std::vector<float> secantLen;
    const size_t numCtrlPnts = ctrlPnts.size();
    for (size_t i = 0; i < numCtrlPnts - 1; ++i)
    {
        const float del_x = ctrlPnts[i + 1].m_x - ctrlPnts[i].m_x;
        const float del_y = ctrlPnts[i + 1].m_y - ctrlPnts[i].m_y;
        secantSlope.push_back(del_y / del_x);
        secantLen.push_back(std::sqrt(del_x * del_x + del_y * del_y));
    }
    if (numCtrlPnts == 2)
    {
        slopes.push_back(secantSlope[0]);
        slopes.push_back(secantSlope[0]);
        return;
    }
    size_t i = 0;
    while (true)
    {
        size_t j = i;
        float DL = secantLen[i];
        while ((j < numCtrlPnts - 2) && (std::fabs(secantSlope[j + 1] - secantSlope[j]) < 1e-6f))
        {
            DL += secantLen[j + 1];
            j++;
        }
        for (size_t k = i; k <= j; ++k)
        {
            secantLen[k] = DL;
        }
        if (j >= numCtrlPnts - 3)
        {
            break;
        }
        i = j + 1;
    }
    slopes.push_back(0.f);
    for (size_t k = 1; k < numCtrlPnts - 1; ++k)
    {
        const float s = (secantLen[k] * secantSlope[k] + secantLen[k - 1] * secantSlope[k - 1]) /
                        (secantLen[k] + secantLen[k - 1]);
        slopes.push_back(s);
    }
    slopes.push_back(std::max(0.01f, 0.5f * (3.f * secantSlope[numCtrlPnts - 2] -
                                     slopes[numCtrlPnts - 2])));
    slopes[0] = std::max(0.01f, 0.5f * (3.f * secantSlope[0] - slopes[1]));
}

//------------------------------------------------------------------------------------------------
//
void FitRGBSpline(const std::vector<GradingControlPoint> & ctrlPnts,
                  const std::vector<float> & slopes,
                  std::vector<float> & knots,
                  std::vector<float> & coefsA,
                  std::vector<float> & coefsB,
                  std::vector<float> & coefsC)
{
    const size_t numCtrlPnts = ctrlPnts.size();

    knots.push_back(ctrlPnts[0].m_x);
    for (size_t i = 0; i < numCtrlPnts - 1; ++i)
    {
        const float xi = ctrlPnts[i].m_x;
        const float xi_pl1 = ctrlPnts[i + 1].m_x;
        const float yi = ctrlPnts[i].m_y;
        const float yi_pl1 = ctrlPnts[i + 1].m_y;
        const float del_x = xi_pl1 - xi;
        const float del_y = yi_pl1 - yi;
        const float secantSlope = del_y / del_x;
        if (std::fabs((slopes[i] + slopes[i + 1]) - 2.f * secantSlope) < 1e-6f)
        {
            coefsC.push_back(yi);
            coefsB.push_back(slopes[i]);
            coefsA.push_back(0.5f * (slopes[i + 1] - slopes[i]) / del_x);
        }
        else
        {
            float ksi = 0.f;
            const float aa = slopes[i] - secantSlope;
            const float bb = slopes[i + 1] - secantSlope;
            if (aa * bb >= 0.f)
            {
                ksi = (xi + xi_pl1) * 0.5f;
            }
            else
            {
                if (std::fabs(aa) > std::fabs(bb))
                {
                    ksi = xi_pl1 + aa * del_x / (slopes[i + 1] - slopes[i]);
                }
                else
                {
                    ksi = xi + bb * del_x / (slopes[i + 1] - slopes[i]);
                }
            }
            const float s_bar = (2.f * secantSlope - slopes[i + 1]) +
                (slopes[i + 1] - slopes[i]) * (ksi - xi) / del_x;
            const float eta = (s_bar - slopes[i]) / (ksi - xi);
            coefsC.push_back(yi);
            coefsB.push_back(slopes[i]);
            coefsA.push_back(0.5f * eta);
            coefsC.push_back(yi + slopes[i] * (ksi - xi) + 0.5f * eta * (ksi - xi) * (ksi - xi));
            coefsB.push_back(s_bar);
            coefsA.push_back(0.5f * (slopes[i + 1] - s_bar) / (xi_pl1 - ksi));
            knots.push_back(ksi);
        }
        knots.push_back(xi_pl1);
    }
}

//------------------------------------------------------------------------------------------------
//
bool AdjustRGBSlopes(const std::vector<GradingControlPoint> & ctrlPnts,
                     std::vector<float> & slopes,
                     std::vector<float> & knots)
{
    bool adjustment_done = false;
    size_t i = 0, j = 0;
    const size_t n = knots.size();
    while (j < n)
    {
        if (ctrlPnts[i].m_x != knots[j])
        {
            const float ksi = knots[j];
            const float xi = ctrlPnts[i].m_x;
            const float xi_pl1 = ctrlPnts[i + 1].m_x;
            const float yi = ctrlPnts[i].m_y;
            const float yi_pl1 = ctrlPnts[i + 1].m_y;
            const float s_bar = (2.f * (yi_pl1 - yi) - (ksi - xi) * slopes[i] -
                                 (xi_pl1 - ksi) * slopes[i + 1]) / (xi_pl1 - xi);
            if (s_bar < 0.f)
            {
                adjustment_done = true;
                float secant = (yi_pl1 - yi) / (xi_pl1 - xi);
                const float blend_slope = ((ksi - xi) * slopes[i] + (xi_pl1 - ksi) * slopes[i + 1]) /
                                          (xi_pl1 - xi);
                float aim_slope = 0.01f * 0.5f * (slopes[i] + slopes[i + 1]);
                if (aim_slope > secant)
                {
                    aim_slope = secant;
                }
                const float adjust = (2.f * secant - aim_slope) / blend_slope;
                slopes[i] = slopes[i] * adjust;
                slopes[i + 1] = slopes[i + 1] * adjust;
            }
            i++;
        }
        j++;
    }
    return adjustment_done;
}

}   // namespace

//------------------------------------------------------------------------------------------------
//
GradingBSplineCurveRcPtr GradingBSplineCurve::Create(size_t size)
{
    auto newSpline = std::make_shared<GradingBSplineCurveImpl>(size);
    GradingBSplineCurveRcPtr res = newSpline;
    return res;
}

GradingBSplineCurveRcPtr GradingBSplineCurve::Create(size_t size, BSplineType splineType)
{
    auto newSpline = std::make_shared<GradingBSplineCurveImpl>(size, splineType);
    GradingBSplineCurveRcPtr res = newSpline;
    return res;
}

GradingBSplineCurveRcPtr GradingBSplineCurve::Create(size_t size, HueCurveType curveType)
{
    const BSplineType splineType = GradingHueCurve::GetBSplineTypeForHueCurveType(curveType);
    auto newSpline = std::make_shared<GradingBSplineCurveImpl>(size, splineType);
    GradingBSplineCurveRcPtr res = newSpline;
    return res;
}

GradingBSplineCurveRcPtr GradingBSplineCurve::Create(std::initializer_list<GradingControlPoint> values)
{
    auto newSpline = std::make_shared<GradingBSplineCurveImpl>(values.size());
    size_t i = 0;
    for (const auto & c : values)
    {
        newSpline->getControlPoint(i++) = c;
    }
    GradingBSplineCurveRcPtr res;
    res = newSpline;
    return res;
}

GradingBSplineCurveRcPtr GradingBSplineCurve::Create(std::initializer_list<GradingControlPoint> values, BSplineType splineType)
{
    auto newSpline = std::make_shared<GradingBSplineCurveImpl>(values.size(), splineType);
    size_t i = 0;
    for (const auto & c : values)
    {
        newSpline->getControlPoint(i++) = c;
    }
    GradingBSplineCurveRcPtr res;
    res = newSpline;
    return res;
}

GradingBSplineCurveRcPtr GradingBSplineCurve::Create(std::initializer_list<GradingControlPoint> values, HueCurveType curveType)
{
    const BSplineType splineType = GradingHueCurve::GetBSplineTypeForHueCurveType(curveType);
    auto newSpline = std::make_shared<GradingBSplineCurveImpl>(values.size(), splineType);
    size_t i = 0;
    for (const auto & c : values)
    {
        newSpline->getControlPoint(i++) = c;
    }
    GradingBSplineCurveRcPtr res;
    res = newSpline;
    return res;
}

GradingBSplineCurveImpl::GradingBSplineCurveImpl(size_t size)
    : m_controlPoints(size), m_slopesArray(size, 0.f), m_splineType(B_SPLINE)
{
}

GradingBSplineCurveImpl::GradingBSplineCurveImpl(size_t size, BSplineType splineType)
    : m_controlPoints(size), m_slopesArray(size, 0.f), m_splineType(splineType)
{
}

GradingBSplineCurveImpl::GradingBSplineCurveImpl(const std::vector<GradingControlPoint> & controlPoints)
    : m_controlPoints(controlPoints), m_slopesArray(controlPoints.size(), 0.f), m_splineType(B_SPLINE)
{
}

GradingBSplineCurveImpl::GradingBSplineCurveImpl(const std::vector<GradingControlPoint> & controlPoints, BSplineType splineType)
    : m_controlPoints(controlPoints), m_slopesArray(controlPoints.size(), 0.f), m_splineType(splineType)
{
}

GradingBSplineCurveRcPtr GradingBSplineCurveImpl::createEditableCopy() const
{
    auto copy = std::make_shared<GradingBSplineCurveImpl>(0);
    copy->m_controlPoints = m_controlPoints;
    copy->m_slopesArray = m_slopesArray;
    copy->m_splineType = m_splineType;
    GradingBSplineCurveRcPtr res;
    res = copy;
    return res;
}

BSplineType GradingBSplineCurveImpl::getSplineType() const
{
    return m_splineType;
}

void GradingBSplineCurveImpl::setSplineType(BSplineType splineType)
{
    m_splineType = splineType;
}

size_t GradingBSplineCurveImpl::getNumControlPoints() const noexcept
{
    return m_controlPoints.size();
}

void GradingBSplineCurveImpl::setNumControlPoints(size_t size)
{
    m_controlPoints.resize(size);
    m_slopesArray.resize(size, 0.f);
}

void GradingBSplineCurveImpl::validateIndex(size_t index) const
{
    const size_t numPoints = m_controlPoints.size();
    if (index >= numPoints)
    {
        std::ostringstream oss;
        oss << "There are '"<< numPoints << "' control points. '" << index << "' is out of bounds.";
        throw Exception(oss.str().c_str());
    }
}

const GradingControlPoint & GradingBSplineCurveImpl::getControlPoint(size_t index) const
{
    validateIndex(index);
    return m_controlPoints[index];
}

GradingControlPoint & GradingBSplineCurveImpl::getControlPoint(size_t index)
{
    validateIndex(index);
    return m_controlPoints[index];
}

float GradingBSplineCurveImpl::getSlope(size_t index) const
{
    validateIndex(index);
    return m_slopesArray[index];
}

void GradingBSplineCurveImpl::setSlope(size_t index, float slope)
{
    validateIndex(index);
    m_slopesArray[index] = slope;
}

bool GradingBSplineCurveImpl::slopesAreDefault() const
{
    for (size_t i = 0; i < m_slopesArray.size(); ++i)
    {
        if (m_slopesArray[i] != 0.f)
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------------------------
//
void GradingBSplineCurveImpl::validate() const
{
    const size_t numPoints = m_controlPoints.size();
    if (numPoints < 2)
    {
        throw Exception("There must be at least 2 control points.");
    }
    if (numPoints != m_slopesArray.size())
    {
        throw Exception("The slopes array must be the same length as the control points.");
    }

    // Make sure the x-coordinates are non-decreasing.
    float lastX = -std::numeric_limits<float>::max();
    for (size_t i = 0; i < numPoints; ++i)
    {
        const float x = m_controlPoints[i].m_x;
        if (x < lastX)
        {
            std::ostringstream oss;
            oss << "Control point at index " << i << " has a x coordinate '" << x << "' that is ";
            oss << "less than previous control point x coordinate '" << lastX << "'.";
            throw Exception(oss.str().c_str());
        }
        lastX = x;
    }

    // The x-coordinates for a hue-hue spline must be in [0,1].
    if (m_splineType == HUE_HUE_B_SPLINE)
    {
        if (m_controlPoints[0].m_x < 0.f)
        {
            std::ostringstream oss;
            oss << "The HUE-HUE spline may not have negative x coordinates.";
            throw Exception(oss.str().c_str());
        }
        else if (m_controlPoints[numPoints - 1].m_x > 1.f)
        {
            std::ostringstream oss;
            oss << "The HUE-HUE spline may not have x coordinates greater than one.";
            throw Exception(oss.str().c_str());
        }
    }

    // Make sure the y-coordinates are non-decreasing, for diagonal spline types.
    if ( m_splineType == B_SPLINE          || 
         m_splineType == DIAGONAL_B_SPLINE ||
         m_splineType == HUE_HUE_B_SPLINE )
    {
        float lastY = -std::numeric_limits<float>::max();
        if (m_splineType == HUE_HUE_B_SPLINE)
        {
            // The curve is diagonal but continues in a periodic way, so wrap the last point
            // around and ensure the first point would preserve monotonicity.
            lastY = m_controlPoints[numPoints - 1].m_y - 1.f;
        }

        for (size_t i = 0; i < numPoints; ++i)
        {
            const float y = m_controlPoints[i].m_y;
            if (y < lastY)
            {
                std::ostringstream oss;
                oss << "Control point at index " << i << " has a y coordinate '" << y << "' that is ";
                oss << "less than previous control point y coordinate '" << lastY << "'.";
                throw Exception(oss.str().c_str());
            }
            lastY = y;
        }
    }

    // Don't allow only x values of 0 and 1 for periodic curves (since they are essentially only one point).
    if (numPoints == 2)
    {    
        // Periodic curve types only.
        if ( m_splineType == PERIODIC_1_B_SPLINE || 
             m_splineType == PERIODIC_0_B_SPLINE || 
             m_splineType == HUE_HUE_B_SPLINE )
        {
            const float del_x = m_controlPoints[1].m_x - m_controlPoints[0].m_x;
            if (std::abs(1.f - del_x) < 1e-3)
            {
                throw Exception("The periodic spline x coordinates may not wrap to the same value.");
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
//
bool GradingBSplineCurveImpl::isIdentity() const
{
    bool isIdentity = true;
    if( m_splineType == DIAGONAL_B_SPLINE || m_splineType == B_SPLINE || m_splineType == HUE_HUE_B_SPLINE )
    {    
       isIdentity = std::all_of(m_controlPoints.cbegin(), 
                                m_controlPoints.cend(), 
                                [](const GradingControlPoint& cp) { return cp.m_x == cp.m_y; });
    } 
    else if( m_splineType == PERIODIC_0_B_SPLINE )
    {
       isIdentity = std::all_of(m_controlPoints.cbegin(), 
                                m_controlPoints.cend(), 
                                [](const GradingControlPoint& cp) { return cp.m_y == 0.f; });
    } 
    else if( m_splineType == HORIZONTAL1_B_SPLINE || m_splineType == PERIODIC_1_B_SPLINE )
    {
       isIdentity = std::all_of(m_controlPoints.cbegin(), 
                                m_controlPoints.cend(), 
                                [](const GradingControlPoint& cp) { return cp.m_y == 1.f; });
    } else
    {
        throw Exception("Unknown curve type: Could not determine if curve is identity.");
    }

    if (!isIdentity || !slopesAreDefault())
    {
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------------------------
//
bool IsGradingCurveIdentity(const ConstGradingBSplineCurveRcPtr & curve)
{
    auto curveImpl = dynamic_cast<const GradingBSplineCurveImpl *>(curve.get());
    if (curveImpl)
    {
        return curveImpl->isIdentity();
    }
    return false;
}

//------------------------------------------------------------------------------------------------
//
void GradingBSplineCurveImpl::computeKnotsAndCoefsForRGBCurve(KnotsCoefs & knotsCoefs, int curveIdx) const
{
    // Skip invalid data and identity.
    if (m_controlPoints.size() < 2 || isIdentity())
    {
        // Identity curve: offset is -1 and count is 0.
        knotsCoefs.m_knotsOffsetsArray[curveIdx * 2] = -1;
        knotsCoefs.m_knotsOffsetsArray[curveIdx * 2 + 1] = 0;
        knotsCoefs.m_coefsOffsetsArray[curveIdx * 2] = -1;
        knotsCoefs.m_coefsOffsetsArray[curveIdx * 2 + 1] = 0;
        return;
    }
    else
    {
        std::vector<float> knots;
        std::vector<float> coefsA;
        std::vector<float> coefsB;
        std::vector<float> coefsC;
        std::vector<float> slopes;

        if ( !slopesAreDefault() && (m_slopesArray.size() == m_controlPoints.size()) )
        {
            // If the user-supplied slopes are non-zero, use those.
            slopes = m_slopesArray;
        }
        else
        {
            // Otherwise, estimate slopes based on the control points.
            EstimateRGBSlopes(m_controlPoints, slopes);
        }

        FitRGBSpline(m_controlPoints, slopes, knots, coefsA, coefsB, coefsC);

        bool adjustment_done = AdjustRGBSlopes(m_controlPoints, slopes, knots);

        if (adjustment_done)
        {
            knots.clear();
            coefsA.clear();  coefsB.clear();  coefsC.clear();
            FitRGBSpline(m_controlPoints, slopes, knots, coefsA, coefsB, coefsC);
        }

        const int numKnots = static_cast<int>(knotsCoefs.m_numKnots);
        const int newKnots = static_cast<int>(knots.size());
        const int numCoefs = static_cast<int>(knotsCoefs.m_numCoefs);
        const int newCoefs = static_cast<int>(coefsA.size() * 3);

        if (numKnots + newKnots > KnotsCoefs::MAX_NUM_KNOTS ||
            numCoefs + newCoefs > KnotsCoefs::MAX_NUM_COEFS)
        {
            throw Exception("RGB curve: maximum number of control points reached.");
        }

        knotsCoefs.m_knotsOffsetsArray[curveIdx * 2] = numKnots;
        knotsCoefs.m_knotsOffsetsArray[curveIdx * 2 + 1] = newKnots;
        knotsCoefs.m_coefsOffsetsArray[curveIdx * 2] = numCoefs;
        knotsCoefs.m_coefsOffsetsArray[curveIdx * 2 + 1] = newCoefs;

        const unsigned coefsSize = (unsigned) coefsA.size();
        std::copy(knots.begin(), knots.end(), knotsCoefs.m_knotsArray.begin() + numKnots);
        std::copy(coefsA.begin(), coefsA.end(), knotsCoefs.m_coefsArray.begin() + numCoefs);
        std::copy(coefsB.begin(), coefsB.end(), knotsCoefs.m_coefsArray.begin() + numCoefs + coefsSize);
        std::copy(coefsC.begin(), coefsC.end(), knotsCoefs.m_coefsArray.begin() + numCoefs + coefsSize * 2);
        
        knotsCoefs.m_numKnots += newKnots;
        knotsCoefs.m_numCoefs += newCoefs;
    }
}

//------------------------------------------------------------------------------------------------
//
void GradingBSplineCurveImpl::computeKnotsAndCoefsForHueCurve(KnotsCoefs & knotsCoefs,
                                                              int curveIdx,
                                                              bool drawCurveOnly) const
{
    // Return 0 knots and coefficients when the curve is identity.
    if (m_controlPoints.size() < 2 || isIdentity()) 
    {
        if (!drawCurveOnly)
        {
            // In this case, do not add any knots or coefs. This will allow localBypass
            // to be true if all the curves are identities.

            // Identity curve: offset is -1 and count is 0.
            knotsCoefs.m_knotsOffsetsArray[curveIdx * 2] = -1;
            knotsCoefs.m_knotsOffsetsArray[curveIdx * 2 + 1] = 0;
            knotsCoefs.m_coefsOffsetsArray[curveIdx * 2] = -1;
            knotsCoefs.m_coefsOffsetsArray[curveIdx * 2 + 1] = 0;
            return;
        }
        else
        {
            // DrawCurveOnly is set when drawing the splines for a UI. In this mode, the
            // spline is always set on the HueSat curve and the HueCurve eval only computes
            // that one curve. But the curve/spline type are not known, so the polynomial
            // must be set so that it returns the correct values, even if it is an identity.
            // Note that the value returned for an identity varies among the spline types.

            // Set knots and coefficients that represent an identity curve.

            const int numKnots = static_cast<int>(knotsCoefs.m_numKnots);
            const int numCoefs = static_cast<int>(knotsCoefs.m_numCoefs);

            const int N_IDENTITY_KNOTS = 2;
            const int N_IDENTITY_COEFS = 3;

            knotsCoefs.m_knotsOffsetsArray[curveIdx * 2] = numKnots;
            knotsCoefs.m_knotsOffsetsArray[curveIdx * 2 + 1] = N_IDENTITY_KNOTS;
            knotsCoefs.m_coefsOffsetsArray[curveIdx * 2] = knotsCoefs.m_numCoefs;
            knotsCoefs.m_coefsOffsetsArray[curveIdx * 2 + 1] = N_IDENTITY_COEFS;

            knotsCoefs.m_knotsArray.begin()[numKnots] = 0.f;
            knotsCoefs.m_knotsArray.begin()[numKnots + 1] = 1.f;

            // Identity curve are linear or constant, so set the quadratic coefficient to zero.
            knotsCoefs.m_coefsArray.begin()[numCoefs] = 0.f;

            // Set the linear coefficient to match the slope of the identity curve.
            const float linearCoef = m_splineType == BSplineType::DIAGONAL_B_SPLINE || 
                                     m_splineType == BSplineType::HUE_HUE_B_SPLINE ? 
                                     1.0f : 0.f;

            knotsCoefs.m_coefsArray.begin()[numCoefs + 1] = linearCoef;

            // Set the constant coefficient for an identity curve.
            const float constantCoef = m_splineType == BSplineType::PERIODIC_1_B_SPLINE || 
                                       m_splineType == BSplineType::HORIZONTAL1_B_SPLINE ? 
                                       1.0f : 0.f;

            knotsCoefs.m_coefsArray.begin()[numCoefs + 2] = constantCoef;

            knotsCoefs.m_numKnots += N_IDENTITY_KNOTS;
            knotsCoefs.m_numCoefs += N_IDENTITY_COEFS;

            return;
        }
    }

    bool isPeriodic = false;
    bool isHorizontal = true;
    if (m_splineType == BSplineType::PERIODIC_1_B_SPLINE  ||
        m_splineType == BSplineType::PERIODIC_0_B_SPLINE  ||
        m_splineType == BSplineType::HUE_HUE_B_SPLINE)
    {
        isPeriodic = true;
    }
    if (m_splineType == BSplineType::DIAGONAL_B_SPLINE  ||
        m_splineType == BSplineType::HUE_HUE_B_SPLINE)
    {
        isHorizontal = false;
    }

    std::vector<GradingControlPoint> resultCtrlPnts;
    PrepHueCurveData(m_controlPoints, resultCtrlPnts, isPeriodic, isHorizontal);

    // For the purposes of slope estimation, consider the hue-hue spline to be horizontal.
    if (m_splineType == BSplineType::HUE_HUE_B_SPLINE)
    {
        isHorizontal = true;
    }

    std::vector<float> slopes;
    if ( !slopesAreDefault() && (m_slopesArray.size() == m_controlPoints.size()) )
    {
        // If the user-supplied slopes are non-zero, use those.
        slopes = m_slopesArray;
        
        // We need to ensure equal number of slopes and control points for the spline fit.
        if(isPeriodic)
        {
            float firstSlope = slopes.back();
            slopes.insert(slopes.begin(), firstSlope);

            float lastSlope = slopes.front();
            slopes.push_back(lastSlope);
        }
    }
    else
    {
        EstimateHueSlopes(resultCtrlPnts, slopes, isPeriodic, isHorizontal);
    }

    std::vector<float> knots;
    std::vector<float> coefsA;
    std::vector<float> coefsB;
    std::vector<float> coefsC;
    FitHueSpline(resultCtrlPnts, slopes, knots, coefsA, coefsB, coefsC);

    const int numKnots = static_cast<int>(knotsCoefs.m_numKnots);
    const int newKnots = static_cast<int>(knots.size());
    const int numCoefs = static_cast<int>(knotsCoefs.m_numCoefs);
    const int newCoefs = static_cast<int>(coefsA.size() * 3);

    if (numKnots + newKnots > KnotsCoefs::MAX_NUM_KNOTS ||
        numCoefs + newCoefs > KnotsCoefs::MAX_NUM_COEFS)
    {
        throw Exception("Hue curve: maximum number of control points reached.");
    }

    knotsCoefs.m_knotsOffsetsArray[curveIdx * 2] = numKnots;
    knotsCoefs.m_knotsOffsetsArray[curveIdx * 2 + 1] = newKnots;
    knotsCoefs.m_coefsOffsetsArray[curveIdx * 2] = numCoefs;
    knotsCoefs.m_coefsOffsetsArray[curveIdx * 2 + 1] = newCoefs;

    const unsigned coefsSize = (unsigned) coefsA.size();
    std::copy(knots.begin(), knots.end(), knotsCoefs.m_knotsArray.begin() + numKnots);
    std::copy(coefsA.begin(), coefsA.end(), knotsCoefs.m_coefsArray.begin() + numCoefs);
    std::copy(coefsB.begin(), coefsB.end(), knotsCoefs.m_coefsArray.begin() + numCoefs + coefsSize);
    std::copy(coefsC.begin(), coefsC.end(), knotsCoefs.m_coefsArray.begin() + numCoefs + coefsSize * 2);
    
    knotsCoefs.m_numKnots += newKnots;
    knotsCoefs.m_numCoefs += newCoefs;
}

//------------------------------------------------------------------------------------------------
//
void GradingBSplineCurveImpl::computeKnotsAndCoefs(KnotsCoefs & knotsCoefs, int curveIdx, bool drawCurveOnly) const
{
   if(m_splineType == BSplineType::B_SPLINE)
   {
       computeKnotsAndCoefsForRGBCurve(knotsCoefs, curveIdx);
   }
   else
   {
       computeKnotsAndCoefsForHueCurve(knotsCoefs, curveIdx, drawCurveOnly);
   }
}

//------------------------------------------------------------------------------------------------
//
void GradingBSplineCurveImpl::AddShaderEvalFwd(GpuShaderText & st, 
                                               const std::string & knotsOffsets,
                                               const std::string & coefsOffsets,
                                               const std::string & knots, 
                                               const std::string & coefs)
{
    // See GradingHue/RGBCurveOpGPU.cpp:AddCurveEvalMethodTextToShaderProgram.
    // The input arguments are:
    //      curveIdx -- The index of the curve being evaluated.
    //             x -- The input value.
    //    identity_x -- The desired output if there is no curve to evaluate.

    st.newLine() << "int knotsOffs = " << knotsOffsets << "[curveIdx * 2];";
    st.newLine() << "int knotsCnt = " << knotsOffsets << "[curveIdx * 2 + 1];";
    st.newLine() << "int coefsOffs = " << coefsOffsets << "[curveIdx * 2];";
    st.newLine() << "int coefsCnt = " << coefsOffsets << "[curveIdx * 2 + 1];";
    st.newLine() << "int coefsSets = coefsCnt / 3;";
    // If the curve has the default/identity values, the coef data is empty, return the identity.
    st.newLine() << "if (coefsSets == 0)";
    st.newLine() << "{";
    st.newLine() << "  return identity_x;";
    st.newLine() << "}";

    st.newLine() << "float knStart = " << knots << "[knotsOffs];";
    st.newLine() << "float knEnd = " << knots << "[knotsOffs + knotsCnt - 1];";

    st.newLine() << "if (x <= knStart)";
    st.newLine() << "{";
    st.newLine() << "  float B = " << coefs << "[coefsOffs + coefsSets];";
    st.newLine() << "  float C = " << coefs << "[coefsOffs + coefsSets * 2];";
    st.newLine() << "  return (x - knStart) * B + C;";
    st.newLine() << "}";

    st.newLine() << "else if (x >= knEnd)";
    st.newLine() << "{";
    st.newLine() << "  float A = " << coefs << "[coefsOffs + coefsSets - 1];";
    st.newLine() << "  float B = " << coefs << "[coefsOffs + coefsSets * 2 - 1];";
    st.newLine() << "  float C = " << coefs << "[coefsOffs + coefsSets * 3 - 1];";
    st.newLine() << "  float kn = " << knots << "[knotsOffs + knotsCnt - 2];";
    st.newLine() << "  float t = knEnd - kn;";
    st.newLine() << "  float slope = 2. * A * t + B;";
    st.newLine() << "  float offs = ( A * t + B ) * t + C;";
    st.newLine() << "  return (x - knEnd) * slope + offs;";
    st.newLine() << "}";

    // else
    st.newLine() << "int i = 0;";
    st.newLine() << "for (i = 0; i < knotsCnt - 2; ++i)";
    st.newLine() << "{";
    st.newLine() << "  if (x < " << knots << "[knotsOffs + i + 1])";
    st.newLine() << "  {";
    st.newLine() << "    break;";
    st.newLine() << "  }";
    st.newLine() << "}";

    st.newLine() << "float A = " << coefs << "[coefsOffs + i];";
    st.newLine() << "float B = " << coefs << "[coefsOffs + coefsSets + i];";
    st.newLine() << "float C = " << coefs << "[coefsOffs + coefsSets * 2 + i];";
    st.newLine() << "float kn = " << knots << "[knotsOffs + i];";
    st.newLine() << "float t = x - kn;";
    st.newLine() << "return ( A * t + B ) * t + C;";
}

//------------------------------------------------------------------------------------------------
//
void GradingBSplineCurveImpl::AddShaderEvalRev(GpuShaderText & st, 
                                               const std::string & knotsOffsets,
                                               const std::string & coefsOffsets,
                                               const std::string & knots, 
                                               const std::string & coefs)
{
    // See GradingHue/RGBCurveOpGPU.cpp:AddCurveEvalMethodTextToShaderProgram.
    // The input arguments are:
    //      curveIdx -- The index of the curve being evaluated.
    //             x -- The input value.

    st.newLine() << "int knotsOffs = " << knotsOffsets << "[curveIdx * 2];";
    st.newLine() << "int knotsCnt = " << knotsOffsets << "[curveIdx * 2 + 1];";
    st.newLine() << "int coefsOffs = " << coefsOffsets << "[curveIdx * 2];";
    st.newLine() << "int coefsCnt = " << coefsOffsets << "[curveIdx * 2 + 1];";
    st.newLine() << "int coefsSets = coefsCnt / 3;";

    st.newLine() << "if (coefsSets == 0)";
    st.newLine() << "{";
    st.newLine() << "  return x;";
    st.newLine() << "}";

    st.newLine() << "float knStart = " << knots << "[knotsOffs];";
    st.newLine() << "float knEnd = " << knots << "[knotsOffs + knotsCnt - 1];";
    st.newLine() << "float knStartY = " << coefs << "[coefsOffs + coefsSets * 2];";
    st.newLine() << "float knEndY;";
    st.newLine() << "{";
    st.newLine() << "  float A = " << coefs << "[coefsOffs + coefsSets - 1];";
    st.newLine() << "  float B = " << coefs << "[coefsOffs + coefsSets * 2 - 1];";
    st.newLine() << "  float C = " << coefs << "[coefsOffs + coefsSets * 3 - 1];";
    st.newLine() << "  float kn = " << knots << "[knotsOffs + knotsCnt - 2];";
    st.newLine() << "  float t = knEnd - kn;";
    st.newLine() << "  knEndY = ( A * t + B ) * t + C;";
    st.newLine() << "}";

    st.newLine() << "if (x <= knStartY)";
    st.newLine() << "{";
    st.newLine() << "  float B = " << coefs << "[coefsOffs + coefsSets];";
    st.newLine() << "  float C = " << coefs << "[coefsOffs + coefsSets * 2];";
    st.newLine() << "  return abs(B) < 1e-5 ? knStart : (x - C) / B + knStart;";
    st.newLine() << "}";

    st.newLine() << "else if (x >= knEndY)";
    st.newLine() << "{";
    st.newLine() << "  float A = " << coefs << "[coefsOffs + coefsSets - 1];";
    st.newLine() << "  float B = " << coefs << "[coefsOffs + coefsSets * 2 - 1];";
    st.newLine() << "  float C = " << coefs << "[coefsOffs + coefsSets * 3 - 1];";
    st.newLine() << "  float kn = " << knots << "[knotsOffs + knotsCnt - 2];";
    st.newLine() << "  float t = knEnd - kn;";
    st.newLine() << "  float slope = 2. * A * t + B;";
    st.newLine() << "  float offs = ( A * t + B ) * t + C;";
    st.newLine() << "  return abs(slope) < 1e-5 ? knEnd : (x - offs) / slope + knEnd;";
    st.newLine() << "}";

    // else
    st.newLine() << "int i = 0;";
    st.newLine() << "for (i = 0; i < knotsCnt - 2; ++i)";
    st.newLine() << "{";
    st.newLine() << "  if (x < " << coefs << "[coefsOffs + coefsSets * 2 + i + 1])";
    st.newLine() << "  {";
    st.newLine() << "    break;";
    st.newLine() << "  }";
    st.newLine() << "}";

    st.newLine() << "float A = " << coefs << "[coefsOffs + i];";
    st.newLine() << "float B = " << coefs << "[coefsOffs + coefsSets + i];";
    st.newLine() << "float C = " << coefs << "[coefsOffs + coefsSets * 2 + i];";
    st.newLine() << "float kn = " << knots << "[knotsOffs + i];";
    st.newLine() << "float C0 = C - x;";
    st.newLine() << "float discrim = sqrt(B * B - 4. * A * C0);";
    st.newLine() << "return kn + (-2. * C0) / (discrim + B);";
}

//------------------------------------------------------------------------------------------------
//
void GradingBSplineCurveImpl::AddShaderEvalRevHue(GpuShaderText & st, 
                                                  const std::string & knotsOffsets,
                                                  const std::string & coefsOffsets,
                                                  const std::string & knots, 
                                                  const std::string & coefs)
{
    // See GradingHueCurveOpGPU.cpp:AddCurveEvalMethodTextToShaderProgram.
    // The input arguments are:
    //      curveIdx -- The index of the curve being evaluated.
    //             x -- The input value.

    st.newLine() << "int knotsOffs = " << knotsOffsets << "[curveIdx * 2];";
    st.newLine() << "int knotsCnt = " << knotsOffsets << "[curveIdx * 2 + 1];";
    st.newLine() << "int coefsOffs = " << coefsOffsets << "[curveIdx * 2];";
    st.newLine() << "int coefsCnt = " << coefsOffsets << "[curveIdx * 2 + 1];";
    st.newLine() << "int coefsSets = coefsCnt / 3;";

    st.newLine() << "if (coefsSets == 0)";
    st.newLine() << "{";
    st.newLine() << "  return x;";
    st.newLine() << "}";

    st.newLine() << "float knStart = " << knots << "[knotsOffs];";
    st.newLine() << "float knEnd = " << knots << "[knotsOffs + knotsCnt - 1];";
    st.newLine() << "float knStartY = " << coefs << "[coefsOffs + coefsSets * 2];";
    st.newLine() << "float knEndY;";
    st.newLine() << "{";
    st.newLine() << "  float A = " << coefs << "[coefsOffs + coefsSets - 1];";
    st.newLine() << "  float B = " << coefs << "[coefsOffs + coefsSets * 2 - 1];";
    st.newLine() << "  float C = " << coefs << "[coefsOffs + coefsSets * 3 - 1];";
    st.newLine() << "  float kn = " << knots << "[knotsOffs + knotsCnt - 2];";
    st.newLine() << "  float t = knEnd - kn;";
    st.newLine() << "  knEndY = ( A * t + B ) * t + C;";
    // The HUE-FX curve is index 7 and requires special handling.
    st.newLine() << "  knEndY = (curveIdx == 7) ? knEndY + knEnd : knEndY;";
    st.newLine() << "}";

    st.newLine() << "if (x < knStartY)";
    st.newLine() << "{";
    st.newLine() << "  x = x + ceil(knStartY - x);";
    st.newLine() << "}";

    st.newLine() << "else if (x > knEndY)";
    st.newLine() << "{";
    st.newLine() << "  x = x - ceil(x - knEndY);";
    st.newLine() << "}";

    st.newLine() << "int i = 0;";
    st.newLine() << "for (i = 0; i < knotsCnt - 2; ++i)";
    st.newLine() << "{";
    st.newLine() << "  float curve_x = " << coefs << "[coefsOffs + coefsSets * 2 + i + 1];";
    st.newLine() << "  curve_x = (curveIdx == 7) ? curve_x + " << knots << "[knotsOffs + i + 1] : curve_x;";
    st.newLine() << "  if (x < curve_x)";
    st.newLine() << "  {";
    st.newLine() << "    break;";
    st.newLine() << "  }";
    st.newLine() << "}";

    st.newLine() << "float A = " << coefs << "[coefsOffs + i];";
    st.newLine() << "float B = " << coefs << "[coefsOffs + coefsSets + i];";
    st.newLine() << "float C = " << coefs << "[coefsOffs + coefsSets * 2 + i];";
    st.newLine() << "float kn = " << knots << "[knotsOffs + i];";
    st.newLine() << "if (curveIdx == 7)";
    st.newLine() << "{";
    st.newLine() << "  C = C + kn;";
    st.newLine() << "  B = B + 1.;";
    st.newLine() << "}";
    st.newLine() << "float C0 = C - x;";
    st.newLine() << "float discrim = sqrt(B * B - 4. * A * C0);";
    st.newLine() << "return kn + (-2. * C0) / (discrim + B);";
}

//------------------------------------------------------------------------------------------------
//
GradingBSplineCurveImpl::KnotsCoefs::KnotsCoefs(size_t numCurves)
{
    m_knotsOffsetsArray.resize(2 * numCurves);
    m_coefsOffsetsArray.resize(2 * numCurves);

    m_coefsArray.resize(DynamicPropertyGradingRGBCurveImpl::GetMaxCoefs());
    m_knotsArray.resize(DynamicPropertyGradingRGBCurveImpl::GetMaxKnots());
}

//------------------------------------------------------------------------------------------------
//
float GradingBSplineCurveImpl::KnotsCoefs::evalCurve(int c, float x, float identity_x) const
{
    // NB: When evaluating hue curves, x should be wrapped to [0,1) by the caller
    // so there is no extrapolation.

    const int coefsSets = m_coefsOffsetsArray[2 * c + 1] / 3;
    if (coefsSets == 0)
    {
        return identity_x;
    }
    const int coefsOffs = m_coefsOffsetsArray[2 * c];
    const int knotsCnt = m_knotsOffsetsArray[2 * c + 1];
    const int knotsOffs = m_knotsOffsetsArray[2 * c];

    const float knStart = m_knotsArray[knotsOffs];
    const float knEnd = m_knotsArray[knotsOffs + knotsCnt - 1];

    if (x <= knStart)
    {
        const float B = m_coefsArray[coefsOffs + coefsSets];
        const float C = m_coefsArray[coefsOffs + coefsSets * 2];
        return (x - knStart) * B + C;
    }
    else if (x >= knEnd)
    {
        const float A  = m_coefsArray[coefsOffs + coefsSets - 1];
        const float B  = m_coefsArray[coefsOffs + coefsSets * 2 - 1];
        const float C  = m_coefsArray[coefsOffs + coefsSets * 3 - 1];
        const float kn = m_knotsArray[knotsOffs + knotsCnt - 2];
        const float t = knEnd - kn;
        const float slope = 2.f * A * t + B;
        const float offs = (A * t + B) * t + C;
        return (x - knEnd) * slope + offs;
    }
    else
    {
        int i;
        for (i = 0; i < knotsCnt - 2; ++i)
        {
            if (x < m_knotsArray[knotsOffs + i + 1])
                break;
        }
        const float A  = m_coefsArray[coefsOffs + i];
        const float B  = m_coefsArray[coefsOffs + coefsSets + i];
        const float C  = m_coefsArray[coefsOffs + coefsSets * 2 + i];
        const float kn = m_knotsArray[knotsOffs + i];
        const float t = x - kn;
        return (A * t + B) * t + C;
    }
}

//------------------------------------------------------------------------------------------------
//
float GradingBSplineCurveImpl::KnotsCoefs::evalCurveRev(int c, float y) const
{
    // Note: This is only intended to invert the monotonic curve types.
    // The horizontal curve types only need to be evaluated in the forward
    // direction, even when inverting the hue curve transform. The exception
    // is the HueFX curve but that has its own inversion function below.

    const int coefsSets = m_coefsOffsetsArray[2 * c + 1] / 3;
    if (coefsSets == 0)
    {
        return y;
    }
    const int coefsOffs = m_coefsOffsetsArray[2 * c];
    const int knotsCnt = m_knotsOffsetsArray[2 * c + 1];
    const int knotsOffs = m_knotsOffsetsArray[2 * c];

    const float knStart = m_knotsArray[knotsOffs];
    const float knEnd = m_knotsArray[knotsOffs + knotsCnt - 1];
    const float knStartY = m_coefsArray[coefsOffs + coefsSets * 2];
    float knEndY;
    {
        const float A  = m_coefsArray[coefsOffs + coefsSets - 1];
        const float B  = m_coefsArray[coefsOffs + coefsSets * 2 - 1];
        const float C  = m_coefsArray[coefsOffs + coefsSets * 3 - 1];
        const float kn = m_knotsArray[knotsOffs + knotsCnt - 2];
        const float t = knEnd - kn;
        knEndY = (A * t + B) * t + C;
    }

    if (y <= knStartY)
    {
        // Extrapolate low side.
        const float B = m_coefsArray[coefsOffs + coefsSets];
        const float C = m_coefsArray[coefsOffs + coefsSets * 2];
        return std::fabs(B) < 1e-5f ? knStart : (y - C) / B + knStart;
    }
    else if (y >= knEndY)
    {
        // Extrapolate high side.
        const float A  = m_coefsArray[coefsOffs + coefsSets - 1];
        const float B  = m_coefsArray[coefsOffs + coefsSets * 2 - 1];
        const float C  = m_coefsArray[coefsOffs + coefsSets * 3 - 1];
        const float kn = m_knotsArray[knotsOffs + knotsCnt - 2];
        const float t = knEnd - kn;
        const float slope = 2.f * A * t + B;
        const float offs = (A * t + B) * t + C;
        return std::fabs(slope) < 1e-5f ? knEnd : (y - offs) / slope + knEnd;
    }
    else
    {
        int i;
        for (i = 0; i < knotsCnt - 2; ++i)
        {
            if (y < m_coefsArray[coefsOffs + coefsSets * 2 + i + 1])
                break;
        }
        const float A  = m_coefsArray[coefsOffs + i];
        const float B  = m_coefsArray[coefsOffs + coefsSets + i];
        const float C  = m_coefsArray[coefsOffs + coefsSets * 2 + i];
        const float kn = m_knotsArray[knotsOffs + i];
        const float C0 = C - y;
        const float discrim = sqrt(B * B - 4.f * A * C0);
        return kn + (-2.f * C0) / (discrim + B);
    }
}

//------------------------------------------------------------------------------------------------
//
float GradingBSplineCurveImpl::KnotsCoefs::evalCurveRevHue(int c, float y) const
{
    // This function is specifically to invert the HueFX and Hue-Hue curve types.
    //
    // The output of the HueFX curve is a "delta hue" signal that is
    // added on to the incoming hue:  HueOut = HueIn + HueFX(HueIn).
    // The input to this function should be the HueOut, in other words,
    // the sum of the HueIn and delta hue. It returns HueIn.

    const int coefsSets = m_coefsOffsetsArray[2 * c + 1] / 3;
    if (coefsSets == 0)
    {
        return y;
    }

    const int coefsOffs = m_coefsOffsetsArray[2 * c];
    const int knotsCnt = m_knotsOffsetsArray[2 * c + 1];
    const int knotsOffs = m_knotsOffsetsArray[2 * c];

    const float knStart = m_knotsArray[knotsOffs];
    const float knEnd = m_knotsArray[knotsOffs + knotsCnt - 1];
    float knStartY = m_coefsArray[coefsOffs + coefsSets * 2];
    const bool isHfx = c == 7;
    knStartY = isHfx ? knStartY + knStart : knStartY;
    float knEndY;
    {
        const float A  = m_coefsArray[coefsOffs + coefsSets - 1];
        const float B  = m_coefsArray[coefsOffs + coefsSets * 2 - 1];
        const float C  = m_coefsArray[coefsOffs + coefsSets * 3 - 1];
        const float kn = m_knotsArray[knotsOffs + knotsCnt - 2];
        const float t = knEnd - kn;
        knEndY = (A * t + B) * t + C;
        knEndY = isHfx ? knEndY + knEnd : knEndY;
    }

    if (y < knStartY)
    {
        // Wrap up into the valid hue range.
        y += std::ceil(knStartY - y);
    }
    else if (y > knEndY)
    {
        // Wrap down into the valid hue range.
        y -= std::ceil(y - knEndY);
    }

    int i;
    for (i = 0; i < knotsCnt - 2; ++i)
    {
        float curve_y = m_coefsArray[coefsOffs + coefsSets * 2 + i + 1];
        curve_y = isHfx ? curve_y + m_knotsArray[knotsOffs + i + 1] : curve_y;

        if (y < curve_y)
            break;
    }
    const float A  = m_coefsArray[coefsOffs + i];
    float B  = m_coefsArray[coefsOffs + coefsSets + i];
    float C  = m_coefsArray[coefsOffs + coefsSets * 2 + i];
    const float kn = m_knotsArray[knotsOffs + i];
    if (isHfx)
    {
        C += kn;     // shift curve up so left edge is on the main diagonal
        B += 1.f;    // add diagonal line
    }
    const float C0 = C - y;
    const float discrim = sqrt(B * B - 4.f * A * C0);
    return kn + (-2.f * C0) / (discrim + B);
    // Note: The caller should wrap to ensure the output is a hue on [0,1).
}

bool operator==(const GradingControlPoint & lhs, const GradingControlPoint & rhs)
{
    return lhs.m_x == rhs.m_x && lhs.m_y == rhs.m_y;
}

bool operator!=(const GradingControlPoint & lhs, const GradingControlPoint & rhs)
{
    return !(lhs == rhs);
}

bool operator==(const GradingBSplineCurve & lhs, const GradingBSplineCurve & rhs)
{
    if (lhs.getSplineType() != rhs.getSplineType())
    {
        return false;
    }

    const auto num = lhs.getNumControlPoints();
    if (num == rhs.getNumControlPoints())
    {
        for (size_t i = 0; i < num; ++i)
        {
            if ( lhs.getControlPoint(i) != rhs.getControlPoint(i)  ||
                 lhs.getSlope(i) != rhs.getSlope(i) )
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool operator!=(const GradingBSplineCurve & lhs, const GradingBSplineCurve & rhs)
{
    return !(lhs == rhs);
}

} // namespace OCIO_NAMESPACE

