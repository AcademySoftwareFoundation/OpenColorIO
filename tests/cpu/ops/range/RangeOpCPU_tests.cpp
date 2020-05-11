// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <limits>

#include "ops/range/RangeOpCPU.cpp"

#include "utils/StringUtils.h"
#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
static constexpr float g_error = 1e-7f;
}

OCIO_ADD_TEST(RangeOpCPU, identity)
{
    OCIO::RangeOpDataRcPtr range = std::make_shared<OCIO::RangeOpData>();
    range->setMinInValue(0.);
    range->setMinOutValue(0.);
    OCIO_CHECK_NO_THROW(range->validate());
    OCIO_CHECK_ASSERT(range->isIdentity());
    OCIO_CHECK_ASSERT(!range->isNoOp());

    OCIO::ConstRangeOpDataRcPtr r = range;

    OCIO::ConstOpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "RangeMinRenderer"));
}

OCIO_ADD_TEST(RangeOpCPU, scale_with_low_and_high_clippings)
{
    OCIO::RangeOpDataRcPtr range = std::make_shared<OCIO::RangeOpData>(0., 1., 0.5, 1.5);

    OCIO_CHECK_NO_THROW(range->validate());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::ConstOpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "RangeScaleMinMaxRenderer"));

    const long numPixels = 9;
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f,
                                   qnan,   qnan,  qnan, 0.0f,
                                   0.0f,   0.0f,  0.0f, qnan,
                                    inf,    inf,   inf, 0.0f,
                                   0.0f,   0.0f,  0.0f,  inf,
                                   -inf,   -inf,  -inf, 0.0f,
                                   0.0f,   0.0f,  0.0f, -inf };

    OCIO_CHECK_NO_THROW(op->apply(&image[0], &image[0], numPixels));

    OCIO_CHECK_CLOSE(image[0],  0.50f, g_error);
    OCIO_CHECK_CLOSE(image[1],  0.50f, g_error);
    OCIO_CHECK_CLOSE(image[2],  1.00f, g_error);
    OCIO_CHECK_CLOSE(image[3],  0.00f, g_error);

    OCIO_CHECK_CLOSE(image[4],  1.25f, g_error);
    OCIO_CHECK_CLOSE(image[5],  1.50f, g_error);
    OCIO_CHECK_CLOSE(image[6],  1.50f, g_error);
    OCIO_CHECK_CLOSE(image[7],  1.00f, g_error);

    OCIO_CHECK_CLOSE(image[8],  1.50f, g_error);
    OCIO_CHECK_CLOSE(image[9],  1.50f, g_error);
    OCIO_CHECK_CLOSE(image[10], 1.50f, g_error);
    OCIO_CHECK_CLOSE(image[11], 0.00f, g_error);

    OCIO_CHECK_EQUAL(image[12], 0.50f);
    OCIO_CHECK_EQUAL(image[13], 0.50f);
    OCIO_CHECK_EQUAL(image[14], 0.50f);
    OCIO_CHECK_EQUAL(image[15], 0.00f);

    OCIO_CHECK_EQUAL(image[16], 0.50f);
    OCIO_CHECK_EQUAL(image[17], 0.50f);
    OCIO_CHECK_EQUAL(image[18], 0.50f);
    OCIO_CHECK_ASSERT(OCIO::IsNan(image[19]));

    OCIO_CHECK_EQUAL(image[20], 1.50f);
    OCIO_CHECK_EQUAL(image[21], 1.50f);
    OCIO_CHECK_EQUAL(image[22], 1.50f);
    OCIO_CHECK_EQUAL(image[23], 0.0f);

    OCIO_CHECK_EQUAL(image[24], 0.50f);
    OCIO_CHECK_EQUAL(image[25], 0.50f);
    OCIO_CHECK_EQUAL(image[26], 0.50f);
    OCIO_CHECK_EQUAL(image[27], inf);

    OCIO_CHECK_EQUAL(image[28], 0.50f);
    OCIO_CHECK_EQUAL(image[29], 0.50f);
    OCIO_CHECK_EQUAL(image[30], 0.50f);
    OCIO_CHECK_EQUAL(image[31], 0.0f);

    OCIO_CHECK_EQUAL(image[32], 0.50f);
    OCIO_CHECK_EQUAL(image[33], 0.50f);
    OCIO_CHECK_EQUAL(image[34], 0.50f);
    OCIO_CHECK_EQUAL(image[35], -inf);
}

OCIO_ADD_TEST(RangeOpCPU, scale_with_low_and_high_clippings_2)
{
    OCIO::RangeOpDataRcPtr range = std::make_shared<OCIO::RangeOpData>(0., 1., 0., 1.5);

    OCIO_CHECK_NO_THROW(range->validate());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::ConstOpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "RangeScaleMinMaxRenderer"));

    const long numPixels = 3;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f };

    OCIO_CHECK_NO_THROW(op->apply(&image[0], &image[0], numPixels));

    OCIO_CHECK_CLOSE(image[0],  0.000f, g_error);
    OCIO_CHECK_CLOSE(image[1],  0.000f, g_error);
    OCIO_CHECK_CLOSE(image[2],  0.750f, g_error);
    OCIO_CHECK_CLOSE(image[3],  0.000f, g_error);

    OCIO_CHECK_CLOSE(image[4],  1.125f, g_error);
    OCIO_CHECK_CLOSE(image[5],  1.500f, g_error);
    OCIO_CHECK_CLOSE(image[6],  1.500f, g_error);
    OCIO_CHECK_CLOSE(image[7],  1.000f, g_error);

    OCIO_CHECK_CLOSE(image[8],  1.500f, g_error);
    OCIO_CHECK_CLOSE(image[9],  1.500f, g_error);
    OCIO_CHECK_CLOSE(image[10], 1.500f, g_error);
    OCIO_CHECK_CLOSE(image[11], 0.000f, g_error);
}

OCIO_ADD_TEST(RangeOpCPU, offset_with_low_and_high_clippings)
{
    OCIO::RangeOpDataRcPtr range = std::make_shared<OCIO::RangeOpData>(0., 1., 1., 2.);

    OCIO_CHECK_NO_THROW(range->validate());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::ConstOpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "RangeScaleMinMaxRenderer"));

    const long numPixels = 3;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f };

    OCIO_CHECK_NO_THROW(op->apply(&image[0], &image[0], numPixels));

    OCIO_CHECK_CLOSE(image[0],  1.00f, g_error);
    OCIO_CHECK_CLOSE(image[1],  1.00f, g_error);
    OCIO_CHECK_CLOSE(image[2],  1.50f, g_error);
    OCIO_CHECK_CLOSE(image[3],  0.00f, g_error);

    OCIO_CHECK_CLOSE(image[4],  1.75f, g_error);
    OCIO_CHECK_CLOSE(image[5],  2.00f, g_error);
    OCIO_CHECK_CLOSE(image[6],  2.00f, g_error);
    OCIO_CHECK_CLOSE(image[7],  1.00f, g_error);

    OCIO_CHECK_CLOSE(image[8],  2.00f, g_error);
    OCIO_CHECK_CLOSE(image[9],  2.00f, g_error);
    OCIO_CHECK_CLOSE(image[10], 2.00f, g_error);
    OCIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OCIO_ADD_TEST(RangeOpCPU, low_and_high_clippings)
{
    OCIO::RangeOpDataRcPtr range = std::make_shared<OCIO::RangeOpData>(1., 2., 1., 2.);

    OCIO_CHECK_NO_THROW(range->validate());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::ConstOpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "RangeMinMaxRenderer"));

    const long numPixels = 4;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f,
                                  2.00f,  2.50f, 2.75f, 1.0f };

    OCIO_CHECK_NO_THROW(op->apply(&image[0], &image[0], numPixels));

    OCIO_CHECK_CLOSE(image[0],  1.00f, g_error);
    OCIO_CHECK_CLOSE(image[1],  1.00f, g_error);
    OCIO_CHECK_CLOSE(image[2],  1.00f, g_error);
    OCIO_CHECK_CLOSE(image[3],  0.00f, g_error);

    OCIO_CHECK_CLOSE(image[4],  1.00f, g_error);
    OCIO_CHECK_CLOSE(image[5],  1.00f, g_error);
    OCIO_CHECK_CLOSE(image[6],  1.25f, g_error);
    OCIO_CHECK_CLOSE(image[7],  1.00f, g_error);

    OCIO_CHECK_CLOSE(image[8],  1.25f, g_error);
    OCIO_CHECK_CLOSE(image[9],  1.50f, g_error);
    OCIO_CHECK_CLOSE(image[10], 1.75f, g_error);
    OCIO_CHECK_CLOSE(image[11], 0.00f, g_error);

    OCIO_CHECK_CLOSE(image[12], 2.00f, g_error);
    OCIO_CHECK_CLOSE(image[13], 2.00f, g_error);
    OCIO_CHECK_CLOSE(image[14], 2.00f, g_error);
    OCIO_CHECK_CLOSE(image[15], 1.00f, g_error);
}

OCIO_ADD_TEST(RangeOpCPU, low_clipping)
{
    OCIO::RangeOpDataRcPtr range
        = std::make_shared<OCIO::RangeOpData>(-0.1, OCIO::RangeOpData::EmptyValue(), 
                                              -0.1, OCIO::RangeOpData::EmptyValue());

    OCIO_CHECK_NO_THROW(range->validate());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::ConstOpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "RangeMinRenderer"));

    const long numPixels = 3;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f };

    OCIO_CHECK_NO_THROW(op->apply(&image[0], &image[0], numPixels));

    OCIO_CHECK_CLOSE(image[0], -0.10f, g_error);
    OCIO_CHECK_CLOSE(image[1], -0.10f, g_error);
    OCIO_CHECK_CLOSE(image[2],  0.50f, g_error);
    OCIO_CHECK_CLOSE(image[3],  0.00f, g_error);

    OCIO_CHECK_CLOSE(image[4],  0.75f, g_error);
    OCIO_CHECK_CLOSE(image[5],  1.00f, g_error);
    OCIO_CHECK_CLOSE(image[6],  1.25f, g_error);
    OCIO_CHECK_CLOSE(image[7],  1.00f, g_error);

    OCIO_CHECK_CLOSE(image[8],  1.25f, g_error);
    OCIO_CHECK_CLOSE(image[9],  1.50f, g_error);
    OCIO_CHECK_CLOSE(image[10], 1.75f, g_error);
    OCIO_CHECK_CLOSE(image[11], 0.00f, g_error);
}

OCIO_ADD_TEST(RangeOpCPU, high_clipping)
{
    OCIO::RangeOpDataRcPtr range 
        = std::make_shared<OCIO::RangeOpData>(OCIO::RangeOpData::EmptyValue(), 1.1, 
                                              OCIO::RangeOpData::EmptyValue(), 1.1);

    OCIO_CHECK_NO_THROW(range->validate());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO::ConstOpCPURcPtr op = OCIO::GetRangeRenderer(r);

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "RangeMaxRenderer"));

    const long numPixels = 3;
    float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                  0.75f,  1.00f, 1.25f, 1.0f,
                                  1.25f,  1.50f, 1.75f, 0.0f };

    OCIO_CHECK_NO_THROW(op->apply(&image[0], &image[0], numPixels));

    OCIO_CHECK_CLOSE(image[0],  -0.50f, g_error);
    OCIO_CHECK_CLOSE(image[1],  -0.25f, g_error);
    OCIO_CHECK_CLOSE(image[2],   0.50f, g_error);
    OCIO_CHECK_CLOSE(image[3],   0.00f, g_error);

    OCIO_CHECK_CLOSE(image[4],   0.75f, g_error);
    OCIO_CHECK_CLOSE(image[5],   1.00f, g_error);
    OCIO_CHECK_CLOSE(image[6],   1.10f, g_error);
    OCIO_CHECK_CLOSE(image[7],   1.00f, g_error);

    OCIO_CHECK_CLOSE(image[8],   1.10f, g_error);
    OCIO_CHECK_CLOSE(image[9],   1.10f, g_error);
    OCIO_CHECK_CLOSE(image[10],  1.10f, g_error);
    OCIO_CHECK_CLOSE(image[11],  0.00f, g_error);
}

OCIO_ADD_TEST(RangeOpCPU, inverse)
{
    // Based on scale_with_low_and_high_clippings_2. Setting the direction to inverse and swap the
    // in/out values should give the same numeric result.
    OCIO::RangeOpDataRcPtr range = std::make_shared<OCIO::RangeOpData>(0., 1.5, 0., 1.);
    range->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(range->validate());

    OCIO::ConstRangeOpDataRcPtr r = range;
    OCIO_CHECK_THROW_WHAT(OCIO::GetRangeRenderer(r), OCIO::Exception, "Op::finalize has to be called");

    r = range->getAsForward();
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetRangeRenderer(r));

    const OCIO::OpCPU & c = *op;
    const std::string typeName(typeid(c).name());
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "RangeScaleMinMaxRenderer"));

    const long numPixels = 3;
    float image[4 * numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                    0.75f,  1.00f, 1.25f, 1.0f,
                                    1.25f,  1.50f, 1.75f, 0.0f };

    OCIO_CHECK_NO_THROW(op->apply(&image[0], &image[0], numPixels));

    OCIO_CHECK_CLOSE(image[0], 0.000f, g_error);
    OCIO_CHECK_CLOSE(image[1], 0.000f, g_error);
    OCIO_CHECK_CLOSE(image[2], 0.750f, g_error);
    OCIO_CHECK_CLOSE(image[3], 0.000f, g_error);

    OCIO_CHECK_CLOSE(image[4], 1.125f, g_error);
    OCIO_CHECK_CLOSE(image[5], 1.500f, g_error);
    OCIO_CHECK_CLOSE(image[6], 1.500f, g_error);
    OCIO_CHECK_CLOSE(image[7], 1.000f, g_error);

    OCIO_CHECK_CLOSE(image[8], 1.500f, g_error);
    OCIO_CHECK_CLOSE(image[9], 1.500f, g_error);
    OCIO_CHECK_CLOSE(image[10], 1.500f, g_error);
    OCIO_CHECK_CLOSE(image[11], 0.000f, g_error);
}
