// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatSpi3D.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FileFormatSpi3D, format_info)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.getFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(1, formatInfoVec.size());
    OCIO_CHECK_EQUAL("spi3d", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("spi3d", formatInfoVec[0].extension);
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

OCIO_ADD_TEST(FileFormatSpi3D, test)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string spi3dFile("spi_ocio_srgb_test.spi3d");
    OCIO_CHECK_NO_THROW(cachedFile = LoadLutFile(spi3dFile));

    OCIO_CHECK_ASSERT((bool)cachedFile);
    OCIO_CHECK_ASSERT((bool)(cachedFile->lut));

    const OCIO::Array & lutArray = cachedFile->lut->getArray();
    OCIO_CHECK_EQUAL(32, lutArray.getLength());
    OCIO_CHECK_EQUAL(32*32*32*3, lutArray.getNumValues());

    OCIO_CHECK_EQUAL(0.040157f, lutArray[0]);
    OCIO_CHECK_EQUAL(0.038904f, lutArray[1]);
    OCIO_CHECK_EQUAL(0.028316f, lutArray[2]);
    // 10 2 12
    OCIO_CHECK_EQUAL(0.102161f, lutArray[30948]);
    OCIO_CHECK_EQUAL(0.032187f, lutArray[30949]);
    OCIO_CHECK_EQUAL(0.175453f, lutArray[30950]);
}

void ReadSpi3d(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME, OCIO::INTERP_DEFAULT);
}

OCIO_ADD_TEST(FileFormatSpi3D, read_failure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "SPILUT 1.0\n"
            "3 3\n"
            "2 2 2\n"
            "0 0 0 0.0 0.0 0.0\n"
            "0 0 1 0.0 0.0 0.9\n"
            "0 1 0 0.0 0.7 0.0\n"
            "0 1 1 0.0 0.8 0.8\n"
            "1 0 0 0.7 0.0 0.1\n"
            "1 0 1 0.7 0.6 0.1\n"
            "1 1 0 0.6 0.7 0.1\n"
            "1 1 1 0.6 0.7 0.7\n";

        OCIO_CHECK_NO_THROW(ReadSpi3d(SAMPLE_NO_ERROR));
    }
    {
        // Wrong first line
        const std::string SAMPLE_ERROR =
            "SPI LUT 1.0\n"
            "3 3\n"
            "2 2 2\n"
            "0 0 0 0.0 0.0 0.0\n"
            "0 0 1 0.0 0.0 0.9\n"
            "0 1 0 0.0 0.7 0.0\n"
            "0 1 1 0.0 0.8 0.8\n"
            "1 0 0 0.7 0.0 0.1\n"
            "1 0 1 0.7 0.6 0.1\n"
            "1 1 0 0.6 0.7 0.1\n"
            "1 1 1 0.6 0.7 0.7\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi3d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Expected 'SPILUT'");
    }
    {
        // 3 line is not 3 ints
        const std::string SAMPLE_ERROR =
            "SPILUT 1.0\n"
            "3 3\n"
            "42\n"
            "0 0 0 0.0 0.0 0.0\n"
            "0 0 1 0.0 0.0 0.9\n"
            "0 1 0 0.0 0.7 0.0\n"
            "0 1 1 0.0 0.8 0.8\n"
            "1 0 0 0.7 0.0 0.1\n"
            "1 0 1 0.7 0.6 0.1\n"
            "1 1 0 0.6 0.7 0.1\n"
            "1 1 1 0.6 0.7 0.7\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi3d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Error while reading LUT size");
    }
    {
        // index out of range
        const std::string SAMPLE_ERROR =
            "SPILUT 1.0\n"
            "3 3\n"
            "2 2 2\n"
            "0 2 0 0.0 0.0 0.0\n"
            "0 0 1 0.0 0.0 0.9\n"
            "0 1 0 0.0 0.7 0.0\n"
            "0 1 1 0.0 0.8 0.8\n"
            "1 0 0 0.7 0.0 0.1\n"
            "1 0 1 0.7 0.6 0.1\n"
            "1 1 0 0.6 0.7 0.1\n"
            "1 1 1 0.6 0.7 0.7\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi3d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "that falls outside of the cube");
    }
    {
        // Duplicated indices
        const std::string SAMPLE_ERROR =
            "SPILUT 1.0\n"
            "3 3\n"
            "2 2 2\n"
            "0 0 0 0.0 0.0 0.0\n"
            "0 0 1 0.0 0.0 0.9\n"
            "0 0 1 0.0 0.0 0.9\n"
            "0 1 0 0.0 0.7 0.0\n"
            "0 1 1 0.0 0.8 0.8\n"
            "1 0 1 0.7 0.6 0.1\n"
            "1 1 0 0.6 0.7 0.1\n"
            "1 1 1 0.6 0.7 0.7\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi3d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "A LUT entry is specified multiple times");
    }
    {
        // Not enough entries
        const std::string SAMPLE_ERROR =
            "SPILUT 1.0\n"
            "3 3\n"
            "2 2 2\n"
            "0 0 0 0.0 0.0 0.0\n"
            "0 0 1 0.0 0.0 0.9\n"
            "0 1 0 0.0 0.7 0.0\n"
            "0 1 1 0.0 0.8 0.8\n"
            "1 0 1 0.7 0.6 0.1\n"
            "1 1 0 0.6 0.7 0.1\n"
            "1 1 1 0.6 0.7 0.7\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi3d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Not enough entries found");
    }
}

OCIO_ADD_TEST(FileFormatSpi3D, lut_interpolation_option)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    const std::string filePath{ std::string(OCIO::GetTestFilesDir()) + "/spi_ocio_srgb_test.spi3d" };

    OCIO::FileTransformRcPtr fileTransform = OCIO::FileTransform::Create();
    fileTransform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    fileTransform->setSrc(filePath.c_str());

    // Check that the specified value (INTERP_BEST) may be set.

    fileTransform->setInterpolation(OCIO::INTERP_BEST);
    OCIO::ConstProcessorRcPtr proc;
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    auto group = proc->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_EQUAL(transform->getTransformType(), OCIO::TRANSFORM_TYPE_LUT3D);
    auto lut3D = OCIO_DYNAMIC_POINTER_CAST<OCIO::Lut3DTransform>(transform);
    OCIO_REQUIRE_ASSERT(lut3D);
    OCIO_CHECK_EQUAL(lut3D->getInterpolation(), OCIO::INTERP_BEST);

    // Check that the specified value (INTERP_DEFAULT) may be set.

    fileTransform->setInterpolation(OCIO::INTERP_DEFAULT);
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    group = proc->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    transform = group->getTransform(0);
    lut3D = OCIO_DYNAMIC_POINTER_CAST<OCIO::Lut3DTransform>(transform);
    OCIO_REQUIRE_ASSERT(lut3D);
    OCIO_CHECK_EQUAL(lut3D->getInterpolation(), OCIO::INTERP_DEFAULT);

    // Additional FileTransforms that do not specify interpolation use "default" and not "best",
    // so the order they enter the cache does not matter.
    fileTransform = OCIO::FileTransform::Create();
    fileTransform->setSrc(filePath.c_str());
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    group = proc->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    transform = group->getTransform(0);
    lut3D = OCIO_DYNAMIC_POINTER_CAST<OCIO::Lut3DTransform>(transform);
    OCIO_REQUIRE_ASSERT(lut3D);
    OCIO_CHECK_EQUAL(lut3D->getInterpolation(), OCIO::INTERP_DEFAULT);

    // File transform specify an interpolation that is not supported by LUT3D, log warning and
    // use the default interpolation.

    fileTransform->setInterpolation(OCIO::INTERP_CUBIC);
    {
        OCIO::LogGuard g;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
        OCIO_CHECK_NE(StringUtils::Find(g.output(),
                                        "'cubic' is not allowed with the given file"),
                      std::string::npos);
    }
    group = proc->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    transform = group->getTransform(0);
    lut3D = OCIO_DYNAMIC_POINTER_CAST<OCIO::Lut3DTransform>(transform);
    OCIO_REQUIRE_ASSERT(lut3D);
    OCIO_CHECK_EQUAL(lut3D->getInterpolation(), OCIO::INTERP_DEFAULT);
}
