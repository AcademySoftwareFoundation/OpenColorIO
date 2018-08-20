/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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


#include "CPUCDLOp.h"

#include "../BitDepthUtils.h"
#include "SSE2.h"

#include <algorithm>

OCIO_NAMESPACE_ENTER
{
    static const float RcpMinValue = 1e-2f;

    inline float Reciprocal(const float x)
    {
        return 1.0f / std::max(x, RcpMinValue);
    }

    namespace CDLOpUtil
    {
        RenderParams::RenderParams()
            : m_isReverse(false)
            , m_isNoClamp(false)
        {
            setSlope(1.0f, 1.0f, 1.0f, 1.0f);
            setOffset(0.0f, 0.0f, 0.0f, 1.0f);
            setPower(1.0f, 1.0f, 1.0f, 1.0f);
            setSaturation(1.0f);
        }

        void RenderParams::setSlope(const float r,
                                    const float g,
                                    const float b,
                                    const float a)
        {
            m_slope[0] = r;
            m_slope[1] = g;
            m_slope[2] = b;
            m_slope[3] = a;
        }

        void RenderParams::setOffset(const float r,
                                     const float g,
                                     const float b,
                                     const float a)
        {
            m_offset[0] = r;
            m_offset[1] = g;
            m_offset[2] = b;
            m_offset[3] = a;
        }

        void RenderParams::setPower(const float r,
                                    const float g,
                                    const float b,
                                    const float a)
        {
            m_power[0] = r;
            m_power[1] = g;
            m_power[2] = b;
            m_power[3] = a;
        }

        void RenderParams::setSaturation(const float sat)
        {
            m_saturation = sat;
        }

        void RenderParams::update(const OpData::OpDataCDLRcPtr & cdl)
        {
            float slope[4], offset[4], power[4];
            cdl->getSlopeParams().getRGBA(slope);
            cdl->getOffsetParams().getRGBA(offset);
            cdl->getPowerParams().getRGBA(power);

            const float saturation = (float)cdl->getSaturation();
            const OpData::CDL::CDLStyle style = cdl->getCDLStyle();

            m_isReverse
                = (style == OpData::CDL::CDL_V1_2_REV)
                || (style == OpData::CDL::CDL_NO_CLAMP_REV);

            m_isNoClamp
                = (style == OpData::CDL::CDL_NO_CLAMP_FWD)
                || (style == OpData::CDL::CDL_NO_CLAMP_REV);

            if (isReverse())
            {
                // Reverse render parameters
                setSlope(Reciprocal(slope[0]),
                    Reciprocal(slope[1]),
                    Reciprocal(slope[2]),
                    Reciprocal(slope[3]));
                setOffset(-offset[0], -offset[1], -offset[2], -offset[3]);
                setPower(Reciprocal(power[0]),
                    Reciprocal(power[1]),
                    Reciprocal(power[2]),
                    Reciprocal(power[3]));
                setSaturation(Reciprocal(saturation));
            }
            else
            {
                // Forward render parameters
                setSlope(slope[0], slope[1], slope[2], slope[3]);
                setOffset(offset[0], offset[1], offset[2], offset[3]);
                setPower(power[0], power[1], power[2], power[3]);
                setSaturation(saturation);
            }
        }
    }

    namespace
    {
        static const __m128 LumaWeights = _mm_setr_ps(0.2126f, 0.7152f, 0.0722f, 0.0);

        // Load a given pixel from the pixel list into a SSE register
        inline __m128 LoadPixel(const float * rgbaBuffer, float & inAlpha)
        {
            inAlpha = rgbaBuffer[3];
            return _mm_setr_ps(rgbaBuffer[0], rgbaBuffer[1], rgbaBuffer[2], 0.0f);
        }

        // Store the pixel's values into a given pixel list's position
        inline void StorePixel(float * rgbaBuffer, const __m128 pix, const float outAlpha)
        {
            // TODO: use aligned version if rgbaBuffer alignement can be controlled (see OCIO_ALIGN)
            _mm_storeu_ps(rgbaBuffer, pix);

            rgbaBuffer[3] = outAlpha;
        }

        // Map pixel values from the input domain to the unit domain
        inline void ApplyInScale(__m128 & pix, const __m128 inScale)
        {
            pix = _mm_mul_ps(pix, inScale);
        }

        // Map result from unit domain to the output domain
        inline void ApplyOutScale(__m128& pix, const __m128 outScale)
        {
            pix = _mm_mul_ps(pix, outScale);
        }

        // Apply the slope component to the the pixel's values
        inline void ApplySlope(__m128& pix, const __m128 slope)
        {
            pix = _mm_mul_ps(pix, slope);
        }

        // Apply the offset component to the the pixel's values
        inline void ApplyOffset(__m128& pix, const __m128 offset)
        {
            pix = _mm_add_ps(pix, offset);
        }

        // Conditionally clamp the pixel's values to the range [0, 1]
        // When the template argument is true, the clamp mode is used,
        // and the values in pix are clamped to the range [0,1]. When
        // the argument is false, nothing is done.
        template<bool>
        inline void ApplyClamp(__m128& pix)
        {
            pix = _mm_min_ps(_mm_max_ps(pix, EZERO), EONE);
        }

        template<>
        inline void ApplyClamp<false>(__m128&)
        {
        }

        // Apply the power component to the the pixel's values
        // When the template argument is true, the the values in pix
        // are clamped to the range [0,1] and the power operation is
        // applied. When the argument is false, the values in pix are
        // not clamped before the power operation is applied. When the
        // base is negative in this mode, pixel values are just passed
        // through.
        template<bool>
        inline void ApplyPower(__m128& pix, const __m128& power)
        {
            ApplyClamp<true>(pix);
            pix = ssePower(pix, power);
        }

        template<>
        inline void ApplyPower<false>(__m128& pix, const __m128& power)
        {
            __m128 negMask = _mm_cmplt_ps(pix, EZERO);
            __m128 pixPower = ssePower(pix, power);
            pix = sseSelect(pixPower, pix, negMask);
        }

        // Apply the saturation component to the the pixel's values
        inline void ApplySaturation(__m128& pix, const __m128 saturation)
        {
            // Compute luma: dot product of pixel values and the luma weigths
            __m128 luma = _mm_mul_ps(pix, LumaWeights);

            // luma = [ x+y , y+x , z+w , w+z ]
            luma = _mm_add_ps(luma, _mm_shuffle_ps(luma, luma, _MM_SHUFFLE(2,3,0,1)));

            // luma = [ x+y+z+w , y+x+w+z , z+w+x+y , w+z+y+x ]
            luma = _mm_add_ps(luma, _mm_shuffle_ps(luma, luma, _MM_SHUFFLE(1,0,3,2)));

            // Apply saturation
            pix = _mm_add_ps(luma, _mm_mul_ps(saturation, _mm_sub_ps(pix, luma)));
        }

    };

    CPUCDLOp::CPUCDLOp(const OpData::OpDataCDLRcPtr & cdl)
        :   CPUOp()
        ,   m_alphaScale(1.0f)
    {
        m_inScale = 1.0f / GetBitDepthMaxValue(cdl->getInputBitDepth());
        m_outScale = GetBitDepthMaxValue(cdl->getOutputBitDepth());
        m_alphaScale = m_inScale * m_outScale;

        m_renderParams.update(cdl);
    }


    void CPUCDLOp::loadRenderParams(__m128 & inScale, 
                                    __m128 & outScale,
                                    __m128 & slope,
                                    __m128 & offset,
                                    __m128 & power,
                                    __m128 & saturation) const
    {
        inScale    = _mm_set1_ps(m_inScale);
        outScale   = _mm_set1_ps(m_outScale);
        slope      = _mm_loadu_ps(m_renderParams.getSlope());
        offset     = _mm_loadu_ps(m_renderParams.getOffset());
        power      = _mm_loadu_ps(m_renderParams.getPower());
        saturation = _mm_set1_ps(m_renderParams.getSaturation());
    }

    CDLRendererV1_2Fwd::CDLRendererV1_2Fwd(const OpData::OpDataCDLRcPtr & cdl)
        :   CPUCDLOp(cdl)
    {
    }

    void CDLRendererV1_2Fwd::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        _apply<true>(rgbaBuffer, numPixels);
    }

    template<bool CLAMP>
    void CDLRendererV1_2Fwd::_apply(float * rgbaBuffer, unsigned numPixels) const
    {
#ifdef USE_SSE
        __m128 inScale, outScale, slope, offset, power, saturation, pix;
        loadRenderParams(inScale, outScale, slope, offset, power, saturation);

        float inAlpha;

        // TODO: Add the ability to not overwrite the input buffer.
        // At that time, memory align the output buffer (see OCIO_ALIGN).
        float * rgba = rgbaBuffer;

        for(unsigned idx=0; idx<numPixels; ++idx)
        {
            pix = LoadPixel(rgba, inAlpha);

            ApplyInScale(pix, inScale);

            ApplySlope(pix, slope);
            ApplyOffset(pix, offset);

            ApplyPower<CLAMP>(pix, power);

            ApplySaturation(pix, saturation);
            ApplyClamp<CLAMP>(pix);

            ApplyOutScale(pix, outScale);

            StorePixel(rgba, pix, inAlpha * m_alphaScale);

            rgba += 4;
        }
#else
#pragma message( __FILE__ "CDL requires SSE.")
#endif
    }

    CDLRendererNoClampFwd::CDLRendererNoClampFwd(const OpData::OpDataCDLRcPtr & cdl)
        :   CDLRendererV1_2Fwd(cdl)
    {
    }

    void CDLRendererNoClampFwd::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        _apply<false>(rgbaBuffer, numPixels);
    }

    CDLRendererV1_2Rev::CDLRendererV1_2Rev(const OpData::OpDataCDLRcPtr & cdl)
        :   CPUCDLOp(cdl)
    {
    }

    void CDLRendererV1_2Rev::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        _apply<true>(rgbaBuffer, numPixels);
    }

    template<bool CLAMP>
    void CDLRendererV1_2Rev::_apply(float * rgbaBuffer, unsigned numPixels) const
    {
#ifdef USE_SSE
        __m128 inScale, outScale, slopeRev, offsetRev, powerRev, saturationRev, pix;
        loadRenderParams(inScale, outScale, slopeRev, offsetRev, powerRev, saturationRev);

        float inAlpha = 1.0f;

        float * rgba = rgbaBuffer;

        for(unsigned idx=0; idx<numPixels; ++idx)
        {
            pix = LoadPixel(rgba, inAlpha);

            ApplyInScale(pix, inScale);

            ApplyClamp<CLAMP>(pix);
            ApplySaturation(pix, saturationRev);

            ApplyPower<CLAMP>(pix, powerRev);

            ApplyOffset(pix, offsetRev);
            ApplySlope(pix, slopeRev);
            ApplyClamp<CLAMP>(pix);

            ApplyOutScale(pix, outScale);
            StorePixel(rgba, pix, inAlpha * m_alphaScale);

            rgba += 4;
        }
#endif
    }

    CDLRendererNoClampRev::CDLRendererNoClampRev(const OpData::OpDataCDLRcPtr & cdl)
        :   CDLRendererV1_2Rev(cdl)
    {
    }

    void CDLRendererNoClampRev::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        _apply<false>(rgbaBuffer, numPixels);
    }

    CPUOpRcPtr CPUCDLOp::GetRenderer(const OpData::OpDataCDLRcPtr & cdl)
    {
        CPUOpRcPtr op(new CPUNoOp);

        switch(cdl->getCDLStyle())
        {
            case OpData::CDL::CDL_V1_2_FWD:
            {
                op.reset(new CDLRendererV1_2Fwd(cdl));
                break;
            }
            case OpData::CDL::CDL_NO_CLAMP_FWD:
            {
                op.reset(new CDLRendererNoClampFwd(cdl));
                break;
            }
            case OpData::CDL::CDL_V1_2_REV:
            {
                op.reset(new CDLRendererV1_2Rev(cdl));
                break;
            }
            case OpData::CDL::CDL_NO_CLAMP_REV:
            {
                op.reset(new CDLRendererNoClampRev(cdl));
                break;
            }

            default:
                throw Exception("Unknown CDL style");
        }

        return op;
    }

}
OCIO_NAMESPACE_EXIT
