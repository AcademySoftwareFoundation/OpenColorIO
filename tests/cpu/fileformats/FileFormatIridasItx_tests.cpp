// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatIridasItx.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
void ReadIridasItx(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME, OCIO::INTERP_DEFAULT);
}

}

OCIO_ADD_TEST(FileFormatIridasItx, read_failure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "LUT_3D_SIZE 2\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OCIO_CHECK_NO_THROW(ReadIridasItx(SAMPLE_NO_ERROR));
    }
    {
        // Wrong LUT_3D_SIZE tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2 2\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OCIO_CHECK_THROW_WHAT(ReadIridasItx(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Malformed LUT_3D_SIZE tag");
    }
    {
        // Unexpected tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"

            "WRONG_TAG\n"
            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OCIO_CHECK_THROW_WHAT(ReadIridasItx(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Malformed color triples specified");
    }
    {
        // Wrong number of entries
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "0.0 1.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OCIO_CHECK_THROW_WHAT(ReadIridasItx(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Incorrect number of 3D LUT entries");
    }
}

OCIO_ADD_TEST(FileFormatIridasItx, load_3d_op)
{
    const std::string fileName("iridas_3d.itx");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context,
        OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_EQUAL("<FileNoOp>", ops[0]->getInfo());
    OCIO_CHECK_EQUAL("<Lut3DOp>", ops[1]->getInfo());

    auto op1 = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData1 = op1->data();
    auto lut = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opData1);
    OCIO_REQUIRE_ASSERT(lut);
    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    auto & lutArray = lut->getArray();
    OCIO_REQUIRE_EQUAL(lutArray.getNumValues(), 24);
    OCIO_CHECK_EQUAL(lutArray[0], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[1], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[2], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[3], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[4], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[5], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[6], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[7], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[8], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[9], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[10], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[11], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[12], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[13], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[14], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[15], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[16], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[17], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[18], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[19], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[20], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[21], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[22], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[23], 2.0f);
}

