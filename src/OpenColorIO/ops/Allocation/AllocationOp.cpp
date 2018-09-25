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

#include "ops/Allocation/AllocationOp.h"
#include "ops/Log/LogOps.h"
#include "ops/Matrix/MatrixOps.h"
#include "Op.h"

#include <OpenColorIO/OpenColorIO.h>
#include <string.h>

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
            
            
            // Log Settings
            // output = k * log(mx+b, base) + kb
            
            float k[3] = { 1.0f, 1.0f, 1.0f };
            float m[3] = { 1.0f, 1.0f, 1.0f };
            float b[3] = { 0.0f, 0.0f, 0.0f };
            float base[3] = { 2.0f, 2.0f, 2.0f };
            float kb[3] = { 0.0f, 0.0f, 0.0f };
            
            if(data.vars.size() >= 3)
            {
                for(int i=0; i<3; ++i)
                {
                    b[i] = data.vars[2];
                }
            }
            
            if(dir == TRANSFORM_DIR_FORWARD)
            {
                CreateLogOp(ops, k, m, b, base, kb, dir);
                
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
                
                CreateLogOp(ops, k, m, b, base, kb, dir);
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
#include "unittest.h"

OIIO_ADD_TEST(AllocationOps, Create)
{
    OpRcPtrVec ops;
    AllocationData allocData;
    allocData.allocation = ALLOCATION_UNKNOWN;
    OIIO_CHECK_THROW_WHAT(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_FORWARD),
        OCIO::Exception, "Unsupported Allocation Type");
    OIIO_CHECK_EQUAL(ops.size(), 0);
    OIIO_CHECK_THROW_WHAT(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_INVERSE),
        OCIO::Exception, "Unsupported Allocation Type");
    OIIO_CHECK_EQUAL(ops.size(), 0);
    OIIO_CHECK_THROW_WHAT(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "Unsupported Allocation Type");
    OIIO_CHECK_EQUAL(ops.size(), 0);

    allocData.allocation = ALLOCATION_UNIFORM;
    // No allocation data leads to identity, not transform will be created
    OIIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_UNKNOWN));
    OIIO_CHECK_EQUAL(ops.size(), 0);
    OIIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 0);
    OIIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 0);

    // adding data to avoid identity. Fit transform will be created (if valid).
    allocData.vars.push_back(0.0f);
    allocData.vars.push_back(10.0f);
    OIIO_CHECK_THROW_WHAT(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "unspecified transform direction");
    OIIO_CHECK_EQUAL(ops.size(), 0);
    OIIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    const OpRcPtr forwardFitOp = ops[0];
    ops.clear();
    OIIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(forwardFitOp->isInverse(ops[0]), true);
    ops.clear();

    allocData.allocation = ALLOCATION_LG2;

    // default is not identity
    allocData.vars.clear();
    OIIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 2);
    // second op is a fit transform
    OIIO_CHECK_EQUAL(forwardFitOp->isSameType(ops[1]), true);
    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops));
    OIIO_CHECK_EQUAL(ops.size(), 2);
    const OpRcPtr defaultLogOp = ops[0];
    const OpRcPtr defaultFitOp = ops[1];

    const float error = 1e-6f;
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
        OIIO_CHECK_CLOSE(dstLog[idx], tmp[idx], error);
    }

    memcpy(tmp, &src[0], 4 * NB_PIXELS * sizeof(float));

    ops[1]->apply(tmp, NB_PIXELS);

    for (unsigned idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OIIO_CHECK_CLOSE(dstFit[idx], tmp[idx], error);
    }

    ops.clear();

    OIIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OIIO_CHECK_EQUAL(defaultFitOp->isInverse(ops[0]), true);
    OIIO_CHECK_EQUAL(defaultLogOp->isInverse(ops[1]), true);

    ops.clear();
    OIIO_CHECK_THROW_WHAT(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "unspecified transform direction");
    OIIO_CHECK_EQUAL(ops.size(), 0);

    // adding data to target identity, only Log op is created (if valid)
    allocData.vars.push_back(0.0f);
    allocData.vars.push_back(1.0f);

    OIIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(defaultLogOp->isSameType(ops[0]), true);
    ops.clear();
    OIIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(defaultLogOp->isSameType(ops[0]), true);
    ops.clear();
    OIIO_CHECK_THROW_WHAT(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "unspecified transform direction");
    OIIO_CHECK_EQUAL(ops.size(), 0);

    // change log intercept
    allocData.vars.push_back(10.0f);
    OIIO_CHECK_NO_THROW(
        CreateAllocationOps(ops, allocData, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    ops[0]->finalize();

    memcpy(tmp, &src[0], 4 * NB_PIXELS * sizeof(float));

    const float dstLogShift[NB_PIXELS * 4] = {
        3.34482837f, 3.35049725f, 3.36457253f, 0.4f,
        3.29865813f, 3.29278183f, 5.39231730f, 123.4f,
        3.45943165f, 3.45943165f, 3.45943165f, 1.0f };

    ops[0]->apply(tmp, NB_PIXELS);

    for (unsigned idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OIIO_CHECK_CLOSE(dstLogShift[idx], tmp[idx], error);
    }

}

#endif