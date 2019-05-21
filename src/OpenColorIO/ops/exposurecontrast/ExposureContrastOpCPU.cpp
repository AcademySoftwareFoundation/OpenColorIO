/*
Copyright (c) 2019 Autodesk Inc., et al.
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
    explicit ECRendererBase(ConstExposureContrastOpDataRcPtr & ec);
    virtual ~ECRendererBase();

protected:
    virtual void updateData(ConstExposureContrastOpDataRcPtr & ec) = 0;

    DynamicPropertyImplRcPtr m_exposure;
    DynamicPropertyImplRcPtr m_contrast;
    DynamicPropertyImplRcPtr m_gamma;
    float m_inScale = 1.0f;
    float m_outScale = 1.0f;
    float m_alphaScale = 1.0f;
    float m_iPivot = 0.0f;
    float m_oPivot = 0.0f;
    float m_logExposureStep = 0.088f;
};

ECRendererBase::ECRendererBase(ConstExposureContrastOpDataRcPtr & ec)
    : OpCPU()
{
    m_inScale = GetBitDepthMaxValue(ec->getInputBitDepth());

    m_outScale = GetBitDepthMaxValue(ec->getOutputBitDepth());

    m_alphaScale = m_outScale / m_inScale;

    // Copy DynamicPropertyImpl sharedPtr so that if content changes in ec
    // change will be reflected here.
    m_exposure = ec->getExposureProperty();
    m_contrast = ec->getContrastProperty();
    m_gamma = ec->getGammaProperty();
}

ECRendererBase::~ECRendererBase()
{
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
    const float pivot = (float)std::max(EC::MIN_PIVOT, ec->getPivot());
    m_iPivot = pivot * m_inScale;
    m_oPivot = pivot * m_outScale;
}

void ECLinearRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    // TODO: allow negative contrast?
    const float contrastVal = (float)std::max(EC::MIN_CONTRAST,
                                              m_contrast->getDoubleValue() *
                                              m_gamma->getDoubleValue());
    const float exposureOverPivotVal = powf(2.f, (float)m_exposure->getDoubleValue())
                                                 / m_iPivot;

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE
    __m128 contrast = _mm_set1_ps(contrastVal);
    __m128 exposure_over_pivot = _mm_set1_ps(exposureOverPivotVal);
    __m128 oPiv = _mm_set1_ps(m_oPivot);

    if (contrastVal == 1.f)
    {
        __m128 multiplier = _mm_mul_ps(exposure_over_pivot, oPiv);
        for (long idx = 0; idx<numPixels; ++idx)
        {
            const float outAlpha = in[3] * m_alphaScale;
            __m128 data = _mm_set_ps(in[3],
                                     in[2],
                                     in[1],
                                     in[0]);
            //
            // out = in * exposure / iPivot * oPivot;
            //
            _mm_storeu_ps(out, _mm_mul_ps(data, multiplier));
            out[3] = outAlpha;

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx<numPixels; ++idx)
        {
            const float outAlpha = in[3] * m_alphaScale;
            __m128 data = _mm_set_ps(in[3],
                                     in[2],
                                     in[1],
                                     in[0]);

            //
            // out = powf( i * exposure / iPivot, contrast ) * oPivot
            //
            _mm_storeu_ps(out,
                _mm_mul_ps(
                    ssePower(
                        _mm_mul_ps(
                            data,
                            exposure_over_pivot),
                        contrast),
                    oPiv));
            out[3] = outAlpha;

            in += 4;
            out += 4;
        }
    }
#else
    if (contrastVal == 1.f)
    {
        const float multiplier = exposureOverPivotVal * m_oPivot;
        for (long idx = 0; idx<numPixels; ++idx)
        {
            //
            // out = in * exposure / iPivot * oPivot;
            //
            out[0] = in[0] * multiplier;
            out[1] = in[1] * multiplier;
            out[2] = in[2] * multiplier;
            out[3] = in[3] * m_alphaScale;

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx<numPixels; ++idx)
        {

            //
            // out = powf( i * exposure / iPivot, contrast ) * oPivot
            //
            // Note: With std::max NAN becomes 0.
            out[0] = powf(std::max(0.0f, in[0] * exposureOverPivotVal),
                           contrastVal) * m_oPivot;
            out[1] = powf(std::max(0.0f, in[1] * exposureOverPivotVal),
                           contrastVal) * m_oPivot;
            out[2] = powf(std::max(0.0f, in[2] * exposureOverPivotVal),
                           contrastVal) * m_oPivot;
            out[3] = in[3] * m_alphaScale;

            in += 4;
            out += 4;
        }
    }
#endif
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
    const float pivot = (float)std::max(EC::MIN_PIVOT, ec->getPivot());
    m_iPivot = pivot * m_inScale;
    m_oPivot = pivot * m_outScale;
}

void ECLinearRevRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    // TODO: allow negative contrast?
    const float contrastVal = (float)std::max(EC::MIN_CONTRAST,
                                              (m_contrast->getDoubleValue() * m_gamma->getDoubleValue()));
    const float invContrastVal = 1.f / contrastVal;
    const float opivotOverExposureVal = m_oPivot / powf(2.f, (float)m_exposure->getDoubleValue());
    const float invIpivotVal = 1.f / m_iPivot;

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE
    __m128 inv_contrast = _mm_set1_ps(invContrastVal);
    __m128 opivot_over_exposure = _mm_set1_ps(opivotOverExposureVal);
    __m128 inv_ipivot = _mm_set1_ps(invIpivotVal);

    if (contrastVal == 1.f)
    {
        _mm_mul_ps(opivot_over_exposure,inv_ipivot);
        for (long idx = 0; idx<numPixels; ++idx)
        {
            const float outAlpha = in[3] * m_alphaScale;
            __m128 data = _mm_set_ps(in[3],
                                     in[2],
                                     in[1],
                                     in[0]);

            //
            // out = (in / ( exposure * iPivot)) * oPivot;
            //
            _mm_storeu_ps(out,
                _mm_mul_ps(data, opivot_over_exposure));

            out[3] = outAlpha;

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx<numPixels; ++idx)
        {
            const float outAlpha = in[3] * m_alphaScale;
            __m128 data = _mm_set_ps(in[3],
                                     in[2],
                                     in[1],
                                     in[0]);

            //
            // out = powf( i / iPivot, 1 / contrast ) * oPivot / exposure
            //
            _mm_storeu_ps(out,
                _mm_mul_ps(
                    ssePower(
                        _mm_mul_ps(
                            data,
                            inv_ipivot),
                        inv_contrast),
                    opivot_over_exposure));

            out[3] = outAlpha;

            in += 4;
            out += 4;
        }
    }
#else
    if (contrastVal == 1.f)
    {
        const float multiplier = opivotOverExposureVal * invIpivotVal;
        for (long idx = 0; idx<numPixels; ++idx)
        {
            out[0] = in[0] * multiplier;
            out[1] = in[1] * multiplier;
            out[2] = in[2] * multiplier;
            out[3] = in[3] * m_alphaScale;

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx<numPixels; ++idx)
        {
            //
            // out = powf( i / iPivot, 1 / contrast ) * oPivot / exposure
            //
            out[0] = powf(std::max(0.0f, in[0] * invIpivotVal),
                           invContrastVal) * opivotOverExposureVal;
            out[1] = powf(std::max(0.0f, in[1] * invIpivotVal),
                           invContrastVal) * opivotOverExposureVal;
            out[2] = powf(std::max(0.0f, in[2] * invIpivotVal),
                           invContrastVal) * opivotOverExposureVal;
            out[3] = in[3] * m_alphaScale;

            in += 4;
            out += 4;
        }
    }
#endif
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
    const float pivot = powf((float)std::max(EC::MIN_PIVOT, ec->getPivot()),
                             (float)EC::VIDEO_OETF_POWER);
    m_iPivot = pivot * m_inScale;
    m_oPivot = pivot * m_outScale;
}

void ECVideoRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    // TODO: allow negative contrast?
    const float contrastVal = (float)std::max(EC::MIN_CONTRAST,
                                              (m_contrast->getDoubleValue() * m_gamma->getDoubleValue()));
    const float exposureOverPivotVal = powf(powf(2.f, (float)m_exposure->getDoubleValue()),
                                            (float)EC::VIDEO_OETF_POWER) / m_iPivot;

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE
    __m128 contrast = _mm_set1_ps(contrastVal);
    __m128 exposure_over_pivot = _mm_set1_ps(exposureOverPivotVal);
    __m128 oPiv = _mm_set1_ps(m_oPivot);

    if (contrastVal == 1.f)
    {
        __m128 multiplier = _mm_mul_ps(exposure_over_pivot, oPiv);
        for (long idx = 0; idx<numPixels; ++idx)
        {
            const float outAlpha = in[3] * m_alphaScale;
            __m128 data = _mm_set_ps(in[3],
                                     in[2],
                                     in[1],
                                     in[0]);

            //
            // out = in * exposure / iPivot * oPivot;
            //
            _mm_storeu_ps(out, _mm_mul_ps(data, multiplier));
            out[3] = outAlpha;

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx<numPixels; ++idx)
        {
            const float outAlpha = in[3] * m_alphaScale;
            __m128 data = _mm_set_ps(out[3],
                                     out[2],
                                     out[1],
                                     out[0]);

            //
            // out = powf( i * exposure / iPivot, contrast ) * oPivot
            //
            _mm_storeu_ps(out,
                _mm_mul_ps(
                    ssePower(
                        _mm_mul_ps(
                            data,
                            exposure_over_pivot),
                        contrast),
                    oPiv));
            out[3] = outAlpha;

            in += 4;
            out += 4;
        }
    }
#else
    if (contrastVal == 1.f)
    {
        const float multiplier = exposureOverPivotVal * m_oPivot;

        for (long idx = 0; idx<numPixels; ++idx)
        {
            //
            // out = in * exposure / iPivot * oPivot;
            //
            out[0] = in[0] * multiplier;
            out[1] = in[1] * multiplier;
            out[2] = in[2] * multiplier;
            out[3] = in[3] * m_alphaScale;

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx<numPixels; ++idx)
        {
            //
            // out = powf( i * exposure / iPivot, contrast ) * oPivot
            //
            out[0] = powf(std::max(0.0f, in[0] * exposureOverPivotVal),
                           contrastVal) * m_oPivot;
            out[1] = powf(std::max(0.0f, in[1] * exposureOverPivotVal),
                           contrastVal) * m_oPivot;
            out[2] = powf(std::max(0.0f, in[2] * exposureOverPivotVal),
                           contrastVal) * m_oPivot;
            out[3] = in[3] * m_alphaScale;

            in += 4;
            out += 4;
        }
    }
#endif
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
    const float pivot = powf((float)std::max(EC::MIN_PIVOT, ec->getPivot()),
                             (float)EC::VIDEO_OETF_POWER);
    m_iPivot = pivot * m_inScale;
    m_oPivot = pivot * m_outScale;
}

void ECVideoRevRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    // TODO: allow negative contrast?
    const float contrastVal = (float)std::max(EC::MIN_CONTRAST,
                                              (m_contrast->getDoubleValue() * m_gamma->getDoubleValue()));
    const float invContrastVal = 1.f / contrastVal;
    const float opivotOverExposureVal = m_oPivot / powf(powf(2.f, (float)m_exposure->getDoubleValue()),
                                                        (float)EC::VIDEO_OETF_POWER);
    const float invIpivotVal = 1.f / m_iPivot;


    const float * in = (float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE
    __m128 inv_contrast = _mm_set1_ps(invContrastVal);
    __m128 opivot_over_exposure = _mm_set1_ps(opivotOverExposureVal);
    __m128 inv_ipivot = _mm_set1_ps(invIpivotVal);

    if (contrastVal == 1.f)
    {
        __m128 multiplier = _mm_mul_ps(opivot_over_exposure, inv_ipivot);
            for (long idx = 0; idx<numPixels; ++idx)
        {
            const float outAlpha = in[3] * m_alphaScale;
            __m128 data = _mm_set_ps(in[3],
                                     in[2],
                                     in[1],
                                     in[0]);

            //
            // out = (in / ( exposure * iPivot)) * oPivot;
            //
            _mm_storeu_ps(out, _mm_mul_ps(data, multiplier));
            out[3] = outAlpha;

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx<numPixels; ++idx)
        {
            const float outAlpha = in[3] * m_alphaScale;
            __m128 data = _mm_set_ps(in[3],
                                     in[2],
                                     in[1],
                                     in[0]);

            //
            // out = powf( i / iPivot, 1 / contrast ) * oPivot / exposure
            //
            _mm_storeu_ps(out,
                _mm_mul_ps(
                    ssePower(
                        _mm_mul_ps(
                            data,
                            inv_ipivot),
                        inv_contrast),
                    opivot_over_exposure));
            out[3] = outAlpha;

            in += 4;
            out += 4;
        }
    }
#else
    if (contrastVal == 1.f)
    {
        const float multiplier = opivotOverExposureVal * invIpivotVal;
        for (long idx = 0; idx<numPixels; ++idx)
        {
            //
            // out = (in / ( exposure * iPivot)) * oPivot;
            //
            out[0] = in[0] * multiplier;
            out[1] = in[1] * multiplier;
            out[2] = in[2] * multiplier;
            out[3] = in[3] * m_alphaScale;

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx<numPixels; ++idx)
        {
            //
            // out = powf( i / iPivot, 1 / contrast ) * oPivot / exposure
            //
            out[0] = powf(std::max(0.0f, in[0] * invIpivotVal),
                           invContrastVal) * opivotOverExposureVal;
            out[1] = powf(std::max(0.0f, in[1] * invIpivotVal),
                           invContrastVal) * opivotOverExposureVal;
            out[2] = powf(std::max(0.0f, in[2] * invIpivotVal),
                           invContrastVal) * opivotOverExposureVal;
            out[3] = in[3] * m_alphaScale;

            in += 4;
            out += 4;
        }
    }
#endif
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
    const float logPivot = (float)std::max(0., log2(pivot / 0.18) *
                                               ec->getLogExposureStep() +
                                               ec->getLogMidGray());

    m_iPivot = logPivot * m_inScale;
    m_oPivot = logPivot * m_outScale;
    m_logExposureStep = (float)ec->getLogExposureStep();
}

void ECLogarithmicRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float exposureVal = (float)m_exposure->getDoubleValue() *
                              m_logExposureStep * m_inScale;
    const float contrastVal
        = (float)std::max(EC::MIN_CONTRAST,
                          (m_contrast->getDoubleValue() * m_gamma->getDoubleValue())) * m_alphaScale;
    const float offsetVal = (exposureVal - m_iPivot) * contrastVal + m_oPivot;

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE
    // Equation is:
    // out = ( ( (in/iscale) + expos) - pivot ) * contrast + pivot ) * oscale
    // Rearrange as:
    // out = [in * contrast*oscale/iscale] + [(expos - pivot) * contrast * oscale + pivot * oscale]

    __m128 contrast = _mm_set1_ps(contrastVal);
    __m128 offset = _mm_set1_ps(offsetVal);

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float outAlpha = in[3] * m_alphaScale;
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
        out[3] = in[3] * m_alphaScale;

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
    const float logPivot = (float)std::max(0., log2(pivot / 0.18) *
                                                ec->getLogExposureStep() +
                                                ec->getLogMidGray());

    m_iPivot = logPivot * m_inScale;
    m_oPivot = logPivot * m_outScale;
}

void ECLogarithmicRevRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float exposureVal = (float)m_exposure->getDoubleValue() *
                              m_logExposureStep * m_inScale;
    const float inv_contrastVal
        = (float)std::max(EC::MIN_CONTRAST,
                          1. / (m_contrast->getDoubleValue() * m_gamma->getDoubleValue()))
          * m_alphaScale;
    const float negOffsetVal = m_oPivot - m_iPivot * inv_contrastVal -
                               exposureVal * m_alphaScale;

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE
    // Equation is:
    // out = ( ( (in/iscale) - pivot) / contrast + pivot) - exposure ) * oscale
    // Rearrange as:
    // out = [in*oscale/iscale/contrast] - [pivot*oscale/contrast + pivot*oscale - exposure*oscale]

    __m128 inv_contrast = _mm_set1_ps(inv_contrastVal);
    __m128 neg_offset = _mm_set1_ps(negOffsetVal);

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float outAlpha = in[3] * m_alphaScale;
        __m128 data = _mm_set_ps(in[3],
                                 in[2],
                                 in[1],
                                 in[0]);

        //
        // out = ( in * inv_contrast ) + neg_offset
        //
        _mm_storeu_ps(out,
            _mm_add_ps(neg_offset,
                _mm_mul_ps(data, inv_contrast)));
        out[3] = outAlpha;

        in += 4;
        out += 4;
    }
#else
    for (long idx = 0; idx<numPixels; ++idx)
    {
        //
        // out = ( in * inv_contrast ) + neg_offset
        //
        out[0] = in[0] * inv_contrastVal + negOffsetVal;
        out[1] = in[1] * inv_contrastVal + negOffsetVal;
        out[2] = in[2] * inv_contrastVal + negOffsetVal;
        out[3] = in[3] * m_alphaScale;

        in += 4;
        out += 4;
    }
#endif
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
#include "unittest.h"

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
    //     powf(       std::max(0.0f, t[0] * exposure / m_iPivot     ), contrast) * m_oPivot,

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

OIIO_ADD_TEST(ExposureContrastRenderer, video)
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
            OCIO::BIT_DEPTH_F32,
            OCIO::BIT_DEPTH_F32,
            OCIO::ExposureContrastOpData::STYLE_VIDEO);

    ec->getExposureProperty()->makeDynamic();
    ec->getContrastProperty()->makeDynamic();
    ec->getGammaProperty()->makeDynamic();

    OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
    OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);
    OIIO_CHECK_ASSERT(OCIO::DynamicPtrCast<OCIO::ECVideoRenderer>(renderer));
    std::vector<float> rgba = rgbaImage;

    renderer->apply(rgba.data(), rgba.data(), 4);

    OIIO_CHECK_EQUAL(rgba[0], videoECVal(rgbaImage[0], const_ec));
    OIIO_CHECK_EQUAL(rgba[1], videoECVal(rgbaImage[1], const_ec));
    OIIO_CHECK_EQUAL(rgba[2], videoECVal(rgbaImage[2], const_ec));
    OIIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OIIO_CHECK_EQUAL(rgba[4], videoECVal(rgbaImage[4], const_ec));
    OIIO_CHECK_EQUAL(rgba[5], videoECVal(rgbaImage[5], const_ec));
    OIIO_CHECK_EQUAL(rgba[6], videoECVal(rgbaImage[6], const_ec));
    OIIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);

    OIIO_CHECK_ASSERT(std::isnan(rgba[8]));
    OIIO_CHECK_ASSERT(std::isnan(rgba[9]));
    OIIO_CHECK_ASSERT(std::isnan(rgba[10]));
    OIIO_CHECK_EQUAL(rgba[12], videoECVal(rgbaImage[12], const_ec));
    OIIO_CHECK_EQUAL(rgba[13], videoECVal(rgbaImage[13], const_ec));
    OIIO_CHECK_EQUAL(rgba[14], videoECVal(rgbaImage[14], const_ec));

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
    OIIO_CHECK_CLOSE(rgba[0], videoECVal(rgbaImage[0], const_ec), error);
    OIIO_CHECK_CLOSE(rgba[1], videoECVal(rgbaImage[1], const_ec), error);
    OIIO_CHECK_CLOSE(rgba[2], videoECVal(rgbaImage[2], const_ec), error);
    OIIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OIIO_CHECK_CLOSE(rgba[4], videoECVal(rgbaImage[4], const_ec), error);
    OIIO_CHECK_CLOSE(rgba[5], videoECVal(rgbaImage[5], const_ec), error);
    OIIO_CHECK_CLOSE(rgba[6], videoECVal(rgbaImage[6], const_ec), error);
    OIIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);

    OIIO_CHECK_CLOSE(rgba[8], videoECVal(rgbaImage[8], const_ec), error);
    OIIO_CHECK_CLOSE(rgba[9], videoECVal(rgbaImage[9], const_ec), error);
    OIIO_CHECK_CLOSE(rgba[10], videoECVal(rgbaImage[10], const_ec), error);
    OIIO_CHECK_EQUAL(rgba[12], videoECVal(rgbaImage[12], const_ec));
    OIIO_CHECK_EQUAL(rgba[13], videoECVal(rgbaImage[13], const_ec));
    OIIO_CHECK_EQUAL(rgba[14], videoECVal(rgbaImage[14], const_ec));
}

OIIO_ADD_TEST(ExposureContrastRenderer, log)
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
            OCIO::BIT_DEPTH_F32,
            OCIO::BIT_DEPTH_F32,
            OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC);

    ec->getExposureProperty()->makeDynamic();
    ec->getContrastProperty()->makeDynamic();
    ec->getGammaProperty()->makeDynamic();
    ec->setExposure(1.2);
    ec->setPivot(0.18);

    OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
    OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);
    OIIO_CHECK_ASSERT(OCIO::DynamicPtrCast<OCIO::ECLogarithmicRenderer>(renderer));

    std::vector<float> rgba = rgbaImage;

    renderer->apply(rgba.data(), rgba.data(), 4);

    const float inMax = OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F32);
    const float outMax = OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(rgba[0], logECVal(rgbaImage[0], const_ec, inMax, outMax));
    OIIO_CHECK_EQUAL(rgba[1], logECVal(rgbaImage[1], const_ec, inMax, outMax));
    OIIO_CHECK_EQUAL(rgba[2], logECVal(rgbaImage[2], const_ec, inMax, outMax));
    OIIO_CHECK_EQUAL(rgba[3], rgbaImage[3] * outMax / inMax);
    OIIO_CHECK_EQUAL(rgba[4], logECVal(rgbaImage[4], const_ec, inMax, outMax));
    OIIO_CHECK_EQUAL(rgba[5], logECVal(rgbaImage[5], const_ec, inMax, outMax));
    OIIO_CHECK_EQUAL(rgba[6], logECVal(rgbaImage[6], const_ec, inMax, outMax));
    OIIO_CHECK_EQUAL(rgba[7], rgbaImage[7] * outMax / inMax);

    OIIO_CHECK_ASSERT(std::isnan(rgba[8]));
    OIIO_CHECK_ASSERT(std::isnan(rgba[9]));
    OIIO_CHECK_ASSERT(std::isnan(rgba[10]));
    OIIO_CHECK_EQUAL(rgba[12], logECVal(rgbaImage[12], const_ec));
    OIIO_CHECK_EQUAL(rgba[13], logECVal(rgbaImage[13], const_ec));
    OIIO_CHECK_EQUAL(rgba[14], logECVal(rgbaImage[14], const_ec));

    // Re-test with differenc E/C values.
    //
    ec->setExposure(0.2);
    ec->setContrast(0.5);
    ec->setGamma(1.6);
    
    rgba = rgbaImage;
    renderer->apply(rgba.data(), rgba.data(), 4);

    OIIO_CHECK_EQUAL(rgba[0], logECVal(rgbaImage[0], const_ec, inMax, outMax));
    OIIO_CHECK_EQUAL(rgba[1], logECVal(rgbaImage[1], const_ec, inMax, outMax));
    OIIO_CHECK_EQUAL(rgba[2], logECVal(rgbaImage[2], const_ec, inMax, outMax));
    OIIO_CHECK_EQUAL(rgba[3], rgbaImage[3] * outMax / inMax);
    OIIO_CHECK_EQUAL(rgba[4], logECVal(rgbaImage[4], const_ec, inMax, outMax));
    OIIO_CHECK_EQUAL(rgba[5], logECVal(rgbaImage[5], const_ec, inMax, outMax));
    OIIO_CHECK_EQUAL(rgba[6], logECVal(rgbaImage[6], const_ec, inMax, outMax));
    OIIO_CHECK_EQUAL(rgba[7], rgbaImage[7] * outMax / inMax);

    OIIO_CHECK_ASSERT(std::isnan(rgba[8]));
    OIIO_CHECK_ASSERT(std::isnan(rgba[9]));
    OIIO_CHECK_ASSERT(std::isnan(rgba[10]));
    OIIO_CHECK_EQUAL(rgba[12], logECVal(rgbaImage[12], const_ec));
    OIIO_CHECK_EQUAL(rgba[13], logECVal(rgbaImage[13], const_ec));
    OIIO_CHECK_EQUAL(rgba[14], logECVal(rgbaImage[14], const_ec));
}

OIIO_ADD_TEST(ExposureContrastRenderer, linear)
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
            OCIO::BIT_DEPTH_F32,
            OCIO::BIT_DEPTH_F32,
            OCIO::ExposureContrastOpData::STYLE_LINEAR);

    ec->getExposureProperty()->makeDynamic();
    ec->getContrastProperty()->makeDynamic();
    ec->getGammaProperty()->makeDynamic();

    OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
    OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);
    OIIO_CHECK_ASSERT(OCIO::DynamicPtrCast<OCIO::ECLinearRenderer>(renderer));

    std::vector<float> rgba = rgbaImage;

    renderer->apply(rgba.data(), rgba.data(), 4);

    OIIO_CHECK_EQUAL(rgba[0], linECVal(rgbaImage[0], const_ec));
    OIIO_CHECK_EQUAL(rgba[1], linECVal(rgbaImage[1], const_ec));
    OIIO_CHECK_EQUAL(rgba[2], linECVal(rgbaImage[2], const_ec));
    OIIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OIIO_CHECK_EQUAL(rgba[4], linECVal(rgbaImage[4], const_ec));
    OIIO_CHECK_EQUAL(rgba[5], linECVal(rgbaImage[5], const_ec));
    OIIO_CHECK_EQUAL(rgba[6], linECVal(rgbaImage[6], const_ec));
    OIIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);

    OIIO_CHECK_ASSERT(std::isnan(rgba[8]));
    OIIO_CHECK_ASSERT(std::isnan(rgba[9]));
    OIIO_CHECK_ASSERT(std::isnan(rgba[10]));
    OIIO_CHECK_EQUAL(rgba[12], linECVal(rgbaImage[12], const_ec));
    OIIO_CHECK_EQUAL(rgba[13], linECVal(rgbaImage[13], const_ec));
    OIIO_CHECK_EQUAL(rgba[14], linECVal(rgbaImage[14], const_ec));

    // Re-test with different E/C values.
    //
    ec->setExposure(0.2);
    ec->setContrast(1.5);
    ec->setGamma(1.2);

    rgba = rgbaImage;
    renderer->apply(rgba.data(), rgba.data(), 4);

    // As the ssePower is an approximation, strict equality is not possible.
    const float error = 5e-5f;

    OIIO_CHECK_CLOSE(rgba[0], linECVal(rgbaImage[0], const_ec), error);
    OIIO_CHECK_CLOSE(rgba[1], linECVal(rgbaImage[1], const_ec), error);
    OIIO_CHECK_CLOSE(rgba[2], linECVal(rgbaImage[2], const_ec), error);
    OIIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OIIO_CHECK_CLOSE(rgba[4], linECVal(rgbaImage[4], const_ec), error);
    OIIO_CHECK_CLOSE(rgba[5], linECVal(rgbaImage[5], const_ec), error);
    OIIO_CHECK_CLOSE(rgba[6], linECVal(rgbaImage[6], const_ec), error);
    OIIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);

    OIIO_CHECK_CLOSE(rgba[8], linECVal(rgbaImage[8], const_ec), error);
    OIIO_CHECK_CLOSE(rgba[9], linECVal(rgbaImage[9], const_ec), error);
    OIIO_CHECK_CLOSE(rgba[10], linECVal(rgbaImage[10], const_ec), error);
    OIIO_CHECK_EQUAL(rgba[12], linECVal(rgbaImage[12], const_ec));
    OIIO_CHECK_EQUAL(rgba[13], linECVal(rgbaImage[13], const_ec));
    OIIO_CHECK_EQUAL(rgba[14], linECVal(rgbaImage[14], const_ec));
}

namespace
{
void TestECInverse(OCIO::ExposureContrastOpData::Style style)
{
    const std::vector<float> rgbaImage { 0.0f, 0.5f, 1.f,     0.f,
                                         0.2f, 0.8f,  .99f, 128.f };

    OCIO::ExposureContrastOpDataRcPtr ec =
        std::make_shared<OCIO::ExposureContrastOpData>(
            OCIO::BIT_DEPTH_F32,
            OCIO::BIT_DEPTH_F32,
            style);

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

    OIIO_CHECK_CLOSE(rgba[0], rgbaImage[0], error);
    OIIO_CHECK_CLOSE(rgba[1], rgbaImage[1], error);
    OIIO_CHECK_CLOSE(rgba[2], rgbaImage[2], error);
    OIIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OIIO_CHECK_CLOSE(rgba[4], rgbaImage[4], error);
    OIIO_CHECK_CLOSE(rgba[5], rgbaImage[5], error);
    OIIO_CHECK_CLOSE(rgba[6], rgbaImage[6], error);
    OIIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);
}
}

OIIO_ADD_TEST(ExposureContrastRenderer, inverse)
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
        std::make_shared<OCIO::ExposureContrastOpData>(
            OCIO::BIT_DEPTH_F32,
            OCIO::BIT_DEPTH_F32,
            style);

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
                OIIO_CHECK_EQUAL(rgba[i], rgbaRef[i]);
            }
            else
            {
                OIIO_CHECK_NE(rgba[i], rgbaRef[i]);
            }
        }
    }
}

OIIO_ADD_TEST(ExposureContrastRenderer, log_params)
{
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_VIDEO, false);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_VIDEO_REV, false);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_LINEAR, false);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_LINEAR_REV, false);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC, true);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC_REV, true);
}

#endif
