// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <algorithm>

#include "transforms/FileTransform.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FileTransform, basic)
{
    auto ft = OCIO::FileTransform::Create();
    OCIO_CHECK_EQUAL(ft->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    ft->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(ft->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_EQUAL(std::string(ft->getSrc()), "");
    const std::string src{ "source" };
    ft->setSrc(src.c_str());
    OCIO_CHECK_EQUAL(src, ft->getSrc());

    OCIO_CHECK_EQUAL(std::string(ft->getCCCId()), "");
    const std::string cccid{ "cccid" };
    ft->setCCCId(cccid.c_str());
    OCIO_CHECK_EQUAL(cccid, ft->getCCCId());

    OCIO_CHECK_EQUAL(ft->getCDLStyle(), OCIO::CDL_NO_CLAMP);
    ft->setCDLStyle(OCIO::CDL_ASC);
    OCIO_CHECK_EQUAL(ft->getCDLStyle(), OCIO::CDL_ASC);

    OCIO_CHECK_EQUAL(ft->getInterpolation(), OCIO::INTERP_UNKNOWN);
    ft->setInterpolation(OCIO::INTERP_LINEAR);
    OCIO_CHECK_EQUAL(ft->getInterpolation(), OCIO::INTERP_LINEAR);
}

OCIO_ADD_TEST(FileTransform, load_file_ok)
{
    OCIO::ConstProcessorRcPtr proc;

    // Discreet 1D LUT.
    const std::string discreetLut("logtolin_8to8.lut");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(discreetLut));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    // Houdini 1D LUT.
    const std::string houdiniLut("houdini.lut");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(houdiniLut));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    // Discreet 3D LUT file.
    const std::string discree3DtLut("discreet-3d-lut.3dl");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(discree3DtLut));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    // 3D LUT file.
    const std::string crosstalk3DtLut("crosstalk.3dl");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(crosstalk3DtLut));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    // Lustre 3D LUT file.
    const std::string lustre3DtLut("lustre_33x33x33.3dl");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(lustre3DtLut));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    // Autodesk color transform format.
    const std::string ctfTransform("matrix_example4x4.ctf");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(ctfTransform));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    // Academy/ASC common LUT format.
    const std::string clfRangeTransform("clf/range.clf");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(clfRangeTransform));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    // Academy/ASC common LUT format.
    const std::string clfMatTransform("clf/matrix_example.clf");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(clfMatTransform));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    // Test other types of CLF/CTF elements.
    const std::string clfCdlTransform("clf/cdl_clamp_fwd.clf");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(clfCdlTransform));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string clfLut1Transform("clf/lut1d_example.clf");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(clfLut1Transform));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string clfLut3Transform("clf/lut3d_identity_12i_16f.clf");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(clfLut3Transform));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string ctFFfTransform("fixed_function.ctf");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(ctFFfTransform));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string ctfGammaTransform("gamma_test1.ctf");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(ctfGammaTransform));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string ctfLogTransform("log_logtolin.ctf");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(ctfLogTransform));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string ctfInvLut1Transform("lut1d_inv.ctf");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(ctfInvLut1Transform));
    OCIO_CHECK_ASSERT(!proc->isNoOp());

    const std::string ctfInvLut3Transform("lut3d_example_Inv.ctf");
    OCIO_CHECK_NO_THROW(proc = OCIO::GetFileTransformProcessor(ctfInvLut3Transform));
    OCIO_CHECK_ASSERT(!proc->isNoOp());
}

OCIO_ADD_TEST(FileTransform, load_file_fail)
{
    // Legacy Lustre 1D LUT files. Similar to supported formats but actually
    // are different formats.
    // Test that they are correctly recognized as unreadable.
    {
        const std::string lustreOldLut("legacy_slog_to_log_v3_lustre.lut");
        OCIO_CHECK_THROW_WHAT(OCIO::GetFileTransformProcessor(lustreOldLut),
                              OCIO::Exception, "could not be loaded");
    }

    {
        const std::string lustreOldLut("legacy_flmlk_desat.lut");
        OCIO_CHECK_THROW_WHAT(OCIO::GetFileTransformProcessor(lustreOldLut),
                              OCIO::Exception, "could not be loaded");
    }

    // Invalid ASCII file.
    const std::string unKnown("error_unknown_format.txt");
    OCIO_CHECK_THROW_WHAT(OCIO::GetFileTransformProcessor(unKnown),
                          OCIO::Exception, "error_unknown_format.txt' could not be loaded");

    // Unsupported file extension.
    // It's in fact a binary jpg file i.e. all readers must fail.
    const std::string binaryFile("rgb-cmy.jpg");
    OCIO_CHECK_THROW_WHAT(OCIO::GetFileTransformProcessor(binaryFile),
                          OCIO::Exception, "rgb-cmy.jpg' could not be loaded");

    // Supported file extension with a wrong content.
    // It's in fact a binary png file i.e. all readers must fail.
    const std::string faultyCLFFile("clf/illegal/image_png.clf");
    OCIO_CHECK_THROW_WHAT(OCIO::GetFileTransformProcessor(faultyCLFFile), 
                          OCIO::Exception, "image_png.clf' could not be loaded");

    // Missing file.
    const std::string missing("missing.file");
    OCIO_CHECK_THROW_WHAT(OCIO::GetFileTransformProcessor(missing),
                          OCIO::Exception, "missing.file' could not be located");
}

namespace
{
bool FormatNameFoundByExtension(const std::string & extension, const std::string & formatName)
{
    bool foundIt = false;
    OCIO::FormatRegistry & formatRegistry = OCIO::FormatRegistry::GetInstance();

    OCIO::FileFormatVector possibleFormats;
    formatRegistry.getFileFormatForExtension(extension, possibleFormats);
    OCIO::FileFormatVector::const_iterator endFormat = possibleFormats.end();
    OCIO::FileFormatVector::const_iterator itFormat = possibleFormats.begin();
    while (itFormat != endFormat && !foundIt)
    {
        OCIO::FileFormat * tryFormat = *itFormat;

        if (formatName == tryFormat->getName())
            foundIt = true;

        ++itFormat;
    }
    return foundIt;
}

bool FormatExtensionFoundByName(const std::string & extension, const std::string & formatName)
{
    bool foundIt = false;
    OCIO::FormatRegistry & formatRegistry = OCIO::FormatRegistry::GetInstance();

    OCIO::FileFormat * fileFormat = formatRegistry.getFileFormatByName(formatName);
    if (fileFormat)
    {
        OCIO::FormatInfoVec formatInfoVec;
        fileFormat->getFormatInfo(formatInfoVec);

        for (unsigned int i = 0; i < formatInfoVec.size() && !foundIt; ++i)
        {
            if (extension == formatInfoVec[i].extension)
                foundIt = true;

        }
    }
    return foundIt;
}
}

OCIO_ADD_TEST(FileTransform, all_formats)
{
    OCIO::FormatRegistry & formatRegistry = OCIO::FormatRegistry::GetInstance();
    OCIO_CHECK_EQUAL(19, formatRegistry.getNumRawFormats());
    OCIO_CHECK_EQUAL(24, formatRegistry.getNumFormats(OCIO::FORMAT_CAPABILITY_READ));
    OCIO_CHECK_EQUAL(10, formatRegistry.getNumFormats(OCIO::FORMAT_CAPABILITY_BAKE));
    OCIO_CHECK_EQUAL(2,  formatRegistry.getNumFormats(OCIO::FORMAT_CAPABILITY_WRITE));

    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("3dl", "flame"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("cc", "ColorCorrection"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("ccc", "ColorCorrectionCollection"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("cdl", "ColorDecisionList"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("clf", OCIO::FILEFORMAT_CLF));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("csp", "cinespace"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("cub", "truelight"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("cube", "iridas_cube"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("cube", "resolve_cube"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("itx", "iridas_itx"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("icc", "International Color Consortium profile"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("look", "iridas_look"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("lut", "houdini"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("lut", "Discreet 1D LUT"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("mga", "pandora_mga"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("spi1d", "spi1d"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("spi3d", "spi3d"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("spimtx", "spimtx"));
    OCIO_CHECK_ASSERT(FormatNameFoundByExtension("vf", "nukevf"));
    // When a FileFormat handles 2 "formats" it declares both names
    // but only exposes one name using the getName() function.
    OCIO_CHECK_ASSERT(!FormatNameFoundByExtension("3dl", "lustre"));
    OCIO_CHECK_ASSERT(!FormatNameFoundByExtension("m3d", "pandora_m3d"));
    OCIO_CHECK_ASSERT(!FormatNameFoundByExtension("icm", "Image Color Matching"));
    OCIO_CHECK_ASSERT(!FormatNameFoundByExtension("ctf", OCIO::FILEFORMAT_CTF));

    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("3dl", "flame"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("3dl", "lustre"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("cc", "ColorCorrection"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("ccc", "ColorCorrectionCollection"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("cdl", "ColorDecisionList"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("clf", OCIO::FILEFORMAT_CLF));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("ctf", OCIO::FILEFORMAT_CTF));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("csp", "cinespace"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("cub", "truelight"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("cube", "iridas_cube"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("cube", "resolve_cube"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("itx", "iridas_itx"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("icc", "International Color Consortium profile"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("icm", "International Color Consortium profile"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("look", "iridas_look"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("lut", "houdini"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("lut", "Discreet 1D LUT"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("m3d", "pandora_m3d"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("mga", "pandora_mga"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("spi1d", "spi1d"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("spi3d", "spi3d"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("spimtx", "spimtx"));
    OCIO_CHECK_ASSERT(FormatExtensionFoundByName("vf", "nukevf"));
}

namespace
{
void ValidateFormatByIndex(OCIO::FormatRegistry &reg, int cap)
{
    int numFormat = reg.getNumFormats(cap);

    // Check out of bounds access
    OCIO_CHECK_EQUAL(0, OCIO::Platform::Strcasecmp(reg.getFormatNameByIndex(cap, -1), ""));
    OCIO_CHECK_EQUAL(0, OCIO::Platform::Strcasecmp(reg.getFormatExtensionByIndex(cap, -1), ""));
    OCIO_CHECK_EQUAL(0, OCIO::Platform::Strcasecmp(reg.getFormatNameByIndex(cap, numFormat), ""));
    OCIO_CHECK_EQUAL(0, OCIO::Platform::Strcasecmp(reg.getFormatExtensionByIndex(cap, numFormat), ""));

    // Check valid access
    for (int i = 0; i < numFormat; ++i) {
        OCIO_CHECK_NE(0, OCIO::Platform::Strcasecmp(reg.getFormatNameByIndex(cap, i), ""));
        OCIO_CHECK_NE(0, OCIO::Platform::Strcasecmp(reg.getFormatExtensionByIndex(cap, i), ""));
    }
}
}

OCIO_ADD_TEST(FileTransform, format_by_index)
{
    OCIO::FormatRegistry & formatRegistry = OCIO::FormatRegistry::GetInstance();
    ValidateFormatByIndex(formatRegistry, OCIO::FORMAT_CAPABILITY_WRITE);
    ValidateFormatByIndex(formatRegistry, OCIO::FORMAT_CAPABILITY_BAKE);
    ValidateFormatByIndex(formatRegistry, OCIO::FORMAT_CAPABILITY_READ);
}

OCIO_ADD_TEST(FileTransform, validate)
{
    OCIO::FileTransformRcPtr tr = OCIO::FileTransform::Create();

    tr->setSrc("lut3d_17x17x17_32f_12i.clf");
    OCIO_CHECK_NO_THROW(tr->validate());

    tr->setSrc("");
    OCIO_CHECK_THROW(tr->validate(), OCIO::Exception);
}
