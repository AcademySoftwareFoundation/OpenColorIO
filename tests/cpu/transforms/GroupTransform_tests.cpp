// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/GroupTransform.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(GroupTransform, basic)
{
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    OCIO_CHECK_EQUAL(group->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    group->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(group->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_EQUAL(group->getNumTransforms(), 0);

    auto & groupData = group->getFormatMetadata();
    OCIO_CHECK_EQUAL(std::string(groupData.getElementName()), OCIO::METADATA_ROOT);
    OCIO_CHECK_EQUAL(groupData.getNumAttributes(), 0);
    OCIO_CHECK_EQUAL(groupData.getNumChildrenElements(), 0);

    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();
    group->appendTransform(matrix);
    OCIO::FixedFunctionTransformRcPtr ff =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    group->appendTransform(ff);

    OCIO_CHECK_EQUAL(group->getNumTransforms(), 2);

    auto t0 = group->getTransform(0);
    auto m0 = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(t0);
    OCIO_CHECK_ASSERT(m0);

    auto t1 = group->getTransform(1);
    auto ff1 = OCIO_DYNAMIC_POINTER_CAST<OCIO::FixedFunctionTransform>(t1);
    OCIO_CHECK_ASSERT(ff1);

    auto & metadata = group->getFormatMetadata();
    OCIO_CHECK_EQUAL(std::string(metadata.getElementName()), OCIO::METADATA_ROOT);
    OCIO_CHECK_EQUAL(std::string(metadata.getElementValue()), "");
    OCIO_CHECK_EQUAL(metadata.getNumAttributes(), 0);
    OCIO_CHECK_EQUAL(metadata.getNumChildrenElements(), 0);
    metadata.addAttribute("att1", "val1");
    metadata.addChildElement("child1", "content1");
}

namespace
{
std::string GetFormatName(const std::string & extension)
{
    // All extensions are lower case.
    std::string requestedExt{ StringUtils::Lower(extension) };
    for (int i = 0; i < OCIO::GroupTransform::GetNumWriteFormats(); ++i)
    {
        std::string formatExt(OCIO::GroupTransform::GetFormatExtensionByIndex(i));
        if (requestedExt == formatExt)
        {
            return std::string{ OCIO::GroupTransform::GetFormatNameByIndex(i) };
        }
    }
    return std::string{ "" };
}
}

OCIO_ADD_TEST(GroupTransform, write_formats)
{
    OCIO_CHECK_EQUAL(OCIO::GroupTransform::GetNumWriteFormats(), 5);

    OCIO_CHECK_EQUAL(GetFormatName("CLF"), OCIO::FILEFORMAT_CLF);
    OCIO_CHECK_EQUAL(GetFormatName("CTF"), OCIO::FILEFORMAT_CTF);
    OCIO_CHECK_EQUAL(GetFormatName("cc"), OCIO::FILEFORMAT_COLOR_CORRECTION);
    OCIO_CHECK_EQUAL(GetFormatName("ccc"), OCIO::FILEFORMAT_COLOR_CORRECTION_COLLECTION);
    OCIO_CHECK_EQUAL(GetFormatName("cdl"), OCIO::FILEFORMAT_COLOR_DECISION_LIST);
    OCIO_CHECK_ASSERT(GetFormatName("XXX").empty());
}

OCIO_ADD_TEST(GroupTransform, write_with_noops)
{
    // The unit test validates that the no-ops are transparent when writing to a lut file format.

    const OCIO::ConstConfigRcPtr config = OCIO::Config::Create();

    OCIO::FileTransformRcPtr file;
    OCIO::GroupTransformRcPtr group;
    OCIO::ConstProcessorRcPtr proc;

    // Step 1 - Write to CLF from a group transform.

    {
        OCIO_CHECK_NO_THROW(file = OCIO::CreateFileTransform("logtolin_8to8.lut"));

        group = OCIO::GroupTransform::Create();
        OCIO_CHECK_NO_THROW(group->appendTransform(file));

        std::ostringstream oss;
        OCIO_CHECK_NO_THROW(group->write(config, OCIO::FILEFORMAT_CLF, oss));
    }
    
    // Step 2 - Write to CLF from a processor.

    {
        OCIO_CHECK_NO_THROW(file = OCIO::CreateFileTransform("logtolin_8to8.lut"));

        OCIO_CHECK_NO_THROW(proc = config->getProcessor(file));
        OCIO_CHECK_NO_THROW(group = proc->createGroupTransform());

        std::ostringstream oss;
        OCIO_CHECK_NO_THROW(group->write(config, OCIO::FILEFORMAT_CLF, oss));
    }
}

namespace
{
std::string ReadTestFile(const std::string & fileName)
{
    const std::string filePath(OCIO::GetTestFilesDir() + "/" + fileName);

    std::ifstream fstream;
    OCIO::Platform::OpenInputFileStream(fstream,
                                        filePath.c_str(),
                                        std::ios_base::in | std::ios_base::binary);
    if (fstream.fail())
    {
        std::ostringstream os;
        os << "Error opening test file: " << filePath;
        throw OCIO::Exception(os.str().c_str());
    }

    std::stringstream buffer;
    buffer << fstream.rdbuf();
    return buffer.str();
}
}

OCIO_ADD_TEST(GroupTransform, parse_from_buffer)
{
    // Parse a SPI1D LUT from a memory buffer.
    {
        const std::string content = ReadTestFile("lut1d_1.spi1d");

        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = OCIO::GroupTransform::ParseFromBuffer(content.c_str(),
                                                                          content.size()));
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);

        auto lut = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Lut1DTransform>(group->getTransform(0));
        OCIO_REQUIRE_ASSERT(lut);
        OCIO_CHECK_EQUAL(lut->getLength(), 512U);

        float r = 0.f, g = 0.f, b = 0.f;
        lut->getValue(1, r, g, b);
        OCIO_CHECK_CLOSE(r, 0.00195695f, 1e-8f);
        OCIO_CHECK_CLOSE(g, 0.00195695f, 1e-8f);
        OCIO_CHECK_CLOSE(b, 0.00195695f, 1e-8f);

        // Parsing the same buffer again works and gives the same result.
        OCIO::GroupTransformRcPtr group2;
        OCIO_CHECK_NO_THROW(group2 = OCIO::GroupTransform::ParseFromBuffer(content.c_str(),
                                                                           content.size()));
        OCIO_REQUIRE_ASSERT(group2);
        OCIO_REQUIRE_EQUAL(group2->getNumTransforms(), 1);
    }

    // Parse a CLF LUT from a memory buffer. This also validates that a different buffer
    // parsed within the same process is not confused with the previous one by the global
    // file caches.
    {
        const std::string content = ReadTestFile("clf/lut1d_example.clf");

        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = OCIO::GroupTransform::ParseFromBuffer(content.c_str(),
                                                                          content.size()));
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);

        auto lut = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Lut1DTransform>(group->getTransform(0));
        OCIO_REQUIRE_ASSERT(lut);
        OCIO_CHECK_EQUAL(lut->getLength(), 65U);
    }

    // Null or empty buffer must throw.
    {
        OCIO_CHECK_THROW_WHAT(OCIO::GroupTransform::ParseFromBuffer(nullptr, 0),
                              OCIO::Exception,
                              "buffer is null or empty");

        const std::string content = ReadTestFile("lut1d_1.spi1d");
        OCIO_CHECK_THROW_WHAT(OCIO::GroupTransform::ParseFromBuffer(content.c_str(), 0),
                              OCIO::Exception,
                              "buffer is null or empty");
    }

    // Malformed buffer contents must throw rather than crash or silently succeed.
    {
        const std::string content = "This is not the content of any supported LUT format.";
        OCIO_CHECK_THROW_WHAT(OCIO::GroupTransform::ParseFromBuffer(content.c_str(),
                                                                    content.size()),
                              OCIO::Exception,
                              "Error parsing LUT from buffer");
    }
}
