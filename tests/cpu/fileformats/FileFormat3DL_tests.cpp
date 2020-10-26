// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormat3DL.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO::LocalCachedFileRcPtr LoadLutFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::in);
}

OCIO_ADD_TEST(FileFormat3DL, format_info)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.getFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(2, formatInfoVec.size());
    OCIO_CHECK_EQUAL("flame", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("lustre", formatInfoVec[1].name);
    OCIO_CHECK_EQUAL("3dl", formatInfoVec[0].extension);
    OCIO_CHECK_EQUAL("3dl", formatInfoVec[1].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ | OCIO::FORMAT_CAPABILITY_BAKE,
                     formatInfoVec[0].capabilities);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ | OCIO::FORMAT_CAPABILITY_BAKE,
                     formatInfoVec[1].capabilities);
}

OCIO_ADD_TEST(FileFormat3DL, bake)
{

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
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        const double rgb[3] = { 0.0, 0.1, 0.2 };
        transform1->setOffset(rgb);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    // TODO: Add support for comments in the writer.
    baker->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION,
                                               "MetaData not written");
    baker->setFormat("flame");
    baker->setInputSpace("lnf");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
    baker->setCubeSize(2);
    std::ostringstream outputFlame;
    baker->bake(outputFlame);

    const StringUtils::StringVec osvecFlame = StringUtils::SplitByLines(outputFlame.str());

    std::ostringstream outputLustre;
    baker->setFormat("lustre");
    baker->bake(outputLustre);
    const StringUtils::StringVec osvecLustre = StringUtils::SplitByLines(outputLustre.str());

    std::ostringstream bout;
    bout << "3DMESH" << "\n";
    bout << "Mesh 0 12" << "\n";
    bout << "0 114 227 341 455 568 682 796 909 1023" << "\n";
    bout << "0 410 819" << "\n";
    bout << "0 410 4095" << "\n";
    bout << "0 4095 819" << "\n";
    bout << "0 4095 4095" << "\n";
    bout << "4095 410 819" << "\n";
    bout << "4095 410 4095" << "\n";
    bout << "4095 4095 819" << "\n";
    bout << "4095 4095 4095" << "\n";
    bout << "" << "\n";
    bout << "LUT8" << "\n";
    bout << "gamma 1.0" << "\n";

    const StringUtils::StringVec resvec = StringUtils::SplitByLines(bout.str());
    OCIO_CHECK_EQUAL(resvec.size(), osvecLustre.size());
    OCIO_CHECK_EQUAL(resvec.size() - 4, osvecFlame.size());

    OCIO_CHECK_EQUAL(resvec[0], osvecLustre[0]);
    OCIO_CHECK_EQUAL(resvec[1], osvecLustre[1]);
    for (unsigned int i = 0; i < osvecFlame.size(); ++i)
    {
        OCIO_CHECK_EQUAL(resvec[i+2], osvecFlame[i]);
        OCIO_CHECK_EQUAL(resvec[i+2], osvecLustre[i+2]);
    }
    size_t last = resvec.size() - 2;
    OCIO_CHECK_EQUAL(resvec[last], osvecLustre[last]);
    OCIO_CHECK_EQUAL(resvec[last+1], osvecLustre[last+1]);


}

// FILE      EXPECTED MAX    CORRECTLY DECODED IF MAX IN THIS RANGE 
// 8-bit     255             [0, 511]      
// 10-bit    1023            [512, 2047]
// 12-bit    4095            [2048, 8191]
// 14-bit    16383           [8192, 32767]
// 16-bit    65535           [32768, 131071]

OCIO_ADD_TEST(FileFormat3DL, get_likely_lut_bitdepth)
{
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(-1), -1);
    
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(0), 8);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(1), 8);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(255), 8);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(256), 8);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(511), 8);

    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(512), 10);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(1023), 10);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(1024), 10);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(2047), 10);

    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(2048), 12);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(4095), 12);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(4096), 12);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(8191), 12);

    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(16383), 16);

    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(65535), 16);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(65536), 16);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(131071), 16);

    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(131072), 16);
}


OCIO_ADD_TEST(FileFormat3DL, load)
{
    // Discreet 3D LUT file
    const std::string discree3DtLut("discreet-3d-lut.3dl");

    OCIO::LocalCachedFileRcPtr lutFile;
    OCIO_CHECK_NO_THROW(lutFile = LoadLutFile(discree3DtLut));

    OCIO_REQUIRE_ASSERT(!lutFile->lut1D);
    OCIO_REQUIRE_ASSERT(lutFile->lut3D);

    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT12, lutFile->lut3D->getFileOutputBitDepth());
    OCIO_CHECK_EQUAL(17, lutFile->lut3D->getGridSize());

    const float scale = (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT12);
    // File and lut are using the same order.
    // 41: 54 323 597
    const auto & lutArray = lutFile->lut3D->getArray();
    OCIO_CHECK_EQUAL( 54.f, scale * lutArray[41 * 3]);
    OCIO_CHECK_EQUAL(323.f, scale * lutArray[41 * 3 + 1]);
    OCIO_CHECK_EQUAL(597.f, scale * lutArray[41 * 3 + 2]);

    // 4591: 4025 3426 0
    OCIO_CHECK_EQUAL(4025.f, scale * lutArray[4591 * 3]);
    OCIO_CHECK_EQUAL(3426.f, scale * lutArray[4591 * 3 + 1]);
    OCIO_CHECK_EQUAL(   0.f, scale * lutArray[4591 * 3 + 2]);

    const std::string discree3DtLutFail("error_truncated_file.3dl");

    OCIO_CHECK_THROW_WHAT(LoadLutFile(discree3DtLutFail),
                          OCIO::Exception,
                          "Cannot infer 3D LUT size");

}

OCIO::LocalCachedFileRcPtr Read3dl(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME, OCIO::INTERP_DEFAULT);
    return OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
}

OCIO_ADD_TEST(FileFormat3DL, parse_1d)
{
    {
        // Rounding down test.
        const std::string NO3DLUT =
            "#Tokens required by applications - do not edit\n"
            "\n"
            "3DMESH\n"
            "Mesh 4 10\n"
            "0 63 127 191 255 319 383 447 511 575 639 703 767 831 895 959 1023\n";

        OCIO::LocalCachedFileRcPtr lutFile = Read3dl(NO3DLUT);

        OCIO_REQUIRE_ASSERT(lutFile);

        OCIO_CHECK_ASSERT(!lutFile->lut1D);
        OCIO_CHECK_ASSERT(!lutFile->lut3D);
    }
    {
        // Rounding up test.
        const std::string NO3DLUT =
            "#Tokens required by applications - do not edit\n"
            "\n"
            "3DMESH\n"
            "Mesh 4 10\n"
            "0 64 128 192 256 320 384 448 512 576 640 704 768 832 896 960 1023\n";

        OCIO::LocalCachedFileRcPtr lutFile = Read3dl(NO3DLUT);

        OCIO_REQUIRE_ASSERT(lutFile);

        OCIO_CHECK_ASSERT(!lutFile->lut1D);
    }
    {
        // Not an identity test.
        const std::string NO3DLUT =
            "#Tokens required by applications - do not edit\n"
            "\n"
            "3DMESH\n"
            "Mesh 4 10\n"
            "0 64 128 192 256 320 384 448 512 576 640 704 768 832 896 960 1020\n";

        OCIO::LocalCachedFileRcPtr lutFile = Read3dl(NO3DLUT);

        OCIO_REQUIRE_ASSERT(lutFile);

        OCIO_REQUIRE_ASSERT(lutFile->lut1D);
        OCIO_CHECK_EQUAL(lutFile->lut1D->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    }
}
