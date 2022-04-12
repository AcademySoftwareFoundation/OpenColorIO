// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatIridasCube.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FileFormatIridasCube, format_info)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.getFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(1, formatInfoVec.size());
    OCIO_CHECK_EQUAL("iridas_cube", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("cube", formatInfoVec[0].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ | OCIO::FORMAT_CAPABILITY_BAKE,
                     formatInfoVec[0].capabilities);
}

OCIO::LocalCachedFileRcPtr ReadIridasCube(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME, OCIO::INTERP_DEFAULT);

    return OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
}

OCIO_ADD_TEST(FileFormatIridasCube, read_failure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "LUT_3D_SIZE 2\n"
            "DOMAIN_MIN 0.0 0.0 0.0\n"
            "DOMAIN_MAX 1.0 1.0 1.0\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OCIO_CHECK_NO_THROW(ReadIridasCube(SAMPLE_NO_ERROR));
    }
    {
        // Wrong LUT_3D_SIZE tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2 2\n"
            "DOMAIN_MIN 0.0 0.0 0.0\n"
            "DOMAIN_MAX 1.0 1.0 1.0\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OCIO_CHECK_THROW_WHAT(ReadIridasCube(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Malformed 'LUT_3D_SIZE' tag");
    }
    {
        // Wrong DOMAIN_MIN tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"
            "DOMAIN_MIN 0.0 0.0\n"
            "DOMAIN_MAX 1.0 1.0 1.0\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OCIO_CHECK_THROW_WHAT(ReadIridasCube(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Malformed 'DOMAIN_MIN' tag");
    }
    {
        // Wrong DOMAIN_MAX tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"
            "DOMAIN_MIN 0.0 0.0 0.0\n"
            "DOMAIN_MAX 1.0 1.0 1.0 1.0\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OCIO_CHECK_THROW_WHAT(ReadIridasCube(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Malformed 'DOMAIN_MAX' tag");
    }
    {
        // Unexpected tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"
            "DOMAIN_MIN 0.0 0.0 0.0\n"
            "DOMAIN_MAX 1.0 1.0 1.0\n"
            "WRONG_TAG\n"
            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OCIO_CHECK_THROW_WHAT(ReadIridasCube(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Malformed color triples specified");
    }
    {
        // Wrong number of entries
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"
            "DOMAIN_MIN 0.0 0.0 0.0\n"
            "DOMAIN_MAX 1.0 1.0 1.0\n"

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

        OCIO_CHECK_THROW_WHAT(ReadIridasCube(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Incorrect number of 3D LUT entries");
    }
}

OCIO_ADD_TEST(FileFormatIridasCube, no_shaper)
{
    // check baker output
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        config->addColorSpace(cs);
    }

    std::ostringstream bout;
    bout << "# Alexa conversion LUT, logc2video. Full in/full out." << "\n";
    bout << "# created by alexalutconv (2.11)"                      << "\n";
    bout << ""                                                      << "\n";
    bout << "LUT_3D_SIZE 2"                                         << "\n";
    bout << "0.000000 0.000000 0.000000"                            << "\n";
    bout << "1.000000 0.000000 0.000000"                            << "\n";
    bout << "0.000000 1.000000 0.000000"                            << "\n";
    bout << "1.000000 1.000000 0.000000"                            << "\n";
    bout << "0.000000 0.000000 1.000000"                            << "\n";
    bout << "1.000000 0.000000 1.000000"                            << "\n";
    bout << "0.000000 1.000000 1.000000"                            << "\n";
    bout << "1.000000 1.000000 1.000000"                            << "\n";

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);

    baker->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION,
                                               "Alexa conversion LUT, "
                                               "logc2video. Full in/full out.");
    baker->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION,
                                               "created by alexalutconv (2.11)");
    baker->setFormat("iridas_cube");
    baker->setInputSpace("lnf");
    baker->setTargetSpace("target");
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);

    //
    const StringUtils::StringVec osvec  = StringUtils::SplitByLines(output.str());
    const StringUtils::StringVec resvec = StringUtils::SplitByLines(bout.str());
    OCIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
    {
        OCIO_CHECK_EQUAL(osvec[i], resvec[i]);
    }
}

OCIO_ADD_TEST(FileFormatIridasCube, load_1d_op)
{
    const std::string fileName("iridas_1d.cube");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context,
        OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_CHECK_EQUAL("<FileNoOp>", ops[0]->getInfo());
    OCIO_CHECK_EQUAL("<MatrixOffsetOp>", ops[1]->getInfo());
    OCIO_CHECK_EQUAL("<Lut1DOp>", ops[2]->getInfo());

    auto op1 = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData1 = op1->data();
    auto mat = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opData1);
    OCIO_REQUIRE_ASSERT(mat);
    auto & matArray = mat->getArray();
    OCIO_CHECK_EQUAL(matArray[0], 0.25f);
    OCIO_CHECK_EQUAL(matArray[1], 0.0f);
    OCIO_CHECK_EQUAL(matArray[2], 0.0f);
    OCIO_CHECK_EQUAL(matArray[3], 0.0f);
    OCIO_CHECK_EQUAL(matArray[4], 0.0f);
    OCIO_CHECK_EQUAL(matArray[5], 1.0f);
    OCIO_CHECK_EQUAL(matArray[6], 0.0f);
    OCIO_CHECK_EQUAL(matArray[7], 0.0f);
    OCIO_CHECK_EQUAL(matArray[8], 0.0f);
    OCIO_CHECK_EQUAL(matArray[9], 0.0f);
    OCIO_CHECK_EQUAL(matArray[10], 1.0f);
    OCIO_CHECK_EQUAL(matArray[11], 0.0f);
    OCIO_CHECK_EQUAL(matArray[12], 0.0f);
    OCIO_CHECK_EQUAL(matArray[13], 0.0f);
    OCIO_CHECK_EQUAL(matArray[14], 0.0f);
    OCIO_CHECK_EQUAL(matArray[15], 1.0f);

    auto & matOffsets = mat->getOffsets();
    OCIO_CHECK_EQUAL(matOffsets[0], 0.5f);
    OCIO_CHECK_EQUAL(matOffsets[1], -1.0f);
    OCIO_CHECK_EQUAL(matOffsets[2], 0.0f);
    OCIO_CHECK_EQUAL(matOffsets[3], 0.0f);

    auto op2 = std::const_pointer_cast<const OCIO::Op>(ops[2]);
    auto opData2 = op2->data();
    auto lut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData2);
    OCIO_REQUIRE_ASSERT(lut);
    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    auto & lutArray = lut->getArray();
    OCIO_REQUIRE_EQUAL(lutArray.getNumValues(), 15);

    OCIO_CHECK_EQUAL(lutArray[0], -1.0f);
    OCIO_CHECK_EQUAL(lutArray[1], -2.0f);
    OCIO_CHECK_EQUAL(lutArray[2], -3.0f);
    OCIO_CHECK_EQUAL(lutArray[3], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[4], 0.1f);
    OCIO_CHECK_EQUAL(lutArray[5], 0.2f);
    OCIO_CHECK_EQUAL(lutArray[6], 0.4f);
    OCIO_CHECK_EQUAL(lutArray[7], 0.5f);
    OCIO_CHECK_EQUAL(lutArray[8], 0.6f);
    OCIO_CHECK_EQUAL(lutArray[9], 0.8f);
    OCIO_CHECK_EQUAL(lutArray[10], 0.9f);
    OCIO_CHECK_EQUAL(lutArray[11], 1.0f);
    OCIO_CHECK_EQUAL(lutArray[12], 1.0f);
    OCIO_CHECK_EQUAL(lutArray[13], 2.1f);
    OCIO_CHECK_EQUAL(lutArray[14], 3.2f);
}

OCIO_ADD_TEST(FileFormatIridasCube, load_3d_op)
{
    const std::string fileName("iridas_3d.cube");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context,
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
    OCIO_CHECK_EQUAL(matArray[0], 0.5f);
    OCIO_CHECK_EQUAL(matArray[1], 0.0f);
    OCIO_CHECK_EQUAL(matArray[2], 0.0f);
    OCIO_CHECK_EQUAL(matArray[3], 0.0f);
    OCIO_CHECK_EQUAL(matArray[4], 0.0f);
    OCIO_CHECK_EQUAL(matArray[5], 1.0f);
    OCIO_CHECK_EQUAL(matArray[6], 0.0f);
    OCIO_CHECK_EQUAL(matArray[7], 0.0f);
    OCIO_CHECK_EQUAL(matArray[8], 0.0f);
    OCIO_CHECK_EQUAL(matArray[9], 0.0f);
    OCIO_CHECK_EQUAL(matArray[10], 1.0f);
    OCIO_CHECK_EQUAL(matArray[11], 0.0f);
    OCIO_CHECK_EQUAL(matArray[12], 0.0f);
    OCIO_CHECK_EQUAL(matArray[13], 0.0f);
    OCIO_CHECK_EQUAL(matArray[14], 0.0f);
    OCIO_CHECK_EQUAL(matArray[15], 1.0f);

    auto & matOffsets = mat->getOffsets();
    OCIO_CHECK_EQUAL(matOffsets[0], 0.0f);
    OCIO_CHECK_EQUAL(matOffsets[1], -1.0f);
    OCIO_CHECK_EQUAL(matOffsets[2], 0.0f);
    OCIO_CHECK_EQUAL(matOffsets[3], 0.0f);

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

