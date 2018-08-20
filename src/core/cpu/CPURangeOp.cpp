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


#include "CPURangeOp.h"

#include "../MathUtils.h"
#include <algorithm>

OCIO_NAMESPACE_ENTER
{

    CPURangeOp::CPURangeOp(const OpData::OpDataRangeRcPtr & range)
        :   CPUOp()
        ,   m_scale(0.0f)
        ,   m_offset(0.0f)
        ,   m_lowerBound(0.0f)
        ,   m_upperBound(0.0f)
        ,   m_alphaScale(0.0f)
    {
        m_scale      = range->getScale();
        m_offset     = range->getOffset();
        m_lowerBound = range->getLowBound();
        m_upperBound = range->getHighBound();
        m_alphaScale = range->getAlphaScale();
    }

    RangeScaleMinMaxRenderer::RangeScaleMinMaxRenderer(const OpData::OpDataRangeRcPtr & range)
        :  CPURangeOp(range)
    {
    }

    void RangeScaleMinMaxRenderer::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        float * rgba = rgbaBuffer;

        for(unsigned idx=0; idx<numPixels; ++idx)
        {
            const float t[3] = { rgba[0] * m_scale + m_offset,
                                 rgba[1] * m_scale + m_offset,
                                 rgba[2] * m_scale + m_offset };

            rgba[0] = clamp(t[0], m_lowerBound, m_upperBound);
            rgba[1] = clamp(t[1], m_lowerBound, m_upperBound);
            rgba[2] = clamp(t[2], m_lowerBound, m_upperBound);
            rgba[3] = rgba[3] * m_alphaScale;

            rgba += 4;
        }
    }

    RangeScaleMinRenderer::RangeScaleMinRenderer(const OpData::OpDataRangeRcPtr & range)
        :  CPURangeOp(range)
    {
    }

    void RangeScaleMinRenderer::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        float * rgba = rgbaBuffer;

        for(unsigned idx=0; idx<numPixels; ++idx)
        {
            rgba[0] = std::max(m_lowerBound, rgba[0] * m_scale + m_offset);
            rgba[1] = std::max(m_lowerBound, rgba[1] * m_scale + m_offset);
            rgba[2] = std::max(m_lowerBound, rgba[2] * m_scale + m_offset);
            rgba[3] = rgba[3] * m_alphaScale;

            rgba += 4;
        }
    }

    RangeScaleMaxRenderer::RangeScaleMaxRenderer(const OpData::OpDataRangeRcPtr & range)
        :  CPURangeOp(range)
    {
    }

    void RangeScaleMaxRenderer::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        float * rgba = rgbaBuffer;

        for(unsigned idx=0; idx<numPixels; ++idx)
        {
            rgba[0] = std::min(rgba[0] * m_scale + m_offset, m_upperBound),
            rgba[1] = std::min(rgba[1] * m_scale + m_offset, m_upperBound),
            rgba[2] = std::min(rgba[2] * m_scale + m_offset, m_upperBound),
            rgba[3] = rgba[3] * m_alphaScale;

            rgba += 4;
        }
    }

    // NOTE: Currently there is no way to create the Scale renderer.  If a Range Op
    // has a min or max defined (which is necessary to have an offset), then it clamps.  
    // If it doesn't, then it is just a bit depth conversion and is therefore an identity.
    // The optimizer currently replaces identities with a scale matrix.
    //
    // TODO: Now that CLF allows non-clamping Ranges, could avoid turning
    // these ranges into matrices in the XML reader?
    //
    RangeScaleRenderer::RangeScaleRenderer(const OpData::OpDataRangeRcPtr & range)
        :  CPURangeOp(range)
    {
    }

    void RangeScaleRenderer::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        float * rgba = rgbaBuffer;

        for(unsigned idx=0; idx<numPixels; ++idx)
        {
            rgba[0] = rgba[0] * m_scale + m_offset;
            rgba[1] = rgba[1] * m_scale + m_offset;
            rgba[2] = rgba[2] * m_scale + m_offset;
            rgba[3] = rgba[3] * m_alphaScale;

            rgba += 4;
        }
    }

    RangeMinMaxRenderer::RangeMinMaxRenderer(const OpData::OpDataRangeRcPtr & range)
        :  CPURangeOp(range)
    {
    }

    void RangeMinMaxRenderer::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        float * rgba = rgbaBuffer;

        for(unsigned idx=0; idx<numPixels; ++idx)
        {
            rgba[0] = clamp(rgba[0], m_lowerBound, m_upperBound);
            rgba[1] = clamp(rgba[1], m_lowerBound, m_upperBound);
            rgba[2] = clamp(rgba[2], m_lowerBound, m_upperBound);

            rgba += 4;
        }
    }

    RangeMinRenderer::RangeMinRenderer(const OpData::OpDataRangeRcPtr & range)
        :  CPURangeOp(range)
    {
    }

    void RangeMinRenderer::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        float * rgba = rgbaBuffer;

        for(unsigned idx=0; idx<numPixels; ++idx)
        {
            // Note: Although _scale is not applied in this renderer, it is ok.
            // The dispatcher will only call this renderer if _scale == 1, so this
            // renderer would not be called if there is a bit-depth conversion.
            // Likewise, _alphaScale = 1, so no need to scale alpha.

            rgba[0] = std::max(m_lowerBound, rgba[0]);
            rgba[1] = std::max(m_lowerBound, rgba[1]);
            rgba[2] = std::max(m_lowerBound, rgba[2]);

            rgba += 4;
        }
    }

    RangeMaxRenderer::RangeMaxRenderer(const OpData::OpDataRangeRcPtr & range)
        :  CPURangeOp(range)
    {
    }

    void RangeMaxRenderer::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        float * rgba = rgbaBuffer;

        for(unsigned idx=0; idx<numPixels; ++idx)
        {
            rgba[0] = std::min(rgba[0], m_upperBound);
            rgba[1] = std::min(rgba[1], m_upperBound);
            rgba[2] = std::min(rgba[2], m_upperBound);

            rgba += 4;
        }
    }


    CPUOpRcPtr CPURangeOp::GetRenderer(const OpData::OpDataRangeRcPtr & range)
    {
        CPUOpRcPtr op(new CPUNoOp);

        if (range->scales(false))
        {
            if (range->minClips())
            {
                if (range->maxClips())
                {
                    op.reset(new RangeScaleMinMaxRenderer(range));
                }
                else
                {
                    op.reset(new RangeScaleMinRenderer(range));
                }
            }
            else
            {
                if (range->maxClips())
                {
                    op.reset(new RangeScaleMaxRenderer(range));
                }
                else
                {
                    // (Currently we will not get here, see comment above.)
                    op.reset(new RangeScaleRenderer(range));
                }
            }
        }
        else  // implies _scale = 1, _alphaScale = 1, _offset = 0
        {
            if (range->minClips())
            {
                if (range->maxClips())
                {
                    op.reset(new RangeMinMaxRenderer(range));
                }
                else
                {
                    op.reset(new RangeMinRenderer(range));
                }
            }
            else if (range->maxClips())
            {
                op.reset(new RangeMaxRenderer(range));
            }

            // Else, no rendering/scaling is needed and we return a null renderer.
        }

        return op;
    }
}
OCIO_NAMESPACE_EXIT
