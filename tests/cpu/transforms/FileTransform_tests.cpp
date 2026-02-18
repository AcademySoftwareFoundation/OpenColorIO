// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <algorithm>

#include "transforms/FileTransform.cpp"

#include "ContextVariableUtils.h"
#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"
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

    OCIO_CHECK_EQUAL(ft->getInterpolation(), OCIO::INTERP_DEFAULT);
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
    const std::string clfMatTransform("clf/pre-smpte_only/matrix_example.clf");
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
    OCIO_CHECK_EQUAL(12, formatRegistry.getNumFormats(OCIO::FORMAT_CAPABILITY_BAKE));
    OCIO_CHECK_EQUAL(5,  formatRegistry.getNumFormats(OCIO::FORMAT_CAPABILITY_WRITE));

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

OCIO_ADD_TEST(FileTransform, is_format_extension_supported)
{
    OCIO::FormatRegistry & formatRegistry = OCIO::FormatRegistry::GetInstance();
    OCIO_CHECK_EQUAL(formatRegistry.isFormatExtensionSupported("foo"), false);
    OCIO_CHECK_EQUAL(formatRegistry.isFormatExtensionSupported("bar"), false);
    OCIO_CHECK_EQUAL(formatRegistry.isFormatExtensionSupported("."), false);
    OCIO_CHECK_ASSERT(formatRegistry.isFormatExtensionSupported("cdl"));
    OCIO_CHECK_ASSERT(formatRegistry.isFormatExtensionSupported(".cdl"));
    OCIO_CHECK_ASSERT(formatRegistry.isFormatExtensionSupported("Cdl"));
    OCIO_CHECK_ASSERT(formatRegistry.isFormatExtensionSupported(".Cdl"));
    OCIO_CHECK_ASSERT(formatRegistry.isFormatExtensionSupported("3dl"));
    OCIO_CHECK_ASSERT(formatRegistry.isFormatExtensionSupported(".3dl"));
}

OCIO_ADD_TEST(FileTransform, validate)
{
    OCIO::FileTransformRcPtr tr = OCIO::FileTransform::Create();

    tr->setSrc("lut3d_17x17x17_32f_12i.clf");
    OCIO_CHECK_NO_THROW(tr->validate());

    tr->setSrc("");
    OCIO_CHECK_THROW_WHAT(tr->validate(), OCIO::Exception,
                          "FileTransform: empty file path");
}

OCIO_ADD_TEST(FileTransform, interpolation_validity)
{
    OCIO::ConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateRaw()->createEditableCopy());
    cfg->setSearchPath(OCIO::GetTestFilesDir().c_str());
    OCIO_CHECK_NO_THROW(cfg->validate());

    OCIO::FileTransformRcPtr tr = OCIO::FileTransform::Create();
    tr->setSrc("lut1d_1.spi1d");

    OCIO_CHECK_NO_THROW(tr->validate());

    // File transform with format requiring a valid interpolation using default interpolation.
    OCIO_CHECK_NO_THROW(cfg->getProcessor(tr));

    // UNKNOWN can't be used by a LUT file, so the interp on the LUT is set to DEFAULT and a
    // warning is logged. 

    tr->setInterpolation(OCIO::INTERP_UNKNOWN);
    OCIO_CHECK_NO_THROW(tr->validate());
    {
        OCIO::LogGuard log;
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);
        OCIO_CHECK_NO_THROW(cfg->getProcessor(tr));
        OCIO_CHECK_EQUAL(log.output(), "[OpenColorIO Warning]: Interpolation specified by "
                                       "FileTransform 'unknown' is not allowed with the "
                                       "given file: 'lut1d_1.spi1d'.\n");
    }

    // TETRAHEDRAL can't be used for Spi1d, default is used instead (and a warning is logged).

    tr->setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    {
        OCIO::LogGuard log;
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);
        OCIO_CHECK_NO_THROW(cfg->getProcessor(tr));
        OCIO_CHECK_EQUAL(log.output(), "[OpenColorIO Warning]: Interpolation specified by "
                                       "FileTransform 'tetrahedral' is not allowed with the "
                                       "given file: 'lut1d_1.spi1d'.\n");
    }

    // Matrices ignore interpolation, so UNKNOWN is ignored and not even logged.  Note that the
    // spi example configs use interpolation=unknown for matrix files.

    tr->setInterpolation(OCIO::INTERP_UNKNOWN);
    tr->setSrc("camera_to_aces.spimtx");
    OCIO_CHECK_NO_THROW(cfg->getProcessor(tr));
}

OCIO_ADD_TEST(FileTransform, context_variables)
{
    // Test context variables with a FileTransform i.e. the file name or the search_path could
    // contain one or several context variables.
 
    OCIO::ContextRcPtr usedContextVars = OCIO::Context::Create();

    OCIO::ConfigRcPtr cfg = OCIO::Config::CreateRaw()->createEditableCopy();
    cfg->setSearchPath(OCIO::GetTestFilesDir().c_str());
    OCIO::ContextRcPtr ctx = cfg->getCurrentContext()->createEditableCopy();


    // Case 1 - The 'filename' contains a context variable.

    OCIO_CHECK_NO_THROW(ctx->setStringVar("ENV1", "exposure_contrast_linear.ctf"));
    OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
    // The 'filename' contains a context variable.
    file->setSrc("$ENV1");

    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *ctx, *file, usedContextVars));

    // Check the used context variables.

    OCIO_CHECK_EQUAL(1, usedContextVars->getNumStringVars());
    OCIO_CHECK_EQUAL(std::string("ENV1"), usedContextVars->getStringVarNameByIndex(0));
    OCIO_CHECK_EQUAL(std::string("exposure_contrast_linear.ctf"), usedContextVars->getStringVarByIndex(0));

    // The 'filename' is *not* anymore a context variable.

    file->setSrc("exposure_contrast_linear.ctf");

    usedContextVars = OCIO::Context::Create(); // New & empty instance.
    OCIO_CHECK_ASSERT(!CollectContextVariables(*cfg, *ctx, *file, usedContextVars));
    OCIO_CHECK_EQUAL(0, usedContextVars->getNumStringVars());


    // Case 2 - The 'search_path' now contains a context variable.

    cfg->setSearchPath("$PATH1");
    ctx = cfg->getCurrentContext()->createEditableCopy();
    file->setSrc("exposure_contrast_linear.ctf");

    OCIO_CHECK_NO_THROW(ctx->setStringVar("PATH1", OCIO::GetTestFilesDir().c_str()));

    usedContextVars = OCIO::Context::Create();
    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *ctx, *file, usedContextVars));

    OCIO_CHECK_EQUAL(1, usedContextVars->getNumStringVars());
    OCIO_CHECK_EQUAL(std::string("PATH1"), usedContextVars->getStringVarNameByIndex(0));
    OCIO_CHECK_EQUAL(OCIO::GetTestFilesDir(), usedContextVars->getStringVarByIndex(0));

    // The 'search_path' is *not* anymore a context variable.
    cfg->setSearchPath(OCIO::GetTestFilesDir().c_str());
    ctx = cfg->getCurrentContext()->createEditableCopy();

    usedContextVars = OCIO::Context::Create(); // New & empty instance.
    OCIO_CHECK_ASSERT(!CollectContextVariables(*cfg, *ctx, *file, usedContextVars));
    OCIO_CHECK_EQUAL(0, usedContextVars->getNumStringVars());


    // Case 3 - The 'filename' and the 'search_path' now contain a context variable.

    cfg->setSearchPath("$PATH1");
    file->setSrc("$ENV1");

    ctx = cfg->getCurrentContext()->createEditableCopy();
    OCIO_CHECK_NO_THROW(ctx->setStringVar("PATH1", OCIO::GetTestFilesDir().c_str()));
    OCIO_CHECK_NO_THROW(ctx->setStringVar("ENV1", "exposure_contrast_linear.ctf"));

    usedContextVars = OCIO::Context::Create(); // New & empty instance.
    OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *ctx, *file, usedContextVars));

    OCIO_CHECK_EQUAL(2, usedContextVars->getNumStringVars());
    OCIO_CHECK_EQUAL(std::string("PATH1"), usedContextVars->getStringVarNameByIndex(0));
    OCIO_CHECK_EQUAL(OCIO::GetTestFilesDir(), usedContextVars->getStringVarByIndex(0));
    OCIO_CHECK_EQUAL(std::string("ENV1"), usedContextVars->getStringVarNameByIndex(1));
    OCIO_CHECK_EQUAL(std::string("exposure_contrast_linear.ctf"), usedContextVars->getStringVarByIndex(1));

    // A basic check to validate that context variables are correctly used. 
    OCIO_CHECK_NO_THROW(cfg->getProcessor(ctx, file, OCIO::TRANSFORM_DIR_FORWARD));


    {
        // Case 4 - The 'cccid' now contains a context variable
        static const std::string CONFIG = 
            "ocio_profile_version: 2\n"
            "\n"
            "environment:\n"
            "  CCPREFIX: cc\n"
            "  CCNUM: 02\n"
            "\n"
            "search_path: " + OCIO::GetTestFilesDir() + "\n"
            "\n"
            "roles:\n"
            "  default: cs1\n"
            "\n"
            "displays:\n"
            "  disp1:\n"
            "    - !<View> {name: view1, colorspace: cs2}\n"
            "\n"
            "colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: cs1\n"
            "\n"
            "  - !<ColorSpace>\n"
            "    name: cs2\n"
            "    from_scene_reference: !<FileTransform> {src: cdl_test1.ccc, cccid: $CCPREFIX00$CCNUM}\n";

            std::istringstream iss;
            iss.str(CONFIG);

            OCIO::ConstConfigRcPtr cfg;
            OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss));
            OCIO_CHECK_NO_THROW(cfg->validate());

            ctx = cfg->getCurrentContext()->createEditableCopy();
            OCIO_CHECK_NO_THROW(ctx->setStringVar("CCNUM", "01"));

            usedContextVars = OCIO::Context::Create(); // New & empty instance.
            OCIO::ConstTransformRcPtr tr1 = cfg->getColorSpace("cs2")->getTransform(
                OCIO::COLORSPACE_DIR_FROM_REFERENCE
            );
            OCIO::ConstFileTransformRcPtr fTr1 = OCIO::DynamicPtrCast<const OCIO::FileTransform>(tr1);
            OCIO_CHECK_ASSERT(fTr1);
            
            OCIO_CHECK_ASSERT(CollectContextVariables(*cfg, *ctx, *fTr1, usedContextVars));
            OCIO_CHECK_EQUAL(2, usedContextVars->getNumStringVars());
            OCIO_CHECK_EQUAL(std::string("CCPREFIX"), usedContextVars->getStringVarNameByIndex(0));
            OCIO_CHECK_EQUAL(std::string("cc"), usedContextVars->getStringVarByIndex(0));
            OCIO_CHECK_EQUAL(std::string("CCNUM"), usedContextVars->getStringVarNameByIndex(1));
            OCIO_CHECK_EQUAL(std::string("01"), usedContextVars->getStringVarByIndex(1));
    }
}

OCIO_ADD_TEST(FileTransform, cc_file_with_different_file_extension)
{
    static const std::string BASE_CONFIG = 
    "ocio_profile_version: 1\n"
    "description: Minimal\n"
    "search_path: " + OCIO::GetTestFilesDir() + "\n"
    "\n"
    "roles:\n"
    "  default: basic\n"
    "  scene_linear: basic\n"
    "  data: basic\n"
    "  reference: basic\n"
    "  compositing_log: basic\n"
    "  color_timing: basic\n"
    "  color_picking: basic\n"
    "  texture_paint: basic\n"
    "  matte_paint: basic\n"
    "  rendering: basic\n"
    "  aces_interchange: basic\n"
    "  cie_xyz_d65_interchange: basic\n"
    "\n"
    "displays:\n"
    "  display:\n"
    "    - !<View> {name: basic, colorspace: basic }\n"
    "    - !<View> {name: cdl, colorspace: basic_cdl }\n"
    "\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "    name: basic\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: basic_cdl\n";

    {
        static const std::string CONFIG = BASE_CONFIG +
        "    from_reference: !<FileTransform> { src: cdl_test_cc_file_with_extension.cdl }\n";
        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConstConfigRcPtr cfg;
        OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss));
        OCIO_CHECK_NO_THROW(cfg->validate());


        OCIO::ConstTransformRcPtr tr1 = cfg->getColorSpace("basic_cdl")->getTransform(
            OCIO::COLORSPACE_DIR_FROM_REFERENCE
        );
        OCIO::ConstFileTransformRcPtr fTr1 = OCIO::DynamicPtrCast<const OCIO::FileTransform>(tr1);
        OCIO_CHECK_ASSERT(fTr1);
        OCIO_CHECK_NO_THROW(cfg->getProcessor(tr1));
    }
    {
        static const std::string CONFIG = BASE_CONFIG +
        "    from_reference: !<FileTransform> { src: cdl_test_cc_file_with_extension.ccc }\n";
        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConstConfigRcPtr cfg;
        OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss));
        OCIO_CHECK_NO_THROW(cfg->validate());


        OCIO::ConstTransformRcPtr tr2 = cfg->getColorSpace("basic_cdl")->getTransform(
            OCIO::COLORSPACE_DIR_FROM_REFERENCE
        );
        OCIO::ConstFileTransformRcPtr fTr2 = OCIO::DynamicPtrCast<const OCIO::FileTransform>(tr2);
        OCIO_CHECK_ASSERT(fTr2);
        OCIO_CHECK_NO_THROW(cfg->getProcessor(tr2));
    }
}
