// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <pystring.h>

#include "apphelpers/mergeconfigs/MergeConfigsHelpers.cpp"

#include "UnitTestUtils.h"

#include "ConfigUtils.h"
#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"

namespace OCIO = OCIO_NAMESPACE;

using MergeStrategy = OCIO::ConfigMergingParameters::MergeStrategies;

OCIO::ConstConfigRcPtr getBaseConfig()
{
    std::vector<std::string> basePaths = { 
        std::string(OCIO::GetTestFilesDir()),
        std::string("configs"),
        std::string("mergeconfigs"),
        std::string("base_config.yaml")
    }; 
    const std::string baseCfgPath = pystring::os::path::normpath(pystring::os::path::join(basePaths));
    return OCIO::Config::CreateFromFile(baseCfgPath.c_str());
}

OCIO::ConstConfigRcPtr getInputConfig()
{
    std::vector<std::string> inputPaths = { 
        std::string(OCIO::GetTestFilesDir()),
        std::string("configs"),
        std::string("mergeconfigs"),
        std::string("input_config.yaml")
    }; 
    const std::string inputCfgPath = pystring::os::path::normpath(pystring::os::path::join(inputPaths));
    return OCIO::Config::CreateFromFile(inputCfgPath.c_str());
}

OCIO::ConstConfigRcPtr getConfig(std::string filename)
{
    std::vector<std::string> inputPaths = { 
        std::string(OCIO::GetTestFilesDir()),
        std::string("configs"),
        std::string("mergeconfigs"),
        std::string(filename)
    }; 
    const std::string inputCfgPath = pystring::os::path::normpath(pystring::os::path::join(inputPaths));
    return OCIO::Config::CreateFromFile(inputCfgPath.c_str());
}

void compareEnvironmentVar(const OCIO::ConfigRcPtr & mergedConfig,
                           const std::vector<std::string> & expectedNames,
                           const std::vector<std::string> & expectedValues,
                           int line)
{
    for (int i = 0; i < mergedConfig->getNumEnvironmentVars(); i++)
    {
        std::string name = mergedConfig->getEnvironmentVarNameByIndex(i);
        OCIO_CHECK_EQUAL_FROM(name, expectedNames.at(i), line);
        OCIO_CHECK_EQUAL_FROM(std::string(mergedConfig->getEnvironmentVarDefault(name.c_str())), 
                                expectedValues.at(i),
                                line);
    }
}

OCIO::ConstColorSpaceRcPtr checkColorSpace(const OCIO::ConstConfigRcPtr mergedConfig, 
                                           const char * refName,
                                           const int index,
                                           OCIO::SearchReferenceSpaceType refType,
                                           int line)
{
    const char * name = mergedConfig->getColorSpaceNameByIndex(refType, 
                                                               OCIO::COLORSPACE_ALL,
                                                               index);
    OCIO_CHECK_EQUAL_FROM(std::string(name), std::string(refName), line);
    OCIO::ConstColorSpaceRcPtr cs = mergedConfig->getColorSpace(refName);
    OCIO_REQUIRE_ASSERT_FROM(cs, line);
    return cs;
}

OCIO::ConstNamedTransformRcPtr checkNamedTransform(const OCIO::ConstConfigRcPtr mergedConfig, 
                                                   const char * refName,
                                                   const int index,
                                                   int line)
{
    const char * name = mergedConfig->getNamedTransformNameByIndex(OCIO::NAMEDTRANSFORM_ALL, index);
    OCIO_CHECK_EQUAL_FROM(std::string(name), std::string(refName), line);
    OCIO::ConstNamedTransformRcPtr nt = mergedConfig->getNamedTransform(refName);
    OCIO_REQUIRE_ASSERT_FROM(nt, line);
    return nt;
}

// FIXME: REMOVE
static constexpr char PREFIX[] { "The Input config contains a value that would override the Base config: " };

template<typename Arg>
void loadStringsHelper(std::vector<std::string>& strings, Arg arg) 
{
    strings.push_back(arg);
}

template<typename Arg, typename... Args>
void loadStringsHelper(std::vector<std::string>& strings, Arg arg, Args... args) {
    strings.push_back(arg);
    loadStringsHelper(strings, args...);
}

enum LogType
{
    LOG_TYPE_INFO,
    LOG_TYPE_WARNING,
    LOG_TYPE_ERROR,
};

template<typename... Args>
void checkForLogOrException(LogType type, int line, std::function<void()> setup, Args... args)
{
    std::vector<std::string> strings;
    strings.reserve(sizeof...(args));
    loadStringsHelper(strings, args...);

    // Use INFO rather than DEBUG for the guard to avoid a lot of OpOptimizers.cpp output.
    OCIO::LogGuard logGuard(OCIO::LOGGING_LEVEL_INFO);
    try
    {
        setup();

        for(const auto & s : strings) 
        {
            if (type == LOG_TYPE_ERROR)
            {
// FIXME
//                OCIO_CHECK_ASSERT_FROM(OCIO::checkAndMuteError(logGuard, s), line);

                const bool errorFound = OCIO::checkAndMuteError(logGuard, s);
                if (!errorFound)
                {
                    std::cout << "This error was not found: " << s.c_str() << "\n";
                }
                OCIO_CHECK_ASSERT_FROM(errorFound, line);
            }
            else if (type == LOG_TYPE_WARNING)
            {
                const bool warningFound = OCIO::checkAndMuteWarning(logGuard, s);
                if (!warningFound)
                {
                    std::cout << "This warning was not found: " << s.c_str() << "\n";
                }
                OCIO_CHECK_ASSERT_FROM(warningFound, line);
            }  
        }

        // If all messages have not been removed from the log at this point, it's unexpected.
        if (!logGuard.empty())
        {
            std::cout << "The following unexpected messages were encountered:\n";
            logGuard.print();
        }
        OCIO_CHECK_ASSERT_FROM(logGuard.empty(), line);
    }
    catch(const OCIO::Exception & e)
    {
        // Only checking the first string because only the first Exception gets out.
        OCIO_CHECK_EQUAL_FROM(std::string(e.what()), strings.at(0), line);
    }
}

OCIO_ADD_TEST(MergeConfigs, ociom_parser)
{
    std::vector<std::string> paths = { 
        std::string(OCIO::GetTestFilesDir()),
        std::string("configs"),
        std::string("mergeconfigs"),
        std::string("parser_test.ociom")
    }; 
    const std::string ociomPath = pystring::os::path::normpath(pystring::os::path::join(paths));

    OCIO::ConstConfigMergerRcPtr merger = OCIO::ConfigMerger::CreateFromFile(ociomPath.c_str());

    // The parser_test.ociom contains only one merge.
    OCIO::ConstConfigMergingParametersRcPtr p = merger->getParams(0);

    // Test that the all the options are loaded correctly.
    // Note that is does not test all possibilities.
    // e.g. it does not test all the strategies for all sections.

    OCIO_CHECK_EQUAL(std::string(p->getBaseConfigName()), std::string("base0.ocio"));
    OCIO_CHECK_EQUAL(std::string(p->getInputConfigName()), std::string("input0.ocio"));

    OCIO_CHECK_EQUAL(std::string(p->getInputFamilyPrefix()), std::string("abc"));
    OCIO_CHECK_EQUAL(std::string(p->getBaseFamilyPrefix()), std::string("def"));
    OCIO_CHECK_EQUAL(p->isInputFirst(), true);
    OCIO_CHECK_EQUAL(p->isErrorOnConflict(), false);
    // PreferInput
    OCIO_CHECK_EQUAL(p->getDefaultStrategy(), MergeStrategy::STRATEGY_INPUT_ONLY);
    OCIO_CHECK_EQUAL(p->isAvoidDuplicates(), true);
    OCIO_CHECK_EQUAL(p->isAssumeCommonReferenceSpace(), false);

    OCIO_CHECK_EQUAL(std::string(p->getName()), std::string("my merge"));
    OCIO_CHECK_EQUAL(std::string(p->getDescription()), std::string("my desc"));
    OCIO_CHECK_EQUAL(std::string(p->getSearchPath()), std::string("abc"));

    // Expecting two environment variables.
    OCIO_CHECK_EQUAL(p->getNumEnvironmentVars(), 2);
    OCIO_CHECK_EQUAL(std::string(p->getEnvironmentVar(0)), "test");
    OCIO_CHECK_EQUAL(std::string(p->getEnvironmentVarValue(0)), "valueOther");
    OCIO_CHECK_EQUAL(std::string(p->getEnvironmentVar(1)), "test1");
    OCIO_CHECK_EQUAL(std::string(p->getEnvironmentVarValue(1)), "value123");

    OCIO_CHECK_EQUAL(std::string(p->getActiveDisplays()), std::string("D1, D2"));
    OCIO_CHECK_EQUAL(std::string(p->getActiveViews()), std::string("V1, V2"));
    OCIO_CHECK_EQUAL(std::string(p->getInactiveColorSpaces()), std::string("I1, I2"));

    // PreferInput
    OCIO_CHECK_EQUAL(p->getRoles(), MergeStrategy::STRATEGY_PREFER_INPUT);
    // PreferBase
    OCIO_CHECK_EQUAL(p->getFileRules(), MergeStrategy::STRATEGY_PREFER_BASE);
    // InputOnly
    OCIO_CHECK_EQUAL(p->getDisplayViews(), MergeStrategy::STRATEGY_INPUT_ONLY);
    // BaseOnly
    OCIO_CHECK_EQUAL(p->getLooks(), MergeStrategy::STRATEGY_BASE_ONLY);
    // Remove
    OCIO_CHECK_EQUAL(p->getColorspaces(), MergeStrategy::STRATEGY_REMOVE);
    // PreferBase
    OCIO_CHECK_EQUAL(p->getNamedTransforms(), MergeStrategy::STRATEGY_PREFER_BASE);
}

//TODOCED Add test for OCIOM writer

OCIO_ADD_TEST(MergeConfigs, ociom_parser_no_overrides)
{
    std::vector<std::string> paths = { 
        std::string(OCIO::GetTestFilesDir()),
        std::string("configs"),
        std::string("mergeconfigs"),
        std::string("parser_test_no_overrides.ociom")
    }; 
    const std::string ociomPath = pystring::os::path::normpath(pystring::os::path::join(paths));

    OCIO::ConstConfigMergerRcPtr merger = OCIO::ConfigMerger::CreateFromFile(ociomPath.c_str());

    // The parser_test.ociom contains only one merge.
    OCIO::ConstConfigMergingParametersRcPtr p = merger->getParams(0);

    // Test that the all the options are loaded correctly.
    // Note that is does not test all possibilities.
    // e.g. it does not test all the strategies for all sections.

    OCIO_CHECK_EQUAL(std::string(p->getBaseConfigName()), std::string("input0.ocio"));
    OCIO_CHECK_EQUAL(std::string(p->getInputConfigName()), std::string("input2.ocio"));

    OCIO_CHECK_EQUAL(std::string(p->getInputFamilyPrefix()), std::string("abc"));
    OCIO_CHECK_EQUAL(std::string(p->getBaseFamilyPrefix()), std::string("def"));
    OCIO_CHECK_EQUAL(p->isInputFirst(), true);
    OCIO_CHECK_EQUAL(p->isErrorOnConflict(), false);
    // PreferInput
    OCIO_CHECK_EQUAL(p->getDefaultStrategy(), MergeStrategy::STRATEGY_INPUT_ONLY);
    OCIO_CHECK_EQUAL(p->isAvoidDuplicates(), true);
    OCIO_CHECK_EQUAL(p->isAssumeCommonReferenceSpace(), false);

    OCIO_CHECK_EQUAL(std::string(p->getName()), std::string(""));
    OCIO_CHECK_EQUAL(std::string(p->getDescription()), std::string(""));
    OCIO_CHECK_EQUAL(std::string(p->getSearchPath()), std::string(""));

    // Expecting 0 environment variables.
    OCIO_CHECK_EQUAL(p->getNumEnvironmentVars(), 0);

    OCIO_CHECK_EQUAL(std::string(p->getActiveDisplays()), std::string(""));
    OCIO_CHECK_EQUAL(std::string(p->getActiveViews()), std::string(""));
    OCIO_CHECK_EQUAL(std::string(p->getInactiveColorSpaces()), std::string(""));

    // PreferInput
    OCIO_CHECK_EQUAL(p->getRoles(), MergeStrategy::STRATEGY_PREFER_INPUT);
    // PreferBase
    OCIO_CHECK_EQUAL(p->getFileRules(), MergeStrategy::STRATEGY_PREFER_BASE);
    // InputOnly
    OCIO_CHECK_EQUAL(p->getDisplayViews(), MergeStrategy::STRATEGY_INPUT_ONLY);
    // BaseOnly
    OCIO_CHECK_EQUAL(p->getLooks(), MergeStrategy::STRATEGY_BASE_ONLY);
    // Remove
    OCIO_CHECK_EQUAL(p->getColorspaces(), MergeStrategy::STRATEGY_REMOVE);
    // PreferBase
    OCIO_CHECK_EQUAL(p->getNamedTransforms(), MergeStrategy::STRATEGY_PREFER_BASE);
}

OCIO_ADD_TEST(MergeConfigs, overrides)
{
    OCIO::ConstConfigRcPtr baseConfig;
    OCIO_CHECK_NO_THROW(baseConfig = getBaseConfig());

    OCIO::ConstConfigRcPtr inputConfig;
    OCIO_CHECK_NO_THROW(inputConfig = getInputConfig());

    // Test that the overrides options are taken into account in the merging process.

    auto setupBasics = [](OCIO::ConfigMergerRcPtr & merger, MergeStrategy strategy) -> OCIO::ConfigMergingParametersRcPtr
    {
        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        merger->addParams(params);
        merger->getParams(0)->setDisplayViews(strategy);
        merger->getParams(0)->setColorspaces(strategy);
        // Not looking for duplicates as this test does not test that.
        merger->getParams(0)->setAvoidDuplicates(false);
//        merger->getParams(0)->setAssumeCommonReferenceSpace(true);

                // Set the overrides.
                merger->getParams(0)->setName("OVR Name");
                merger->getParams(0)->setDescription("OVR Desc");
                merger->getParams(0)->setSearchPath("OVR1,OVR2");
                merger->getParams(0)->addEnvironmentVar("OVR1", "VALUE1");
                merger->getParams(0)->addEnvironmentVar("OVR2", "VALUE2");
                merger->getParams(0)->setActiveDisplays("OVR DISP 1,OVR DISP 2");
                merger->getParams(0)->setActiveViews("OVR VIEW 1,OVR VIEW 2");
                merger->getParams(0)->setInactiveColorspaces("view_1, ACES2065-1");

        return params;
    };

    auto doTests = [](OCIO::ConfigRcPtr & mergedConfig, int line)
    {
        OCIO_CHECK_EQUAL_FROM(std::string(mergedConfig->getName()), std::string("OVR Name"), line);
        OCIO_CHECK_EQUAL_FROM(std::string(mergedConfig->getDescription()), std::string("OVR Desc"), line);

        OCIO_CHECK_EQUAL_FROM(std::string(mergedConfig->getSearchPath()), std::string("OVR1,OVR2"), line);

        std::vector<std::string> expectedNames = { "OVR1", "OVR2" };
        std::vector<std::string> expectedValues = { "VALUE1", "VALUE2" };
        OCIO_CHECK_EQUAL_FROM(mergedConfig->getNumEnvironmentVars(), 2, line);
        compareEnvironmentVar(mergedConfig, expectedNames, expectedValues, line);

        OCIO_CHECK_EQUAL_FROM(std::string(mergedConfig->getActiveDisplays()), "OVR DISP 1, OVR DISP 2", line);
        OCIO_CHECK_EQUAL_FROM(std::string(mergedConfig->getActiveViews()), 
                              "OVR VIEW 1, OVR VIEW 2", line);

        OCIO_CHECK_EQUAL_FROM(std::string(mergedConfig->getInactiveColorSpaces()), "view_1, ACES2065-1", line);
    };

    // Test sections with strategy = PreferInput
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
           [&options]() 
            {
                // Merge name and description.
                OCIO::GeneralMerger(options).merge();
                // Merge active_display, active_views.
                OCIO::DisplayViewMerger(options).merge();
                // Merge inactive_colorspaces, environment and search_path.
                OCIO::ColorspacesMerger(options).merge();
            },
            "The Input config contains a value that would override the Base config: shared_views: SHARED_1",
            "The Input config contains a value that would override the Base config: display: DISP_1, view: VIEW_1",
            "The Input config contains a value that would override the Base config: default_view_transform: Un-tone-mapped-2",
            "The Input config contains a value that would override the Base config: viewing_rules: RULE_1",
            "Color space 'ACES2065-1' will replace a color space in the base config.",
            "Color space 'view_1' will replace a color space in the base config.");
        doTests(mergedConfig, __LINE__);
    }

    // Test sections with strategy = PreferBase.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
           [&options]() 
            {
                // Merge name and description.
                OCIO::GeneralMerger(options).merge();
                // Merge active_display, active_views.
                OCIO::DisplayViewMerger(options).merge();
                // Merge inactive_colorspaces, environment and search_path.
                OCIO::ColorspacesMerger(options).merge();
            },
            "The Input config contains a value that would override the Base config: shared_views: SHARED_1",
            "The Input config contains a value that would override the Base config: display: DISP_1, view: VIEW_1",
            "The Input config contains a value that would override the Base config: default_view_transform: Un-tone-mapped-2",
            "The Input config contains a value that would override the Base config: viewing_rules: RULE_1",
            "Color space 'ACES2065-1' was not merged as it's already present in the base config.",
            "Color space 'view_1' was not merged as it's already present in the base config.");
        doTests(mergedConfig, __LINE__);
    }

    // Test sections with strategy = InputOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_INPUT_ONLY);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        // Merge name and description.
        OCIO::GeneralMerger(options).merge();
        // Merge active_display, active_views.
        OCIO::DisplayViewMerger(options).merge();
        // Merge inactive_colorspaces, environment and search_path.
        OCIO::ColorspacesMerger(options).merge();
        doTests(mergedConfig, __LINE__);
    }

    // Test sections with strategy = BaseOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_BASE_ONLY);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        // Merge name and description.
        OCIO::GeneralMerger(options).merge();
        // Merge active_display, active_views.
        OCIO::DisplayViewMerger(options).merge();
        // Merge inactive_colorspaces, environment and search_path.
        OCIO::ColorspacesMerger(options).merge();
        doTests(mergedConfig, __LINE__);
    }

    // Strategy Remove is not tested as the overrides do not affect that strategy.
}

OCIO_ADD_TEST(MergeConfigs, general_section)
{
    OCIO::ConstConfigRcPtr baseConfig;
    OCIO_CHECK_NO_THROW(baseConfig = getBaseConfig());

    OCIO::ConstConfigRcPtr inputConfig;
    OCIO_CHECK_NO_THROW(inputConfig = getInputConfig());

//     auto setupGeneral = [&baseConfig, &inputConfig](
//                         OCIO::ConfigMergerRcPtr & merger,
//                         OCIO::ConfigRcPtr mergedConfig,
//                         std::function<void(OCIO::ConfigMergerRcPtr &)> cb)
//     {
//         OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
//         merger->addParams(params);
// 
//         if (cb)
//         {
//             cb(merger);
//         }
// 
//         OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
//         OCIO::GeneralMerger(options).merge();
//     };

/// FIXME InputFirst has no impact on general.  Should test name, desc, profile version.

    // Test general merger (uses default strategy) with options InputFirst = true.
//     {
//         OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
//         OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
//         setupGeneral(merger, 
//                      mergedConfig, 
//                      { 
//                          merger->getParams(0)->setInputFirst(true);
//                      });
//         
// 
//         OCIO_CHECK_EQUAL(std::string(mergedConfig->getName()), std::string("input0"));
//         OCIO_CHECK_EQUAL(std::string(mergedConfig->getDescription()), std::string("My description 2"));
//     }
// 
//     // Test general merger (uses default strategy) with options InputFirst = false.
//     {
//         OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
//         OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
//         setupGeneral(merger, 
//                      mergedConfig, 
//                      [](OCIO::ConfigMergerRcPtr & merger)
//                      { 
//                          merger->getParams(0)->setInputFirst(false);
//                      });
// 
//         OCIO_CHECK_EQUAL(std::string(mergedConfig->getName()), std::string("input0"));
//         OCIO_CHECK_EQUAL(std::string(mergedConfig->getDescription()), std::string("My description 2"));
//     }
}

OCIO_ADD_TEST(MergeConfigs, roles_section)
{
    // Allowed strategies: PreferInput, PreferBase, InputOnly, BaseOnly, Remove
    // Allowed merge options: ErrorOnConflict.  

    OCIO::ConstConfigRcPtr baseConfig;
    OCIO_CHECK_NO_THROW(baseConfig = getBaseConfig());

    OCIO::ConstConfigRcPtr inputConfig;
    OCIO_CHECK_NO_THROW(inputConfig = getInputConfig());

    auto setupBasics = [](OCIO::ConfigMergerRcPtr & merger, MergeStrategy strategy) -> OCIO::ConfigMergingParametersRcPtr
    {
        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        merger->addParams(params);
        merger->getParams(0)->setRoles(strategy);
        merger->getParams(0)->setDefaultStrategy(strategy);
        return params;
    };

    // Test that the default strategy is used as a fallback if the section strategy was not defined.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        // Using STRATEGY_UNSET as this simulates that the section
        // is missing from the OCIOM file.
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_UNSET);
        // Simulate settings from OCIOM file.
        merger->getParams(0)->setDefaultStrategy(MergeStrategy::STRATEGY_INPUT_ONLY);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                               [&options]() { OCIO::RolesMerger(options).merge(); },
                               "The Input config contains a role 'g22_ap1_tx' that would override an alias of Base config color space 'Gamma 2.2 AP1 - Texture'",
                               "The Input config contains a role 'nt_base' that would override Base config named transform: 'nt_base'");

        OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("aces_interchange")),
                         std::string("ACES2065-1"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("texture_paint")),
                         std::string("ACEScct - SomeOtherName"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("matte_paint")),
                         std::string("sRGB - Texture"));
    }

    // Test Roles section with strategy = PreferInput
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                               [&options]() { OCIO::RolesMerger(options).merge(); },
                               "The Input config contains a role 'g22_ap1_tx' that would override an alias of Base config color space 'Gamma 2.2 AP1 - Texture'",
                               "The Input config contains a role that would override Base config role 'texture_paint'.",
                               "The Input config contains a role 'nt_base' that would override Base config named transform: 'nt_base'");

        OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 4);

        // Following three roles were overwritten by input config.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("aces_interchange")),
                         std::string("ACES2065-1"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("texture_paint")),
                         std::string("ACEScct - SomeOtherName"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("matte_paint")),
                         std::string("sRGB - Texture"));

        // Following role come from base config.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("data")),
                         std::string("Raw"));
    }

    // Test Roles section with strategy = PreferBase
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                               [&options]() { OCIO::RolesMerger(options).merge(); },
                               "The Input config contains a role 'g22_ap1_tx' that would override an alias of Base config color space 'Gamma 2.2 AP1 - Texture'",
                               "The Input config contains a role that would override Base config role 'texture_paint'.",
                               "The Input config contains a role 'nt_base' that would override Base config named transform: 'nt_base'");

        OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 4);

        // Following three roles comes form the base config.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("aces_interchange")),
                         std::string("ACES2065-1"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("texture_paint")),
                         std::string("ACEScct"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("data")),
                         std::string("Raw"));

        // Following role come from input config.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("matte_paint")),
                         std::string("sRGB - Texture"));
    }

    // Test Roles section with strategy = InputOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_INPUT_ONLY);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                               [&options]() { OCIO::RolesMerger(options).merge(); },
                               "The Input config contains a role 'g22_ap1_tx' that would override an alias of Base config color space 'Gamma 2.2 AP1 - Texture'",
                               "The Input config contains a role 'nt_base' that would override Base config named transform: 'nt_base'");

        OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 3);

        // Following three roles comes form the input config.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("aces_interchange")),
                         std::string("ACES2065-1"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("texture_paint")),
                         std::string("ACEScct - SomeOtherName"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("matte_paint")),
                         std::string("sRGB - Texture"));
    }

    // Test Roles section with strategy = BaseOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_BASE_ONLY);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::RolesMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 3);

        // Following three roles comes form the base config.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("aces_interchange")),
                         std::string("ACES2065-1"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("texture_paint")),
                         std::string("ACEScct"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("data")),
                         std::string("Raw"));
    }

    // Test Roles section with strategy = Remove
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_REMOVE);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::RolesMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 1);

        // This is the only role in base that is not in input.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("data")),
                         std::string("Raw"));
    }

    // Test Roles section with strategy = PreferInput and option ErrorOnConflict = true.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
        merger->getParams(0)->setErrorOnConflict(true);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_ERROR, __LINE__, 
                               [&options]() { OCIO::RolesMerger(options).merge(); },
                               "The Input config contains a role 'g22_ap1_tx' that would override an alias of Base config color space 'Gamma 2.2 AP1 - Texture'.");
    }
}

OCIO_ADD_TEST(MergeConfigs, file_rules_section)
{
    OCIO::ConstConfigRcPtr baseConfig;
    OCIO_CHECK_NO_THROW(baseConfig = getBaseConfig());

    OCIO::ConstConfigRcPtr inputConfig;
    OCIO_CHECK_NO_THROW(inputConfig = getInputConfig());

    auto setupBasics = [](OCIO::ConfigMergerRcPtr & merger, MergeStrategy strategy) -> OCIO::ConfigMergingParametersRcPtr
    {
        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        merger->addParams(params);
        merger->getParams(0)->setFileRules(strategy);
        merger->getParams(0)->setAssumeCommonReferenceSpace(true);
        merger->getParams(0)->setAvoidDuplicates(false);

        return params;
    };

    // Allowed strategies: All
    // Allowed merge options: All  

    // Test that the default strategy is used as a fallback if the section strategy was not defined.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        // Using STRATEGY_UNSET as this simulates that the section is missing from the OCIOM file.
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_UNSET);
        // Simulate settings from OCIOM file.
        merger->getParams(0)->setDefaultStrategy(MergeStrategy::STRATEGY_INPUT_ONLY);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::FileRulesMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->isStrictParsingEnabled(), false);

        OCIO::ConstFileRulesRcPtr fr = mergedConfig->getFileRules();

        OCIO_CHECK_EQUAL(fr->getNumEntries(), 5);

        OCIO_CHECK_EQUAL(std::string(fr->getName(0)), "LogC");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(0)), "ACES2065-1");

        OCIO_CHECK_EQUAL(std::string(fr->getName(1)), "TIFF");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(1)), "sRGB - Texture");

        OCIO_CHECK_EQUAL(std::string(fr->getName(2)), "JPEG");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(2)), "Linear Rec.2020");
        OCIO_CHECK_EQUAL(std::string(fr->getRegex(2)), ".*\\.jpeg$");

        OCIO_CHECK_EQUAL(std::string(fr->getName(3)), "ColorSpaceNamePathSearch");

        OCIO_CHECK_EQUAL(std::string(fr->getName(4)), "Default");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(4)), "ACEScct - SomeOtherName");
    }

    // Test FileRules section with strategy = PreferInput.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
        merger->getParams(0)->setInputFirst(true);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                [&options]() { OCIO::FileRulesMerger(options).merge(); },
                                "The Input config contains a value that would override the Base config: file_rules: TIFF",
                                "The Input config contains a value that would override the Base config: file_rules: Default");

        OCIO::ConstFileRulesRcPtr fr = mergedConfig->getFileRules();

        OCIO_CHECK_EQUAL(fr->getNumEntries(), 6);

        OCIO_CHECK_EQUAL(std::string(fr->getName(0)), "LogC");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(0)), "ACES2065-1");

        OCIO_CHECK_EQUAL(std::string(fr->getName(1)), "TIFF");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(1)), "sRGB - Texture");
        OCIO_CHECK_EQUAL(std::string(fr->getRegex(1)), ".*\\.TIF?F$");
        // Verify that the custom keys are merged.
        OCIO_CHECK_EQUAL(std::string(fr->getCustomKeyName(1, 0)), "key1");
        OCIO_CHECK_EQUAL(std::string(fr->getCustomKeyValue(1, 0)), "value1");
        OCIO_CHECK_EQUAL(std::string(fr->getCustomKeyName(1, 1)), "key2");
        OCIO_CHECK_EQUAL(std::string(fr->getCustomKeyValue(1, 1)), "value2");

        OCIO_CHECK_EQUAL(std::string(fr->getName(2)), "JPEG");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(2)), "Linear Rec.2020");
        OCIO_CHECK_EQUAL(std::string(fr->getRegex(2)), ".*\\.jpeg$");

        OCIO_CHECK_EQUAL(std::string(fr->getName(3)), "ColorSpaceNamePathSearch");

        OCIO_CHECK_EQUAL(std::string(fr->getName(4)), "OpenEXR");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(4)), "ACEScct");
        OCIO_CHECK_EQUAL(std::string(fr->getPattern(4)), "*");
        OCIO_CHECK_EQUAL(std::string(fr->getExtension(4)), "exr");

        OCIO_CHECK_EQUAL(std::string(fr->getName(5)), "Default");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(5)), "ACEScct - SomeOtherName");
    } 

    // Test FileRules section with strategy = PreferInput, options InputFirst = false.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
        merger->getParams(0)->setInputFirst(false);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                [&options]() { OCIO::FileRulesMerger(options).merge(); },
                                "The Input config contains a value that would override the Base config: file_rules: TIFF",
                                "The Input config contains a value that would override the Base config: file_rules: Default");

        OCIO_CHECK_EQUAL(mergedConfig->isStrictParsingEnabled(), false);

        OCIO::ConstFileRulesRcPtr fr = mergedConfig->getFileRules();

        OCIO_CHECK_EQUAL(fr->getNumEntries(), 6);

        OCIO_CHECK_EQUAL(std::string(fr->getName(0)), "LogC");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(0)), "ACES2065-1");

        OCIO_CHECK_EQUAL(std::string(fr->getName(1)), "TIFF");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(1)), "sRGB - Texture");
        // Verify that the custom keys are merged.
        OCIO_CHECK_EQUAL(std::string(fr->getCustomKeyName(1, 0)), "key1");
        OCIO_CHECK_EQUAL(std::string(fr->getCustomKeyValue(1, 0)), "value1");
        OCIO_CHECK_EQUAL(std::string(fr->getCustomKeyName(1, 1)), "key2");
        OCIO_CHECK_EQUAL(std::string(fr->getCustomKeyValue(1, 1)), "value2");

        OCIO_CHECK_EQUAL(std::string(fr->getName(2)), "OpenEXR");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(2)), "ACEScct");
        OCIO_CHECK_EQUAL(std::string(fr->getPattern(2)), "*");
        OCIO_CHECK_EQUAL(std::string(fr->getExtension(2)), "exr");

        OCIO_CHECK_EQUAL(std::string(fr->getName(3)), "ColorSpaceNamePathSearch");

        OCIO_CHECK_EQUAL(std::string(fr->getName(4)), "JPEG");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(4)), "Linear Rec.2020");
        OCIO_CHECK_EQUAL(std::string(fr->getRegex(4)), ".*\\.jpeg$");

        OCIO_CHECK_EQUAL(std::string(fr->getName(5)), "Default");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(5)), "ACEScct - SomeOtherName");
    } 

    // Test FileRules section with strategy = PreferBase.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
        merger->getParams(0)->setInputFirst(true);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                [&options]() { OCIO::FileRulesMerger(options).merge(); },
                                "The Input config contains a value that would override the Base config: file_rules: TIFF",
                                "The Input config contains a value that would override the Base config: file_rules: Default");

        OCIO_CHECK_EQUAL(mergedConfig->isStrictParsingEnabled(), true);

        OCIO::ConstFileRulesRcPtr fr = mergedConfig->getFileRules();

        OCIO_CHECK_EQUAL(fr->getNumEntries(), 6);

        OCIO_CHECK_EQUAL(std::string(fr->getName(0)), "LogC");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(0)), "ACES2065-1");

        OCIO_CHECK_EQUAL(std::string(fr->getName(1)), "TIFF");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(1)), "Gamma 2.2 AP1 - Texture");
        OCIO_CHECK_EQUAL(fr->getNumCustomKeys(1), 0);

        OCIO_CHECK_EQUAL(std::string(fr->getName(2)), "JPEG");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(2)), "Linear Rec.2020");
        OCIO_CHECK_EQUAL(std::string(fr->getRegex(2)), ".*\\.jpeg$");

        OCIO_CHECK_EQUAL(std::string(fr->getName(3)), "ColorSpaceNamePathSearch");

        OCIO_CHECK_EQUAL(std::string(fr->getName(4)), "OpenEXR");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(4)), "ACEScct");
        OCIO_CHECK_EQUAL(std::string(fr->getPattern(4)), "*");
        OCIO_CHECK_EQUAL(std::string(fr->getExtension(4)), "exr");

        OCIO_CHECK_EQUAL(std::string(fr->getName(5)), "Default");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(5)), "Raw");
    } 

    // Test FileRules section with strategy = PreferBase, options InputFirst = false.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
        merger->getParams(0)->setInputFirst(false);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                [&options]() { OCIO::FileRulesMerger(options).merge(); },
                                "The Input config contains a value that would override the Base config: file_rules: TIFF",
                                "The Input config contains a value that would override the Base config: file_rules: Default");

        OCIO_CHECK_EQUAL(mergedConfig->isStrictParsingEnabled(), true);

        OCIO::ConstFileRulesRcPtr fr = mergedConfig->getFileRules();

        OCIO_CHECK_EQUAL(fr->getNumEntries(), 6);

        OCIO_CHECK_EQUAL(std::string(fr->getName(0)), "LogC");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(0)), "ACES2065-1");

        OCIO_CHECK_EQUAL(std::string(fr->getName(1)), "TIFF");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(1)), "Gamma 2.2 AP1 - Texture");

        OCIO_CHECK_EQUAL(std::string(fr->getName(2)), "OpenEXR");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(2)), "ACEScct");
        OCIO_CHECK_EQUAL(std::string(fr->getPattern(2)), "*");
        OCIO_CHECK_EQUAL(std::string(fr->getExtension(2)), "exr");

        OCIO_CHECK_EQUAL(std::string(fr->getName(3)), "ColorSpaceNamePathSearch");

        OCIO_CHECK_EQUAL(std::string(fr->getName(4)), "JPEG");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(4)), "Linear Rec.2020");
        OCIO_CHECK_EQUAL(std::string(fr->getRegex(4)), ".*\\.jpeg$");

        OCIO_CHECK_EQUAL(std::string(fr->getName(5)), "Default");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(5)), "Raw");
    } 

    // Test FileRules section with strategy = InputOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_INPUT_ONLY);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::FileRulesMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->isStrictParsingEnabled(), false);

        OCIO::ConstFileRulesRcPtr fr = mergedConfig->getFileRules();

        OCIO_CHECK_EQUAL(fr->getNumEntries(), 5);

        OCIO_CHECK_EQUAL(std::string(fr->getName(0)), "LogC");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(0)), "ACES2065-1");

        OCIO_CHECK_EQUAL(std::string(fr->getName(1)), "TIFF");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(1)), "sRGB - Texture");

        OCIO_CHECK_EQUAL(std::string(fr->getName(2)), "JPEG");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(2)), "Linear Rec.2020");
        OCIO_CHECK_EQUAL(std::string(fr->getRegex(2)), ".*\\.jpeg$");

        OCIO_CHECK_EQUAL(std::string(fr->getName(3)), "ColorSpaceNamePathSearch");

        OCIO_CHECK_EQUAL(std::string(fr->getName(4)), "Default");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(4)), "ACEScct - SomeOtherName");
    } 

    // Test FileRules section with strategy = BaseOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_BASE_ONLY);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::FileRulesMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->isStrictParsingEnabled(), true);

        OCIO::ConstFileRulesRcPtr fr = mergedConfig->getFileRules();

        OCIO_CHECK_EQUAL(std::string(fr->getName(0)), "LogC");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(0)), "ACES2065-1");

        OCIO_CHECK_EQUAL(std::string(fr->getName(1)), "TIFF");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(1)), "Gamma 2.2 AP1 - Texture");

        OCIO_CHECK_EQUAL(std::string(fr->getName(2)), "OpenEXR");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(2)), "ACEScct");
        OCIO_CHECK_EQUAL(std::string(fr->getPattern(2)), "*");
        OCIO_CHECK_EQUAL(std::string(fr->getExtension(2)), "exr");

        OCIO_CHECK_EQUAL(std::string(fr->getName(3)), "ColorSpaceNamePathSearch");

        OCIO_CHECK_EQUAL(std::string(fr->getName(4)), "Default");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(4)), "Raw");
    } 

    // Test FileRules section with strategy = Remove.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_REMOVE);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::FileRulesMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->isStrictParsingEnabled(), true);

        OCIO::ConstFileRulesRcPtr fr = mergedConfig->getFileRules();

        OCIO_CHECK_EQUAL(fr->getNumEntries(), 2);

        OCIO_CHECK_EQUAL(std::string(fr->getName(0)), "OpenEXR");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(0)), "ACEScct");
        OCIO_CHECK_EQUAL(std::string(fr->getPattern(0)), "*");
        OCIO_CHECK_EQUAL(std::string(fr->getExtension(0)), "exr");

        OCIO_CHECK_EQUAL(std::string(fr->getName(1)), "Default");
        OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(1)), "Raw");
    }

    // Test FileRules section with strategy = PreferInput and copying ColorSpaceNamePathSearch.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();

        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        merger->addParams(params);
        merger->getParams(0)->setFileRules(MergeStrategy::STRATEGY_PREFER_INPUT);

        {
            OCIO::ConfigRcPtr editableInputConfig = inputConfig->createEditableCopy();
            OCIO::FileRulesRcPtr inputFr = editableInputConfig->getFileRules()->createEditableCopy();
            // Delete ColorSpaceNamePathSearch, so it is only in the base and must be copied over.
            inputFr->removeRule(3);
            editableInputConfig->setFileRules(inputFr);

            OCIO::MergeHandlerOptions options = { baseConfig, editableInputConfig, 
                                                  params, mergedConfig };
            checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                [&options]() { OCIO::FileRulesMerger(options).merge(); },
                "The Input config contains a value that would override the Base config: file_rules: TIFF",
                "The Input config contains a value that would override the Base config: file_rules: Default");

            OCIO::ConstFileRulesRcPtr fr = mergedConfig->getFileRules();

            OCIO_CHECK_EQUAL(fr->getNumEntries(), 6);

            OCIO_CHECK_EQUAL(std::string(fr->getName(0)), "LogC");
            OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(0)), "ACES2065-1");

            OCIO_CHECK_EQUAL(std::string(fr->getName(1)), "TIFF");
            OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(1)), "sRGB - Texture");
            OCIO_CHECK_EQUAL(std::string(fr->getRegex(1)), ".*\\.TIF?F$");
            // Verify that the custom keys are merged.
            OCIO_CHECK_EQUAL(std::string(fr->getCustomKeyName(1, 0)), "key1");
            OCIO_CHECK_EQUAL(std::string(fr->getCustomKeyValue(1, 0)), "value1");
            OCIO_CHECK_EQUAL(std::string(fr->getCustomKeyName(1, 1)), "key2");
            OCIO_CHECK_EQUAL(std::string(fr->getCustomKeyValue(1, 1)), "value2");

            OCIO_CHECK_EQUAL(std::string(fr->getName(2)), "JPEG");
            OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(2)), "Linear Rec.2020");
            OCIO_CHECK_EQUAL(std::string(fr->getRegex(2)), ".*\\.jpeg$");

            OCIO_CHECK_EQUAL(std::string(fr->getName(3)), "OpenEXR");
            OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(3)), "ACEScct");
            OCIO_CHECK_EQUAL(std::string(fr->getPattern(3)), "*");
            OCIO_CHECK_EQUAL(std::string(fr->getExtension(3)), "exr");

            OCIO_CHECK_EQUAL(std::string(fr->getName(4)), "ColorSpaceNamePathSearch");

            OCIO_CHECK_EQUAL(std::string(fr->getName(5)), "Default");
            OCIO_CHECK_EQUAL(std::string(fr->getColorSpace(5)), "ACEScct - SomeOtherName");
        }
    }

    // Test that error_on_conflicts is processed correctly.
    // strategy = PreferInput, InputFirst = true
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();

        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        merger->addParams(params);
        merger->getParams(0)->setFileRules(MergeStrategy::STRATEGY_PREFER_INPUT);
        merger->getParams(0)->setErrorOnConflict(true);
        merger->getParams(0)->setDefaultStrategy(MergeStrategy::STRATEGY_PREFER_INPUT);

        // Test that an error is thrown when the input config's COLORSPACE is different.
        {
            OCIO::ConfigRcPtr editableBaseConfig = baseConfig->createEditableCopy();
            OCIO::FileRulesRcPtr baseFr = OCIO::FileRules::Create();
            baseFr->insertRule(0, "ruleTestColorspace", "colorspace1", "*abc*", "*");
            editableBaseConfig->setFileRules(baseFr);

            OCIO::ConfigRcPtr editableInputConfig = inputConfig->createEditableCopy();
            OCIO::FileRulesRcPtr inputFr = OCIO::FileRules::Create();
            inputFr->insertRule(0, "ruleTestColorspace", "colorspace2", "*abc*", "*");
            editableInputConfig->setFileRules(inputFr);

            OCIO::MergeHandlerOptions options = { editableBaseConfig, editableInputConfig, 
                                                  params, mergedConfig };
            checkForLogOrException(LOG_TYPE_ERROR, __LINE__, 
                                   [&options]() { OCIO::FileRulesMerger(options).merge(); },
                                   std::string(PREFIX) + std::string("file_rules: ruleTestColorspace"));
        }

        // Test that an error is thrown when the input config's REGEX is different.
        {
            OCIO::ConfigRcPtr editableBaseConfig = baseConfig->createEditableCopy();
            OCIO::FileRulesRcPtr baseFr = OCIO::FileRules::Create();
            baseFr->insertRule(0, "ruleTestColorspace", "colorspace1", ".*\\.TIF?F$");
            editableBaseConfig->setFileRules(baseFr);

            OCIO::ConfigRcPtr editableInputConfig = inputConfig->createEditableCopy();
            OCIO::FileRulesRcPtr inputFr = OCIO::FileRules::Create();
            inputFr->insertRule(0, "ruleTestColorspace", "colorspace1", ".*\\.TIF?F");
            editableInputConfig->setFileRules(inputFr);

            OCIO::MergeHandlerOptions options = { editableBaseConfig, editableInputConfig, 
                                                  params, mergedConfig };
            checkForLogOrException(LOG_TYPE_ERROR, __LINE__, 
                                   [&options]() { OCIO::FileRulesMerger(options).merge(); },
                                   std::string(PREFIX) + std::string("file_rules: ruleTestColorspace"));
        }

        // Test that an error is thrown when the input config's PATTERN is different.
        {
            OCIO::ConfigRcPtr editableBaseConfig = baseConfig->createEditableCopy();
            OCIO::FileRulesRcPtr baseFr = OCIO::FileRules::Create();
            baseFr->insertRule(0, "ruleTestPattern", "colorspace1", "*abc*", "*");
            editableBaseConfig->setFileRules(baseFr);

            OCIO::ConfigRcPtr editableInputConfig = inputConfig->createEditableCopy();
            OCIO::FileRulesRcPtr inputFr = OCIO::FileRules::Create();
            inputFr->insertRule(0, "ruleTestPattern", "colorspace1", "*abcd*", "*");
            editableInputConfig->setFileRules(inputFr);

            OCIO::MergeHandlerOptions options = { editableBaseConfig, editableInputConfig, 
                                                  params, mergedConfig };
            checkForLogOrException(LOG_TYPE_ERROR, __LINE__, 
                                   [&options]() { OCIO::FileRulesMerger(options).merge(); },
                                   std::string(PREFIX) + std::string("file_rules: ruleTestPattern"));
        }

        // Test that an error is thrown when the input config's EXTENSION is different.
        {
            OCIO::ConfigRcPtr editableBaseConfig = baseConfig->createEditableCopy();
            OCIO::FileRulesRcPtr baseFr = OCIO::FileRules::Create();
            baseFr->insertRule(0, "ruleTestExtension", "colorspace1", "*abc*", "*");
            editableBaseConfig->setFileRules(baseFr);

            OCIO::ConfigRcPtr editableInputConfig = inputConfig->createEditableCopy();
            OCIO::FileRulesRcPtr inputFr = OCIO::FileRules::Create();
            inputFr->insertRule(0, "ruleTestExtension", "colorspace1", "*abc*", "*a");
            editableInputConfig->setFileRules(inputFr);

            OCIO::MergeHandlerOptions options = { editableBaseConfig, editableInputConfig, 
                                                  params, mergedConfig };
            checkForLogOrException(LOG_TYPE_ERROR, __LINE__, 
                                   [&options]() { OCIO::FileRulesMerger(options).merge(); },
                                   std::string(PREFIX) + std::string("file_rules: ruleTestExtension"));
        }

        // Test that an error is thrown when the input config's CUSTOM KEYS are different.
        {
            OCIO::ConfigRcPtr editableBaseConfig = baseConfig->createEditableCopy();
            OCIO::FileRulesRcPtr baseFr = OCIO::FileRules::Create();
            baseFr->insertRule(0, "ruleTestCustomKeys", "colorspace1", "*abc*", "*");
            baseFr->setCustomKey(0, "key1", "value1");
            baseFr->setCustomKey(0, "key2", "value2");
            editableBaseConfig->setFileRules(baseFr);

            OCIO::ConfigRcPtr editableInputConfig = inputConfig->createEditableCopy();
            OCIO::FileRulesRcPtr inputFr = OCIO::FileRules::Create();
            inputFr->insertRule(0, "ruleTestCustomKeys", "colorspace1", "*abc*", "*");
            inputFr->setCustomKey(0, "key1", "value1");
            inputFr->setCustomKey(0, "key2", "value22");
            editableInputConfig->setFileRules(inputFr);

            OCIO::MergeHandlerOptions options = { editableBaseConfig, editableInputConfig, 
                                                  params, mergedConfig };
            checkForLogOrException(LOG_TYPE_ERROR, __LINE__, 
                                   [&options]() { OCIO::FileRulesMerger(options).merge(); },
                                   std::string(PREFIX) + std::string("file_rules: ruleTestCustomKeys"));
        }

        // Test that no error is thrown when the input config's CUSTOM KEYS are the same.
        {
            OCIO::ConfigRcPtr editableBaseConfig = baseConfig->createEditableCopy();
            OCIO::FileRulesRcPtr baseFr = OCIO::FileRules::Create();
            baseFr->insertRule(0, "ruleTestCustomKeys", "colorspace1", "*abc*", "*");
            baseFr->setCustomKey(0, "key1", "value1");
            baseFr->setCustomKey(0, "key2", "value2");
            editableBaseConfig->setFileRules(baseFr);

            OCIO::ConfigRcPtr editableInputConfig = inputConfig->createEditableCopy();
            OCIO::FileRulesRcPtr inputFr = OCIO::FileRules::Create();
            inputFr->insertRule(0, "ruleTestCustomKeys", "colorspace1", "*abc*", "*");
            inputFr->setCustomKey(0, "key2", "value2");
            inputFr->setCustomKey(0, "key1", "value1");  // must be equal even in a different order
            editableInputConfig->setFileRules(inputFr);

            OCIO::MergeHandlerOptions options = { editableBaseConfig, editableInputConfig, 
                                                  params, mergedConfig };
            OCIO_CHECK_NO_THROW(OCIO::FileRulesMerger(options).merge());
        }
    }
}

OCIO_ADD_TEST(MergeConfigs, displays_views_section)
{
    OCIO::ConstConfigRcPtr baseConfig;
    OCIO_CHECK_NO_THROW(baseConfig = getBaseConfig());

    OCIO::ConstConfigRcPtr inputConfig;
    OCIO_CHECK_NO_THROW(inputConfig = getInputConfig());

    auto setupBasics = [](OCIO::ConfigMergerRcPtr & merger, MergeStrategy strategy) -> OCIO::ConfigMergingParametersRcPtr
    {
        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        merger->addParams(params);
        merger->getParams(0)->setDisplayViews(strategy);
        return params;
    };

    // Allowed strategies: All
    // Allowed merge options: All

    // Test that the default strategy is used as a fallback if the section strategy was not defined.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();

        // Using STRATEGY_UNSET as this simulates that the section
        // is missing from the OCIOM file.
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_UNSET);
        // Simulate settings from OCIOM file.
        merger->getParams(0)->setDefaultStrategy(MergeStrategy::STRATEGY_INPUT_ONLY);
        merger->getParams(0)->setInputFirst(true);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::DisplayViewMerger(options).merge();

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveDisplays()), "DISP_1, DISP_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveViews()), 
                                     "SHARED_1, SHARED_3, VIEW_1, VIEW_3");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDefaultViewTransformName()), "Un-tone-mapped-2");

        // Validate shared_views
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, nullptr), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 0)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 1)), "SHARED_3");

        // Validate displays
        OCIO_CHECK_EQUAL(mergedConfig->getNumDisplaysAll(), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(0)), "DISP_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(1)), "DISP_3");

        // Validate display/views

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_1"), 1);
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, "DISP_1"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED,"DISP_1", 0)), "VIEW_1");
        // Make sure this is the right VIEW_1 by checking the colorspace.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "VIEW_1")), "view_1B");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 0)), "SHARED_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_3")), "log_3");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 1)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_1")), "lin_3");

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3", 0)), "VIEW_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3", 1)), "VIEW_3");

        // Validate view_transforms
        OCIO_CHECK_EQUAL(mergedConfig->getNumViewTransforms(), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(0)), "SDR Video");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(1)), "Un-tone-mapped-2");

       // Validate viewing_rules
        OCIO::ConstViewingRulesRcPtr rules = mergedConfig->getViewingRules();

        OCIO_CHECK_EQUAL(rules->getNumEntries(), 2);

        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), "RULE_1");
        OCIO_CHECK_EQUAL(rules->getNumColorSpaces(0), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(0, 0)), "sRGB - Texture");

        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), "RULE_3");
        OCIO_CHECK_EQUAL(rules->getNumColorSpaces(1), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1, 0)), "Linear Rec.2020");
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1, 1)), "ACEScct - SomeOtherName");

        // Validate virtual_display
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED), 2); 
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_SHARED), 1); 

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 0)), "ACES");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 1)), "Lin");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_SHARED, 0)), "SHARED_3");
    }

    // Test display/views with strategy = PreferInput, options InputFirst = true.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
        merger->getParams(0)->setInputFirst(true);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
           [&options]() { OCIO::DisplayViewMerger(options).merge(); },
            "The Input config contains a value that would override the Base config: shared_views: SHARED_1",
            "The Input config contains a value that would override the Base config: display: DISP_1, view: VIEW_1",
            "The Input config contains a value that would override the Base config: default_view_transform: Un-tone-mapped-2",
            "The Input config contains a value that would override the Base config: viewing_rules: RULE_1");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveDisplays()), "DISP_1, DISP_3, DISP_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveViews()), 
                                     "SHARED_1, SHARED_3, VIEW_1, VIEW_3, SHARED_2, VIEW_2");

        // Validate default_view_transform
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDefaultViewTransformName()), "Un-tone-mapped-2");

        // Validate shared_views
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, nullptr), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 0)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 1)), "SHARED_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 2)), "SHARED_2");

        // Validate displays
        OCIO_CHECK_EQUAL(mergedConfig->getNumDisplaysAll(), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(0)), "DISP_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(1)), "DISP_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(2)), "DISP_2");

        // Validate display/views
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_1"), 1);
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, "DISP_1"), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED,"DISP_1", 0)), "VIEW_1");
        // Make sure this is the right VIEW_1 by checking the colorspace.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "VIEW_1")), "view_1B");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewRule("DISP_1", "VIEW_1")), "RULE_3");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 0)), "SHARED_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_3")), "log_3");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 1)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_1")), "lin_3");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 2)), "SHARED_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_2")), "<USE_DISPLAY_NAME>");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewTransformName("DISP_1", "SHARED_2")), "SDR Video");

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3", 0)), "VIEW_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3", 1)), "VIEW_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewLooks("DISP_3", "VIEW_3")), "look_input");

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2", 0)), "VIEW_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2", 1)), "VIEW_2");

        // Validate view_transforms

        OCIO_CHECK_EQUAL(mergedConfig->getNumViewTransforms(), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(0)), "SDR Video");
        OCIO::ConstTransformRcPtr tf;
        OCIO_CHECK_NO_THROW(tf = mergedConfig->getViewTransform("SDR Video")
                                             ->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));
        auto bi = OCIO_DYNAMIC_POINTER_CAST<const OCIO::BuiltinTransform>(tf);
        OCIO_REQUIRE_ASSERT(bi);
        OCIO_CHECK_EQUAL(std::string(bi->getStyle()), "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-P3lim_1.1");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(1)), "Un-tone-mapped-2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(2)), "Un-tone-mapped");
        
        // Validate viewing_rules
        OCIO::ConstViewingRulesRcPtr rules = mergedConfig->getViewingRules();

        OCIO_CHECK_EQUAL(rules->getNumEntries(), 3);

        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), "RULE_1");
        OCIO_CHECK_EQUAL(rules->getNumColorSpaces(0), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(0, 0)), "sRGB - Texture");

        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), "RULE_3");
        OCIO_CHECK_EQUAL(rules->getNumColorSpaces(1), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1, 0)), "Linear Rec.2020");
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1, 1)), "ACEScct - SomeOtherName");

        OCIO_CHECK_EQUAL(std::string(rules->getName(2)), "RULE_2");
        OCIO_CHECK_EQUAL(rules->getNumEncodings(2), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getEncoding(2, 0)), "scene-linear");

        // Validate virtual_display
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED), 3); 
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_SHARED), 2); 

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 0)), "ACES");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 1)), "Lin");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 2)), "Log");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_SHARED, 0)), "SHARED_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_SHARED, 1)), "SHARED_1");
    } 

    // Test display/views with strategy=PreferInput, options InputFirst = false.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
        merger->getParams(0)->setInputFirst(false);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
           [&options]() { OCIO::DisplayViewMerger(options).merge(); },
            "The Input config contains a value that would override the Base config: shared_views: SHARED_1",
            "The Input config contains a value that would override the Base config: display: DISP_1, view: VIEW_1",
            "The Input config contains a value that would override the Base config: default_view_transform: Un-tone-mapped-2",
            "The Input config contains a value that would override the Base config: viewing_rules: RULE_1");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveDisplays()), "DISP_1, DISP_2, DISP_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveViews()), 
                                     "SHARED_1, SHARED_2, VIEW_1, VIEW_2, SHARED_3, VIEW_3");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDefaultViewTransformName()), "Un-tone-mapped-2");

        // Validate shared_views
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, nullptr), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 0)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 1)), "SHARED_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 2)), "SHARED_3");

        // Validate displays
        OCIO_CHECK_EQUAL(mergedConfig->getNumDisplaysAll(), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(0)), "DISP_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(1)), "DISP_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(2)), "DISP_3");

        // Validate display/views

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_1"), 1);
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, "DISP_1"), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED,"DISP_1", 0)), "VIEW_1");
        // Make sure this is the right VIEW_1 by checking the colorspace.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "VIEW_1")), "view_1B");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewRule("DISP_1", "VIEW_1")), "RULE_3");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 0)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_1")), "lin_3");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 1)), "SHARED_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_2")), "<USE_DISPLAY_NAME>");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewTransformName("DISP_1", "SHARED_2")), "SDR Video");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 2)), "SHARED_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_3")), "log_3");

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2", 0)), "VIEW_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2", 1)), "VIEW_2");

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3", 0)), "VIEW_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3", 1)), "VIEW_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewLooks("DISP_3", "VIEW_3")), "look_input");
        
        // Validate view_transforms

        OCIO_CHECK_EQUAL(mergedConfig->getNumViewTransforms(), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(0)), std::string("SDR Video"));
        OCIO::ConstTransformRcPtr tf;
        OCIO_CHECK_NO_THROW(tf = mergedConfig->getViewTransform("SDR Video")
                                             ->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));
        auto bi = OCIO_DYNAMIC_POINTER_CAST<const OCIO::BuiltinTransform>(tf);
        OCIO_REQUIRE_ASSERT(bi);
        OCIO_CHECK_EQUAL(std::string(bi->getStyle()), "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-P3lim_1.1");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(1)), std::string("Un-tone-mapped"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(2)), std::string("Un-tone-mapped-2"));

        // Validate viewing_rules
        OCIO::ConstViewingRulesRcPtr rules = mergedConfig->getViewingRules();

        OCIO_CHECK_EQUAL(rules->getNumEntries(), 3);

        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), "RULE_1");
        OCIO_CHECK_EQUAL(rules->getNumColorSpaces(0), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(0, 0)), "sRGB - Texture");

        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), "RULE_2");
        OCIO_CHECK_EQUAL(rules->getNumEncodings(1), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getEncoding(1, 0)), "scene-linear");

        OCIO_CHECK_EQUAL(std::string(rules->getName(2)), "RULE_3");
        OCIO_CHECK_EQUAL(rules->getNumColorSpaces(2), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(2, 0)), "Linear Rec.2020");
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(2, 1)), "ACEScct - SomeOtherName");

        // Validate virtual_display
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED), 3); 
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_SHARED), 2); 

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 0)), "ACES");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 1)), "Log");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 2)), "Lin");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_SHARED, 0)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_SHARED, 1)), "SHARED_3");
    }

    // Test display/views with strategy = PreferBase, options InputFirst = true.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
        merger->getParams(0)->setInputFirst(true);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
           [&options]() { OCIO::DisplayViewMerger(options).merge(); },
            "The Input config contains a value that would override the Base config: shared_views: SHARED_1",
            "The Input config contains a value that would override the Base config: display: DISP_1, view: VIEW_1",
            "The Input config contains a value that would override the Base config: default_view_transform: Un-tone-mapped-2",
            "The Input config contains a value that would override the Base config: viewing_rules: RULE_1");
        
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveDisplays()), "DISP_1, DISP_3, DISP_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveViews()), 
                                     "SHARED_1, SHARED_3, VIEW_1, VIEW_3, SHARED_2, VIEW_2");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDefaultViewTransformName()), "SDR Video");

        // Validate shared_views
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, nullptr), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 0)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 1)), "SHARED_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 2)), "SHARED_2");

        // Validate displays
        OCIO_CHECK_EQUAL(mergedConfig->getNumDisplaysAll(), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(0)), "DISP_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(1)), "DISP_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(2)), "DISP_2");

        // Validate display/views

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_1"), 1);
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, "DISP_1"), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED,"DISP_1", 0)), "VIEW_1");
        // Make sure this is the right VIEW_1 by checking the colorspace.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "VIEW_1")), "view_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewRule("DISP_1", "VIEW_1")), "RULE_1");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 0)), "SHARED_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_3")), "log_3");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 1)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_1")), "lin_1");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 2)), "SHARED_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_2")), "<USE_DISPLAY_NAME>");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewTransformName("DISP_1", "SHARED_2")), "SDR Video");

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3", 0)), "VIEW_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3", 1)), "VIEW_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewLooks("DISP_3", "VIEW_3")), "look_input");

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2", 0)), "VIEW_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewRule("DISP_2", "VIEW_1")), "RULE_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2", 1)), "VIEW_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewLooks("DISP_2", "VIEW_2")), "look_base");

        // Validate view_transforms

        OCIO_CHECK_EQUAL(mergedConfig->getNumViewTransforms(), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(0)), "SDR Video");
        OCIO::ConstTransformRcPtr tf;
        OCIO_CHECK_NO_THROW(tf = mergedConfig->getViewTransform("SDR Video")
                                             ->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));
        auto bi = OCIO_DYNAMIC_POINTER_CAST<const OCIO::BuiltinTransform>(tf);
        OCIO_REQUIRE_ASSERT(bi);
        OCIO_CHECK_EQUAL(std::string(bi->getStyle()), "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO_1.0");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(1)), "Un-tone-mapped-2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(2)), "Un-tone-mapped");

        // Validate viewing_rules
        OCIO::ConstViewingRulesRcPtr rules = mergedConfig->getViewingRules();

        OCIO_CHECK_EQUAL(rules->getNumEntries(), 3);

        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), "RULE_1");
        OCIO_CHECK_EQUAL(rules->getNumColorSpaces(0), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(0, 0)), "Gamma 2.2 AP1 - Texture");

        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), "RULE_3");
        OCIO_CHECK_EQUAL(rules->getNumColorSpaces(1), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1, 0)), "Linear Rec.2020");
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1, 1)), "ACEScct - SomeOtherName");

        OCIO_CHECK_EQUAL(std::string(rules->getName(2)), "RULE_2");
        OCIO_CHECK_EQUAL(rules->getNumEncodings(2), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getEncoding(2, 0)), "scene-linear");

        // Validate virtual_display
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED), 3); 
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_SHARED), 2); 

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 0)), "ACES");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 1)), "Lin");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 2)), "Log");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_SHARED, 0)), "SHARED_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_SHARED, 1)), "SHARED_1");
    }

    // Test display/views with strategy = PreferBase, options InputFirst = false.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
        merger->getParams(0)->setInputFirst(false);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
           [&options]() { OCIO::DisplayViewMerger(options).merge(); },
            "The Input config contains a value that would override the Base config: shared_views: SHARED_1",
            "The Input config contains a value that would override the Base config: display: DISP_1, view: VIEW_1",
            "The Input config contains a value that would override the Base config: default_view_transform: Un-tone-mapped-2",
            "The Input config contains a value that would override the Base config: viewing_rules: RULE_1");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveDisplays()), "DISP_1, DISP_2, DISP_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveViews()), 
                                     "SHARED_1, SHARED_2, VIEW_1, VIEW_2, SHARED_3, VIEW_3");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDefaultViewTransformName()), "SDR Video");

        // Validate shared_views
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, nullptr), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 0)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 1)), "SHARED_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 2)), "SHARED_3");

        // Validate displays
        OCIO_CHECK_EQUAL(mergedConfig->getNumDisplaysAll(), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(0)), "DISP_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(1)), "DISP_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(2)), "DISP_3");

        // Validate display/views

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_1"), 1);
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, "DISP_1"), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED,"DISP_1", 0)), "VIEW_1");
        // Make sure this is the right VIEW_1 by checking the colorspace.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "VIEW_1")), "view_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewRule("DISP_1", "VIEW_1")), "RULE_1");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 0)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_1")), "lin_1");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 1)), "SHARED_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_2")), "<USE_DISPLAY_NAME>");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewTransformName("DISP_1", "SHARED_2")), "SDR Video");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 2)), "SHARED_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_3")), "log_3");

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2", 0)), "VIEW_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewRule("DISP_2", "VIEW_1")), "RULE_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2", 1)), "VIEW_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewLooks("DISP_2", "VIEW_2")), "look_base");

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3", 0)), "VIEW_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3", 1)), "VIEW_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewLooks("DISP_3", "VIEW_3")), "look_input");

        // Validate view_transforms

        OCIO_CHECK_EQUAL(mergedConfig->getNumViewTransforms(), 3);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(0)), "SDR Video");
        OCIO::ConstTransformRcPtr tf;
        OCIO_CHECK_NO_THROW(tf = mergedConfig->getViewTransform("SDR Video")
                                             ->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));
        auto bi = OCIO_DYNAMIC_POINTER_CAST<const OCIO::BuiltinTransform>(tf);
        OCIO_REQUIRE_ASSERT(bi);
        OCIO_CHECK_EQUAL(std::string(bi->getStyle()), "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO_1.0");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(1)), "Un-tone-mapped");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(2)), "Un-tone-mapped-2");

       // Validate viewing_rules
        OCIO::ConstViewingRulesRcPtr rules = mergedConfig->getViewingRules();

        OCIO_CHECK_EQUAL(rules->getNumEntries(), 3);

        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), "RULE_1");
        OCIO_CHECK_EQUAL(rules->getNumColorSpaces(0), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(0, 0)), "Gamma 2.2 AP1 - Texture");

        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), "RULE_2");
        OCIO_CHECK_EQUAL(rules->getNumEncodings(1), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getEncoding(1, 0)), "scene-linear");

        OCIO_CHECK_EQUAL(std::string(rules->getName(2)), "RULE_3");
        OCIO_CHECK_EQUAL(rules->getNumColorSpaces(2), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(2, 0)), "Linear Rec.2020");
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(2, 1)), "ACEScct - SomeOtherName");

        // Validate virtual_display
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED), 3); 
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_SHARED), 2); 

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 0)), "ACES");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 1)), "Log");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 2)), "Lin");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_SHARED, 0)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_SHARED, 1)), "SHARED_3");
    }

    // Test display/views with strategy = BaseOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_BASE_ONLY);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::DisplayViewMerger(options).merge();

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveDisplays()), "DISP_1, DISP_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveViews()), 
                                     "SHARED_1, SHARED_2, VIEW_1, VIEW_2");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDefaultViewTransformName()), "SDR Video");

        // Validate shared_views
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, nullptr), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 0)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 1)), "SHARED_2");

        // Validate displays
        OCIO_CHECK_EQUAL(mergedConfig->getNumDisplaysAll(), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(0)), "DISP_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(1)), "DISP_2");

        // Validate display/views

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_1"), 1);
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, "DISP_1"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED,"DISP_1", 0)), "VIEW_1");
        // Make sure this is the right VIEW_1 by checking the colorspace.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "VIEW_1")), "view_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewRule("DISP_1", "VIEW_1")), "RULE_1");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 0)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_1")), "lin_1");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 1)), "SHARED_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_2")), "<USE_DISPLAY_NAME>");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewTransformName("DISP_1", "SHARED_2")), "SDR Video");

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2", 0)), "VIEW_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewRule("DISP_2", "VIEW_1")), "RULE_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2", 1)), "VIEW_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewLooks("DISP_2", "VIEW_2")), "look_base");

        // Validate view_transforms
        OCIO_CHECK_EQUAL(mergedConfig->getNumViewTransforms(), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(0)), "SDR Video");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(1)), "Un-tone-mapped");

       // Validate viewing_rules
        OCIO::ConstViewingRulesRcPtr rules = mergedConfig->getViewingRules();

        OCIO_CHECK_EQUAL(rules->getNumEntries(), 2);

        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), "RULE_1");
        OCIO_CHECK_EQUAL(rules->getNumColorSpaces(0), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(0, 0)), "Gamma 2.2 AP1 - Texture");

        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), std::string("RULE_2"));
        OCIO_CHECK_EQUAL(rules->getNumEncodings(1), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getEncoding(1, 0)), "scene-linear");

        // Validate virtual_display
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED), 2); 
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_SHARED), 1); 

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 0)), "ACES");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 1)), "Log");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_SHARED, 0)), "SHARED_1");
    }

    // Test display/views with strategy = InputOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_INPUT_ONLY);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::DisplayViewMerger(options).merge();

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveDisplays()), "DISP_1, DISP_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveViews()), 
                                     "SHARED_1, SHARED_3, VIEW_1, VIEW_3");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDefaultViewTransformName()), "Un-tone-mapped-2");

        // Validate shared_views
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, nullptr), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 0)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 1)), "SHARED_3");

        // Validate displays
        OCIO_CHECK_EQUAL(mergedConfig->getNumDisplaysAll(), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(0)), "DISP_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplay(1)), "DISP_3");

        // Validate display/views

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_1"), 1);
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, "DISP_1"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED,"DISP_1", 0)), "VIEW_1");
        // Make sure this is the right VIEW_1 by checking the colorspace.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "VIEW_1")), "view_1B");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewRule("DISP_1", "VIEW_1")), "RULE_3");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 0)), "SHARED_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_3")), "log_3");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 1)), "SHARED_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_1")), "lin_3");

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3", 0)), "VIEW_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_3", 1)), "VIEW_3");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewLooks("DISP_3", "VIEW_3")), "look_input");

        // Validate view_transforms

        OCIO_CHECK_EQUAL(mergedConfig->getNumViewTransforms(), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(0)), "SDR Video");
        OCIO::ConstTransformRcPtr tf;
        OCIO_CHECK_NO_THROW(tf = mergedConfig->getViewTransform("SDR Video")
                                             ->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));
        auto bi = OCIO_DYNAMIC_POINTER_CAST<const OCIO::BuiltinTransform>(tf);
        OCIO_REQUIRE_ASSERT(bi);
        OCIO_CHECK_EQUAL(std::string(bi->getStyle()), "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-P3lim_1.1");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(1)), "Un-tone-mapped-2");

        // Validate viewing_rules
        OCIO::ConstViewingRulesRcPtr rules = mergedConfig->getViewingRules();

        OCIO_CHECK_EQUAL(rules->getNumEntries(), 2);

        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), "RULE_1");
        OCIO_CHECK_EQUAL(rules->getNumColorSpaces(0), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(0, 0)), "sRGB - Texture");

        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), "RULE_3");
        OCIO_CHECK_EQUAL(rules->getNumColorSpaces(1), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1, 0)), "Linear Rec.2020");
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1, 1)), "ACEScct - SomeOtherName");

        // Validate virtual_display
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED), 2); 
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_SHARED), 1); 

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 0)), "ACES");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 1)), "Lin");

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getVirtualDisplayView(OCIO::VIEW_SHARED, 0)), "SHARED_3");
    }

    // Test display/views with strategy = Remove
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_REMOVE);
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::DisplayViewMerger(options).merge();

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveDisplays()), "DISP_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getActiveViews()), "SHARED_2, VIEW_2");

        // Note that the "SDR Video" view transform was removed, so the "SDR Video" value
        // of the default view transform was reset to empty (will use the first one by default).
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDefaultViewTransformName()), "");

        // Validate shared_views
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, nullptr), 1);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED, nullptr, 0)), "SHARED_2");

        // Validate displays
        OCIO_CHECK_EQUAL(mergedConfig->getNumDisplaysAll(), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayAll(0)), "DISP_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayAll(1)), "DISP_2");

        // Validate display/views
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_1"), 0);
        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_SHARED, "DISP_1"), 1);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_SHARED,"DISP_1", 0)), "SHARED_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewColorSpaceName("DISP_1", "SHARED_2")), "<USE_DISPLAY_NAME>");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewTransformName("DISP_1", "SHARED_2")), "SDR Video");

        OCIO_CHECK_EQUAL(mergedConfig->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2"), 2);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2", 0)), "VIEW_1");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewRule("DISP_2", "VIEW_1")), "RULE_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getView(OCIO::VIEW_DISPLAY_DEFINED, "DISP_2", 1)), "VIEW_2");
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getDisplayViewLooks("DISP_2", "VIEW_2")), "look_base");

        // Validate view_transforms
        OCIO_CHECK_EQUAL(mergedConfig->getNumViewTransforms(), 1);
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(0)), std::string("Un-tone-mapped"));

        // Validate viewing_rules
        OCIO::ConstViewingRulesRcPtr rules = mergedConfig->getViewingRules();
        OCIO_CHECK_EQUAL(rules->getNumEntries(), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), std::string("RULE_2"));
        OCIO_CHECK_EQUAL(rules->getNumEncodings(0), 1);
        OCIO_CHECK_EQUAL(std::string(rules->getEncoding(0, 0)), "scene-linear");

        // Validate virtual_display
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED), 1); 
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayNumViews(OCIO::VIEW_SHARED), 1); 

        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 0), std::string("Log"));
        OCIO_CHECK_EQUAL(mergedConfig->getVirtualDisplayView(OCIO::VIEW_SHARED, 0), std::string("SHARED_1"));
    }

    // Test that error_on_conflicts is processed correctly.
    // strategy = PreferInput, InputFirst = false
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();

        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        merger->addParams(params);
        merger->getParams(0)->setDisplayViews(MergeStrategy::STRATEGY_PREFER_INPUT);
        merger->getParams(0)->setInputFirst(false);
        merger->getParams(0)->setErrorOnConflict(true);

        // Test that an error is thrown when the input config's COLORSPACE is different
        {
            OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
            checkForLogOrException(LOG_TYPE_ERROR, __LINE__,
                                   [&options]() { OCIO::DisplayViewMerger(options).merge(); },
                                   std::string(PREFIX) + std::string("shared_views: SHARED_1"),
                                   std::string(PREFIX) + std::string("display: DISP_1, views: VIEW_1"),
                                   std::string(PREFIX) + std::string("default_view_transform: SDR Video"),
                                   std::string(PREFIX) + std::string("view_transforms: SDR Video"),
                                   std::string(PREFIX) + std::string("viewing_rules: RULE_1"),
                                   std::string(PREFIX) + std::string("virtual_display: ACES"));
        }
    }
}

OCIO_ADD_TEST(MergeConfigs, colorspaces_section)
{
    OCIO::ConstConfigRcPtr baseConfig;
    OCIO_CHECK_NO_THROW(baseConfig = getConfig("base_colorspaces_config.yaml"));

    OCIO::ConstConfigRcPtr inputConfig;
    OCIO_CHECK_NO_THROW(inputConfig = getConfig("input_colorspaces_config.yaml"));

    auto setupBasics = [](OCIO::ConfigMergerRcPtr & merger, MergeStrategy strategy) -> OCIO::ConfigMergingParametersRcPtr
    {
        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        merger->addParams(params);
        merger->getParams(0)->setColorspaces(strategy);
        merger->getParams(0)->setDefaultStrategy(strategy);

        merger->getParams(0)->setInputFamilyPrefix("Input/");
        merger->getParams(0)->setBaseFamilyPrefix("Base/");

        merger->getParams(0)->setAssumeCommonReferenceSpace(true);
        merger->getParams(0)->setAvoidDuplicates(false);

        return params;
    };

    // Test that the default strategy is used as a fallback if the section strategy was not defined.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();

        // Using STRATEGY_UNSET as this simulates that the section is missing from the OCIOM file.
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_UNSET);
        // Simulate settings from OCIOM file.
        merger->getParams(0)->setDefaultStrategy(MergeStrategy::STRATEGY_INPUT_ONLY);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::ColorspacesMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->getFamilySeparator(), '~');

        std::vector<std::string> expectedNames = { "test", "test3" };
        std::vector<std::string> expectedValues = { "differentValue", "value3" };
        compareEnvironmentVar(mergedConfig, expectedNames, expectedValues, __LINE__);

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getSearchPath()), std::string(".:def"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getInactiveColorSpaces()), "ACES2065-1, sRGB - Display");
    }

    // Test Colorspaces with strategy = PreferInput, options InputFirst = true.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                "Color space 'sRGB - Display' will replace a color space in the base config",
                                "Color space 'look' will replace a color space in the base config",
                                "Merged color space 'look' has a different reference space type than the color space it's replacing",
                                "Merged color space 'ACES2065-1' has a conflict with alias 'aces' in color space 'ACEScg'",
                                "The name of merged color space 'sRGB' has a conflict with an alias in color space 'sRGB - Texture'");

        OCIO_CHECK_EQUAL(mergedConfig->getFamilySeparator(), '~');

        // Note that the environment vars are always written in alphabetical order,
        // so the InputFirst directive doesn't apply to this specific element.
        std::vector<std::string> expectedNames = { "test", "test1", "test3" };
        std::vector<std::string> expectedValues = { "differentValue", "value1", "value3" };
        compareEnvironmentVar(mergedConfig, expectedNames, expectedValues, __LINE__);

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getSearchPath()), std::string(".:def:abc"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getInactiveColorSpaces()), "ACES2065-1, sRGB - Display, sRGB - Texture, ACEScg");
        
        OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 6);
        OCIO::ConstColorSpaceRcPtr cs = nullptr;

        // Display colorspaces.

        cs = checkColorSpace(mergedConfig, "sRGB - Display", 0, OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("srgb_display"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Input~Display~Standard"));
        OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from input"));

        cs = checkColorSpace(mergedConfig, "look", 1, OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("look1"));
        OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from input"));

        // Scene colorspaces.

        cs = checkColorSpace(mergedConfig, "ACES2065-1", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("aces"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Input~ACES~Linear"));

        cs = checkColorSpace(mergedConfig, "sRGB", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("my_srgb"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Input~Texture~"));

        cs = checkColorSpace(mergedConfig, "ACEScg", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Base~ACES~Linear"));
        
        cs = checkColorSpace(mergedConfig, "sRGB - Texture", 3, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        // Note "srgb" is removed as an alias since it is a color space name in the input config.
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("srgb_tx"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Base~Texture"));
        OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from base"));

        // Note that the "look" scene color space is not merged since there is already a display color space 
        // with that name.
    }

    // Test Colorspaces with strategy=PreferInput, options InputFirst = false.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
        merger->getParams(0)->setInputFirst(false);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                "Color space 'sRGB - Display' will replace a color space in the base config",
                                "Color space 'look' will replace a color space in the base config",
                                "Merged color space 'look' has a different reference space type than the color space it's replacing",
                                "Merged color space 'ACES2065-1' has a conflict with alias 'aces' in color space 'ACEScg'",
                                "The name of merged color space 'sRGB' has a conflict with an alias in color space 'sRGB - Texture'");

        OCIO_CHECK_EQUAL(mergedConfig->getFamilySeparator(), '~');

        std::vector<std::string> expectedNames = { "test", "test1", "test3" };
        std::vector<std::string> expectedValues = { "differentValue", "value1", "value3" };
        compareEnvironmentVar(mergedConfig, expectedNames, expectedValues, __LINE__);

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getSearchPath()), std::string(".:abc:def"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getInactiveColorSpaces()), "sRGB - Texture, sRGB - Display, ACEScg, ACES2065-1");

        OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 6);
        OCIO::ConstColorSpaceRcPtr cs = nullptr;

        // Display colorspaces.

        cs = checkColorSpace(mergedConfig, "sRGB - Display", 0, OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, __LINE__);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("srgb_display"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Input~Display~Standard"));
        OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from input"));

        cs = checkColorSpace(mergedConfig, "look", 1, OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, __LINE__);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("look1"));
        OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from input"));

        // Scene colorspaces.

        cs = checkColorSpace(mergedConfig, "ACEScg", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Base~ACES~Linear"));

        cs = checkColorSpace(mergedConfig, "sRGB - Texture", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("srgb_tx"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Base~Texture"));
        OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from base"));

        cs = checkColorSpace(mergedConfig, "ACES2065-1", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("aces"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Input~ACES~Linear"));

        cs = checkColorSpace(mergedConfig, "sRGB", 3, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("my_srgb"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Input~Texture~"));
    }

    // Test Colorspaces with strategy = PreferBase, options InputFirst = true.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                "Color space 'sRGB - Display' was not merged as it's already present in the base config",
                                "Color space 'look' was not merged as it's already present in the base config",
                                "Merged color space 'ACES2065-1' has a conflict with alias 'aces' in color space 'ACEScg'",
                                "Color space 'sRGB' was not merged as it conflicts with an alias in color space 'sRGB - Texture'");
      
        OCIO_CHECK_EQUAL(mergedConfig->getFamilySeparator(), '#');  

        std::vector<std::string> expectedNames = { "test", "test1", "test3" };
        std::vector<std::string> expectedValues = { "value", "value1", "value3" };
        compareEnvironmentVar(mergedConfig, expectedNames, expectedValues, __LINE__);

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getSearchPath()), std::string(".:def:abc"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getInactiveColorSpaces()), "ACES2065-1, sRGB - Display, sRGB - Texture, ACEScg");

        OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 5);
        OCIO::ConstColorSpaceRcPtr cs = nullptr;

        // Display colorspaces.

        cs = checkColorSpace(mergedConfig, "sRGB - Display", 0, OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, __LINE__);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("srgb_display"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Base#Display#Basic"));
        OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from base"));

        // Scene colorspaces.

        cs = checkColorSpace(mergedConfig, "ACES2065-1", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Input#ACES#Linear"));
        OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from input"));

        cs = checkColorSpace(mergedConfig, "ACEScg", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("aces"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Base#ACES#Linear"));

        cs = checkColorSpace(mergedConfig, "sRGB - Texture", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 2);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("srgb"));
        OCIO_CHECK_EQUAL(cs->getAlias(1), std::string("srgb_tx"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Base#Texture"));
        OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from base"));

        cs = checkColorSpace(mergedConfig, "look", 3, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
        OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from base"));
    }

    // Test Colorspaces with strategy = PreferBase, options InputFirst = false.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
        merger->getParams(0)->setInputFirst(false);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                "Color space 'sRGB - Display' was not merged as it's already present in the base config",
                                "Color space 'look' was not merged as it's already present in the base config",
                                "Merged color space 'ACES2065-1' has a conflict with alias 'aces' in color space 'ACEScg'",
                                "Color space 'sRGB' was not merged as it conflicts with an alias in color space 'sRGB - Texture'");

        OCIO_CHECK_EQUAL(mergedConfig->getFamilySeparator(), '#');

        std::vector<std::string> expectedNames = { "test", "test1", "test3" };
        std::vector<std::string> expectedValues = { "value", "value1", "value3" };
        compareEnvironmentVar(mergedConfig, expectedNames, expectedValues, __LINE__);

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getSearchPath()), std::string(".:abc:def"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getInactiveColorSpaces()), "sRGB - Texture, sRGB - Display, ACEScg, ACES2065-1");

        OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 5);
        OCIO::ConstColorSpaceRcPtr cs = nullptr;

        // Display colorspaces.

        cs = checkColorSpace(mergedConfig, "sRGB - Display", 0, OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, __LINE__);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("srgb_display"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Base#Display#Basic"));
        OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from base"));

        // Scene colorspaces.

        cs = checkColorSpace(mergedConfig, "ACEScg", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("aces"));

        cs = checkColorSpace(mergedConfig, "sRGB - Texture", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 2);
        OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("srgb"));
        OCIO_CHECK_EQUAL(cs->getAlias(1), std::string("srgb_tx"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Base#Texture"));

        cs = checkColorSpace(mergedConfig, "look", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
        OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from base"));

        cs = checkColorSpace(mergedConfig, "ACES2065-1", 3, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
    }

    // Test Colorspaces with strategy = BaseOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_BASE_ONLY);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::ColorspacesMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->getFamilySeparator(), '#');  

        std::vector<std::string> expectedNames = { "test", "test1" };
        std::vector<std::string> expectedValues = { "value", "value1" };
        compareEnvironmentVar(mergedConfig, expectedNames, expectedValues, __LINE__);

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getSearchPath()), std::string(".:abc"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getInactiveColorSpaces()), "sRGB - Texture, sRGB - Display, ACEScg");


        OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, 
                                                         OCIO::COLORSPACE_ALL), 4);
        OCIO::ConstColorSpaceRcPtr cs = nullptr;

        cs = checkColorSpace(mergedConfig, "sRGB - Display", 0, OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, __LINE__);
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Display#Basic"));
    
        cs = checkColorSpace(mergedConfig, "ACEScg", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("ACES#Linear"));

        cs = checkColorSpace(mergedConfig, "sRGB - Texture", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Texture"));

        cs = checkColorSpace(mergedConfig, "look", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string(""));
    }

    // Test Colorspaces with strategy = InputOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_INPUT_ONLY);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::ColorspacesMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->getFamilySeparator(), '~');  

        std::vector<std::string> expectedNames = { "test", "test3" };
        std::vector<std::string> expectedValues = { "differentValue", "value3" };
        compareEnvironmentVar(mergedConfig, expectedNames, expectedValues, __LINE__);

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getSearchPath()), std::string(".:def"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getInactiveColorSpaces()), "ACES2065-1, sRGB - Display");

        OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, 
                                                         OCIO::COLORSPACE_ALL), 4);
        OCIO::ConstColorSpaceRcPtr cs = nullptr;

        cs = checkColorSpace(mergedConfig, "sRGB - Display", 0, OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, __LINE__);
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Display~Standard"));

        cs = checkColorSpace(mergedConfig, "look", 1, OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, __LINE__);
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Display~Standard"));

        cs = checkColorSpace(mergedConfig, "ACES2065-1", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("ACES~Linear"));

        cs = checkColorSpace(mergedConfig, "sRGB", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Texture~"));
    }

    // Test Colorspaces with strategy = Remove
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_REMOVE);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::ColorspacesMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->getFamilySeparator(), '#');

        std::vector<std::string> expectedNames = { "test1" };
        std::vector<std::string> expectedValues = { "value1" };
        compareEnvironmentVar(mergedConfig, expectedNames, expectedValues, __LINE__);

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getSearchPath()), std::string("abc"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getInactiveColorSpaces()), "sRGB - Texture, ACEScg");

        OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, 
                                                         OCIO::COLORSPACE_ALL), 2);
        OCIO::ConstColorSpaceRcPtr cs = nullptr;

        cs = checkColorSpace(mergedConfig, "ACEScg", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("ACES#Linear"));

        cs = checkColorSpace(mergedConfig, "sRGB - Texture", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Texture"));
    }
}

OCIO_ADD_TEST(MergeConfigs, colorspaces_section_common_reference_and_duplicates)
{
    // Base config display ref space: CIE-XYZ-D65, scene ref space: ACES2065-1.
    // Input config display ref space: linear Rec.709, scene ref space: linear Rec.709
    //
    // Both configs have the role: cie_xyz_d65_interchange: CIE-XYZ-D65
    // but not the aces_interchange role, so heuristics will be used for that.
    //
    // The merged configs will contain color spaces from the input config where
    // the reference space has been converted to that of the base config.
    // The base reference spaces are always used, regardless of strategy.
    //
    // Duplicates are removed, even though they use different reference spaces.

    std::vector<std::string> pathsBase = { 
        std::string(OCIO::GetTestFilesDir()),
        std::string("configs"),
        std::string("mergeconfigs"),
        std::string("merged1"),
        std::string("base1.ocio")
    }; 
    const std::string basePath = pystring::os::path::normpath(pystring::os::path::join(pathsBase));

    std::vector<std::string> pathsInput = { 
        std::string(OCIO::GetTestFilesDir()),
        std::string("configs"),
        std::string("mergeconfigs"),
        std::string("merged1"),
        std::string("input1.ocio")
    }; 
    const std::string inputPath = pystring::os::path::normpath(pystring::os::path::join(pathsInput));

    OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromFile(basePath.c_str());
    OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromFile(inputPath.c_str());

    auto setupBasics = [](OCIO::ConfigMergerRcPtr & merger, MergeStrategy strategy) -> OCIO::ConfigMergingParametersRcPtr
    {
        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        merger->addParams(params);
        merger->getParams(0)->setColorspaces(strategy);
        merger->getParams(0)->setDefaultStrategy(strategy);

        merger->getParams(0)->setInputFamilyPrefix("Input/");
        merger->getParams(0)->setBaseFamilyPrefix("Base/");

        merger->getParams(0)->setAssumeCommonReferenceSpace(false);
        merger->getParams(0)->setAvoidDuplicates(true);

        return params;
    };

    // PreferInput, Input first.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
        merger->getParams(0)->setInputFirst(true);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                [&options]() { OCIO::RolesMerger(options).merge();
                                               OCIO::DisplayViewMerger(options).merge();
                                               OCIO::ColorspacesMerger(options).merge(); },
//                                "The Input config contains a role that would override Base config role 'aces_interchange'.",
                                "Equivalent input color space 'sRGB - Display' replaces 'sRGB - Display' in the base config, preserving aliases.",
                                "Equivalent input color space 'CIE-XYZ-D65' replaces 'CIE-XYZ-D65' in the base config, preserving aliases.",
                                "Equivalent input color space 'ACES2065-1' replaces 'ap0' in the base config, preserving aliases.",
                                "Equivalent input color space 'sRGB' replaces 'sRGB - Texture' in the base config, preserving aliases.",
                                "Merged color space 'ACES2065-1' has a conflict with alias 'aces' in color space 'ACEScg'");

        OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 1);
//         OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("aces_interchange")),
//                          std::string("ACES2065-1"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("cie_xyz_d65_interchange")),
                         std::string("CIE-XYZ-D65"));

        OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, 
                                                         OCIO::COLORSPACE_ALL), 7);

        // Display-referred spaces.
        {
            auto cs = checkColorSpace(mergedConfig, "sRGB - Display", 0, OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, __LINE__);
            OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
            // Check for alias srgb_display (added from base config).
            OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("srgb_display"));
            OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from input"));

            // Check that the input config reference space was converted to the base reference space.
            // See ConfigUtils_tests.cpp for more detailed testing of the reference space conversion.
            {
                OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE));
                OCIO::ConstTransformRcPtr t = cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE);
                OCIO_REQUIRE_ASSERT(t);
                OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
                auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
                OCIO_REQUIRE_ASSERT(gtx);
                OCIO_REQUIRE_EQUAL(gtx->getNumTransforms(), 2);
                OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
                OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_EXPONENT_WITH_LINEAR);
            }

            auto cs1 = checkColorSpace(mergedConfig, "CIE-XYZ-D65", 1, OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, __LINE__);
            OCIO_CHECK_EQUAL(cs1->getNumAliases(), 1);
            OCIO_CHECK_EQUAL(cs1->getAlias(0), std::string("cie_xyz_d65"));
            {
                OCIO_REQUIRE_ASSERT(!cs1->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE));
                OCIO::ConstTransformRcPtr t = cs1->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE);
                OCIO_REQUIRE_ASSERT(t);
                OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
                auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
                OCIO_REQUIRE_ASSERT(gtx);
                OCIO_REQUIRE_EQUAL(gtx->getNumTransforms(), 2);
                OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
                OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
            }
        }

        // Scene-referred spaces.
        {
            // This is recognized as a duplicate, even though the name is different in the two configs.
            auto cs = checkColorSpace(mergedConfig, "ACES2065-1", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            OCIO_CHECK_EQUAL(cs->getNumAliases(), 2);
            OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("aces"));
            // Check for alias ap0 (added from base config).
            OCIO_CHECK_EQUAL(cs->getAlias(1), std::string("ap0"));
            {
                OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
                OCIO::ConstTransformRcPtr t = cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
                OCIO_REQUIRE_ASSERT(t);
                OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
                auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
                OCIO_REQUIRE_ASSERT(gtx);
                OCIO_REQUIRE_EQUAL(gtx->getNumTransforms(), 2);
                OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
                OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
            }

            // This is recognized as a duplicate, even though the name is different in the two configs.
            auto cs1 = checkColorSpace(mergedConfig, "sRGB", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            OCIO_CHECK_EQUAL(cs1->getNumAliases(), 2);
            // Check for alias sRGB - Texture (added from base config colorspace name).
            OCIO_CHECK_EQUAL(cs1->getAlias(0), std::string("sRGB - Texture"));
            // Check for alias srgb_tx (added from base config).
            OCIO_CHECK_EQUAL(cs1->getAlias(1), std::string("srgb_tx"));
            {
                OCIO_REQUIRE_ASSERT(!cs1->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
                OCIO::ConstTransformRcPtr t = cs1->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
                OCIO_REQUIRE_ASSERT(t);
                OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
                auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
                OCIO_REQUIRE_ASSERT(gtx);
                OCIO_REQUIRE_EQUAL(gtx->getNumTransforms(), 2);
                OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_EXPONENT_WITH_LINEAR);
                OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
            }

            auto cs2 = checkColorSpace(mergedConfig, "rec709", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            {
                OCIO_REQUIRE_ASSERT(!cs2->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
                OCIO::ConstTransformRcPtr t = cs2->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
                OCIO_REQUIRE_ASSERT(t);
                OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
                auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
                OCIO_REQUIRE_ASSERT(gtx);
                OCIO_REQUIRE_EQUAL(gtx->getNumTransforms(), 1);
                OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
            }

            auto cs3 = checkColorSpace(mergedConfig, "Raw", 3, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            OCIO_CHECK_EQUAL(cs3->getNumAliases(), 1);
            OCIO_CHECK_EQUAL(cs3->getAlias(0), std::string("Utility - Raw"));
            OCIO_CHECK_ASSERT(cs3->isData());
            {
                OCIO_REQUIRE_ASSERT(!cs3->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE));
                OCIO_REQUIRE_ASSERT(!cs3->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
            }

            auto cs4 = checkColorSpace(mergedConfig, "ACEScg", 4, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            OCIO_CHECK_EQUAL(cs4->getNumAliases(), 0);
            {
                OCIO_REQUIRE_ASSERT(!cs4->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
                OCIO::ConstTransformRcPtr t = cs4->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
                OCIO_REQUIRE_ASSERT(t);
                OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_BUILTIN);
            }
        }

        // View transforms.
        {
            OCIO_CHECK_EQUAL(mergedConfig->getNumViewTransforms(), 2);
            OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(0)), "SDR Video");
            OCIO_CHECK_EQUAL(mergedConfig->getViewTransform("SDR Video")->getDescription(), std::string("from input"));
            OCIO::ConstTransformRcPtr tf;
            OCIO_CHECK_NO_THROW(tf = mergedConfig->getViewTransform("SDR Video")
                                                 ->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));

            // Validate the reference space conversion was added to the transform from the input config.
            OCIO_CHECK_EQUAL(tf->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
            auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(tf);
            OCIO_REQUIRE_ASSERT(gtx);
            OCIO_REQUIRE_EQUAL(gtx->getNumTransforms(), 3);
            OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
            OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_BUILTIN);
            OCIO_CHECK_EQUAL(gtx->getTransform(2)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);

            OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(1)), "vt2");
            OCIO_CHECK_NO_THROW(tf = mergedConfig->getViewTransform("vt2")
                                                 ->getTransform(OCIO::VIEWTRANSFORM_DIR_TO_REFERENCE));

            // Validate the reference space conversion was not added to the transform from the base config.
            OCIO_CHECK_EQUAL(tf->getTransformType(), OCIO::TRANSFORM_TYPE_EXPONENT_WITH_LINEAR);
        }
    }

    // PreferBase, Input first.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
        merger->getParams(0)->setInputFirst(true);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                [&options]() { OCIO::RolesMerger(options).merge();
                                               OCIO::DisplayViewMerger(options).merge();
                                               OCIO::ColorspacesMerger(options).merge(); },
//                                "The Input config contains a role that would override Base config role 'aces_interchange'.",
                                "Equivalent base color space 'sRGB - Display' overrides 'sRGB - Display' in the input config, preserving aliases.",
                                "Equivalent base color space 'CIE-XYZ-D65' overrides 'CIE-XYZ-D65' in the input config, preserving aliases.",
                                "Equivalent base color space 'ap0' overrides 'ACES2065-1' in the input config, preserving aliases.",
                                "Equivalent base color space 'sRGB - Texture' overrides 'sRGB' in the input config, preserving aliases.",
                                "Input color space 'ACES2065-1' is a duplicate of base color space 'ap0' but was "
                                    "unable to add alias 'aces' since it conflicts with base color space 'ACEScg'.");

        OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 1);
//         OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("aces_interchange")),
//                          std::string("ap0"));
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getRoleColorSpace("cie_xyz_d65_interchange")),
                         std::string("CIE-XYZ-D65"));

        OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, 
                                                         OCIO::COLORSPACE_ALL), 7);

        // Display-referred spaces.
        {
            auto cs = checkColorSpace(mergedConfig, "sRGB - Display", 0, OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, __LINE__);
            OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
            OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("srgb_display"));
            OCIO_CHECK_EQUAL(cs->getDescription(), std::string("from base"));
            {
                OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE));
                OCIO::ConstTransformRcPtr t = cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE);
                OCIO_REQUIRE_ASSERT(t);
                OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_BUILTIN);
            }

            auto cs1 = checkColorSpace(mergedConfig, "CIE-XYZ-D65", 1, OCIO::SEARCH_REFERENCE_SPACE_DISPLAY, __LINE__);
            OCIO_CHECK_EQUAL(cs1->getNumAliases(), 1);
            OCIO_CHECK_EQUAL(cs1->getAlias(0), std::string("cie_xyz_d65"));
            {
                OCIO_REQUIRE_ASSERT(!cs1->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE));
                OCIO_REQUIRE_ASSERT(!cs1->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
            }
        }

        // Scene-referred spaces.
        {
            auto cs = checkColorSpace(mergedConfig, "rec709", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            {
                OCIO_REQUIRE_ASSERT(!cs->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
                OCIO::ConstTransformRcPtr t = cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
                OCIO_REQUIRE_ASSERT(t);
                OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
                auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
                OCIO_REQUIRE_ASSERT(gtx);
                OCIO_REQUIRE_EQUAL(gtx->getNumTransforms(), 1);
                OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
            }

            auto cs1 = checkColorSpace(mergedConfig, "Raw", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            OCIO_CHECK_EQUAL(cs1->getNumAliases(), 1);
            OCIO_CHECK_EQUAL(cs1->getAlias(0), std::string("Utility - Raw"));
            OCIO_CHECK_ASSERT(cs1->isData());
            {
                OCIO_REQUIRE_ASSERT(!cs1->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE));
                OCIO_REQUIRE_ASSERT(!cs1->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
            }

            auto cs2 = checkColorSpace(mergedConfig, "ACEScg", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            OCIO_CHECK_EQUAL(cs2->getNumAliases(), 1);
            OCIO_CHECK_EQUAL(cs2->getAlias(0), std::string("aces"));
            {
                OCIO_REQUIRE_ASSERT(!cs2->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
                OCIO::ConstTransformRcPtr t = cs2->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
                OCIO_REQUIRE_ASSERT(t);
                OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_BUILTIN);
            }

            auto cs3 = checkColorSpace(mergedConfig, "ap0", 3, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            OCIO_CHECK_EQUAL(cs3->getNumAliases(), 1);
            OCIO_CHECK_EQUAL(cs3->getAlias(0), std::string("ACES2065-1"));
            OCIO_CHECK_ASSERT(!(cs3->isData()));
            {
                OCIO_REQUIRE_ASSERT(!cs3->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE));
                OCIO_REQUIRE_ASSERT(!cs3->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE));
            }

            auto cs4 = checkColorSpace(mergedConfig, "sRGB - Texture", 4, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            OCIO_CHECK_EQUAL(cs4->getNumAliases(), 2);
            OCIO_CHECK_EQUAL(cs4->getAlias(0), std::string("srgb"));
            OCIO_CHECK_EQUAL(cs4->getAlias(1), std::string("srgb_tx"));
            {
                OCIO_REQUIRE_ASSERT(!cs4->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE));
                OCIO::ConstTransformRcPtr t = cs4->getTransform(OCIO::COLORSPACE_DIR_FROM_REFERENCE);
                OCIO_REQUIRE_ASSERT(t);
                OCIO_CHECK_EQUAL(t->getTransformType(), OCIO::TRANSFORM_TYPE_GROUP);
                auto gtx = OCIO::DynamicPtrCast<const OCIO::GroupTransform>(t);
                OCIO_REQUIRE_ASSERT(gtx);
                OCIO_REQUIRE_EQUAL(gtx->getNumTransforms(), 2);
                OCIO_CHECK_EQUAL(gtx->getTransform(0)->getTransformType(), OCIO::TRANSFORM_TYPE_MATRIX);
                OCIO_CHECK_EQUAL(gtx->getTransform(1)->getTransformType(), OCIO::TRANSFORM_TYPE_EXPONENT_WITH_LINEAR);
            }
        }

        // View transforms.
        {
            OCIO_CHECK_EQUAL(mergedConfig->getNumViewTransforms(), 2);
            OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(0)), "SDR Video");
            OCIO_CHECK_EQUAL(mergedConfig->getViewTransform("SDR Video")->getDescription(), std::string("from base"));
            OCIO::ConstTransformRcPtr tf;
            OCIO_CHECK_NO_THROW(tf = mergedConfig->getViewTransform("SDR Video")
                                                 ->getTransform(OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));

            // Validate that no reference space conversion was added, since the base transform was used.
            OCIO_CHECK_EQUAL(tf->getTransformType(), OCIO::TRANSFORM_TYPE_BUILTIN);

            OCIO_CHECK_EQUAL(std::string(mergedConfig->getViewTransformNameByIndex(1)), "vt2");
            OCIO_CHECK_NO_THROW(tf = mergedConfig->getViewTransform("vt2")
                                                 ->getTransform(OCIO::VIEWTRANSFORM_DIR_TO_REFERENCE));

            // Validate the reference space conversion was not added to the transform from the base config.
            OCIO_CHECK_EQUAL(tf->getTransformType(), OCIO::TRANSFORM_TYPE_EXPONENT_WITH_LINEAR);
        }
    }

    // Nothing special to test for Input only and Base only.
}

OCIO_ADD_TEST(MergeConfigs, colorspaces_section_errors)
{
    auto setupBasics = [](OCIO::ConfigMergerRcPtr & merger, MergeStrategy strategy) -> OCIO::ConfigMergingParametersRcPtr
    {
        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        merger->addParams(params);
        // Note that these tests run several of the mergers.
        merger->getParams(0)->setRoles(strategy);
        merger->getParams(0)->setColorspaces(strategy);
        merger->getParams(0)->setNamedTransforms(strategy);
        merger->getParams(0)->setDefaultStrategy(strategy);
        
        merger->getParams(0)->setInputFamilyPrefix("Input/");
        merger->getParams(0)->setBaseFamilyPrefix("Base/");

        merger->getParams(0)->setAssumeCommonReferenceSpace(true);
        merger->getParams(0)->setAvoidDuplicates(false);

        return params;
    };

    // Test ADD_CS_ERROR_NAME_IDENTICAL_TO_A_ROLE_NAME
    {
        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: A}

roles:
    b: colorspace_a

colorspaces:
- !<ColorSpace>
    name: colorspace_a
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: B}

colorspaces:
- !<ColorSpace>
    name: B
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);

            // The role takes priority over the inbound colorspace.
            // The conflicting color space should not be added to the merged config (skipped).
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::RolesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                       "Color space 'B' was not merged as it's identical to a role name");

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getRoleName(0), std::string("b"));

                // Colorspace A should not be added to the merged config (skipped)
                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(0), std::string("colorspace_a"));
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(true);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::RolesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                       "Color space 'B' was not merged as it's identical to a role name");

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getRoleName(0), std::string("b"));

                // Colorspace A should not be added to the merged config (skipped)
                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(0), std::string("colorspace_a"));
            }
            // Testing the error message when Error on conflict is enabled.
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);
                merger->getParams(0)->setErrorOnConflict(true);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::RolesMerger(options).merge();
                OCIO_CHECK_THROW_WHAT(OCIO::ColorspacesMerger(options).merge(), 
                                      OCIO::Exception,
                                       "Color space 'B' was not merged as it's identical to a role name");
            } 
        }

        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: A}

colorspaces:
- !<ColorSpace>
    name: A
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: B}

roles:
    A: colorspace_b

colorspaces:
- !<ColorSpace>
    name: colorspace_b
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);

            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::RolesMerger(options).merge(); },
                                       "The Input config contains a role 'a' that would override Base config color space 'A'");
                OCIO::ColorspacesMerger(options).merge();

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(0), std::string("colorspace_b"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(1), std::string("A"));
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::RolesMerger(options).merge(); },
                                       "The Input config contains a role 'a' that would override Base config color space 'A'");
                OCIO::ColorspacesMerger(options).merge();

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(0), std::string("colorspace_b"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(1), std::string("A"));
            }  
        }
    }

    // test ADD_CS_ERROR_NAME_IDENTICAL_TO_NT_NAME_OR_ALIAS
    {
        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
- !<Rule> {name: Default, colorspace: cs_base}

named_transforms:
- !<NamedTransform>
    name: nt_base
    encoding: log
    inverse_transform: !<MatrixTransform> {name: inverse, offset: [-0.2, -0.1, -0.1, 0]}
- !<NamedTransform>
    name: nt_base_extra
    aliases: [nt_base2]
    encoding: log
    inverse_transform: !<MatrixTransform> {name: inverse, offset: [-0.2, -0.1, -0.1, 0]}

colorspaces:
- !<ColorSpace>
    name: cs_base
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
- !<Rule> {name: Default, colorspace: nt_base}

colorspaces:
- !<ColorSpace>
    name: nt_base
- !<ColorSpace>
    name: nt_base2
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'nt_base' was not merged as there's a color space with that name",
                                       "Merged Base named transform 'nt_base_extra' has an alias 'nt_base2' that conflicts with color space 'nt_base2'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);
                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "nt_base_extra", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 3);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(0), std::string("cs_base"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(1), std::string("nt_base"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(2), std::string("nt_base2"));
            }
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'nt_base' was not merged as there's a color space with that name",
                                       "Merged Base named transform 'nt_base_extra' has an alias 'nt_base2' that conflicts with color space 'nt_base2'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);
                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "nt_base_extra", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 3);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(0), std::string("cs_base"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(1), std::string("nt_base"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(2), std::string("nt_base2"));
            }
            // Testing the error message when Error on conflict is enabled.
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();

                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);
                merger->getParams(0)->setErrorOnConflict(true);

                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                OCIO_CHECK_THROW_WHAT(OCIO::NamedTransformsMerger(options).merge(), 
                                      OCIO::Exception,
                                      "Named transform 'nt_base' was not merged as there's a color space with that name");
            } 
        }

        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: nt_input}

colorspaces:
- !<ColorSpace>
    name: nt_input
- !<ColorSpace>
    name: nt_input2
)" };

    constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: cs_input}

named_transforms:
  - !<NamedTransform>
    name: nt_input
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}

  - !<NamedTransform>
    name: nt_input_extra
    aliases: [nt_input2]
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}

colorspaces:
- !<ColorSpace>
    name: cs_input
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(true);

                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'nt_input' was not merged as there's a color space with that name",
                                       "Merged Input named transform 'nt_input_extra' has an alias 'nt_input2' that conflicts with color space 'nt_input2'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);
                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "nt_input_extra", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 3);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(0), std::string("cs_input"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(1), std::string("nt_input"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(2), std::string("nt_input2"));
            }  

            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'nt_input' was not merged as there's a color space with that name",
                                       "Merged Input named transform 'nt_input_extra' has an alias 'nt_input2' that conflicts with color space 'nt_input2'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);
                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "nt_input_extra", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 3);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(0), std::string("nt_input"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(1), std::string("nt_input2"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(2), std::string("cs_input"));
            }  
        }
    }  

    // test ADD_CS_ERROR_NAME_CONTAIN_CTX_VAR_TOKEN
    // Not handled as it can't happen in this context.
    // The config will error out while loading the config (before the merge process).

    // test ADD_CS_ERROR_ALIAS_IDENTICAL_TO_A_ROLE_NAME
    {
        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csBase}

roles:
    role_base: csBase

colorspaces:
- !<ColorSpace>
    name: csBase

)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csInput}

colorspaces:
- !<ColorSpace>
    name: csInput
    aliases: [role_base]
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::RolesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                       "Merged color space 'csInput' has an alias 'role_base' that conflicts with a role");

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getRoleName(0), std::string("role_base"));

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(0), std::string("csBase"));
                const char * name = mergedConfig->getColorSpaceNameByIndex(1);
                OCIO_CHECK_EQUAL(std::string(name), std::string("csInput"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpace(name)->getNumAliases(), 0);
            }
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::RolesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                       "Merged color space 'csInput' has an alias 'role_base' that conflicts with a role");

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getRoleName(0), std::string("role_base"));

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(0), std::string("csBase"));
                const char * name = mergedConfig->getColorSpaceNameByIndex(1);
                OCIO_CHECK_EQUAL(std::string(name), std::string("csInput"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpace(name)->getNumAliases(), 0);
            }
            // Testing the error message when Error on conflict is enabled.
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setErrorOnConflict(true);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::RolesMerger(options).merge();
                OCIO_CHECK_THROW_WHAT(OCIO::ColorspacesMerger(options).merge(), 
                                      OCIO::Exception,
                                       "Merged color space 'csInput' has an alias 'role_base' that conflicts with a role");
            } 
        }

        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csBase}

colorspaces:
- !<ColorSpace>
    name: csBase
    aliases: [role_input]
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csInput}

roles:
    role_input: csInput

colorspaces:
- !<ColorSpace>
    name: csInput
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::RolesMerger(options).merge(); },
                                       "The Input config contains a role 'role_input' that would override an alias of Base config color space 'csBase'");
                OCIO::ColorspacesMerger(options).merge();
                                
                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(0), std::string("csInput"));
                const char * name = mergedConfig->getColorSpaceNameByIndex(1);
                OCIO_CHECK_EQUAL(std::string(name), std::string("csBase"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpace(name)->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpace(name)->getAlias(0), std::string("role_input"));
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::RolesMerger(options).merge(); },
                                       "The Input config contains a role 'role_input' that would override an alias of Base config color space 'csBase'");
                OCIO::ColorspacesMerger(options).merge();

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpaceNameByIndex(0), std::string("csInput"));
                const char * name = mergedConfig->getColorSpaceNameByIndex(1);
                OCIO_CHECK_EQUAL(std::string(name), std::string("csBase"));
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpace(name)->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getColorSpace(name)->getAlias(0), std::string("role_input"));
            }  
        }
    }

    // test ADD_CS_ERROR_ALIAS_IDENTICAL_TO_NT_NAME_OR_ALIAS
    {
        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: A}

named_transforms:
  - !<NamedTransform>
    name: nt_base
    encoding: log
    inverse_transform: !<MatrixTransform> {name: inverse, offset: [-0.2, -0.1, -0.1, 0]}

  - !<NamedTransform>
    name: nt_base_extra
    aliases: [nt_base2]
    encoding: log
    inverse_transform: !<MatrixTransform> {name: inverse, offset: [-0.2, -0.1, -0.1, 0]}
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: cs_input}

colorspaces:
- !<ColorSpace>
    name: cs_input
    aliases: [nt_base]
- !<ColorSpace>
    name: cs_input2
    aliases: [nt_base2]
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'nt_base' was not merged as there's a color space alias with that name",
                                       "Merged Base named transform 'nt_base_extra' has a conflict with alias 'nt_base2' in color space 'cs_input2'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);
                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "nt_base_extra", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "cs_input", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("nt_base"));

                cs = checkColorSpace(mergedConfig, "cs_input2", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("nt_base2"));
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'nt_base' was not merged as there's a color space alias with that name",
                                       "Merged Base named transform 'nt_base_extra' has a conflict with alias 'nt_base2' in color space 'cs_input2'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);
                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "nt_base_extra", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);
                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "cs_input", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("nt_base"));

                cs = checkColorSpace(mergedConfig, "cs_input2", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("nt_base2"));
            }
            // Testing the error message when Error on conflict is enabled.
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setErrorOnConflict(true);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                OCIO_CHECK_THROW_WHAT(OCIO::NamedTransformsMerger(options).merge(), 
                                      OCIO::Exception,
                                      "Named transform 'nt_base' was not merged as there's a color space alias with that name");
            } 
        }

        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: cs_base}

colorspaces:
- !<ColorSpace>
    name: cs_base
    aliases: [nt_input]
- !<ColorSpace>
    name: cs_base2
    aliases: [nt_input2]
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: B}

named_transforms:
  - !<NamedTransform>
    name: nt_input
    encoding: log
    inverse_transform: !<MatrixTransform> {name: inverse, offset: [-0.2, -0.1, -0.1, 0]}
  - !<NamedTransform>
    name: nt_input_extra
    aliases: [nt_input2]
    encoding: log
    inverse_transform: !<MatrixTransform> {name: inverse, offset: [-0.2, -0.1, -0.1, 0]}
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'nt_input' was not merged as there's a color space alias with that name",
                                       "Merged Input named transform 'nt_input_extra' has a conflict with alias 'nt_input2' in color space 'cs_base2'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);
                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "nt_input_extra", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);                

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "cs_base", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("nt_input"));

                cs = checkColorSpace(mergedConfig, "cs_base2", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("nt_input2"));
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'nt_input' was not merged as there's a color space alias with that name",
                                       "Merged Input named transform 'nt_input_extra' has a conflict with alias 'nt_input2' in color space 'cs_base2'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);
                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "nt_input_extra", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);                

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "cs_base", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("nt_input"));

                cs = checkColorSpace(mergedConfig, "cs_base2", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("nt_input2"));
            }  
        }
    }

    // test ADD_CS_ERROR_ALIAS_CONTAIN_CTX_VAR_TOKEN
    // Not handled as it can't happen in this context.
    // The config will error out while loading the config (before the merge process).

    // test ADD_CS_ERROR_ALIAS_IDENTICAL_TO_EXISTING_COLORSPACE_ALIAS
    {
        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: cs_base}

colorspaces:
- !<ColorSpace>
    name: cs_base
    aliases: [my_colorspace]
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: cs_input}

colorspaces:
- !<ColorSpace>
    name: cs_input
    aliases: [my_colorspace]
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);

            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                       "Merged color space 'cs_input' has a conflict with alias 'my_colorspace' in color space 'cs_base'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "cs_base", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);

                cs = checkColorSpace(mergedConfig, "cs_input", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("my_colorspace"));
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                       "Merged color space 'cs_input' has a conflict with alias 'my_colorspace' in color space 'cs_base'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "cs_base", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("my_colorspace"));

                cs = checkColorSpace(mergedConfig, "cs_input", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
            }
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                       "Merged color space 'cs_input' has a conflict with alias 'my_colorspace' in color space 'cs_base'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "cs_input", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("my_colorspace"));

                cs = checkColorSpace(mergedConfig, "cs_base", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                       "Merged color space 'cs_input' has a conflict with alias 'my_colorspace' in color space 'cs_base'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "cs_input", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);

                cs = checkColorSpace(mergedConfig, "cs_base", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("my_colorspace"));
            }
            // Testing the error message when Error on conflict is enabled.
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setErrorOnConflict(true);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO_CHECK_THROW_WHAT(OCIO::ColorspacesMerger(options).merge(), 
                                      OCIO::Exception,
                                      "Merged color space 'cs_input' has a conflict with alias 'my_colorspace' in color space 'cs_base'");
            } 
        }
    }

    // Test ADD_CS_ERROR_NAME_IDENTICAL_TO_EXISTING_COLORSPACE_ALIAS
    {
        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: A}

colorspaces:
- !<ColorSpace>
    name: A
    aliases: [B]
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: B}

colorspaces:
- !<ColorSpace>
    name: B
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                        "The name of merged color space 'B' has a conflict with an alias in color space 'A'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "A", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);

                cs = checkColorSpace(mergedConfig, "B", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                        "Color space 'B' was not merged as it conflicts with an alias in color space 'A'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 1);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "A", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("B"));
            }
            // Testing the error message when Error on conflict is enabled.
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setErrorOnConflict(true);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO_CHECK_THROW_WHAT(OCIO::ColorspacesMerger(options).merge(), 
                                      OCIO::Exception,
                                      "The name of merged color space 'B' has a conflict with an alias in color space 'A'");
            } 
        }

        // ADD_CS_ERROR_ALIAS_IDENTICAL_TO_EXISTING_COLORSPACE_NAME
        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: A}

colorspaces:
- !<ColorSpace>
    name: A
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: B}

colorspaces:
- !<ColorSpace>
    name: B
    aliases: [A]
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                       "Merged color space 'B' has an alias 'A' that conflicts with color space 'A'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 1);

                OCIO::ConstColorSpaceRcPtr cs = checkColorSpace(mergedConfig, "B", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("A"));
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                       "Merged color space 'B' has an alias 'A' that conflicts with color space 'A'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 1);

                OCIO::ConstColorSpaceRcPtr cs = checkColorSpace(mergedConfig, "B", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("A"));
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                       "Merged color space 'B' has an alias 'A' that conflicts with color space 'A'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "B", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);

                cs = checkColorSpace(mergedConfig, "A", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::ColorspacesMerger(options).merge(); },
                                       "Merged color space 'B' has an alias 'A' that conflicts with color space 'A'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "A", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);

                cs = checkColorSpace(mergedConfig, "B", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 0);
            }
            // Testing the error message when Error on conflict is enabled.
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setErrorOnConflict(true);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO_CHECK_THROW_WHAT(OCIO::ColorspacesMerger(options).merge(), 
                                      OCIO::Exception,
                                       "Merged color space 'B' has an alias 'A' that conflicts with color space 'A'");
            } 
        }
    }
}

OCIO_ADD_TEST(MergeConfigs, looks_section)
{
    OCIO::ConstConfigRcPtr baseConfig;
    OCIO_CHECK_NO_THROW(baseConfig = getBaseConfig());

    OCIO::ConstConfigRcPtr inputConfig;
    OCIO_CHECK_NO_THROW(inputConfig = getInputConfig());

    auto setupLooks = []
                      (OCIO::ConstConfigRcPtr baseConfig,
                       OCIO::ConstConfigRcPtr inputConfig,
                       OCIO::ConfigMergerRcPtr & merger,
                       OCIO::ConfigRcPtr mergedConfig,
                       MergeStrategy strategy,
                       std::function<void(OCIO::ConfigMergerRcPtr &)> cb)
    {
        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        merger->addParams(params);
        merger->getParams(0)->setLooks(strategy);
        
        merger->getParams(0)->setInputFamilyPrefix("Input/");
        merger->getParams(0)->setBaseFamilyPrefix("Base/");

        if (cb)
        {
            cb(merger);
        }

        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::LooksMerger(options).merge();
    };


    // Test that the default strategy is used as a fallback if the section strategy was not defined.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        setupLooks(baseConfig, inputConfig, merger, 
                   mergedConfig,
                   // Using STRATEGY_UNSET as this simulate that the section
                   // is missing from the OCIOM file.
                   MergeStrategy::STRATEGY_UNSET,
                   [](OCIO::ConfigMergerRcPtr & merger)
                   {
                       // Simulate settings from OCIOM file.
                       merger->getParams(0)->setDefaultStrategy(MergeStrategy::STRATEGY_INPUT_ONLY);
                   });
        
        OCIO_CHECK_EQUAL(mergedConfig->getNumLooks(), 2);
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(0), std::string("look_both"));
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(1), std::string("look_input"));

        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(0))->getProcessSpace(),
                         std::string("ACEScct - SomeOtherName"));
        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(1))->getProcessSpace(),
                         std::string("log_3"));
    }

    // Test Looks with strategy = PreferInput, options InputFirst = true.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();

        setupLooks(baseConfig, inputConfig, merger, 
                   mergedConfig, 
                   MergeStrategy::STRATEGY_PREFER_INPUT,
                   [](OCIO::ConfigMergerRcPtr & merger)
                   { 
                      merger->getParams(0)->setAssumeCommonReferenceSpace(true);
                   });
                   
        OCIO_CHECK_EQUAL(mergedConfig->getNumLooks(), 3);
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(0), std::string("look_both"));
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(1), std::string("look_input"));
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(2), std::string("look_base"));

        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(0))->getProcessSpace(),
                         std::string("ACEScct - SomeOtherName"));
        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(1))->getProcessSpace(),
                         std::string("log_3"));
        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(2))->getProcessSpace(),
                         std::string("log_1"));                
    }

    // Test Looks with strategy=PreferInput, options InputFirst = false.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();

        setupLooks(baseConfig, inputConfig, merger, 
                   mergedConfig, 
                   MergeStrategy::STRATEGY_PREFER_INPUT,
                   [](OCIO::ConfigMergerRcPtr & merger)
                   { 
                       merger->getParams(0)->setInputFirst(false);
                       merger->getParams(0)->setAssumeCommonReferenceSpace(true);
                   });

        OCIO_CHECK_EQUAL(mergedConfig->getNumLooks(), 3);
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(0), std::string("look_both"));
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(1), std::string("look_base"));
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(2), std::string("look_input"));

        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(0))->getProcessSpace(),
                         std::string("ACEScct - SomeOtherName"));
        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(1))->getProcessSpace(),
                         std::string("log_1"));
        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(2))->getProcessSpace(),
                         std::string("log_3"));
    }

    // Test Looks with strategy = PreferBase, options InputFirst = true.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        setupLooks(baseConfig, inputConfig, merger, 
                   mergedConfig, 
                   MergeStrategy::STRATEGY_PREFER_BASE,
                   [](OCIO::ConfigMergerRcPtr & merger)
                   { 
                       merger->getParams(0)->setInputFirst(true);
                       merger->getParams(0)->setAssumeCommonReferenceSpace(true);
                   });

        OCIO_CHECK_EQUAL(mergedConfig->getNumLooks(), 3);
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(0), std::string("look_both"));
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(1), std::string("look_input"));
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(2), std::string("look_base"));

        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(0))->getProcessSpace(),
                         std::string("ACES2065-1"));
        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(1))->getProcessSpace(),
                         std::string("log_3"));
        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(2))->getProcessSpace(),
                         std::string("log_1"));
    }

    // Test Looks with strategy = PreferBase, options InputFirst = false.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        setupLooks(baseConfig, inputConfig, merger, 
                   mergedConfig, 
                   MergeStrategy::STRATEGY_PREFER_BASE,
                   [](OCIO::ConfigMergerRcPtr & merger)
                   { 
                       merger->getParams(0)->setInputFirst(false);
                       merger->getParams(0)->setAssumeCommonReferenceSpace(true);
                   });

        OCIO_CHECK_EQUAL(mergedConfig->getNumLooks(), 3);
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(0), std::string("look_both"));
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(1), std::string("look_base"));
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(2), std::string("look_input"));

        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(0))->getProcessSpace(),
                         std::string("ACES2065-1"));
        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(1))->getProcessSpace(),
                         std::string("log_1"));
        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(2))->getProcessSpace(),
                         std::string("log_3"));
    }

    // Test Looks with strategy = BaseOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        setupLooks(baseConfig, inputConfig, merger, 
                   mergedConfig, 
                   MergeStrategy::STRATEGY_BASE_ONLY,
                   nullptr);

        OCIO_CHECK_EQUAL(mergedConfig->getNumLooks(), 2);
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(0), std::string("look_both"));
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(1), std::string("look_base"));

        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(0))->getProcessSpace(),
                         std::string("ACES2065-1"));
        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(1))->getProcessSpace(),
                         std::string("log_1"));
    }

    // Test Looks with strategy = InputOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        setupLooks(baseConfig, inputConfig, merger, 
                   mergedConfig, 
                   MergeStrategy::STRATEGY_INPUT_ONLY,
                   nullptr);

        OCIO_CHECK_EQUAL(mergedConfig->getNumLooks(), 2);
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(0), std::string("look_both"));
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(1), std::string("look_input"));

        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(0))->getProcessSpace(),
                         std::string("ACEScct - SomeOtherName"));
        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(1))->getProcessSpace(),
                         std::string("log_3"));
    }

    // Test Looks with strategy = Remove
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        setupLooks(baseConfig, inputConfig, merger, 
                   mergedConfig, 
                   MergeStrategy::STRATEGY_REMOVE,
                   nullptr);

        OCIO_CHECK_EQUAL(mergedConfig->getNumLooks(), 1);
        OCIO_CHECK_EQUAL(mergedConfig->getLookNameByIndex(0), std::string("look_base"));
        OCIO_CHECK_EQUAL(mergedConfig->getLook(mergedConfig->getLookNameByIndex(0))->getProcessSpace(),
                         std::string("log_1"));
    }
}

OCIO_ADD_TEST(MergeConfigs, named_transform_section)
{
    OCIO::ConstConfigRcPtr baseConfig;
    OCIO_CHECK_NO_THROW(baseConfig = getBaseConfig());

    OCIO::ConstConfigRcPtr inputConfig;
    OCIO_CHECK_NO_THROW(inputConfig = getInputConfig());

    auto setupBasics = [](OCIO::ConfigMergerRcPtr & merger, MergeStrategy strategy) -> OCIO::ConfigMergingParametersRcPtr
    {
        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        merger->addParams(params);
        // Note that these tests run several of the mergers.
        // Need to run the color space merger too, since that affects how the named transform
        // merger will work (in terms of avoiding conflicts with color space names).
        merger->getParams(0)->setRoles(strategy);
        merger->getParams(0)->setColorspaces(strategy);
        merger->getParams(0)->setNamedTransforms(strategy);
        merger->getParams(0)->setDefaultStrategy(strategy);

        merger->getParams(0)->setInputFamilyPrefix("Input/");
        merger->getParams(0)->setBaseFamilyPrefix("Base/");

        merger->getParams(0)->setAssumeCommonReferenceSpace(true);
        merger->getParams(0)->setAvoidDuplicates(true);
        merger->getParams(0)->setInputFirst(true);

        return params;
    };

    // Test that the default strategy is used as a fallback if the section strategy was not defined.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        // Using STRATEGY_UNSET as this simulate that the section
        // is missing from the OCIOM file.
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_UNSET);
        // Simulate settings from OCIOM file.
        merger->getParams(0)->setDefaultStrategy(MergeStrategy::STRATEGY_INPUT_ONLY);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::ColorspacesMerger(options).merge();
        OCIO::NamedTransformsMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 3);

        OCIO::ConstNamedTransformRcPtr nt = nullptr;
        nt = checkNamedTransform(mergedConfig, "nt_both", 0, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 2);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("Utility - Raw"));
        OCIO_CHECK_EQUAL(nt->getAlias(1), std::string("nametr"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string(""));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from input"));

        nt = checkNamedTransform(mergedConfig, "nt_input", 1, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 2);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("Raw"));
        OCIO_CHECK_EQUAL(nt->getAlias(1), std::string("in nt"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Raw"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from input"));

        nt = checkNamedTransform(mergedConfig, "view_2", 2, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("g22_ap1"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Raw"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from input"));
    }

    // Test NamedTransform with strategy = PreferInput, options InputFirst = true.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
            [&options]() { OCIO::ColorspacesMerger(options).merge(); },
//                                "Color space 'view_1' will replace a color space in the base config");
            "Equivalent input color space 'ACES2065-1' replaces 'ACES2065-1' in the base config, preserving aliases.",
            "Equivalent input color space 'ACEScct - SomeOtherName' replaces 'ACEScct' in the base config, preserving aliases.",
            "Equivalent input color space 'view_1' replaces 'view_1' in the base config, preserving aliases.",
            "Equivalent input color space 'view_1B' replaces 'view_1' in the base config, preserving aliases.",
            "Equivalent input color space 'view_3' replaces 'view_2' in the base config, preserving aliases.",
            "Equivalent input color space 'log_3' replaces 'log_1' in the base config, preserving aliases.",
            "Equivalent input color space 'lin_3' replaces 'ACES2065-1' in the base config, preserving aliases.");
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
            [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
            "Named transform 'nt_both' will replace a named transform in the base config",
            "Merged Base named transform 'nt_both' has a conflict with alias 'srgb_tx' in color space 'sRGB - Texture'",
            "Merged Base named transform 'nt_base' has an alias 'view_3' that conflicts with color space 'view_3'",
            "Merged Input named transform 'nt_both' has a conflict with alias 'Utility - Raw' in color space 'Raw'",
            "The name of merged named transform 'nt_input' has a conflict with an alias in named transform 'nt_base'",
            "Merged Input named transform 'nt_input' has an alias 'Raw' that conflicts with color space 'Raw'",
//            "Named transform 'view_2' was not merged as there's a color space with that name");
            "Named transform 'view_2' was not merged as there's a color space alias with that name.");

        OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 3);

        OCIO::ConstNamedTransformRcPtr nt = nullptr;
        nt = checkNamedTransform(mergedConfig, "nt_both", 0, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("nametr"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Input@"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from input"));

        nt = checkNamedTransform(mergedConfig, "nt_input", 1, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("in nt"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Input@Raw"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from input"));

        nt = checkNamedTransform(mergedConfig, "nt_base", 2, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Base@nt"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from base"));
    }

    // Test NamedTransform with strategy=PreferInput, options InputFirst = false.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
        merger->getParams(0)->setInputFirst(false);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
            [&options]() { OCIO::ColorspacesMerger(options).merge(); },
//             "Color space 'view_1' will replace a color space in the base config");
            "Equivalent input color space 'ACES2065-1' replaces 'ACES2065-1' in the base config, preserving aliases.",
            "Equivalent input color space 'ACEScct - SomeOtherName' replaces 'ACEScct' in the base config, preserving aliases.",
            "Equivalent input color space 'view_1' replaces 'view_1' in the base config, preserving aliases.",
            "Equivalent input color space 'view_1B' replaces 'view_1' in the base config, preserving aliases.",
            "Equivalent input color space 'view_3' replaces 'view_2' in the base config, preserving aliases.",
            "Equivalent input color space 'log_3' replaces 'log_1' in the base config, preserving aliases.",
            "Equivalent input color space 'lin_3' replaces 'ACES2065-1' in the base config, preserving aliases.");
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
            [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
            "Named transform 'nt_both' will replace a named transform in the base config",
            "Merged Base named transform 'nt_both' has a conflict with alias 'srgb_tx' in color space 'sRGB - Texture'",
            "Merged Base named transform 'nt_base' has an alias 'view_3' that conflicts with color space 'view_3'",
            "Merged Input named transform 'nt_both' has a conflict with alias 'Utility - Raw' in color space 'Raw'",
            "The name of merged named transform 'nt_input' has a conflict with an alias in named transform 'nt_base'",
            "Merged Input named transform 'nt_input' has an alias 'Raw' that conflicts with color space 'Raw'",
//            "Named transform 'view_2' was not merged as there's a color space with that name");
            "Named transform 'view_2' was not merged as there's a color space alias with that name.");

        OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 3);

        OCIO::ConstNamedTransformRcPtr nt = nullptr;
        nt = checkNamedTransform(mergedConfig, "nt_base", 0, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Base@nt"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from base"));

        nt = checkNamedTransform(mergedConfig, "nt_both", 1, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("nametr"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Input@"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from input"));

        nt = checkNamedTransform(mergedConfig, "nt_input", 2, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("in nt"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Input@Raw"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from input"));

        OCIO_CHECK_EQUAL(std::string(mergedConfig->getInactiveColorSpaces()), 
///            "Gamma 2.2 AP1 - Texture, Linear Rec.2020, nt_both, nt_input, view_2");
            "Gamma 2.2 AP1 - Texture, Linear Rec.2020, nt_both, nt_input");
    }

    // Test NamedTransform with strategy = PreferBase, options InputFirst = true.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
            [&options]() { OCIO::ColorspacesMerger(options).merge(); },
//             "Color space 'view_1' was not merged as it's already present in the base config");
            "Equivalent base color space 'ACES2065-1' overrides 'ACES2065-1' in the input config, preserving aliases.",
            "Equivalent base color space 'ACEScct' overrides 'ACEScct - SomeOtherName' in the input config, preserving aliases.",
            "Equivalent base color space 'view_1' overrides 'view_1' in the input config, preserving aliases.",
            "Equivalent base color space 'view_1' overrides 'view_1B' in the input config, preserving aliases.",
            "Equivalent base color space 'view_2' overrides 'view_3' in the input config, preserving aliases.",
            "Equivalent base color space 'log_1' overrides 'log_3' in the input config, preserving aliases.",
            "Equivalent base color space 'ACES2065-1' overrides 'lin_3' in the input config, preserving aliases.");
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
            [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
            "Merged Base named transform 'nt_both' has a conflict with alias 'srgb_tx' in color space 'sRGB - Texture'",
//            "Merged Base named transform 'nt_base' has an alias 'view_3' that conflicts with color space 'view_3'",
            "Merged Base named transform 'nt_base' has a conflict with alias 'view_3' in color space 'view_2'.",
            "Named transform 'nt_both' was not merged as it's already present in the base config",
            "Named transform 'nt_input' was not merged as it conflicts with an alias in named transform 'nt_base'",
            "Named transform 'view_2' was not merged as there's a color space with that name");

        OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 2);

        OCIO::ConstNamedTransformRcPtr nt = nullptr;
        nt = checkNamedTransform(mergedConfig, "nt_both", 0, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("namet2"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Base#"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from base"));

        nt = checkNamedTransform(mergedConfig, "nt_base", 1, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("nt_input"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Base#nt"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from base"));

        // NB: The nt_input is included referring to the alias in the base config, not the input config.
        OCIO_CHECK_EQUAL(std::string(mergedConfig->getInactiveColorSpaces()), "Linear Rec.2020, nt_both, view_2, Gamma 2.2 AP1 - Texture");
    }

    // Test NamedTransform with strategy = PreferBase, options InputFirst = false.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
        merger->getParams(0)->setInputFirst(false);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
            [&options]() { OCIO::ColorspacesMerger(options).merge(); },
//             "Color space 'view_1' was not merged as it's already present in the base config");
            "Equivalent base color space 'ACES2065-1' overrides 'ACES2065-1' in the input config, preserving aliases.",
            "Equivalent base color space 'ACEScct' overrides 'ACEScct - SomeOtherName' in the input config, preserving aliases.",
            "Equivalent base color space 'view_1' overrides 'view_1' in the input config, preserving aliases.",
            "Equivalent base color space 'view_1' overrides 'view_1B' in the input config, preserving aliases.",
            "Equivalent base color space 'view_2' overrides 'view_3' in the input config, preserving aliases.",
            "Equivalent base color space 'log_1' overrides 'log_3' in the input config, preserving aliases.",
            "Equivalent base color space 'ACES2065-1' overrides 'lin_3' in the input config, preserving aliases.");
        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
            [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
            "Merged Base named transform 'nt_both' has a conflict with alias 'srgb_tx' in color space 'sRGB - Texture'",
//            "Merged Base named transform 'nt_base' has an alias 'view_3' that conflicts with color space 'view_3'",
            "Merged Base named transform 'nt_base' has a conflict with alias 'view_3' in color space 'view_2'.",
            "Named transform 'nt_both' was not merged as it's already present in the base config",
            "Named transform 'nt_input' was not merged as it conflicts with an alias in named transform 'nt_base'",
            "Named transform 'view_2' was not merged as there's a color space with that name");

        OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 2);

        OCIO::ConstNamedTransformRcPtr nt = nullptr;
        nt = checkNamedTransform(mergedConfig, "nt_both", 0, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("namet2"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Base#"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from base"));

        nt = checkNamedTransform(mergedConfig, "nt_base", 1, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("nt_input"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Base#nt"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from base"));
    }

    // Test NamedTransform with strategy = BaseOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_BASE_ONLY);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::ColorspacesMerger(options).merge();
        OCIO::NamedTransformsMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 2);

        OCIO::ConstNamedTransformRcPtr nt = nullptr;
        nt = checkNamedTransform(mergedConfig, "nt_both", 0, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 2);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("srgb_tx"));
        OCIO_CHECK_EQUAL(nt->getAlias(1), std::string("namet2"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string(""));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from base"));

        nt = checkNamedTransform(mergedConfig, "nt_base", 1, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 2);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("view_3"));
        OCIO_CHECK_EQUAL(nt->getAlias(1), std::string("nt_input"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("nt"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from base"));
    }

    // Test NamedTransform with strategy = InputOnly.
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_INPUT_ONLY);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::ColorspacesMerger(options).merge();
        OCIO::NamedTransformsMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 3);

        OCIO::ConstNamedTransformRcPtr nt = nullptr;
        nt = checkNamedTransform(mergedConfig, "nt_both", 0, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 2);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("Utility - Raw"));
        OCIO_CHECK_EQUAL(nt->getAlias(1), std::string("nametr"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string(""));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from input"));

        nt = checkNamedTransform(mergedConfig, "nt_input", 1, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 2);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("Raw"));
        OCIO_CHECK_EQUAL(nt->getAlias(1), std::string("in nt"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Raw"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from input"));

        nt = checkNamedTransform(mergedConfig, "view_2", 2, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("g22_ap1"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("Raw"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from input"));
    }

    // Test NamedTransform with strategy = Remove
    {
        OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
        auto params = setupBasics(merger, MergeStrategy::STRATEGY_REMOVE);

        OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
        OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        OCIO::ColorspacesMerger(options).merge();
        OCIO::NamedTransformsMerger(options).merge();

        OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);

        OCIO::ConstNamedTransformRcPtr nt = nullptr;
        nt = checkNamedTransform(mergedConfig, "nt_base", 0, __LINE__);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 2);
        OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("view_3"));
        OCIO_CHECK_EQUAL(nt->getAlias(1), std::string("nt_input"));
        OCIO_CHECK_EQUAL(nt->getFamily(), std::string("nt"));
        OCIO_CHECK_EQUAL(nt->getDescription(), std::string("from base"));
    }
}

OCIO_ADD_TEST(MergeConfigs, named_transform_section_errors)
{
    auto setupBasics = [](OCIO::ConfigMergerRcPtr & merger, MergeStrategy strategy) -> OCIO::ConfigMergingParametersRcPtr
    {
        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        merger->addParams(params);
        // Note that these tests run several of the mergers.
        merger->getParams(0)->setRoles(strategy);
        merger->getParams(0)->setColorspaces(strategy);
        merger->getParams(0)->setNamedTransforms(strategy);
        merger->getParams(0)->setDefaultStrategy(strategy);
        
        merger->getParams(0)->setInputFamilyPrefix("Input/");
        merger->getParams(0)->setBaseFamilyPrefix("Base/");

        merger->getParams(0)->setAssumeCommonReferenceSpace(true);
        merger->getParams(0)->setAvoidDuplicates(false);

        return params;
    };

    // Test ADD_NT_ERROR_AT_LEAST_ONE_TRANSFORM,
    {

    }

    // Test ADD_NT_ERROR_NAME_IDENTICAL_TO_A_ROLE_NAME,
    {
        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: A}

roles:
    b: Raw

colorspaces:
- !<ColorSpace>
    name: Raw

named_transforms:
  - !<NamedTransform>
    name: A
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: B}

named_transforms:
  - !<NamedTransform>
    name: B
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);

            // The role takes priority.
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::RolesMerger(options).merge();
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'B' was not merged as it's identical to a role name");

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getRoleName(0), std::string("b"));

                // NamedTransform B should not be added to the merged config (skipped).
                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getNamedTransformNameByIndex(0), std::string("A"));
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::RolesMerger(options).merge();
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'B' was not merged as it's identical to a role name");

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getRoleName(0), std::string("b"));

                // NamedTransform B should not be added to the merged config (skipped)
                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getNamedTransformNameByIndex(0), std::string("A"));
            }
            // Testing the error message when Error on conflict is enabled.
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setErrorOnConflict(true);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO_CHECK_THROW_WHAT(OCIO::NamedTransformsMerger(options).merge(), 
                                      OCIO::Exception,
                                      "Named transform 'B' was not merged as it's identical to a role name");
            } 
        }

        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: A}

named_transforms:
  - !<NamedTransform>
    name: A
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: B}

roles:
    a: B

colorspaces:
- !<ColorSpace>
    name: B
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);

            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::RolesMerger(options).merge(); },
                                       "The Input config contains a role 'a' that would override Base config named transform: 'A'");
                OCIO::ColorspacesMerger(options).merge();
                OCIO::NamedTransformsMerger(options).merge();

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);
                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "A", 0, __LINE__);
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::RolesMerger(options).merge(); },
                                       "The Input config contains a role 'a' that would override Base config named transform: 'A'");
                OCIO::ColorspacesMerger(options).merge();
                OCIO::NamedTransformsMerger(options).merge();

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);
                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "A", 0, __LINE__);
            }  
        }
    }

    // Test ADD_NT_ERROR_NAME_IDENTICAL_TO_COLORSPACE_OR_ALIAS,
    {
        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: B}

colorspaces:
- !<ColorSpace>
    name: B
- !<ColorSpace>
    name: myB
    aliases: [B1]
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csB}

colorspaces:
- !<ColorSpace>
    name: csB

named_transforms:
  - !<NamedTransform>
    name: B
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}

  - !<NamedTransform>
    name: B1
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);

            // The role take priority over the inbound colorspace.
            // The conflicting colorspace should not be added to the merged config (skipped).
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'B' was not merged as there's a color space with that name",
                                       "Named transform 'B1' was not merged as there's a color space alias with that name");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 3);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "B", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                cs = checkColorSpace(mergedConfig, "myB", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("B1"));
                cs = checkColorSpace(mergedConfig, "csB", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'B' was not merged as there's a color space with that name",
                                       "Named transform 'B1' was not merged as there's a color space alias with that name");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 3);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "B", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);

                cs = checkColorSpace(mergedConfig, "myB", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("B1"));

                cs = checkColorSpace(mergedConfig, "csB", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            }
            // Testing the error message when Error on conflict is enabled.
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setErrorOnConflict(true);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                OCIO_CHECK_THROW_WHAT(OCIO::NamedTransformsMerger(options).merge(), 
                                      OCIO::Exception,
                                       "Named transform 'B' was not merged as there's a color space with that name");
            } 
        }

        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csA}

colorspaces:
- !<ColorSpace>
    name: csA

named_transforms:
  - !<NamedTransform>
    name: A
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}

  - !<NamedTransform>
    name: A1
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: A}

colorspaces:
- !<ColorSpace>
    name: A
- !<ColorSpace>
    name: myA
    aliases: [A1]
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'A' was not merged as there's a color space with that name",
                                       "Named transform 'A1' was not merged as there's a color space alias with that name");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 3);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "A", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                cs = checkColorSpace(mergedConfig, "myA", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("A1"));
                cs = checkColorSpace(mergedConfig, "csA", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            }
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'A' was not merged as there's a color space with that name",
                                       "Named transform 'A1' was not merged as there's a color space alias with that name");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 3);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "A", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                cs = checkColorSpace(mergedConfig, "myA", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("A1"));
                cs = checkColorSpace(mergedConfig, "csA", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            }  
        }
    }

    // Test ADD_NT_ERROR_NAME_CONTAIN_CTX_VAR_TOKEN,
    // Not handled as it can't happen in this context.
    // The config will error out while loading the config (before the merge process).

    // Test ADD_NT_ERROR_NAME_IDENTICAL_TO_EXISTING_NT_ALIAS,
    {
        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csA}

colorspaces:
- !<ColorSpace>
    name: csA

named_transforms:
  - !<NamedTransform>
    name: A
    aliases: [B]
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csB}

colorspaces:
- !<ColorSpace>
    name: csB

named_transforms:
  - !<NamedTransform>
    name: B
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "The name of merged named transform 'B' has a conflict with an alias in named transform 'A'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 2);

                OCIO::ConstNamedTransformRcPtr nt = nullptr;
                nt = checkNamedTransform(mergedConfig, "A", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
                nt = checkNamedTransform(mergedConfig, "B", 1, __LINE__);
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "The name of merged named transform 'B' has a conflict with an alias in named transform 'A'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 2);

                OCIO::ConstNamedTransformRcPtr nt = nullptr;
                nt = checkNamedTransform(mergedConfig, "A", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
                nt = checkNamedTransform(mergedConfig, "B", 1, __LINE__);
            }
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setErrorOnConflict(true);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                OCIO_CHECK_THROW_WHAT(OCIO::NamedTransformsMerger(options).merge(), 
                                      OCIO::Exception,
                                       "The name of merged named transform 'B' has a conflict with an alias in named transform 'A'");
            } 
        }

        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csA}

colorspaces:
- !<ColorSpace>
    name: csA

named_transforms:
  - !<NamedTransform>
    name: A
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: B}

colorspaces:
- !<ColorSpace>
    name: csB

named_transforms:
  - !<NamedTransform>
    name: B
    aliases: [A]
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Merged Input named transform 'B' has an alias 'A' that conflicts with named transform 'A'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);

                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "B", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("A"));
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();

                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);

                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Merged Input named transform 'B' has an alias 'A' that conflicts with named transform 'A'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 2);

                OCIO::ConstNamedTransformRcPtr nt = nullptr;
                nt = checkNamedTransform(mergedConfig, "B", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
                nt = checkNamedTransform(mergedConfig, "A", 1, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
            }  
        }
    }

    // Test ADD_NT_ERROR_ALIAS_IDENTICAL_TO_A_ROLE_NAME,
    {
        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csA}

roles:
    b1: csA

colorspaces:
- !<ColorSpace>
    name: csA
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csB}

colorspaces:
- !<ColorSpace>
    name: csB

named_transforms:
  - !<NamedTransform>
    name: B
    aliases: [B1]
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::RolesMerger(options).merge();
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Merged Input named transform 'B' has an alias 'B1' that conflicts with a role");

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getRoleName(0), std::string("b1"));

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);

                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "B", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::RolesMerger(options).merge();
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Merged Input named transform 'B' has an alias 'B1' that conflicts with a role");

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 1);
                OCIO_CHECK_EQUAL(mergedConfig->getRoleName(0), std::string("b1"));

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);

                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "B", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
            }
            // Testing the error message when Error on conflict is enabled.
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setErrorOnConflict(true);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::RolesMerger(options).merge();
                OCIO::ColorspacesMerger(options).merge();
                OCIO_CHECK_THROW_WHAT(OCIO::NamedTransformsMerger(options).merge(), 
                                      OCIO::Exception,
                                      "Merged Input named transform 'B' has an alias 'B1' that conflicts with a role");
            } 
        }

        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csA}

colorspaces:
- !<ColorSpace>
    name: csA

named_transforms:
  - !<NamedTransform>
    name: B
    aliases: [A1]
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csB}

roles:
    a1: csB

colorspaces:
- !<ColorSpace>
    name: csB
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::RolesMerger(options).merge(); },
                                       "The Input config contains a role 'a1' that would override an alias of Base config named transform: 'B'");
                OCIO::ColorspacesMerger(options).merge();
                OCIO::NamedTransformsMerger(options).merge();

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);

                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "B", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("A1"));
            }
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::RolesMerger(options).merge(); },
                                       "The Input config contains a role 'a1' that would override an alias of Base config named transform: 'B'");
                OCIO::ColorspacesMerger(options).merge();
                OCIO::NamedTransformsMerger(options).merge();

                OCIO_CHECK_EQUAL(mergedConfig->getNumRoles(), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 1);

                OCIO::ConstNamedTransformRcPtr nt = checkNamedTransform(mergedConfig, "B", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("A1"));
            }  
        }
    }

    // Test ADD_NT_ERROR_ALIAS_IDENTICAL_TO_COLORSPACE_OR_ALIAS,
    {
        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csA}

colorspaces:
- !<ColorSpace>
    name: csA
- !<ColorSpace>
    name: B
- !<ColorSpace>
    name: myB
    aliases: [B1]
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csB}

colorspaces:
- !<ColorSpace>
    name: csB

named_transforms:
  - !<NamedTransform>
    name: B
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}

  - !<NamedTransform>
    name: B1
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);

            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'B' was not merged as there's a color space with that name",
                                       "Named transform 'B1' was not merged as there's a color space alias with that name");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 4);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "csA", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                cs = checkColorSpace(mergedConfig, "B", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                cs = checkColorSpace(mergedConfig, "myB", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("B1"));
                cs = checkColorSpace(mergedConfig, "csB", 3, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'B' was not merged as there's a color space with that name",
                                       "Named transform 'B1' was not merged as there's a color space alias with that name");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 4);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "csA", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                cs = checkColorSpace(mergedConfig, "B", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                cs = checkColorSpace(mergedConfig, "myB", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("B1"));
                cs = checkColorSpace(mergedConfig, "csB", 3, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
            }
            // Testing the error message when Error on conflict is enabled.
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setErrorOnConflict(true);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                OCIO_CHECK_THROW_WHAT(OCIO::NamedTransformsMerger(options).merge(), 
                                      OCIO::Exception,
                                       "Named transform 'B' was not merged as there's a color space with that name");
            } 
        }

        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csA}

colorspaces:
- !<ColorSpace>
    name: csA

named_transforms:
  - !<NamedTransform>
    name: A
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}

  - !<NamedTransform>
    name: A1
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csB}

colorspaces:
- !<ColorSpace>
    name: csB
- !<ColorSpace>
    name: A
- !<ColorSpace>
    name: myA
    aliases: [A1]
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'A' was not merged as there's a color space with that name",
                                       "Named transform 'A1' was not merged as there's a color space alias with that name");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 4);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "csA", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                cs = checkColorSpace(mergedConfig, "csB", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                cs = checkColorSpace(mergedConfig, "A", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                cs = checkColorSpace(mergedConfig, "myA", 3, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("A1"));
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Named transform 'A' was not merged as there's a color space with that name",
                                       "Named transform 'A1' was not merged as there's a color space alias with that name");

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 0);

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 4);

                OCIO::ConstColorSpaceRcPtr cs = nullptr;
                cs = checkColorSpace(mergedConfig, "csA", 0, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                cs = checkColorSpace(mergedConfig, "csB", 1, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                cs = checkColorSpace(mergedConfig, "A", 2, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                cs = checkColorSpace(mergedConfig, "myA", 3, OCIO::SEARCH_REFERENCE_SPACE_SCENE, __LINE__);
                OCIO_CHECK_EQUAL(cs->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(cs->getAlias(0), std::string("A1"));
            }  
        }
    }

    // Test ADD_NT_ERROR_ALIAS_CONTAIN_CTX_VAR_TOKEN,
    // Not handled as it can't happen in this context.
    // The config will error out while loading the config (before the merge process).

    // Test ADD_NT_ERROR_ALIAS_IDENTICAL_TO_EXISTING_NT_ALIAS
    {
        {
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csA}

colorspaces:
- !<ColorSpace>
    name: csA

named_transforms:
  - !<NamedTransform>
    name: A
    aliases: [my_colorspace]
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: csB}

colorspaces:
- !<ColorSpace>
    name: csB

named_transforms:
  - !<NamedTransform>
    name: B
    aliases: [my_colorspace]
    categories: [ working-space, basic-3d, advanced-2d ]
    encoding: sdr-video
    transform: !<MatrixTransform> {name: forwardBase, offset: [0.1, 0.2, 0.3, 0.4]}
)" };
            std::istringstream bss(BASE);
            std::istringstream iss(INPUT);
            OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
            OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);

            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Merged Input named transform 'B' has a conflict with alias 'my_colorspace' in named transform 'A'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 2);

                OCIO::ConstNamedTransformRcPtr nt = nullptr;
                nt = checkNamedTransform(mergedConfig, "A", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
                nt = checkNamedTransform(mergedConfig, "B", 1, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("my_colorspace"));
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Merged Input named transform 'B' has a conflict with alias 'my_colorspace' in named transform 'A'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 2);

                OCIO::ConstNamedTransformRcPtr nt = nullptr;
                nt = checkNamedTransform(mergedConfig, "B", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("my_colorspace"));
                nt = checkNamedTransform(mergedConfig, "A", 1, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
            }  
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);
                merger->getParams(0)->setInputFirst(false);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Merged Input named transform 'B' has a conflict with alias 'my_colorspace' in named transform 'A'");

                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 2);

                OCIO::ConstNamedTransformRcPtr nt = nullptr;
                nt = checkNamedTransform(mergedConfig, "A", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("my_colorspace"));
                nt = checkNamedTransform(mergedConfig, "B", 1, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
            }
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_BASE);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                       [&options]() { OCIO::NamedTransformsMerger(options).merge(); },
                                       "Merged Input named transform 'B' has a conflict with alias 'my_colorspace' in named transform 'A'");
                           
                OCIO_CHECK_EQUAL(mergedConfig->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL, OCIO::COLORSPACE_ALL), 2);

                OCIO_CHECK_EQUAL(mergedConfig->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 2);

                OCIO::ConstNamedTransformRcPtr nt = nullptr;
                nt = checkNamedTransform(mergedConfig, "B", 0, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
                nt = checkNamedTransform(mergedConfig, "A", 1, __LINE__);
                OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
                OCIO_CHECK_EQUAL(nt->getAlias(0), std::string("my_colorspace"));
            }  
            // Testing the error message when Error on conflict is enabled.
            {
                OCIO::ConfigMergerRcPtr merger = OCIO::ConfigMerger::Create();
                auto params = setupBasics(merger, MergeStrategy::STRATEGY_PREFER_INPUT);
                merger->getParams(0)->setErrorOnConflict(true);

                OCIO::ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();
                OCIO::MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
                OCIO::ColorspacesMerger(options).merge();
                OCIO_CHECK_THROW_WHAT(OCIO::NamedTransformsMerger(options).merge(), 
                                      OCIO::Exception,
                                       "Merged Input named transform 'B' has a conflict with alias 'my_colorspace' in named transform 'A'");
            } 
        }
    }
}

OCIO_ADD_TEST(MergeConfigs, merges_with_ociom_file)
{
    {
        std::vector<std::string> paths = { 
            std::string(OCIO::GetTestFilesDir()),
            std::string("configs"),
            std::string("mergeconfigs"),
            std::string("merged1"),
            std::string("merged1.ociom")
        }; 
        const std::string ociomPath = pystring::os::path::normpath(pystring::os::path::join(paths));

        // PreferInput, Input first
        {
            OCIO::ConstConfigMergerRcPtr merger = OCIO::ConfigMerger::CreateFromFile(ociomPath.c_str());
            OCIO::ConstConfigMergerRcPtr newMerger;
            checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                   [&newMerger, &merger]() { newMerger = OCIO::ConfigMergingHelpers::MergeConfigs(merger); },
                                   "The Input config contains a value that would override the Base config: file_rules: Default",
                                   "Merged color space 'ACES2065-1' has a conflict with alias 'aces' in color space 'ACEScg'",
                "Equivalent input color space 'sRGB - Display' replaces 'sRGB - Display' in the base config, preserving aliases.",
                "Equivalent input color space 'CIE-XYZ-D65' replaces 'CIE-XYZ-D65' in the base config, preserving aliases.",
                "Equivalent input color space 'ACES2065-1' replaces 'ap0' in the base config, preserving aliases.",
                "Equivalent input color space 'sRGB' replaces 'sRGB - Texture' in the base config, preserving aliases.");
// TODO: Last one should not be necessary.
//                                   "The Input config contains a role that would override Base config role 'aces_interchange'");
            OCIO::ConstConfigRcPtr mergedConfig = newMerger->getMergedConfig();
            std::ostringstream oss;
            mergedConfig->serialize(oss);

            constexpr const char * RESULT {
R"(ocio_profile_version: 2.1

environment:
  SHOT: 001a
  TEXTURE_SPACE: sRGB - Texture
search_path:
  - lut_dir
  - luts
  - .
strictparsing: true
family_separator: "~"
luma: [0.2126, 0.7152, 0.0722]
name: Merged1
description: Basic merge with default strategy

roles:
  cie_xyz_d65_interchange: CIE-XYZ-D65

file_rules:
  - !<Rule> {name: Default, colorspace: sRGB}

displays:
  sRGB - Display:
    - !<View> {name: Raw, colorspace: raw}
    - !<View> {name: ACES 1.0 - SDR Video, view_transform: SDR Video, display_colorspace: sRGB - Display}

active_displays: []
active_views: []

view_transforms:
  - !<ViewTransform>
    name: SDR Video
    description: from input
    from_scene_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [0.439632981919492, 0.382988698151554, 0.177378319928956, 0, 0.0897764429588422, 0.813439428748978, 0.0967841282921771, 0, 0.0175411703831728, 0.111546553302387, 0.870912276314442, 0, 0, 0, 0, 1], direction: inverse}
        - !<BuiltinTransform> {style: ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-P3lim_1.1}
        - !<MatrixTransform> {matrix: [0.412390799266, 0.357584339384, 0.180480788402, 0, 0.212639005872, 0.715168678768, 0.072192315361, 0, 0.019330818716, 0.119194779795, 0.95053215225, 0, 0, 0, 0, 1]}

  - !<ViewTransform>
    name: vt2
    description: from base
    to_display_reference: !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055}

display_colorspaces:
  - !<ColorSpace>
    name: sRGB - Display
    aliases: [srgb_display]
    family: Display~Standard
    equalitygroup: ""
    bitdepth: unknown
    description: from input
    isdata: false
    allocation: uniform
    from_display_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [0.412390799266, 0.357584339384, 0.180480788402, 0, 0.212639005872, 0.715168678768, 0.072192315361, 0, 0.019330818716, 0.119194779795, 0.95053215225, 0, 0, 0, 0, 1], direction: inverse}
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

  - !<ColorSpace>
    name: CIE-XYZ-D65
    aliases: [cie_xyz_d65]
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    description: The \"CIE XYZ (D65)\" display connection colorspace.
    isdata: false
    allocation: uniform
    from_display_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [0.412390799266, 0.357584339384, 0.180480788402, 0, 0.212639005872, 0.715168678768, 0.072192315361, 0, 0.019330818716, 0.119194779795, 0.95053215225, 0, 0, 0, 0, 1], direction: inverse}
        - !<MatrixTransform> {matrix: [0.412390799266, 0.357584339384, 0.180480788402, 0, 0.212639005872, 0.715168678768, 0.072192315361, 0, 0.019330818716, 0.119194779795, 0.95053215225, 0, 0, 0, 0, 1]}

colorspaces:
  - !<ColorSpace>
    name: ACES2065-1
    aliases: [aces, ap0]
    family: ACES~Linear
    equalitygroup: ""
    bitdepth: unknown
    description: from input
    isdata: false
    allocation: uniform
    to_scene_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [2.521686186744, -1.13413098824, -0.387555198504, 0, -0.27647991423, 1.372719087668, -0.096239173438, 0, -0.015378064966, -0.152975335867, 1.168353400833, 0, 0, 0, 0, 1]}
        - !<MatrixTransform> {matrix: [0.439632981919492, 0.382988698151554, 0.177378319928956, 0, 0.0897764429588422, 0.813439428748978, 0.0967841282921771, 0, 0.0175411703831728, 0.111546553302387, 0.870912276314442, 0, 0, 0, 0, 1]}

  - !<ColorSpace>
      name: sRGB
      aliases: [sRGB - Texture, srgb_tx]
      family: Texture~
      equalitygroup: ""
      bitdepth: unknown
      description: from input
      isdata: false
      allocation: uniform
      to_scene_reference: !<GroupTransform>
        children:
          - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055}
          - !<MatrixTransform> {matrix: [0.439632981919492, 0.382988698151554, 0.177378319928956, 0, 0.0897764429588422, 0.813439428748978, 0.0967841282921771, 0, 0.0175411703831728, 0.111546553302387, 0.870912276314442, 0, 0, 0, 0, 1]}

  - !<ColorSpace>
    name: rec709
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    description: from input
    isdata: false
    allocation: uniform
    to_scene_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [0.439632981919492, 0.382988698151554, 0.177378319928956, 0, 0.0897764429588422, 0.813439428748978, 0.0967841282921771, 0, 0.0175411703831728, 0.111546553302387, 0.870912276314442, 0, 0, 0, 0, 1]}

  - !<ColorSpace>
    name: Raw
    aliases: [Utility - Raw]
    family: Utility
    equalitygroup: ""
    bitdepth: 32f
    description: The utility "Raw" colorspace.
    isdata: true
    categories: [file-io]
    allocation: uniform

  - !<ColorSpace>
    name: ACEScg
    family: ACES~Linear
    equalitygroup: ""
    bitdepth: unknown
    description: from base
    isdata: false
    allocation: uniform
    to_scene_reference: !<BuiltinTransform> {style: ACEScg_to_ACES2065-1}
    )" };

            std::istringstream resultIss;
            resultIss.str(RESULT);
            OCIO::ConstConfigRcPtr resultConfig = OCIO::Config::CreateFromStream(resultIss);
            std::ostringstream ossResult;
            resultConfig->serialize(ossResult);

            //Testing the string of each config

            OCIO_CHECK_EQUAL(oss.str(), ossResult.str());

        }
    }
/*
    // Test is very similar as the previous one but it has two merges in
    // the OCIOM file and it is using the output of the first merged config
    // as the input for the second merge.
    {
        std::vector<std::string> paths = { 
            std::string(OCIO::GetTestFilesDir()),
            std::string("configs"),
            std::string("mergeconfigs"),
            std::string("merged2"),
            std::string("merged.ociom")
        }; 
        const std::string ociomPath = pystring::os::path::normpath(pystring::os::path::join(paths));

        // PreferInput, Input first
        {
            OCIO::ConstConfigMergerRcPtr merger = OCIO::ConfigMerger::CreateFromFile(ociomPath.c_str());
            OCIO::ConstConfigMergerRcPtr newMerger;
            checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
                                   [&newMerger, &merger]() { newMerger = OCIO::ConfigMergingHelpers::MergeConfigs(merger); },
                                   "The Input config contains a value that would override the Base config: file_rules: Default",
                                   "Merged color space 'ACES2065-1' has a conflict with alias 'aces' in color space 'ACEScg'");
            OCIO::ConstConfigRcPtr mergedConfig = newMerger->getMergedConfig();
            std::ostringstream oss;
            mergedConfig->serialize(oss);

            constexpr const char * RESULT {
R"(ocio_profile_version: 2.1

environment:
  {}
search_path: lut_dir
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]
name: Merged2
description: Same as the input

roles:
  cie_xyz_d65_interchange: CIE-XYZ-D65

file_rules:
  - !<Rule> {name: Default, colorspace: sRGB}

displays:
  sRGB - Display:
    - !<View> {name: Raw, colorspace: raw}
    - !<View> {name: ACES 1.0 - SDR Video, view_transform: SDR Video, display_colorspace: sRGB - Display}

active_displays: []
active_views: []
inactive_colorspaces: [ACES2065-1]

view_transforms:
  - !<ViewTransform>
    name: SDR Video
    from_scene_reference: !<BuiltinTransform> {style: ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-VIDEO-P3lim_1.1}

display_colorspaces:
  - !<ColorSpace>
    name: sRGB - Display
    aliases: [srgb_display]
    family: Display~Standard
    equalitygroup: ""
    bitdepth: unknown
    description: from input
    isdata: false
    allocation: uniform
    from_display_reference: !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}
  
  - !<ColorSpace>
    name: CIE-XYZ-D65
    aliases: [cie_xyz_d65]
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    description: The \"CIE XYZ (D65)\" display connection colorspace.
    isdata: false
    allocation: uniform
    from_display_reference: !<MatrixTransform> {matrix: [0.412390799266, 0.357584339384, 0.180480788402, 0, 0.212639005872, 0.715168678768, 0.072192315361, 0, 0.019330818716, 0.119194779795, 0.95053215225, 0, 0, 0, 0, 1]}

colorspaces:
  - !<ColorSpace>
    name: ACES2065-1
    aliases: [aces]
    family: ACES~Linear
    equalitygroup: ""
    bitdepth: unknown
    description: from input
    isdata: false
    allocation: uniform
    to_scene_reference: !<MatrixTransform> {matrix: [2.521686186744, -1.13413098824, -0.387555198504, 0, -0.27647991423, 1.372719087668, -0.096239173438, 0, -0.015378064966, -0.152975335867, 1.168353400833, 0, 0, 0, 0, 1]}
  
  - !<ColorSpace>
    name: sRGB
    family: Texture~
    equalitygroup: ""
    bitdepth: unknown
    description: from input
    isdata: false
    allocation: uniform
    to_scene_reference: !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055}
  
  - !<ColorSpace>
    name: rec709
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    description: from input
    isdata: false
    allocation: uniform
  
  - !<ColorSpace>
    name: Raw
    aliases: [Utility - Raw]
    family: Utility
    equalitygroup: ""
    bitdepth: 32f
    description: The utility "Raw" colorspace.
    isdata: true
    categories: [file-io]
    allocation: uniform
    )" };

            std::istringstream resultIss;
            resultIss.str(RESULT);
            OCIO::ConstConfigRcPtr resultConfig = OCIO::Config::CreateFromStream(resultIss);
            std::ostringstream ossResult;
            resultConfig->serialize(ossResult);

            //Testing the string of each config
//            OCIO_CHECK_EQUAL(oss.str(), ossResult.str());
        }
    }
*/
    // Test with external LUT files.
    {
        std::vector<std::string> paths = { 
            std::string(OCIO::GetTestFilesDir()),
            std::string("configs"),
            std::string("mergeconfigs"),
            std::string("merged3"),
            std::string("merged.ociom")
        }; 
        const std::string ociomPath = pystring::os::path::normpath(pystring::os::path::join(paths));

        // PreferInput, Input first
        {
            OCIO::ConstConfigMergerRcPtr merger = OCIO::ConfigMerger::CreateFromFile(ociomPath.c_str());
            merger->getParams(0)->setAssumeCommonReferenceSpace(true);
            OCIO::ConstConfigMergerRcPtr newMerger = OCIO::ConfigMergingHelpers::MergeConfigs(merger);
            OCIO::ConstConfigRcPtr mergedConfig = newMerger->getMergedConfig();
//             std::ostringstream oss;
//             mergedConfig->serialize(oss);
            OCIO_CHECK_NO_THROW(mergedConfig->validate());

          {
            OCIO_CHECK_EQUAL(mergedConfig->getSearchPath(), std::string("./$SHOT:./shot1:shot2:."));
            auto cs = mergedConfig->getColorSpace("shot1_lut1_cs");
            auto tf = cs->getTransform(OCIO::COLORSPACE_DIR_TO_REFERENCE);
            auto ftf = OCIO::DynamicPtrCast<const OCIO::FileTransform>(tf);
            OCIO_REQUIRE_ASSERT(ftf);
            OCIO_CHECK_EQUAL(ftf->getSrc(), std::string("lut1.clf"));
            OCIO_CHECK_NO_THROW(mergedConfig->getProcessor(ftf))
          }
          {
            auto look = mergedConfig->getLook("shot_look");
            auto ltf = look->getTransform();
            OCIO_CHECK_NO_THROW(mergedConfig->getProcessor(ltf));
          }
        }
    }

    // Test that a merge could go wrong if the search_paths are merged with a different strategy
    // than the other sections.
    {
        std::vector<std::string> paths = { 
            std::string(OCIO::GetTestFilesDir()),
            std::string("configs"),
            std::string("mergeconfigs"),
            std::string("merged3"),
            std::string("merged.ociom")
        }; 
        const std::string ociomPath = pystring::os::path::normpath(pystring::os::path::join(paths));

        {
            OCIO::ConstConfigMergerRcPtr merger = OCIO::ConfigMerger::CreateFromFile(ociomPath.c_str());
            merger->getParams(0)->setAssumeCommonReferenceSpace(true);
            // Changing the strategy for colorspace merger to BASE ONLY.
            // This will break the looks "shot_look" (from input) as it needs the search paths 
            // from the input config. (search_paths are managed by the colorspace merger).
            merger->getParams(0)->setColorspaces(OCIO::ConfigMergingParameters::STRATEGY_INPUT_ONLY);
            // The rest of the merges uses PreferInput strategy.
        
            OCIO::ConstConfigMergerRcPtr newMerger = OCIO::ConfigMergingHelpers::MergeConfigs(merger);
            OCIO::ConstConfigRcPtr mergedConfig = newMerger->getMergedConfig();
            auto look = mergedConfig->getLook("shot_look");
            auto ltf = look->getTransform();

            // Expected to throw as the search_paths were merged following the InputOnly strategy 
            // and the looks were merged following the PreferInput (see OCIOM file default strategy).
            // Therefore, the look's FileTransform can not find "look.cdl" and throws an exception.
            OCIO_CHECK_THROW(mergedConfig->getProcessor(ltf), OCIO::Exception);

            // It can happen with any section that uses the search_paths such as looks,
            // named transforms, and colorspaces.
        }
    }

    // Test with a built-in config.
    {
        std::vector<std::string> paths = { 
            std::string(OCIO::GetTestFilesDir()),
            std::string("configs"),
            std::string("mergeconfigs"),
            std::string("merged4"),
            std::string("merged.ociom")
        }; 
        const std::string ociomPath = pystring::os::path::normpath(pystring::os::path::join(paths));

        // InputOnly
        {
            OCIO::ConstConfigMergerRcPtr merger = OCIO::ConfigMerger::CreateFromFile(ociomPath.c_str());
            OCIO::ConstConfigMergerRcPtr newMerger = OCIO::ConfigMergingHelpers::MergeConfigs(merger);
            OCIO::ConstConfigRcPtr mergedConfig = newMerger->getMergedConfig();
//             std::ostringstream oss;
//             mergedConfig->serialize(oss);

            // Test that the merged config is the same of the built-in config used as input.
            auto bConfig = OCIO::Config::CreateFromBuiltinConfig("cg-config-v1.0.0_aces-v1.3_ocio-v2.1");
//            OCIO_CHECK_EQUAL(std::string(mergedConfig->getCacheID()), std::string(bConfig->getCacheID()));
        }
    }
}
    // Test with an OCIOZ archive
    // {
    //     std::vector<std::string> paths = { 
    //         std::string(OCIO::GetTestFilesDir()),
    //         std::string("configs"),
    //         std::string("mergeconfigs"),
    //         std::string("merged4"),
    //         std::string("merged.ociom")
    //     }; 
    //     const std::string ociomPath = pystring::os::path::normpath(pystring::os::path::join(paths));

    //     // InputOnly
    //     {
    //         OCIO::ConstConfigMergerRcPtr merger = OCIO::ConfigMerger::CreateFromFile(ociomPath.c_str());
    //         // Update the merge to point to the OCIOZ archive as the input.
    //         merger->getParams(0)->setInputConfigName("");

    //         OCIO::ConstConfigMergerRcPtr newMerger = OCIO::ConfigMergingHelpers::MergeConfigs(merger);
    //         OCIO::ConstConfigRcPtr mergedConfig = newMerger->getMergedConfig();
    //         std::ostringstream oss;
    //         mergedConfig->serialize(oss);
    //         OCIO_CHECK_NO_THROW(mergedConfig->validate());
    //     }
    //}

OCIO_ADD_TEST(MergeConfigs, merge_in_memory_configs)
{
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: A}

roles:
    a: colorspace_a

colorspaces:
- !<ColorSpace>
    name: colorspace_a
    family: utility
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: B}

colorspaces:
- !<ColorSpace>
    name: B
    family: aces
)" };

            constexpr const char * RESULT {
R"(ocio_profile_version: 2

roles:
  a: colorspace_a

file_rules:
  - !<Rule> {name: Default, colorspace: B}

colorspaces:
  - !<ColorSpace>
    name: colorspace_a
    family: Base/utility
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform

  - !<ColorSpace>
    name: B
    family: Input/aces
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform)" };

        std::istringstream bss(BASE);
        std::istringstream iss(INPUT);
        OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
        OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);

        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        params->setInputFirst(false);
        MergeStrategy strategy = MergeStrategy::STRATEGY_PREFER_INPUT;
        params->setRoles(strategy);
        params->setColorspaces(strategy);
        params->setNamedTransforms(strategy);
        params->setDefaultStrategy(strategy);
        params->setInputFamilyPrefix("Input/");
        params->setBaseFamilyPrefix("Base/");
        params->setAssumeCommonReferenceSpace(true);
        params->setAvoidDuplicates(false);

//        OCIO::ConfigRcPtr mergedConfig = OCIO::ConfigMergingHelpers::MergeConfigs(params, baseConfig, inputConfig);
        OCIO::ConfigRcPtr mergedConfig;

        checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
//                               [&newMerger, &merger]() { newMerger = OCIO::ConfigMergingHelpers::MergeConfigs(merger); },
                               [&mergedConfig, &params, &baseConfig, &inputConfig]() { mergedConfig = OCIO::ConfigMergingHelpers::MergeConfigs(params, baseConfig, inputConfig); },
                               "The Input config contains a value that would override the Base config: file_rules: Default");
//                               "Merged color space 'ACES2065-1' has a conflict with alias 'aces' in color space 'ACEScg'",
// TODO: Last one should not be necessary.
//                               "The Input config contains a role that would override Base config role 'aces_interchange'");


// FIXME: Add a test to check this result.
//     std::ostringstream ossResult;
//     mergedConfig->serialize(ossResult);
//     std::cout << ossResult.str() << "\n";

            std::ostringstream oss;
            mergedConfig->serialize(oss);

            std::istringstream resultIss;
            resultIss.str(RESULT);
            OCIO::ConstConfigRcPtr resultConfig = OCIO::Config::CreateFromStream(resultIss);
            std::ostringstream ossResult;
            resultConfig->serialize(ossResult);

            //Testing the string of each config
           OCIO_CHECK_EQUAL(oss.str(), ossResult.str());
}

OCIO_ADD_TEST(MergeConfigs, merge_single_colorspace)
{
            constexpr const char * BASE {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: A}

roles:
    a: colorspace_a

colorspaces:
- !<ColorSpace>
    name: colorspace_a
    family: utility
)" };

            constexpr const char * INPUT {
R"(ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: B}

colorspaces:
- !<ColorSpace>
    name: B
    family: aces
)" };
        std::istringstream bss(BASE);
        std::istringstream iss(INPUT);
        OCIO::ConstConfigRcPtr baseConfig = OCIO::Config::CreateFromStream(bss);
        OCIO::ConstConfigRcPtr inputConfig = OCIO::Config::CreateFromStream(iss);
        OCIO::ConstColorSpaceRcPtr colorspace = inputConfig->getColorSpace("B");

        OCIO::ConfigMergingParametersRcPtr params = OCIO::ConfigMergingParameters::Create();
        params->setInputFirst(false);
        MergeStrategy strategy = MergeStrategy::STRATEGY_PREFER_INPUT;
        params->setRoles(strategy);
        params->setColorspaces(strategy);
        params->setNamedTransforms(strategy);
        params->setDefaultStrategy(strategy);
        params->setInputFamilyPrefix("Input/");
        params->setBaseFamilyPrefix("Base/");
        params->setAssumeCommonReferenceSpace(true);
        params->setAvoidDuplicates(false);

        OCIO::ConfigRcPtr mergedConfig = OCIO::ConfigMergingHelpers::MergeColorSpace(params, baseConfig, colorspace);

// FIXME: Add a test to check this result.
//     std::ostringstream ossResult;
//     mergedConfig->serialize(ossResult);
//     std::cout << ossResult.str() << "\n";
}

OCIO_ADD_TEST(MergeConfigs, avoid_duplicate_color_spaces)
{
    {
//         std::vector<std::string> paths = { 
//             std::string(OCIO::GetTestFilesDir()),
//             std::string("configs"),
//             std::string("mergeconfigs"),
//             std::string("merged1"),
//             std::string("merged1.ociom")
//         }; 
//         const std::string ociomPath = pystring::os::path::normpath(pystring::os::path::join(paths));
        const std::string ociomPath = "/Users/walkerdo/Documents/work/Autodesk/color/adsk_color_mgmt/OCIO/configs/merging/merge_flame_core.ociom";

        // PreferInput, Input first
        {
            OCIO::ConstConfigMergerRcPtr merger = OCIO::ConfigMerger::CreateFromFile(ociomPath.c_str());
            OCIO::ConstConfigMergerRcPtr newMerger;
//             checkForLogOrException(LOG_TYPE_WARNING, __LINE__, 
//                                    [&newMerger, &merger]() { newMerger = OCIO::ConfigMergingHelpers::MergeConfigs(merger); },
//                                    "The Input config contains a value that would override the Base config: file_rules: Default",
//                                    "Merged color space 'ACES2065-1' has a conflict with alias 'aces' in color space 'ACEScg'");
// TODO: Last one should not be necessary.
//                                   "The Input config contains a role that would override Base config role 'aces_interchange'");
            newMerger = OCIO::ConfigMergingHelpers::MergeConfigs(merger);
            OCIO::ConstConfigRcPtr mergedConfig = newMerger->getMergedConfig();
            std::ostringstream oss;
            mergedConfig->serialize(oss);
//     std::cout << oss.str() << "\n";
        }
    }
}
