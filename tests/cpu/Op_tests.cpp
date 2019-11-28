// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "Op.cpp"

#include "ops/noop/NoOps.h"
#include "UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;

void Apply(const OCIO::OpRcPtrVec & ops, float * source, long numPixels)
{
    for(const auto & op : ops)
    {
        op->apply(source, numPixels);
    }
}

OCIO_ADD_TEST(FinalizeOpVec, optimize_combine)
{
    const double m1[16] = { 1.1, 0.2, 0.3, 0.4,
                            0.5, 1.6, 0.7, 0.8,
                            0.2, 0.1, 1.1, 0.2,
                            0.3, 0.4, 0.5, 1.6 };

    const double v1[4] = { -0.5, -0.25, 0.25, 0.0 };

    const double m2[16] = { 1.1, -0.1, -0.1, 0.0,
                            0.1,  0.9, -0.2, 0.0,
                            0.05, 0.0,  1.1, 0.0,
                            0.0,  0.0,  0.0, 1.0 };
    const double v2[4] = { -0.2, -0.1, -0.1, -0.2 };

    const float source[] = { 0.1f,  0.2f,  0.3f,   0.4f,
                            -0.1f, -0.2f, 50.0f, 123.4f,
                             1.0f,  1.0f,  1.0f,   1.0f };
    const float error = 1e-4f;

    const double base = 10.0;
    const double logSlope[3] = { 0.18, 0.18, 0.18 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    const double logOffset[3] = { 1.0, 1.0, 1.0 };

    // Combining ops
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m2, v2, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_EQUAL(ops.size(), 2);

        // No optimize: keep both matrix ops
        OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(ops.size(), 2);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: Combine 2 matrix ops
        OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
        OCIO_CHECK_EQUAL(ops.size(), 1);

        // apply op
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }

    // remove NoOp at the beginning
    {
        OCIO::OpRcPtrVec ops;
        // NoOp
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                        linSlope, linOffset,
                                        OCIO::TRANSFORM_DIR_FORWARD));

        OCIO_CHECK_EQUAL(ops.size(), 3);

        // No optimize: keep both all ops
        OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(ops.size(), 3);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: remove the no op
        OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<LogOp>");

        // apply ops
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }

    // remove NoOp in the middle
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, OCIO::TRANSFORM_DIR_FORWARD));
        // NoOp
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                        linSlope, linOffset,
                                        OCIO::TRANSFORM_DIR_FORWARD));

        OCIO_CHECK_EQUAL(ops.size(), 3);

        // No optimize: keep both all ops
        OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(ops.size(), 3);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: remove the no op
        OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<LogOp>");

        // apply ops
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }

    // remove NoOp in the end
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                        linSlope, linOffset,
                                        OCIO::TRANSFORM_DIR_FORWARD));
        // NoOp
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));

        OCIO_CHECK_EQUAL(ops.size(), 3);

        // No optimize: keep both all ops
        OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(ops.size(), 3);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: remove the no op
        OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<LogOp>");

        // apply ops
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }

    // remove several NoOp
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                        linSlope, linOffset,
                                        OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));

        OCIO_CHECK_EQUAL(ops.size(), 9);

        // No optimize: keep both all ops
        OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(ops.size(), 9);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: remove the no op
        OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<LogOp>");

        // apply ops
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i < 12; ++i)
        {
            OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }
}

OCIO_ADD_TEST(CreateOpVecFromOpDataVec, basic)
{
    OCIO::ConstOpDataVec opDataVec;
    auto mat = OCIO::MatrixOpData::CreateDiagonalMatrix(2.0);
    opDataVec.push_back(mat);

    auto range = std::make_shared<OCIO::RangeOpData>(0.0, 1.0, 0.5, 1.5);

    opDataVec.push_back(range);

    OCIO_REQUIRE_EQUAL(opDataVec.size(), 2);
	
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::CreateOpVecFromOpDataVec(ops, opDataVec, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_REQUIRE_EQUAL(ops.size(), 2);

        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<RangeOp>");
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::CreateOpVecFromOpDataVec(ops, opDataVec, OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_REQUIRE_EQUAL(ops.size(), 2);

        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<RangeOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<MatrixOffsetOp>");
    }
}

OCIO_ADD_TEST(Op, non_dynamic_ops)
{
    double scale[4] = { 2.0, 2.0, 2.0, 1.0 };

    OCIO::OpRcPtrVec ops;
    OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    // Test that non-dynamic ops such as matrix respond properly to dynamic
    // property requests.
    OCIO_CHECK_ASSERT(!ops[0]->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));
    OCIO_CHECK_ASSERT(!ops[0]->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST));
    OCIO_CHECK_ASSERT(!ops[0]->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA));

    OCIO_CHECK_THROW_WHAT(ops[0]->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA),
                          OCIO::Exception, "does not implement dynamic property");
}

OCIO_ADD_TEST(OpRcPtrVec, erase_insert)
{
    OCIO::OpRcPtrVec ops;
    auto mat = OCIO::MatrixOpData::CreateDiagonalMatrix(1.1);
    mat->setID("First");
    OCIO::CreateMatrixOp(ops, mat, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    auto range = std::make_shared<OCIO::RangeOpData>(0.0, 1.0, 0.5, 1.5);

    OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    // Test push_back.
    mat = OCIO::MatrixOpData::CreateDiagonalMatrix(1.3);
    OCIO::CreateMatrixOp(ops, mat, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 3);

    // Test erase.
    ops.erase(ops.begin()+1);

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<MatrixOffsetOp>");

    // Test erase.
    OCIO::CreateLogOp(ops, 1.2, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateLogOp(ops, 1.1, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 5);

    ops.erase(ops.begin()+1, ops.begin()+4);

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<RangeOp>");

    // Test insert.
    OCIO::OpRcPtrVec ops1 = ops;

    OCIO_REQUIRE_EQUAL(ops1.size(), 2);

    ops1.insert(ops1.begin()+1, ops.begin(), ops.begin()+1);
    OCIO_REQUIRE_EQUAL(ops1.size(), 3);
    OCIO_CHECK_EQUAL(ops1[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_EQUAL(ops1[1]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_EQUAL(ops1[2]->getInfo(), "<RangeOp>");

    // Test operator +=.
    OCIO::OpRcPtrVec ops2 = ops;
    OCIO_REQUIRE_EQUAL(ops2.size(), 2);

    ops2 += ops2;

    OCIO_REQUIRE_EQUAL(ops2.size(), 4);
    OCIO_CHECK_EQUAL(ops2[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_EQUAL(ops2[1]->getInfo(), "<RangeOp>");
    OCIO_CHECK_EQUAL(ops2[2]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_EQUAL(ops2[3]->getInfo(), "<RangeOp>");
}

OCIO_ADD_TEST(OpData, equality)
{
    auto mat1 = OCIO::MatrixOpData::CreateDiagonalMatrix(1.1);
    auto mat2 = mat1->clone();

    // Use the MatrixOpData::operator==().
    OCIO_CHECK_ASSERT(*mat2==*mat1);

    auto range = std::make_shared<OCIO::RangeOpData>(0.0, 1.0, 0.5, 1.5);

    // Use the MatrixOpData::operator==().
    OCIO_CHECK_ASSERT( !(*mat2==*range) );

    // Use the RangeOpData::operator==().
    OCIO_CHECK_ASSERT( !(*range==*mat1) );

    // Use the OpData::operator==().
    auto op1 = OCIO::DynamicPtrCast<OCIO::OpData>(range);
    OCIO_CHECK_ASSERT( !(*op1==*mat1) );    

    // Use the OpData::operator==().
    auto op2 = OCIO::DynamicPtrCast<OCIO::OpData>(mat2);
    OCIO_CHECK_ASSERT( !(*op2==*op1) );

    // Change something.

    // Use the MatrixOpData::operator==().
    OCIO_CHECK_ASSERT( *mat2==*mat1 );

    // Use the OpData::operator==().
    OCIO_CHECK_ASSERT( *op2==*mat1 );

    mat2->setOffsetValue(1, mat2->getOffsetValue(1) + 1.0);

    // Use the MatrixOpData::operator==().
    OCIO_CHECK_ASSERT( !(*mat2==*mat1) );

    // Use the OpData::operator==().
    OCIO_CHECK_ASSERT( !(*op2==*mat1) );
}

