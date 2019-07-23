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

#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Allocation/AllocationOp.h"
#include "ops/Log/LogOps.h"
#include "ops/Matrix/MatrixOps.h"
#include "Op.h"

OCIO_NAMESPACE_ENTER
{

    void CreateAllocationOps(OpRcPtrVec & ops,
                             const AllocationData & data,
                             TransformDirection dir)
    {
        if(data.allocation == ALLOCATION_UNIFORM)
        {
            float oldmin[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            float oldmax[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            float newmin[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            float newmax[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            
            if(data.vars.size() >= 2)
            {
                for(int i=0; i<3; ++i)
                {
                    oldmin[i] = data.vars[0];
                    oldmax[i] = data.vars[1];
                }
            }
            
            CreateFitOp(ops,
                        oldmin, oldmax,
                        newmin, newmax,
                        dir);
        }
        else if(data.allocation == ALLOCATION_LG2)
        {
            float oldmin[4] = { -10.0f, -10.0f, -10.0f, 0.0f };
            float oldmax[4] = { 6.0f, 6.0f, 6.0f, 1.0f };
            float newmin[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            float newmax[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            
            if(data.vars.size() >= 2)
            {
                for(int i=0; i<3; ++i)
                {
                    oldmin[i] = data.vars[0];
                    oldmax[i] = data.vars[1];
                }
            }
            
            
            // Log Settings.
            // output = logSlope * log( linSlope * input + linOffset, base ) + logOffset
            
            double base = 2.0;
            double logSlope[3]  = { 1.0, 1.0, 1.0 };
            double linSlope[3]  = { 1.0, 1.0, 1.0 };
            double linOffset[3] = { 0.0, 0.0, 0.0 };
            double logOffset[3] = { 0.0, 0.0, 0.0 };
            
            if(data.vars.size() >= 3)
            {
                for(int i=0; i<3; ++i)
                {
                    linOffset[i] = data.vars[2];
                }
            }
            
            if(dir == TRANSFORM_DIR_FORWARD)
            {
                CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset, dir);
                
                CreateFitOp(ops,
                            oldmin, oldmax,
                            newmin, newmax,
                            dir);
            }
            else if(dir == TRANSFORM_DIR_INVERSE)
            {
                CreateFitOp(ops,
                            oldmin, oldmax,
                            newmin, newmax,
                            dir);
                
                CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset, dir);
            }
            else
            {
                throw Exception("Cannot BuildAllocationOps, unspecified transform direction.");
            }
        }
        else
        {
            throw Exception("Unsupported Allocation Type.");
        }
    }
    
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

OCIO_NAMESPACE_USING

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OCIO_ADD_TEST(AllocationOps, Create)
{
    OpRcPtrVec ops;
    AllocationData allocData;
    allocData.allocation = ALLOCATION_UNKNOWN;
    OCIO_CHECK_THROW_WHAT(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_FORWARD),
                            OCIO::Exception, "Unsupported Allocation Type");
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_THROW_WHAT(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_INVERSE),
                            OCIO::Exception, "Unsupported Allocation Type");
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_THROW_WHAT(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_UNKNOWN),
                            OCIO::Exception, "Unsupported Allocation Type");
    OCIO_CHECK_EQUAL(ops.size(), 0);

    allocData.allocation = ALLOCATION_UNIFORM;
    // No allocation data leads to identity, not transform will be created
    OCIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_UNKNOWN));
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    // adding data to avoid identity. Fit transform will be created (if valid).
    allocData.vars.push_back(0.0f);
    allocData.vars.push_back(10.0f);
    OCIO_CHECK_THROW_WHAT(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_UNKNOWN),
                            OCIO::Exception, "unspecified transform direction");
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    const OpRcPtr forwardFitOp = ops[0];
    ops.clear();
    OCIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO_CHECK_EQUAL(forwardFitOp->isInverse(op0), true);
    ops.clear();

    allocData.allocation = ALLOCATION_LG2;

    // default is not identity
    allocData.vars.clear();
    OCIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op1 = ops[1];
    // second op is a fit transform
    OCIO_CHECK_EQUAL(forwardFitOp->isSameType(op1), true);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    ConstOpRcPtr defaultLogOp = ops[0];
    ConstOpRcPtr defaultFitOp = ops[1];

#ifndef USE_SSE
    const float error = 1e-6f;
#else
    const float error = 2e-5f;
#endif
    const unsigned NB_PIXELS = 3;
    const float src[NB_PIXELS * 4] = {
         0.16f,  0.2f,  0.3f,   0.4f,
        -0.16f, -0.2f, 32.0f, 123.4f,
         1.0f,  1.0f,  1.0f,   1.0f };

    const float dstLog[NB_PIXELS * 4] = {
        -2.64385629f,  -2.32192802f,  -1.73696554f,   0.4f,
        -126.0f, -126.0f, 5.0f, 123.4f,
        0.0f,  0.0f,  0.0f,   1.0f };

    const float dstFit[NB_PIXELS * 4] = {
        0.635f,  0.6375f, 0.64375f, 0.4f,
        0.615f,  0.6125f, 2.625f, 123.4f,
        0.6875f, 0.6875f, 0.6875f,  1.0f };

    float tmp[NB_PIXELS * 4];
    memcpy(tmp, &src[0], 4 * NB_PIXELS * sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for (unsigned idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OCIO_CHECK_CLOSE(dstLog[idx], tmp[idx], error);
    }

    memcpy(tmp, &src[0], 4 * NB_PIXELS * sizeof(float));

    ops[1]->apply(tmp, NB_PIXELS);

    for (unsigned idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OCIO_CHECK_CLOSE(dstFit[idx], tmp[idx], error);
    }

    ops.clear();

    OCIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    op0 = ops[0];
    op1 = ops[1];
    OCIO_CHECK_EQUAL(defaultFitOp->isInverse(op0), true);
    OCIO_CHECK_EQUAL(defaultLogOp->isInverse(op1), true);

    ops.clear();
    OCIO_CHECK_THROW_WHAT(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_UNKNOWN),
                            OCIO::Exception, "unspecified transform direction");
    OCIO_CHECK_EQUAL(ops.size(), 0);

    // adding data to target identity, only Log op is created (if valid)
    allocData.vars.push_back(0.0f);
    allocData.vars.push_back(1.0f);

    OCIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    op0 = ops[0];
    OCIO_CHECK_EQUAL(defaultLogOp->isSameType(op0), true);
    ops.clear();
    OCIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    op0 = ops[0];
    OCIO_CHECK_EQUAL(defaultLogOp->isSameType(op0), true);
    ops.clear();
    OCIO_CHECK_THROW_WHAT(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_UNKNOWN),
                            OCIO::Exception, "unspecified transform direction");
    OCIO_CHECK_EQUAL(ops.size(), 0);

    // change log intercept
    allocData.vars.push_back(10.0f);
    OCIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    ops[0]->finalize(OCIO::FINALIZATION_EXACT);

    memcpy(tmp, &src[0], 4 * NB_PIXELS * sizeof(float));

    const float dstLogShift[NB_PIXELS * 4] = {
        3.34482837f, 3.35049725f, 3.36457253f, 0.4f,
        3.29865813f, 3.29278183f, 5.39231730f, 123.4f,
        3.45943165f, 3.45943165f, 3.45943165f, 1.0f };

    ops[0]->apply(tmp, NB_PIXELS);

    for (unsigned idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OCIO_CHECK_CLOSE(dstLogShift[idx], tmp[idx], error);
    }

}

#endif