// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatDiscreet1DL.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
void TestToolsStripBlank(const char * stringToStripChar,
                         const std::string & stringResult)
{
    constexpr size_t stringToStripSize = 200;
    char stringToStrip[stringToStripSize];
    snprintf(stringToStrip, stringToStripSize, "%s", stringToStripChar);
    OCIO::ReplaceTabsAndStripSpaces(stringToStrip);
    std::string strippedString(stringToStrip);
    OCIO_CHECK_EQUAL(stringResult, strippedString);
}

void TestToolsStripEndNewLine(const char * stringToStripChar,
                              const std::string & stringResult)
{

    constexpr size_t stringToStripSize = 200;
    char stringToStrip[stringToStripSize];
    snprintf(stringToStrip, stringToStripSize, "%s", stringToStripChar);
    OCIO::StripEndNewLine(stringToStrip);
    std::string strippedString(stringToStrip);
    OCIO_CHECK_EQUAL(stringResult, strippedString);
}

}

OCIO_ADD_TEST(FileFormatD1DL, test_string_util)
{
    TestToolsStripBlank("this is a test", "this is a test");
    TestToolsStripBlank("   this is a test      ", "this is a test");
    TestToolsStripBlank(" \t  this\tis a test    \t  ", "this is a test");
    TestToolsStripBlank("\t \t  this is a  test    \t  \t", "this is a  test");
    TestToolsStripBlank("\t \t  this\nis a\t\ttest    \t  \t", "this\nis a  test");
    TestToolsStripBlank("", "");

    TestToolsStripEndNewLine("", "");
    TestToolsStripEndNewLine("\n", "");
    TestToolsStripEndNewLine("\r", "");
    TestToolsStripEndNewLine("a\n", "a");
    TestToolsStripEndNewLine("b\r", "b");
    TestToolsStripEndNewLine("\na", "\na");
    TestToolsStripEndNewLine("\rb", "\rb");
}

OCIO::LocalCachedFileRcPtr LoadLutFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::in);
}

OCIO_ADD_TEST(FileFormatD1DL, test_lut1d_8i_8i)
{
    OCIO::LocalCachedFileRcPtr lutFile;
    const std::string discreetLut("logtolin_8to8.lut");
    OCIO_CHECK_NO_THROW(lutFile = LoadLutFile(discreetLut));

    OCIO_CHECK_EQUAL(lutFile->lut1D->getID(), "");
    OCIO_CHECK_EQUAL(lutFile->lut1D->getName(), "");

    OCIO_CHECK_EQUAL(lutFile->lut1D->getInterpolation(), OCIO::INTERP_DEFAULT);
    OCIO_CHECK_EQUAL(lutFile->lut1D->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    OCIO_CHECK_ASSERT(!lutFile->lut1D->isInputHalfDomain());
    OCIO_CHECK_ASSERT(!lutFile->lut1D->isOutputRawHalfs());

    OCIO_CHECK_EQUAL(lutFile->lut1D->getArray().getLength(), 256ul);
    OCIO_CHECK_EQUAL(lutFile->lut1D->getArray().getNumValues(), (unsigned long)256 * 3);
    OCIO_CHECK_EQUAL(lutFile->lut1D->getArray().getNumColorComponents(), 3ul);

    // Select some samples to verify the LUT was fully read.
    const unsigned long sampleInterval = 13ul;
    const float expectedSampleValues[] =
    {
          0.0,   0.0,   0.0,   0.0,   0.0,   0.0,   1.0,   3.0,   6.0,   9.0,
         12.0,  15.0,  18.0,  22.0,  25.0,  30.0,  33.0,  37.0,  43.0,  48.0,
         52.0,  59.0,  64.0,  70.0,  78.0,  85.0,  92.0, 101.0, 109.0, 117.0,
        129.0, 138.0, 148.0, 161.0, 173.0, 185.0, 201.0, 214.0, 229.0, 248.0,
        255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0,
        255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 255.0
    };

    const OCIO::Array::Values & lut1DValues = lutFile->lut1D->getArray().getValues();
    for (unsigned long li = 0, ei = 0; li<lut1DValues.size(); li += sampleInterval, ++ei)
    {
        OCIO_CHECK_EQUAL(lut1DValues[li] * 255.0f, expectedSampleValues[ei]);
    }
}

OCIO_ADD_TEST(FileFormatD1DL, test_lut1d_12i_16f)
{
    OCIO::LocalCachedFileRcPtr lutFile;
    const std::string discreetLut1216fp("Test_12to16fp.lut");
    OCIO_CHECK_NO_THROW(lutFile = LoadLutFile(discreetLut1216fp));

    OCIO_CHECK_EQUAL(lutFile->lut1D->getID(), "");
    OCIO_CHECK_EQUAL(lutFile->lut1D->getName(), "");

    OCIO_CHECK_EQUAL(lutFile->lut1D->getInterpolation(), OCIO::INTERP_DEFAULT);
    OCIO_CHECK_EQUAL(lutFile->lut1D->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F16);

    OCIO_CHECK_ASSERT(!lutFile->lut1D->isInputHalfDomain());
    OCIO_CHECK_ASSERT(!lutFile->lut1D->isOutputRawHalfs());

    OCIO_CHECK_EQUAL(lutFile->lut1D->getArray().getLength(), 4096ul);
    OCIO_CHECK_EQUAL(lutFile->lut1D->getArray().getNumValues(), (unsigned long)4096 * 3);
    OCIO_CHECK_EQUAL(lutFile->lut1D->getArray().getNumColorComponents(), 3ul);

    // Select some samples to verify the LUT was fully read.
    const unsigned long sampleInterval = 207ul;
    const unsigned short expectedSampleValues[] =
    {
            0, 12546, 13171, 13491, 13705, 13898, 14074, 14238, 14365, 14438,
        14507, 14574, 14638, 14700, 14760, 14818, 14875, 14930, 14983, 15037,
        15094, 15156, 15222, 15294, 15366, 15408, 15453, 15501, 15553, 15609,
        15669, 15733, 15802, 15876, 15954, 16038, 16128, 16224, 16327, 16410,
        16468, 16530, 16596, 16667, 16741, 16821, 16905, 16995, 17090, 17191,
        17298, 17410, 17470, 17534, 17602, 17673, 17749, 17829, 17914, 18003
    };

    const OCIO::Array::Values & lut1DValues = lutFile->lut1D->getArray().getValues();
    for (unsigned long li = 0, ei = 0; li<lut1DValues.size(); li += sampleInterval, ++ei)
    {
        OCIO_CHECK_EQUAL(half(lut1DValues[li]).bits(), expectedSampleValues[ei]);
    }
}

OCIO_ADD_TEST(FileFormatD1DL, test_lut1d_16f_16f)
{
    OCIO::LocalCachedFileRcPtr lutFile;
    const std::string discreetLut16fp16fp("photo_default_16fpto16fp.lut");
    OCIO_CHECK_NO_THROW(lutFile = LoadLutFile(discreetLut16fp16fp));

    OCIO_CHECK_EQUAL(lutFile->lut1D->getInterpolation(), OCIO::INTERP_DEFAULT);
    OCIO_CHECK_EQUAL(lutFile->lut1D->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F16);

    OCIO_CHECK_ASSERT(lutFile->lut1D->isInputHalfDomain());
    OCIO_CHECK_ASSERT(!lutFile->lut1D->isOutputRawHalfs());

    OCIO_CHECK_EQUAL(lutFile->lut1D->getArray().getLength(), 65536ul);
    OCIO_CHECK_EQUAL(lutFile->lut1D->getArray().getNumValues(), (unsigned long)65536 * 3);
    OCIO_CHECK_EQUAL(lutFile->lut1D->getArray().getNumColorComponents(), 3ul);

    // Select some samples to verify the LUT was fully read.
    const unsigned long sampleInterval = 3277ul;
    const float expectedSampleValues[] =
    {
            0,   242,   554,  1265,  2463,  3679,  4918,  6234,  7815,  9945,
        11918, 13222, 14063, 14616, 14958, 15176, 15266, 15349, 15398, 15442,
        15488, 15536, 15586, 15637, 15690, 15745, 15802, 15862, 15923, 15987,
        32770, 33862, 34954, 36047, 37139, 38231, 39324, 40416, 41508, 42601,
        43693, 44785, 45878, 46970, 48062, 49155, 50247, 51339, 52432, 53524,
        54616, 55709, 56801, 57893, 58986, 60078, 61170, 62263, 63355, 64447
    };

    const OCIO::Array::Values & lut1DValues = lutFile->lut1D->getArray().getValues();
    for (unsigned li = 0, ei = 0; li<lut1DValues.size(); li += sampleInterval, ++ei)
    {
        OCIO_CHECK_EQUAL(half(lut1DValues[li]).bits(), expectedSampleValues[ei]);
    }
}

OCIO_ADD_TEST(FileFormatD1DL, test_lut1d_16f_12i)
{
    OCIO::LocalCachedFileRcPtr lutFile;
    const std::string discreetLut16fp12("Test_16fpto12.lut");
    OCIO_CHECK_NO_THROW(lutFile = LoadLutFile(discreetLut16fp12));

    OCIO_CHECK_EQUAL(lutFile->lut1D->getInterpolation(), OCIO::INTERP_DEFAULT);
    OCIO_CHECK_EQUAL(lutFile->lut1D->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    OCIO_CHECK_ASSERT(lutFile->lut1D->isInputHalfDomain());
    OCIO_CHECK_ASSERT(!lutFile->lut1D->isOutputRawHalfs());

    OCIO_CHECK_EQUAL(lutFile->lut1D->getArray().getLength(), 65536ul);
    OCIO_CHECK_EQUAL(lutFile->lut1D->getArray().getNumValues(), (unsigned long)65536 * 3);
    OCIO_CHECK_EQUAL(lutFile->lut1D->getArray().getNumColorComponents(), 3ul);

    // Select some samples to verify the LUT was fully read.
    const unsigned long sampleInterval = 3277ul;
    const float expectedSampleValues[] =
    {
           0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    1.0f,    3.0f,
          10.0f,   36.0f,  130.0f,  466.0f, 1585.0f, 2660.0f, 3595.0f, 4095.0f, 4095.0f, 4095.0f,
        4095.0f, 4095.0f, 4095.0f, 4095.0f, 4095.0f, 4095.0f, 4095.0f, 4095.0f, 4095.0f, 4095.0f,
           0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,
           0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,
           0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f
    };

    const OCIO::Array::Values & lut1DValues = lutFile->lut1D->getArray().getValues();
    for (unsigned li = 0, ei = 0; li<lut1DValues.size(); li += sampleInterval, ++ei)
    {
        OCIO_CHECK_EQUAL(lut1DValues[li] * 4095.0f, expectedSampleValues[ei]);
    }
}

OCIO_ADD_TEST(FileFormatD1DL, test_bad_file)
{
    // Bad file.
    const std::string truncatedLut("error_truncated_file.lut");
    OCIO_CHECK_THROW(LoadLutFile(truncatedLut), OCIO::Exception);
}
