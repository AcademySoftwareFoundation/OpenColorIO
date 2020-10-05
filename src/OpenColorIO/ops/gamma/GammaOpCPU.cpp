// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "ops/gamma/GammaOpCPU.h"
#include "ops/gamma/GammaOpUtils.h"

#include "SSE.h"


namespace OCIO_NAMESPACE
{

// Note: The parameters are validated when the op is created so that the
// math below does not require checks for divide by 0, etc.


// Base class for the Gamma (i.e. basic style) operation renderers.
class GammaBasicOpCPU : public OpCPU
{
public:
    GammaBasicOpCPU() = delete;
    GammaBasicOpCPU(const GammaBasicOpCPU &) = delete;
    explicit GammaBasicOpCPU(ConstGammaOpDataRcPtr & gamma);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void update(ConstGammaOpDataRcPtr & gamma);

protected:
    float m_redGamma;
    float m_grnGamma;
    float m_bluGamma;
    float m_alpGamma;
};

#ifdef USE_SSE
class GammaBasicOpCPUSSE : public GammaBasicOpCPU
{
public:
    explicit GammaBasicOpCPUSSE(ConstGammaOpDataRcPtr & gamma)
        : GammaBasicOpCPU(gamma)
    {
    }

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};
#endif

class GammaBasicMirrorOpCPU : public GammaBasicOpCPU
{
public:
    GammaBasicMirrorOpCPU() = delete;
    GammaBasicMirrorOpCPU(const GammaBasicOpCPU &) = delete;
    explicit GammaBasicMirrorOpCPU(ConstGammaOpDataRcPtr & gamma);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

#ifdef USE_SSE
class GammaBasicMirrorOpCPUSSE : public GammaBasicMirrorOpCPU
{
public:
    explicit GammaBasicMirrorOpCPUSSE(ConstGammaOpDataRcPtr & gamma)
        : GammaBasicMirrorOpCPU(gamma)
    {
    }

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};
#endif

class GammaBasicPassThruOpCPU : public GammaBasicOpCPU
{
public:
    GammaBasicPassThruOpCPU() = delete;
    GammaBasicPassThruOpCPU(const GammaBasicOpCPU &) = delete;
    explicit GammaBasicPassThruOpCPU(ConstGammaOpDataRcPtr & gamma);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

#ifdef USE_SSE
class GammaBasicPassThruOpCPUSSE : public GammaBasicPassThruOpCPU
{
public:
    explicit GammaBasicPassThruOpCPUSSE(ConstGammaOpDataRcPtr & gamma)
        : GammaBasicPassThruOpCPU(gamma)
    {
    }

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};
#endif

class GammaMoncurveOpCPU : public OpCPU
{
protected:
    explicit GammaMoncurveOpCPU(ConstGammaOpDataRcPtr &) : OpCPU() {}

protected:
    RendererParams m_red;
    RendererParams m_green;
    RendererParams m_blue;
    RendererParams m_alpha;
};

class GammaMoncurveOpCPUFwd : public GammaMoncurveOpCPU
{
public:
    explicit GammaMoncurveOpCPUFwd(ConstGammaOpDataRcPtr & gamma);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void update(ConstGammaOpDataRcPtr & gamma);
};

#ifdef USE_SSE
class GammaMoncurveOpCPUFwdSSE : public GammaMoncurveOpCPUFwd
{
public:
    explicit GammaMoncurveOpCPUFwdSSE(ConstGammaOpDataRcPtr & gamma)
        : GammaMoncurveOpCPUFwd(gamma)
    {
    }

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};
#endif

class GammaMoncurveOpCPURev : public GammaMoncurveOpCPU
{
public:
    explicit GammaMoncurveOpCPURev(ConstGammaOpDataRcPtr & gamma);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void update(ConstGammaOpDataRcPtr & gamma);

};

#ifdef USE_SSE
class GammaMoncurveOpCPURevSSE : public GammaMoncurveOpCPURev
{
public:
    explicit GammaMoncurveOpCPURevSSE(ConstGammaOpDataRcPtr & gamma)
        : GammaMoncurveOpCPURev(gamma)
    {
    }

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};
#endif

class GammaMoncurveMirrorOpCPUFwd : public GammaMoncurveOpCPU
{
public:
    explicit GammaMoncurveMirrorOpCPUFwd(ConstGammaOpDataRcPtr & gamma);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void update(ConstGammaOpDataRcPtr & gamma);
};

#ifdef USE_SSE
class GammaMoncurveMirrorOpCPUFwdSSE : public GammaMoncurveMirrorOpCPUFwd
{
public:
    explicit GammaMoncurveMirrorOpCPUFwdSSE(ConstGammaOpDataRcPtr & gamma)
        : GammaMoncurveMirrorOpCPUFwd(gamma)
    {
    }

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};
#endif

class GammaMoncurveMirrorOpCPURev : public GammaMoncurveOpCPU
{
public:
    explicit GammaMoncurveMirrorOpCPURev(ConstGammaOpDataRcPtr & gamma);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void update(ConstGammaOpDataRcPtr & gamma);
};

#ifdef USE_SSE
class GammaMoncurveMirrorOpCPURevSSE : public GammaMoncurveMirrorOpCPURev
{
public:
    explicit GammaMoncurveMirrorOpCPURevSSE(ConstGammaOpDataRcPtr & gamma)
        : GammaMoncurveMirrorOpCPURev(gamma)
    {
    }

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};
#endif

ConstOpCPURcPtr GetGammaRenderer(ConstGammaOpDataRcPtr & gamma, bool fastPower)
{
#ifndef USE_SSE
    std::ignore = fastPower;
#endif

    switch(gamma->getStyle())
    {
        case GammaOpData::MONCURVE_FWD:
        {
#ifdef USE_SSE
            if (fastPower) return std::make_shared<GammaMoncurveOpCPUFwdSSE>(gamma);
            else
#endif
                return std::make_shared<GammaMoncurveOpCPUFwd>(gamma);
            break;
        }

        case GammaOpData::MONCURVE_REV:
        {
#ifdef USE_SSE
            if (fastPower) return std::make_shared<GammaMoncurveOpCPURevSSE>(gamma);
            else
#endif
                return std::make_shared<GammaMoncurveOpCPURev>(gamma);
            break;
        }

        case GammaOpData::MONCURVE_MIRROR_FWD:
        {
#ifdef USE_SSE
            if (fastPower) return std::make_shared<GammaMoncurveMirrorOpCPUFwdSSE>(gamma);
            else
#endif
                return std::make_shared<GammaMoncurveMirrorOpCPUFwd>(gamma);
            break;
        }

        case GammaOpData::MONCURVE_MIRROR_REV:
        {
#ifdef USE_SSE
            if (fastPower) return std::make_shared<GammaMoncurveMirrorOpCPURevSSE>(gamma);
            else
#endif
                return std::make_shared<GammaMoncurveMirrorOpCPURev>(gamma);
            break;
        }

        case GammaOpData::BASIC_FWD:
        case GammaOpData::BASIC_REV:
        {
#ifdef USE_SSE
            if (fastPower) return std::make_shared<GammaBasicOpCPUSSE>(gamma);
            else
#endif
                return std::make_shared<GammaBasicOpCPU>(gamma);
            break;
        }
        case GammaOpData::BASIC_MIRROR_FWD:
        case GammaOpData::BASIC_MIRROR_REV:
        {
#ifdef USE_SSE
            if (fastPower) return std::make_shared<GammaBasicMirrorOpCPUSSE>(gamma);
            else
#endif
                return std::make_shared<GammaBasicMirrorOpCPU>(gamma);
            break;
        }
        case GammaOpData::BASIC_PASS_THRU_FWD:
        case GammaOpData::BASIC_PASS_THRU_REV:
        {
#ifdef USE_SSE
            if (fastPower) return std::make_shared<GammaBasicPassThruOpCPUSSE>(gamma);
            else
#endif
                return std::make_shared<GammaBasicPassThruOpCPU>(gamma);
            break;
        }
    }

    throw Exception("Unsupported Gamma style");
}


GammaBasicOpCPU::GammaBasicOpCPU(ConstGammaOpDataRcPtr & gamma)
    :   OpCPU()
    ,   m_redGamma(0.0f)
    ,   m_grnGamma(0.0f)
    ,   m_bluGamma(0.0f)
    ,   m_alpGamma(0.0f)
{
    update(gamma);
}

void GammaBasicOpCPU::update(ConstGammaOpDataRcPtr & gamma)
{
    // The gamma calculations are done in normalized space.
    const auto style = gamma->getStyle();
    const bool forward = (style == GammaOpData::BASIC_FWD) ||
                         (style == GammaOpData::BASIC_MIRROR_FWD) ||
                         (style == GammaOpData::BASIC_PASS_THRU_FWD);

    // Calculate the actual power used in the function.
    m_redGamma = (float)(forward ? gamma->getRedParams()[0]   : 1. / gamma->getRedParams()[0]);
    m_grnGamma = (float)(forward ? gamma->getGreenParams()[0] : 1. / gamma->getGreenParams()[0]);
    m_bluGamma = (float)(forward ? gamma->getBlueParams()[0]  : 1. / gamma->getBlueParams()[0]);
    m_alpGamma = (float)(forward ? gamma->getAlphaParams()[0] : 1. / gamma->getAlphaParams()[0]);
}

#ifdef USE_SSE
void GammaBasicOpCPUSSE::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const __m128 gamma = _mm_set_ps(m_alpGamma, m_bluGamma, m_grnGamma, m_redGamma);

    for(long idx=0; idx<numPixels; ++idx)
    {
        __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

        pixel = ssePower(pixel, gamma);

        _mm_storeu_ps(out, pixel);

        in  += 4;
        out += 4;
    }
}
#endif // USE_SSE

void GammaBasicOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;
    
    for(long idx=0; idx<numPixels; ++idx)
    {
        const float pixel[4] = { std::max(0.0f, in[0]), 
                                 std::max(0.0f, in[1]), 
                                 std::max(0.0f, in[2]),
                                 std::max(0.0f, in[3]) };

        out[0] = std::pow(pixel[0], m_redGamma);
        out[1] = std::pow(pixel[1], m_grnGamma);
        out[2] = std::pow(pixel[2], m_bluGamma);
        out[3] = std::pow(pixel[3], m_alpGamma);

        in  += 4;
        out += 4;
    }
}

GammaBasicMirrorOpCPU::GammaBasicMirrorOpCPU(ConstGammaOpDataRcPtr & gamma)
    : GammaBasicOpCPU(gamma)
{
}

#ifdef USE_SSE
void GammaBasicMirrorOpCPUSSE::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const __m128 gamma = _mm_set_ps(m_alpGamma, m_bluGamma, m_grnGamma, m_redGamma);

    for (long idx = 0; idx<numPixels; ++idx)
    {
        __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);
        __m128 sign_pix = _mm_and_ps(pixel, ESIGN_MASK);
        __m128 abs_pix = _mm_and_ps(pixel, EABS_MASK);

        pixel = ssePower(abs_pix, gamma);
        pixel = _mm_or_ps(sign_pix, pixel);

        _mm_storeu_ps(out, pixel);

        in += 4;
        out += 4;
    }
}
#endif

void GammaBasicMirrorOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float sign[4] = { std::copysign(1.0f, in[0]), std::copysign(1.0f, in[1]),
                                std::copysign(1.0f, in[2]), std::copysign(1.0f, in[3]) };
        const float pixel[4] = { std::fabs(in[0]), std::fabs(in[1]),
                                 std::fabs(in[2]), std::fabs(in[3]) };

        out[0] = sign[0] * std::pow(pixel[0], m_redGamma);
        out[1] = sign[1] * std::pow(pixel[1], m_grnGamma);
        out[2] = sign[2] * std::pow(pixel[2], m_bluGamma);
        out[3] = sign[3] * std::pow(pixel[3], m_alpGamma);

        in += 4;
        out += 4;
    }
}

GammaBasicPassThruOpCPU::GammaBasicPassThruOpCPU(ConstGammaOpDataRcPtr & gamma)
    : GammaBasicOpCPU(gamma)
{
}

#ifdef USE_SSE
void GammaBasicPassThruOpCPUSSE::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const __m128 gamma = _mm_set_ps(m_alpGamma, m_bluGamma, m_grnGamma, m_redGamma);
    const __m128 breakPnt = _mm_set_ps(0.0, 0.f, 0.f, 0.f);

    for (long idx = 0; idx<numPixels; ++idx)
    {
        __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);
        __m128 data = pixel;

        data = ssePower(data, gamma);

        __m128 flag = _mm_cmpgt_ps(pixel, breakPnt);

        data = _mm_or_ps(_mm_and_ps(flag, data),
                         _mm_andnot_ps(flag, pixel));

        _mm_storeu_ps(out, data);

        in += 4;
        out += 4;
    }
}
#endif

void GammaBasicPassThruOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float pixel[4] = { in[0], in[1], in[2], in[3] };

        out[0] = pixel[0] > 0.f ? std::pow(pixel[0], m_redGamma) : pixel[0];
        out[1] = pixel[1] > 0.f ? std::pow(pixel[1], m_grnGamma) : pixel[1];
        out[2] = pixel[2] > 0.f ? std::pow(pixel[2], m_bluGamma) : pixel[2];
        out[3] = pixel[3] > 0.f ? std::pow(pixel[3], m_alpGamma) : pixel[3];

        in += 4;
        out += 4;
    }
}

GammaMoncurveOpCPUFwd::GammaMoncurveOpCPUFwd(ConstGammaOpDataRcPtr & gamma)
    :   GammaMoncurveOpCPU(gamma)
{
    update(gamma);
}

void GammaMoncurveOpCPUFwd::update(ConstGammaOpDataRcPtr & gamma)
{
    ComputeParamsFwd(gamma->getRedParams(),   m_red);
    ComputeParamsFwd(gamma->getGreenParams(), m_green);
    ComputeParamsFwd(gamma->getBlueParams(),  m_blue);
    ComputeParamsFwd(gamma->getAlphaParams(), m_alpha);
}

#ifdef USE_SSE
void GammaMoncurveOpCPUFwdSSE::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const __m128 scale = _mm_set_ps(m_alpha.scale, m_blue.scale,
                                    m_green.scale, m_red.scale);

    const __m128 offset = _mm_set_ps(m_alpha.offset, m_blue.offset,
                                     m_green.offset, m_red.offset);

    const __m128 gamma = _mm_set_ps(m_alpha.gamma, m_blue.gamma,
                                    m_green.gamma, m_red.gamma);

    const __m128 breakPnt = _mm_set_ps(m_alpha.breakPnt, m_blue.breakPnt,
                                       m_green.breakPnt, m_red.breakPnt);

    const __m128 slope = _mm_set_ps(m_alpha.slope, m_blue.slope,
                                    m_green.slope, m_red.slope);

    for(long idx=0; idx<numPixels; ++idx)
    {
        __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

        __m128 data = _mm_add_ps(_mm_mul_ps(pixel, scale), offset);

        data = ssePower(data, gamma);

        __m128 flag = _mm_cmpgt_ps( pixel, breakPnt);

        data = _mm_or_ps(_mm_and_ps(flag, data),
                         _mm_andnot_ps(flag, _mm_mul_ps(pixel, slope )));

        _mm_storeu_ps(out, data);

        in  += 4;
        out += 4;
    }
}
#endif // USE_SSE

void GammaMoncurveOpCPUFwd::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const float red[5] = { m_red.scale,  m_red.offset,
                           m_red.gamma,  m_red.breakPnt, m_red.slope };
    const float grn[5] = { m_green.scale, m_green.offset, 
                           m_green.gamma, m_green.breakPnt, m_green.slope };
    const float blu[5] = { m_blue.scale,  m_blue.offset,
                           m_blue.gamma,  m_blue.breakPnt, m_blue.slope };
    const float alp[5] = { m_alpha.scale, m_alpha.offset, 
                           m_alpha.gamma, m_alpha.breakPnt, m_alpha.slope };

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float pixel[4] = { in[0], in[1], in[2], in[3] };

        const float data[4] = { std::pow(pixel[0] * red[0] + red[1], red[2]),
                                std::pow(pixel[1] * grn[0] + grn[1], grn[2]),
                                std::pow(pixel[2] * blu[0] + blu[1], blu[2]),
                                std::pow(pixel[3] * alp[0] + alp[1], alp[2]) };

        out[0] = pixel[0]<=red[3] ? pixel[0] * red[4] : data[0];
        out[1] = pixel[1]<=grn[3] ? pixel[1] * grn[4] : data[1];
        out[2] = pixel[2]<=blu[3] ? pixel[2] * blu[4] : data[2];
        out[3] = pixel[3]<=alp[3] ? pixel[3] * alp[4] : data[3];

        in  += 4;
        out += 4;
    }
}

GammaMoncurveOpCPURev::GammaMoncurveOpCPURev(ConstGammaOpDataRcPtr & gamma)
    :   GammaMoncurveOpCPU(gamma)
{
    update(gamma);
}

void GammaMoncurveOpCPURev::update(ConstGammaOpDataRcPtr & gamma)
{
    ComputeParamsRev(gamma->getRedParams(),   m_red);
    ComputeParamsRev(gamma->getGreenParams(), m_green);
    ComputeParamsRev(gamma->getBlueParams(),  m_blue);
    ComputeParamsRev(gamma->getAlphaParams(), m_alpha);
}

#ifdef USE_SSE
void GammaMoncurveOpCPURevSSE::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const __m128 scale = _mm_set_ps(m_alpha.scale, m_blue.scale,
                                    m_green.scale, m_red.scale);

    const __m128 offset = _mm_set_ps(m_alpha.offset, m_blue.offset,
                                     m_green.offset, m_red.offset);

    const __m128 gamma = _mm_set_ps(m_alpha.gamma, m_blue.gamma,
                                    m_green.gamma, m_red.gamma);

    const __m128 breakPnt = _mm_set_ps(m_alpha.breakPnt, m_blue.breakPnt,
                                       m_green.breakPnt, m_red.breakPnt);

    const __m128 slope = _mm_set_ps(m_alpha.slope, m_blue.slope,
                                    m_green.slope, m_red.slope);

    for(long idx=0; idx<numPixels; ++idx)
    {
        __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

        __m128 data = ssePower(pixel, gamma);

        data = _mm_sub_ps(_mm_mul_ps(data, scale), offset);

        __m128 flag = _mm_cmpgt_ps(pixel, breakPnt);

        data = _mm_or_ps(_mm_and_ps(flag, data),
                         _mm_andnot_ps(flag, _mm_mul_ps(pixel, slope)));

        _mm_storeu_ps(out, data);

        in  += 4;
        out += 4;
    }
}
#endif

void GammaMoncurveOpCPURev::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const float red[5] = { m_red.gamma,  m_red.scale,
                           m_red.offset, m_red.breakPnt, m_red.slope };
    const float grn[5] = { m_green.gamma, m_green.scale, 
                           m_green.offset,m_green.breakPnt, m_green.slope };
    const float blu[5] = { m_blue.gamma,  m_blue.scale,
                           m_blue.offset, m_blue.breakPnt, m_blue.slope  };
    const float alp[5] = { m_alpha.gamma,  m_alpha.scale, 
                           m_alpha.offset, m_alpha.breakPnt, m_alpha.slope };

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float pixel[4] = { in[0], in[1], in[2], in[3] };

        const float data[4] = { std::pow(pixel[0], red[0]) * red[1] - red[2],
                                std::pow(pixel[1], grn[0]) * grn[1] - grn[2],
                                std::pow(pixel[2], blu[0]) * blu[1] - blu[2],
                                std::pow(pixel[3], alp[0]) * alp[1] - alp[2] };

        out[0] = pixel[0]<=red[3] ? pixel[0] * red[4] : data[0];
        out[1] = pixel[1]<=grn[3] ? pixel[1] * grn[4] : data[1];
        out[2] = pixel[2]<=blu[3] ? pixel[2] * blu[4] : data[2];
        out[3] = pixel[3]<=alp[3] ? pixel[3] * alp[4] : data[3];

        in  += 4;
        out += 4;
    }
}

GammaMoncurveMirrorOpCPUFwd::GammaMoncurveMirrorOpCPUFwd(ConstGammaOpDataRcPtr & gamma)
    : GammaMoncurveOpCPU(gamma)
{
    update(gamma);
}

void GammaMoncurveMirrorOpCPUFwd::update(ConstGammaOpDataRcPtr & gamma)
{
    ComputeParamsFwd(gamma->getRedParams(), m_red);
    ComputeParamsFwd(gamma->getGreenParams(), m_green);
    ComputeParamsFwd(gamma->getBlueParams(), m_blue);
    ComputeParamsFwd(gamma->getAlphaParams(), m_alpha);
}

#ifdef USE_SSE
void GammaMoncurveMirrorOpCPUFwdSSE::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const __m128 scale = _mm_set_ps(m_alpha.scale, m_blue.scale,
                                    m_green.scale, m_red.scale);

    const __m128 offset = _mm_set_ps(m_alpha.offset, m_blue.offset,
                                     m_green.offset, m_red.offset);

    const __m128 gamma = _mm_set_ps(m_alpha.gamma, m_blue.gamma,
                                    m_green.gamma, m_red.gamma);

    const __m128 breakPnt = _mm_set_ps(m_alpha.breakPnt, m_blue.breakPnt,
                                       m_green.breakPnt, m_red.breakPnt);

    const __m128 slope = _mm_set_ps(m_alpha.slope, m_blue.slope,
                                    m_green.slope, m_red.slope);

    for (long idx = 0; idx < numPixels; ++idx)
    {
        __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);
        __m128 sign_pix = _mm_and_ps(pixel, ESIGN_MASK);
        __m128 abs_pix = _mm_and_ps(pixel, EABS_MASK);

        __m128 data = _mm_add_ps(_mm_mul_ps(abs_pix, scale), offset);

        data = ssePower(data, gamma);

        __m128 flagbrk = _mm_cmpgt_ps(abs_pix, breakPnt);

        data = _mm_or_ps(_mm_and_ps(flagbrk, data),
                         _mm_andnot_ps(flagbrk, _mm_mul_ps(abs_pix, slope)));

        data = _mm_or_ps(sign_pix, data);

        _mm_storeu_ps(out, data);

        in  += 4;
        out += 4;
    }
}
#endif

void GammaMoncurveMirrorOpCPUFwd::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;
    const float red[5] = { m_red.scale,  m_red.offset,
                           m_red.gamma,  m_red.breakPnt, m_red.slope };
    const float grn[5] = { m_green.scale, m_green.offset,
                           m_green.gamma, m_green.breakPnt, m_green.slope };
    const float blu[5] = { m_blue.scale,  m_blue.offset,
                           m_blue.gamma,  m_blue.breakPnt, m_blue.slope };
    const float alp[5] = { m_alpha.scale, m_alpha.offset,
                           m_alpha.gamma, m_alpha.breakPnt, m_alpha.slope };

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float sign[4] = { std::copysign(1.0f, in[0]), std::copysign(1.0f, in[1]),
                                std::copysign(1.0f, in[0]), std::copysign(1.0f, in[3]) };

        const float pixel[4] = { std::fabs(in[0]), std::fabs(in[1]),
                                 std::fabs(in[2]), std::fabs(in[3]) };

        const float data[4] = { std::pow(pixel[0] * red[0] + red[1], red[2]),
                                std::pow(pixel[1] * grn[0] + grn[1], grn[2]),
                                std::pow(pixel[2] * blu[0] + blu[1], blu[2]),
                                std::pow(pixel[3] * alp[0] + alp[1], alp[2]) };

        out[0] = sign[0] * (pixel[0] <= red[3] ? pixel[0] * red[4] : data[0]);
        out[1] = sign[1] * (pixel[1] <= grn[3] ? pixel[1] * grn[4] : data[1]);
        out[2] = sign[2] * (pixel[2] <= blu[3] ? pixel[2] * blu[4] : data[2]);
        out[3] = sign[3] * (pixel[3] <= alp[3] ? pixel[3] * alp[4] : data[3]);

        in  += 4;
        out += 4;
    }
}

GammaMoncurveMirrorOpCPURev::GammaMoncurveMirrorOpCPURev(ConstGammaOpDataRcPtr & gamma)
    : GammaMoncurveOpCPU(gamma)
{
    update(gamma);
}

void GammaMoncurveMirrorOpCPURev::update(ConstGammaOpDataRcPtr & gamma)
{
    ComputeParamsRev(gamma->getRedParams(), m_red);
    ComputeParamsRev(gamma->getGreenParams(), m_green);
    ComputeParamsRev(gamma->getBlueParams(), m_blue);
    ComputeParamsRev(gamma->getAlphaParams(), m_alpha);
}

#ifdef USE_SSE
void GammaMoncurveMirrorOpCPURevSSE::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const __m128 scale = _mm_set_ps(m_alpha.scale, m_blue.scale,
                                    m_green.scale, m_red.scale);

    const __m128 offset = _mm_set_ps(m_alpha.offset, m_blue.offset,
                                     m_green.offset, m_red.offset);

    const __m128 gamma = _mm_set_ps(m_alpha.gamma, m_blue.gamma,
                                    m_green.gamma, m_red.gamma);

    const __m128 breakPnt = _mm_set_ps(m_alpha.breakPnt, m_blue.breakPnt,
                                       m_green.breakPnt, m_red.breakPnt);

    const __m128 slope = _mm_set_ps(m_alpha.slope, m_blue.slope,
                                    m_green.slope, m_red.slope);

    for (long idx = 0; idx < numPixels; ++idx)
    {
        __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);
        __m128 sign_pix = _mm_and_ps(pixel, ESIGN_MASK);
        __m128 abs_pix = _mm_and_ps(pixel, EABS_MASK);

        __m128 data = ssePower(abs_pix, gamma);

        data = _mm_sub_ps(_mm_mul_ps(data, scale), offset);

        __m128 flagbrk = _mm_cmpgt_ps(abs_pix, breakPnt);

        data = _mm_or_ps(_mm_and_ps(flagbrk, data),
                         _mm_andnot_ps(flagbrk, _mm_mul_ps(abs_pix, slope)));

        data = _mm_or_ps(sign_pix, data);

        _mm_storeu_ps(out, data);

        in += 4;
        out += 4;
    }
}
#endif

void GammaMoncurveMirrorOpCPURev::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const float red[5] = { m_red.gamma,  m_red.scale,
                           m_red.offset, m_red.breakPnt, m_red.slope };
    const float grn[5] = { m_green.gamma, m_green.scale,
                           m_green.offset,m_green.breakPnt, m_green.slope };
    const float blu[5] = { m_blue.gamma,  m_blue.scale,
                           m_blue.offset, m_blue.breakPnt, m_blue.slope };
    const float alp[5] = { m_alpha.gamma,  m_alpha.scale,
                           m_alpha.offset, m_alpha.breakPnt, m_alpha.slope };

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float sign[4] = { std::copysign(1.0f, in[0]), std::copysign(1.0f, in[1]),
                                std::copysign(1.0f, in[0]), std::copysign(1.0f, in[3]) };

        const float pixel[4] = { std::fabs(in[0]), std::fabs(in[1]),
                                 std::fabs(in[2]), std::fabs(in[3]) };

        const float data[4] = { std::pow(pixel[0], red[0]) * red[1] - red[2],
                                std::pow(pixel[1], grn[0]) * grn[1] - grn[2],
                                std::pow(pixel[2], blu[0]) * blu[1] - blu[2],
                                std::pow(pixel[3], alp[0]) * alp[1] - alp[2] };

        out[0] = sign[0] * (pixel[0] <= red[3] ? pixel[0] * red[4] : data[0]);
        out[1] = sign[1] * (pixel[1] <= grn[3] ? pixel[1] * grn[4] : data[1]);
        out[2] = sign[2] * (pixel[2] <= blu[3] ? pixel[2] * blu[4] : data[2]);
        out[3] = sign[3] * (pixel[3] <= alp[3] ? pixel[3] * alp[4] : data[3]);

        in  += 4;
        out += 4;
    }
}

} // namespace OCIO_NAMESPACE