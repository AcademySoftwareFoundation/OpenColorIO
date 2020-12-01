// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "DynamicProperty.h"
#include "ops/exposurecontrast/ExposureContrastOpCPU.h"
#include "SSE.h"

namespace OCIO_NAMESPACE
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

    DynamicPropertyDoubleImplRcPtr m_exposure;
    DynamicPropertyDoubleImplRcPtr m_contrast;
    DynamicPropertyDoubleImplRcPtr m_gamma;

    float m_pivot = 0.0f;
    float m_logExposureStep = 0.088f;
};

ECRendererBase::ECRendererBase(ConstExposureContrastOpDataRcPtr & ec)
    : OpCPU()
{
    // Initialized with the instances from the processor and decouple them.
    m_exposure = ec->getExposureProperty();
    m_contrast = ec->getContrastProperty();
    m_gamma = ec->getGammaProperty();
    if (m_exposure->isDynamic())
    {
        m_exposure = m_exposure->createEditableCopy();
    }
    if (m_contrast->isDynamic())
    {
        m_contrast = m_contrast->createEditableCopy();
    }
    if (m_gamma->isDynamic())
    {
        m_gamma = m_gamma->createEditableCopy();
    }
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
        case DYNAMIC_PROPERTY_GRADING_PRIMARY:
        case DYNAMIC_PROPERTY_GRADING_RGBCURVE:
        case DYNAMIC_PROPERTY_GRADING_TONE:
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
        case DYNAMIC_PROPERTY_GRADING_PRIMARY:
        case DYNAMIC_PROPERTY_GRADING_RGBCURVE:
        case DYNAMIC_PROPERTY_GRADING_TONE:
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
    // TODO: is it worth adding a code path without dynamic parameters?
    const float contrastVal = (float)std::max(EC::MIN_CONTRAST,
                                              m_contrast->getValue() *
                                              m_gamma->getValue());
    const float exposureVal = powf(2.f, (float)m_exposure->getValue());

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
                                              (m_contrast->getValue() * m_gamma->getValue()));
    const float invContrastVal = 1.f / contrastVal;
    const float invExposureVal = 1.f / powf(2.f, (float)m_exposure->getValue());

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
                                              (m_contrast->getValue() * m_gamma->getValue()));
    const float exposureVal = powf(powf(2.f, (float)m_exposure->getValue()),
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
                                              (m_contrast->getValue() * m_gamma->getValue()));
    const float invContrastVal = 1.f / contrastVal;
    const float invExposureVal = 1.f / powf(powf(2.f, (float)m_exposure->getValue()),
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
    const float exposureVal = (float)m_exposure->getValue() *
                              m_logExposureStep;
    const float contrastVal
        = (float)std::max(EC::MIN_CONTRAST,
                          (m_contrast->getValue() * m_gamma->getValue()));
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
    const float exposureVal = (float)m_exposure->getValue() *
                              m_logExposureStep;
    const float inv_contrastVal
        = (float)std::max(EC::MIN_CONTRAST,
                          1. / (m_contrast->getValue() * m_gamma->getValue()));
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

} // namespace OCIO_NAMESPACE

