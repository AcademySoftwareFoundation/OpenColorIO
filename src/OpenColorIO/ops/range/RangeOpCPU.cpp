// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "ops/matrix/MatrixOpCPU.h"
#include "ops/range/RangeOpCPU.h"

namespace OCIO_NAMESPACE
{

class RangeOpCPU : public OpCPU
{
public:

    RangeOpCPU(ConstRangeOpDataRcPtr & range);

protected:
    float m_scale;
    float m_offset;
    float m_lowerBound;
    float m_upperBound;

private:
    RangeOpCPU() = delete;
};

class RangeScaleMinMaxRenderer : public RangeOpCPU
{
public:
    RangeScaleMinMaxRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class RangeScaleMinRenderer : public RangeOpCPU
{
public:
    RangeScaleMinRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class RangeScaleMaxRenderer : public RangeOpCPU
{
public:
    RangeScaleMaxRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class RangeScaleRenderer : public RangeOpCPU
{
public:
    RangeScaleRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class RangeMinMaxRenderer : public RangeOpCPU
{
public:
    RangeMinMaxRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class RangeMinRenderer : public RangeOpCPU
{
public:
    RangeMinRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class RangeMaxRenderer : public RangeOpCPU
{
public:
    RangeMaxRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(const void * inImg, void * outImg, long numPixels) const override;
};


RangeOpCPU::RangeOpCPU(ConstRangeOpDataRcPtr & range)
    :   OpCPU()
    ,   m_scale(0.0f)
    ,   m_offset(0.0f)
    ,   m_lowerBound(0.0f)
    ,   m_upperBound(0.0f)
{
    m_scale      = (float)range->getScale();
    m_offset     = (float)range->getOffset();
    m_lowerBound = (float)range->getMinOutValue();
    m_upperBound = (float)range->getMaxOutValue();
}

RangeScaleMinMaxRenderer::RangeScaleMinMaxRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeScaleMinMaxRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float t[3] = { in[0] * m_scale + m_offset,
                             in[1] * m_scale + m_offset,
                             in[2] * m_scale + m_offset };

        // NaNs become m_lowerBound.
        out[0] = Clamp(t[0], m_lowerBound, m_upperBound);
        out[1] = Clamp(t[1], m_lowerBound, m_upperBound);
        out[2] = Clamp(t[2], m_lowerBound, m_upperBound);
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

RangeScaleMinRenderer::RangeScaleMinRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeScaleMinRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        out[0] = in[0] * m_scale + m_offset;
        out[1] = in[1] * m_scale + m_offset;
        out[2] = in[2] * m_scale + m_offset;

        // NaNs become m_lowerBound.
        out[0] = std::max(m_lowerBound, out[0]);
        out[1] = std::max(m_lowerBound, out[1]);
        out[2] = std::max(m_lowerBound, out[2]);
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

RangeScaleMaxRenderer::RangeScaleMaxRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeScaleMaxRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        out[0] = in[0] * m_scale + m_offset;
        out[1] = in[1] * m_scale + m_offset;
        out[2] = in[2] * m_scale + m_offset;

        // NaNs become m_upperBound.
        out[0] = std::min(m_upperBound, out[0]),
        out[1] = std::min(m_upperBound, out[1]),
        out[2] = std::min(m_upperBound, out[2]),
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

// NOTE: Currently there is no way to create the Scale renderer.  If a Range Op
// has a min or max defined (which is necessary to have an offset), then it clamps.
// If it doesn't, then it is just a bit depth conversion and is therefore an identity.
// The optimizer currently replaces identities with a scale matrix.
//
RangeScaleRenderer::RangeScaleRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeScaleRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        out[0] = in[0] * m_scale + m_offset;
        out[1] = in[1] * m_scale + m_offset;
        out[2] = in[2] * m_scale + m_offset;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

RangeMinMaxRenderer::RangeMinMaxRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeMinMaxRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        // NaNs become m_lowerBound.
        out[0] = Clamp(in[0], m_lowerBound, m_upperBound);
        out[1] = Clamp(in[1], m_lowerBound, m_upperBound);
        out[2] = Clamp(in[2], m_lowerBound, m_upperBound);
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

RangeMinRenderer::RangeMinRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeMinRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        // NaNs become m_lowerBound.
        out[0] = std::max(m_lowerBound, in[0]);
        out[1] = std::max(m_lowerBound, in[1]);
        out[2] = std::max(m_lowerBound, in[2]);
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

RangeMaxRenderer::RangeMaxRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeMaxRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        // NaNs become m_upperBound.
        out[0] = std::min(m_upperBound, in[0]);
        out[1] = std::min(m_upperBound, in[1]);
        out[2] = std::min(m_upperBound, in[2]);
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}


ConstOpCPURcPtr GetRangeRenderer(ConstRangeOpDataRcPtr & range)
{
    if (range->scales())
    {
        if (!range->minIsEmpty())
        {
            if (!range->maxIsEmpty())
            {
                return std::make_shared<RangeScaleMinMaxRenderer>(range);
            }
            else
            {
                return std::make_shared<RangeScaleMinRenderer>(range);
            }
        }
        else
        {
            if (!range->maxIsEmpty())
            {
                return std::make_shared<RangeScaleMaxRenderer>(range);
            }
            else
            {
                // (Currently we will not get here, see comment above.)
                return std::make_shared<RangeScaleRenderer>(range);
            }
        }
    }
    else  // implies m_scale = 1, m_offset = 0
    {
        if (!range->minIsEmpty())
        {
            if (!range->maxIsEmpty())
            {
                return std::make_shared<RangeMinMaxRenderer>(range);
            }
            else
            {
                return std::make_shared<RangeMinRenderer>(range);
            }
        }
        else if (!range->maxIsEmpty())
        {
            return std::make_shared<RangeMaxRenderer>(range);
        }

        // Else, no rendering/scaling is needed.
    }

    // Add identity matrix renderer.
    auto mat = std::make_shared<const MatrixOpData>();
    return GetMatrixRenderer(mat);
}

} // namespace OCIO_NAMESPACE

