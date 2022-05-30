// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "CG.h"
#include "builtinconfigs/BuiltinConfigRegistry.cpp"
#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

// TODO CED: OCIO_ADD_TEST for public registry class
OCIO_ADD_TEST(BuiltinConfigs, basic)
{
    OCIO::ConstBuiltinConfigRegistryRcPtr registry = OCIO::BuiltinConfigRegistry::Get();
    
    OCIO_CHECK_EQUAL(registry->getNumConfigs(), 1);

    OCIO_CHECK_EQUAL(std::string(registry->getConfigName(0)), std::string("cg-config-v0.1.0_aces-v1.3_ocio-v2.1.1"));

    OCIO_CHECK_EQUAL(std::string(registry->getConfig(0)), std::string(CG_CONFIG_V010_ACES_V130_OCIO_V211));

    OCIO_CHECK_EQUAL(std::string(registry->getConfigByName("cg-config-v0.1.0_aces-v1.3_ocio-v2.1.1")), std::string(CG_CONFIG_V010_ACES_V130_OCIO_V211));

    OCIO_CHECK_EQUAL(std::string(registry->getDefaultConfigName()), std::string("cg-config-v0.1.0_aces-v1.3_ocio-v2.1.1"));

    OCIO_CHECK_EQUAL(registry->isConfigRecommended(0), true);
}

OCIO_ADD_TEST(BuiltinConfigs, basic_impl)
{
    // Create a empty registry and check the number of configs
    OCIO::BuiltinConfigRegistryImpl registry;
    OCIO_CHECK_EQUAL(registry.getNumConfigs(), 0);

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
        "My simple config name #1", 
        SIMPLE_CONFIG.c_str(),
        false
    ));
    // Add second config.
    OCIO_CHECK_NO_THROW(registry.addBuiltin(
        "My simple config name #2", 
        SIMPLE_CONFIG.c_str(),
        true
    ));

    registry.setDefaultBuiltinConfig("My simple config name #2");

    // ********************************
    // Testing some expected failures.
    // ********************************

    // Test isConfigRecommended using an invalid config index.
    OCIO_CHECK_THROW_WHAT(
        registry.isConfigRecommended(999),
        OCIO::Exception,
        "Config index is out of range."
    );

    // Test getConfigName using an invalid config index.
    OCIO_CHECK_THROW_WHAT(
        registry.getConfigName(999),
        OCIO::Exception,
        "Config index is out of range."
    );

    // Test getConfig using an invalid config index.
    OCIO_CHECK_THROW_WHAT(
        registry.getConfig(999),
        OCIO::Exception,
        "Config index is out of range."
    );

    // Test getConfigByName using an unknown config name.
    OCIO_CHECK_EQUAL(registry.getConfigByName("I do not exist"), "");

    {
        // Create a empty registry.
        OCIO::BuiltinConfigRegistryImpl registry;

        // Test the default built-in config name without any built-in config.
        OCIO_CHECK_THROW_WHAT(
            registry.getDefaultConfigName(),
            OCIO::Exception,
            "Internal error - There is no default built-ins config."
        );
    }

    {
        // Create a empty registry.
        OCIO::BuiltinConfigRegistryImpl registry;

        // Test the default built-in config name without any built-in config.
        OCIO_CHECK_THROW_WHAT(
            registry.getDefaultConfigName(),
            OCIO::Exception,
            "Internal error - There is no default built-ins config."
        );

        // Add configs into the built-ins config registry
        std::string SIMPLE_CONFIG =
        "ocio_profile_version: 1\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "      name: raw\n"
        "  - !<ColorSpace>\n"
        "      name: raw\n"
        "strictparsing: false\n"
        "roles:\n"
        "  default: raw\n"
        "displays:\n"
        "  sRGB:\n"
        "  - !<View> {name: Raw, colorspace: raw}\n"
        "\n";
        
        // Add first config without specifying that it is the default.
        OCIO_CHECK_NO_THROW(registry.addBuiltin(
            "My simple config name #1", 
            SIMPLE_CONFIG.c_str(),
            false
        ));

        // Test the default built-in config name without any built-in config.
        OCIO_CHECK_THROW_WHAT(
            registry.getDefaultConfigName(),
            OCIO::Exception,
            "Internal error - There is no default built-ins config."
        );
    }
}