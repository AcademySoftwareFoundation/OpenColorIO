// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "DynamicProperty.h"
#include "ops/exposurecontrast/ExposureContrastOpCPU.h"
#include "SSE.h"

OCIO_NAMESPACE_ENTER
{

namespace
{

class ECRendererBase : public OpCPU
{
public:
    ECRendererBase() = delete;
    ECRendererBase(const ECRendererBase &) = delete;   
    explicit ECRendererBase(ConstExposureContrastOpDataRcPtr & ec);
    virtual ~ECRendererBase();

    bool hasDynamicProperty(DynamicPropertyType type) const override;
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const override;

protected:
    virtual void updateData(ConstExposureContrastOpDataRcPtr & ec) = 0;

    DynamicPropertyImplRcPtr m_exposure;
    DynamicPropertyImplRcPtr m_contrast;
    DynamicPropertyImplRcPtr m_gamma;

    float m_pivot = 0.0f;
    float m_logExposureStep = 0.088f;
};

ECRendererBase::ECRendererBase(ConstExposureContrastOpDataRcPtr & ec)
    : OpCPU()
{
    // Copy DynamicPropertyImpl sharedPtr so that if content changes in ec
    // change will be reflected here.
    m_exposure = ec->getExposureProperty();
    m_contrast = ec->getContrastProperty();
    m_gamma = ec->getGammaProperty();
}

ECRendererBase::~ECRendererBase()
{
}

bool ECRendererBase::hasDynamicProperty(DynamicPropertyType type) const
{
    bool res = false;
    switch (type)
    {
        case DYNAMIC_PROPERTY_EXPOSURE:
            res = m_exposure->isDynamic();
            break;
        case DYNAMIC_PROPERTY_CONTRAST:
            res = m_contrast->isDynamic();
            break;
        case DYNAMIC_PROPERTY_GAMMA:
            res = m_gamma->isDynamic();
            break;
        default:
            break;
    }
    
    return res;
}

DynamicPropertyRcPtr ECRendererBase::getDynamicProperty(DynamicPropertyType type) const
{
    switch (type)
    {
        case DYNAMIC_PROPERTY_EXPOSURE:
            if (m_exposure->isDynamic())
            {
                return m_exposure;
            }
            break;
        case DYNAMIC_PROPERTY_CONTRAST:
            if (m_contrast->isDynamic())
            {
                return m_contrast;
            }
            break;
        case DYNAMIC_PROPERTY_GAMMA:
            if (m_gamma->isDynamic())
            {
                return m_gamma;
            }
            break;
        default:
            throw Exception("Dynamic property type not supported by ExposureContrast.");
            break;
    }

    throw Exception("ExposureContrast property is not dynamic.");
}


class ECLinearRenderer : public ECRendererBase
{
public:
    explicit ECLinearRenderer(ConstExposureContrastOpDataRcPtr & ec);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void updateData(ConstExposureContrastOpDataRcPtr & ec) override;
};

ECLinearRenderer::ECLinearRenderer(ConstExposureContrastOpDataRcPtr & ec)
    : ECRendererBase(ec)
{
    updateData(ec);
}

void ECLinearRenderer::updateData(ConstExposureContrastOpDataRcPtr & ec)
{
    m_pivot = (float)std::max(EC::MIN_PIVOT, ec->getPivot());
}

void ECLinearRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    // TODO: allow negative contrast?
    // TODO: is it worth adding a code path without dynamic paramaters?
    const float contrastVal = (float)std::max(EC::MIN_CONTRAST,
                                              m_contrast->getDoubleValue() *
                                              m_gamma->getDoubleValue());
    const float exposureVal = powf(2.f, (float)m_exposure->getDoubleValue());

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    if (contrastVal == 1.f)
    {
        for (long idx = 0; idx<numPixels; ++idx)
        {
            //
            // out = in * exposure;
            //
            out[0] = in[0] * exposureVal;
            out[1] = in[1] * exposureVal;
            out[2] = in[2] * exposureVal;
            out[3] = in[3];

            in += 4;
            out += 4;
        }
    }
    else
    {
#ifdef USE_SSE
        __m128 contrast = _mm_set1_ps(contrastVal);
        __m128 exposure_over_pivot = _mm_set1_ps(exposureVal / m_pivot);
        __m128 piv = _mm_set1_ps(m_pivot);
        for (long idx = 0; idx<numPixels; ++idx)
        {
            const float outAlpha = in[3];
            __m128 data = _mm_set_ps(in[3],
                                     in[2],
                                     in[1],
                                     in[0]);

            //
            // out = powf( i * exposure / pivot, contrast ) * pivot
            //
            _mm_storeu_ps(out,
                _mm_mul_ps(
                    ssePower(
                        _mm_mul_ps(
                            data,
                            exposure_over_pivot),
                        contrast),
                    piv));
            out[3] = outAlpha;

            in += 4;
            out += 4;
        }
#else
        const float exposureOverPivotVal = exposureVal / m_pivot;
        for (long idx = 0; idx<numPixels; ++idx)
        {

            //
            // out = powf( i * exposure / iPivot, contrast ) * oPivot
            //
            // Note: With std::max NAN becomes 0.
            out[0] = powf(std::max(0.0f, in[0] * exposureOverPivotVal),
                           contrastVal) * m_pivot;
            out[1] = powf(std::max(0.0f, in[1] * exposureOverPivotVal),
                           contrastVal) * m_pivot;
            out[2] = powf(std::max(0.0f, in[2] * exposureOverPivotVal),
                           contrastVal) * m_pivot;
            out[3] = in[3];

            in += 4;
            out += 4;
        }
#endif
    }
}

class ECLinearRevRenderer : public ECRendererBase
{
public:
    explicit ECLinearRevRenderer(ConstExposureContrastOpDataRcPtr & ec);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void updateData(ConstExposureContrastOpDataRcPtr & ec) override;
};

ECLinearRevRenderer::ECLinearRevRenderer(ConstExposureContrastOpDataRcPtr & ec)
    : ECRendererBase(ec)
{
    updateData(ec);
}

void ECLinearRevRenderer::updateData(ConstExposureContrastOpDataRcPtr & ec)
{
    m_pivot = (float)std::max(EC::MIN_PIVOT, ec->getPivot());
}

void ECLinearRevRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    // TODO: allow negative contrast?
    const float contrastVal = (float)std::max(EC::MIN_CONTRAST,
                                              (m_contrast->getDoubleValue() * m_gamma->getDoubleValue()));
    const float invContrastVal = 1.f / contrastVal;
    const float invExposureVal = 1.f / powf(2.f, (float)m_exposure->getDoubleValue());

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    if (contrastVal == 1.f)
    {
        for (long idx = 0; idx<numPixels; ++idx)
        {
            out[0] = in[0] * invExposureVal;
            out[1] = in[1] * invExposureVal;
            out[2] = in[2] * invExposureVal;
            out[3] = in[3];

            in += 4;
            out += 4;
        }
    }
    else
    {
#ifdef USE_SSE
        __m128 inv_contrast = _mm_set1_ps(invContrastVal);

        const float pivotOverExposureVal = m_pivot * invExposureVal;
        const float invPivotVal = 1.f / m_pivot;

        __m128 pivot_over_exposure = _mm_set1_ps(pivotOverExposureVal);
        __m128 inv_pivot = _mm_set1_ps(invPivotVal);

        for (long idx = 0; idx<numPixels; ++idx)
        {
            const float outAlpha = in[3];
            __m128 data = _mm_set_ps(in[3],
                                     in[2],
                                     in[1],
                                     in[0]);

            //
            // out = powf( i / pivot, 1 / contrast ) * pivot / exposure
            //
            _mm_storeu_ps(out,
                _mm_mul_ps(
                    ssePower(
                        _mm_mul_ps(
                            data,
                            inv_pivot),
                        inv_contrast),
                    pivot_over_exposure));

            out[3] = outAlpha;

            in += 4;
            out += 4;
        }
#else
        const float pivotOverExposureVal = m_pivot * invExposureVal;
        const float invPivotVal = 1.f / m_pivot;

        for (long idx = 0; idx<numPixels; ++idx)
        {
            //
            // out = powf( i / pivot, 1 / contrast ) * pivot / exposure
            //
            out[0] = powf(std::max(0.0f, in[0] * invPivotVal),
                           invContrastVal) * pivotOverExposureVal;
            out[1] = powf(std::max(0.0f, in[1] * invPivotVal),
                           invContrastVal) * pivotOverExposureVal;
            out[2] = powf(std::max(0.0f, in[2] * invPivotVal),
                           invContrastVal) * pivotOverExposureVal;
            out[3] = in[3];

            in += 4;
            out += 4;
        }
#endif
    }
}

class ECVideoRenderer : public ECRendererBase
{
public:
    explicit ECVideoRenderer(ConstExposureContrastOpDataRcPtr & ec);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void updateData(ConstExposureContrastOpDataRcPtr & ec) override;
};

ECVideoRenderer::ECVideoRenderer(ConstExposureContrastOpDataRcPtr & ec)
    : ECRendererBase(ec)
{
    updateData(ec);
}

void ECVideoRenderer::updateData(ConstExposureContrastOpDataRcPtr & ec)
{
    m_pivot = powf((float)std::max(EC::MIN_PIVOT, ec->getPivot()),
                   (float)EC::VIDEO_OETF_POWER);
}

void ECVideoRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    // TODO: allow negative contrast?
    const float contrastVal = (float)std::max(EC::MIN_CONTRAST,
                                              (m_contrast->getDoubleValue() * m_gamma->getDoubleValue()));
    const float exposureVal = powf(powf(2.f, (float)m_exposure->getDoubleValue()),
                                   (float)EC::VIDEO_OETF_POWER);

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    if (contrastVal == 1.f)
    {
        for (long idx = 0; idx<numPixels; ++idx)
        {
            //
            // out = in * exposure;
            //
            out[0] = in[0] * exposureVal;
            out[1] = in[1] * exposureVal;
            out[2] = in[2] * exposureVal;
            out[3] = in[3];

            in += 4;
            out += 4;
        }
    }
    else
    {
#ifdef USE_SSE
        __m128 contrast = _mm_set1_ps(contrastVal);
        __m128 exposure_over_pivot = _mm_set1_ps(exposureVal / m_pivot);
        __m128 piv = _mm_set1_ps(m_pivot);
        for (long idx = 0; idx<numPixels; ++idx)
        {
            const float outAlpha = in[3];
            __m128 data = _mm_set_ps(in[3],
                                     in[2],
                                     in[1],
                                     in[0]);

            //
            // out = powf( i * exposure / pivot, contrast ) * pivot
            //
            _mm_storeu_ps(out,
                _mm_mul_ps(
                    ssePower(
                        _mm_mul_ps(
                            data,
                            exposure_over_pivot),
                        contrast),
                    piv));
            out[3] = outAlpha;

            in += 4;
            out += 4;
        }
#else
        const float exposureOverPivotVal = exposureVal / m_pivot;
        for (long idx = 0; idx<numPixels; ++idx)
        {
            //
            // out = powf( i * exposure / pivot, contrast ) * pivot
            //
            out[0] = powf(std::max(0.0f, in[0] * exposureOverPivotVal),
                           contrastVal) * m_pivot;
            out[1] = powf(std::max(0.0f, in[1] * exposureOverPivotVal),
                           contrastVal) * m_pivot;
            out[2] = powf(std::max(0.0f, in[2] * exposureOverPivotVal),
                           contrastVal) * m_pivot;
            out[3] = in[3];

            in += 4;
            out += 4;
        }
#endif
    }
}

class ECVideoRevRenderer : public ECRendererBase
{
public:
    explicit ECVideoRevRenderer(ConstExposureContrastOpDataRcPtr & ec);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void updateData(ConstExposureContrastOpDataRcPtr & ec) override;
};

ECVideoRevRenderer::ECVideoRevRenderer(ConstExposureContrastOpDataRcPtr & ec)
    : ECRendererBase(ec)
{
    updateData(ec);
}

void ECVideoRevRenderer::updateData(ConstExposureContrastOpDataRcPtr & ec)
{
    m_pivot = powf((float)std::max(EC::MIN_PIVOT, ec->getPivot()),
                   (float)EC::VIDEO_OETF_POWER);
}

void ECVideoRevRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    // TODO: allow negative contrast?
    const float contrastVal = (float)std::max(EC::MIN_CONTRAST,
                                              (m_contrast->getDoubleValue() * m_gamma->getDoubleValue()));
    const float invContrastVal = 1.f / contrastVal;
    const float invExposureVal = 1.f / powf(powf(2.f, (float)m_exposure->getDoubleValue()),
                                            (float)EC::VIDEO_OETF_POWER);
    const float pivotOverExposureVal = m_pivot * invExposureVal;
    const float invPivotVal = 1.f / m_pivot;

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    if (contrastVal == 1.f)
    {
        for (long idx = 0; idx<numPixels; ++idx)
        {
            //
            // out = in / exposure
            //
            out[0] = in[0] * invExposureVal;
            out[1] = in[1] * invExposureVal;
            out[2] = in[2] * invExposureVal;
            out[3] = in[3];

            in += 4;
            out += 4;
        }
    }
    else
    {
#ifdef USE_SSE
        __m128 inv_contrast = _mm_set1_ps(invContrastVal);
        __m128 pivot_over_exposure = _mm_set1_ps(pivotOverExposureVal);
        __m128 inv_pivot = _mm_set1_ps(invPivotVal);

        for (long idx = 0; idx<numPixels; ++idx)
        {
            const float outAlpha = in[3];
            __m128 data = _mm_set_ps(in[3],
                                     in[2],
                                     in[1],
                                     in[0]);

            //
            // out = powf( i / pivot, 1 / contrast ) * pivot / exposure
            //
            _mm_storeu_ps(out,
                _mm_mul_ps(
                    ssePower(
                        _mm_mul_ps(
                            data,
                            inv_pivot),
                        inv_contrast),
                    pivot_over_exposure));
            out[3] = outAlpha;

            in += 4;
            out += 4;
        }
#else
        for (long idx = 0; idx<numPixels; ++idx)
        {
            //
            // out = powf( i / pivot, 1 / contrast ) * pivot / exposure
            //
            out[0] = powf(std::max(0.0f, in[0] * invPivotVal),
                           invContrastVal) * pivotOverExposureVal;
            out[1] = powf(std::max(0.0f, in[1] * invPivotVal),
                           invContrastVal) * pivotOverExposureVal;
            out[2] = powf(std::max(0.0f, in[2] * invPivotVal),
                           invContrastVal) * pivotOverExposureVal;
            out[3] = in[3];

            in += 4;
            out += 4;
        }
#endif
    }
}

class ECLogarithmicRenderer : public ECRendererBase
{
public:
    explicit ECLogarithmicRenderer(ConstExposureContrastOpDataRcPtr & ec);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void updateData(ConstExposureContrastOpDataRcPtr & ec) override;
};

ECLogarithmicRenderer::ECLogarithmicRenderer(ConstExposureContrastOpDataRcPtr & ec)
    : ECRendererBase(ec)
{
    updateData(ec);
}

void ECLogarithmicRenderer::updateData(ConstExposureContrastOpDataRcPtr & ec)
{
    const float pivot = (float)std::max(EC::MIN_PIVOT, ec->getPivot());
    m_pivot = (float)std::max(0., log2(pivot / 0.18) *
                                  ec->getLogExposureStep() +
                                  ec->getLogMidGray());

    m_logExposureStep = (float)ec->getLogExposureStep();
}

void ECLogarithmicRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float exposureVal = (float)m_exposure->getDoubleValue() *
                              m_logExposureStep;
    const float contrastVal
        = (float)std::max(EC::MIN_CONTRAST,
                          (m_contrast->getDoubleValue() * m_gamma->getDoubleValue()));
    const float offsetVal = (exposureVal - m_pivot) * contrastVal + m_pivot;

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE
    // Equation is:
    // out = ( (in + expos) - pivot ) * contrast + pivot
    // Rearrange as:
    // out = [in * contrast] + [(expos - pivot) * contrast + pivot]

    __m128 contrast = _mm_set1_ps(contrastVal);
    __m128 offset = _mm_set1_ps(offsetVal);

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float outAlpha = in[3];
        __m128 data = _mm_set_ps(in[3],
                                 in[2],
                                 in[1],
                                 in[0]);

        //
        // out = ( in * contrast ) + offset
        //
        _mm_storeu_ps(out,
            _mm_add_ps(offset,
                _mm_mul_ps(data, contrast)));
        out[3] = outAlpha;

        in += 4;
        out += 4;
    }
#else
    for (long idx = 0; idx<numPixels; ++idx)
    {
        //
        // out = ( in * contrast ) + offset
        //
        out[0] = in[0] * contrastVal + offsetVal;
        out[1] = in[1] * contrastVal + offsetVal;
        out[2] = in[2] * contrastVal + offsetVal;
        out[3] = in[3];

        in += 4;
        out += 4;
    }
#endif
}

class ECLogarithmicRevRenderer : public ECRendererBase
{
public:
    explicit ECLogarithmicRevRenderer(ConstExposureContrastOpDataRcPtr & ec);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void updateData(ConstExposureContrastOpDataRcPtr & ec) override;
};

ECLogarithmicRevRenderer::ECLogarithmicRevRenderer(ConstExposureContrastOpDataRcPtr & ec)
    : ECRendererBase(ec)
{
    updateData(ec);
}

void ECLogarithmicRevRenderer::updateData(ConstExposureContrastOpDataRcPtr & ec)
{
    const float pivot = (float)std::max(EC::MIN_PIVOT, ec->getPivot());
    m_pivot = (float)std::max(0., log2(pivot / 0.18) *
                                  ec->getLogExposureStep() +
                                  ec->getLogMidGray());
}

void ECLogarithmicRevRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float exposureVal = (float)m_exposure->getDoubleValue() *
                              m_logExposureStep;
    const float inv_contrastVal
        = (float)std::max(EC::MIN_CONTRAST,
                          1. / (m_contrast->getDoubleValue() * m_gamma->getDoubleValue()));
    const float negOffsetVal = m_pivot - m_pivot * inv_contrastVal -
                               exposureVal;

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        //
        // out = ( in * inv_contrast ) + neg_offset
        //
        out[0] = in[0] * inv_contrastVal + negOffsetVal;
        out[1] = in[1] * inv_contrastVal + negOffsetVal;
        out[2] = in[2] * inv_contrastVal + negOffsetVal;
        out[3] = in[3];

        in += 4;
        out += 4;
    }
}

}

OpCPURcPtr GetExposureContrastCPURenderer(ConstExposureContrastOpDataRcPtr & ec)
{
    switch (ec->getStyle())
    {
    case ExposureContrastOpData::STYLE_LINEAR:
        return std::make_shared<ECLinearRenderer>(ec);
    case ExposureContrastOpData::STYLE_LINEAR_REV:
        return std::make_shared<ECLinearRevRenderer>(ec);
    case ExposureContrastOpData::STYLE_VIDEO:
        return std::make_shared<ECVideoRenderer>(ec);
    case ExposureContrastOpData::STYLE_VIDEO_REV:
        return std::make_shared<ECVideoRevRenderer>(ec);
    case ExposureContrastOpData::STYLE_LOGARITHMIC:
        return std::make_shared<ECLogarithmicRenderer>(ec);
    case ExposureContrastOpData::STYLE_LOGARITHMIC_REV:
        return std::make_shared<ECLogarithmicRevRenderer>(ec);
    }

    throw Exception("Unknown exposure contrast style");
    return OpCPURcPtr();
}

}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

namespace
{
float videoECVal(float in, OCIO::ConstExposureContrastOpDataRcPtr & ec,
                 float inMax = 1., float outMax = 1.)
{
    const float exposure
        = powf(powf(2.f, (float)ec->getExposure()),
               (float)OCIO::EC::VIDEO_OETF_POWER);
    const float contrast
        = (float)std::max(OCIO::EC::MIN_CONTRAST, (ec->getContrast()*ec->getGamma()));
    const float pivot
        = powf((float)std::max(OCIO::EC::MIN_PIVOT, ec->getPivot()),
               (float)OCIO::EC::VIDEO_OETF_POWER);

    if (contrast == 1.f)
    {
        return in * exposure / (pivot*inMax) * pivot * outMax;
    }

    return powf((float)std::max(0.0f, in   * exposure / (pivot*inMax)), contrast) * pivot * outMax;
    //     powf(       std::max(0.0f, t[0] * exposure / m_pivot      ), contrast) * m_pivot,

}

float logECVal(float in, OCIO::ConstExposureContrastOpDataRcPtr & ec,
               float inMax = 1., float outMax = 1.)
{
    const float exposure = (float)(ec->getLogExposureStep() *
                                   ec->getExposure()) * inMax;
    const float contrast
        = (float)std::max(OCIO::EC::MIN_CONTRAST, (ec->getContrast()*ec->getGamma())) * outMax / inMax;

    const float pivot = (float)std::max(OCIO::EC::MIN_PIVOT, ec->getPivot());
    const float logPivot =
        (float)std::max(0., log2(pivot / 0.18) *
                            ec->getLogExposureStep() + ec->getLogMidGray());
    const float offset = (exposure - (logPivot*inMax)) * contrast + (logPivot*outMax);

    return (in * contrast) + offset;
    //     (t[0] * contrast ) + offset,

}

float linECVal(float in, OCIO::ConstExposureContrastOpDataRcPtr & ec,
               float inMax = 1., float outMax = 1.)
{
    const float exposure = powf(2., (float)ec->getExposure());
    const float contrast = (float)std::max(OCIO::EC::MIN_CONTRAST,
                                           (ec->getContrast()*ec->getGamma()));
    const float pivot = (float)std::max(OCIO::EC::MIN_PIVOT, ec->getPivot());
    
    if (contrast == 1.f)
    {
        return in * exposure / (pivot*inMax) * pivot * outMax;
    }
    
    return powf((float)std::max(0.0f, in * exposure / (pivot*inMax)),
                contrast) * pivot * outMax;
    //     powf( MAX(0.0f, t[0] * exposure / _iPivot), contrast) * _oPivot,
    
}

static constexpr float qnan = std::numeric_limits<float>::quiet_NaN();
static constexpr float inf = std::numeric_limits<float>::infinity();

}

OCIO_ADD_TEST(ExposureContrastRenderer, video)
{
    //
    // Video case, no scaling
    //
    const std::vector<float> rgbaImage { 0.0367126f, 0.5f, 1.f,     0.f,
                                         0.2f,       0.f,   .99f, 128.f,
                                         qnan,       qnan, qnan,    0.f,
                                         inf,        inf,  inf,     0.f};

    OCIO::ExposureContrastOpDataRcPtr ec =
        std::make_shared<OCIO::ExposureContrastOpData>(
            OCIO::ExposureContrastOpData::STYLE_VIDEO);

    ec->getExposureProperty()->makeDynamic();
    ec->getContrastProperty()->makeDynamic();
    ec->getGammaProperty()->makeDynamic();

    OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
    OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);
    OCIO_CHECK_ASSERT(OCIO::DynamicPtrCast<OCIO::ECVideoRenderer>(renderer));
    std::vector<float> rgba = rgbaImage;

    renderer->apply(rgba.data(), rgba.data(), 4);

    OCIO_CHECK_EQUAL(rgba[0], videoECVal(rgbaImage[0], const_ec));
    OCIO_CHECK_EQUAL(rgba[1], videoECVal(rgbaImage[1], const_ec));
    OCIO_CHECK_EQUAL(rgba[2], videoECVal(rgbaImage[2], const_ec));
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OCIO_CHECK_EQUAL(rgba[4], videoECVal(rgbaImage[4], const_ec));
    OCIO_CHECK_EQUAL(rgba[5], videoECVal(rgbaImage[5], const_ec));
    OCIO_CHECK_EQUAL(rgba[6], videoECVal(rgbaImage[6], const_ec));
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);

    OCIO_CHECK_ASSERT(std::isnan(rgba[8]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[9]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[10]));
    OCIO_CHECK_EQUAL(rgba[12], videoECVal(rgbaImage[12], const_ec));
    OCIO_CHECK_EQUAL(rgba[13], videoECVal(rgbaImage[13], const_ec));
    OCIO_CHECK_EQUAL(rgba[14], videoECVal(rgbaImage[14], const_ec));

    // Re-test with different E/C values.
    //
    ec->setExposure(0.2);
    ec->setContrast(1.0);
    // TODO: When base < 1, ssePower(Inf, base) != Inf.  It returns various
    // large numbers, e.g., 3.6e11 for base=0.3 ranging up to 4.0e36 for base=0.95.
    // ec->setContrast(0.5);
    ec->setGamma(1.2);

    rgba = rgbaImage;
    renderer->apply(rgba.data(), rgba.data(), 4);

    // As the ssePower is an approximation, strict equality is not possible.
    const float error = 1e-5f;
    OCIO_CHECK_CLOSE(rgba[0], videoECVal(rgbaImage[0], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[1], videoECVal(rgbaImage[1], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[2], videoECVal(rgbaImage[2], const_ec), error);
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OCIO_CHECK_CLOSE(rgba[4], videoECVal(rgbaImage[4], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[5], videoECVal(rgbaImage[5], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[6], videoECVal(rgbaImage[6], const_ec), error);
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);

    OCIO_CHECK_CLOSE(rgba[8], videoECVal(rgbaImage[8], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[9], videoECVal(rgbaImage[9], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[10], videoECVal(rgbaImage[10], const_ec), error);
    OCIO_CHECK_EQUAL(rgba[12], videoECVal(rgbaImage[12], const_ec));
    OCIO_CHECK_EQUAL(rgba[13], videoECVal(rgbaImage[13], const_ec));
    OCIO_CHECK_EQUAL(rgba[14], videoECVal(rgbaImage[14], const_ec));
}

OCIO_ADD_TEST(ExposureContrastRenderer, log)
{
    //
    // Log case, no scaling
    //
    const std::vector<float> rgbaImage { 0.0367126f, 0.5f, 1.f,    0.f,
                                         0.2f,       0.f,  .99f, 128.f,
                                         qnan,       qnan, qnan,   0.f,
                                          inf,       inf,  inf,    0.f};
    OCIO::ExposureContrastOpDataRcPtr ec =
        std::make_shared<OCIO::ExposureContrastOpData>(
            OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC);

    ec->getExposureProperty()->makeDynamic();
    ec->getContrastProperty()->makeDynamic();
    ec->getGammaProperty()->makeDynamic();
    ec->setExposure(1.2);
    ec->setPivot(0.18);

    OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
    OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);
    OCIO_CHECK_ASSERT(OCIO::DynamicPtrCast<OCIO::ECLogarithmicRenderer>(renderer));

    std::vector<float> rgba = rgbaImage;

    renderer->apply(rgba.data(), rgba.data(), 4);

    const float inMax = (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F32);
    const float outMax = (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F32);

    OCIO_CHECK_EQUAL(rgba[0], logECVal(rgbaImage[0], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[1], logECVal(rgbaImage[1], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[2], logECVal(rgbaImage[2], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3] * outMax / inMax);
    OCIO_CHECK_EQUAL(rgba[4], logECVal(rgbaImage[4], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[5], logECVal(rgbaImage[5], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[6], logECVal(rgbaImage[6], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7] * outMax / inMax);

    OCIO_CHECK_ASSERT(std::isnan(rgba[8]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[9]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[10]));
    OCIO_CHECK_EQUAL(rgba[12], logECVal(rgbaImage[12], const_ec));
    OCIO_CHECK_EQUAL(rgba[13], logECVal(rgbaImage[13], const_ec));
    OCIO_CHECK_EQUAL(rgba[14], logECVal(rgbaImage[14], const_ec));

    // Re-test with differenc E/C values.
    //
    ec->setExposure(0.2);
    ec->setContrast(0.5);
    ec->setGamma(1.6);
    
    rgba = rgbaImage;
    renderer->apply(rgba.data(), rgba.data(), 4);

    OCIO_CHECK_EQUAL(rgba[0], logECVal(rgbaImage[0], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[1], logECVal(rgbaImage[1], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[2], logECVal(rgbaImage[2], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3] * outMax / inMax);
    OCIO_CHECK_EQUAL(rgba[4], logECVal(rgbaImage[4], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[5], logECVal(rgbaImage[5], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[6], logECVal(rgbaImage[6], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7] * outMax / inMax);

    OCIO_CHECK_ASSERT(std::isnan(rgba[8]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[9]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[10]));
    OCIO_CHECK_EQUAL(rgba[12], logECVal(rgbaImage[12], const_ec));
    OCIO_CHECK_EQUAL(rgba[13], logECVal(rgbaImage[13], const_ec));
    OCIO_CHECK_EQUAL(rgba[14], logECVal(rgbaImage[14], const_ec));
}

OCIO_ADD_TEST(ExposureContrastRenderer, linear)
{
    //
    // Linear case, no scaling
    //
    const std::vector<float> rgbaImage { 0.0f, 0.5f, 1.f,     0.f,
                                         0.2f, 0.8f,  .99f, 128.f,
                                         qnan, qnan, qnan,    0.f,
                                         inf,  inf,  inf,     0.f};
    OCIO::ExposureContrastOpDataRcPtr ec =
        std::make_shared<OCIO::ExposureContrastOpData>(
            OCIO::ExposureContrastOpData::STYLE_LINEAR);

    ec->getExposureProperty()->makeDynamic();
    ec->getContrastProperty()->makeDynamic();
    ec->getGammaProperty()->makeDynamic();

    OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
    OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);
    OCIO_CHECK_ASSERT(OCIO::DynamicPtrCast<OCIO::ECLinearRenderer>(renderer));

    std::vector<float> rgba = rgbaImage;

    renderer->apply(rgba.data(), rgba.data(), 4);

    OCIO_CHECK_EQUAL(rgba[0], linECVal(rgbaImage[0], const_ec));
    OCIO_CHECK_EQUAL(rgba[1], linECVal(rgbaImage[1], const_ec));
    OCIO_CHECK_EQUAL(rgba[2], linECVal(rgbaImage[2], const_ec));
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OCIO_CHECK_EQUAL(rgba[4], linECVal(rgbaImage[4], const_ec));
    OCIO_CHECK_EQUAL(rgba[5], linECVal(rgbaImage[5], const_ec));
    OCIO_CHECK_EQUAL(rgba[6], linECVal(rgbaImage[6], const_ec));
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);

    OCIO_CHECK_ASSERT(std::isnan(rgba[8]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[9]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[10]));
    OCIO_CHECK_EQUAL(rgba[12], linECVal(rgbaImage[12], const_ec));
    OCIO_CHECK_EQUAL(rgba[13], linECVal(rgbaImage[13], const_ec));
    OCIO_CHECK_EQUAL(rgba[14], linECVal(rgbaImage[14], const_ec));

    // Re-test with different E/C values.
    //
    ec->setExposure(0.2);
    ec->setContrast(1.5);
    ec->setGamma(1.2);

    rgba = rgbaImage;
    renderer->apply(rgba.data(), rgba.data(), 4);

    // As the ssePower is an approximation, strict equality is not possible.
    const float error = 5e-5f;

    OCIO_CHECK_CLOSE(rgba[0], linECVal(rgbaImage[0], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[1], linECVal(rgbaImage[1], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[2], linECVal(rgbaImage[2], const_ec), error);
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OCIO_CHECK_CLOSE(rgba[4], linECVal(rgbaImage[4], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[5], linECVal(rgbaImage[5], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[6], linECVal(rgbaImage[6], const_ec), error);
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);

    OCIO_CHECK_CLOSE(rgba[8], linECVal(rgbaImage[8], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[9], linECVal(rgbaImage[9], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[10], linECVal(rgbaImage[10], const_ec), error);
    OCIO_CHECK_EQUAL(rgba[12], linECVal(rgbaImage[12], const_ec));
    OCIO_CHECK_EQUAL(rgba[13], linECVal(rgbaImage[13], const_ec));
    OCIO_CHECK_EQUAL(rgba[14], linECVal(rgbaImage[14], const_ec));
}

namespace
{
void TestECInverse(OCIO::ExposureContrastOpData::Style style)
{
    const std::vector<float> rgbaImage { 0.0f, 0.5f, 1.f,     0.f,
                                         0.2f, 0.8f,  .99f, 128.f };

    OCIO::ExposureContrastOpDataRcPtr ec =
        std::make_shared<OCIO::ExposureContrastOpData>(style);

    ec->setExposure(1.5);
    ec->setContrast(0.5);
    ec->setGamma(1.1);
    ec->setPivot(0.18);

    OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
    OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);

    std::vector<float> rgba = rgbaImage;
    renderer->apply(rgba.data(), rgba.data(), 2);

    OCIO::ConstExposureContrastOpDataRcPtr const_eci = ec->inverse();
    OCIO::OpCPURcPtr rendereri = OCIO::GetExposureContrastCPURenderer(const_eci);
    rendereri->apply(rgba.data(), rgba.data(), 2);

    // As the ssePower is an approximation, strict equality is not possible.
    const float error = 1e-5f;

    OCIO_CHECK_CLOSE(rgba[0], rgbaImage[0], error);
    OCIO_CHECK_CLOSE(rgba[1], rgbaImage[1], error);
    OCIO_CHECK_CLOSE(rgba[2], rgbaImage[2], error);
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OCIO_CHECK_CLOSE(rgba[4], rgbaImage[4], error);
    OCIO_CHECK_CLOSE(rgba[5], rgbaImage[5], error);
    OCIO_CHECK_CLOSE(rgba[6], rgbaImage[6], error);
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);
}
}

OCIO_ADD_TEST(ExposureContrastRenderer, inverse)
{
    TestECInverse(OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC);
    TestECInverse(OCIO::ExposureContrastOpData::STYLE_LINEAR);
    TestECInverse(OCIO::ExposureContrastOpData::STYLE_VIDEO);
}

void TestLogParamForStyle(OCIO::ExposureContrastOpData::Style style, bool hasEffect)
{
    const std::vector<float> rgbaImage { 0.1f, 0.2f, 0.3f, 0.f,
                                                     0.4f, 0.5f, 0.6f, 0.f,
                                                     0.7f, 0.8f, 0.9f, 0.f };

    OCIO::ExposureContrastOpDataRcPtr ec =
        std::make_shared<OCIO::ExposureContrastOpData>(style);

    ec->setExposure(0.2);
    ec->setContrast(1.0);
    ec->setGamma(1.2);

    std::vector<float> rgbaRef = rgbaImage;

    // Reference.
    {
        OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
        OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);
        renderer->apply(rgbaRef.data(), rgbaRef.data(), 3);
    }

    // Change log parameters.
    ec->setLogExposureStep(0.1);
    ec->setLogMidGray(0.4);
    std::vector<float> rgba = rgbaImage;
    {
        OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
        OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);
        renderer->apply(rgba.data(), rgba.data(), 3);
        for (int i = 0; i < 12; ++i)
        {
            if (!hasEffect || i%4 == 3) // EC does not affect alpha
            {
                OCIO_CHECK_EQUAL(rgba[i], rgbaRef[i]);
            }
            else
            {
                OCIO_CHECK_NE(rgba[i], rgbaRef[i]);
            }
        }
    }
}

OCIO_ADD_TEST(ExposureContrastRenderer, log_params)
{
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_VIDEO, false);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_VIDEO_REV, false);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_LINEAR, false);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_LINEAR_REV, false);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC, true);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC_REV, true);
}

#endif
