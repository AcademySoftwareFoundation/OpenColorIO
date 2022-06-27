// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "builtinconfigs/BuiltinConfigRegistry.cpp"
#include "testutils/UnitTest.h"

#include "CG.cpp"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(BuiltinConfigs, basic)
{
    const OCIO::BuiltinConfigRegistry & registry = OCIO::BuiltinConfigRegistry::Get();
    
    OCIO_CHECK_EQUAL(registry.getNumBuiltinConfigs(), 1);

    OCIO_CHECK_EQUAL(
        std::string(registry.getBuiltinConfigName(0)), 
        std::string("cg-config-v0.1.0_aces-v1.3_ocio-v2.1.1")
    );

    OCIO_CHECK_EQUAL(
        std::string(registry.getBuiltinConfigUIName(0)), 
        std::string("Academy Color Encoding System - CG Config [COLORSPACES v0.1.0] [ACES v1.3] [OCIO v2.1.1]")
    );

    OCIO_CHECK_EQUAL(
        std::string(registry.getBuiltinConfig(0)), 
        std::string(CG_CONFIG_V010_ACES_V130_OCIO_V211)
    );

    OCIO_CHECK_EQUAL(
        std::string(registry.getBuiltinConfigByName("cg-config-v0.1.0_aces-v1.3_ocio-v2.1.1")), 
        std::string(CG_CONFIG_V010_ACES_V130_OCIO_V211)
    );

    OCIO_CHECK_EQUAL(
        std::string(registry.getDefaultBuiltinConfigName()), 
        std::string("cg-config-v0.1.0_aces-v1.3_ocio-v2.1.1")
        );

    OCIO_CHECK_EQUAL(registry.isBuiltinConfigRecommended(0), true);

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
            "My simple config name #1",
            "My simple config display name #1",
            SIMPLE_CONFIG.c_str(),
            false
        ));
        // Add second config.
        OCIO_CHECK_NO_THROW(registry.addBuiltin(
            "My simple config name #2",
            "My simple config display name #2",
            SIMPLE_CONFIG.c_str(),
            true
        ));

        OCIO_CHECK_EQUAL(registry.getNumBuiltinConfigs(), 2);

        // Tests to check if the config #1 was added correctly.
        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigName(0)), 
            std::string("My simple config name #1")
        );
        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigUIName(0)), 
            std::string("My simple config display name #1")
        );

        // Tests to check if the config #2 was added correctly.
        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigName(1)), 
            std::string("My simple config name #2")
        );
        OCIO_CHECK_EQUAL(
            std::string(registry.getBuiltinConfigUIName(1)), 
            std::string("My simple config display name #2")
        );
    }
}