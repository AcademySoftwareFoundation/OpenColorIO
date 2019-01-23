/*
Copyright (c) 2018 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <algorithm>
#include <math.h>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "ops/Gamma/GammaOpUtils.h"


OCIO_NAMESPACE_ENTER
{

namespace
{

// Here we calculate the extra parameters used for the moncurve model.
// The break point and slope of the linear segment are implied by the
// gamma and offset.  The idea is that the linear segment has to meet
// the power function at the point where the value and slope of the
// two segments match.

// The moncurve model would get a div by 0 error with gain=1, offset=0,
// so the values need to get fudged slightly. We do that here rather
// than during construction or validation so that the object may
// contain the neat looking values since these are what would get written
// to a ctf file.

const double EPS = 1e-6;

double monCurveGammaFwd(const GammaOpData::Params & p)
{
    return std::max(p[0], 1. + EPS);
}

double monCurveOffsetFwd(const GammaOpData::Params & p)
{
    const double offset = std::max(p[1], EPS);
    return offset / (1. + offset);
}

double monCurveBreakFwd(const GammaOpData::Params & p)
{
    // Break point between the linear and power functions.
    const double gamma  = std::max(p[0], 1. + EPS);
    const double offset = std::max(p[1], EPS);
    return offset / (gamma - 1.);
}

double monCurveSlopeFwd(const GammaOpData::Params & p)
{
    // Slope of the linear segment.
    const double gamma  = std::max(p[0], 1. + EPS);
    const double offset = std::max(p[1], EPS);
    const double a = (gamma - 1.) / offset;
    const double b = offset * gamma / (( gamma - 1.) * ( 1. + offset));
    return a * pow(b, gamma);
}

double monCurveScaleFwd(const GammaOpData::Params & p)
{
    // This just rearranges the equation a little so we can get by
    // with a single multiply rather than two.
    const double offset = std::max(p[1], EPS);
    return 1. / (1. + offset);
}

// Same stuff for the inverse of the forward function.

double monCurveGammaRev(const GammaOpData::Params & p)
{
    return 1. / std::max(p[0], 1. + EPS);
}

double monCurveOffsetRev(const GammaOpData::Params & p)
{
    return std::max(p[1], EPS);
}

double monCurveBreakRev(const GammaOpData::Params & p)
{
    const double gamma  = std::max(p[0], 1. + EPS);
    const double offset = std::max(p[1], EPS);
    const double a = offset * gamma;
    const double b = (gamma - 1.) * (1. + offset);
    return pow( a / b, gamma );
}

double monCurveSlopeRev(const GammaOpData::Params & p)
{
    const double gamma  = std::max(p[0], 1. + EPS);
    const double offset = std::max(p[1], EPS);
    const double a = (gamma - 1.) / offset;
    const double b = (1. + offset) / gamma;
    return pow(a, gamma - 1.) * pow(b, gamma);
}

double monCurveScaleRev(const GammaOpData::Params & p)
{
    const double offset = std::max(p[1], EPS);
    return 1. + offset;
}



};

void ComputeParamsFwd(const GammaOpData::Params & gParams,
                      BitDepth inBitDepth,
                      BitDepth outBitDepth,
                      RendererParams & rParams)
{
    rParams.gamma    = float(monCurveGammaFwd(gParams));
    rParams.offset   = float(monCurveOffsetFwd(gParams));
    rParams.breakPnt = float(monCurveBreakFwd(gParams) 
                        * GetBitDepthMaxValue(inBitDepth));
    rParams.slope    = float(monCurveSlopeFwd(gParams) 
                        * GetBitDepthMaxValue(outBitDepth) 
                        / GetBitDepthMaxValue(inBitDepth));
    rParams.scale    = float(monCurveScaleFwd(gParams)
                        / GetBitDepthMaxValue(inBitDepth));
}

void ComputeParamsRev(const GammaOpData::Params & gParams,
                      BitDepth inBitDepth,
                      BitDepth outBitDepth,
                      RendererParams & rParams)
{
    rParams.gamma    = float(monCurveGammaRev(gParams));
    rParams.offset   = float(monCurveOffsetRev(gParams)
                        * GetBitDepthMaxValue(outBitDepth));
    rParams.breakPnt = float(monCurveBreakRev(gParams)
                        * GetBitDepthMaxValue(inBitDepth));
    rParams.slope    = float(monCurveSlopeRev(gParams)
                        * GetBitDepthMaxValue(outBitDepth)
                        / GetBitDepthMaxValue(inBitDepth));
    rParams.scale    = float(monCurveScaleRev(gParams)
                        * GetBitDepthMaxValue(outBitDepth));
}

}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "MathUtils.h"
#include "unittest.h"

OIIO_ADD_TEST(GammaOpUtils, compute_params_forward)
{
    const OCIO::GammaOpData::Params gParams = { 2.0f, 0.1f };
    OCIO::RendererParams rParams;

    // F32 to F32

    {
        ComputeParamsFwd(gParams, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, rParams);

        OIIO_CHECK_EQUAL(rParams.gamma,    2.0f);
        OIIO_CHECK_EQUAL(rParams.offset,   float(0.1 / (1. + 0.1)));
        OIIO_CHECK_EQUAL(rParams.breakPnt, float(0.1 / (2. - 1. )));
        OIIO_CHECK_EQUAL(rParams.scale,    float(1.  / (1. + 0.1)));

        OIIO_CHECK_ASSERT(OCIO::EqualWithAbsError(rParams.slope, 0.33057851f, 1e-7f));
    }

    // UINT10 to F16

    {
        ComputeParamsFwd(gParams, OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_F16, rParams);

        OIIO_CHECK_EQUAL(rParams.gamma,    2.0f);
        OIIO_CHECK_EQUAL(rParams.offset,   float( 0.1 / (1. + 0.1)));
        OIIO_CHECK_EQUAL(rParams.breakPnt, float((0.1 / (2. - 1. )) * 1023.));
        OIIO_CHECK_EQUAL(rParams.scale,    float((1.  / (1. + 0.1)) / 1023.));

        OIIO_CHECK_ASSERT(OCIO::EqualWithAbsError(rParams.slope, 0.00032314f, 1e-7f));
    }
}


OIIO_ADD_TEST(GammaOpUtils, compute_params_reverse)
{
    const OCIO::GammaOpData::Params gParams = { 2.0f, 0.1f };
    OCIO::RendererParams rParams;

    // F32 to F32

    {
        ComputeParamsRev(gParams, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, rParams);

        OIIO_CHECK_EQUAL(rParams.gamma,    0.5f );
        OIIO_CHECK_EQUAL(rParams.offset,   0.1f);
        OIIO_CHECK_EQUAL(rParams.scale,    1.0f + 0.1f);

        OIIO_CHECK_ASSERT(OCIO::EqualWithAbsError(rParams.breakPnt, 0.03305785f, 1e-7f));
        OIIO_CHECK_ASSERT(OCIO::EqualWithAbsError(rParams.slope,    3.02499986f, 1e-7f));
    }

    // UINT10 to F16

    {
        ComputeParamsRev(gParams, OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_F16, rParams);

        OIIO_CHECK_EQUAL(rParams.gamma,    0.5f);
        OIIO_CHECK_EQUAL(rParams.offset,   0.1f);
        OIIO_CHECK_EQUAL(rParams.scale,    1.0f + 0.1f);

        OIIO_CHECK_ASSERT(OCIO::EqualWithAbsError(rParams.breakPnt, 33.81818390f, 1e-7f));
        OIIO_CHECK_ASSERT(OCIO::EqualWithAbsError(rParams.slope,     0.00295698f, 1e-7f));
    }

}


#endif // OCIO_UNIT_TEST
