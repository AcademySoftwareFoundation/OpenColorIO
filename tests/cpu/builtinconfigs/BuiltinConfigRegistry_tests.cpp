#include "CG.h"
#include "builtinconfigs/BuiltinConfigRegistry.cpp"
#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(BuiltinsConfig, basic)
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
    "      name: raw\n"
    "strictparsing: false\n"
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
        false,
        false
    ));
    // Add second config.
    OCIO_CHECK_NO_THROW(registry.addBuiltin(
        "My simple config name #2", 
        SIMPLE_CONFIG.c_str(),
        false, // set second config as default
        true
    ));

    // Test the number of configs
    OCIO_CHECK_EQUAL(registry.getNumConfigs(), 2);

    // Test if second config is recommended.
    OCIO_CHECK_EQUAL(registry.isConfigRecommended(1), true);
    // Test if second config is deprecated.
    OCIO_CHECK_EQUAL(registry.isConfigDepecrated(1), false);
    
    // Test the config name of the second one.
    OCIO_CHECK_EQUAL(std::string(registry.getConfigName(1)), std::string("My simple config name #2"));
    
    // Test if the config saved is correct using getConfig.
    OCIO_CHECK_EQUAL(std::string(registry.getConfig(1)), SIMPLE_CONFIG);
    
    // Test if the config saved is correct using getConfigByName.
    OCIO_CHECK_EQUAL(std::string(registry.getConfigByName("My simple config name #2")), SIMPLE_CONFIG);

    // Test the default built-in config name.
    OCIO_CHECK_EQUAL(std::string(registry.getDefaultConfigName()), std::string("My simple config name #2"));
    

    // ********************************
    // Testing some expected failures.
    // ********************************

    // Test isConfigRecommended using an invalid config index.
    OCIO_CHECK_THROW_WHAT(
        registry.isConfigRecommended(999),
        OCIO::Exception,
        "Config index is out of range."
    );

    // Test isConfigDepecrated using an invalid config index.
    OCIO_CHECK_THROW_WHAT(
        registry.isConfigDepecrated(999),
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
            false,
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