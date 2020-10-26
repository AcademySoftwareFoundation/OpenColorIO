// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <fstream>

#include "fileformats/FileFormatVF.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FileFormatVF, format_info)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.getFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(1, formatInfoVec.size());
    OCIO_CHECK_EQUAL("nukevf", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("vf", formatInfoVec[0].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
                     formatInfoVec[0].capabilities);
}

namespace
{
void ReadVF(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file.
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME, OCIO::INTERP_DEFAULT);
}
}

OCIO_ADD_TEST(FileFormatVF, read_failure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "#Inventor V2.1 ascii\n"
            "grid_size 2 2 2\n"
            "global_transform 1 0 0 0  0 1 0 0  0 0 1 0  0 0 0 1 \n"
            "data\n"
            "0 0 0\n"
            "0 0 1\n"
            "0 1 0\n"
            "0 1 1\n"
            "1 0 0\n"
            "1 0 1\n"
            "1 1 0\n"
            "1 1 1\n";

        OCIO_CHECK_NO_THROW(ReadVF(SAMPLE_NO_ERROR));
    }
    {
        // Too much data
        const std::string SAMPLE_ERROR =
            "#Inventor V2.1 ascii\n"
            "grid_size 2 2 2\n"
            "global_transform 1 0 0 0  0 1 0 0  0 0 1 0  0 0 0 1 \n"
            "data\n"
            "0 0 0\n"
            "0 0 1\n"
            "0 1 0\n"
            "0 1 1\n"
            "1 0 0\n"
            "1 0 1\n"
            "1 1 0\n"
            "1 1 0\n"
            "1 1 1\n";

        OCIO_CHECK_THROW_WHAT(ReadVF(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Incorrect number of 3D LUT entries");
    }
}

OCIO_ADD_TEST(FileFormatVF, load_ops)
{
    const std::string vfFileName("nuke_3d.vf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, vfFileName, context,
        OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_CHECK_EQUAL("<FileNoOp>", ops[0]->getInfo());
    OCIO_CHECK_EQUAL("<MatrixOffsetOp>", ops[1]->getInfo());
    OCIO_CHECK_EQUAL("<Lut3DOp>", ops[2]->getInfo());

    auto op1 = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData1 = op1->data();
    auto mat = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opData1);
    OCIO_REQUIRE_ASSERT(mat);
    auto & matArray = mat->getArray();
    OCIO_CHECK_EQUAL(matArray[0], 2.0f);
    OCIO_CHECK_EQUAL(matArray[1], 0.0f);
    OCIO_CHECK_EQUAL(matArray[2], 0.0f);
    OCIO_CHECK_EQUAL(matArray[3], 0.0f);
    OCIO_CHECK_EQUAL(matArray[4], 0.0f);
    OCIO_CHECK_EQUAL(matArray[5], 2.0f);
    OCIO_CHECK_EQUAL(matArray[6], 0.0f);
    OCIO_CHECK_EQUAL(matArray[7], 0.0f);
    OCIO_CHECK_EQUAL(matArray[8], 0.0f);
    OCIO_CHECK_EQUAL(matArray[9], 0.0f);
    OCIO_CHECK_EQUAL(matArray[10], 2.0f);
    OCIO_CHECK_EQUAL(matArray[11], 0.0f);
    OCIO_CHECK_EQUAL(matArray[12], 0.0f);
    OCIO_CHECK_EQUAL(matArray[13], 0.0f);
    OCIO_CHECK_EQUAL(matArray[14], 0.0f);
    OCIO_CHECK_EQUAL(matArray[15], 1.0f);

    auto op2 = std::const_pointer_cast<const OCIO::Op>(ops[2]);
    auto opData2 = op2->data();
    auto lut = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opData2);
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

