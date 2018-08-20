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


#include "CPULut3DOp.h"

#include "../opdata/OpDataTools.h"

#include "../BitDepthUtils.h"
#include "../MathUtils.h"
#include "../Platform.h"

#include "SSE2.h"

#include <algorithm>
#include <math.h>
#include <stdint.h>


OCIO_NAMESPACE_ENTER
{
    namespace
    {
        void CheckLut3D(const OpData::OpDataLut3DRcPtr lut)
        {
            if(lut->getInputBitDepth()==BIT_DEPTH_UNKNOWN)
            {
                throw Exception("Unknown bit depth");
            }

            if(lut->getOutputBitDepth()==BIT_DEPTH_UNKNOWN)
            {
                throw Exception("Unknown bit depth");
            }

            if(lut->getConcreteInterpolation() != INTERP_LINEAR
               && lut->getConcreteInterpolation()!= INTERP_TETRAHEDRAL)
            {
                throw Exception("Unknown interpolation algorithm");
            }
        }

        //----------------------------------------------------------------------------
        // RGB channel ordering.
        // Pixels ordered in such a way that the blue coordinate changes fastest,
        // then the green coordinate, and finally, the red coordinate changes slowest
        //
        inline __m128i GetLut3DIndices(const __m128i &idxR,
                                       const __m128i &idxG,
                                       const __m128i &idxB,
                                       const __m128i /*&sizesR*/,
                                       const __m128i &sizesG,
                                       const __m128i &sizesB)
        {
            // SSE2 doesn't have 4-way multiplication for integer registers, so we need
            // split them into two register and multiply-add them separately, and then
            // combine the results.

            // r02 = { sizesG * idxR0, -, sizesG * idxR2, - }
            // r13 = { sizesG * idxR1, -, sizesG * idxR3, - }
            __m128i r02 = _mm_mul_epu32(sizesG, idxR);
            __m128i r13 = _mm_mul_epu32(sizesG, _mm_srli_si128(idxR,4));

            // r02 = { idxG0 + sizesG * idxR0, -, idxG2 + sizesG * idxR2, - }
            // r13 = { idxG1 + sizesG * idxR1, -, idxG3 + sizesG * idxR3, - }
            r02 = _mm_add_epi32(idxG, r02);
            r13 = _mm_add_epi32(_mm_srli_si128(idxG,4), r13);

            // r02 = { sizesB * (idxG0 + sizesG * idxR0), -, sizesB * (idxG2 + sizesG * idxR2), - }
            // r13 = { sizesB * (idxG1 + sizesG * idxR1), -, sizesB * (idxG3 + sizesG * idxR3), - }
            r02 = _mm_mul_epu32(sizesB, r02);
            r13 = _mm_mul_epu32(sizesB, r13);

            // r02 = { idxB0 + sizesB * (idxG0 + sizesG * idxR0), -, idxB2 + sizesB * (idxG2 + sizesG * idxR2), - }
            // r13 = { idxB1 + sizesB * (idxG1 + sizesG * idxR1), -, idxB3 + sizesB * (idxG3 + sizesG * idxR3), - }
            r02 = _mm_add_epi32(idxB, r02);
            r13 = _mm_add_epi32(_mm_srli_si128(idxB,4), r13);

            // r = { idxB0 + sizesB * (idxG0 + sizesG * idxR0),
            //       idxB1 + sizesB * (idxG1 + sizesG * idxR1),
            //       idxB2 + sizesB * (idxG2 + sizesG * idxR2),
            //       idxB3 + sizesB * (idxG3 + sizesG * idxR3) }
            __m128i r = _mm_unpacklo_epi32(_mm_shuffle_epi32(r02, _MM_SHUFFLE(0,0,2,0)),
                _mm_shuffle_epi32(r13, _MM_SHUFFLE(0,0,2,0)));

            // return { 4 * (idxB0 + sizesB * (idxG0 + sizesG * idxR0)),
            //          4 * (idxB1 + sizesB * (idxG1 + sizesG * idxR1)),
            //          4 * (idxB2 + sizesB * (idxG2 + sizesG * idxR2)),
            //          4 * (idxB3 + sizesB * (idxG3 + sizesG * idxR3)) }
            return _mm_slli_epi32(r, 2);
        }

        inline void LookupNearest4(float* optLut,
            const __m128i &rIndices,
            const __m128i &gIndices,
            const __m128i &bIndices,
            const __m128i &dim,
            __m128 res[4])
        {
            __m128i offsets = GetLut3DIndices(rIndices, gIndices, bIndices, dim, dim, dim);

            OCIO_ALIGN(int offsetInt[4]);
            _mm_store_si128((__m128i*)offsetInt, offsets);

            res[0] = _mm_load_ps(optLut + offsetInt[0]);
            res[1] = _mm_load_ps(optLut + offsetInt[1]);
            res[2] = _mm_load_ps(optLut + offsetInt[2]);
            res[3] = _mm_load_ps(optLut + offsetInt[3]);
        }

    }



    BaseLut3DRenderer::BaseLut3DRenderer(const OpData::OpDataLut3DRcPtr & lut)
        : CPUOp()
        , m_optLut(0x0)
        , m_dim(0)
        , m_step(0.0f)
        , m_maxIdx(0)
        , m_alphaScale(0.)
    {
        updateData(lut);
    }

    BaseLut3DRenderer::~BaseLut3DRenderer()
    {
        Platform::AlignedFree(m_optLut);
    }

    void BaseLut3DRenderer::updateData(const OpData::OpDataLut3DRcPtr & lut)
    {
        CheckLut3D(lut);

        m_alphaScale = GetBitDepthMaxValue(lut->getOutputBitDepth())
            / GetBitDepthMaxValue(lut->getInputBitDepth());

        m_dim = lut->getArray().getLength();

        m_maxIdx = (float)(m_dim - 1);

        m_step = 1.0f
            / OpData::GetValueStepSize(lut->getInputBitDepth(),
                                       m_dim);

        Platform::AlignedFree(m_optLut);
        m_optLut = createOptLut(lut->getArray().getValues());
    }

    // Creates a LUT aligned to a 16 byte boundary with RGB and 0 for alpha
    // in order to be able to load the LUT using _mm_load_ps.
    float* BaseLut3DRenderer::createOptLut(const OpData::Array::Values& lut) const
    {
        const unsigned maxEntries = m_dim * m_dim * m_dim;

        float *optLut =
            (float*)Platform::AlignedMalloc(maxEntries * 4 * sizeof(float),
                16);

        float* currentValue = optLut;
        for (unsigned idx = 0; idx<maxEntries; idx++)
        {
            currentValue[0] = SanitizeFloat(lut[idx * 3]);
            currentValue[1] = SanitizeFloat(lut[idx * 3 + 1]);
            currentValue[2] = SanitizeFloat(lut[idx * 3 + 2]);
            currentValue[3] = 0.0f;

            currentValue += 4;
        }

        return optLut;
    }


    Lut3DTetrahedralRenderer::Lut3DTetrahedralRenderer(const OpData::OpDataLut3DRcPtr & lut)
        : BaseLut3DRenderer(lut)
    {
    }

    Lut3DTetrahedralRenderer::~Lut3DTetrahedralRenderer()
    {
    }

    void Lut3DTetrahedralRenderer::apply(float *rgbaBuffer, unsigned nPixels) const
    {
        __m128 step = _mm_set1_ps(m_step);
        __m128 maxIdx = _mm_set1_ps((float)(m_dim - 1));
        __m128i dim = _mm_set1_epi32(m_dim);

        __m128 v[4];
        OCIO_ALIGN(float cmpDelta[4]);

        float * inBuffer = rgbaBuffer;
        float * outBuffer = rgbaBuffer;

        for (unsigned i = 0; i < nPixels; ++i)
        {
            float newAlpha = (float)inBuffer[3] * m_alphaScale;
            
            __m128 data = _mm_set_ps(inBuffer[3], inBuffer[2], inBuffer[1], inBuffer[0]);

            __m128 idx = _mm_mul_ps(data, step);

            idx = _mm_max_ps(idx, EZERO);  // NaNs become 0
            idx = _mm_min_ps(idx, maxIdx);

            // lowIdxInt32 = floor(idx), with lowIdx in [0, maxIdx]
            __m128i lowIdxInt32 = _mm_cvttps_epi32(idx);
            __m128 lowIdx = _mm_cvtepi32_ps(lowIdxInt32);

            // highIdxInt32 = ceil(idx), with highIdx in [1, maxIdx]
            __m128i highIdxInt32 = _mm_sub_epi32(lowIdxInt32,
                _mm_castps_si128(_mm_cmplt_ps(lowIdx, maxIdx)));

            __m128 delta = _mm_sub_ps(idx, lowIdx);
            __m128 delta0 = _mm_shuffle_ps(delta, delta, _MM_SHUFFLE(0, 0, 0, 0));
            __m128 delta1 = _mm_shuffle_ps(delta, delta, _MM_SHUFFLE(1, 1, 1, 1));
            __m128 delta2 = _mm_shuffle_ps(delta, delta, _MM_SHUFFLE(2, 2, 2, 2));

            // lh01 = {L0, H0, L1, H1}
            // lh23 = {L2, H2, L3, H3}, L3 and H3 are not used
            __m128i lh01 = _mm_unpacklo_epi32(lowIdxInt32, highIdxInt32);
            __m128i lh23 = _mm_unpackhi_epi32(lowIdxInt32, highIdxInt32);

            // Since the cube is split along the main diagonal, the lowest corner
            // and highest corner are always used.
            // v[0] = { L0, L1, L2 }
            // v[3] = { H0, H1, H2 }

            __m128i idxR, idxG, idxB;
            // Store vertices transposed on idxR, idxG and idxB:
            // idxR = { v0r, v1r, v2r, v3r }
            // idxG = { v0g, v1g, v2g, v3g }
            // idxB = { v0b, v1b, v2b, v3b }

            // Vertices differences (vi-vj) to be multiplied by the delta factors
            __m128 dv0, dv1, dv2;

            // In tetrahedral interpolation, the cube is divided along the main
            // diagonal into 6 tetrahedra.  We compare the relative fractional 
            // position within the cube (deltaArray) to know which tetrahedron
            // we are in and therefore which four vertices of the cube we need.

            // cmpDelta = { delta[0] >= delta[1], delta[1] >= delta[2], delta[2] >= delta[0], - }
            _mm_store_ps(cmpDelta,
                         _mm_cmpge_ps(delta,
                                      _mm_shuffle_ps(delta,
                                                     delta,
                                                     _MM_SHUFFLE(0, 0, 2, 1))));

            if (cmpDelta[0])  // delta[0] > delta[1]
            {
                if (cmpDelta[1])  // delta[1] > delta[2]
                {
                    // R > G > B

                    // v[1] = { H0, L1, L2 }
                    // v[2] = { H0, H1, L2 }

                    // idxR = { L0, H0, H0, H0 }
                    // idxG = { L1, L1, H1, H1 }
                    // idxB = { L2, L2, L2, H2 }
                    idxR = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 1, 1, 0));
                    idxG = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(3, 3, 2, 2));
                    idxB = _mm_shuffle_epi32(lh23, _MM_SHUFFLE(1, 0, 0, 0));

                    LookupNearest4(m_optLut, idxR, idxG, idxB, dim, v);

                    // Order: R G B => 0 1 2
                    dv0 = _mm_sub_ps(v[1], v[0]);
                    dv1 = _mm_sub_ps(v[2], v[1]);
                    dv2 = _mm_sub_ps(v[3], v[2]);
                }
                else if (!cmpDelta[2])  // delta[0] > delta[2]
                {
                    // R > B > G

                    // v[1] = { H0, L1, L2 }
                    // v[2] = { H0, L1, H2 }

                    // idxR = { L0, H0, H0, H0 }
                    // idxG = { L1, L1, L1, H1 }
                    // idxB = { L2, L2, H2, H2 }
                    idxR = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 1, 1, 0));
                    idxG = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(3, 2, 2, 2));
                    idxB = _mm_shuffle_epi32(lh23, _MM_SHUFFLE(1, 1, 0, 0));

                    LookupNearest4(m_optLut, idxR, idxG, idxB, dim, v);

                    // Order: R B G => 0 2 1
                    dv0 = _mm_sub_ps(v[1], v[0]);
                    dv2 = _mm_sub_ps(v[2], v[1]);
                    dv1 = _mm_sub_ps(v[3], v[2]);
                }
                else
                {
                    // B > R > G

                    // v[1] = { L0, L1, H2 }
                    // v[2] = { H0, L1, H2 }

                    // idxR = { L0, L0, H0, H0 }
                    // idxG = { L1, L1, L1, H1 }
                    // idxB = { L2, H2, H2, H2 }
                    idxR = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 1, 0, 0));
                    idxG = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(3, 2, 2, 2));
                    idxB = _mm_shuffle_epi32(lh23, _MM_SHUFFLE(1, 1, 1, 0));

                    LookupNearest4(m_optLut, idxR, idxG, idxB, dim, v);

                    // Order: B R G => 2 0 1
                    dv2 = _mm_sub_ps(v[1], v[0]);
                    dv0 = _mm_sub_ps(v[2], v[1]);
                    dv1 = _mm_sub_ps(v[3], v[2]);
                }
            }
            else
            {
                if (!cmpDelta[1])  // delta[2] > delta[1]
                {
                    // B > G > R

                    // v[1] = { L0, L1, H2 }
                    // v[2] = { L0, H1, H2 }

                    // idxR = { L0, L0, L0, H0 }
                    // idxG = { L1, L1, H1, H1 }
                    // idxB = { L2, H2, H2, H2 }
                    idxR = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 0, 0, 0));
                    idxG = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(3, 3, 2, 2));
                    idxB = _mm_shuffle_epi32(lh23, _MM_SHUFFLE(1, 1, 1, 0));

                    LookupNearest4(m_optLut, idxR, idxG, idxB, dim, v);

                    // Order: B G R => 2 1 0
                    dv2 = _mm_sub_ps(v[1], v[0]);
                    dv1 = _mm_sub_ps(v[2], v[1]);
                    dv0 = _mm_sub_ps(v[3], v[2]);
                }
                else if (!cmpDelta[2])  // delta[0] > delta[2]
                {
                    // G > R > B

                    // v[1] = { L0, H1, L2 }
                    // v[2] = { H0, H1, L2 }

                    // idxR = { L0, L0, H0, H0 }
                    // idxG = { L1, H1, H1, H1 }
                    // idxB = { L2, L2, L2, H2 }
                    idxR = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 1, 0, 0));
                    idxG = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(3, 3, 3, 2));
                    idxB = _mm_shuffle_epi32(lh23, _MM_SHUFFLE(1, 0, 0, 0));

                    LookupNearest4(m_optLut, idxR, idxG, idxB, dim, v);

                    // Order: G R B => 1 0 2
                    dv1 = _mm_sub_ps(v[1], v[0]);
                    dv0 = _mm_sub_ps(v[2], v[1]);
                    dv2 = _mm_sub_ps(v[3], v[2]);
                }
                else
                {
                    // G > B > R

                    // v[1] = { L0, H1, L2 }
                    // v[2] = { L0, H1, H2 }

                    // idxR = { L0, L0, L0, H0 }
                    // idxG = { L1, H1, H1, H1 }
                    // idxB = { L2, L2, H2, H2 }
                    idxR = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 0, 0, 0));
                    idxG = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(3, 3, 3, 2));
                    idxB = _mm_shuffle_epi32(lh23, _MM_SHUFFLE(1, 1, 0, 0));

                    LookupNearest4(m_optLut, idxR, idxG, idxB, dim, v);

                    // Order: G B R => 1 2 0
                    dv1 = _mm_sub_ps(v[1], v[0]);
                    dv2 = _mm_sub_ps(v[2], v[1]);
                    dv0 = _mm_sub_ps(v[3], v[2]);
                }
            }

            __m128 result = _mm_add_ps(_mm_add_ps(v[0], _mm_mul_ps(delta0, dv0)),
                _mm_add_ps(_mm_mul_ps(delta1, dv1), _mm_mul_ps(delta2, dv2)));

            // TODO: use aligned version if outBuffer alignement can be controlled (see OCIO_ALIGN)
            _mm_storeu_ps(outBuffer, result);

            outBuffer[3] = newAlpha;

            inBuffer += 4;
            outBuffer += 4;
        }
    }

    Lut3DRenderer::Lut3DRenderer(const OpData::OpDataLut3DRcPtr & lut)
        : BaseLut3DRenderer(lut)
    {
    }
    
    Lut3DRenderer::~Lut3DRenderer()
    {
    }

    void Lut3DRenderer::apply(float *rgbaBuffer, unsigned nPixels) const
    {
        __m128 step = _mm_set1_ps(m_step);
        __m128 maxIdx = _mm_set1_ps((float)(m_dim - 1));
        __m128i dim = _mm_set1_epi32(m_dim);

        __m128 v[8];

        float * imgBuffer = rgbaBuffer;

        for (unsigned i = 0; i < nPixels; ++i)
        {
            float newAlpha = (float)imgBuffer[3] * m_alphaScale;

            __m128 data = _mm_set_ps(imgBuffer[3],
                                     imgBuffer[2],
                                     imgBuffer[1],
                                     imgBuffer[0]);

            __m128 idx = _mm_mul_ps(data, step);

            idx = _mm_max_ps(idx, EZERO);  // NaNs become 0
            idx = _mm_min_ps(idx, maxIdx);

            // lowIdxInt32 = floor(idx), with lowIdx in [0, maxIdx]
            __m128i lowIdxInt32 = _mm_cvttps_epi32(idx);
            __m128 lowIdx = _mm_cvtepi32_ps(lowIdxInt32);

            // highIdxInt32 = ceil(idx), with highIdx in [1, maxIdx]
            __m128i highIdxInt32 = _mm_sub_epi32(lowIdxInt32,
                _mm_castps_si128(_mm_cmplt_ps(lowIdx, maxIdx)));


            __m128 delta = _mm_sub_ps(idx, lowIdx);

            // lh01 = {L0, H0, L1, H1}
            // lh23 = {L2, H2, L3, H3}, L3 and H3 are not used
            __m128i lh01 = _mm_unpacklo_epi32(lowIdxInt32, highIdxInt32);
            __m128i lh23 = _mm_unpackhi_epi32(lowIdxInt32, highIdxInt32);

            // v[0] = { L0, L1, L2 }
            // v[1] = { L0, L1, H2 }
            // v[2] = { L0, H1, L2 }
            // v[3] = { L0, H1, H2 }
            // v[4] = { H0, L1, L2 }
            // v[5] = { H0, L1, H2 }
            // v[6] = { H0, H1, L2 }
            // v[7] = { H0, H1, H2 }

            __m128i idxR_L0, idxR_H0, idxG, idxB;
            // Store vertices transposed on idxR, idxG and idxB:

            // idxR_L0 = { L0, L0, L0, L0 }
            // idxR_H0 = { H0, H0, H0, H0 }
            // idxG = { L1, L1, H1, H1 }
            // idxB = { L2, H2, L2, H2 }
            idxR_L0 = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(0, 0, 0, 0));
            idxR_H0 = _mm_shuffle_epi32(lh01, _MM_SHUFFLE(1, 1, 1, 1));
            idxG = _mm_unpackhi_epi32(lh01, lh01);
            idxB = _mm_unpacklo_epi64(lh23, lh23);

            // Lookup 8 corners of cube
            LookupNearest4(m_optLut, idxR_L0, idxG, idxB, dim, v);
            LookupNearest4(m_optLut, idxR_H0, idxG, idxB, dim, v + 4);

            // Perform the trilinear interpolation
            __m128 wr = _mm_shuffle_ps(delta, delta, _MM_SHUFFLE(0, 0, 0, 0));
            __m128 wg = _mm_shuffle_ps(delta, delta, _MM_SHUFFLE(1, 1, 1, 1));
            __m128 wb = _mm_shuffle_ps(delta, delta, _MM_SHUFFLE(2, 2, 2, 2));

            __m128 oneMinusWr = _mm_sub_ps(EONE, wr);
            __m128 oneMinusWg = _mm_sub_ps(EONE, wg);
            __m128 oneMinusWb = _mm_sub_ps(EONE, wb);

            // Compute linear interpolation along the blue axis
            __m128 blue1(_mm_add_ps(_mm_mul_ps(v[0], oneMinusWb),
                _mm_mul_ps(v[1], wb)));

            __m128 blue2(_mm_add_ps(_mm_mul_ps(v[2], oneMinusWb),
                _mm_mul_ps(v[3], wb)));

            __m128 blue3(_mm_add_ps(_mm_mul_ps(v[4], oneMinusWb),
                _mm_mul_ps(v[5], wb)));

            __m128 blue4(_mm_add_ps(_mm_mul_ps(v[6], oneMinusWb),
                _mm_mul_ps(v[7], wb)));

            // Compute linear interpolation along the green axis
            __m128 green1(_mm_add_ps(_mm_mul_ps(blue1, oneMinusWg),
                _mm_mul_ps(blue2, wg)));

            __m128 green2(_mm_add_ps(_mm_mul_ps(blue3, oneMinusWg),
                _mm_mul_ps(blue4, wg)));

            // Compute linear interpolation along the red axis
            __m128 result = _mm_add_ps(_mm_mul_ps(green1, oneMinusWr),
                _mm_mul_ps(green2, wr));

            // TODO: use aligned version if imgBuffer alignement can be controlled (see OCIO_ALIGN)
            _mm_storeu_ps(imgBuffer, result);

            imgBuffer[3] = newAlpha;

            imgBuffer += 4;
        }

    }

    CPUOpRcPtr Lut3DRenderer::GetRenderer(const OpData::OpDataLut3DRcPtr & lut)
    {
        CPUOpRcPtr op(new CPUNoOp);

        if (lut->getConcreteInterpolation() == INTERP_TETRAHEDRAL)
        {
            op.reset(new Lut3DTetrahedralRenderer(lut));
        }
        else
        {
            op.reset(new Lut3DRenderer(lut));
        }

        return op;
    }

}
OCIO_NAMESPACE_EXIT

