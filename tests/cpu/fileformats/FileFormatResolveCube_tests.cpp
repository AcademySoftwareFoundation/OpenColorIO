// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatResolveCube.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
OCIO::LocalCachedFileRcPtr ReadResolveCube(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME, OCIO::INTERP_DEFAULT);

    return OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
}
}

OCIO_ADD_TEST(FileFormatResolveCube, format_info)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.getFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(1, formatInfoVec.size());
    OCIO_CHECK_EQUAL("resolve_cube", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("cube", formatInfoVec[0].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ | OCIO::FORMAT_CAPABILITY_BAKE,
                     formatInfoVec[0].capabilities);
}

OCIO_ADD_TEST(FileFormatResolveCube, read_1d)
{
    const std::string SAMPLE =
        "LUT_1D_SIZE 2\n"
        "LUT_1D_INPUT_RANGE 0.0 1.0\n"

        "0.0 0.0 0.0\n"
        "1.0 0.0 0.0\n";

    OCIO_CHECK_NO_THROW(ReadResolveCube(SAMPLE));
}

OCIO_ADD_TEST(FileFormatResolveCube, read_3d)
{
    const std::string SAMPLE =
        "LUT_3D_SIZE 2\n"
        "LUT_3D_INPUT_RANGE 0.0 1.0\n"

        "0.0 0.0 0.0\n"
        "1.0 0.0 0.0\n"
        "0.0 1.0 0.0\n"
        "1.0 1.0 0.0\n"
        "0.0 0.0 1.0\n"
        "1.0 0.0 1.0\n"
        "0.0 1.0 1.0\n"
        "1.0 1.0 1.0\n";

    OCIO_CHECK_NO_THROW(ReadResolveCube(SAMPLE));
}

OCIO_ADD_TEST(FileFormatResolveCube, read_1d_3d)
{
    const std::string SAMPLE =
        "LUT_1D_SIZE 6\n"
        "LUT_1D_INPUT_RANGE 0.0 1.0\n"
        "LUT_3D_SIZE 3\n"
        "LUT_3D_INPUT_RANGE 0.0 1.0\n"

        "1.0 1.0 1.0\n"
        "0.8 0.8 0.8\n"
        "0.6 0.6 0.6\n"
        "0.4 0.4 0.4\n"
        "0.2 0.2 0.2\n"
        "0.0 0.0 0.0\n"
        "1.0 1.0 1.0\n"
        "0.5 1.0 1.0\n"
        "0.0 1.0 1.0\n"
        "1.0 0.5 1.0\n"
        "0.5 0.5 1.0\n"
        "0.0 0.5 1.0\n"
        "1.0 0.0 1.0\n"
        "0.5 0.0 1.0\n"
        "0.0 0.0 1.0\n"
        "1.0 1.0 0.5\n"
        "0.5 1.0 0.5\n"
        "0.0 1.0 0.5\n"
        "1.0 0.5 0.5\n"
        "0.5 0.5 0.5\n"
        "0.0 0.5 0.5\n"
        "1.0 0.0 0.5\n"
        "0.5 0.0 0.5\n"
        "0.0 0.0 0.5\n"
        "1.0 1.0 0.0\n"
        "0.5 1.0 0.0\n"
        "0.0 1.0 0.0\n"
        "1.0 0.5 0.0\n"
        "0.5 0.5 0.0\n"
        "0.0 0.5 0.0\n"
        "1.0 0.0 0.0\n"
        "0.5 0.0 0.0\n"
        "0.0 0.0 0.0\n";

    OCIO_CHECK_NO_THROW(ReadResolveCube(SAMPLE));
}

OCIO_ADD_TEST(FileFormatResolveCube, read_default_range)
{
    const std::string SAMPLE_1D =
        "LUT_1D_SIZE 2\n"

        "0.0 0.0 0.0\n"
        "1.0 0.0 0.0\n";

    OCIO_CHECK_NO_THROW(ReadResolveCube(SAMPLE_1D));

    const std::string SAMPLE_3D =
        "LUT_3D_SIZE 2\n"

        "0.0 0.0 0.0\n"
        "1.0 0.0 0.0\n"
        "0.0 1.0 0.0\n"
        "1.0 1.0 0.0\n"
        "0.0 0.0 1.0\n"
        "1.0 0.0 1.0\n"
        "0.0 1.0 1.0\n"
        "1.0 1.0 1.0\n";

    OCIO_CHECK_NO_THROW(ReadResolveCube(SAMPLE_3D));

    const std::string SAMPLE_1D3D =
        "LUT_1D_SIZE 2\n"
        "LUT_3D_SIZE 2\n"

        "0.0 0.0 0.0\n"
        "1.0 1.0 1.0\n"
        "0.0 0.0 0.0\n"
        "1.0 0.0 0.0\n"
        "0.0 1.0 0.0\n"
        "1.0 1.0 0.0\n"
        "0.0 0.0 1.0\n"
        "1.0 0.0 1.0\n"
        "0.0 1.0 1.0\n"
        "1.0 1.0 1.0\n";

    OCIO_CHECK_NO_THROW(ReadResolveCube(SAMPLE_1D3D));
}


OCIO_ADD_TEST(FileFormatResolveCube, read_failure)
{
    {
        // Wrong LUT_3D_SIZE tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2 2\n"
            "LUT_3D_INPUT_RANGE 0.0 1.0\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OCIO_CHECK_THROW(ReadResolveCube(SAMPLE_ERROR), OCIO::Exception);
    }
    {
        // Wrong LUT_3D_INPUT_RANGE tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"
            "LUT_3D_INPUT_RANGE 0.0 1.0 2.0\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OCIO_CHECK_THROW(ReadResolveCube(SAMPLE_ERROR), OCIO::Exception);
    }
    {
        // Comment after header
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"
            "LUT_3D_INPUT_RANGE 0.0 1.0 2.0\n"
            "# Malformed comment\n"
            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OCIO_CHECK_THROW(ReadResolveCube(SAMPLE_ERROR), OCIO::Exception);
    }
    {
        // Unexpected tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"
            "LUT_3D_INPUT_RANGE 0.0 1.0 2.0\n"
            "WRONG_TAG\n"
            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OCIO_CHECK_THROW(ReadResolveCube(SAMPLE_ERROR), OCIO::Exception);
    }
    {
        // Wrong number of entries
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"
            "LUT_3D_INPUT_RANGE 0.0 1.0 2.0\n"

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

        OCIO_CHECK_THROW(ReadResolveCube(SAMPLE_ERROR), OCIO::Exception);
    }
}

OCIO_ADD_TEST(FileFormatResolveCube, bake_1d)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("input");
        cs->setFamily("input");
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
    bout << "LUT_1D_SIZE 2"                                         << "\n";
    bout << "0.000000 0.000000 0.000000"                            << "\n";
    bout << "1.000000 1.000000 1.000000"                            << "\n";

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("resolve_cube");
    baker->setInputSpace("input");
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

OCIO_ADD_TEST(FileFormatResolveCube, bake_1d_shaper)
{
    OCIO::BakerRcPtr bake;
    std::ostringstream os;

    constexpr auto myProfile = R"(
        ocio_profile_version: 1

        colorspaces:
        - !<ColorSpace>
          name : Raw
          isdata : false

        - !<ColorSpace>
          name: Log2
          isdata: false
          from_reference: !<GroupTransform>
            children:
              - !<MatrixTransform> {matrix: [5.55556, 0, 0, 0, 0, 5.55556, 0, 0, 0, 0, 5.55556, 0, 0, 0, 0, 1]}
              - !<LogTransform> {base: 2}
              - !<MatrixTransform> {offset: [6.5, 6.5, 6.5, 0]}
              - !<MatrixTransform> {matrix: [0.076923, 0, 0, 0, 0, 0.076923, 0, 0, 0, 0, 0.076923, 0, 0, 0, 0, 1]}
    )";

    std::istringstream is(myProfile);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_REQUIRE_ASSERT(config);

    {
        // Lin to Log
        std::ostringstream bout;
        bout << "LUT_1D_SIZE 10"                                        << "\n";
        bout << "LUT_1D_INPUT_RANGE 0.001989 16.291878"                 << "\n";
        bout << "0.000000 0.000000 0.000000"                            << "\n";
        bout << "0.756268 0.756268 0.756268"                            << "\n";
        bout << "0.833130 0.833130 0.833130"                            << "\n";
        bout << "0.878107 0.878107 0.878107"                            << "\n";
        bout << "0.910023 0.910023 0.910023"                            << "\n";
        bout << "0.934780 0.934780 0.934780"                            << "\n";
        bout << "0.955010 0.955010 0.955010"                            << "\n";
        bout << "0.972114 0.972114 0.972114"                            << "\n";
        bout << "0.986931 0.986931 0.986931"                            << "\n";
        bout << "1.000000 1.000000 1.000000"                            << "\n";

        OCIO::BakerRcPtr baker = OCIO::Baker::Create();
        baker->setConfig(config);
        baker->setFormat("resolve_cube");
        baker->setInputSpace("Raw");
        baker->setTargetSpace("Log2");
        baker->setShaperSpace("Log2");
        baker->setCubeSize(10);
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

    {
        // Log to Lin
        std::ostringstream bout;
        bout << "LUT_1D_SIZE 10"                                        << "\n";
        bout << "0.001989 0.001989 0.001989"                            << "\n";
        bout << "0.005413 0.005413 0.005413"                            << "\n";
        bout << "0.014731 0.014731 0.014731"                            << "\n";
        bout << "0.040091 0.040091 0.040091"                            << "\n";
        bout << "0.109110 0.109110 0.109110"                            << "\n";
        bout << "0.296951 0.296951 0.296951"                            << "\n";
        bout << "0.808177 0.808177 0.808177"                            << "\n";
        bout << "2.199522 2.199522 2.199522"                            << "\n";
        bout << "5.986179 5.986179 5.986179"                            << "\n";
        bout << "16.291878 16.291878 16.291878"                         << "\n";

        OCIO::BakerRcPtr baker = OCIO::Baker::Create();
        baker->setConfig(config);
        baker->setFormat("resolve_cube");
        baker->setInputSpace("Log2");
        baker->setTargetSpace("Raw");
        baker->setCubeSize(10);
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
}

OCIO_ADD_TEST(FileFormatResolveCube, bake_3d)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("input");
        cs->setFamily("input");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");

        // Set saturation to cause channel crosstalk, making a 3D LUT
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        transform1->setSat(0.5f);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);

        config->addColorSpace(cs);
    }

    std::ostringstream bout;
    bout << "# OpenColorIO Test Line 1"                             << "\n";
    bout << "# OpenColorIO Test Line 2"                             << "\n";
    bout << ""                                                      << "\n";
    bout << "LUT_3D_SIZE 2"                                         << "\n";
    bout << "0.000000 0.000000 0.000000"                            << "\n";
    bout << "0.606300 0.106300 0.106300"                            << "\n";
    bout << "0.357600 0.857600 0.357600"                            << "\n";
    bout << "0.963900 0.963900 0.463900"                            << "\n";
    bout << "0.036100 0.036100 0.536100"                            << "\n";
    bout << "0.642400 0.142400 0.642400"                            << "\n";
    bout << "0.393700 0.893700 0.893700"                            << "\n";
    bout << "1.000000 1.000000 1.000000"                            << "\n";

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION,
                                               "OpenColorIO Test Line 1");
    baker->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION,
                                               "OpenColorIO Test Line 2");
    baker->setFormat("resolve_cube");
    baker->setInputSpace("input");
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

OCIO_ADD_TEST(FileFormatResolveCube, bake_1d_3d)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("input");
        cs->setFamily("input");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("shaper");
        cs->setFamily("shaper");
        OCIO::ExponentTransformRcPtr transform1 = OCIO::ExponentTransform::Create();
        double test[4] = {2.2, 2.2, 2.2, 1.0};
        transform1->setValue(test);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_TO_REFERENCE);
        config->addColorSpace(cs);
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");

        // Set saturation to cause channel crosstalk, making a 3D LUT
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        transform1->setSat(0.5f);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);

        config->addColorSpace(cs);
    }

    std::ostringstream bout;
    bout << "LUT_1D_SIZE 10"                                        << "\n";
    bout << "LUT_1D_INPUT_RANGE 0.000000 1.000000"                  << "\n";
    bout << "LUT_3D_SIZE 2"                                         << "\n";
    bout << "0.000000 0.000000 0.000000"                            << "\n";
    bout << "0.368344 0.368344 0.368344"                            << "\n";
    bout << "0.504760 0.504760 0.504760"                            << "\n";
    bout << "0.606913 0.606913 0.606913"                            << "\n";
    bout << "0.691699 0.691699 0.691699"                            << "\n";
    bout << "0.765539 0.765539 0.765539"                            << "\n";
    bout << "0.831684 0.831684 0.831684"                            << "\n";
    bout << "0.892049 0.892049 0.892049"                            << "\n";
    bout << "0.947870 0.947870 0.947870"                            << "\n";
    bout << "1.000000 1.000000 1.000000"                            << "\n";
    bout << "0.000000 0.000000 0.000000"                            << "\n";
    bout << "0.606300 0.106300 0.106300"                            << "\n";
    bout << "0.357600 0.857600 0.357600"                            << "\n";
    bout << "0.963900 0.963900 0.463900"                            << "\n";
    bout << "0.036100 0.036100 0.536100"                            << "\n";
    bout << "0.642400 0.142400 0.642400"                            << "\n";
    bout << "0.393700 0.893700 0.893700"                            << "\n";
    bout << "1.000000 1.000000 1.000000"                            << "\n";

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("resolve_cube");
    baker->setInputSpace("input");
    baker->setShaperSpace("shaper");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
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

OCIO_ADD_TEST(FileFormatResolveCube, load_ops)
{
    const std::string fileName("resolve_1d3d.cube");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context,
        OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 5);
    OCIO_CHECK_EQUAL("<FileNoOp>", ops[0]->getInfo());
    OCIO_CHECK_EQUAL("<MatrixOffsetOp>", ops[1]->getInfo());
    OCIO_CHECK_EQUAL("<Lut1DOp>", ops[2]->getInfo());
    OCIO_CHECK_EQUAL("<MatrixOffsetOp>", ops[3]->getInfo());
    OCIO_CHECK_EQUAL("<Lut3DOp>", ops[4]->getInfo());

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
    OCIO_CHECK_EQUAL(matArray[5], 0.25f);
    OCIO_CHECK_EQUAL(matArray[6], 0.0f);
    OCIO_CHECK_EQUAL(matArray[7], 0.0f);
    OCIO_CHECK_EQUAL(matArray[8], 0.0f);
    OCIO_CHECK_EQUAL(matArray[9], 0.0f);
    OCIO_CHECK_EQUAL(matArray[10], 0.25f);
    OCIO_CHECK_EQUAL(matArray[11], 0.0f);
    OCIO_CHECK_EQUAL(matArray[12], 0.0f);
    OCIO_CHECK_EQUAL(matArray[13], 0.0f);
    OCIO_CHECK_EQUAL(matArray[14], 0.0f);
    OCIO_CHECK_EQUAL(matArray[15], 1.0f);

    auto & matOffsets = mat->getOffsets();
    OCIO_CHECK_EQUAL(matOffsets[0], 0.25f);
    OCIO_CHECK_EQUAL(matOffsets[1], 0.25f);
    OCIO_CHECK_EQUAL(matOffsets[2], 0.25f);
    OCIO_CHECK_EQUAL(matOffsets[3], 0.0f);

    auto op2 = std::const_pointer_cast<const OCIO::Op>(ops[2]);
    auto opData2 = op2->data();
    auto lut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData2);
    OCIO_REQUIRE_ASSERT(lut);
    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    auto & lutArray = lut->getArray();
    OCIO_REQUIRE_EQUAL(lutArray.getNumValues(), 18);

    OCIO_CHECK_EQUAL(lutArray[0], 3.3f);
    OCIO_CHECK_EQUAL(lutArray[1], 3.4f);
    OCIO_CHECK_EQUAL(lutArray[2], 3.5f);
    OCIO_CHECK_EQUAL(lutArray[3], 3.0f);
    OCIO_CHECK_EQUAL(lutArray[4], 3.1f);
    OCIO_CHECK_EQUAL(lutArray[5], 3.2f);
    OCIO_CHECK_EQUAL(lutArray[6], 2.2f);
    OCIO_CHECK_EQUAL(lutArray[7], 2.3f);
    OCIO_CHECK_EQUAL(lutArray[8], 2.4f);
    OCIO_CHECK_EQUAL(lutArray[9], 2.1f);
    OCIO_CHECK_EQUAL(lutArray[10], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[11], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[12], 1.0f);
    OCIO_CHECK_EQUAL(lutArray[13], 1.0f);
    OCIO_CHECK_EQUAL(lutArray[14], 1.0f);
    OCIO_CHECK_EQUAL(lutArray[15], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[16], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[17], 0.0f);

    auto op3 = std::const_pointer_cast<const OCIO::Op>(ops[3]);
    auto opData3 = op3->data();
    auto mat3 = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opData3);
    OCIO_REQUIRE_ASSERT(mat3);
    auto & mat3Array = mat3->getArray();
    OCIO_CHECK_EQUAL(mat3Array[0], 0.25f);
    OCIO_CHECK_EQUAL(mat3Array[1], 0.0f);
    OCIO_CHECK_EQUAL(mat3Array[2], 0.0f);
    OCIO_CHECK_EQUAL(mat3Array[3], 0.0f);
    OCIO_CHECK_EQUAL(mat3Array[4], 0.0f);
    OCIO_CHECK_EQUAL(mat3Array[5], 0.25f);
    OCIO_CHECK_EQUAL(mat3Array[6], 0.0f);
    OCIO_CHECK_EQUAL(mat3Array[7], 0.0f);
    OCIO_CHECK_EQUAL(mat3Array[8], 0.0f);
    OCIO_CHECK_EQUAL(mat3Array[9], 0.0f);
    OCIO_CHECK_EQUAL(mat3Array[10], 0.25f);
    OCIO_CHECK_EQUAL(mat3Array[11], 0.0f);
    OCIO_CHECK_EQUAL(mat3Array[12], 0.0f);
    OCIO_CHECK_EQUAL(mat3Array[13], 0.0f);
    OCIO_CHECK_EQUAL(mat3Array[14], 0.0f);
    OCIO_CHECK_EQUAL(mat3Array[15], 1.0f);

    auto & mat3Offsets = mat->getOffsets();
    OCIO_CHECK_EQUAL(mat3Offsets[0], 0.25f);
    OCIO_CHECK_EQUAL(mat3Offsets[1], 0.25f);
    OCIO_CHECK_EQUAL(mat3Offsets[2], 0.25f);
    OCIO_CHECK_EQUAL(mat3Offsets[3], 0.0f);

    auto op4 = std::const_pointer_cast<const OCIO::Op>(ops[4]);
    auto opData4 = op4->data();
    auto lut4 = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opData4);
    OCIO_REQUIRE_ASSERT(lut4);
    OCIO_CHECK_EQUAL(lut4->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    auto & lut4Array = lut4->getArray();
    OCIO_REQUIRE_EQUAL(lut4Array.getNumValues(), 81);

    // File line 11 - R:0 - G:0 - B:0
    OCIO_CHECK_EQUAL(lut4Array[0], 1.1f);
    OCIO_CHECK_EQUAL(lut4Array[1], 1.1f);
    OCIO_CHECK_EQUAL(lut4Array[2], 1.1f);

    // File line 23 - R:0 - G:1 - B:1
    OCIO_CHECK_EQUAL(lut4Array[12], 1.0f);
    OCIO_CHECK_EQUAL(lut4Array[13], 0.5f);
    OCIO_CHECK_EQUAL(lut4Array[14], 0.5f);

    // File line 31 - R:2 - G:0 - B:2
    OCIO_CHECK_EQUAL(lut4Array[60], 0.0f);
    OCIO_CHECK_EQUAL(lut4Array[61], 1.0f);
    OCIO_CHECK_EQUAL(lut4Array[62], 0.0f);

}
