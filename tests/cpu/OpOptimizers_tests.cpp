// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "OpOptimizers.cpp"

#include "ops/cdl/CDLOp.h"
#include "ops/exponent/ExponentOp.h"
#include "ops/exposurecontrast/ExposureContrastOp.h"
#include "ops/fixedfunction/FixedFunctionOp.h"
#include "ops/gamma/GammaOp.h"
#include "ops/log/LogOp.h"
#include "ops/matrix/MatrixOp.h"
#include "ops/range/RangeOp.h"
#include "testutils/UnitTest.h"
#include "transforms/FileTransform.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
OCIO::OptimizationFlags AllBut(OCIO::OptimizationFlags notFlag)
{
    return static_cast<OCIO::OptimizationFlags>(OCIO::OPTIMIZATION_ALL & ~notFlag);
}

void CompareRender(OCIO::OpRcPtrVec & ops1, OCIO::OpRcPtrVec & ops2,
                   unsigned line, float errorThreshold,
                   bool forceAlphaInRange = false)
{
    std::vector<float> img1 = {
        0.778f,  0.824f,   0.885f,  0.153f,
        0.044f,  0.014f,   0.088f,  0.999f,
        0.488f,  0.381f,   0.f,     0.f,
        1.000f,  1.52e-4f, 0.0229f, 1.f,
        0.f,    -0.1f,    -2.f,    -0.1f,
        2.f,     1.9f,     0.f,     2.f };

    if (forceAlphaInRange)
    {
        img1[19] = 0.f;
    }

    std::vector<float> img2 = img1;

    const long nbPixels = (long)img1.size() / 4;

    for (const auto & op : ops1)
    {
        // NB: This hard-codes OPTIMIZATION_FAST_LOG_EXP_POW to off, see Op.h.
        op->apply(&img1[0], &img1[0], nbPixels);
    }

    for (const auto & op : ops2)
    {
        op->apply(&img2[0], &img2[0], nbPixels);
    }

    for (size_t idx = 0; idx < img1.size(); ++idx)
    {
        OCIO_CHECK_CLOSE_FROM(img1[idx], img2[idx], errorThreshold, line);
    }
}

} // namespace

OCIO_ADD_TEST(OpOptimizers, remove_leading_clamp_identity)
{
    OCIO::OpRcPtrVec ops;

    auto range = std::make_shared<OCIO::RangeOpData>(0., 1., 0., 1.);
    auto range2 = std::make_shared<OCIO::RangeOpData>(0., 1., 0., 2.);
    auto matrix = std::make_shared<OCIO::MatrixOpData>();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveLeadingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr o0 = ops[0];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::MatrixType);
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 3);
    OCIO::RemoveLeadingClampIdentity(ops);
    OCIO_CHECK_EQUAL(ops.size(), 0);
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveLeadingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    o0 = ops[0];
    OCIO::ConstOpRcPtr o1 = ops[1];
    OCIO::ConstOpRcPtr o2 = ops[2];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::MatrixType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o2->data()->getType(), OCIO::OpData::RangeType);

    ops.clear();

    // First range is not an identity, nothing to remove.
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveLeadingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    o0 = ops[0];
    o1 = ops[1];
    o2 = ops[2];
    OCIO::ConstOpRcPtr o3 = ops[3];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::MatrixType);
    OCIO_CHECK_EQUAL(o2->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o3->data()->getType(), OCIO::OpData::RangeType);
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveLeadingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    o0 = ops[0];
    o1 = ops[1];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::MatrixType);
    ops.clear();
}

OCIO_ADD_TEST(OpOptimizers, remove_trailing_clamp_identity)
{
    OCIO::OpRcPtrVec ops;

    auto range = std::make_shared<OCIO::RangeOpData>(0., 1., 0., 1.);
    auto range2 = std::make_shared<OCIO::RangeOpData>(0., 1., 0., 2.);
    auto matrix = std::make_shared<OCIO::MatrixOpData>();

    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveTrailingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr o0 = ops[0];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::MatrixType);
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 3);
    OCIO::RemoveTrailingClampIdentity(ops);
    OCIO_CHECK_EQUAL(ops.size(), 0);
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveTrailingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    o0 = ops[0];
    OCIO::ConstOpRcPtr o1 = ops[1];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::MatrixType);
    ops.clear();

    // Last range is not an identity, nothing to remove.
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveTrailingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    o0 = ops[0];
    o1 = ops[1];
    OCIO::ConstOpRcPtr o2 = ops[2];
    OCIO::ConstOpRcPtr o3 = ops[3];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::MatrixType);
    OCIO_CHECK_EQUAL(o2->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o3->data()->getType(), OCIO::OpData::RangeType);
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveTrailingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    o0 = ops[0];
    o1 = ops[1];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::MatrixType);
    ops.clear();
}

OCIO_ADD_TEST(OpOptimizers, remove_inverse_ops)
{
    OCIO::OpRcPtrVec ops;

    auto func = std::make_shared<OCIO::FixedFunctionOpData>(
        OCIO::FixedFunctionOpData::ACES_RED_MOD_03_FWD);

    const double logSlope[3]  = {0.18, 0.18, 0.18};
    const double linSlope[3]  = {2.0, 2.0, 2.0};
    const double linOffset[3] = {0.1, 0.1, 0.1};
    const double base         = 10.0;
    const double logOffset[3] = {1.0, 1.0, 1.0};

    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_INVERSE);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(ops.size(), 4);

    // Inverse + forward log are optimized as no-op then forward and inverse exponent are
    // optimized as no-op within the same call.
    OCIO::RemoveInverseOps(ops, OCIO::OPTIMIZATION_ALL);
    OCIO_CHECK_EQUAL(ops.size(), 0);
    ops.clear();

    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_INVERSE);
    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(ops.size(), 4);

    // Forward + inverse log are optimized as a clamping range that stays between 
    // forward and inverse exponents.
    OCIO::RemoveInverseOps(ops, OCIO::OPTIMIZATION_ALL);
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<FixedFunctionOp>");
    OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<RangeOp>");
    OCIO_CHECK_EQUAL(ops[2]->getInfo(), "<FixedFunctionOp>");
    ops.clear();

    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_INVERSE);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(ops.size(), 5);

    OCIO::RemoveInverseOps(ops, OCIO::OPTIMIZATION_ALL);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<FixedFunctionOp>");
}

OCIO_ADD_TEST(OpOptimizers, combine_ops)
{
    double m1[4] = {2.0, 2.0, 2.0, 1.0};
    double m2[4] = {0.5, 0.5, 0.5, 1.0};
    double m3[4] = {0.6, 0.6, 0.6, 1.0};
    double m4[4] = {0.7, 0.7, 0.7, 1.0};

    const double exp[4] = {1.2, 1.3, 1.4, 1.5};

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 1);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 1);
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m3, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::CombineOps(ops, AllBut(OCIO::OPTIMIZATION_COMP_MATRIX));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 1);
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m3, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m4, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 3);
        OCIO::CombineOps(ops, AllBut(OCIO::OPTIMIZATION_COMP_MATRIX));
        OCIO_CHECK_EQUAL(ops.size(), 3);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        // CombineOps removes at most one pair on each call, repeat to combine all pairs.
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 1);
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m2, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::CombineOps(ops, AllBut(OCIO::OPTIMIZATION_COMP_MATRIX));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 0);
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_INVERSE);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::CombineOps(ops, AllBut(OCIO::OPTIMIZATION_COMP_MATRIX));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 0);
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 5);
        OCIO::CombineOps(ops, AllBut(OCIO::OPTIMIZATION_COMP_MATRIX));
        OCIO_CHECK_EQUAL(ops.size(), 5);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        // CombineOps removes at most one pair on each call, repeat to combine all pairs.
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 1);
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateExponentOp(ops, exp, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m2, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateExponentOp(ops, exp, OCIO::TRANSFORM_DIR_INVERSE);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_EQUAL(ops.size(), 4);
        OCIO::CombineOps(ops, AllBut(OCIO::OPTIMIZATION_COMP_MATRIX));
        OCIO_CHECK_EQUAL(ops.size(), 4);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        // CombineOps removes at most one pair on each call, repeat to combine all pairs.
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 0);
    }
}

OCIO_ADD_TEST(OpOptimizers, prefer_pair_inverse_over_combine)
{
    // When a pair of forward / inverse LUTs with non 0 to 1 domain are used
    // as process space for a Look (eg. CDL), the Optimizer tries to combine
    // them when the Look results in a no-op. Here we make sure this result
    // in an appropriate clamp instead of a new half-domain LUT resulting
    // from the naive composition of the two LUTs.

    OCIO::OpRcPtrVec ops;

    // This spi1d uses "From -1.0 2.0", so the forward direction would become a 
    // Matrix to do the scaling followed by a Lut1D, and the inverse is a Lut1D
    // followed by a Matrix. Note that although the matrices compose into an identity,
    // they are both forward direction and *not* pair inverses of each other.
    const std::string fileName("lut1d_4.spi1d");
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_INVERSE));

    // The default negativeStyle is basicPassThruFwd, hence this op will be 
    // removed as a no-op on the first optimization pass.
    const double exp_null[4] = {1.0, 1.0, 1.0, 1.0};
    OCIO::CreateExponentOp(ops, exp_null, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_ALL));
    // The starting list of ops is this:
    //     FileNoOp --> Lut1D --> Matrix --> Gamma --> FileNoOp --> Matrix --> Lut1D
    // This becomes the following on the first pass after no-ops are removed:
    //     Lut1D --> Matrix --> Matrix --> Lut1D
    // The matrices are combined and removed on the first pass, leaving this:
    //     Lut1D --> Lut1D
    // Second pass: the LUTs are identified as a pair of inverses and replaced with a Range:
    //     Range

    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr op = ops[0];
    OCIO_REQUIRE_ASSERT(op);
    auto range = OCIO_DYNAMIC_POINTER_CAST<const OCIO::RangeOpData>(op->data());
    OCIO_REQUIRE_ASSERT(range);
}

OCIO_ADD_TEST(OpOptimizers, non_optimizable)
{
    OCIO::OpRcPtrVec ops;
    // Create non identity Matrix.
    const double m44[16] = { 2., 0., 0., 0.,
                             0., 1., 0., 0.,
                             0., 0., 1., 0.,
                             0., 0., 0., 1. };
    const double offset4[4] = { 0., 0., 0., 0. };
    OCIO::CreateMatrixOffsetOp(ops, m44, offset4, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_EQUAL(ops.size(), 1);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO::ConstOpRcPtr op = ops[0];
    OCIO_REQUIRE_ASSERT(op);
    auto mat = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(op->data()); 
    OCIO_REQUIRE_ASSERT(mat);

    OCIO_CHECK_EQUAL(mat->getArray().getValues()[0], 2.);
    OCIO_CHECK_ASSERT(mat->isDiagonal());
}

OCIO_ADD_TEST(OpOptimizers, optimizable)
{
    OCIO::OpRcPtrVec ops;
    // Create identity Matrix.
    double m44[16] = { 1., 0., 0., 0.,
                       0., 1., 0., 0.,
                       0., 0., 1., 0.,
                       0., 0., 0., 1. };
    const double offset4[4] = { 0., 0., 0., 0. };
    OCIO::CreateMatrixOffsetOp(ops, m44, offset4, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_EQUAL(ops.size(), 1);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));

    // Identity matrix is a no-op and is removed. CPU processor will re-add an identity matrix
    // if there are no ops left.
    OCIO_CHECK_EQUAL(ops.size(), 0);
    ops.clear();

    // Add identity matrix.
    OCIO::CreateMatrixOffsetOp(ops, m44, offset4, OCIO::TRANSFORM_DIR_FORWARD);

    // No more an 'identity matrix'.
    m44[0] = 2.;
    m44[1] = 2.;
    OCIO::CreateMatrixOffsetOp(ops, m44, offset4, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_EQUAL(ops.size(), 2);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO::ConstOpRcPtr op = ops[0];
    OCIO_REQUIRE_ASSERT(op);
    auto mat = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(op->data());
    OCIO_REQUIRE_ASSERT(mat);
    OCIO_CHECK_ASSERT(!mat->isIdentity());
    OCIO_CHECK_ASSERT(!mat->isDiagonal());
}

OCIO_ADD_TEST(OpOptimizers, optimization)
{
    // This is a transform consisting of a Lut1d, Matrix, Matrix, Lut1d.
    // The matrices and luts are inverses of one another, so when they are
    // composed they become identities which are then replaced.
    // So this one test actually tests quite a lot of the optimize and compose
    // functionality.

    const std::string fileName("opt_test1.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 5);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_NONE));
    OCIO_CHECK_EQUAL(ops.size(), 4);

    OCIO::OpRcPtrVec optOps = ops.clone();
    OCIO_CHECK_EQUAL(optOps.size(), 4);
    OCIO_CHECK_NO_THROW(optOps.finalize());
    OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(optOps.size(), 1);
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");

    // Compare renders.
    CompareRender(ops, optOps, __LINE__, 1e-6f);
}

OCIO_ADD_TEST(OpOptimizers, optimization2)
{
    // This transform has the following ops:
    // 1 Lut1D, half domain, effectively an identity
    // 2 Matrix, bit depth conversion identity
    // 3 Matrix, bit depth conversion identity
    // 4 Matrix, almost identity
    // 5 Range, clamp identity
    // 6 Lut1D, half domain, raw halfs, identity
    // 7 Lut1D, raw halfs, identity
    // 8 Matrix, not identity
    // 9 Matrix, not identity
    // 10 Lut1D, almost identity
    // 11 Lut1D, almost identity that composes to an identity with the previous one
    // 12 Lut3D, not identity
    // 13 Lut3D, not identity but composes to an identity with the previous one

    const std::string fileName("opt_test2.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 14);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_NONE));
    OCIO_CHECK_EQUAL(ops.size(), 13);

    OCIO::OpRcPtrVec optOps = ops.clone();
    OCIO_CHECK_EQUAL(optOps.size(), 13);

    // No need to remove OPTIMIZATION_COMP_SEPARABLE_PREFIX because optimization is for F32.
    OCIO_CHECK_NO_THROW(optOps.finalize());
    OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_GOOD));
    OCIO_REQUIRE_EQUAL(optOps.size(), 4);

    // Op 1 is exactly an identity except for the first value which is 0.000001. Since the
    // outDepth=16i, this gets normalized by 1/65536, which puts it well under the noise
    // threshold so it's optimized it out as an identity.

    // Ops 2 & 3 are identities and removed.
    // This is op 4.
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<MatrixOffsetOp>");
    // Op 5 is a clamp identity and removed.
    // Ops 6 & 7 are identities and are replaced by ranges, one is removed.
    OCIO_CHECK_EQUAL(optOps[1]->getInfo(), "<RangeOp>");
    // This is op 8 composed with op 9.
    OCIO_CHECK_EQUAL(optOps[2]->getInfo(), "<MatrixOffsetOp>");
    // Ops 10 & 11 composed become an identity, is replaced with a range,
    // which is then removed as a clamp identity
    // This is op 12 composed with op 13.  It is an identity.
    // NB: We don't try to detect Lut3DOp identities.
    OCIO_CHECK_EQUAL(optOps[3]->getInfo(), "<Lut3DOp>");

    // Compare renders.
    CompareRender(ops, optOps, __LINE__, 1e-6f);
}

OCIO_ADD_TEST(OpOptimizers, lut1d_identities)
{
    // This transform has the following ops:
    // 1 Lut1D, identity
    // 2 Lut1D, identity
    // 3 Lut1D, not quite identity
    // 4 Lut1D, half-domain identity (note 16i outDepth)
    // 5 Lut1D, half-domain not an identity (values clamped due to rawHalfs encoding)
    // 6 Lut1D, identity

    const std::string fileName("lut1d_identity_test.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_REQUIRE_EQUAL(ops.size(), 7);

    OCIO_CHECK_ASSERT(ops[1]->isIdentity());
    OCIO_CHECK_ASSERT(ops[2]->isIdentity());
    OCIO_CHECK_ASSERT(false == ops[3]->isIdentity());
    OCIO_CHECK_ASSERT(ops[4]->isIdentity());
    OCIO_CHECK_ASSERT(false == ops[5]->isIdentity());
    OCIO_CHECK_ASSERT(ops[6]->isIdentity());

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_NONE));

    OCIO::OpRcPtrVec optOps = ops.clone();
    OCIO_CHECK_EQUAL(optOps.size(), 6);

    OCIO_CHECK_NO_THROW(optOps.finalize());
    OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(optOps.size(), 2);

    // The first two LUTs should get detected as identities and replaced with ranges which then
    // get removed as clamp identities. LUT 3 is not identity and is not removed.
    // The next LUT should also get detected as an identity, and replaced with a matrix (rather
    // than a range since it is half-domain) which is then optimized out. The next LUT is almost
    // an identity except the large values are clamped due to the rawHalfs encoding, so it is not
    // removed. The final LUT is a normal domain to be replaced with a range. The two Luts get
    // combined into a single Lut with a standard domain.

    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<Lut1DOp>");
    OCIO_CHECK_EQUAL(optOps[1]->getInfo(), "<RangeOp>");

    OCIO::ConstOpRcPtr lutOp = optOps[0];
    auto lutData = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(lutOp->data());
    OCIO_REQUIRE_ASSERT(lutData);
    OCIO_CHECK_EQUAL(lutData->getArray().getLength(), 65536);
    OCIO_CHECK_ASSERT(!lutData->isInputHalfDomain());

    // Now check that the optimized transform renders the same as the original.
    // TODO: Shall investigate why this test requires a bigger error.
    CompareRender(ops, optOps, __LINE__, 3e-4f);
}

OCIO_ADD_TEST(OpOptimizers, lut1d_identity_replacement)
{
    // Test that an identity Lut1D becomes a range but a half-domain becomes a matrix.
    {
        auto lutData = std::make_shared<OCIO::Lut1DOpData>(3);
        OCIO_CHECK_ASSERT(lutData->isIdentity());

        OCIO::OpRcPtrVec ops;
        OCIO::CreateLut1DOp(ops, lutData, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 1);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(ops.size(), 1);

        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<RangeOp>");
    }
    {
        // By setting the filterNaNs argument to true, the constructor replaces NaN values with 0
        // and this causes the LUT to technically no longer be an identity since the values are no
        // longer exactly what is in a half float.
        OCIO::Lut1DOpDataRcPtr lutData
            = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_OUTPUT_HALF_CODE,
                                                  65536, true);
        lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_F32);
        OCIO_CHECK_ASSERT(!lutData->isIdentity());
        OCIO_CHECK_ASSERT(lutData->isInputHalfDomain());
    }
    {
        // By default, this constructor creates an 'identity lut'.
        OCIO::Lut1DOpDataRcPtr lutData
            = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_OUTPUT_HALF_CODE,
                                                  65536, false);
        lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_F32);
        OCIO_CHECK_ASSERT(lutData->isIdentity());
        OCIO_CHECK_ASSERT(lutData->isInputHalfDomain());

        OCIO::OpRcPtrVec ops;
        OCIO::CreateLut1DOp(ops, lutData, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 1);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));

        // Half domain LUT 1d is a no-op.
        // CPU processor will add an identity matrix.
        OCIO_CHECK_EQUAL(ops.size(), 0);
    }
}

OCIO_ADD_TEST(OpOptimizers, lut1d_identity_replacement_order)
{
    // See issue #1737, https://github.com/AcademySoftwareFoundation/OpenColorIO/issues/1737.

    // This CTF contains a single LUT1D, inverse direction, normal (not half) domain.
    // It contains values from -6 to +3.4.
    const std::string fileName("lut1d_inverse_gpu.ctf");
    OCIO::ContextRcPtr context = OCIO::Context::Create();

    OCIO::OpRcPtrVec inv_ops;
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(inv_ops, fileName, context,
                                        // FWD direction simply means don't swap the direction, the
                                        // file contains an inverse LUT1D and leave it that way.
                                           OCIO::TRANSFORM_DIR_FORWARD));
    OCIO::OpRcPtrVec fwd_ops;
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(fwd_ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_INVERSE));

    // Check forward LUT1D followed by inverse LUT1D.
    {
        OCIO::OpRcPtrVec fwd_inv_ops = fwd_ops;
        fwd_inv_ops += inv_ops;

        OCIO_CHECK_NO_THROW(fwd_inv_ops.finalize());
        OCIO_CHECK_NO_THROW(fwd_inv_ops.optimize(OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(fwd_inv_ops.size(), 2);  // no optmization was done

        OCIO::OpRcPtrVec optOps = fwd_inv_ops.clone();
        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");

        // Compare renders.
        CompareRender(fwd_inv_ops, optOps, __LINE__, 1e-6f);
    }

    // Check inverse LUT1D followed by forward LUT1D.
    {
        OCIO::OpRcPtrVec inv_fwd_ops = inv_ops;
        inv_fwd_ops += fwd_ops;

        OCIO_CHECK_NO_THROW(inv_fwd_ops.finalize());
        OCIO_CHECK_NO_THROW(inv_fwd_ops.optimize(OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(inv_fwd_ops.size(), 2);  // no optmization was done

        OCIO::OpRcPtrVec optOps = inv_fwd_ops.clone();
        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");

        // Compare renders.
        CompareRender(inv_fwd_ops, optOps, __LINE__, 1e-6f);
    }
}

OCIO_ADD_TEST(OpOptimizers, lut1d_half_domain_keep_prior_range)
{
    // A half-domain LUT should not allow removal of a prior range op.

    OCIO::OpRcPtrVec ops;
    OCIO::CreateRangeOp(ops, 0., 1., 0., 1., OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_OUTPUT_HALF_CODE,
                                              65536, false);
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_F32);

    // Add no-op LUT.
    OCIO::CreateLut1DOp(ops, lutData, OCIO::TRANSFORM_DIR_INVERSE);

    // Add another LUT.
    lutData = lutData->clone();
    auto & lutArray = lutData->getArray();
    for (auto & val : lutArray.getValues())
    {
        val = -val;
    }
    OCIO::CreateLut1DOp(ops, lutData, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_EQUAL(ops.size(), 3);

    OCIO::OpRcPtrVec optOps = ops.clone();
    OCIO_CHECK_EQUAL(optOps.size(), 3);
    OCIO_CHECK_NO_THROW(optOps.finalize());
    OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(optOps.size(), 2);

    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
    OCIO_CHECK_EQUAL(optOps[1]->getInfo(), "<Lut1DOp>");

    // Now check that the optimized transform renders the same as the original.
    CompareRender(ops, optOps, __LINE__, 1e-6f);
}

OCIO_ADD_TEST(OpOptimizers, range_composition)
{
    const double emptyValue = OCIO::RangeOpData::EmptyValue();
    {
        // Two identity clamp negs ranges are collapsed into one.
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0., emptyValue, 0., emptyValue, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0., emptyValue, 0., emptyValue, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_EQUAL(ops.size(), 2);

        OCIO::OpRcPtrVec optOps = ops.clone();
        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        // Non identity ranges are combined.
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0.1, emptyValue, 0.1, emptyValue, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.2, emptyValue, 0.2, emptyValue, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_EQUAL(ops.size(), 2);

        OCIO::OpRcPtrVec optOps = ops.clone();
        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        // Non identity ranges are combined.
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0.1, emptyValue, 0.1, emptyValue, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, emptyValue, 0.1, emptyValue, OCIO::TRANSFORM_DIR_INVERSE);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_REQUIRE_EQUAL(ops.size(), 2);

        OCIO::OpRcPtrVec optOps = ops.clone();
        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        // Non identity ranges are combined.
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, -0.1, emptyValue, -0.1, emptyValue, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, -0.1, emptyValue, -0.1, emptyValue, OCIO::TRANSFORM_DIR_INVERSE);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_REQUIRE_EQUAL(ops.size(), 2);

        OCIO::OpRcPtrVec optOps = ops.clone();
        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        // A clamp negs range is dropped before a more restrictive one.
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0., emptyValue, 0., emptyValue, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, 2., 0., 2., OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_EQUAL(ops.size(), 2);

        OCIO::OpRcPtrVec optOps = ops.clone();
        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0., 0.5, 0., 0.5, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, 1., 0.1, 1., OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_EQUAL(ops.size(), 2);

        OCIO::OpRcPtrVec optOps = ops.clone();
        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0., 0.6, 0., 0.5, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, 1., 0.2, 1., OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0., 1., 0., 2., OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_EQUAL(ops.size(), 3);

        OCIO::OpRcPtrVec optOps = ops.clone();
        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0., 0.5, 0., 0.5, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.6, 1., 0.6, 1., OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_EQUAL(ops.size(), 2);

        OCIO::OpRcPtrVec optOps = ops.clone();
        // Two Ranges with non-overlapping pass regions are replaced with a clamp to a constant.
        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0.6, 1., 0.6, 1., OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0., 0.5, 0., 0.5, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_EQUAL(ops.size(), 2);

        OCIO::OpRcPtrVec optOps = ops.clone();
        // Ranges can not be combined out domain of the first does not intersect
        // with in domain of the second.
        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0., 0.6, 0., 0.5, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, 1., 0.2, 1., OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, 1., 0.2, 1., OCIO::TRANSFORM_DIR_INVERSE);
        OCIO::CreateRangeOp(ops, 0., 1., 0., 2., OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, 0.8, 0.2, 1., OCIO::TRANSFORM_DIR_INVERSE);
        OCIO::CreateRangeOp(ops, 0.2, 0.6, 0.1, 0.7, OCIO::TRANSFORM_DIR_INVERSE);

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_EQUAL(ops.size(), 6);

        OCIO::OpRcPtrVec optOps = ops.clone();
        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
}

OCIO_ADD_TEST(OpOptimizers, invlut_pair_identities)
{
    // The file contains an InverseLUT1D and LUT1D, both with the same array, followed by
    // an InverseLUT3D and LUT3D, also both with the same array.  The pairs should each get
    // replaced by a range and then the ranges should be combined.
    const std::string fileName("lut_inv_pairs.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 5);

    // Remove no ops & finalize for computation.
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_NONE));

    OCIO_CHECK_EQUAL(ops.size(), 4);

    OCIO::OpRcPtrVec optOps = ops.clone();
    OCIO_CHECK_EQUAL(optOps.size(), 4);
    OCIO_CHECK_NO_THROW(optOps.finalize());
    OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(optOps.size(), 1);
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
    // (A range that clamps on either side is not a no-op.)
    OCIO_CHECK_ASSERT(!optOps[0]->isNoOp());

    // Now check that the optimized transform renders the same as the original.
    CompareRender(ops, optOps, __LINE__, 1e-6f);
}

OCIO_ADD_TEST(OpOptimizers, mntr_identities)
{
    // Forward and inverse monitor transforms should become an identity.
    const std::string fileName("mntr_srgb_identity.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 5);

    // Remove no ops & finalize for computation.
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_NONE));

    OCIO_CHECK_EQUAL(ops.size(), 4);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));

    // Identity transform.
    OCIO_CHECK_EQUAL(ops.size(), 0);
}

OCIO_ADD_TEST(OpOptimizers, gamma_comp)
{
    // This transform has a pair of gammas separated by an identity matrix
    // that should compose into a single (non-identity) gamma that then should
    // be identified as a pair identity with another gamma and be replaced with
    // a clamp-negs range.

    const std::string fileName("gamma_comp_test.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 5);

    // Remove no ops & finalize for computation.
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_NONE));

    OCIO_CHECK_EQUAL(ops.size(), 4);

    OCIO::OpRcPtrVec optOps            = ops.clone();
    OCIO::OpRcPtrVec optOps_noComp     = ops.clone();

    OCIO_CHECK_EQUAL(optOps_noComp.size(), 4);
    OCIO_CHECK_NO_THROW(optOps_noComp.finalize());
    OCIO_CHECK_NO_THROW(optOps_noComp.optimize(AllBut(OCIO::OPTIMIZATION_COMP_GAMMA)));
    // Identity matrix is removed but gamma are not combined.
    OCIO_REQUIRE_EQUAL(optOps_noComp.size(), 3);
    OCIO_CHECK_EQUAL(optOps_noComp[0]->getInfo(), "<GammaOp>");
    OCIO_CHECK_EQUAL(optOps_noComp[1]->getInfo(), "<GammaOp>");
    OCIO_CHECK_EQUAL(optOps_noComp[2]->getInfo(), "<GammaOp>");

    CompareRender(ops, optOps_noComp, __LINE__, 1e-6f);

    OCIO_CHECK_NO_THROW(optOps.finalize());
    OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));

    // Identity matrix is removed and gammas combined and optimized as a range.
    OCIO_REQUIRE_EQUAL(optOps.size(), 1);
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");

    // Now check that the optimized transform renders the same as the original.
    // TODO: Gamma is clamping alpha, and Range does not.
    CompareRender(ops, optOps, __LINE__, 1e-4f, true);
}

OCIO_ADD_TEST(OpOptimizers, gamma_comp_test2)
{
    // This transform has a pair of gammas separated by a pair of matrices that
    // compose into an identity matrix and get optimized out. Then the gammas
    // get composed into a non-identity gamma. Finally the exponent is inverted
    // (to follow the convention of keeping it > 1) and the direction is inverted.

    const std::string fileName("gamma_comp_test2.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 5);

    // Remove no ops & finalize for computation.
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_NONE));

    OCIO_CHECK_EQUAL(ops.size(), 4);

    OCIO::OpRcPtrVec optOps            = ops.clone();
    OCIO::OpRcPtrVec optOps_noComp     = ops.clone();

    OCIO_CHECK_EQUAL(optOps_noComp.size(), 4);
    OCIO_CHECK_NO_THROW(optOps_noComp.finalize());
    // NB: The op->apply function used here hard-codes OPTIMIZATION_FAST_LOG_EXP_POW to off.
    OCIO_CHECK_NO_THROW(optOps_noComp.optimize(AllBut(OCIO::OPTIMIZATION_COMP_GAMMA)));
    OCIO_REQUIRE_EQUAL(optOps_noComp.size(), 2);
    OCIO_CHECK_EQUAL(optOps_noComp[0]->getInfo(), "<GammaOp>");
    OCIO_CHECK_EQUAL(optOps_noComp[1]->getInfo(), "<GammaOp>");

    // Due to rounding error in the two 3x3 matrix multiplies with much larger values, the
    // 1.52e-4 input value is off by 60% going into the second gamma (see ociochecklut -s).
    // Therefore the optOps_noComp and optOps are actually more accurate than ops here.
    CompareRender(ops, optOps_noComp, __LINE__, 1e-4f);

    OCIO_CHECK_NO_THROW(optOps.finalize());
    OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));

    // Now check that the optimized transform renders the same as the original.
    CompareRender(ops, optOps, __LINE__, 1e-4f);

    // Check the op is as expected.
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    OCIO_REQUIRE_EQUAL(optOps.size(), 1);
    OCIO::ConstOpRcPtr op(optOps[0]);
    OCIO::CreateGammaTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto gTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::ExponentTransform>(transform);
    OCIO_REQUIRE_ASSERT(gTransform);
    OCIO_CHECK_EQUAL(gTransform->getNegativeStyle(), OCIO::NEGATIVE_PASS_THRU);
    OCIO_CHECK_EQUAL(gTransform->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    double vals[4];
    gTransform->getValue(vals);
    OCIO_CHECK_CLOSE(vals[0], 2.2/1.8, 1e-6f);
    OCIO_CHECK_CLOSE(vals[1], 2.2/1.8, 1e-6f);
    OCIO_CHECK_CLOSE(vals[2], 2.2/1.8, 1e-6f);
    OCIO_CHECK_EQUAL(vals[3], 1.);
}

OCIO_ADD_TEST(OpOptimizers, gamma_comp_identity)
{
    OCIO::OpRcPtrVec ops;

    OCIO::GammaOpData::Params params1 = { 0.45 };
    OCIO::GammaOpData::Params paramsA = { 1. };

    auto gamma1 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_FWD,
                                                      params1, params1, params1, paramsA);

    // Note that gamma2 is not a pair inverse of gamma1, it is another FWD gamma where the
    // parameter is an inverse. Therefore it won't get replaced as a pair inverse, it must
    // be composed into an identity, which may then be replaced. Since the BASIC_FWD style
    // clamps negatives, it is replaced with a Range.
    OCIO::GammaOpData::Params params2 = { 1. / 0.45 };

    auto gamma2 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_FWD,
                                                      params2, params2, params2, paramsA);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma2, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_EQUAL(ops.size(), 2);

    {
        OCIO::OpRcPtrVec optOps = ops.clone();

        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(AllBut(OCIO::OPTIMIZATION_IDENTITY_GAMMA)));

        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<GammaOp>");
    }
    {
        OCIO::OpRcPtrVec optOps = ops.clone();

        // BASIC gammas are composed resulting in an identity, that get optimized as a range.
        OCIO_CHECK_NO_THROW(optOps.finalize());
        OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));

        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
    }

    // Now do the same test with MONCURVE rather than BASIC style.

    ops.clear();

    params1 = { 2., 0.5 };
    params2 = { 2., 0.6 };
    paramsA = { 1., 0. };
    gamma1 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::MONCURVE_FWD,
                                                 params1, params1, params1, paramsA);
    gamma2 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::MONCURVE_FWD,
                                                 params2, params2, params2, paramsA);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma2, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_EQUAL(ops.size(), 2);

    OCIO::OpRcPtrVec optOps = ops.clone();

    // MONCURVE composition is not supported yet.
    OCIO_CHECK_NO_THROW(optOps.finalize());
    OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(optOps.size(), 2);
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<GammaOp>");
    OCIO_CHECK_EQUAL(optOps[1]->getInfo(), "<GammaOp>");
}

OCIO_ADD_TEST(OpOptimizers, log_identities)
{
    // Log fwd and rev transforms should become a range.
    // This transform has two pair of LogOps separated by an identity matrix
    // that should optimize into a range.

    const std::string fileName("log_identities.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));


    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 6);

    // Remove no ops & finalize for computation.
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_NONE));

    OCIO_CHECK_EQUAL(ops.size(), 5);

    OCIO::OpRcPtrVec optOps    = ops.clone();
    OCIO::OpRcPtrVec optOpsOff = ops.clone();

    OCIO_CHECK_EQUAL(optOpsOff.size(), 5);
    OCIO_CHECK_NO_THROW(optOpsOff.finalize());
    OCIO_CHECK_NO_THROW(optOpsOff.optimize(AllBut(OCIO::OPTIMIZATION_PAIR_IDENTITY_LOG)));
    // Only the identity matrix is optimized.
    OCIO_CHECK_EQUAL(optOpsOff.size(), 4);

    OCIO_CHECK_EQUAL(optOps.size(), 5);
    OCIO_CHECK_NO_THROW(optOps.finalize());
    OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
    // Identity matrix is optimized and forward/inverse log are combined.
    OCIO_REQUIRE_EQUAL(optOps.size(), 1);

    // (A range that clamps on either side is not a no-op.)
    OCIO_CHECK_ASSERT(!optOps[0]->isNoOp());
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");

    // Now check that the optimized transform renders the same as the original.
    CompareRender(ops, optOps, __LINE__, 1e-4f);
}

OCIO_ADD_TEST(OpOptimizers, range_lut)
{
    // Non-identity range before a Lut1D should not be removed.

    const std::string fileName("range_lut.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 3);

    // Remove no ops & finalize for computation.
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_NONE));

    OCIO_CHECK_EQUAL(ops.size(), 2);

    OCIO::OpRcPtrVec optOps = ops.clone();
    OCIO_CHECK_EQUAL(optOps.size(), 2);
    OCIO_CHECK_NO_THROW(optOps.finalize());
    OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(optOps.size(), 2);
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
    OCIO_CHECK_EQUAL(optOps[1]->getInfo(), "<Lut1DOp>");

    // Now check that the optimized transform renders the same as the original.
    CompareRender(ops, optOps, __LINE__, 1e-6f);
}

OCIO_ADD_TEST(OpOptimizers, dynamic_ops)
{
    // Non-identity matrix.
    OCIO::MatrixOpDataRcPtr matrix = std::make_shared<OCIO::MatrixOpData>();
    matrix->setArrayValue(0, 2.);

    // Identity exposure contrast.
    OCIO::ExposureContrastOpDataRcPtr exposure = std::make_shared<OCIO::ExposureContrastOpData>();

    OCIO::ExposureContrastOpDataRcPtr exposureDyn = exposure->clone();
    exposureDyn->getExposureProperty()->makeDynamic();

    // Test with non dynamic exposure contrast.
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, exposure,
                                                           OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        OCIO_CHECK_ASSERT(!ops[0]->isIdentity());
        OCIO_CHECK_ASSERT(ops[1]->isIdentity());

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
        // It gets optimized.
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    }

    // Test with dynamic exposure contrast.
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, exposureDyn,
                                                           OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_REQUIRE_EQUAL(ops.size(), 3);
        OCIO_CHECK_ASSERT(!ops[0]->isIdentity());

        // Exposure contrast is dynamic.
        OCIO_CHECK_ASSERT(ops[1]->isDynamic());
        OCIO_CHECK_ASSERT(!ops[1]->isIdentity());

        OCIO_CHECK_ASSERT(!ops[2]->isIdentity());

        // It does not get optimized with default flags (OPTIMIZATION_NO_DYNAMIC_PROPERTIES off).
        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(ops.size(), 3);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<ExposureContrastOp>");
        OCIO_CHECK_EQUAL(ops[2]->getInfo(), "<MatrixOffsetOp>");

        // It does get optimized if flag is set.
        // OPTIMIZATION_ALL includes OPTIMIZATION_NO_DYNAMIC_PROPERTIES.
        // Exposure contrast will get optimized and the 2 matrix will be composed.
        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_ALL));
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    }
}

OCIO_ADD_TEST(OpOptimizers, gamma_prefix)
{
    OCIO::OpRcPtrVec originalOps;

    OCIO::GammaOpData::Params params1 = {2.6};
    OCIO::GammaOpData::Params paramsA = {1.};

    OCIO::GammaOpDataRcPtr gamma1
        = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_REV, 
                                              params1, params1, params1, paramsA);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(originalOps, gamma1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 1);

    OCIO::OpRcPtrVec optimizedOps = originalOps.clone();

    // Optimize it.
    OCIO_CHECK_NO_THROW(optimizedOps.finalize());
    OCIO_CHECK_NO_THROW(optimizedOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(optimizedOps.optimizeForBitdepth(OCIO::BIT_DEPTH_UINT16,
                                                         OCIO::BIT_DEPTH_F32,
                                                         OCIO::OPTIMIZATION_COMP_SEPARABLE_PREFIX));

    // Validate the result.

    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 1);

    OCIO::ConstOpRcPtr o1              = optimizedOps[0];
    OCIO::ConstLut1DOpDataRcPtr oData1 = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(o1->data());
    OCIO_REQUIRE_ASSERT(oData1);
    OCIO_CHECK_EQUAL(oData1->getType(), OCIO::OpData::Lut1DType);
    OCIO_CHECK_EQUAL(oData1->getArray().getLength(), 65536);
    originalOps.clear();

    // However, if the input bit depth is F32, it should not be optimized.

    OCIO::GammaOpDataRcPtr gamma2
        = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_REV, 
                                              params1, params1, params1, paramsA);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(originalOps, gamma2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 1);

    // Optimize it.
    OCIO_CHECK_NO_THROW(originalOps.finalize());
    OCIO_CHECK_NO_THROW(originalOps.optimize(OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(originalOps.size(), 1);
    OCIO::ConstOpRcPtr o2 = originalOps[0];
    OCIO_CHECK_EQUAL(o2->data()->getType(), OCIO::OpData::GammaType);
}

OCIO_ADD_TEST(OpOptimizers, multi_op_prefix)
{
    // Test prefix optimization of a complex transform.

    OCIO::OpRcPtrVec originalOps;

    OCIO::MatrixOpDataRcPtr matrix = std::make_shared<OCIO::MatrixOpData>();
    matrix->setArrayValue(0, 2.);

    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(originalOps, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 1);

    OCIO::RangeOpDataRcPtr range
        = std::make_shared<OCIO::RangeOpData>(0., 1., -1000./65535., 66000./65535);

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(originalOps, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 2);

    OCIO::OpRcPtrVec optimizedOps = originalOps.clone();

    // Nothing to optimize.
    OCIO_CHECK_NO_THROW(optimizedOps.finalize());
    OCIO_CHECK_NO_THROW(optimizedOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(optimizedOps.optimizeForBitdepth(OCIO::BIT_DEPTH_UINT8,
                                                         OCIO::BIT_DEPTH_F32,
                                                         OCIO::OPTIMIZATION_COMP_SEPARABLE_PREFIX));

    // Validate ops are unchanged.

    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 2U);

    std::string originalID;
    std::string optimizedID;
    OCIO_CHECK_NO_THROW(originalID = originalOps[0]->getCacheID());
    OCIO_CHECK_NO_THROW(optimizedID = optimizedOps[0]->getCacheID());

    OCIO_CHECK_EQUAL(originalID, optimizedID);

    OCIO_CHECK_NO_THROW(originalID = originalOps[1]->getCacheID());
    OCIO_CHECK_NO_THROW(optimizedID = optimizedOps[1]->getCacheID());

    OCIO_CHECK_EQUAL(originalID, optimizedID);

    // Add more ops to originalOps.
    const OCIO::CDLOpData::ChannelParams slope(1.35, 1.1, 0.071);
    const OCIO::CDLOpData::ChannelParams offset(0.05, -0.23, 0.11);
    const OCIO::CDLOpData::ChannelParams power(1.27, 0.81, 0.2);
    const double saturation = 1.;

    OCIO::CDLOpDataRcPtr cdl
        = std::make_shared<OCIO::CDLOpData>(OCIO::CDLOpData::CDL_V1_2_FWD, 
                                            slope, offset, power, saturation);

    OCIO_CHECK_NO_THROW(OCIO::CreateCDLOp(originalOps, cdl, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(originalOps.finalize());
    OCIO_REQUIRE_EQUAL(originalOps.size(), 3);

    optimizedOps = originalOps.clone();

    // Optimize it.
    OCIO_CHECK_NO_THROW(optimizedOps.finalize());
    OCIO_CHECK_NO_THROW(optimizedOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(optimizedOps.optimizeForBitdepth(OCIO::BIT_DEPTH_UINT8,
                                                         OCIO::BIT_DEPTH_F32,
                                                         OCIO::OPTIMIZATION_COMP_SEPARABLE_PREFIX));

    // Validate the result.

    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 1U);

    OCIO::ConstOpRcPtr o              = optimizedOps[0];
    OCIO::ConstLut1DOpDataRcPtr oData = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(o->data());
    OCIO_CHECK_EQUAL(oData->getType(), OCIO::OpData::Lut1DType);
    OCIO_CHECK_EQUAL(oData->getArray().getLength(), 256);

    // Make sure originalOps are ready to render.
    OCIO_CHECK_NO_THROW(originalOps.finalize());

    // Although finalized for UINT8, the transform may still be evaluated at 32f to verify that
    // it is a good approximation to the original.
    CompareRender(originalOps, optimizedOps, __LINE__, 5e-5f);
}

OCIO_ADD_TEST(OpOptimizers, dyn_properties_prefix)
{
    // Test prefix optimization of a complex transform.

    OCIO::OpRcPtrVec originalOps;

    OCIO::MatrixOpDataRcPtr matrix = std::make_shared<OCIO::MatrixOpData>();
    matrix->setArrayValue(0, 2.);

    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(originalOps, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 1);

    OCIO::ExposureContrastOpDataRcPtr exposure = std::make_shared<OCIO::ExposureContrastOpData>();

    exposure->setExposure(1.2);
    exposure->setPivot(0.5);

    OCIO_CHECK_NO_THROW(
        OCIO::CreateExposureContrastOp(originalOps, exposure, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 2);

    exposure = exposure->clone();
    exposure->getExposureProperty()->makeDynamic();

    OCIO_CHECK_NO_THROW(
        OCIO::CreateExposureContrastOp(originalOps, exposure, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 3);

    OCIO::OpRcPtrVec optimizedOps = originalOps.clone();

    // Optimize it.
    OCIO_CHECK_NO_THROW(optimizedOps.finalize());
    OCIO_CHECK_NO_THROW(optimizedOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(optimizedOps.optimizeForBitdepth(OCIO::BIT_DEPTH_UINT8,
                                                         OCIO::BIT_DEPTH_F32,
                                                         OCIO::OPTIMIZATION_COMP_SEPARABLE_PREFIX));

    // Validate the result.

    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 2U);

    OCIO::ConstOpRcPtr o              = optimizedOps[0];
    OCIO::ConstLut1DOpDataRcPtr oData = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(o->data());
    OCIO_CHECK_ASSERT(oData);
    OCIO_CHECK_EQUAL(oData->getType(), OCIO::OpData::Lut1DType);
    OCIO_CHECK_EQUAL(oData->getArray().getLength(), 256);
    OCIO_CHECK_CLOSE(oData->getArray()[255 * 3 + 0], 4.5947948f, 1.e-6f);
    OCIO_CHECK_CLOSE(oData->getArray()[255 * 3 + 1], 2.2973969f, 1.e-6f);
    OCIO_CHECK_CLOSE(oData->getArray()[255 * 3 + 2], 2.2973969f, 1.e-6f);

    o = optimizedOps[1];
    OCIO::ConstExposureContrastOpDataRcPtr exp =
        OCIO::DynamicPtrCast<const OCIO::ExposureContrastOpData>(o->data());

    OCIO_CHECK_ASSERT(exp);
    OCIO_CHECK_EQUAL(exp->getType(), OCIO::OpData::ExposureContrastType);
    OCIO_CHECK_ASSERT(exp->isDynamic());
}

OCIO_ADD_TEST(OpOptimizers, opt_prefix_test1)
{
    const std::string fileName("opt_prefix_test1.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 12);

    OCIO_CHECK_NO_THROW(OCIO::RemoveNoOpTypes(ops));

    OCIO_CHECK_EQUAL(ops.size(), 11);

    OCIO::OpRcPtrVec optOps = ops.clone();
    OCIO_CHECK_EQUAL(optOps.size(), 11);
    // Ignore dynamic properties.
    OCIO_CHECK_NO_THROW(optOps.finalize());
    OCIO_CHECK_NO_THROW(optOps.optimize(OCIO::OPTIMIZATION_ALL));
    OCIO_CHECK_NO_THROW(optOps.optimizeForBitdepth(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32,
                                                   OCIO::OPTIMIZATION_COMP_SEPARABLE_PREFIX));

    OCIO_REQUIRE_EQUAL(optOps.size(), 3);

    OCIO::ConstOpRcPtr o0 = optOps[0];
    OCIO::ConstOpRcPtr o1 = optOps[1];
    OCIO::ConstOpRcPtr o2 = optOps[2];

    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::Lut1DType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::MatrixType);
    OCIO_CHECK_EQUAL(o2->data()->getType(), OCIO::OpData::GammaType);

    auto lut0 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Lut1DOpData>(o0->data());
    OCIO_CHECK_ASSERT(!lut0->isIdentity());
    OCIO_CHECK_EQUAL(lut0->getArray().getLength(), 65536u);
}

OCIO_ADD_TEST(OpOptimizers, replace_ops)
{
    auto cdlData = std::make_shared<OCIO::CDLOpData>();
    OCIO::CDLOpData::ChannelParams newOffsetParams(0.09);
    cdlData->setOffsetParams(newOffsetParams);
    cdlData->setSaturation(1.23);
    cdlData->setSlopeParams(OCIO::CDLOpData::ChannelParams(0.8, 0.9, 1.1));
    cdlData->setStyle(OCIO::CDLOpData::CDL_NO_CLAMP_FWD);

    OCIO::OpRcPtrVec originalOps;

    OCIO_CHECK_NO_THROW(
        OCIO::CreateCDLOp(originalOps, cdlData, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 1);

    OCIO::OpRcPtrVec optimizedOps = originalOps.clone();

    // Verify that default optimization includes replacing ops.
    OCIO_CHECK_ASSERT(OCIO::HasFlag(OCIO::OPTIMIZATION_DEFAULT,
                                    OCIO::OPTIMIZATION_SIMPLIFY_OPS));

    // Optimize it: CDL is replaced by a matrix.
    OCIO_CHECK_NO_THROW(optimizedOps.finalize());
    OCIO_CHECK_NO_THROW(optimizedOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 1);

    OCIO::ConstOpRcPtr o = optimizedOps[0];
    auto oData = o->data();
    OCIO_CHECK_EQUAL(oData->getType(), OCIO::OpData::MatrixType);

    // No optimization: keep CDL.
    optimizedOps = originalOps.clone();
    OCIO_CHECK_NO_THROW(optimizedOps.finalize());
    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 1);

    o = optimizedOps[0];
    oData = o->data();
    OCIO_CHECK_EQUAL(oData->getType(), OCIO::OpData::CDLType);

    // Only replace. CDL is replaced by 2 matrices, one for offset and slope, one for saturation.
    // Default optimization would combine them.
    optimizedOps = originalOps.clone();
    OCIO_CHECK_NO_THROW(optimizedOps.finalize());
    OCIO_CHECK_NO_THROW(optimizedOps.optimize(OCIO::OPTIMIZATION_SIMPLIFY_OPS));
    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 2);

    o = optimizedOps[0];
    oData = o->data();
    OCIO_CHECK_EQUAL(oData->getType(), OCIO::OpData::MatrixType);

    o = optimizedOps[1];
    oData = o->data();
    OCIO_CHECK_EQUAL(oData->getType(), OCIO::OpData::MatrixType);

    // Use clamping style.
    cdlData->setStyle(OCIO::CDLOpData::CDL_V1_2_FWD);

    optimizedOps.clear();

    OCIO_CHECK_NO_THROW(
        OCIO::CreateCDLOp(optimizedOps, cdlData, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 1);

    // Optimize it: CDL replaced by 2 matrices and 2 clamps.
    OCIO_CHECK_NO_THROW(optimizedOps.finalize());
    OCIO_CHECK_NO_THROW(optimizedOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 4);

    o = optimizedOps[0];
    oData = o->data();
    OCIO_CHECK_EQUAL(oData->getType(), OCIO::OpData::MatrixType);

    o = optimizedOps[1];
    oData = o->data();
    OCIO_CHECK_EQUAL(oData->getType(), OCIO::OpData::RangeType);

    o = optimizedOps[2];
    oData = o->data();
    OCIO_CHECK_EQUAL(oData->getType(), OCIO::OpData::MatrixType);

    o = optimizedOps[3];
    oData = o->data();
    OCIO_CHECK_EQUAL(oData->getType(), OCIO::OpData::RangeType);

    // With a non-identity power.
    cdlData->setPowerParams(OCIO::CDLOpData::ChannelParams(1, 1, 1.0001));

    optimizedOps.clear();

    OCIO_CHECK_NO_THROW(
        OCIO::CreateCDLOp(optimizedOps, cdlData, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 1);

    // Optimize it: CDL is not replaced.
    OCIO_CHECK_NO_THROW(optimizedOps.finalize());
    OCIO_CHECK_NO_THROW(optimizedOps.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 1);

    o = optimizedOps[0];
    oData = o->data();
    OCIO_CHECK_EQUAL(oData->getType(), OCIO::OpData::CDLType);
}
