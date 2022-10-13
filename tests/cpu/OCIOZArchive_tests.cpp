// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "OpenColorIO/OpenColorIO.h"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{
struct FileCreationGuard
{
    explicit FileCreationGuard(unsigned lineNo)
    {
        OCIO_CHECK_NO_THROW_FROM(m_filename = OCIO::Platform::CreateTempFilename(""), lineNo);
    }
    ~FileCreationGuard()
    {
        // Even if not strictly required on most OSes, perform the cleanup.
        std::remove(m_filename.c_str());
    }

    std::string m_filename;
};

struct DirectoryCreationGuard
{
    explicit DirectoryCreationGuard(const std::string name, unsigned lineNo)
    {
        OCIO_CHECK_NO_THROW_FROM(
            m_directoryPath = OCIO::CreateTemporaryDirectory(name), lineNo
        );
    }
    ~DirectoryCreationGuard()
    {
        // Even if not strictly required on most OSes, perform the cleanup.
        OCIO::RemoveTemporaryDirectory(m_directoryPath);
    }

    std::string m_directoryPath;
};
} //anon.


OCIO_ADD_TEST(OCIOZArchive, is_config_archivable)
{
    // This test primarily tests the isArchivable method from the Config object

    const std::string CONFIG = 
        "ocio_profile_version: 2\n"
        "\n"
        "search_path:\n"
        "  - abc\n"
        "  - def\n"
        "environment:\n"
        "  MYLUT: exposure_contrast_linear.ctf\n"
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
        "    from_scene_reference: !<FileTransform> {src: ./$MYLUT}\n";

    std::istringstream iss;
    iss.str(CONFIG);

    OCIO::ConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
    // Since a working directory is needed to archive a config, setting a fake working directory 
    // in order to test the search paths and FileTransform source logic.
#ifdef _WIN32
    cfg->setWorkingDir(R"(C:\fake_working_dir)");
#else
    cfg->setWorkingDir("/fake_working_dir");
#endif
    OCIO_CHECK_NO_THROW(cfg->validate());

    // Testing a few scenario by modifying the search_paths.

    // Testing search paths
    {
        /*
         * Legal scenarios
         */
        
        // Valid search path.
        cfg->setSearchPath("luts");
        OCIO_CHECK_EQUAL(true, cfg->isArchivable());

        cfg->setSearchPath(R"(luts/myluts1)");
        OCIO_CHECK_EQUAL(true, cfg->isArchivable());

        cfg->setSearchPath(R"(luts\myluts1)");
        OCIO_CHECK_EQUAL(true, cfg->isArchivable());

        // Valid Search path starting with "./" or ".\".
        cfg->setSearchPath(R"(./myLuts)");
        OCIO_CHECK_EQUAL(true, cfg->isArchivable());

        cfg->setSearchPath(R"(.\myLuts)");
        OCIO_CHECK_EQUAL(true, cfg->isArchivable());

        // Valid search path starting with "./" or ".\" and a context variable.
        cfg->setSearchPath(R"(./$SHOT/myluts)");
        OCIO_CHECK_EQUAL(true, cfg->isArchivable());

        cfg->setSearchPath(R"(.\$SHOT\myluts)");
        OCIO_CHECK_EQUAL(true, cfg->isArchivable());

        cfg->setSearchPath(R"(luts/$SHOT)");
        OCIO_CHECK_EQUAL(true, cfg->isArchivable());

        cfg->setSearchPath(R"(luts/$SHOT/luts1)");
        OCIO_CHECK_EQUAL(true, cfg->isArchivable());

        cfg->setSearchPath(R"(luts\$SHOT)");
        OCIO_CHECK_EQUAL(true, cfg->isArchivable());

        cfg->setSearchPath(R"(luts\$SHOT\luts1)");
        OCIO_CHECK_EQUAL(true, cfg->isArchivable());

        /*
         * Illegal scenarios
         */

        // Illegal search path starting with "..".
        cfg->setSearchPath(R"(luts:../luts)");
        OCIO_CHECK_EQUAL(false, cfg->isArchivable());

        cfg->setSearchPath(R"(luts:..\myLuts)");
        OCIO_CHECK_EQUAL(false, cfg->isArchivable());

        // Illegal search path starting with a context variable.
        cfg->setSearchPath(R"(luts:$SHOT)");
        OCIO_CHECK_EQUAL(false, cfg->isArchivable());

        // Illegal search path with absolute path.
        cfg->setSearchPath(R"(luts:/luts)");
        OCIO_CHECK_EQUAL(false, cfg->isArchivable());

        cfg->setSearchPath(R"(luts:/$SHOT)");
        OCIO_CHECK_EQUAL(false, cfg->isArchivable());

#ifdef _WIN32
        cfg->clearSearchPaths();
        cfg->addSearchPath(R"(C:\luts)");
        OCIO_CHECK_EQUAL(false, cfg->isArchivable());

        cfg->clearSearchPaths();
        cfg->addSearchPath(R"(C:\)");
        OCIO_CHECK_EQUAL(false, cfg->isArchivable());

        cfg->clearSearchPaths();
        cfg->addSearchPath(R"(C:\$SHOT)");
        OCIO_CHECK_EQUAL(false, cfg->isArchivable());
#endif
    }

    // Clear search paths so it doesn't affect the tests below.
    cfg->clearSearchPaths();

    // Lambda function to facilitate adding a new FileTransform to a config.
    auto addFTAndTestIsArchivable = [&cfg](const std::string & path, bool isArchivable)
    {
        std::string fullPath = pystring::os::path::join(path, "fake_lut.clf");
        auto ft = OCIO::FileTransform::Create();
        ft->setSrc(fullPath.c_str());

        auto cs = OCIO::ColorSpace::Create();
        cs->setName("csTest");
        cs->setTransform(ft, OCIO::COLORSPACE_DIR_TO_REFERENCE);
        cfg->addColorSpace(cs);

        OCIO_CHECK_EQUAL(isArchivable, cfg->isArchivable());

        cfg->removeColorSpace("csTest");
    };

    // Testing FileTransfrom paths
    {
        /*
         * Legal scenarios
         */

        // Valid FileTransform path.
        addFTAndTestIsArchivable("luts", true);
        addFTAndTestIsArchivable(R"(luts/myluts1)", true);
        addFTAndTestIsArchivable(R"(luts\myluts1)", true);

        // Valid Search path starting with "./" or ".\".
        addFTAndTestIsArchivable(R"(./myLuts)", true);
        addFTAndTestIsArchivable(R"(.\myLuts)", true);

        // Valid search path starting with "./" or ".\" and a context variable.
        addFTAndTestIsArchivable(R"(./$SHOT/myluts)", true);
        addFTAndTestIsArchivable(R"(.\$SHOT\myluts)", true);
        addFTAndTestIsArchivable(R"(luts/$SHOT)", true);
        addFTAndTestIsArchivable(R"(luts/$SHOT/luts1)", true);
        addFTAndTestIsArchivable(R"(luts\$SHOT)", true);
        addFTAndTestIsArchivable(R"(luts\$SHOT\luts1)", true);

        /*
         * Illegal scenarios
         */

        // Illegal search path starting with "..".
        addFTAndTestIsArchivable(R"(../luts)", false);
        addFTAndTestIsArchivable(R"(..\myLuts)", false);

        // Illegal search path starting with a context variable.
        addFTAndTestIsArchivable(R"($SHOT)", false);

        // Illegal search path with absolute path.
        addFTAndTestIsArchivable(R"(/luts)", false);
        addFTAndTestIsArchivable(R"(/$SHOT)", false);

#ifdef _WIN32
        addFTAndTestIsArchivable(R"(C:\luts)", false);
        addFTAndTestIsArchivable(R"(C:\)", false);
        addFTAndTestIsArchivable(R"(\$SHOT)", false);
#endif
    }
}

OCIO_ADD_TEST(OCIOZArchive, context_test_for_search_paths_and_filetransform_source_path)
{
    std::vector<std::string> pathsWindows = { 
        std::string(OCIO::GetTestFilesDir()),
        std::string("configs"),
        std::string("context_test1"),
        std::string("context_test1_windows.ocioz")
    };                                      
    static const std::string archivePathWindows = pystring::os::path::normpath(
        pystring::os::path::join(pathsWindows)
    );

    OCIO::ConfigRcPtr cfgWindowsArchive;
    OCIO_CHECK_NO_THROW(
        cfgWindowsArchive = OCIO::Config::CreateFromFile(
            archivePathWindows.c_str())->createEditableCopy());
    OCIO_CHECK_NO_THROW(cfgWindowsArchive->validate());

    std::vector<std::string> pathsLinux = { 
        std::string(OCIO::GetTestFilesDir()),
        std::string("configs"),
        std::string("context_test1"),
        std::string("context_test1_linux.ocioz")
    };                                      
    static const std::string archivePathLinux = pystring::os::path::normpath(
        pystring::os::path::join(pathsLinux)
    );

    OCIO::ConfigRcPtr cfgLinuxArchive;
    OCIO_CHECK_NO_THROW(
        cfgLinuxArchive = OCIO::Config::CreateFromFile(
            archivePathLinux.c_str())->createEditableCopy());
    OCIO_CHECK_NO_THROW(cfgLinuxArchive->validate());

    //  OCIO will pick up context vars from the environment that runs the test,
    //  so set these explicitly, even though the config has default values.

    OCIO::ContextRcPtr ctxWindowsArchive = cfgWindowsArchive->getCurrentContext()->createEditableCopy();
    ctxWindowsArchive->setStringVar("SHOT", "none");
    ctxWindowsArchive->setStringVar("LUT_PATH", "none");
    ctxWindowsArchive->setStringVar("CAMERA", "none");
    ctxWindowsArchive->setStringVar("CCCID", "none");

    OCIO::ContextRcPtr ctxLinuxArchive = cfgLinuxArchive->getCurrentContext()->createEditableCopy();
    ctxLinuxArchive->setStringVar("SHOT", "none");
    ctxLinuxArchive->setStringVar("LUT_PATH", "none");
    ctxLinuxArchive->setStringVar("CAMERA", "none");
    ctxLinuxArchive->setStringVar("CCCID", "none");

    double mat[16] = { 0., 0., 0., 0.,
                       0., 0., 0., 0.,
                       0., 0., 0., 0.,
                       0., 0., 0., 0. };

    auto testPaths = [&mat](const OCIO::ConfigRcPtr & cfg, const OCIO::ContextRcPtr ctx) 
    {
        {
            // This is independent of the context.
            OCIO::ConstProcessorRcPtr processor = cfg->getProcessor(ctx, "shot1_lut1_cs", "reference");
            OCIO::ConstTransformRcPtr tr = processor->createGroupTransform()->getTransform(0);
            auto mtx = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
            OCIO_REQUIRE_ASSERT(mtx);
            mtx->getMatrix(mat);
            OCIO_CHECK_EQUAL(mat[0], 10.);
        }

        {
            // This is independent of the context.
            OCIO::ConstProcessorRcPtr processor = cfg->getProcessor(ctx, "shot2_lut1_cs", "reference");
            OCIO::ConstTransformRcPtr tr = processor->createGroupTransform()->getTransform(0);
            auto mtx = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
            OCIO_REQUIRE_ASSERT(mtx);
            mtx->getMatrix(mat);
            OCIO_CHECK_EQUAL(mat[0], 20.);
        }

        {
            // This is independent of the context.
            OCIO::ConstProcessorRcPtr processor = cfg->getProcessor(ctx, "shot2_lut2_cs", "reference");
            OCIO::ConstTransformRcPtr tr = processor->createGroupTransform()->getTransform(0);
            auto mtx = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
            OCIO_REQUIRE_ASSERT(mtx);
            mtx->getMatrix(mat);
            OCIO_CHECK_EQUAL(mat[0], 2.);
        }

        {
            // This is independent of the context but the file is in a second level sub-directory.
            OCIO::ConstProcessorRcPtr processor = cfg->getProcessor(ctx, "lut3_cs", "reference");
            OCIO::ConstTransformRcPtr tr = processor->createGroupTransform()->getTransform(0);
            auto mtx = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
            OCIO_REQUIRE_ASSERT(mtx);
            mtx->getMatrix(mat);
            OCIO_CHECK_EQUAL(mat[0], 3.);
        }

        {
            // This uses a context for the FileTransform src but is independent of the search_path.
            ctx->setStringVar("LUT_PATH", "shot3/lut1.clf");
            OCIO::ConstProcessorRcPtr processor = cfg->getProcessor(ctx, "lut_path_cs", "reference");
            OCIO::ConstTransformRcPtr tr = processor->createGroupTransform()->getTransform(0);
            auto mtx = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
            OCIO_REQUIRE_ASSERT(mtx);
            mtx->getMatrix(mat);
            OCIO_CHECK_EQUAL(mat[0], 30.);
        }

        {
            // The FileTransform src is ambiguous and the context configures the search_path.
            ctx->setStringVar("SHOT", ".");     // (use the working directory)
            OCIO::ConstProcessorRcPtr processor = cfg->getProcessor(ctx, "plain_lut1_cs", "reference");
            OCIO::ConstTransformRcPtr tr = processor->createGroupTransform()->getTransform(0);
            auto mtx = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
            OCIO_REQUIRE_ASSERT(mtx);
            mtx->getMatrix(mat);
            OCIO_CHECK_EQUAL(mat[0], 5.);
        }

        {
            // The FileTransform src is ambiguous and the context configures the search_path.
            ctx->setStringVar("SHOT", "shot2");
            OCIO::ConstProcessorRcPtr processor = cfg->getProcessor(ctx, "plain_lut1_cs", "reference");
            OCIO::ConstTransformRcPtr tr = processor->createGroupTransform()->getTransform(0);
            auto mtx = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
            OCIO_REQUIRE_ASSERT(mtx);
            mtx->getMatrix(mat);
            OCIO_CHECK_EQUAL(mat[0], 20.);
        }

        {
            // The FileTransform src is ambiguous and the context configures the search_path.
            ctx->setStringVar("SHOT", "no_shot");   // (path doesn't exist)
            OCIO::ConstProcessorRcPtr processor = cfg->getProcessor(ctx, "plain_lut1_cs", "reference");
            OCIO::ConstTransformRcPtr tr = processor->createGroupTransform()->getTransform(0);
            auto mtx = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
            OCIO_REQUIRE_ASSERT(mtx);
            mtx->getMatrix(mat);
            OCIO_CHECK_EQUAL(mat[0], 10.);
        }

        {
            // This should be in the archive but is not on the search path at all without the context.
            ctx->setStringVar("SHOT", "no_shot");   // (path doesn't exist)
            OCIO::ConstProcessorRcPtr processor;
            OCIO_CHECK_THROW(
                processor = cfg->getProcessor(ctx, "lut4_cs", "reference"), 
                OCIO::ExceptionMissingFile
            )
        }

        {
            // This should be in the archive but is not on the search path at all without the context.
            ctx->setStringVar("SHOT", "shot4");
            OCIO::ConstProcessorRcPtr processor = cfg->getProcessor(ctx, "lut4_cs", "reference");
            OCIO::ConstTransformRcPtr tr = processor->createGroupTransform()->getTransform(0);
            auto mtx = OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr);
            OCIO_REQUIRE_ASSERT(mtx);
            mtx->getMatrix(mat);
            OCIO_CHECK_EQUAL(mat[0], 4.);
        }
    };

    testPaths(cfgWindowsArchive, ctxWindowsArchive);
    testPaths(cfgLinuxArchive, ctxLinuxArchive);
}

OCIO_ADD_TEST(OCIOZArchive, archive_config_and_compare_to_original)
{
    /**
     * This test is doing the following :
     * 
     * 1 - Create a config object from tests/data/files/configs/context_test1/config.ocio.
     * 2 - Archive the config in step 1 and save it in a temporary folder.
     * 3 - Create a config object using the archived config from step 2.
     * 4 - Compare different elements between the two configs.
     * 
     * Testing CreateFromFile and archive method on a successful path.
     */

    std::vector<std::string> paths = { 
        std::string(OCIO::GetTestFilesDir()),
        std::string("configs"),
        std::string("context_test1"),
        std::string("config.ocio"),
    };                    
    const std::string configPath = pystring::os::path::normpath(
        pystring::os::path::join(paths)
    );

    {
        OCIO::EnvironmentVariableGuard guard("OCIO", configPath);

        // 1 - Create a config from a OCIO file.
        OCIO::ConstConfigRcPtr configFromOcioFile;
        OCIO_CHECK_NO_THROW(configFromOcioFile = OCIO::Config::CreateFromEnv());
        OCIO_REQUIRE_ASSERT(configFromOcioFile);
        OCIO_CHECK_NO_THROW(configFromOcioFile->validate());

        // 2 - Archive the config of step 1.
        std::ostringstream ostringStream;
        OCIO_CHECK_NO_THROW(configFromOcioFile->archive(ostringStream));

        // 3 - Verify that the binary data starts with "PK".
        OCIO_CHECK_EQUAL('P', ostringStream.str()[0]);
        OCIO_CHECK_EQUAL('K', ostringStream.str()[1]);

        // 4 - Save the archive into a temporary file.
        FileCreationGuard fGuard(__LINE__);
        std::ofstream stream(fGuard.m_filename, std::ios_base::out | std::ios_base::binary);
        OCIO_REQUIRE_ASSERT(stream.is_open());
        stream << ostringStream.str();
        stream.close();

        // 5 - Create a config from the archived config of step 4.
        OCIO::ConstConfigRcPtr configFromArchive;
        OCIO_CHECK_NO_THROW(configFromArchive = OCIO::Config::CreateFromFile(
            fGuard.m_filename.c_str()
        ));
        OCIO_REQUIRE_ASSERT(configFromArchive);
        OCIO_CHECK_NO_THROW(configFromArchive->validate());

        // 6 - Compare config cacheID - configFromOcioFile vs configFromArchive.
        OCIO::ConstContextRcPtr context;
        OCIO_CHECK_EQUAL(
            std::string(configFromOcioFile->getCacheID(context)),
            std::string(configFromArchive->getCacheID(context))
        );

        // 7 - Compare a processor cacheID - configFromOcioFile to configFromArchive.
        OCIO::ConstProcessorRcPtr procConfigFromOcioFile, procConfigFromArchive;
        OCIO_CHECK_NO_THROW(procConfigFromOcioFile = configFromOcioFile->getProcessor(
            "plain_lut1_cs", 
            "shot1_lut1_cs"
        ));
        OCIO_REQUIRE_ASSERT(procConfigFromOcioFile);

        OCIO_CHECK_NO_THROW(procConfigFromArchive = configFromArchive->getProcessor(
            "plain_lut1_cs", 
            "shot1_lut1_cs"
        ));
        OCIO_REQUIRE_ASSERT(procConfigFromArchive);

        OCIO_CHECK_EQUAL(
            std::string(procConfigFromOcioFile->getCacheID()), 
            std::string(procConfigFromArchive->getCacheID())
        );

        // 8 - Compare serialization - configFromOcioFile vs configFromArchive.
        std::ostringstream streamToConfigFromOcioFile, streamToConfigFromArchive;
        configFromOcioFile->serialize(streamToConfigFromOcioFile);
        configFromArchive->serialize(streamToConfigFromArchive);
        OCIO_CHECK_EQUAL(
            streamToConfigFromOcioFile.str(), 
            streamToConfigFromArchive.str()
        );
    }
}

OCIO_ADD_TEST(OCIOZArchive, extract_config_and_compare_to_original)
{
    /**
     * This test is doing the following :
     * 
     * 1 - Create a config object from context_test1_windows.ocioz.
     * 2 - Extract the config context_test1_windows.ocioz.
     * 3 - Create a config object using the extracted config in step 1.
     * 4 - Compare different elements between the two configs.
     * 
     * Testing CreateFromFile and ExtractOCIOZArchive on a successful path.
     */

    std::vector<std::string> paths = { 
        std::string(OCIO::GetTestFilesDir()),
        std::string("configs"),
        std::string("context_test1"),
        std::string("context_test1_windows.ocioz"),
    };                    
    const std::string archivePath = pystring::os::path::normpath(
        pystring::os::path::join(paths)
    );

    {
        // 1 - Create config from OCIOZ archive.
        OCIO::ConstConfigRcPtr configFromArchive;
        OCIO_CHECK_NO_THROW(configFromArchive = OCIO::Config::CreateFromFile(
            archivePath.c_str()
        ));
        OCIO_REQUIRE_ASSERT(configFromArchive);
        OCIO_CHECK_NO_THROW(configFromArchive->validate());

        // 2 - Extract the OCIOZ archive to temporary directory.
        DirectoryCreationGuard dGuard("context_test1", __LINE__);
        OCIO_CHECK_NO_THROW(OCIO::ExtractOCIOZArchive(
            archivePath.c_str(), dGuard.m_directoryPath.c_str()
        ));

        // 3 - Create config from extracted OCIOZ archive.
        OCIO::ConstConfigRcPtr configFromExtractedArchive;
        OCIO_CHECK_NO_THROW(configFromExtractedArchive = OCIO::Config::CreateFromFile(
            pystring::os::path::join(dGuard.m_directoryPath, "config.ocio").c_str()
        ));
        OCIO_REQUIRE_ASSERT(configFromExtractedArchive);
        OCIO_CHECK_NO_THROW(configFromExtractedArchive->validate());

        // 4 - Compare config cacheID - configFromArchive vs configFromExtractedArchive.
        OCIO::ConstContextRcPtr context;
        OCIO_CHECK_EQUAL(
            std::string(configFromArchive->getCacheID(context)),
            std::string(configFromExtractedArchive->getCacheID(context))
        );

        // 5 - Compare a processor cacheID - configFromArchive to configFromExtractedArchive.
        OCIO::ConstProcessorRcPtr procConfigFromArchive, procConfigFromExtractedArchive;
        OCIO_CHECK_NO_THROW(procConfigFromArchive = configFromArchive->getProcessor(
            "plain_lut1_cs", 
            "shot1_lut1_cs"
        ));
        OCIO_REQUIRE_ASSERT(procConfigFromArchive);

        OCIO_CHECK_NO_THROW(procConfigFromExtractedArchive = configFromExtractedArchive->getProcessor(
            "plain_lut1_cs", 
            "shot1_lut1_cs"
        ));
        OCIO_REQUIRE_ASSERT(procConfigFromExtractedArchive);

        OCIO_CHECK_EQUAL(
            std::string(procConfigFromArchive->getCacheID()), 
            std::string(procConfigFromExtractedArchive->getCacheID())
        );

        // 6 - Compare serialization - configFromArchive vs configFromExtractedArchive.
        std::ostringstream streamToConfigFromArchive, streamToConfigFromExtractedArchive;
        configFromArchive->serialize(streamToConfigFromArchive);
        configFromExtractedArchive->serialize(streamToConfigFromExtractedArchive);
        OCIO_CHECK_EQUAL(
            streamToConfigFromArchive.str(), 
            streamToConfigFromExtractedArchive.str()
        );
    }
}