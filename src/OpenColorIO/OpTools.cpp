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

#include <OpenColorIO/OpenColorIO.h>


#include "OpTools.h"
#include "BitDepthUtils.h"
#include "ops/Matrix/MatrixOps.h"


OCIO_NAMESPACE_ENTER
{

    unsigned GetLutIdealSize(BitDepth incomingBitDepth)
    {
        // Return the number of entries needed in order to do a lookup 
        // for the specified bit-depth.

        // For 32f, a look-up is impractical so in that case return 64k.

        switch(incomingBitDepth)
        {
            case BIT_DEPTH_UINT8:
            case BIT_DEPTH_UINT10:
            case BIT_DEPTH_UINT12:
            case BIT_DEPTH_UINT16:
                return (unsigned)(GetBitDepthMaxValue(incomingBitDepth) + 1);

            case BIT_DEPTH_UNKNOWN:
            case BIT_DEPTH_UINT14:
            case BIT_DEPTH_UINT32:
            default:
            {
                std::string err("Bit depth is not supported: ");
                err += BitDepthToString(incomingBitDepth);
                throw Exception(err.c_str());
                break;
            }

            case BIT_DEPTH_F16:
            case BIT_DEPTH_F32:
                break;
        }

        return 65536;
    }

    //----------------------------------------------------------------------------------------------
    //
    // Functional composition is a concept from mathematics where two functions
    // are combined into a single function.  This idea may be applied to ops
    // where we generate a single op that has the same (or similar) effect as
    // applying the two ops separately.  The motivation is faster processing.
    //
    // When composing LUTs, the algorithm produces a result which takes the
    // domain of the first op into the range of the last op.  So the algorithm
    // needs to render values through the ops.  In some cases the domain of
    // the first op is sufficient, in other cases we need to create a new more
    // finely sampled domain to try and make the result less lossy.


    //----------------------------------------------------------------------------------------------
    // Calculate a new LUT by evaluating a new domain (A) through a set of ops (B).
    //
    // Note1: The caller must ensure that B is separable (channel independent).
    //
    // Note2: Unlike compose(Lut1DOp,Lut1DOp), this function does not try to resize
    //        the first LUT (A), so the caller needs to create a suitable domain.
    //
    Lut1DRcPtr Compose(const Lut1DRcPtr & A, const OpRcPtrVec & B)
    {
        if (A->outputBitDepth != B[0]->getInputBitDepth())
        {
            throw Exception("A bit depth mismatch forbids the composition of luts");
        }

        OpRcPtrVec ops;

        // Insert an op to compensate for the bitdepth scaling of A.
        //
        // The values in A's array have a certain scaling which needs to be
        // normalized since ops will have an in-depth of 32f.
        // Although it may seem like we could use the constructor to create
        // a bit-depth conversion identity from A's out-depth to 32f,
        // this doesn't work.  When pM gets appended, the set depth call
        // would cause the scale factor to disappear.  In this case, the
        // append set depth and the finalize set depth cancel out and we
        // are left with our desired scale being applied.

        const float iScale = 1.f / GetBitDepthMaxValue(A->outputBitDepth);
        const float iScale4[4] = { iScale, iScale, iScale, iScale };
        CreateScaleOp(ops, iScale4, TRANSFORM_DIR_FORWARD);

        // Copy and append B.
        for(OpRcPtrVec::size_type i=0, size = B.size(); i<size; ++i)
        {
            ops.push_back(B[i]->clone());
        }

        // Insert an op to compensate for the bitdepth scaling of B.
        //
        // We render at 32f but need to create an array which may be inserted
        // into a LUT with B's output depth, so apply the scaling manually.
        // For the explanation of the bit-depths used, see the comment above.

        const float oScale = GetBitDepthMaxValue(B[B.size()-1]->getOutputBitDepth());
        const float oScale4[4] = { oScale, oScale, oScale, oScale };
        CreateScaleOp(ops, oScale4, TRANSFORM_DIR_FORWARD);

        // Evaluate

        const size_t newLength = A->luts[0].size();

        std::vector<float> in;
        in.resize(newLength*4);
        for(size_t idx=0; idx<newLength; ++idx)
        {
            in[4*idx + 0] = A->luts[0][idx];
            in[4*idx + 1] = A->luts[1][idx];
            in[4*idx + 2] = A->luts[2][idx];
            in[4*idx + 3] =	1.0f;
        }

        for(OpRcPtrVec::size_type i=0, size = ops.size(); i<size; ++i)
        {
            ops[i]->apply(&in[0], (unsigned)newLength);
        }

        Lut1DRcPtr lut(Lut1D::Create());
        lut->inputBitDepth  = A->inputBitDepth;
        lut->outputBitDepth = B[B.size()-1]->getOutputBitDepth();
        lut->luts[0].resize(newLength);
        lut->luts[1].resize(newLength);
        lut->luts[2].resize(newLength);

        for(size_t idx=0; idx<newLength; ++idx)
        {
            lut->luts[0][idx] = in[4*idx + 0];
            lut->luts[1][idx] = in[4*idx + 1];
            lut->luts[2][idx] = in[4*idx + 2];
        }

        return lut;
    }

    Lut1DRcPtr Compose(const Lut1DRcPtr & A, const OpRcPtr & B, ComposeMethod compFlag)
    {
        if (A->outputBitDepth != B->getInputBitDepth())
        {
            throw Exception("A bit depth mismatch forbids the composition of luts");
        }

        OpRcPtrVec ops;

        unsigned min_size = 0;
        BitDepth resampleDepth = BIT_DEPTH_UINT16;
        switch (compFlag)
        {
            case COMPOSE_RESAMPLE_NO:
            {
                min_size = 0;
                break;
            }
            case COMPOSE_RESAMPLE_INDEPTH:
            {
                // TODO: Composition of LUTs is a potentially lossy operation.
                //
                // We try to be safe by ensuring that the result will be finely
                // sampled enough to do a look-up for the current input bit-depth,
                // but it is possible that that bit-depth will need to be reset
                // later.
                // In particular, if B is longer than this and the bit-depth is later
                // reset to be higher, we will have thrown away needed precision.
                // RESAMPLE_BIG is designed to avoid that problem but it has a
                // performance cost.
                resampleDepth = A->inputBitDepth;
                min_size = GetLutIdealSize(resampleDepth);
                break;
            }
            case COMPOSE_RESAMPLE_BIG:
            {
                resampleDepth = BIT_DEPTH_UINT16;
                min_size = (unsigned)GetBitDepthMaxValue(resampleDepth) + 1;
                break;
            }

            // TODO: May want to add another style which is the maximum of
            //       B size (careful of half domain), and in-depth ideal size.
        }

        const unsigned Asz = (unsigned)A->luts[0].size();
        const bool goodDomain = Asz >= min_size;
        const bool useOrigDomain = compFlag == COMPOSE_RESAMPLE_NO;

        Lut1DRcPtr domain;

        if ( goodDomain || useOrigDomain )
        {
            // Use the original domain.
            domain = A;
        }
        else
        {
            // Create identity with finer domain.

            // TODO: Should not need to create a new LUT object for this.
            //       Perhaps add a utility function to be shared with the constructor.
            domain = Lut1D::CreateIdentity(resampleDepth, A->inputBitDepth);

            // Interpolate through both LUTs in this case (resample).
            CreateLut1DOp(ops, A, INTERP_LINEAR, TRANSFORM_DIR_FORWARD);
        }

        ops.push_back(B);

        // Create the result LUT by composing the domain through the desired ops.
        return Compose(domain, ops);
    }

}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

#include <cstring>

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(OpTools, Lut1D_compose)
{
    OCIO::Lut1DRcPtr lut1  = OCIO::Lut1D::Create();
    lut1->inputBitDepth    =  OCIO::BIT_DEPTH_F32;
    lut1->outputBitDepth   =  OCIO::BIT_DEPTH_F32;
    lut1->luts[0].resize(8);
    lut1->luts[1].resize(8);
    lut1->luts[2].resize(8);

    lut1->luts[0][0] = 0.0f;      lut1->luts[1][0] = 0.0f;      lut1->luts[2][0] = 0.002333f;
    lut1->luts[0][1] = 0.0f;      lut1->luts[1][1] = 0.291341f; lut1->luts[2][1] = 0.015624f;
    lut1->luts[0][2] = 0.106521f; lut1->luts[1][2] = 0.334331f; lut1->luts[2][2] = 0.462431f;
    lut1->luts[0][3] = 0.515851f; lut1->luts[1][3] = 0.474151f; lut1->luts[2][3] = 0.624611f;
    lut1->luts[0][4] = 0.658791f; lut1->luts[1][4] = 0.527381f; lut1->luts[2][4] = 0.685071f;
    lut1->luts[0][5] = 0.908501f; lut1->luts[1][5] = 0.707951f; lut1->luts[2][5] = 0.886331f;
    lut1->luts[0][6] = 0.926671f; lut1->luts[1][6] = 0.846431f; lut1->luts[2][6] = 1.0f;
    lut1->luts[0][7] = 1.0f;      lut1->luts[1][7] = 1.0f;      lut1->luts[2][7] = 1.0f;

    OCIO::Lut1DRcPtr lut2  = OCIO::Lut1D::Create();
    lut2->inputBitDepth    = OCIO::BIT_DEPTH_F32;
    lut2->outputBitDepth   = OCIO::BIT_DEPTH_F32;
    lut2->luts[0].resize(8);
    lut2->luts[1].resize(8);
    lut2->luts[2].resize(8);

    lut2->luts[0][0] = 0.0f;        lut2->luts[1][0] = 0.0f;       lut2->luts[2][0] = 0.0023303f;
    lut2->luts[0][1] = 0.0f;        lut2->luts[1][1] = 0.0029134f; lut2->luts[2][1] = 0.015624f;
    lut2->luts[0][2] = 0.00010081f; lut2->luts[1][2] = 0.0059806f; lut2->luts[2][2] = 0.023362f;
    lut2->luts[0][3] = 0.0045628f;  lut2->luts[1][3] = 0.024229f;  lut2->luts[2][3] = 0.05822f;
    lut2->luts[0][4] = 0.0082598f;  lut2->luts[1][4] = 0.033831f;  lut2->luts[2][4] = 0.074063f;
    lut2->luts[0][5] = 0.028595f;   lut2->luts[1][5] = 0.075003f;  lut2->luts[2][5] = 0.13552f;
    lut2->luts[0][6] = 0.69154f;    lut2->luts[1][6] = 0.9213f;    lut2->luts[2][6] = 1.0f;
    lut2->luts[0][7] = 0.76038f;    lut2->luts[1][7] = 1.0f;       lut2->luts[2][7] = 1.0f;

    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW( 
        OCIO::CreateLut1DOp(ops, lut2, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);

    {
        const OCIO::Lut1DRcPtr lut = OCIO::Compose(lut1, ops[0], OCIO::COMPOSE_RESAMPLE_NO);

        OIIO_CHECK_EQUAL( lut->luts[0].size(), 8 );

        OIIO_CHECK_CLOSE( lut->luts[0][0], 0.0f,           1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][0], 0.0f,           1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][0], 0.00254739914f, 1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][1], 0.0f,           1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][1], 0.00669934973f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][1], 0.00378420483f, 1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][2], 0.0f,           1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][2], 0.0121908365f,  1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][2], 0.0619750582f,  1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][3], 0.00682150759f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][3], 0.0272925831f,  1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][3], 0.096942015f,   1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][4], 0.0206955168f,  1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][4], 0.0308703855f,  1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][4], 0.12295182f,    1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][5], 0.716288447f,   1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][5], 0.0731772855f,  1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][5], 1.0f,           1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][6], 0.725044191f,   1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][6], 0.857842028f,   1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][6], 1.0f,           1e-6f );
    }

    {
        const OCIO::Lut1DRcPtr lut = Compose(lut1, ops[0], OCIO::COMPOSE_RESAMPLE_INDEPTH);

        OIIO_CHECK_EQUAL( lut->luts[0].size(), 65536 );

        OIIO_CHECK_CLOSE( lut->luts[0][0], 0.0f,           1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][0], 0.0f,           1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][0], 0.00254739914f, 1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][1], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][1], 6.34463504e-07f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][1], 0.00254753046f,  1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][2], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][2], 1.26915984e-06f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][2], 0.00254766271f,  1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][3], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][3], 1.90362334e-06f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][3], 0.00254779495f,  1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][4], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][4], 2.53855251e-06f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][4], 0.0025479272f,   1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][5], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][5], 3.17324884e-06f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][5], 0.00254805945f,  1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][100], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][100], 6.3463347e-05f,  1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][100], 0.00256060902f,  1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][300], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][300], 0.000190390972f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][300], 0.00258703064f,  1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][900], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][900], 0.000571172219f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][900], 0.00266629551f,  1e-6f );
    }
}

OIIO_ADD_TEST(OpTools, Lut1D_compose__with_bit_depth)
{
    OCIO::Lut1DRcPtr lut1  = OCIO::Lut1D::Create();
    lut1->inputBitDepth    =  OCIO::BIT_DEPTH_UINT8;
    lut1->outputBitDepth   =  OCIO::BIT_DEPTH_UINT8;
    lut1->luts[0].resize(2);
    lut1->luts[1].resize(2);
    lut1->luts[2].resize(2);

    lut1->luts[0][0] =  64.0f; lut1->luts[1][0] =  64.0f; lut1->luts[2][0] =  64.0f;
    lut1->luts[0][1] = 196.0f; lut1->luts[1][1] = 196.0f; lut1->luts[2][1] = 196.0f;

    OCIO::Lut1DRcPtr lut2  = OCIO::Lut1D::Create();
    lut2->inputBitDepth    = OCIO::BIT_DEPTH_UINT8;
    lut2->outputBitDepth   = OCIO::BIT_DEPTH_F32;
    lut2->luts[0].resize(32);
    lut2->luts[1].resize(32);
    lut2->luts[2].resize(32);

    lut2->luts[0][ 0] = 0.0f;       lut2->luts[1][ 0] = 0.0f;       lut2->luts[2][ 0] = 0.0023303f;
    lut2->luts[0][ 1] = 0.0f;       lut2->luts[1][ 1] = 0.00018689f;lut2->luts[2][ 1] = 0.0052544f;
    lut2->luts[0][ 2] = 0.0f;       lut2->luts[1][ 2] = 0.0010572f; lut2->luts[2][ 2] = 0.0096338f;
    lut2->luts[0][ 3] = 0.0f;       lut2->luts[1][ 3] = 0.0029134f; lut2->luts[2][ 3] = 0.015624f;
    lut2->luts[0][ 4] = 0.00010081f;lut2->luts[1][ 4] = 0.0059806f; lut2->luts[2][ 4] = 0.023362f;
    lut2->luts[0][ 5] = 0.00070343f;lut2->luts[1][ 5] = 0.010448f;  lut2->luts[2][ 5] = 0.032968f;
    lut2->luts[0][ 6] = 0.002112f;  lut2->luts[1][ 6] = 0.016481f;  lut2->luts[2][ 6] = 0.044554f;
    lut2->luts[0][ 7] = 0.0045628f; lut2->luts[1][ 7] = 0.024229f;  lut2->luts[2][ 7] = 0.05822f;
    lut2->luts[0][ 8] = 0.0082598f; lut2->luts[1][ 8] = 0.033831f;  lut2->luts[2][ 8] = 0.074063f;
    lut2->luts[0][ 9] = 0.013387f;  lut2->luts[1][ 9] = 0.045415f;  lut2->luts[2][ 9] = 0.092171f;
    lut2->luts[0][10] = 0.020113f;  lut2->luts[1][10] = 0.059101f;  lut2->luts[2][10] = 0.11263f;
    lut2->luts[0][11] = 0.028595f;  lut2->luts[1][11] = 0.075003f;  lut2->luts[2][11] = 0.13552f;
    lut2->luts[0][12] = 0.038983f;  lut2->luts[1][12] = 0.093229f;  lut2->luts[2][12] = 0.16091f;
    lut2->luts[0][13] = 0.051418f;  lut2->luts[1][13] = 0.11388f;   lut2->luts[2][13] = 0.18888f;
    lut2->luts[0][14] = 0.066034f;  lut2->luts[1][14] = 0.13706f;   lut2->luts[2][14] = 0.2195f;
    lut2->luts[0][15] = 0.082962f;  lut2->luts[1][15] = 0.16286f;   lut2->luts[2][15] = 0.25283f;
    lut2->luts[0][16] = 0.10233f;   lut2->luts[1][16] = 0.19138f;   lut2->luts[2][16] = 0.28895f;
    lut2->luts[0][17] = 0.12425f;   lut2->luts[1][17] = 0.2227f;    lut2->luts[2][17] = 0.3279f;
    lut2->luts[0][18] = 0.14885f;   lut2->luts[1][18] = 0.25691f;   lut2->luts[2][18] = 0.36976f;
    lut2->luts[0][19] = 0.17623f;   lut2->luts[1][19] = 0.29409f;   lut2->luts[2][19] = 0.41459f;
    lut2->luts[0][20] = 0.20652f;   lut2->luts[1][20] = 0.33433f;   lut2->luts[2][20] = 0.46243f;
    lut2->luts[0][21] = 0.23982f;   lut2->luts[1][21] = 0.3777f;    lut2->luts[2][21] = 0.51334f;
    lut2->luts[0][22] = 0.27622f;   lut2->luts[1][22] = 0.42428f;   lut2->luts[2][22] = 0.56739f;
    lut2->luts[0][23] = 0.31585f;   lut2->luts[1][23] = 0.47415f;   lut2->luts[2][23] = 0.62461f;
    lut2->luts[0][24] = 0.35879f;   lut2->luts[1][24] = 0.52738f;   lut2->luts[2][24] = 0.68507f;
    lut2->luts[0][25] = 0.40515f;   lut2->luts[1][25] = 0.58404f;   lut2->luts[2][25] = 0.74881f;
    lut2->luts[0][26] = 0.45502f;   lut2->luts[1][26] = 0.64421f;   lut2->luts[2][26] = 0.81588f;
    lut2->luts[0][27] = 0.5085f;    lut2->luts[1][27] = 0.70795f;   lut2->luts[2][27] = 0.88633f;
    lut2->luts[0][28] = 0.56569f;   lut2->luts[1][28] = 0.77534f;   lut2->luts[2][28] = 0.96021f;
    lut2->luts[0][29] = 0.62667f;   lut2->luts[1][29] = 0.84643f;   lut2->luts[2][29] = 1.0f;
    lut2->luts[0][30] = 0.69154f;   lut2->luts[1][30] = 0.9213f;    lut2->luts[2][30] = 1.0f;
    lut2->luts[0][31] = 0.76038f;   lut2->luts[1][31] = 1.0f;       lut2->luts[2][31] = 1.0f;

    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW( 
        OCIO::CreateLut1DOp(ops, lut2, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);

    {
        const OCIO::Lut1DRcPtr lut = OCIO::Compose(lut1, ops[0], OCIO::COMPOSE_RESAMPLE_NO);

        OIIO_CHECK_EQUAL( lut->luts[0].size(), 2 );

        OIIO_CHECK_CLOSE( lut->luts[0][0], 0.00744791f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][0], 0.03172233f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][0], 0.07058375f, 1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][1], 0.3513808f,  1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][1], 0.51819527f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][1], 0.67463773f, 1e-6f );
    }
/*
    TODO: To uncomment when the Op bit depth will be implemented

    {
        const OCIO::Lut1DRcPtr lut = Compose(lut1, ops[0], OCIO::COMPOSE_RESAMPLE_INDEPTH);

        OIIO_CHECK_EQUAL( lut->luts[0].size(), 256 );

        OIIO_CHECK_CLOSE( lut->luts[0][0], 0.00744791f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][0], 0.03172233f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][0], 0.07058375f, 1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][127], 0.0f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][127], 0.0f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][127], 0.0f, 1e-6f );

        OIIO_CHECK_CLOSE( lut->luts[0][255], 0.3513808f,  1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[1][255], 0.51819527f, 1e-6f );
        OIIO_CHECK_CLOSE( lut->luts[2][255], 0.67463773f, 1e-6f );
    }
*/
}

#endif // OCIO_UNIT_TEST
