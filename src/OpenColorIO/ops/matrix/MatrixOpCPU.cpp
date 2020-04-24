// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/matrix/MatrixOpCPU.h"
#include "Platform.h"
#include "SSE.h"

namespace OCIO_NAMESPACE
{
namespace
{

class ScaleRenderer : public OpCPU
{
public:
    ScaleRenderer() = delete;
    ScaleRenderer(const ScaleRenderer &) = delete;
    explicit ScaleRenderer(ConstMatrixOpDataRcPtr & mat);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

private:
    float m_scale[4];
};

class ScaleWithOffsetRenderer : public OpCPU
{
public:
    ScaleWithOffsetRenderer() = delete;
    ScaleWithOffsetRenderer(const ScaleRenderer &) = delete;
    explicit ScaleWithOffsetRenderer(ConstMatrixOpDataRcPtr & mat);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

private:
    float m_scale[4];
    float m_offset[4];
};

class MatrixWithOffsetRenderer : public OpCPU
{
public:
    MatrixWithOffsetRenderer() = delete;
    MatrixWithOffsetRenderer(const MatrixWithOffsetRenderer &) = delete;
    explicit MatrixWithOffsetRenderer(ConstMatrixOpDataRcPtr & mat);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

private:

    float m_column1[4];
    float m_column2[4];
    float m_column3[4];
    float m_column4[4];

    float m_offset[4];
};

class MatrixRenderer : public OpCPU
{
public:
    MatrixRenderer() = delete;
    MatrixRenderer(const MatrixRenderer &) = delete;
    MatrixRenderer(ConstMatrixOpDataRcPtr & mat);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

private:
    float m_column1[4];
    float m_column2[4];
    float m_column3[4];
    float m_column4[4];
};

ScaleRenderer::ScaleRenderer(ConstMatrixOpDataRcPtr & mat)
    : OpCPU()
{
    const ArrayDouble::Values & m = mat->getArray().getValues();

    m_scale[0] = (float)m[0];
    m_scale[1] = (float)m[5];
    m_scale[2] = (float)m[10];
    m_scale[3] = (float)m[15];
}

void ScaleRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        out[0] = in[0] * m_scale[0];
        out[1] = in[1] * m_scale[1];
        out[2] = in[2] * m_scale[2];
        out[3] = in[3] * m_scale[3];

        in  += 4;
        out += 4;
    }
}

ScaleWithOffsetRenderer::ScaleWithOffsetRenderer(ConstMatrixOpDataRcPtr & mat)
    : OpCPU()
{
    const ArrayDouble::Values & m = mat->getArray().getValues();

    m_scale[0] = (float)m[0];
    m_scale[1] = (float)m[5];
    m_scale[2] = (float)m[10];
    m_scale[3] = (float)m[15];

    const MatrixOpData::Offsets& o = mat->getOffsets();

    m_offset[0] = (float)o[0];
    m_offset[1] = (float)o[1];
    m_offset[2] = (float)o[2];
    m_offset[3] = (float)o[3];
}

void ScaleWithOffsetRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        out[0] = in[0] * m_scale[0] + m_offset[0];
        out[1] = in[1] * m_scale[1] + m_offset[1];
        out[2] = in[2] * m_scale[2] + m_offset[2];
        out[3] = in[3] * m_scale[3] + m_offset[3];

        in  += 4;
        out += 4;
    }
}

MatrixWithOffsetRenderer::MatrixWithOffsetRenderer(ConstMatrixOpDataRcPtr & mat)
    : OpCPU()
{
    const unsigned long dim = mat->getArray().getLength();
    const unsigned long twoDim = 2 * dim;
    const unsigned long threeDim = 3 * dim;
    const ArrayDouble::Values & m = mat->getArray().getValues();

    // Red multipliers.
    m_column1[0] = (float)m[0];
    m_column1[1] = (float)m[dim];
    m_column1[2] = (float)m[twoDim];
    m_column1[3] = (float)m[threeDim];

    // Green multipliers.
    m_column2[0] = (float)m[1];
    m_column2[1] = (float)m[dim + 1];
    m_column2[2] = (float)m[twoDim + 1];
    m_column2[3] = (float)m[threeDim + 1];

    // Blue multipliers.
    m_column3[0] = (float)m[2];
    m_column3[1] = (float)m[dim + 2];
    m_column3[2] = (float)m[twoDim + 2];
    m_column3[3] = (float)m[threeDim + 2];

    // Alpha multipliers.
    m_column4[0] = (float)m[3];
    m_column4[1] = (float)m[dim + 3];
    m_column4[2] = (float)m[twoDim + 3];
    m_column4[3] = (float)m[threeDim + 3];

    const MatrixOpData::Offsets & o = mat->getOffsets();

    m_offset[0] = (float)o[0];
    m_offset[1] = (float)o[1];
    m_offset[2] = (float)o[2];
    m_offset[3] = (float)o[3];

}

// Apply the rendering
//
// for (unsigned idx = 0; idx<numPixels; ++idx)
// {
//     const float r = rgbaBuffer[0];
//     const float g = rgbaBuffer[1];
//     const float b = rgbaBuffer[2];
//     const float a = rgbaBuffer[3];
//
//     rgbaBuffer[0] = r*m[0] + g*m[1] + b*m[2] + a*m[3];
//     rgbaBuffer[1] = r*m[4] + g*m[5] + b*m[6] + a*m[7];
//     rgbaBuffer[2] = r*m[8] + g*m[9] + b*m[10] + a*m[11];
//     rgbaBuffer[3] = r*m[12] + g*m[13] + b*m[14] + a*m[15];
// }
//
// To better understand the SSE implementation of this algorithm,
// you have to notice that:
// 1) you have four multiplications:
//      rm0 = r*m[]
//      gm1 = g*m[]
//      bm2 = b*m[]
//      am3 = a*m[]
// 2) you have three additions:
//      res1 = rm0 + gm1
//      res2 = bm2 + am3
//      image = res1 + res2
void MatrixWithOffsetRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE
    // Matrix decomposition per _column.
    __m128 m0 = _mm_set_ps(m_column1[3],
                           m_column1[2],
                           m_column1[1],
                           m_column1[0]);
    __m128 m1 = _mm_set_ps(m_column2[3],
                           m_column2[2],
                           m_column2[1],
                           m_column2[0]);
    __m128 m2 = _mm_set_ps(m_column3[3],
                           m_column3[2],
                           m_column3[1],
                           m_column3[0]);
    __m128 m3 = _mm_set_ps(m_column4[3],
                           m_column4[2],
                           m_column4[1],
                           m_column4[0]);
    __m128 o = _mm_set_ps(m_offset[3], m_offset[2], m_offset[1], m_offset[0]);

    for (long idx = 0; idx < numPixels; ++idx)
    {
        __m128 r = _mm_set1_ps(in[0]);
        __m128 g = _mm_set1_ps(in[1]);
        __m128 b = _mm_set1_ps(in[2]);
        __m128 a = _mm_set1_ps(in[3]);

        __m128 rm0 = _mm_mul_ps(m0, r);
        __m128 gm1 = _mm_mul_ps(m1, g);
        __m128 bm2 = _mm_mul_ps(m2, b);
        __m128 am3 = _mm_mul_ps(m3, a);

        __m128 img = _mm_add_ps(_mm_add_ps(rm0, gm1), _mm_add_ps(bm2, am3));
        img = _mm_add_ps(img, o);

        _mm_storeu_ps(out, img);

        in  += 4;
        out += 4;
    }
#else
    for (long idx = 0; idx < numPixels; ++idx)
    {
        const float r = in[0];
        const float g = in[1];
        const float b = in[2];
        const float a = in[3];

        out[0] = r*m_column1[0]
                + g*m_column2[0]
                + b*m_column3[0]
                + a*m_column4[0]
                + m_offset[0];
        out[1] = r*m_column1[1]
                + g*m_column2[1]
                + b*m_column3[1]
                + a*m_column4[1]
                + m_offset[1];
        out[2] = r*m_column1[2]
                + g*m_column2[2]
                + b*m_column3[2]
                + a*m_column4[2]
                + m_offset[2];
        out[3] = r*m_column1[3]
                + g*m_column2[3]
                + b*m_column3[3]
                + a*m_column4[3]
                + m_offset[3];

        in  += 4;
        out += 4;
    }
#endif

}

MatrixRenderer::MatrixRenderer(ConstMatrixOpDataRcPtr & mat)
    : OpCPU()
{
    const unsigned long dim = mat->getArray().getLength();
    const unsigned long twoDim = 2 * dim;
    const unsigned long threeDim = 3 * dim;
    const ArrayDouble::Values & m = mat->getArray().getValues();

    // Red multipliers.
    m_column1[0] = (float)m[0];
    m_column1[1] = (float)m[dim];
    m_column1[2] = (float)m[twoDim];
    m_column1[3] = (float)m[threeDim];

    // Green multipliers.
    m_column2[0] = (float)m[1];
    m_column2[1] = (float)m[dim + 1];
    m_column2[2] = (float)m[twoDim + 1];
    m_column2[3] = (float)m[threeDim + 1];

    // Blue multipliers.
    m_column3[0] = (float)m[2];
    m_column3[1] = (float)m[dim + 2];
    m_column3[2] = (float)m[twoDim + 2];
    m_column3[3] = (float)m[threeDim + 2];

    // Alpha multipliers.
    m_column4[0] = (float)m[3];
    m_column4[1] = (float)m[dim + 3];
    m_column4[2] = (float)m[twoDim + 3];
    m_column4[3] = (float)m[threeDim + 3];
}

void MatrixRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE
    // Matrix decomposition per _column.
    __m128 m0 = _mm_set_ps(m_column1[3],
                           m_column1[2],
                           m_column1[1],
                           m_column1[0]);
    __m128 m1 = _mm_set_ps(m_column2[3],
                           m_column2[2],
                           m_column2[1],
                           m_column2[0]);
    __m128 m2 = _mm_set_ps(m_column3[3],
                           m_column3[2],
                           m_column3[1],
                           m_column3[0]);
    __m128 m3 = _mm_set_ps(m_column4[3],
                           m_column4[2],
                           m_column4[1],
                           m_column4[0]);

    for (long idx = 0; idx < numPixels; ++idx)
    {
        __m128 r = _mm_set1_ps(in[0]);
        __m128 g = _mm_set1_ps(in[1]);
        __m128 b = _mm_set1_ps(in[2]);
        __m128 a = _mm_set1_ps(in[3]);

        __m128 rm0 = _mm_mul_ps(m0, r);
        __m128 gm1 = _mm_mul_ps(m1, g);
        __m128 bm2 = _mm_mul_ps(m2, b);
        __m128 am3 = _mm_mul_ps(m3, a);

        __m128 img = _mm_add_ps(_mm_add_ps(rm0, gm1), _mm_add_ps(bm2, am3));

        _mm_storeu_ps(out, img);

        in  += 4;
        out += 4;
    }
#else
    for (long idx = 0; idx < numPixels; ++idx)
    {
        const float r = in[0];
        const float g = in[1];
        const float b = in[2];
        const float a = in[3];

        out[0] = r*m_column1[0]
               + g*m_column2[0]
               + b*m_column3[0]
               + a*m_column4[0];
        out[1] = r*m_column1[1]
               + g*m_column2[1]
               + b*m_column3[1]
               + a*m_column4[1];
        out[2] = r*m_column1[2]
               + g*m_column2[2]
               + b*m_column3[2]
               + a*m_column4[2];
        out[3] = r*m_column1[3]
               + g*m_column2[3]
               + b*m_column3[3]
               + a*m_column4[3];

        in  += 4;
        out += 4;
    }

#endif
}

}

ConstOpCPURcPtr GetMatrixRenderer(ConstMatrixOpDataRcPtr & mat)
{
    if (mat->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        throw Exception("Op::finalize has to be called.");
    }
    if (mat->isDiagonal())
    {
        if (mat->hasOffsets())
        {
            return std::make_shared<ScaleWithOffsetRenderer>(mat);
        }
        else
        {
            return std::make_shared<ScaleRenderer>(mat);
        }
    }
    else
    {
        if (mat->hasOffsets())
        {
            return std::make_shared<MatrixWithOffsetRenderer>(mat);
        }
        else
        {
            return std::make_shared<MatrixRenderer>(mat);
        }
    }
}

} // namespace OCIO_NAMESPACE

