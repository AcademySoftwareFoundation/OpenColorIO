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


#include "OpDataTools.h"

#include "../MatrixOps.h"
#include "../Lut3DOp.h"
#include "../BitDepthUtils.h"
#include "../Op.h"


OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace OpData
{

float GetValueStepSize(BitDepth bitDepth, unsigned dimension)
{
    return GetBitDepthMaxValue(bitDepth) / ((float)dimension - 1.0f);
}

// Returns the ideal LUT size based on a specific bit depth
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

void EvalTransform(const float * in, 
                    float * out, 
                    unsigned numPixels, 
                    const OpRcPtrVec & ops)
{
    std::vector<float> tmp(numPixels*4);

    // Render the LUT entries (domain) through the ops.

    const float * values = in;
    for(unsigned idx=0; idx<numPixels; ++idx)
    {
        tmp[4*idx + 0] = values[0];
        tmp[4*idx + 1] = values[1];
        tmp[4*idx + 2] = values[2];
        tmp[4*idx + 3] = 1.0f;

        values += 3;
    }

    for(OpRcPtrVec::size_type i=0, size = ops.size(); i<size; ++i)
    {
        ops[i]->apply(&tmp[0], numPixels);
    }

    float * result = out;
    for(unsigned idx=0; idx<numPixels; ++idx)
    {
        result[0] = tmp[4*idx + 0];
        result[1] = tmp[4*idx + 1];
        result[2] = tmp[4*idx + 2];

        result += 3;
    }
}

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


// Calculate a new LUT by evaluating a new domain (A) through a set of ops (B).
//
// Note1: The caller must ensure that B is separable (channel independent).
//
// Note2: Unlike compose(Lut1DOp,Lut1DOp), this function does not try to resize
//        the first LUT (A), so the caller needs to create a suitable domain.
//
OpDataLut1DRcPtr Compose(const Lut1D & A, const OpDataVec & B)
{
    if (B.size() == 0)
    {
        throw Exception("There is nothing to compose the LUT with");
    }
    if (A.getOutputBitDepth() != B[0]->getInputBitDepth())
    {
        throw Exception("A bit depth mismatch forbids the composition of LUTs");
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

    const float iScale = 1.f / GetBitDepthMaxValue(A.getOutputBitDepth());
    const float iScale4[4] = { iScale, iScale, iScale, 1.0f };
    CreateScaleOp(ops, iScale4, TRANSFORM_DIR_FORWARD);

    // Copy and append B.
    for(unsigned i=0, size = B.size(); i<size; ++i)
    {
        CreateOpVecFromOpData(ops, *B[i], TRANSFORM_DIR_FORWARD);
    }

    // Insert an op to compensate for the bitdepth scaling of B.
    //
    // We render at 32f but need to create an array which may be inserted
    // into a LUT with B's output depth, so apply the scaling manually.
    // For the explanation of the bit-depths used, see the comment above.

    const float oScale = GetBitDepthMaxValue(B[B.size()-1]->getOutputBitDepth());
    const float oScale4[4] = { oScale, oScale, oScale, 1.0f };
    CreateScaleOp(ops, oScale4, TRANSFORM_DIR_FORWARD);

    // Create a LUT to hold the composed result.

    // TODO:  May want to revisit metadata propagation.
    Descriptions newDesc;
    newDesc += "LUT from composition";

    // If A has a half-domain, we want the result to have the same.
    const Lut1D::HalfFlags flags 
        = (Lut1D::HalfFlags) (
                        (A.getHalfFlags() &Lut1D::LUT_INPUT_HALF_CODE) );

    OpDataLut1DRcPtr result( new Lut1D(A.getInputBitDepth(),
                                        B[B.size()-1]->getOutputBitDepth(),
                                        "",
                                        "",
                                        newDesc,
                                        INTERP_LINEAR,
                                        flags) );

    // Set up so that the eval directly fills in the array of the result LUT.

    const Array::Values& inValues = A.getArray().getValues();
    const unsigned numPixels = A.getArray().getLength();

    result->getArray().resize(numPixels, 3);  // TODO: 1 or 3 based on A
    Array::Values& outValues = result->getArray().getValues();

    // Evaluate the transforms at 32f.
    // Note: If any ops are bypassed, that will be respected here.

    FinalizeOpVec(ops);
    EvalTransform((const float*)(&inValues[0]), 
                    (float*)(&outValues[0]),
                    numPixels,
                    ops);

    return result;
}

// Compose two Lut1DOps.
//
// Note1: If either LUT uses hue_adjust, composition will not give the
// same result as if they were applied sequentially.  However, we need
// to allow composition because the Lut1D CPU renderer needs it to
// built the lookup table for the hueAdjust renderer.  We could
// potentially do a lock object in that renderer to over-ride the
// hue adjust temporarily like in invLut1d. But for now, we put the
// burdon on the caller to use OpData::Lut1D::mayCompose first.
//
// Note2: Likewise ideally we would prohibit composition if
// hasMatchingBypass is false.  However, since the renderers may need
// to resample the LUTs, do not want to raise an exception or require
// the new domain to be dynamic. So again, it is up to the caller
// verify dynamic and bypass compatibility when calling this function
// in a more general context.
OpDataLut1DRcPtr Compose(const OpDataLut1DRcPtr & A, 
                            const OpDataLut1DRcPtr & B, 
                            ComposeMethod compFlag)
{
    if (A->getOutputBitDepth() != B->getInputBitDepth())
    {
        throw Exception("A bit depth mismatch forbids the composition of LUTs");
    }

    OpDataVec ops;

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
            resampleDepth = A->getInputBitDepth();
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

    const unsigned Asz = A->getArray().getLength();
    const bool goodDomain = A->isInputHalfDomain() || ( Asz >= min_size );
    const bool useOrigDomain = compFlag == COMPOSE_RESAMPLE_NO;

    Lut1D * domain(0x0);
    std::auto_ptr<Lut1D> tmpLut;

    if ( goodDomain || useOrigDomain )
    {
        // Use the original domain.
        domain = (Lut1D*) A.get();
    }
    else
    {
        // Create identity with finer domain.

        // TODO: Should not need to create a new LUT object for this.
        //       Perhaps add a utility function to be shared with the constructor.
        tmpLut.reset(new Lut1D(resampleDepth,
                                A->getInputBitDepth(),
                                A->getId(),
                                A->getName(),
                                A->getDescriptions(),
                                A->getInterpolation(),
                                // half case handled above
                                Lut1D::LUT_STANDARD));

        // Interpolate through both LUTs in this case (resample).
        ops.append(A->clone(OpData::DO_SHALLOW_COPY));

        domain = tmpLut.get();
    }

    // TODO:  Would like to not require a clone simply to prevent the delete
    //        from being called on the op when the opList goes out of scope.
    ops.append(B->clone(OpData::DO_SHALLOW_COPY));

    // Create the result LUT by composing the domain through the desired ops.

    OpDataLut1DRcPtr result = Compose(*domain, ops);

    // Configure the metadata of the result LUT.

    // TODO:  May want to revisit metadata propagation.
    result->setId(A->getId() + B->getId());
    result->setName(A->getName() + B->getName());
    Descriptions newDesc = A->getDescriptions();
    newDesc += B->getDescriptions();
    result->getDescriptions() += newDesc;

    // See note above: Taking these from B since the common use case is for B to be 
    // the original LUT and A to be a new domain (e.g. used in LUT1D renderers).
    // TODO: Adjust domain in Lut1D renderer to be one channel.
    result->setHueAdjust( B->getHueAdjust() );

    // TODO: Per comment above, want to assert here but need to render
    // dynamic bypassed ops and the renderer uses compose to build a LUT
    // with the correct number of entries for the incoming bit depth.
    //if ( A.isBypassed() != B.isBypassed() )
    //{
    //  ASSERT0(RENDERER, ALL, DIFFERENT_BYPASS_STATES);
    //}

    // NB: This also copies whether Dynamic is true/false.

    // TODO: Uncomment when dynamic propertiess are in
    //result->setBypass( B.getBypass() );

    return result;
}

OpDataLut3DRcPtr Compose(const OpDataLut3DRcPtr & A,
                            const OpDataLut3DRcPtr & B)
{
    // TODO: Composition of LUTs is a potentially lossy operation.
    // We try to be safe by making the result at least as big as either A or B
    // but we may want to even increase the resolution further.  However,
    // currently composition is done pairs at a time and we would want to
    // determine the increase size once at the start rather than bumping it up
    // as each pair is done.

    if (A->getOutputBitDepth() != B->getInputBitDepth())
    {
        throw Exception("A bit depth mismatch forbids the composition of LUTs");
    }

    const unsigned min_sz = B->getArray().getLength();
    unsigned n = A->getArray().getLength();
    OpRcPtrVec ops;

    Lut3D *domain(0);
    std::auto_ptr<Lut3D> tmpLut;

    if (n >= min_sz)
    {
        // The range of the first LUT becomes the domain to interp in the second.

        const float iScale = 1.f / GetBitDepthMaxValue(A->getOutputBitDepth());
        const float iScale4[4] = { iScale, iScale, iScale, 1.0f };
        CreateScaleOp(ops, iScale4, TRANSFORM_DIR_FORWARD);

        // Use the original domain.
        domain = A.get();
    }
    else
    {
        // Since the 2nd LUT is more finely sampled, use its grid size.

        // Create identity with finer domain.

        // TODO: Should not need to create a new LUT object for this.
        //       Perhaps add a utility function to be shared with the constructor.
        tmpLut.reset(new Lut3D(A->getInputBitDepth(),
                                BIT_DEPTH_F32,
                                A->getId(),
                                A->getName(),
                                A->getDescriptions(),
                                A->getInterpolation(),
                                min_sz));

        // Interpolate through both LUTs in this case (resample).
        OpDataLut3DRcPtr clonedA(dynamic_cast<Lut3D*>(A->clone(OpData::DO_SHALLOW_COPY)));
        CreateLut3DOp(ops, clonedA, TRANSFORM_DIR_FORWARD);
        domain = tmpLut.get();
    }

    // TODO: Would like to not require a clone simply to prevent the delete
    //       from being called on the op when the opList goes out of scope.
    OpDataLut3DRcPtr clonedB(dynamic_cast<Lut3D*>(B->clone(OpData::DO_SHALLOW_COPY)));
    CreateLut3DOp(ops, clonedB, TRANSFORM_DIR_FORWARD);

    const float iScale = GetBitDepthMaxValue(B->getOutputBitDepth());

    const float iScale4[4] = { iScale, iScale, iScale, 1.0f };
    CreateScaleOp(ops, iScale4, TRANSFORM_DIR_FORWARD);

    Descriptions newDesc = A->getDescriptions();
    newDesc += B->getDescriptions();
    // TODO: May want to revisit metadata propagation.
    OpDataLut3DRcPtr result(new Lut3D(A->getInputBitDepth(),
                                        B->getOutputBitDepth(),
                                        A->getId() + B->getId(),
                                        A->getName() + B->getName(),
                                        newDesc,
                                        A->getInterpolation(),
                                        2));  // we replace it anyway

    const Array::Values& inValues = domain->getArray().getValues();
    const unsigned gridSize = domain->getArray().getLength();
    const unsigned numPixels = gridSize * gridSize * gridSize;

    result->getArray().resize(gridSize, 3);
    Array::Values& outValues = result->getArray().getValues();

    FinalizeOpVec(ops);
    EvalTransform((const float*)(&inValues[0]),
                    (float*)(&outValues[0]),
                    numPixels,
                    ops);

    // TODO: invLutUtil.cpp:makeFastLut3D needs to use compose and it seems
    // possible to have a dynamic and bypassed op that would not be optimized
    // out and so the renderer would need to call compose.  Correct?
    // Perhaps make this a log message rather than risk crashing an app.
    //if ( A.isBypassed() != B.isBypassed() )
    //{
    //  ASSERT0(RENDERER, ALL, DIFFERENT_BYPASS_STATES);
    //}
    // NB: This also copies whether Dynamic is true/false.

    // TODO: Uncomment when dynamic propertiess are in
    // result->setBypass(B.getBypass());

    return result;
}
}
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

#include <cstring>

namespace OCIO = OCIO_NAMESPACE;
#include "../UnitTest.h"

OIIO_ADD_TEST(OpTools, Lut1D_compose)
{

    OCIO::OpData::OpDataLut1DRcPtr 
        lut1(new OCIO::OpData::Lut1D(
            OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, OCIO::OpData::Lut1D::LUT_STANDARD));

    lut1->getArray().resize(8, 3);
    float * values = &lut1->getArray().getValues()[0];

    values[ 0] = 0.0f;      values[ 1] = 0.0f;      values[ 2] = 0.002333f;
    values[ 3] = 0.0f;      values[ 4] = 0.291341f; values[ 5] = 0.015624f;
    values[ 6] = 0.106521f; values[ 7] = 0.334331f; values[ 8] = 0.462431f;
    values[ 9] = 0.515851f; values[10] = 0.474151f; values[11] = 0.624611f;
    values[12] = 0.658791f; values[13] = 0.527381f; values[14] = 0.685071f;
    values[15] = 0.908501f; values[16] = 0.707951f; values[17] = 0.886331f;
    values[18] = 0.926671f; values[19] = 0.846431f; values[20] = 1.0f;
    values[21] = 1.0f;      values[22] = 1.0f;      values[23] = 1.0f;

    OCIO::OpData::OpDataLut1DRcPtr
        lut2(new OCIO::OpData::Lut1D(
            OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, OCIO::OpData::Lut1D::LUT_STANDARD));

    lut2->getArray().resize(8, 3);
    values = &lut2->getArray().getValues()[0];

    values[ 0] = 0.0f;        values[ 1] = 0.0f;       values[ 2] = 0.0023303f;
    values[ 3] = 0.0f;        values[ 4] = 0.0029134f; values[ 5] = 0.015624f;
    values[ 6] = 0.00010081f; values[ 7] = 0.0059806f; values[ 8] = 0.023362f;
    values[ 9] = 0.0045628f;  values[10] = 0.024229f;  values[11] = 0.05822f;
    values[12] = 0.0082598f;  values[13] = 0.033831f;  values[14] = 0.074063f;
    values[15] = 0.028595f;   values[16] = 0.075003f;  values[17] = 0.13552f;
    values[18] = 0.69154f;    values[19] = 0.9213f;    values[20] = 1.0f;
    values[21] = 0.76038f;    values[22] = 1.0f;       values[23] = 1.0f;

    {
        const OCIO::OpData::OpDataLut1DRcPtr lut
            = OCIO::OpData::Compose(lut1, lut2, OCIO::OpData::COMPOSE_RESAMPLE_NO);

        values = &lut->getArray().getValues()[0];

        OIIO_CHECK_EQUAL( lut->getArray().getLength(), 8 );

        OIIO_CHECK_CLOSE( values[0], 0.0f,           1e-6f );
        OIIO_CHECK_CLOSE( values[1], 0.0f,           1e-6f );
        OIIO_CHECK_CLOSE( values[2], 0.00254739914f, 1e-6f );

        OIIO_CHECK_CLOSE( values[3], 0.0f,           1e-6f );
        OIIO_CHECK_CLOSE( values[4], 0.00669934973f, 1e-6f );
        OIIO_CHECK_CLOSE( values[5], 0.00378420483f, 1e-6f );

        OIIO_CHECK_CLOSE( values[6], 0.0f,           1e-6f );
        OIIO_CHECK_CLOSE( values[7], 0.0121908365f,  1e-6f );
        OIIO_CHECK_CLOSE( values[8], 0.0619750582f,  1e-6f );

        OIIO_CHECK_CLOSE( values[ 9], 0.00682150759f, 1e-6f );
        OIIO_CHECK_CLOSE( values[10], 0.0272925831f,  1e-6f );
        OIIO_CHECK_CLOSE( values[11], 0.096942015f,   1e-6f );

        OIIO_CHECK_CLOSE( values[12], 0.0206955168f,  1e-6f );
        OIIO_CHECK_CLOSE( values[13], 0.0308703855f,  1e-6f );
        OIIO_CHECK_CLOSE( values[14], 0.12295182f,    1e-6f );

        OIIO_CHECK_CLOSE( values[15], 0.716288447f,   1e-6f );
        OIIO_CHECK_CLOSE( values[16], 0.0731772855f,  1e-6f );
        OIIO_CHECK_CLOSE( values[17], 1.0f,           1e-6f );

        OIIO_CHECK_CLOSE( values[18], 0.725044191f,   1e-6f );
        OIIO_CHECK_CLOSE( values[19], 0.857842028f,   1e-6f );
        OIIO_CHECK_CLOSE( values[20], 1.0f,           1e-6f );
    }

    {
        const OCIO::OpData::OpDataLut1DRcPtr lut
            = OCIO::OpData::Compose(lut1, lut2, OCIO::OpData::COMPOSE_RESAMPLE_INDEPTH);

        values = &lut->getArray().getValues()[0];

        OIIO_CHECK_EQUAL( lut->getArray().getLength(), 65536 );

        OIIO_CHECK_CLOSE( values[0], 0.0f,           1e-6f );
        OIIO_CHECK_CLOSE( values[1], 0.0f,           1e-6f );
        OIIO_CHECK_CLOSE( values[2], 0.00254739914f, 1e-6f );

        OIIO_CHECK_CLOSE( values[3], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( values[4], 6.34463504e-07f, 1e-6f );
        OIIO_CHECK_CLOSE( values[5], 0.00254753046f,  1e-6f );

        OIIO_CHECK_CLOSE( values[6], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( values[7], 1.26915984e-06f, 1e-6f );
        OIIO_CHECK_CLOSE( values[8], 0.00254766271f,  1e-6f );

        OIIO_CHECK_CLOSE( values[ 9], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( values[10], 1.90362334e-06f, 1e-6f );
        OIIO_CHECK_CLOSE( values[11], 0.00254779495f,  1e-6f );

        OIIO_CHECK_CLOSE( values[12], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( values[13], 2.53855251e-06f, 1e-6f );
        OIIO_CHECK_CLOSE( values[14], 0.0025479272f,   1e-6f );

        OIIO_CHECK_CLOSE( values[15], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( values[16], 3.17324884e-06f, 1e-6f );
        OIIO_CHECK_CLOSE( values[17], 0.00254805945f,  1e-6f );

        OIIO_CHECK_CLOSE( values[300], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( values[301], 6.3463347e-05f,  1e-6f );
        OIIO_CHECK_CLOSE( values[302], 0.00256060902f,  1e-6f );

        OIIO_CHECK_CLOSE( values[900], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( values[901], 0.000190390972f, 1e-6f );
        OIIO_CHECK_CLOSE( values[902], 0.00258703064f,  1e-6f );

        OIIO_CHECK_CLOSE( values[2700], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( values[2701], 0.000571172219f, 1e-6f );
        OIIO_CHECK_CLOSE( values[2702], 0.00266629551f,  1e-6f );
    }
}

// TODO: Missing composition unit tests from transformTools_test.cpp
// Compose tests are in Lut1dOp.cpp & Lut3dOp.cpp

#endif // OCIO_UNIT_TEST
