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

#include "CPUMatrixOp.h"

#include "../MathUtils.h"
#include "SSE2.h"

OCIO_NAMESPACE_ENTER
{

    namespace CPUMatrixOp
    {
        CPUOpRcPtr GetRenderer(const OpData::OpDataMatrixRcPtr & mat)
        {
            CPUOpRcPtr op(new CPUNoOp);

            if (mat->isDiagonal())
            {
                if (mat->hasOffsets())
                {
                    op.reset(new ScaleWithOffsetRenderer(mat));
                }
                else
                {
                    op.reset(new ScaleRenderer(mat));
                }
            }
            else
            {
                if (mat->hasOffsets())
                {
                    op.reset(new MatrixWithOffsetRenderer(mat));
                }
                else
                {
                    op.reset(new MatrixRenderer(mat));
                }
            }

            return op;
        }
    }

    ScaleRenderer::ScaleRenderer(const OpData::OpDataMatrixRcPtr & mat)
        : CPUOp()
    {
        const OpData::ArrayDouble::Values& m = mat->getArray().getValues();

        m_scale[0] = (float)m[0];
        m_scale[1] = (float)m[5];
        m_scale[2] = (float)m[10];
        m_scale[3] = (float)m[15];
    }

    void ScaleRenderer::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        float * rgba = rgbaBuffer;

        for (unsigned idx = 0; idx<numPixels; ++idx)
        {
            rgba[0] *= m_scale[0];
            rgba[1] *= m_scale[1];
            rgba[2] *= m_scale[2];
            rgba[3] *= m_scale[3];

            rgba += 4;
        }
    }


    ScaleWithOffsetRenderer::ScaleWithOffsetRenderer(
        const OpData::OpDataMatrixRcPtr & mat)
        : CPUOp()
    {
        const OpData::ArrayDouble::Values& m = mat->getArray().getValues();

        m_scale[0] = (float)m[0];
        m_scale[1] = (float)m[5];
        m_scale[2] = (float)m[10];
        m_scale[3] = (float)m[15];

        const OpData::Matrix::Offsets& o = mat->getOffsets();

        m_offset[0] = (float)o[0];
        m_offset[1] = (float)o[1];
        m_offset[2] = (float)o[2];
        m_offset[3] = (float)o[3];
    }

    void ScaleWithOffsetRenderer::apply(float * rgbaBuffer,
                                        unsigned numPixels) const
    {
        float * rgba = rgbaBuffer;

        for (unsigned idx = 0; idx<numPixels; ++idx)
        {
            rgba[0] = rgba[0] * m_scale[0] + m_offset[0];
            rgba[1] = rgba[1] * m_scale[1] + m_offset[1];
            rgba[2] = rgba[2] * m_scale[2] + m_offset[2];
            rgba[3] = rgba[3] * m_scale[3] + m_offset[3];

            rgba += 4;
        }
    }

    MatrixWithOffsetRenderer::MatrixWithOffsetRenderer(
        const OpData::OpDataMatrixRcPtr & mat)
        : CPUOp()
    {
        const unsigned dim = mat->getArray().getLength();
        const unsigned twoDim = 2 * dim;
        const unsigned threeDim = 3 * dim;
        const OpData::ArrayDouble::Values& m = mat->getArray().getValues();

        // red multipliers
        m_column1[0] = (float)m[0];
        m_column1[1] = (float)m[dim];
        m_column1[2] = (float)m[twoDim];
        m_column1[3] = (float)m[threeDim];

        // green multipliers
        m_column2[0] = (float)m[1];
        m_column2[1] = (float)m[dim + 1];
        m_column2[2] = (float)m[twoDim + 1];
        m_column2[3] = (float)m[threeDim + 1];

        // blue multipliers
        m_column3[0] = (float)m[2];
        m_column3[1] = (float)m[dim + 2];
        m_column3[2] = (float)m[twoDim + 2];
        m_column3[3] = (float)m[threeDim + 2];

        // alpha multipliers
        m_column4[0] = (float)m[3];
        m_column4[1] = (float)m[dim + 3];
        m_column4[2] = (float)m[twoDim + 3];
        m_column4[3] = (float)m[threeDim + 3];

        const OpData::Matrix::Offsets& o = mat->getOffsets();

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
                                         unsigned numPixels) const
    {
        // Matrix decomposition per _column
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

        for (unsigned idx = 0; idx<numPixels; ++idx)
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
    }

    MatrixRenderer::MatrixRenderer(const OpData::OpDataMatrixRcPtr & mat)
        : CPUOp()
    {
        const unsigned dim = mat->getArray().getLength();
        const unsigned twoDim = 2 * dim;
        const unsigned threeDim = 3 * dim;
        const OpData::ArrayDouble::Values& m = mat->getArray().getValues();

        // red multipliers
        m_column1[0] = (float)m[0];
        m_column1[1] = (float)m[dim];
        m_column1[2] = (float)m[twoDim];
        m_column1[3] = (float)m[threeDim];

        // green multipliers
        m_column2[0] = (float)m[1];
        m_column2[1] = (float)m[dim + 1];
        m_column2[2] = (float)m[twoDim + 1];
        m_column2[3] = (float)m[threeDim + 1];

        // blue multipliers
        m_column3[0] = (float)m[2];
        m_column3[1] = (float)m[dim + 2];
        m_column3[2] = (float)m[twoDim + 2];
        m_column3[3] = (float)m[threeDim + 2];

        // alpha multipliers
        m_column4[0] = (float)m[3];
        m_column4[1] = (float)m[dim + 3];
        m_column4[2] = (float)m[twoDim + 3];
        m_column4[3] = (float)m[threeDim + 3];
    }

    void MatrixRenderer::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        // Matrix decomposition per _column
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

        for (unsigned idx = 0; idx<numPixels; ++idx)
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
    }
}
OCIO_NAMESPACE_EXIT


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "../UnitTest.h"

OCIO_NAMESPACE_USING

OIIO_ADD_TEST(CPUMatrixOp, CheckType)
{
    OCIO::OpData::OpDataMatrixRcPtr mat(OCIO::OpData::Matrix::CreateDiagonalMatrix(
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 2.0));

    // These constructor do not validate that mat is what it is supposed to be
    OCIO::ScaleRenderer sr(mat);
    OCIO::ScaleWithOffsetRenderer sor(mat);
    OCIO::MatrixWithOffsetRenderer mor(mat);
    OCIO::MatrixRenderer mr(mat);

    // Validate that dynamic_cast ca nbe used to validate the type
    OCIO::CPUOp * op = NULL;
    {
        op = &sr;
        OIIO_CHECK_ASSERT(dynamic_cast<OCIO::ScaleRenderer *>(op));
        OIIO_CHECK_ASSERT(NULL == dynamic_cast<OCIO::ScaleWithOffsetRenderer *>(op));
        OIIO_CHECK_ASSERT(NULL == dynamic_cast<OCIO::MatrixWithOffsetRenderer *>(op));
        OIIO_CHECK_ASSERT(NULL == dynamic_cast<OCIO::MatrixRenderer *>(op));
    }
    {
        op = &sor;
        OIIO_CHECK_ASSERT(NULL == dynamic_cast<OCIO::ScaleRenderer *>(op));
        OIIO_CHECK_ASSERT(dynamic_cast<OCIO::ScaleWithOffsetRenderer *>(op));
        OIIO_CHECK_ASSERT(NULL == dynamic_cast<OCIO::MatrixWithOffsetRenderer *>(op));
        OIIO_CHECK_ASSERT(NULL == dynamic_cast<OCIO::MatrixRenderer *>(op));
    }
    {
        op = &mor;
        OIIO_CHECK_ASSERT(NULL == dynamic_cast<OCIO::ScaleRenderer *>(op));
        OIIO_CHECK_ASSERT(NULL == dynamic_cast<OCIO::ScaleWithOffsetRenderer *>(op));
        OIIO_CHECK_ASSERT(dynamic_cast<OCIO::MatrixWithOffsetRenderer *>(op));
        OIIO_CHECK_ASSERT(NULL == dynamic_cast<OCIO::MatrixRenderer *>(op));
    }
    {
        op = &mr;
        OIIO_CHECK_ASSERT(NULL == dynamic_cast<OCIO::ScaleRenderer *>(op));
        OIIO_CHECK_ASSERT(NULL == dynamic_cast<OCIO::ScaleWithOffsetRenderer *>(op));
        OIIO_CHECK_ASSERT(NULL == dynamic_cast<OCIO::MatrixWithOffsetRenderer *>(op));
        OIIO_CHECK_ASSERT(dynamic_cast<OCIO::MatrixRenderer *>(op));
    }
}

OIIO_ADD_TEST(CPUMatrixOp, ScaleRenderer)
{
    OCIO::OpData::OpDataMatrixRcPtr mat(OCIO::OpData::Matrix::CreateDiagonalMatrix(
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 2.0));

    CPUOpRcPtr op = OCIO::CPUMatrixOp::GetRenderer(mat);
    OIIO_CHECK_ASSERT((bool)op);

    OCIO::ScaleRenderer* scaleOp = dynamic_cast<OCIO::ScaleRenderer*>(op.get());
    OIIO_CHECK_ASSERT(scaleOp);

    float rgba[4] = { 4.f, 3.f, 2.f, 1.f };

    op->apply(rgba, 1);

    OIIO_CHECK_EQUAL(rgba[0], 8.f);
    OIIO_CHECK_EQUAL(rgba[1], 6.f);
    OIIO_CHECK_EQUAL(rgba[2], 4.f);
    OIIO_CHECK_EQUAL(rgba[3], 2.f);
}

OIIO_ADD_TEST(CPUMatrixOp, ScaleWithOffsetRenderer)
{
    OCIO::OpData::OpDataMatrixRcPtr mat(OCIO::OpData::Matrix::CreateDiagonalMatrix(
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 2.0));
    
    mat->setOffsetValue(0, 1.f);
    mat->setOffsetValue(1, 2.f);
    mat->setOffsetValue(2, 3.f);
    mat->setOffsetValue(3, 4.f);

    CPUOpRcPtr op = OCIO::CPUMatrixOp::GetRenderer(mat);
    OIIO_CHECK_ASSERT((bool)op);

    OCIO::ScaleWithOffsetRenderer* scaleOffOp = dynamic_cast<OCIO::ScaleWithOffsetRenderer*>(op.get());
    OIIO_CHECK_ASSERT(scaleOffOp);

    float rgba[4] = { 4.f, 3.f, 2.f, 1.f };

    op->apply(rgba, 1);

    OIIO_CHECK_EQUAL(rgba[0], 9.f);
    OIIO_CHECK_EQUAL(rgba[1], 8.f);
    OIIO_CHECK_EQUAL(rgba[2], 7.f);
    OIIO_CHECK_EQUAL(rgba[3], 6.f);
}


OIIO_ADD_TEST(CPUMatrixOp, MatrixWithOffsetRenderer)
{
    OCIO::OpData::OpDataMatrixRcPtr mat(OCIO::OpData::Matrix::CreateDiagonalMatrix(
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 2.0));

    // set offset
    mat->setOffsetValue(0, 1.f);
    mat->setOffsetValue(1, 2.f);
    mat->setOffsetValue(2, 3.f);
    mat->setOffsetValue(3, 4.f);

    // make not diag
    mat->setArrayValue(3, 0.5f);

    CPUOpRcPtr op = OCIO::CPUMatrixOp::GetRenderer(mat);
    OIIO_CHECK_ASSERT((bool)op);

    OCIO::MatrixWithOffsetRenderer* matOffOp = dynamic_cast<OCIO::MatrixWithOffsetRenderer*>(op.get());
    OIIO_CHECK_ASSERT(matOffOp);

    float rgba[4] = { 4.f, 3.f, 2.f, 1.f };

    op->apply(rgba, 1);

    OIIO_CHECK_EQUAL(rgba[0], 9.5f);
    OIIO_CHECK_EQUAL(rgba[1], 8.f);
    OIIO_CHECK_EQUAL(rgba[2], 7.f);
    OIIO_CHECK_EQUAL(rgba[3], 6.f);
}

OIIO_ADD_TEST(CPUMatrixOp, MatrixRenderer)
{
    OCIO::OpData::OpDataMatrixRcPtr mat(OCIO::OpData::Matrix::CreateDiagonalMatrix(
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, 2.0));

    // make not diag
    mat->setArrayValue(3, 0.5f);

    CPUOpRcPtr op = OCIO::CPUMatrixOp::GetRenderer(mat);
    OIIO_CHECK_ASSERT((bool)op);

    OCIO::MatrixRenderer* matOp = dynamic_cast<OCIO::MatrixRenderer*>(op.get());
    OIIO_CHECK_ASSERT(matOp);

    float rgba[4] = { 4.f, 3.f, 2.f, 1.f };

    op->apply(rgba, 1);

    OIIO_CHECK_EQUAL(rgba[0], 8.5f);
    OIIO_CHECK_EQUAL(rgba[1], 6.f);
    OIIO_CHECK_EQUAL(rgba[2], 4.f);
    OIIO_CHECK_EQUAL(rgba[3], 2.f);
}



#endif
