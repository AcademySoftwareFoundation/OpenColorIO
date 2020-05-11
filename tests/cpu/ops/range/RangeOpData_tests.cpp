// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/range/RangeOpData.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(RangeOpData, accessors)
{
    {
    OCIO::RangeOpData r;

    OCIO_CHECK_ASSERT(OCIO::IsNan((float)r.getMinInValue()));
    OCIO_CHECK_ASSERT(OCIO::IsNan((float)r.getMaxInValue()));
    OCIO_CHECK_ASSERT(OCIO::IsNan((float)r.getMinOutValue()));
    OCIO_CHECK_ASSERT(OCIO::IsNan((float)r.getMaxOutValue()));

    // Empty range is not valid.
    OCIO_CHECK_ASSERT(!r.isNoOp());
    OCIO_CHECK_ASSERT(r.isIdentity());
    OCIO_CHECK_THROW_WHAT(r.validate(), OCIO::Exception,
                          "At least minimum or maximum limits");

    double minVal = 1.0;
    double maxVal = 10.0;
    r.setMinInValue(minVal);
    r.setMaxInValue(maxVal);
    r.setMinOutValue(2.0*minVal);
    r.setMaxOutValue(2.0*maxVal);

    OCIO_CHECK_EQUAL(r.getMinInValue(), minVal);
    OCIO_CHECK_EQUAL(r.getMaxInValue(), maxVal);
    OCIO_CHECK_EQUAL(r.getMinOutValue(), 2.0*minVal);
    OCIO_CHECK_EQUAL(r.getMaxOutValue(), 2.0*maxVal);

    OCIO_CHECK_EQUAL(r.getType(), OCIO::OpData::RangeType);
    }

    {
    const float g_error = 1e-7f;

    OCIO::RangeOpData range(0.0, 1.0, 0.5, 1.5);

    OCIO_CHECK_EQUAL(range.getMinInValue(), 0.);
    OCIO_CHECK_EQUAL(range.getMaxInValue(), 1.);
    OCIO_CHECK_EQUAL(range.getMinOutValue(), 0.5);
    OCIO_CHECK_EQUAL(range.getMaxOutValue(), 1.5);

    OCIO_CHECK_NO_THROW(range.setMinInValue(-0.05432));
    OCIO_CHECK_NO_THROW(range.validate());
    OCIO_CHECK_EQUAL(range.getMinInValue(), -0.05432);

    OCIO_CHECK_NO_THROW(range.setMaxInValue(1.05432));
    OCIO_CHECK_NO_THROW(range.validate());
    OCIO_CHECK_EQUAL(range.getMaxInValue(), 1.05432);

    OCIO_CHECK_NO_THROW(range.setMinOutValue(0.05432));
    OCIO_CHECK_NO_THROW(range.validate());
    OCIO_CHECK_EQUAL(range.getMinOutValue(), 0.05432);

    OCIO_CHECK_NO_THROW(range.setMaxOutValue(2.05432));
    OCIO_CHECK_NO_THROW(range.validate());
    OCIO_CHECK_EQUAL(range.getMaxOutValue(), 2.05432);

    OCIO_CHECK_CLOSE(range.getScale(), 1.804012123, g_error);
    OCIO_CHECK_CLOSE(range.getOffset(), 0.1523139385, g_error);
    }

    {
    OCIO::RangeOpData range(0.0, 1.0, 0.5, 1.5);

    OCIO_CHECK_EQUAL(range.getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(range.getFileInputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);
    OCIO_CHECK_EQUAL(range.getFileOutputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);

    // Set file bit-depth and verify.
    OCIO_CHECK_NO_THROW(range.setFileInputBitDepth(OCIO::BIT_DEPTH_UINT8));
    OCIO_CHECK_NO_THROW(range.setFileOutputBitDepth(OCIO::BIT_DEPTH_F32));

    OCIO_CHECK_EQUAL(range.getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(range.getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    // Changing direction does not change values.
    OCIO_CHECK_NO_THROW(range.setDirection(OCIO::TRANSFORM_DIR_INVERSE));

    OCIO_CHECK_EQUAL(range.getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(range.getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(range.getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OCIO_CHECK_EQUAL(range.getMinInValue(), 0.);
    OCIO_CHECK_EQUAL(range.getMaxInValue(), 1.);
    OCIO_CHECK_EQUAL(range.getMinOutValue(), 0.5);
    OCIO_CHECK_EQUAL(range.getMaxOutValue(), 1.5);

    // The getAsForward swaps the bit-depths and the values.
    const auto r = range.getAsForward();
    OCIO_CHECK_EQUAL(r->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(r->getFileInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(r->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    OCIO_CHECK_EQUAL(r->getMinInValue(), 0.5);
    OCIO_CHECK_EQUAL(r->getMaxInValue(), 1.5);
    OCIO_CHECK_EQUAL(r->getMinOutValue(), 0.);
    OCIO_CHECK_EQUAL(r->getMaxOutValue(), 1.);
    }
}

OCIO_ADD_TEST(RangeOpData, range_identity)
{
    OCIO::RangeOpData r1(0., 1., 0., 1. );
    OCIO_CHECK_ASSERT( r1.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( r1.isIdentity() );
    OCIO_CHECK_ASSERT( ! r1.isClampNegs() );

    OCIO::RangeOpData r2(0.1, 1.2, -0.5, 2. );
    OCIO_CHECK_ASSERT( ! r2.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( ! r2.isIdentity() );
    OCIO_CHECK_ASSERT( ! r2.isClampNegs() );

    OCIO::RangeOpData r3(-0.1, 1.0, -0.5, 2. );
    OCIO_CHECK_ASSERT( ! r3.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( ! r3.isIdentity() );
    OCIO_CHECK_ASSERT( ! r3.isClampNegs() );

    OCIO::RangeOpData r4(0., 1., 0.01, 1. );
    OCIO_CHECK_ASSERT( r4.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( ! r4.isIdentity() );
    OCIO_CHECK_ASSERT( ! r4.isClampNegs() );

    OCIO::RangeOpData r5( 0.1, 1., -0.01, 1. );
    OCIO_CHECK_ASSERT( r5.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( ! r5.isIdentity() );
    OCIO_CHECK_ASSERT( ! r5.isClampNegs() );

    OCIO::RangeOpData r6(-0.1, 1.1, -0.1, 1.1 );
    OCIO_CHECK_ASSERT( ! r6.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( r6.isIdentity() );
    OCIO_CHECK_ASSERT( ! r6.isClampNegs() );

    OCIO::RangeOpData r7(0., OCIO::RangeOpData::EmptyValue(), 
                         0., OCIO::RangeOpData::EmptyValue());
    OCIO_CHECK_ASSERT( ! r7.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( r7.isIdentity() );
    OCIO_CHECK_ASSERT( r7.isClampNegs() );

    OCIO::RangeOpData r8(OCIO::RangeOpData::EmptyValue(), 1., 
                         OCIO::RangeOpData::EmptyValue(), 1. );
    OCIO_CHECK_ASSERT( ! r8.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( r8.isIdentity() );
    OCIO_CHECK_ASSERT( ! r8.isClampNegs() );
}

OCIO_ADD_TEST(RangeOpData, identity)
{
    OCIO::RangeOpData r4(0.,
                         OCIO::RangeOpData::EmptyValue(),
                         0.,
                         OCIO::RangeOpData::EmptyValue() );
    OCIO_CHECK_ASSERT(r4.isIdentity());
    OCIO_CHECK_ASSERT(!r4.isNoOp());
    OCIO_CHECK_ASSERT(!r4.hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(!r4.scales());
    OCIO_CHECK_ASSERT(!r4.minIsEmpty());
    OCIO_CHECK_ASSERT(r4.maxIsEmpty());

    OCIO::RangeOpData r5(0., 1., 0., 1. );
    OCIO_CHECK_ASSERT(!r5.scales());
    OCIO_CHECK_ASSERT(r5.isIdentity());
    OCIO_CHECK_ASSERT(!r5.hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(!r5.isNoOp());
    OCIO_CHECK_ASSERT(!r5.minIsEmpty());
    OCIO_CHECK_ASSERT(!r5.maxIsEmpty());

    OCIO::RangeOpData r6(0., 1., -1., 1. );
    OCIO_CHECK_ASSERT(!r6.isIdentity());
    OCIO_CHECK_ASSERT(!r6.isNoOp());
    OCIO_CHECK_ASSERT(!r6.hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(!r6.minIsEmpty());
    OCIO_CHECK_ASSERT(!r6.maxIsEmpty());
    OCIO_CHECK_EQUAL(r6.getMinOutValue(), -1.);
    OCIO_CHECK_EQUAL(r6.getMaxOutValue(), 1.);
    OCIO_CHECK_ASSERT(r6.scales());
}

OCIO_ADD_TEST(RangeOpData, equality)
{
    OCIO::RangeOpData r1(0., 1., -1., 1. );

    OCIO::RangeOpData r2(0.123, 1., -1., 1. );

    OCIO_CHECK_ASSERT(!(r1 == r2));

    OCIO::RangeOpData r3(0., 0.99, -1., 1. );

    OCIO_CHECK_ASSERT(!(r1 == r3));

    OCIO::RangeOpData r4(0., 1., -12., 1. );

    OCIO_CHECK_ASSERT(!(r1 == r4));

    OCIO::RangeOpData r5(0., 1., -1., 1. );

    OCIO_CHECK_ASSERT(r5 == r1);
}

OCIO_ADD_TEST(RangeOpData, validation)
{
    {
        OCIO::RangeOpData r;

        r.setMinInValue(16.);
        r.setMaxInValue(235.);
        // Leave min output empty.
        r.setMaxOutValue(2.);

        OCIO_CHECK_THROW_WHAT(r.validate(),
                              OCIO::Exception,
                              "must be both set or both missing");
    }

    {
        OCIO::RangeOpData r;

        r.setMinInValue(0.0);
        r.setMinOutValue(0.00001);

        OCIO_CHECK_THROW_WHAT(r.validate(),
                              OCIO::Exception,
                              "In and out minimum limits must be equal");
    }

    {
        OCIO::RangeOpData range(0.0, 1.0, 0.5, 1.5);
        OCIO_CHECK_NO_THROW(range.validate());

        OCIO_CHECK_NO_THROW(range.unsetMinInValue()); 
        OCIO_CHECK_THROW_WHAT(range.validate(), 
                              OCIO::Exception,
                              "In and out minimum limits must be both set or both missing");
    }

    {
        OCIO::RangeOpData range(0.0, 1.0, 0.5, 1.5);
        OCIO_CHECK_NO_THROW(range.validate());

        OCIO_CHECK_NO_THROW(range.unsetMinInValue()); 
        OCIO_CHECK_NO_THROW(range.unsetMinOutValue()); 
        OCIO_CHECK_THROW_WHAT(range.validate(), OCIO::Exception,
                              "In and out maximum limits must be equal");
        OCIO_CHECK_NO_THROW(range.setMaxInValue(range.getMaxOutValue()));
        OCIO_CHECK_NO_THROW(range.validate());
    }

    {
        OCIO::RangeOpData range(0.0, 1.0, 0.5, 1.5);
        OCIO_CHECK_NO_THROW(range.validate());

        OCIO_CHECK_NO_THROW(range.unsetMaxInValue()); 
        OCIO_CHECK_THROW_WHAT(range.validate(), 
                              OCIO::Exception,
                              "In and out maximum limits must be both set or both missing");
    }

    {
        OCIO::RangeOpData range(0.0, 1.0, 0.5, 1.5);
        OCIO_CHECK_NO_THROW(range.validate());

        OCIO_CHECK_NO_THROW(range.unsetMaxInValue()); 
        OCIO_CHECK_NO_THROW(range.unsetMaxOutValue()); 
        OCIO_CHECK_THROW_WHAT(range.validate(), OCIO::Exception,
                              "In and out minimum limits must be equal");
        OCIO_CHECK_NO_THROW(range.setMinInValue(range.getMinOutValue()));
        OCIO_CHECK_NO_THROW(range.validate());
    }

    {
        OCIO::RangeOpData range(0.0, 1.0, 0.5, 1.5);
        OCIO_CHECK_NO_THROW(range.validate());

        OCIO_CHECK_NO_THROW(range.setMaxInValue(-125.)); 
        OCIO_CHECK_THROW_WHAT(range.validate(), 
                              OCIO::Exception,
                              "Range maximum input value is less than minimum input value");
    }

    {
        OCIO::RangeOpData range(0.0, 1.0, 0.5, 1.5);
        OCIO_CHECK_NO_THROW(range.validate());

        OCIO_CHECK_NO_THROW(range.setMaxOutValue(-125.)); 
        OCIO_CHECK_THROW_WHAT(range.validate(), 
                              OCIO::Exception,
                              "Range maximum output value is less than minimum output value");
    }
}

namespace
{
void checkInverse(double fwdMinIn,  double fwdMaxIn,
                  double fwdMinOut, double fwdMaxOut,
                  double revMinIn,  double revMaxIn,
                  double revMinOut, double revMaxOut)
{
    OCIO::RangeOpData refOp(fwdMinIn, fwdMaxIn, fwdMinOut, fwdMaxOut,
                            OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::RangeOpDataRcPtr invOp = refOp.getAsForward();

    // The min/max values should be swapped.
    if (OCIO::IsNan(revMinIn))
        OCIO_CHECK_ASSERT(OCIO::IsNan(invOp->getMinInValue()));
    else
        OCIO_CHECK_EQUAL(invOp->getMinInValue(), revMinIn);
    if (OCIO::IsNan(revMaxIn))
        OCIO_CHECK_ASSERT(OCIO::IsNan(invOp->getMaxInValue()));
    else
        OCIO_CHECK_EQUAL(invOp->getMaxInValue(), revMaxIn);
    if (OCIO::IsNan(revMinOut))
        OCIO_CHECK_ASSERT(OCIO::IsNan(invOp->getMinOutValue()));
    else
        OCIO_CHECK_EQUAL(invOp->getMinOutValue(), revMinOut);
    if (OCIO::IsNan(revMaxOut))
        OCIO_CHECK_ASSERT(OCIO::IsNan(invOp->getMaxOutValue()));
    else
        OCIO_CHECK_EQUAL(invOp->getMaxOutValue(), revMaxOut);

    // Check that the computation would be correct.
    // (Doing this in lieu of renderer testing.)

    const float fwdScale  = (float)refOp.getScale();
    const float fwdOffset = (float)refOp.getOffset();
    const float revScale  = (float)invOp->getScale();
    const float revOffset = (float)invOp->getOffset();

    // Want in == (in * fwdScale + fwdOffset) * revScale + revOffset
    // in == in * fwdScale * revScale + fwdOffset * revScale + revOffset
    // in == in * 1. + 0.
    OCIO_CHECK_ASSERT(!OCIO::FloatsDiffer( 1.0f, fwdScale * revScale, 10, false));

    //OCIO_CHECK_ASSERT(!OCIO::floatsDiffer(
    //     0.0f, fwdOffset * revScale + revOffset, 100000, false));
    // The above is correct but we lose too much precision in the subtraction
    // so rearrange the compare as follows to allow a tighter tolerance.
    OCIO_CHECK_ASSERT(!OCIO::FloatsDiffer(fwdOffset * revScale, -revOffset, 500, false));
}
}

OCIO_ADD_TEST(RangeOpData, inverse)
{
    // Results in scale != 1 and offset != 0.
    checkInverse(0.064, 0.940, 0.032, 0.235,
                 0.032, 0.235, 0.064, 0.940);

    // Note: All the following result in clipping only.

    checkInverse(OCIO::RangeOpData::EmptyValue(), 0.235, OCIO::RangeOpData::EmptyValue(), 0.235,
                 OCIO::RangeOpData::EmptyValue(), 0.235, OCIO::RangeOpData::EmptyValue(), 0.235);

    checkInverse(0.64, OCIO::RangeOpData::EmptyValue(), 0.64, OCIO::RangeOpData::EmptyValue(),
                 0.64, OCIO::RangeOpData::EmptyValue(), 0.64, OCIO::RangeOpData::EmptyValue());
}

OCIO_ADD_TEST(RangeOpData, compose)
{
    OCIO::RangeOpData r1(0., 1., 0., 1.);
    OCIO::ConstRangeOpDataRcPtr r2 = std::make_shared<OCIO::RangeOpData>(0.1, 0.9, 0.1, 0.9);

    auto res = r1.compose(r2);
    OCIO_CHECK_EQUAL(res->getMinInValue(),  0.1);
    OCIO_CHECK_EQUAL(res->getMaxInValue(),  0.9);
    OCIO_CHECK_EQUAL(res->getMinOutValue(), 0.1);
    OCIO_CHECK_EQUAL(res->getMaxOutValue(), 0.9);

    OCIO::ConstRangeOpDataRcPtr r3 = std::make_shared<OCIO::RangeOpData>(0.1, 1.9, 0.1, 1.9);
    res = r1.compose(r3);
    OCIO_CHECK_EQUAL(res->getMinInValue(),  0.1);
    OCIO_CHECK_EQUAL(res->getMaxInValue(),  1.0);
    OCIO_CHECK_EQUAL(res->getMinOutValue(), 0.1);
    OCIO_CHECK_EQUAL(res->getMaxOutValue(), 1.0);

    OCIO::ConstRangeOpDataRcPtr r4 = std::make_shared<OCIO::RangeOpData>(0.1, 1.9, 0.2, 1.8);
    res = r1.compose(r4);
    OCIO_CHECK_EQUAL(res->getMinInValue(), 0.1);
    OCIO_CHECK_EQUAL(res->getMaxInValue(), 1.0);
    OCIO_CHECK_EQUAL(res->getMinOutValue(), 0.2);
    OCIO_CHECK_CLOSE(res->getMaxOutValue(), 1.0, 1e-15);

    OCIO::ConstRangeOpDataRcPtr r6 = std::make_shared<OCIO::RangeOpData>(-1.0, 1.0, 0, 1.2);
    res = r1.compose(r6);
    OCIO_CHECK_EQUAL(res->getMinInValue(),  0.);
    OCIO_CHECK_EQUAL(res->getMaxInValue(),  1.);
    OCIO_CHECK_EQUAL(res->getMinOutValue(), 0.6);
    OCIO_CHECK_EQUAL(res->getMaxOutValue(), 1.2);

    OCIO::ConstRangeOpDataRcPtr r7 = std::make_shared<OCIO::RangeOpData>(
        OCIO::RangeOpData::EmptyValue(), 0.5,
        OCIO::RangeOpData::EmptyValue(), 0.5);

    res = r7->compose(r4);
    OCIO_CHECK_EQUAL(res->getMinInValue(),  0.1);
    OCIO_CHECK_EQUAL(res->getMaxInValue(),  0.5);
    OCIO_CHECK_EQUAL(res->getMinOutValue(), 0.2);
    OCIO_CHECK_CLOSE(res->getMaxOutValue(), (0.5 * 1.6 + 0.2 * 1.8 - 0.1 * 1.6) / 1.8, 1e-15);

    res = r4->compose(r7);
    OCIO_CHECK_EQUAL(res->getMinInValue(),  0.1);
    OCIO_CHECK_CLOSE(res->getMaxInValue(),  0.4375, 1e-15);
    OCIO_CHECK_EQUAL(res->getMinOutValue(), 0.2);
    OCIO_CHECK_EQUAL(res->getMaxOutValue(), 0.5);

    OCIO::ConstRangeOpDataRcPtr r8 = std::make_shared<OCIO::RangeOpData>(
        0.5, OCIO::RangeOpData::EmptyValue(),
        0.5, OCIO::RangeOpData::EmptyValue());

    res = r8->compose(r3);
    OCIO_CHECK_EQUAL(res->getMinInValue(),  0.5);
    OCIO_CHECK_EQUAL(res->getMaxInValue(),  1.9);
    OCIO_CHECK_EQUAL(res->getMinOutValue(), 0.5);
    OCIO_CHECK_EQUAL(res->getMaxOutValue(), 1.9);

    res = r4->compose(r8);
    OCIO_CHECK_CLOSE(res->getMinInValue(),  0.4375, 1e-15);
    OCIO_CHECK_EQUAL(res->getMaxInValue(),  1.9);
    OCIO_CHECK_EQUAL(res->getMinOutValue(), 0.5);
    OCIO_CHECK_EQUAL(res->getMaxOutValue(), 1.8);

    OCIO::ConstRangeOpDataRcPtr r9 = std::make_shared<OCIO::RangeOpData>(1.1, 1.9, 1.2, 1.5);
    res = r1.compose(r9);
    OCIO_CHECK_EQUAL(res->getMinInValue(),  0.0);
    OCIO_CHECK_EQUAL(res->getMaxInValue(),  1.0);
    OCIO_CHECK_EQUAL(res->getMinOutValue(), 1.2);
    OCIO_CHECK_EQUAL(res->getMaxOutValue(), 1.2);

    OCIO::ConstRangeOpDataRcPtr r10 = std::make_shared<OCIO::RangeOpData>(-1.1, -0.1, 1.1, 1.9);
    res = r1.compose(r10);
    OCIO_CHECK_EQUAL(res->getMinInValue(), 0.);
    OCIO_CHECK_EQUAL(res->getMaxInValue(), 1.);
    OCIO_CHECK_EQUAL(res->getMinOutValue(), 1.9);
    OCIO_CHECK_EQUAL(res->getMaxOutValue(), 1.9);
}

OCIO_ADD_TEST(RangeOpData, computed_identifier)
{
    std::string id1, id2;

    OCIO::RangeOpData range(0.0, 1.0, 0.5, 1.5);
    OCIO_CHECK_NO_THROW( id1 = range.getCacheID() );

    OCIO_CHECK_NO_THROW( range.unsetMaxInValue() ); 
    OCIO_CHECK_NO_THROW( range.unsetMaxOutValue() ); 
    OCIO_CHECK_NO_THROW( range.setMinOutValue(range.getMinInValue()) );
    OCIO_CHECK_NO_THROW( id2 = range.getCacheID() );

    OCIO_CHECK_ASSERT( id1 != id2 );
    OCIO_CHECK_NO_THROW( id1 = range.getCacheID() );
    OCIO_CHECK_ASSERT( id1 == id2 );
}

