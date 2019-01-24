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

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/Matrix/MatrixOpCPU.h"
#include "Platform.h"
#include "SSE.h"

OCIO_NAMESPACE_ENTER
{
namespace
{

class ScaleRenderer : public OpCPU
{
public:
    ScaleRenderer(const MatrixOpDataRcPtr & mat);

    void apply(float * rgbaBuffer, long numPixels) const override;

private:
    ScaleRenderer() = delete;
    float m_scale[4];
};

class ScaleWithOffsetRenderer : public OpCPU
{
public:
    ScaleWithOffsetRenderer(const MatrixOpDataRcPtr & mat);

    void apply(float * rgbaBuffer, long numPixels) const override;

private:
    ScaleWithOffsetRenderer() = delete;
    float m_scale[4];
    float m_offset[4];
};

class MatrixWithOffsetRenderer : public OpCPU
{
public:
    MatrixWithOffsetRenderer(const MatrixOpDataRcPtr & mat);

    void apply(float * rgbaBuffer, long numPixels) const override;

private:
    MatrixWithOffsetRenderer() = delete;
                                
    float m_column1[4];
    float m_column2[4];
    float m_column3[4];
    float m_column4[4];

    float m_offset[4];
};

class MatrixRenderer : public OpCPU
{
public:
    MatrixRenderer(const MatrixOpDataRcPtr & mat);

    void apply(float * rgbaBuffer, long numPixels) const override;

private:
    MatrixRenderer() = delete;

    float m_column1[4];
    float m_column2[4];
    float m_column3[4];
    float m_column4[4];
};

ScaleRenderer::ScaleRenderer(const MatrixOpDataRcPtr & mat)
    : OpCPU()
{
    const ArrayDouble::Values & m = mat->getArray().getValues();

    m_scale[0] = (float)m[0];
    m_scale[1] = (float)m[5];
    m_scale[2] = (float)m[10];
    m_scale[3] = (float)m[15];
}

void ScaleRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    float * rgba = rgbaBuffer;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        rgba[0] *= m_scale[0];
        rgba[1] *= m_scale[1];
        rgba[2] *= m_scale[2];
        rgba[3] *= m_scale[3];

        rgba += 4;
    }
}

ScaleWithOffsetRenderer::ScaleWithOffsetRenderer(const MatrixOpDataRcPtr & mat)
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

void ScaleWithOffsetRenderer::apply(float * rgbaBuffer,
                                    long numPixels) const
{
    float * rgba = rgbaBuffer;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        rgba[0] = rgba[0] * m_scale[0] + m_offset[0];
        rgba[1] = rgba[1] * m_scale[1] + m_offset[1];
        rgba[2] = rgba[2] * m_scale[2] + m_offset[2];
        rgba[3] = rgba[3] * m_scale[3] + m_offset[3];

        rgba += 4;
    }
}

MatrixWithOffsetRenderer::MatrixWithOffsetRenderer(const MatrixOpDataRcPtr & mat)
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
void MatrixWithOffsetRenderer::apply(float * rgbaBuffer,
                                     long numPixels) const
{
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

    float * rgba = rgbaBuffer;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        __m128 r = _mm_set1_ps(rgba[0]);
        __m128 g = _mm_set1_ps(rgba[1]);
        __m128 b = _mm_set1_ps(rgba[2]);
        __m128 a = _mm_set1_ps(rgba[3]);

        __m128 rm0 = _mm_mul_ps(m0, r);
        __m128 gm1 = _mm_mul_ps(m1, g);
        __m128 bm2 = _mm_mul_ps(m2, b);
        __m128 am3 = _mm_mul_ps(m3, a);

        __m128 img = _mm_add_ps(_mm_add_ps(rm0, gm1), _mm_add_ps(bm2, am3));
        img = _mm_add_ps(img, o);

        _mm_storeu_ps(rgba, img);

        rgba += 4;
    }
#else
    float * rgba = rgbaBuffer;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        const float r = rgba[0];
        const float g = rgba[1];
        const float b = rgba[2];
        const float a = rgba[3];

        rgba[0] = r*m_column1[0]
                + g*m_column2[0]
                + b*m_column3[0]
                + a*m_column4[0]
                + m_offset[0];
        rgba[1] = r*m_column1[1]
                + g*m_column2[1]
                + b*m_column3[1]
                + a*m_column4[1]
                + m_offset[1];
        rgba[2] = r*m_column1[2]
                + g*m_column2[2]
                + b*m_column3[2]
                + a*m_column4[2]
                + m_offset[2];
        rgba[3] = r*m_column1[3]
                + g*m_column2[3]
                + b*m_column3[3]
                + a*m_column4[3]
                + m_offset[3];

        rgba += 4;
    }
#endif

}

MatrixRenderer::MatrixRenderer(const MatrixOpDataRcPtr & mat)
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

void MatrixRenderer::apply(float * rgbaBuffer, long numPixels) const
{
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

    float * rgba = rgbaBuffer;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        __m128 r = _mm_set1_ps(rgba[0]);
        __m128 g = _mm_set1_ps(rgba[1]);
        __m128 b = _mm_set1_ps(rgba[2]);
        __m128 a = _mm_set1_ps(rgba[3]);

        __m128 rm0 = _mm_mul_ps(m0, r);
        __m128 gm1 = _mm_mul_ps(m1, g);
        __m128 bm2 = _mm_mul_ps(m2, b);
        __m128 am3 = _mm_mul_ps(m3, a);

        __m128 img = _mm_add_ps(_mm_add_ps(rm0, gm1), _mm_add_ps(bm2, am3));

        _mm_storeu_ps(rgba, img);

        rgba += 4;
    }
#else
    float * rgba = rgbaBuffer;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        const float r = rgba[0];
        const float g = rgba[1];
        const float b = rgba[2];
        const float a = rgba[3];

        rgba[0] = r*m_column1[0]
                + g*m_column2[0]
                + b*m_column3[0]
                + a*m_column4[0];
        rgba[1] = r*m_column1[1]
                + g*m_column2[1]
                + b*m_column3[1]
                + a*m_column4[1];
        rgba[2] = r*m_column1[2]
                + g*m_column2[2]
                + b*m_column3[2]
                + a*m_column4[2];
        rgba[3] = r*m_column1[3]
                + g*m_column2[3]
                + b*m_column3[3]
                + a*m_column4[3];

        rgba += 4;
    }

#endif
}

}

OpCPURcPtr GetMatrixRenderer(const MatrixOpDataRcPtr & mat)
{
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

}
OCIO_NAMESPACE_EXIT


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

// TODO: syncolor also tests various bit-depths and pixel formats.
// CPURenderer_cases.cpp_inc - CPURendererMatrix
// CPURenderer_cases.cpp_inc - CPURendererMatrixWithOffsets
// CPURenderer_cases.cpp_inc - CPURendererMatrixWithOffsets4
// CPURenderer_cases.cpp_inc - CPURendererMatrixWithOffsets_check_scaling
// CPURenderer_cases.cpp_inc - CPURendererMatrixWithOffsets4_check_scaling
// CPURenderer_cases.cpp_inc - CPURendererMatrix_half

OCIO_NAMESPACE_USING

OIIO_ADD_TEST(MatrixOpCPU, scale_renderer)
{
    MatrixOpDataRcPtr mat(MatrixOpData::CreateDiagonalMatrix(
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 2.0));

    OpCPURcPtr op = GetMatrixRenderer(mat);
    OIIO_CHECK_ASSERT((bool)op);

    ScaleRenderer* scaleOp = dynamic_cast<ScaleRenderer*>(op.get());
    OIIO_CHECK_ASSERT(scaleOp);

    float rgba[4] = { 4.f, 3.f, 2.f, 1.f };

    op->apply(rgba, 1);

    OIIO_CHECK_EQUAL(rgba[0], 8.f);
    OIIO_CHECK_EQUAL(rgba[1], 6.f);
    OIIO_CHECK_EQUAL(rgba[2], 4.f);
    OIIO_CHECK_EQUAL(rgba[3], 2.f);
}

OIIO_ADD_TEST(MatrixOpCPU, scale_with_offset_renderer)
{
    MatrixOpDataRcPtr mat(MatrixOpData::CreateDiagonalMatrix(
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 2.0));
    
    mat->setOffsetValue(0, 1.f);
    mat->setOffsetValue(1, 2.f);
    mat->setOffsetValue(2, 3.f);
    mat->setOffsetValue(3, 4.f);

    OpCPURcPtr op = GetMatrixRenderer(mat);
    OIIO_CHECK_ASSERT((bool)op);

    ScaleWithOffsetRenderer* scaleOffOp = dynamic_cast<ScaleWithOffsetRenderer*>(op.get());
    OIIO_CHECK_ASSERT(scaleOffOp);

    float rgba[4] = { 4.f, 3.f, 2.f, 1.f };

    op->apply(rgba, 1);

    OIIO_CHECK_EQUAL(rgba[0], 9.f);
    OIIO_CHECK_EQUAL(rgba[1], 8.f);
    OIIO_CHECK_EQUAL(rgba[2], 7.f);
    OIIO_CHECK_EQUAL(rgba[3], 6.f);
}


OIIO_ADD_TEST(MatrixOpCPU, matrix_with_offset_renderer)
{
    MatrixOpDataRcPtr mat(MatrixOpData::CreateDiagonalMatrix(
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 2.0));

    // set offset
    mat->setOffsetValue(0, 1.f);
    mat->setOffsetValue(1, 2.f);
    mat->setOffsetValue(2, 3.f);
    mat->setOffsetValue(3, 4.f);

    // make not diag
    mat->setArrayValue(3, 0.5f);

    OpCPURcPtr op = GetMatrixRenderer(mat);
    OIIO_CHECK_ASSERT((bool)op);

    MatrixWithOffsetRenderer* matOffOp = dynamic_cast<MatrixWithOffsetRenderer*>(op.get());
    OIIO_CHECK_ASSERT(matOffOp);

    float rgba[4] = { 4.f, 3.f, 2.f, 1.f };

    op->apply(rgba, 1);

    OIIO_CHECK_EQUAL(rgba[0], 9.5f);
    OIIO_CHECK_EQUAL(rgba[1], 8.f);
    OIIO_CHECK_EQUAL(rgba[2], 7.f);
    OIIO_CHECK_EQUAL(rgba[3], 6.f);
}

OIIO_ADD_TEST(MatrixOpCPU, matrix_renderer)
{
    MatrixOpDataRcPtr mat(MatrixOpData::CreateDiagonalMatrix(
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 2.0));

    // Make not diagonal.
    mat->setArrayValue(3, 0.5f);

    OpCPURcPtr op = GetMatrixRenderer(mat);
    OIIO_CHECK_ASSERT((bool)op);

    MatrixRenderer* matOp = dynamic_cast<MatrixRenderer*>(op.get());
    OIIO_CHECK_ASSERT(matOp);

    float rgba[4] = { 4.f, 3.f, 2.f, 1.f };

    op->apply(rgba, 1);

    OIIO_CHECK_EQUAL(rgba[0], 8.5f);
    OIIO_CHECK_EQUAL(rgba[1], 6.f);
    OIIO_CHECK_EQUAL(rgba[2], 4.f);
    OIIO_CHECK_EQUAL(rgba[3], 2.f);
}



#endif
