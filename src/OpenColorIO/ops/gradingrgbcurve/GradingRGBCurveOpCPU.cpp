// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>
#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/gradingrgbcurve/GradingRGBCurveOpCPU.h"
#include "SSE.h"

namespace OCIO_NAMESPACE
{

namespace
{
class GradingRGBCurveOpCPU : public OpCPU
{
public:
    GradingRGBCurveOpCPU() = delete;
    GradingRGBCurveOpCPU(const GradingRGBCurveOpCPU &) = delete;

    explicit GradingRGBCurveOpCPU(ConstGradingRGBCurveOpDataRcPtr & grgbc);

    bool hasDynamicProperty(DynamicPropertyType type) const override;
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const override;

protected:
    void eval(const GradingBSplineCurveImpl::KnotsCoefs & knotsCoefs,
              float * out, const float * in) const
    {
        out[0] = knotsCoefs.evalCurve(static_cast<int>(RGB_RED), in[0]);
        out[1] = knotsCoefs.evalCurve(static_cast<int>(RGB_GREEN), in[1]);
        out[2] = knotsCoefs.evalCurve(static_cast<int>(RGB_BLUE), in[2]);
        // TODO: Add vectorized version for master curve.
        out[0] = knotsCoefs.evalCurve(static_cast<int>(RGB_MASTER), out[0]);
        out[1] = knotsCoefs.evalCurve(static_cast<int>(RGB_MASTER), out[1]);
        out[2] = knotsCoefs.evalCurve(static_cast<int>(RGB_MASTER), out[2]);
    }
    void evalRev(const GradingBSplineCurveImpl::KnotsCoefs & knotsCoefs,
                 float * out, const float * in) const
    {
        out[0] = knotsCoefs.evalCurveRev(static_cast<int>(RGB_MASTER), in[0]);
        out[1] = knotsCoefs.evalCurveRev(static_cast<int>(RGB_MASTER), in[1]);
        out[2] = knotsCoefs.evalCurveRev(static_cast<int>(RGB_MASTER), in[2]);
        out[0] = knotsCoefs.evalCurveRev(static_cast<int>(RGB_RED), out[0]);
        out[1] = knotsCoefs.evalCurveRev(static_cast<int>(RGB_GREEN), out[1]);
        out[2] = knotsCoefs.evalCurveRev(static_cast<int>(RGB_BLUE), out[2]);
    }

    DynamicPropertyGradingRGBCurveImplRcPtr m_grgbcurve;
};

GradingRGBCurveOpCPU::GradingRGBCurveOpCPU(ConstGradingRGBCurveOpDataRcPtr & grgbc)
    : OpCPU()
{
    m_grgbcurve = grgbc->getDynamicPropertyInternal();
    if (m_grgbcurve->isDynamic())
    {
        m_grgbcurve = m_grgbcurve->createEditableCopy();
    }
}

bool GradingRGBCurveOpCPU::hasDynamicProperty(DynamicPropertyType type) const
{
    bool res = false;
    if (type == DYNAMIC_PROPERTY_GRADING_RGBCURVE)
    {
        res = m_grgbcurve->isDynamic();
    }
    return res;
}

DynamicPropertyRcPtr GradingRGBCurveOpCPU::getDynamicProperty(DynamicPropertyType type) const
{
    if (type == DYNAMIC_PROPERTY_GRADING_RGBCURVE)
    {
        if (m_grgbcurve->isDynamic())
        {
            return m_grgbcurve;
        }
    }
    else
    {
        throw Exception("Dynamic property type not supported by GradingRGBCurve.");
    }

    throw Exception("GradingRGBCurve property is not dynamic.");
}

class GradingRGBCurveFwdOpCPU : public GradingRGBCurveOpCPU
{
public:
    GradingRGBCurveFwdOpCPU(const GradingRGBCurveOpCPU &) = delete;
    GradingRGBCurveFwdOpCPU() = delete;

    explicit GradingRGBCurveFwdOpCPU(ConstGradingRGBCurveOpDataRcPtr & grgbc);
    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

GradingRGBCurveFwdOpCPU::GradingRGBCurveFwdOpCPU(ConstGradingRGBCurveOpDataRcPtr & grgbc)
    : GradingRGBCurveOpCPU(grgbc)
{

}

static constexpr auto PixelSize = 4 * sizeof(float);

void GradingRGBCurveFwdOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_grgbcurve->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        eval(m_grgbcurve->getKnotsCoefs(), out, in);

        out[3] = in[3];

        in += 4;
        out += 4;
    }
}

class GradingRGBCurveLinearFwdOpCPU : public GradingRGBCurveOpCPU
{
public:
    GradingRGBCurveLinearFwdOpCPU() = delete;
    GradingRGBCurveLinearFwdOpCPU(const GradingRGBCurveOpCPU &) = delete;

    explicit GradingRGBCurveLinearFwdOpCPU(ConstGradingRGBCurveOpDataRcPtr & grgbc);
    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

GradingRGBCurveLinearFwdOpCPU::GradingRGBCurveLinearFwdOpCPU(ConstGradingRGBCurveOpDataRcPtr & grgbc)
    : GradingRGBCurveOpCPU(grgbc)
{

}

namespace LogLinConstants
{
    static constexpr float xbrk = 0.0041318374739483946f;
    static constexpr float shift = -0.000157849851665374f;
    static constexpr float m = 1.f / (0.18f + shift);
    static constexpr float gain = 363.034608563f;
    static constexpr float offs = -7.f;
    static constexpr float ybrk = -5.5f;
#ifdef USE_SSE
    const __m128 mxbrk = _mm_set1_ps(xbrk);
    const __m128 mshift = _mm_set1_ps(shift);
    const __m128 mm = _mm_set1_ps(m);
    const __m128 mgain = _mm_set1_ps(gain);
    const __m128 moffs = _mm_set1_ps(offs);
    const __m128 mybrk = _mm_set1_ps(ybrk);
    const __m128 mgainInv = _mm_set1_ps(1.f / gain);
    const __m128 mshift018 = _mm_set1_ps(shift + 0.18f);
    const __m128 mpower = _mm_set1_ps(2.0f);
#else
    static constexpr float base2 = 1.4426950408889634f; // 1/log(2)
#endif
}

inline void LinLog(const float * in, float * out)
{
#ifdef USE_SSE
        __m128 pix = _mm_loadu_ps(in);
        __m128 flag = _mm_cmpgt_ps(pix, LogLinConstants::mxbrk);

        __m128 pixLin = _mm_mul_ps(pix, LogLinConstants::mgain);
        pixLin = _mm_add_ps(pixLin, LogLinConstants::moffs);

        pix = _mm_add_ps(pix, LogLinConstants::mshift);
        pix = _mm_mul_ps(pix, LogLinConstants::mm);
        pix = sseLog2(pix);

        pix = _mm_or_ps(_mm_and_ps(flag, pix),
            _mm_andnot_ps(flag, pixLin));

        _mm_storeu_ps(out, pix);
#else
        out[0] = (in[0] < LogLinConstants::xbrk) ?
                 in[0] * LogLinConstants::gain + LogLinConstants::offs :
                 LogLinConstants::base2 * std::log((in[0] + LogLinConstants::shift) * LogLinConstants::m);
        out[1] = (in[1] < LogLinConstants::xbrk) ?
                 in[1] * LogLinConstants::gain + LogLinConstants::offs :
                 LogLinConstants::base2 * std::log((in[1] + LogLinConstants::shift) * LogLinConstants::m);
        out[2] = (in[2] < LogLinConstants::xbrk) ?
                 in[2] * LogLinConstants::gain + LogLinConstants::offs :
                 LogLinConstants::base2 * std::log((in[2] + LogLinConstants::shift) * LogLinConstants::m);
        out[3] = in[3];
#endif
}

inline void LogLin(float * out)
{
#ifdef USE_SSE
        __m128 pix = _mm_loadu_ps(out);
        __m128 flag = _mm_cmpgt_ps(pix, LogLinConstants::mybrk);

        __m128 pixLin = _mm_sub_ps(pix, LogLinConstants::moffs);
        pixLin = _mm_mul_ps(pixLin, LogLinConstants::mgainInv);

        pix = ssePower(LogLinConstants::mpower, pix);
        pix = _mm_mul_ps(pix, LogLinConstants::mshift018);
        pix = _mm_sub_ps(pix, LogLinConstants::mshift);

        pix = _mm_or_ps(_mm_and_ps(flag, pix),
              _mm_andnot_ps(flag, pixLin));

        _mm_storeu_ps(out, pix);
#else
        out[0] = (out[0] < LogLinConstants::ybrk) ?
                 (out[0] - LogLinConstants::offs) / LogLinConstants::gain :
                 std::pow(2.0f, out[0]) * (0.18f + LogLinConstants::shift) - LogLinConstants::shift;
        out[1] = (out[1] < LogLinConstants::ybrk) ?
                 (out[1] - LogLinConstants::offs) / LogLinConstants::gain :
                 std::pow(2.0f, out[1]) * (0.18f + LogLinConstants::shift) - LogLinConstants::shift;
        out[2] = (out[2] < LogLinConstants::ybrk) ?
                 (out[2] - LogLinConstants::offs) / LogLinConstants::gain :
                 std::pow(2.0f, out[2]) * (0.18f + LogLinConstants::shift) - LogLinConstants::shift;
#endif
}

void GradingRGBCurveLinearFwdOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_grgbcurve->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        LinLog(in, out);

        // Curves.
        eval(m_grgbcurve->getKnotsCoefs(), out, out);

        LogLin(out);

        out[3] = in[3];

        in += 4;
        out += 4;
    }
}

class GradingRGBCurveRevOpCPU : public GradingRGBCurveOpCPU
{
public:
    GradingRGBCurveRevOpCPU(const GradingRGBCurveOpCPU &) = delete;
    GradingRGBCurveRevOpCPU() = delete;

    explicit GradingRGBCurveRevOpCPU(ConstGradingRGBCurveOpDataRcPtr & grgbc);
    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

GradingRGBCurveRevOpCPU::GradingRGBCurveRevOpCPU(ConstGradingRGBCurveOpDataRcPtr & grgbc)
    : GradingRGBCurveOpCPU(grgbc)
{
}

void GradingRGBCurveRevOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_grgbcurve->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        evalRev(m_grgbcurve->getKnotsCoefs(), out, in);

        out[3] = in[3];

        in += 4;
        out += 4;
    }
}

class GradingRGBCurveLinearRevOpCPU : public GradingRGBCurveRevOpCPU
{
public:
    GradingRGBCurveLinearRevOpCPU(const GradingRGBCurveOpCPU &) = delete;
    GradingRGBCurveLinearRevOpCPU() = delete;

    explicit GradingRGBCurveLinearRevOpCPU(ConstGradingRGBCurveOpDataRcPtr & grgbc);
    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

GradingRGBCurveLinearRevOpCPU::GradingRGBCurveLinearRevOpCPU(ConstGradingRGBCurveOpDataRcPtr & grgbc)
    : GradingRGBCurveRevOpCPU(grgbc)
{
}

void GradingRGBCurveLinearRevOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_grgbcurve->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        LinLog(in, out);

        // Curves.
        evalRev(m_grgbcurve->getKnotsCoefs(), out, out);

        LogLin(out);

        out[3] = in[3];

        in += 4;
        out += 4;
    }
}

} // Anonymous namespace

///////////////////////////////////////////////////////////////////////////////

ConstOpCPURcPtr GetGradingRGBCurveCPURenderer(ConstGradingRGBCurveOpDataRcPtr & prim)
{
    const bool linToLog = (prim->getStyle() == GRADING_LIN) && !prim->getBypassLinToLog();

    switch (prim->getDirection())
    {
    case TRANSFORM_DIR_FORWARD:
    {
        if (linToLog)
        {
            return std::make_shared<GradingRGBCurveLinearFwdOpCPU>(prim);
        }
        else
        {
            return std::make_shared<GradingRGBCurveFwdOpCPU>(prim);
        }
        break;
    }
    case TRANSFORM_DIR_INVERSE:
    {
        if (linToLog)
        {
            return std::make_shared<GradingRGBCurveLinearRevOpCPU>(prim);
        }
        else
        {
            return std::make_shared<GradingRGBCurveRevOpCPU>(prim);
        }
        break;
    }
    }

    throw Exception("Illegal GradingRGBCurve direction.");
}

} // namespace OCIO_NAMESPACE
