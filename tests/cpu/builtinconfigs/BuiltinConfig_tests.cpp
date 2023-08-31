// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "builtinconfigs/BuiltinConfigRegistry.cpp"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"
#include "UnitTestLogUtils.h"

#include "CG.cpp"
#include "Studio.cpp"

namespace OCIO = OCIO_NAMESPACE;

// See also the create_builtin_config and resolve_config_path tests in Config_tests.cpp.

OCIO_ADD_TEST(BuiltinConfigs, basic)
{
    const OCIO::BuiltinConfigRegistry & registry = OCIO::BuiltinConfigRegistry::Get();
    
    OCIO_CHECK_EQUAL(registry.getNumBuiltinConfigs(), 4);

    // Test builtin config cg-config-v1.0.0_aces-v1.3_ocio-v2.1.
    {
        const std::string cgConfigName = "cg-config-v1.0.0_aces-v1.3_ocio-v2.1";

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigName(0)), 
            cgConfigName
        );

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigUIName(0)), 
            std::string("Academy Color Encoding System - CG Config [COLORSPACES v1.0.0] "\
            "[ACES v1.3] [OCIO v2.1]")
        );

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfig(0)), 
            std::string(CG_CONFIG_V100_ACES_V13_OCIO_V21)
        );

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigByName(cgConfigName.c_str())), 
            std::string(CG_CONFIG_V100_ACES_V13_OCIO_V21)
        );

        OCIO_CHECK_EQUAL(registry.isBuiltinConfigRecommended(0), false);
    }

    // Test builtin config cg-config-v2.1.0_aces-v1.3_ocio-v2.3.
    {
        const std::string cgConfigName = "cg-config-v2.1.0_aces-v1.3_ocio-v2.3";

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigName(1)), 
            cgConfigName
        );

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigUIName(1)), 
            std::string("Academy Color Encoding System - CG Config [COLORSPACES v2.0.0] "\
            "[ACES v1.3] [OCIO v2.3]")
        );

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfig(1)), 
            std::string(CG_CONFIG_V210_ACES_V13_OCIO_V23)
        );

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigByName(cgConfigName.c_str())), 
            std::string(CG_CONFIG_V210_ACES_V13_OCIO_V23)
        );

        OCIO_CHECK_EQUAL(registry.isBuiltinConfigRecommended(1), true);
    }

    // Test builtin config studio-config-v1.0.0_aces-v1.3_ocio-v2.1.
    {
        const std::string studioConfigName = "studio-config-v1.0.0_aces-v1.3_ocio-v2.1";
        
        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigName(2)), 
            studioConfigName
        );

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigUIName(2)), 
            std::string("Academy Color Encoding System - Studio Config [COLORSPACES v1.0.0] "\
            "[ACES v1.3] [OCIO v2.1]")
        );

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfig(2)), 
            std::string(STUDIO_CONFIG_V100_ACES_V13_OCIO_V21)
        );

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigByName(studioConfigName.c_str())), 
            std::string(STUDIO_CONFIG_V100_ACES_V13_OCIO_V21)
        );

        OCIO_CHECK_EQUAL(registry.isBuiltinConfigRecommended(2), false);
    }

    // Test builtin config studio-config-v2.1.0_aces-v1.3_ocio-v2.3.
    {
        const std::string studioConfigName = "studio-config-v2.1.0_aces-v1.3_ocio-v2.3";
        
        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigName(3)), 
            studioConfigName
        );

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigUIName(3)), 
            std::string("Academy Color Encoding System - Studio Config [COLORSPACES v2.0.0] "\
            "[ACES v1.3] [OCIO v2.3]")
        );

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfig(3)), 
            std::string(STUDIO_CONFIG_V210_ACES_V13_OCIO_V23)
        );

        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigByName(studioConfigName.c_str())), 
            std::string(STUDIO_CONFIG_V210_ACES_V13_OCIO_V23)
        );

        OCIO_CHECK_EQUAL(registry.isBuiltinConfigRecommended(3), true);
    }

    // ********************************
    // Testing some expected failures.
    // ********************************

    // Test isBuiltinConfigRecommended using an invalid config index.
    OCIO_CHECK_THROW_WHAT(
        registry.isBuiltinConfigRecommended(999),
        OCIO::Exception,
        "Config index is out of range."
    );

    // Test getBuiltinConfigName using an invalid config index.
    OCIO_CHECK_THROW_WHAT(
        registry.getBuiltinConfigName(999),
        OCIO::Exception,
        "Config index is out of range."
    );

    // Test getBuiltinConfigUIName using an invalid config index.
    OCIO_CHECK_THROW_WHAT(
        registry.getBuiltinConfigUIName(999),
        OCIO::Exception,
        "Config index is out of range."
    );

    // Test getBuiltinConfig using an invalid config index.
    OCIO_CHECK_THROW_WHAT(
        registry.getBuiltinConfig(999),
        OCIO::Exception,
        "Config index is out of range."
    );

    // Test getBuiltinConfigByName using an unknown config name.
    OCIO_CHECK_THROW_WHAT(
        registry.getBuiltinConfigByName("I do not exist"), 
        OCIO::Exception,
        "Could not find 'I do not exist' in the built-in configurations."
    );
}

OCIO_ADD_TEST(BuiltinConfigs, basic_impl)
{
    {
        // Test the addBuiltin method.

        OCIO::BuiltinConfigRegistryImpl registry;

        // Add configs into the built-ins config registry
        std::string SIMPLE_CONFIG =
        "ocio_profile_version: 1\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "      name: raw\n"
        "  - !<ColorSpace>\n"
        "      name: linear\n"
        "roles:\n"
        "  default: raw\n"
        "displays:\n"
        "  sRGB:\n"
        "  - !<View> {name: Raw, colorspace: raw}\n"
        "\n";
    
        // Add first config.
        OCIO_CHECK_NO_THROW(registry.addBuiltin(
            "simple_config_1",
            "My simple config display name #1",
            SIMPLE_CONFIG.c_str(),
            false
        ));
        // Add second config.
        OCIO_CHECK_NO_THROW(registry.addBuiltin(
            "simple_config_2",
            "My simple config display name #2",
            SIMPLE_CONFIG.c_str(),
            true
        ));

        OCIO_CHECK_EQUAL(registry.getNumBuiltinConfigs(), 2);

        // Tests to check if the config #1 was added correctly.
        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigName(0)), 
            std::string("simple_config_1")
        );
        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigUIName(0)), 
            std::string("My simple config display name #1")
        );

        // Tests to check if the config #2 was added correctly.
        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigName(1)), 
            std::string("simple_config_2")
        );
        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigUIName(1)), 
            std::string("My simple config display name #2")
        );
    }
}

OCIO_ADD_TEST(BuiltinConfigs, create_builtin_config)
{
    auto testFromBuiltinConfig = [](const std::string name,
                                    int numberOfExpectedColorspaces,
                                    const std::string expectedConfigName,
                                    int line)
    {
        {
            // Testing CreateFromBuiltinConfig with a known built-in config name.

            OCIO::ConstConfigRcPtr config;
            OCIO_CHECK_NO_THROW(
                config = OCIO::Config::CreateFromBuiltinConfig(name.c_str())
            );
            OCIO_REQUIRE_ASSERT(config);

            OCIO::LogGuard logGuard;
            OCIO_CHECK_NO_THROW(config->validate());
            // Mute output related to a bug in the initial CG config where the inactive_colorspaces 
            // list has color spaces that don't exist.
            OCIO::muteInactiveColorspaceInfo(logGuard);
            logGuard.print();

            OCIO_CHECK_EQUAL_FROM(
                std::string(config->getName()), 
                expectedConfigName.empty() ? name : expectedConfigName,
                line
            );
            OCIO_CHECK_EQUAL_FROM(config->getNumColorSpaces(), numberOfExpectedColorspaces, line);
        }
    };

    auto testFromEnvAndFromFile = [](const std::string uri,
                                     int numberOfExpectedColorspaces,
                                     const std::string expectedConfigName,
                                     int line) 
    {
        {
            // Testing CreateFromEnv using URI Syntax. 

            OCIO::EnvironmentVariableGuard guard("OCIO", uri);

            OCIO::ConstConfigRcPtr config;
            OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromEnv());
            OCIO_REQUIRE_ASSERT(config);

            OCIO::LogGuard logGuard;
            OCIO_CHECK_NO_THROW(config->validate());
            OCIO::muteInactiveColorspaceInfo(logGuard);
            logGuard.print();

            if (!expectedConfigName.empty())
            {
                OCIO_CHECK_EQUAL_FROM(
                    std::string(config->getName()), 
                    expectedConfigName,
                    line
                );
            }
            OCIO_CHECK_EQUAL_FROM(config->getNumColorSpaces(), numberOfExpectedColorspaces, line);
        }

        {
            // Testing CreateFromFile using URI Syntax.

            OCIO::ConstConfigRcPtr config;
            OCIO_CHECK_NO_THROW(
                config = OCIO::Config::CreateFromFile(uri.c_str())
            );
            OCIO_REQUIRE_ASSERT(config);

            OCIO::LogGuard logGuard;
            OCIO_CHECK_NO_THROW(config->validate());
            OCIO::muteInactiveColorspaceInfo(logGuard);

            if (!expectedConfigName.empty())
            {
                OCIO_CHECK_EQUAL_FROM(
                    std::string(config->getName()), 
                    expectedConfigName,
                    line
                );
            }
            OCIO_CHECK_EQUAL_FROM(config->getNumColorSpaces(), numberOfExpectedColorspaces, line);
        }
    };

    const std::string uriPrefix = OCIO::OCIO_BUILTIN_URI_PREFIX;
    const std::string defaultName = BUILTIN_DEFAULT_NAME;
    const std::string latestCGName = BUILTIN_LATEST_CG_NAME;
    const std::string latestStudioName = BUILTIN_LATEST_STUDIO_NAME;

    // Test that CreateFromFile does not work without ocio:// prefix for built-in config.
    OCIO_CHECK_THROW_WHAT(
        OCIO::Config::CreateFromFile("cg-config-v1.0.0_aces-v1.3_ocio-v2.1"),
        OCIO::Exception,
        "Error could not read 'cg-config-v1.0.0_aces-v1.3_ocio-v2.1' OCIO profile."
    );    

    {
        const std::string cgConfigName = "cg-config-v1.0.0_aces-v1.3_ocio-v2.1";
        const std::string studioConfigName = "studio-config-v1.0.0_aces-v1.3_ocio-v2.1";
        // Test CG builtin config #1
        int nbOfColorspacesForCGConfig1 = 14;
        testFromBuiltinConfig(cgConfigName, nbOfColorspacesForCGConfig1, "", __LINE__);
        testFromEnvAndFromFile(uriPrefix + cgConfigName, nbOfColorspacesForCGConfig1, cgConfigName, __LINE__);

        // Test STUDIO builtin config #1
        int nbOfColorspacesForStudioConfig1 = 39;
        testFromBuiltinConfig(studioConfigName, nbOfColorspacesForStudioConfig1, "", __LINE__);
        testFromEnvAndFromFile(uriPrefix + studioConfigName, nbOfColorspacesForStudioConfig1, studioConfigName, __LINE__);
    }

    {
        // Test default config.
        int nbOfColorspacesForDefaultCGConfig = 15;
        int nbOfColorspacesForDefaultStudioConfig = 41;
        std::string expectedCGName = "cg-config-v2.1.0_aces-v1.3_ocio-v2.3";
        std::string expectedStudioName = "studio-config-v2.1.0_aces-v1.3_ocio-v2.3";

        testFromBuiltinConfig(defaultName, nbOfColorspacesForDefaultCGConfig, expectedCGName, __LINE__);
        testFromBuiltinConfig(uriPrefix + defaultName, nbOfColorspacesForDefaultCGConfig, expectedCGName, __LINE__);
        testFromEnvAndFromFile(uriPrefix + defaultName, nbOfColorspacesForDefaultCGConfig, expectedCGName, __LINE__);

        // Test cg-config-latest.
        testFromBuiltinConfig(latestCGName, nbOfColorspacesForDefaultCGConfig, expectedCGName, __LINE__);
        testFromBuiltinConfig(uriPrefix + latestCGName, nbOfColorspacesForDefaultCGConfig, expectedCGName, __LINE__);
        testFromEnvAndFromFile(uriPrefix + latestCGName, nbOfColorspacesForDefaultCGConfig, expectedCGName, __LINE__);

        // Test studio-config-latest.
        testFromBuiltinConfig(latestStudioName, nbOfColorspacesForDefaultStudioConfig, expectedStudioName, __LINE__);
        testFromBuiltinConfig(uriPrefix + latestStudioName, nbOfColorspacesForDefaultStudioConfig, expectedStudioName, __LINE__);
        testFromEnvAndFromFile(uriPrefix + latestStudioName, nbOfColorspacesForDefaultStudioConfig, expectedStudioName, __LINE__);
    }


    // ********************************
    // Test some expected failures.
    // ********************************

    // Test CreateFromBuiltinConfig with an unknown built-in config name.
    OCIO_CHECK_THROW_WHAT(
        OCIO::Config::CreateFromBuiltinConfig("I-do-not-exist"),
        OCIO::Exception,
        "Could not find 'I-do-not-exist' in the built-in configurations."
    );

    // Test CreateFromFile with an unknown built-in config name using URI syntax.
    OCIO_CHECK_THROW_WHAT(
        OCIO::Config::CreateFromFile("ocio://I-do-not-exist"),
        OCIO::Exception,
        "Could not find 'I-do-not-exist' in the built-in configurations."
    );

    {
        // Testing CreateFromEnv with an unknown built-in config.

        OCIO::EnvironmentVariableGuard guard("OCIO", "ocio://thedefault");

        OCIO_CHECK_THROW_WHAT(
            OCIO::Config::CreateFromEnv(),
            OCIO::Exception,
            "Could not find 'thedefault' in the built-in configurations."
        );
    }
}

OCIO_ADD_TEST(BuiltinConfigs, resolve_config_path)
{
    OCIO_CHECK_EQUAL(
        OCIO::ResolveConfigPath("ocio://default"), 
        std::string(DEFAULT_BUILTIN_CONFIG_URI)
    );

    OCIO_CHECK_EQUAL(
        OCIO::ResolveConfigPath("ocio://cg-config-latest"), 
        std::string(LATEST_CG_BUILTIN_CONFIG_URI)
    );

    OCIO_CHECK_EQUAL(
        OCIO::ResolveConfigPath("ocio://studio-config-latest"), 
        std::string(LATEST_STUDIO_BUILTIN_CONFIG_URI)
    ); 


    // ******************************************************************************
    // Paths that are not starting with "ocio://" are simply returned unmodified.
    // ******************************************************************************

    OCIO_CHECK_EQUAL(
        OCIO::ResolveConfigPath("studio-config-latest"), 
        std::string("studio-config-latest")
    ); 

    OCIO_CHECK_EQUAL(
        OCIO::ResolveConfigPath("studio-config-latest.ocio"), 
        std::string("studio-config-latest.ocio")
    );

    OCIO_CHECK_EQUAL(
        OCIO::ResolveConfigPath("/usr/local/share/aces.ocio"), 
        std::string("/usr/local/share/aces.ocio")
    );

    OCIO_CHECK_EQUAL(
        OCIO::ResolveConfigPath("C:\\myconfig\\config.ocio"), 
        std::string("C:\\myconfig\\config.ocio")
    );

    OCIO_CHECK_EQUAL(
        OCIO::ResolveConfigPath(""), 
        std::string("")
    );

    // *****************************************************            
    // The function does not try to validate to catch 
    // mistakes in URI usage. That's up to the application.
    // *****************************************************

    // Unknown built-in config.
    OCIO_CHECK_EQUAL(
        OCIO::ResolveConfigPath("ocio://not-a-builtin"), 
        std::string("ocio://not-a-builtin")
    );
    
    // Missing "//".
    OCIO_CHECK_EQUAL(
        OCIO::ResolveConfigPath("ocio:default"), 
        std::string("ocio:default")
    );  
}