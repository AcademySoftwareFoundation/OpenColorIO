// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatSpiMtx.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FileFormatSpiMtx, FormatInfo)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.getFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(1, formatInfoVec.size());
    OCIO_CHECK_EQUAL("spimtx", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("spimtx", formatInfoVec[0].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
                     formatInfoVec[0].capabilities);
}

namespace
{
OCIO::LocalCachedFileRcPtr LoadLutFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::in);
}
}

OCIO_ADD_TEST(FileFormatSpiMtx, Test)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string spiMtxFile("camera_to_aces.spimtx");
    OCIO_CHECK_NO_THROW(cachedFile = LoadLutFile(spiMtxFile));

    OCIO_REQUIRE_ASSERT((bool)cachedFile);
    OCIO_CHECK_EQUAL(0.0, cachedFile->offset4[0]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->offset4[1]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->offset4[2]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->offset4[3]);

    OCIO_CHECK_EQUAL(0.754338638f, (float)cachedFile->m44[0]);
    OCIO_CHECK_EQUAL(0.133697046f, (float)cachedFile->m44[1]);
    OCIO_CHECK_EQUAL(0.111968437f, (float)cachedFile->m44[2]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->m44[3]);

    OCIO_CHECK_EQUAL(0.021198141f, (float)cachedFile->m44[4]);
    OCIO_CHECK_EQUAL(1.005410934f, (float)cachedFile->m44[5]);
    OCIO_CHECK_EQUAL(-0.026610548f, (float)cachedFile->m44[6]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->m44[7]);

    OCIO_CHECK_EQUAL(-0.009756991f, (float)cachedFile->m44[8]);
    OCIO_CHECK_EQUAL(0.004508563f, (float)cachedFile->m44[9]);
    OCIO_CHECK_EQUAL(1.005253201f, (float)cachedFile->m44[10]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->m44[11]);

    OCIO_CHECK_EQUAL(0.0, cachedFile->m44[12]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->m44[13]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->m44[14]);
    OCIO_CHECK_EQUAL(1.0, cachedFile->m44[15]);
}

OCIO::LocalCachedFileRcPtr ReadSpiMtx(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME, OCIO::INTERP_DEFAULT);

    return OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
}

OCIO_ADD_TEST(FileFormatSpiMtx, ReadOffset)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_FILE =
            "1 0 0 6553.5\n"
            "0 1 0 32767.5\n"
            "0 0 1 65535.0\n";

        OCIO::LocalCachedFileRcPtr cachedFile;
        OCIO_CHECK_NO_THROW(cachedFile = ReadSpiMtx(SAMPLE_FILE));
        OCIO_REQUIRE_ASSERT((bool)cachedFile);
        OCIO_CHECK_EQUAL(0.1, cachedFile->offset4[0]);
        OCIO_CHECK_EQUAL(0.5, cachedFile->offset4[1]);
        OCIO_CHECK_EQUAL(1.0, cachedFile->offset4[2]);
        OCIO_CHECK_EQUAL(0.0, cachedFile->offset4[3]);
    }
}

OCIO_ADD_TEST(FileFormatSpiMtx, ReadFailure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "1.0 0.0 0.0 0.0\n"
            "0.0 1.0 0.0 0.0\n"
            "0.0 0.0 1.0 0.0\n";

        OCIO_CHECK_NO_THROW(ReadSpiMtx(SAMPLE_NO_ERROR));
    }
    {
        // Wrong number of elements
        const std::string SAMPLE_ERROR =
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "0.0 0.0 1.0\n";

        OCIO_CHECK_THROW_WHAT(ReadSpiMtx(SAMPLE_ERROR),
                              OCIO::Exception,
                              "File must contain 12 float entries");
    }
    {
        // Some elements can' t be read as float
        const std::string SAMPLE_ERROR =
            "1.0 0.0 0.0 0.0\n"
            "0.0 error 0.0 0.0\n"
            "0.0 0.0 1.0 0.0\n";

        OCIO_CHECK_THROW_WHAT(ReadSpiMtx(SAMPLE_ERROR),
                              OCIO::Exception,
                              "File must contain all float entries");
    }
}

