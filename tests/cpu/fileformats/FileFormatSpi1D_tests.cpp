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
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
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
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME);
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

