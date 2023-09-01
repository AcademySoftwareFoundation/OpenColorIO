// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/log/LogOp.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(LogOp, lin_to_log)
{
    const double base = 10.0;
    const double logSlope[3] = { 0.18, 0.18, 0.18 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    const double logOffset[3] = { 1.0, 1.0, 1.0 };

    float data[8] = { 0.01f, 0.1f, 1.0f, 1.0f,
                      10.0f, 100.0f, 1000.0f, 1.0f, };

    const float result[8] = { 0.8342526242885725f,
                              0.90588182584953925f,
                              1.057999473052105462f,
                              1.0f,
                              1.23457529033568797f,
                              1.41422447595451795f,
                              1.59418930777214063f,
                              1.0f };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, base, logSlope, logOffset,
                                          linSlope, linOffset,
                                          OCIO::TRANSFORM_DIR_FORWARD));

    // One operator has been created.
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT((bool)ops[0]);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));

    // Validate properties.
    std::string opCache;
    OCIO_CHECK_NO_THROW(opCache = ops[0]->getCacheID());
    OCIO_CHECK_NE(opCache.size(), 0);
    OCIO_CHECK_EQUAL(ops[0]->isNoOp(), false);
    OCIO_CHECK_EQUAL(ops[0]->hasChannelCrosstalk(), false);

    // Apply the result.
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 2);
    }

    for(int i=0; i<8; ++i)
    {
        OCIO_CHECK_CLOSE( data[i], result[i], 1.0e-3 );
    }
}

OCIO_ADD_TEST(LogOp, log_to_lin)
{
    const double base = 10.0;
    const double logSlope[3] = { 0.18, 0.18, 0.18 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    const double logOffset[3] = { 1.0, 1.0, 1.0 };

    float data[8] = { 0.8342526242885725f,
                      0.90588182584953925f,
                      1.057999473052105462f,
                      1.0f,
                      1.23457529033568797f,
                      1.41422447595451795f,
                      1.59418930777214063f,
                      1.0f };

    const float result[8] = { 0.01f, 0.1f, 1.0f, 1.0f,
                              10.0f, 100.0f, 1000.0f, 1.0f, };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, base, logSlope, logOffset,
                                          linSlope, linOffset,
                                          OCIO::TRANSFORM_DIR_INVERSE));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));

    // Apply the result.
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 2);
    }

    for(int i=0; i<8; ++i)
    {
        OCIO_CHECK_CLOSE( data[i], result[i], 2.0e-3f );
    }
}

OCIO_ADD_TEST(LogOp, inverse)
{
    double base = 10.0;
    const double logSlope[3] = { 0.5, 0.5, 0.5 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    const double logOffset[3] = { 1.0, 1.0, 1.0 };
    const double logSlope2[3] = { 0.5, 1.0, 1.5 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                                          OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                                          OCIO::TRANSFORM_DIR_INVERSE));

    base += 1.0;
    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                                          OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                                          OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, base, logSlope2, logOffset, linSlope, linOffset,
                                          OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, base, logSlope2, logOffset, linSlope, linOffset,
                                          OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 6);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];
    OCIO::ConstOpRcPtr op3 = ops[3];
    OCIO::ConstOpRcPtr op4 = ops[4];
    OCIO::ConstOpRcPtr op5 = ops[5];

    OCIO_CHECK_ASSERT(ops[0]->isSameType(op1));
    OCIO_CHECK_ASSERT(ops[0]->isSameType(op2));
    OCIO::ConstOpRcPtr op3Cloned = ops[3]->clone();
    OCIO_CHECK_ASSERT(ops[0]->isSameType(op3Cloned));

    OCIO_CHECK_EQUAL(ops[0]->isInverse(op0), false);
    OCIO_CHECK_EQUAL(ops[0]->isInverse(op1), true);
    OCIO_CHECK_EQUAL(ops[0]->isInverse(op2), false);
    OCIO_CHECK_EQUAL(ops[0]->isInverse(op3), false);

    OCIO_CHECK_EQUAL(ops[1]->isInverse(op0), true);
    OCIO_CHECK_EQUAL(ops[1]->isInverse(op2), false);
    OCIO_CHECK_EQUAL(ops[1]->isInverse(op3), false);

    OCIO_CHECK_EQUAL(ops[2]->isInverse(op2), false);
    OCIO_CHECK_EQUAL(ops[2]->isInverse(op3), true);

    OCIO_CHECK_EQUAL(ops[3]->isInverse(op3), false);

    // When r, g & b are not equal, ops are not considered inverse 
    // even though they are.
    OCIO_CHECK_EQUAL(ops[4]->isInverse(op5), false);

    const float result[12] = { 0.01f, 0.1f, 1.0f, 1.0f,
                               1.0f, 10.0f, 100.0f, 1.0f,
                               1000.0f, 1.0f, 0.5f, 1.0f };
    float data[12];

    for(int i=0; i<12; ++i)
    {
        data[i] = result[i];
    }

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_NONE));

    ops[0]->apply(data, 3);
    // Note: Skip testing alpha channels.
    OCIO_CHECK_NE( data[0], result[0] );
    OCIO_CHECK_NE( data[1], result[1] );
    OCIO_CHECK_NE( data[2], result[2] );
    OCIO_CHECK_NE( data[4], result[4] );
    OCIO_CHECK_NE( data[5], result[5] );
    OCIO_CHECK_NE( data[6], result[6] );
    OCIO_CHECK_NE( data[8], result[8] );
    OCIO_CHECK_NE( data[9], result[9] );
    OCIO_CHECK_NE( data[10], result[10] );

    ops[1]->apply(data, 3);

#if OCIO_USE_SSE2 == 0
    const float error = 1e-3f;
#else
    const float error = 1e-2f;
#endif // !OCIO_USE_SSE2

    for(int i=0; i<12; ++i)
    {
        OCIO_CHECK_CLOSE( data[i], result[i], error);
    }
}

OCIO_ADD_TEST(LogOp, cache_id)
{
    const double base = 10.0;
    const double logSlope[3] = { 0.18, 0.18, 0.18 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    double logOffset[3] = { 1.0, 1.0, 1.0 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, base, logSlope, logOffset,
                                          linSlope, linOffset,
                                          OCIO::TRANSFORM_DIR_FORWARD));
    logOffset[0] += 1.0f;
    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, base, logSlope, logOffset,
                                          linSlope, linOffset,
                                          OCIO::TRANSFORM_DIR_FORWARD));
    logOffset[0] -= 1.0f;
    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, base, logSlope, logOffset,
                                          linSlope, linOffset,
                                          OCIO::TRANSFORM_DIR_FORWARD));

    // 3 operators have been created
    OCIO_CHECK_EQUAL(ops.size(), 3);
    OCIO_REQUIRE_ASSERT((bool)ops[0]);
    OCIO_REQUIRE_ASSERT((bool)ops[1]);
    OCIO_REQUIRE_ASSERT((bool)ops[2]);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));

    std::string opCacheID0;
    OCIO_CHECK_NO_THROW(opCacheID0 = ops[0]->getCacheID());
    std::string opCacheID1;
    OCIO_CHECK_NO_THROW(opCacheID1 = ops[1]->getCacheID());
    std::string opCacheID2;
    OCIO_CHECK_NO_THROW(opCacheID2 = ops[2]->getCacheID());
    OCIO_CHECK_EQUAL(opCacheID0, opCacheID2);
    OCIO_CHECK_NE(opCacheID0, opCacheID1);
}

OCIO_ADD_TEST(LogOp, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;

    const double base = 1.0;
    const double logSlope[] = { 1.5, 1.6, 1.7 };
    const double linSlope[] = { 1.1, 1.2, 1.3 };
    const double linOffset[] = { 1.0, 2.0, 3.0 };
    const double logOffset[] = { 10.0, 20.0, 30.0 };

    OCIO::LogOpDataRcPtr log = std::make_shared<OCIO::LogOpData>(base, logSlope, logOffset,
                                                                 linSlope, linOffset, direction);

    auto & metadataSource = log->getFormatMetadata();
    metadataSource.addAttribute("name", "test");

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, log, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateLogTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto lTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::LogAffineTransform>(transform);
    OCIO_REQUIRE_ASSERT(lTransform);

    const auto & metadata = lTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeName(0)), "name");
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeValue(0)), "test");

    OCIO_CHECK_EQUAL(lTransform->getDirection(), direction);
    OCIO_CHECK_EQUAL(lTransform->getBase(), base);
    double values[3]{ 0.0 };
    lTransform->getLogSideSlopeValue(values);
    OCIO_CHECK_EQUAL(values[0], logSlope[0]);
    OCIO_CHECK_EQUAL(values[1], logSlope[1]);
    OCIO_CHECK_EQUAL(values[2], logSlope[2]);
    lTransform->getLogSideOffsetValue(values);
    OCIO_CHECK_EQUAL(values[0], logOffset[0]);
    OCIO_CHECK_EQUAL(values[1], logOffset[1]);
    OCIO_CHECK_EQUAL(values[2], logOffset[2]);
    lTransform->getLinSideSlopeValue(values);
    OCIO_CHECK_EQUAL(values[0], linSlope[0]);
    OCIO_CHECK_EQUAL(values[1], linSlope[1]);
    OCIO_CHECK_EQUAL(values[2], linSlope[2]);
    lTransform->getLinSideOffsetValue(values);
    OCIO_CHECK_EQUAL(values[0], linOffset[0]);
    OCIO_CHECK_EQUAL(values[1], linOffset[1]);
    OCIO_CHECK_EQUAL(values[2], linOffset[2]);

    const double linBreak[] = { 0.5, 0.4, 0.3 };
    log->setValue(OCIO::LIN_SIDE_BREAK, linBreak);

    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, log, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_REQUIRE_ASSERT(ops[1]);

    OCIO::GroupTransformRcPtr group1 = OCIO::GroupTransform::Create();
    OCIO::ConstOpRcPtr op1(ops[1]);

    OCIO::CreateLogTransform(group1, op);
    OCIO_REQUIRE_EQUAL(group1->getNumTransforms(), 1);
    auto transform1 = group1->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform1);
    auto lTransform1 = OCIO_DYNAMIC_POINTER_CAST<OCIO::LogCameraTransform>(transform1);
    OCIO_REQUIRE_ASSERT(lTransform1);
    lTransform1->getLinSideBreakValue(values);
    OCIO_CHECK_EQUAL(values[0], linBreak[0]);
    OCIO_CHECK_EQUAL(values[1], linBreak[1]);
    OCIO_CHECK_EQUAL(values[2], linBreak[2]);
    OCIO_CHECK_ASSERT(!lTransform1->getLinearSlopeValue(values));

    const double linearSlope[] = { 0.9, 1.0, 1.1 };
    log->setValue(OCIO::LINEAR_SLOPE, linearSlope);

    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, log, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_REQUIRE_ASSERT(ops[2]);

    OCIO::GroupTransformRcPtr group2 = OCIO::GroupTransform::Create();
    OCIO::ConstOpRcPtr op2(ops[2]);

    OCIO::CreateLogTransform(group2, op);
    OCIO_REQUIRE_EQUAL(group2->getNumTransforms(), 1);
    auto transform2 = group2->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform2);
    auto lTransform2 = OCIO_DYNAMIC_POINTER_CAST<OCIO::LogCameraTransform>(transform2);
    OCIO_REQUIRE_ASSERT(lTransform2);
    lTransform1->getLinSideBreakValue(values);
    OCIO_CHECK_EQUAL(values[0], linBreak[0]);
    OCIO_CHECK_EQUAL(values[1], linBreak[1]);
    OCIO_CHECK_EQUAL(values[2], linBreak[2]);
    OCIO_CHECK_ASSERT(lTransform2->getLinearSlopeValue(values));
    OCIO_CHECK_EQUAL(values[0], linearSlope[0]);
    OCIO_CHECK_EQUAL(values[1], linearSlope[1]);
    OCIO_CHECK_EQUAL(values[2], linearSlope[2]);
}

