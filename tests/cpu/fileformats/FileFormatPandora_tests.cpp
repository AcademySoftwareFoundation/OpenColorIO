// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatPandora.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FileFormatPandora, format_info)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.getFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(2, formatInfoVec.size());
    OCIO_CHECK_EQUAL("pandora_mga", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("mga", formatInfoVec[0].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
                     formatInfoVec[0].capabilities);
    OCIO_CHECK_EQUAL("pandora_m3d", formatInfoVec[1].name);
    OCIO_CHECK_EQUAL("m3d", formatInfoVec[1].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
                     formatInfoVec[1].capabilities);
}

void ReadPandora(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file.
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME, OCIO::INTERP_DEFAULT);
}

OCIO_ADD_TEST(FileFormatPandora, read_failure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "channel 3d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "values red green blue\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255 255\n";

        OCIO_CHECK_NO_THROW(ReadPandora(SAMPLE_NO_ERROR));
    }
    {
        // Wrong channel tag
        const std::string SAMPLE_ERROR =
            "channel 2d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "values red green blue\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255 255\n";

        OCIO_CHECK_THROW_WHAT(ReadPandora(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Only 3D LUTs are currently supported");
    }
    {
        // No value spec (LUT will not be read)
        const std::string SAMPLE_ERROR =
            "channel 3d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255 255\n";

        OCIO_CHECK_THROW_WHAT(ReadPandora(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Incorrect number of 3D LUT entries");
    }
    {
        // Wrong entry
        const std::string SAMPLE_ERROR =
            "channel 3d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "values red green blue\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 WRONG 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255 255\n";

        OCIO_CHECK_THROW_WHAT(ReadPandora(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Expected to find 4 integers");
    }
    {
        // Wrong number of entries
        const std::string SAMPLE_ERROR =
            "channel 3d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "values red green blue\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255   0\n"
            "8 255 255 255\n";

        OCIO_CHECK_THROW_WHAT(ReadPandora(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Incorrect number of 3D LUT entries");
    }
}

OCIO_ADD_TEST(FileFormatPandora, load_op)
{
    const std::string fileName("pandora_3d.m3d");
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
    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    auto & lutArray = lut->getArray();
    OCIO_REQUIRE_EQUAL(lutArray.getNumValues(), 24);
    const float error = 1e-7f;
    OCIO_CHECK_CLOSE(lutArray[0], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[1], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[2], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[3], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[4], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[5], 0.8f, error);
    OCIO_CHECK_CLOSE(lutArray[6], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[7], 0.8f, error);
    OCIO_CHECK_CLOSE(lutArray[8], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[9], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[10], 0.8f, error);
    OCIO_CHECK_CLOSE(lutArray[11], 0.8f, error);
    OCIO_CHECK_CLOSE(lutArray[12], 1.0f, error);
    OCIO_CHECK_CLOSE(lutArray[13], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[14], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[15], 1.0f, error);
    OCIO_CHECK_CLOSE(lutArray[16], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[17], 1.0f, error);
    OCIO_CHECK_CLOSE(lutArray[18], 1.0f, error);
    OCIO_CHECK_CLOSE(lutArray[19], 1.0f, error);
    OCIO_CHECK_CLOSE(lutArray[20], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[21], 1.2f, error);
    OCIO_CHECK_CLOSE(lutArray[22], 1.0f, error);
    OCIO_CHECK_CLOSE(lutArray[23], 1.2f, error);
}

