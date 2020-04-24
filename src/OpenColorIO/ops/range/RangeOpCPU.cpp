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
    ConstRangeOpDataRcPtr rangeFwd = range;
    if (range->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        throw Exception("Op::finalize has to be called.");
    }
    // Both min & max can not be empty at the same time.
    if (rangeFwd->minIsEmpty())
    {
        return std::make_shared<RangeMaxRenderer>(rangeFwd);
    }
    else if (rangeFwd->maxIsEmpty())
    {
        return std::make_shared<RangeMinRenderer>(rangeFwd);
    }

    // Both min and max have values.
    if (!rangeFwd->scales())
    {
        return std::make_shared<RangeMinMaxRenderer>(rangeFwd);
    }
    return std::make_shared<RangeScaleMinMaxRenderer>(rangeFwd);
}

} // namespace OCIO_NAMESPACE

