// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatSpi1D.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FileFormatSpi1D, format_info)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.getFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(1, formatInfoVec.size());
    OCIO_CHECK_EQUAL("spi1d", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("spi1d", formatInfoVec[0].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ | OCIO::FORMAT_CAPABILITY_BAKE,
                     formatInfoVec[0].capabilities);
}

OCIO::LocalCachedFileRcPtr LoadLutFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::in);
}

OCIO_ADD_TEST(FileFormatSpi1D, test)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string spi1dFile("cpf.spi1d");
    OCIO_CHECK_NO_THROW(cachedFile = LoadLutFile(spi1dFile));

    OCIO_REQUIRE_ASSERT(cachedFile);
    OCIO_REQUIRE_ASSERT(cachedFile->lut);
    OCIO_CHECK_EQUAL(cachedFile->lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OCIO_CHECK_EQUAL(0.0f, cachedFile->from_min);
    OCIO_CHECK_EQUAL(1.0f, cachedFile->from_max);

    const OCIO::Array & lutArray = cachedFile->lut->getArray();
    OCIO_CHECK_EQUAL(2048ul, lutArray.getLength());

    OCIO_CHECK_EQUAL(0.0f, lutArray[0]);
    OCIO_CHECK_EQUAL(0.0f, lutArray[1]);
    OCIO_CHECK_EQUAL(0.0f, lutArray[2]);

    OCIO_CHECK_EQUAL(4.511920005404118f, lutArray[1970*3]);
    OCIO_CHECK_EQUAL(4.511920005404118f, lutArray[1970*3 + 1]);
    OCIO_CHECK_EQUAL(4.511920005404118f, lutArray[1970*3 + 2]);
}

OCIO::LocalCachedFileRcPtr ReadSpi1d(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file.
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME, OCIO::INTERP_DEFAULT);
    return OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
}

OCIO_ADD_TEST(FileFormatSpi1D, read_failure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_NO_THROW(ReadSpi1d(SAMPLE_NO_ERROR));
    }
    {
        // Version missing.
        const std::string SAMPLE_ERROR =
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Could not find 'Version' Tag");
    }
    {
        // Version is not 1.
        const std::string SAMPLE_ERROR =
            "Version 2\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Only format version 1 supported");
    }
    {
        // Version can't be scanned.
        const std::string SAMPLE_ERROR =
            "Version A\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Invalid 'Version' Tag");
    }
    {
        // Version case is wrong.
        const std::string SAMPLE_ERROR =
            "VERSION 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Could not find 'Version' Tag");
    }
    {
        // From does not specify 2 floats.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Invalid 'From' Tag");
    }
    {
        // Length is missing.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Could not find 'Length' Tag");
    }
    {
        // Length can't be read.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length A\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Invalid 'Length' Tag");
    }
    {
        // Component is missing.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Could not find 'Components' Tag");
    }
    {
        // Component can't be read.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components A\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Invalid 'Components' Tag");
    }
    {
        // Component not 1 or 2 or 3.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 4\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Components must be [1,2,3]");
    }
    {
        // LUT too short.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Not enough entries found");
    }
    {
        // LUT too long.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "0.0\n"
            "0.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Too many entries found");
    }
    {
        // Components==1 but two components specified in LUT.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0 1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Malformed LUT line");
    }
}

OCIO_ADD_TEST(FileFormatSpi1D, identity)
{
    {
        const std::string SAMPLE_LUT =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.000007\n"
            "}\n";

        OCIO::LocalCachedFileRcPtr parsedLUT;
        OCIO_CHECK_NO_THROW(parsedLUT = ReadSpi1d(SAMPLE_LUT));
        OCIO_REQUIRE_ASSERT(parsedLUT);
        OCIO_REQUIRE_ASSERT(parsedLUT->lut);
        OCIO_CHECK_ASSERT(parsedLUT->lut->isIdentity());
    }
    {
        const std::string SAMPLE_LUT =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.00001\n"
            "}\n";

        OCIO::LocalCachedFileRcPtr parsedLUT;
        OCIO_CHECK_NO_THROW(parsedLUT = ReadSpi1d(SAMPLE_LUT));
        OCIO_REQUIRE_ASSERT(parsedLUT);
        OCIO_REQUIRE_ASSERT(parsedLUT->lut);
        OCIO_CHECK_ASSERT(!parsedLUT->lut->isIdentity());
    }
}

OCIO_ADD_TEST(FileFormatSpi1D, bake_1d)
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
    bout << "Version 1"                                             << "\n";
    bout << "From 0.000000 1.000000"                                << "\n";
    bout << "Length 2"                                              << "\n";
    bout << "Components 3"                                          << "\n";
    bout << "{"                                                     << "\n";
    bout << "    0.000000 0.000000 0.000000"                        << "\n";
    bout << "    1.000000 1.000000 1.000000"                        << "\n";
    bout << "}"                                                     << "\n";

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("spi1d");
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

OCIO_ADD_TEST(FileFormatSpi1D, bake_1d_shaper)
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
        bout << "Version 1"                                             << "\n";
        bout << "From 0.001989 16.291878"                               << "\n";
        bout << "Length 10"                                             << "\n";
        bout << "Components 3"                                          << "\n";
        bout << "{"                                                     << "\n";
        bout << "    0.000000 0.000000 0.000000"                        << "\n";
        bout << "    0.756268 0.756268 0.756268"                        << "\n";
        bout << "    0.833130 0.833130 0.833130"                        << "\n";
        bout << "    0.878107 0.878107 0.878107"                        << "\n";
        bout << "    0.910023 0.910023 0.910023"                        << "\n";
        bout << "    0.934780 0.934780 0.934780"                        << "\n";
        bout << "    0.955010 0.955010 0.955010"                        << "\n";
        bout << "    0.972114 0.972114 0.972114"                        << "\n";
        bout << "    0.986931 0.986931 0.986931"                        << "\n";
        bout << "    1.000000 1.000000 1.000000"                        << "\n";
        bout << "}"                                                     << "\n";

        OCIO::BakerRcPtr baker = OCIO::Baker::Create();
        baker->setConfig(config);
        baker->setFormat("spi1d");
        baker->setInputSpace("Raw");
        baker->setTargetSpace("Log2");
        // The ShaperSpace is used here to derive the range of the LUT.
        // This is needed because the range [0, 1] will not cover the full
        // extent of the log space.
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
        bout << "Version 1"                                             << "\n";
        bout << "From 0.000000 1.000000"                                << "\n";
        bout << "Length 10"                                             << "\n";
        bout << "Components 3"                                          << "\n";
        bout << "{"                                                     << "\n";
        bout << "    0.001989 0.001989 0.001989"                        << "\n";
        bout << "    0.005413 0.005413 0.005413"                        << "\n";
        bout << "    0.014731 0.014731 0.014731"                        << "\n";
        bout << "    0.040091 0.040091 0.040091"                        << "\n";
        bout << "    0.109110 0.109110 0.109110"                        << "\n";
        bout << "    0.296951 0.296951 0.296951"                        << "\n";
        bout << "    0.808177 0.808177 0.808177"                        << "\n";
        bout << "    2.199522 2.199522 2.199522"                        << "\n";
        bout << "    5.986179 5.986179 5.986179"                        << "\n";
        bout << "    16.291878 16.291878 16.291878"                     << "\n";
        bout << "}"                                                     << "\n";

        OCIO::BakerRcPtr baker = OCIO::Baker::Create();
        baker->setConfig(config);
        baker->setFormat("spi1d");
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
