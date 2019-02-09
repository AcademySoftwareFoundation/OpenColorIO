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

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "ops/Range/RangeOpCPU.h"


OCIO_NAMESPACE_ENTER
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
    float m_alphaScale;

private:
    RangeOpCPU() = delete;
};

class RangeScaleMinMaxRenderer : public RangeOpCPU
{
public:
    RangeScaleMinMaxRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const override;
};

class RangeScaleMinRenderer : public RangeOpCPU
{
public:
    RangeScaleMinRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const override;
};

class RangeScaleMaxRenderer : public RangeOpCPU
{
public:
    RangeScaleMaxRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const override;
};

class RangeScaleRenderer : public RangeOpCPU
{
public:
    RangeScaleRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const override;
};

class RangeMinMaxRenderer : public RangeOpCPU
{
public:
    RangeMinMaxRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const override;
};

class RangeMinRenderer : public RangeOpCPU
{
public:
    RangeMinRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const override;
};

class RangeMaxRenderer : public RangeOpCPU
{
public:
    RangeMaxRenderer(ConstRangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const override;
};


RangeOpCPU::RangeOpCPU(ConstRangeOpDataRcPtr & range)
    :   OpCPU()
    ,   m_scale(0.0f)
    ,   m_offset(0.0f)
    ,   m_lowerBound(0.0f)
    ,   m_upperBound(0.0f)
    ,   m_alphaScale(0.0f)
{
    m_scale      = (float)range->getScale();
    m_offset     = (float)range->getOffset();
    m_lowerBound = (float)range->getLowBound();
    m_upperBound = (float)range->getHighBound();
    m_alphaScale = (float)range->getAlphaScale();
}

RangeScaleMinMaxRenderer::RangeScaleMinMaxRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeScaleMinMaxRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    float * rgba = rgbaBuffer;

    for(long idx=0; idx<numPixels; ++idx)
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

RangeScaleMinRenderer::RangeScaleMinRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeScaleMinRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    float * rgba = rgbaBuffer;

    for(long idx=0; idx<numPixels; ++idx)
    {
        rgba[0] = std::max(m_lowerBound, rgba[0] * m_scale + m_offset);
        rgba[1] = std::max(m_lowerBound, rgba[1] * m_scale + m_offset);
        rgba[2] = std::max(m_lowerBound, rgba[2] * m_scale + m_offset);
        rgba[3] = rgba[3] * m_alphaScale;

        rgba += 4;
    }
}

RangeScaleMaxRenderer::RangeScaleMaxRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeScaleMaxRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    float * rgba = rgbaBuffer;

    for(long idx=0; idx<numPixels; ++idx)
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
RangeScaleRenderer::RangeScaleRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeScaleRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    float * rgba = rgbaBuffer;

    for(long idx=0; idx<numPixels; ++idx)
    {
        rgba[0] = rgba[0] * m_scale + m_offset;
        rgba[1] = rgba[1] * m_scale + m_offset;
        rgba[2] = rgba[2] * m_scale + m_offset;
        rgba[3] = rgba[3] * m_alphaScale;

        rgba += 4;
    }
}

RangeMinMaxRenderer::RangeMinMaxRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeMinMaxRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    float * rgba = rgbaBuffer;

    for(long idx=0; idx<numPixels; ++idx)
    {
        rgba[0] = clamp(rgba[0], m_lowerBound, m_upperBound);
        rgba[1] = clamp(rgba[1], m_lowerBound, m_upperBound);
        rgba[2] = clamp(rgba[2], m_lowerBound, m_upperBound);

        rgba += 4;
    }
}

RangeMinRenderer::RangeMinRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeMinRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    float * rgba = rgbaBuffer;

    for(long idx=0; idx<numPixels; ++idx)
    {
        // Note: Although m_scale is not applied in this renderer, it is ok.
        // The dispatcher will only call this renderer if m_scale == 1, so this
        // renderer would not be called if there is a bit-depth conversion.
        // Likewise, m_alphaScale = 1, so no need to scale alpha.

        rgba[0] = std::max(m_lowerBound, rgba[0]);
        rgba[1] = std::max(m_lowerBound, rgba[1]);
        rgba[2] = std::max(m_lowerBound, rgba[2]);

        rgba += 4;
    }
}

RangeMaxRenderer::RangeMaxRenderer(ConstRangeOpDataRcPtr & range)
    :  RangeOpCPU(range)
{
}

void RangeMaxRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    float * rgba = rgbaBuffer;

    for(long idx=0; idx<numPixels; ++idx)
    {
        rgba[0] = std::min(rgba[0], m_upperBound);
        rgba[1] = std::min(rgba[1], m_upperBound);
        rgba[2] = std::min(rgba[2], m_upperBound);

        rgba += 4;
    }
}


OpCPURcPtr GetRangeRenderer(ConstRangeOpDataRcPtr & range)
{
    OpCPURcPtr op(new NoOpCPU);

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





#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "ops/Range/RangeOpData.h"
#include "pystring/pystring.h"
#include "unittest.h"


static const float g_error = 1e-7f;


OIIO_ADD_TEST(RangeOpCPU, identity)
{
    OCIO::RangeOpDataRcPtr range = std::make_shared<OCIO::RangeOpData>();
    OIIO_CHECK_NO_THROW(range->validate());
    OIIO_CHECK_NO_THROW(range->finalize());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::OpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OIIO_CHECK_NE(-1, OCIO::pystring::find(typeName, "NoOpCPU"));

    const long numPixels = 3;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(op->apply(&image[0], numPixels));

    OIIO_CHECK_CLOSE(image[0],  -0.50f, g_error);
    OIIO_CHECK_CLOSE(image[1],  -0.25f, g_error);
    OIIO_CHECK_CLOSE(image[2],   0.50f, g_error);
    OIIO_CHECK_CLOSE(image[3],   0.00f, g_error);

    OIIO_CHECK_CLOSE(image[4],   0.75f, g_error);
    OIIO_CHECK_CLOSE(image[5],   1.00f, g_error);
    OIIO_CHECK_CLOSE(image[6],   1.25f, g_error);
    OIIO_CHECK_CLOSE(image[7],   1.00f, g_error);

    OIIO_CHECK_CLOSE(image[8],   1.25f, g_error);
    OIIO_CHECK_CLOSE(image[9],   1.50f, g_error);
    OIIO_CHECK_CLOSE(image[10],  1.75f, g_error);
    OIIO_CHECK_CLOSE(image[11],  0.00f, g_error);
}

OIIO_ADD_TEST(RangeOpCPU, scale_with_low_and_high_clippings)
{
    OCIO::RangeOpDataRcPtr range 
        = std::make_shared<OCIO::RangeOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 
                                              0., 1., 0.5, 1.5);

    OIIO_CHECK_NO_THROW(range->validate());
    OIIO_CHECK_NO_THROW(range->finalize());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::OpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OIIO_CHECK_NE(-1, OCIO::pystring::find(typeName, "RangeScaleMinMaxRenderer"));

    const long numPixels = 3;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(op->apply(&image[0], numPixels));

    OIIO_CHECK_CLOSE(image[0],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[1],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[2],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.00f, g_error);

    OIIO_CHECK_CLOSE(image[4],  1.25f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.00f, g_error);

    OIIO_CHECK_CLOSE(image[8],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[9],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[10], 1.50f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OIIO_ADD_TEST(RangeOpCPU, scale_with_low_clipping)
{
    OCIO::RangeOpDataRcPtr range 
        = std::make_shared<OCIO::RangeOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 
                                              0.,  OCIO::RangeOpData::EmptyValue(), 
                                              0.5, OCIO::RangeOpData::EmptyValue());

    OIIO_CHECK_NO_THROW(range->validate());
    OIIO_CHECK_NO_THROW(range->finalize());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::OpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OIIO_CHECK_NE(-1, OCIO::pystring::find(typeName, "RangeScaleMinRenderer"));

    const long numPixels = 3;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(op->apply(&image[0], numPixels));

    OIIO_CHECK_CLOSE(image[0],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[1],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[2],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.00f, g_error);

    OIIO_CHECK_CLOSE(image[4],  1.25f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.75f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.00f, g_error);

    OIIO_CHECK_CLOSE(image[8],  1.75f, g_error);
    OIIO_CHECK_CLOSE(image[9],  2.00f, g_error);
    OIIO_CHECK_CLOSE(image[10], 2.25f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OIIO_ADD_TEST(RangeOpCPU, scale_with_high_clipping)
{
    OCIO::RangeOpDataRcPtr range 
        = std::make_shared<OCIO::RangeOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 
                                              OCIO::RangeOpData::EmptyValue(), 1., 
                                              OCIO::RangeOpData::EmptyValue(), 1.5);

    OIIO_CHECK_NO_THROW(range->validate());
    OIIO_CHECK_NO_THROW(range->finalize());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::OpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OIIO_CHECK_NE(-1, OCIO::pystring::find(typeName, "RangeScaleMaxRenderer"));

    const long numPixels = 3;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(op->apply(&image[0], numPixels));

    OIIO_CHECK_CLOSE(image[0],  0.00f, g_error);
    OIIO_CHECK_CLOSE(image[1],  0.25f, g_error);
    OIIO_CHECK_CLOSE(image[2],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.00f, g_error);

    OIIO_CHECK_CLOSE(image[4],  1.25f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.00f, g_error);

    OIIO_CHECK_CLOSE(image[8],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[9],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[10], 1.50f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OIIO_ADD_TEST(RangeOpCPU, scale_with_low_and_high_clippings_2)
{
    OCIO::RangeOpDataRcPtr range 
        = std::make_shared<OCIO::RangeOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 
                                              0., 1., 0., 1.5);

    OIIO_CHECK_NO_THROW(range->validate());
    OIIO_CHECK_NO_THROW(range->finalize());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::OpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OIIO_CHECK_NE(-1, OCIO::pystring::find(typeName, "RangeScaleMinMaxRenderer"));

    const long numPixels = 3;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(op->apply(&image[0], numPixels));

    OIIO_CHECK_CLOSE(image[0],  0.000f, g_error);
    OIIO_CHECK_CLOSE(image[1],  0.000f, g_error);
    OIIO_CHECK_CLOSE(image[2],  0.750f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.000f, g_error);

    OIIO_CHECK_CLOSE(image[4],  1.125f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.500f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.500f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.000f, g_error);

    OIIO_CHECK_CLOSE(image[8],  1.500f, g_error);
    OIIO_CHECK_CLOSE(image[9],  1.500f, g_error);
    OIIO_CHECK_CLOSE(image[10], 1.500f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.000f, g_error);
}

OIIO_ADD_TEST(RangeOpCPU, offset_with_low_and_high_clippings)
{
    OCIO::RangeOpDataRcPtr range 
        = std::make_shared<OCIO::RangeOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 
                                              0., 1., 1., 2.);

    OIIO_CHECK_NO_THROW(range->validate());
    OIIO_CHECK_NO_THROW(range->finalize());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::OpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OIIO_CHECK_NE(-1, OCIO::pystring::find(typeName, "RangeScaleMinMaxRenderer"));

    const long numPixels = 3;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(op->apply(&image[0], numPixels));

    OIIO_CHECK_CLOSE(image[0],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[1],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[2],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.00f, g_error);

    OIIO_CHECK_CLOSE(image[4],  1.75f, g_error);
    OIIO_CHECK_CLOSE(image[5],  2.00f, g_error);
    OIIO_CHECK_CLOSE(image[6],  2.00f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.00f, g_error);

    OIIO_CHECK_CLOSE(image[8],  2.00f, g_error);
    OIIO_CHECK_CLOSE(image[9],  2.00f, g_error);
    OIIO_CHECK_CLOSE(image[10], 2.00f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OIIO_ADD_TEST(RangeOpCPU, low_and_high_clippings)
{
    OCIO::RangeOpDataRcPtr range 
        = std::make_shared<OCIO::RangeOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 
                                              1., 2., 1., 2.);

    OIIO_CHECK_NO_THROW(range->validate());
    OIIO_CHECK_NO_THROW(range->finalize());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::OpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OIIO_CHECK_NE(-1, OCIO::pystring::find(typeName, "RangeMinMaxRenderer"));

    const long numPixels = 4;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f,
                                  2.00f,  2.50f, 2.75f, 1.0f };

    OIIO_CHECK_NO_THROW(op->apply(&image[0], numPixels));

    OIIO_CHECK_CLOSE(image[0],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[1],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[2],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.00f, g_error);

    OIIO_CHECK_CLOSE(image[4],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.25f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.00f, g_error);

    OIIO_CHECK_CLOSE(image[8],  1.25f, g_error);
    OIIO_CHECK_CLOSE(image[9],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[10], 1.75f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.00f, g_error);

    OIIO_CHECK_CLOSE(image[12], 2.00f, g_error);
    OIIO_CHECK_CLOSE(image[13], 2.00f, g_error);
    OIIO_CHECK_CLOSE(image[14], 2.00f, g_error);
    OIIO_CHECK_CLOSE(image[15], 1.00f, g_error);
}

OIIO_ADD_TEST(RangeOpCPU, low_clipping)
{
    OCIO::RangeOpDataRcPtr range 
        = std::make_shared<OCIO::RangeOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 
                                              -0.1, OCIO::RangeOpData::EmptyValue(), 
                                              -0.1, OCIO::RangeOpData::EmptyValue());

    OIIO_CHECK_NO_THROW(range->validate());
    OIIO_CHECK_NO_THROW(range->finalize());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::OpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OIIO_CHECK_NE(-1, OCIO::pystring::find(typeName, "RangeMinRenderer"));

    const long numPixels = 3;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(op->apply(&image[0], numPixels));

    OIIO_CHECK_CLOSE(image[0], -0.10f, g_error);
    OIIO_CHECK_CLOSE(image[1], -0.10f, g_error);
    OIIO_CHECK_CLOSE(image[2],  0.50f, g_error);
    OIIO_CHECK_CLOSE(image[3],  0.00f, g_error);

    OIIO_CHECK_CLOSE(image[4],  0.75f, g_error);
    OIIO_CHECK_CLOSE(image[5],  1.00f, g_error);
    OIIO_CHECK_CLOSE(image[6],  1.25f, g_error);
    OIIO_CHECK_CLOSE(image[7],  1.00f, g_error);

    OIIO_CHECK_CLOSE(image[8],  1.25f, g_error);
    OIIO_CHECK_CLOSE(image[9],  1.50f, g_error);
    OIIO_CHECK_CLOSE(image[10], 1.75f, g_error);
    OIIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OIIO_ADD_TEST(RangeOpCPU, high_clipping)
{
    OCIO::RangeOpDataRcPtr range 
        = std::make_shared<OCIO::RangeOpData>(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 
                                              OCIO::RangeOpData::EmptyValue(), 1.1, 
                                              OCIO::RangeOpData::EmptyValue(), 1.1);

    OIIO_CHECK_NO_THROW(range->validate());
    OIIO_CHECK_NO_THROW(range->finalize());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::OpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OIIO_CHECK_NE(-1, OCIO::pystring::find(typeName, "RangeMaxRenderer"));

    const long numPixels = 3;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f };

    OIIO_CHECK_NO_THROW(op->apply(&image[0], numPixels));

    OIIO_CHECK_CLOSE(image[0],  -0.50f, g_error);
    OIIO_CHECK_CLOSE(image[1],  -0.25f, g_error);
    OIIO_CHECK_CLOSE(image[2],   0.50f, g_error);
    OIIO_CHECK_CLOSE(image[3],   0.00f, g_error);

    OIIO_CHECK_CLOSE(image[4],   0.75f, g_error);
    OIIO_CHECK_CLOSE(image[5],   1.00f, g_error);
    OIIO_CHECK_CLOSE(image[6],   1.10f, g_error);
    OIIO_CHECK_CLOSE(image[7],   1.00f, g_error);

    OIIO_CHECK_CLOSE(image[8],   1.10f, g_error);
    OIIO_CHECK_CLOSE(image[9],   1.10f, g_error);
    OIIO_CHECK_CLOSE(image[10],  1.10f, g_error);
    OIIO_CHECK_CLOSE(image[11],  0.00f, g_error);
}



#endif
