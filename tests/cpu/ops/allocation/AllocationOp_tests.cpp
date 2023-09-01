// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/allocation/AllocationOp.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(AllocationOps, create)
{
    OCIO::OpRcPtrVec ops;
    OCIO::AllocationData allocData;
    allocData.allocation = OCIO::ALLOCATION_UNKNOWN;
    OCIO_CHECK_THROW_WHAT(
        OCIO::CreateAllocationOps(ops, allocData, OCIO::TRANSFORM_DIR_FORWARD),
                                  OCIO::Exception, "Unsupported Allocation Type");
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_THROW_WHAT(
        OCIO::CreateAllocationOps(ops, allocData, OCIO::TRANSFORM_DIR_INVERSE),
                                  OCIO::Exception, "Unsupported Allocation Type");
    OCIO_CHECK_EQUAL(ops.size(), 0);

    allocData.allocation = OCIO::ALLOCATION_UNIFORM;
    // No allocation data leads to identity, identity transform will be created.
    OCIO_CHECK_NO_THROW(
        OCIO::CreateAllocationOps(ops, allocData, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    ops.clear();
    OCIO_CHECK_NO_THROW(
        OCIO::CreateAllocationOps(ops, allocData, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);

    // adding data to avoid identity. Fit transform will be created (if valid).
    allocData.vars.push_back(0.0f);
    allocData.vars.push_back(10.0f);
    ops.clear();
    OCIO_CHECK_NO_THROW(
        OCIO::CreateAllocationOps(ops, allocData, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    const OCIO::OpRcPtr forwardFitOp = ops[0];
    ops.clear();
    OCIO_CHECK_NO_THROW(
        OCIO::CreateAllocationOps(ops, allocData, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    ops.clear();

    allocData.allocation = OCIO::ALLOCATION_LG2;

    // default is not identity
    allocData.vars.clear();
    OCIO_CHECK_NO_THROW(
        OCIO::CreateAllocationOps(ops, allocData, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op1 = ops[1];
    // second op is a fit transform
    OCIO_CHECK_EQUAL(forwardFitOp->isSameType(op1), true);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr defaultLogOp = ops[0];
    OCIO::ConstOpRcPtr defaultFitOp = ops[1];

#if OCIO_USE_SSE2 == 0
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
        OCIO::CreateAllocationOps(ops, allocData, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    op1 = ops[1];
    OCIO_CHECK_EQUAL(defaultLogOp->isInverse(op1), true);

    ops.clear();

    // adding data to target identity, Log op and identity are created
    allocData.vars.push_back(0.0f);
    allocData.vars.push_back(1.0f);

    OCIO_CHECK_NO_THROW(
        OCIO::CreateAllocationOps(ops, allocData, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    // Identity is removed.
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO_CHECK_EQUAL(defaultLogOp->isSameType(op0), true);
    ops.clear();
    OCIO_CHECK_NO_THROW(
        OCIO::CreateAllocationOps(ops, allocData, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    // Identity is removed.
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    op0 = ops[0];
    OCIO_CHECK_EQUAL(defaultLogOp->isSameType(op0), true);
    ops.clear();

    // change log intercept
    allocData.vars.push_back(10.0f);
    OCIO_CHECK_NO_THROW(
        OCIO::CreateAllocationOps(ops, allocData, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

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
