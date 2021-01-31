// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "apphelpers/ColorSpaceHelpers.cpp"
#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"


namespace OCIO = OCIO_NAMESPACE;


#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

#define STR(x) FIELD_STR(x)

static const std::string ocioTestFilesDir(STR(OCIO_UNIT_TEST_FILES_DIR));


// The configuration file used by the unit tests.
#include "configs.data"


OCIO_ADD_TEST(ColorSpaceInfo, read_values)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    // Tests with 'in_1'.

    OCIO::ConstColorSpaceRcPtr cs;
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("in_1"));

    OCIO::ConstColorSpaceInfoRcPtr csInfo;
    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(config, *cs));

    OCIO_CHECK_EQUAL(std::string(cs->getName()), "in_1");
    OCIO_CHECK_EQUAL(std::string(csInfo->getName()), "in_1");

    OCIO_CHECK_EQUAL(std::string(cs->getFamily()), "Input / Camera/Acme");
    OCIO_REQUIRE_EQUAL(csInfo->getNumHierarchyLevels(), 3);
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(0)), "Input");
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(1)), "Camera");
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(2)), "Acme");
    OCIO_CHECK_EQUAL(csInfo->getFamily(), std::string(cs->getFamily()));

    OCIO_CHECK_EQUAL(cs->getDescription(), std::string("An input color space.\nFor the Acme camera."));
    OCIO_CHECK_EQUAL(csInfo->getDescription(), std::string(cs->getDescription()));

    // Tests with 'lin_1'.

    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("lin_1"));
    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(config, *cs));

    OCIO_CHECK_EQUAL(std::string(cs->getName()), "lin_1");
    OCIO_CHECK_EQUAL(std::string(csInfo->getName()), "lin_1");

    OCIO_CHECK_EQUAL(std::string(cs->getFamily()), "");
    OCIO_CHECK_EQUAL(csInfo->getNumHierarchyLevels(), 0);
    OCIO_CHECK_EQUAL(std::string(csInfo->getFamily()), "");

    OCIO_CHECK_EQUAL(std::string(cs->getDescription()), "");
}

OCIO_ADD_TEST(ColorSpaceInfo, change_values)
{
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateRaw());
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::ColorSpaceRcPtr cs;
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("raw")->createEditableCopy());

    OCIO::ConstColorSpaceInfoRcPtr csInfo;
    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(config, *cs));

    OCIO_CHECK_EQUAL(cs->getName(), std::string("raw"));
    OCIO_CHECK_EQUAL(csInfo->getName(), std::string("raw"));

    OCIO_CHECK_EQUAL(cs->getFamily(), std::string("raw"));
    OCIO_REQUIRE_EQUAL(csInfo->getNumHierarchyLevels(), 1);
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(0)), "raw");
    OCIO_CHECK_EQUAL(std::string(csInfo->getFamily()), cs->getFamily());

    OCIO_CHECK_EQUAL(std::string(cs->getDescription()),
                     "A raw color space. Conversions to and from this space are no-ops.");
    OCIO_CHECK_EQUAL(std::string(csInfo->getDescription()), cs->getDescription());

    // Change the family.

    OCIO_CHECK_NO_THROW(cs->setFamily(""));
    OCIO_CHECK_EQUAL(std::string(cs->getFamily()), "");

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(config, *cs));
    OCIO_CHECK_EQUAL(csInfo->getNumHierarchyLevels(), 0);
    OCIO_CHECK_EQUAL(std::string(csInfo->getFamily()), "");

    OCIO_CHECK_NO_THROW(cs->setFamily("Acme     /   Camera"));
    OCIO_CHECK_EQUAL(std::string(cs->getFamily()), "Acme     /   Camera");

    OCIO::ConfigRcPtr cfg = config->createEditableCopy();

    // No family separator.

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator(0)); // i.e. that's the null character

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, *cs));
    OCIO_REQUIRE_EQUAL(csInfo->getNumHierarchyLevels(), 1);
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(0)), cs->getFamily());

    // '/' is the new family separator.

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator('/'));

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, *cs));
    OCIO_REQUIRE_EQUAL(csInfo->getNumHierarchyLevels(), 2);
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(0)), "Acme");
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(1)), "Camera");
    OCIO_CHECK_EQUAL(std::string(csInfo->getFamily()), cs->getFamily());

    // '-' is the new family separator.

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator('-'));

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, *cs));
    OCIO_REQUIRE_EQUAL(csInfo->getNumHierarchyLevels(), 1);
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(0)), cs->getFamily());

    // Reset to the v2 default family separator i.e. default to '/'.

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator(OCIO::Config::GetDefaultFamilySeparator()));

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, *cs));
    OCIO_REQUIRE_EQUAL(csInfo->getNumHierarchyLevels(), 2);
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(0)), "Acme");
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(1)), "Camera");
    OCIO_CHECK_EQUAL(std::string(csInfo->getFamily()), cs->getFamily());

    // Change the description.

    OCIO_CHECK_NO_THROW(cs->setDescription("desc 1\n\n\n desc 2"));
    OCIO_CHECK_EQUAL(cs->getDescription(), std::string("desc 1\n\n\n desc 2"));

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, *cs));
    OCIO_CHECK_EQUAL(std::string(csInfo->getDescription()), cs->getDescription());
}

OCIO_ADD_TEST(ColorSpaceMenuHelper, no_color_spaces)
{
    std::istringstream is{ R"(ocio_profile_version: 2

environment:
  {}

search_path: luts
strictparsing: true
family_separator: /
luma: [0.2126, 0.7152, 0.0722]

roles:
  rendering: test_1
  default: raw

view_transforms:
  - !<ViewTransform>
    name: view_transform
    from_scene_reference: !<MatrixTransform> {}

displays:
  DISP_1:
    - !<View> {name: VIEW_1, colorspace: test_1}
    - !<View> {name: VIEW_2, colorspace: test_2}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: raw
    family: Raw
    description: A raw color space. Conversions to and from this space are no-ops.
    isdata: true
    allocation: uniform

  - !<ColorSpace>
    name: test_1
    categories: [ working-space, basic-2d ]
    encoding: scene-linear

  - !<ColorSpace>
    name: test_2
    categories: [ working-space ]
    encoding: scene-linear
 )" };

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::ColorSpaceMenuHelperRcPtr menuHelper;

    // Use app-oriented categories with exact case.

    auto params = OCIO::ColorSpaceMenuParameters::Create(config);

    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 3);

    params->setIncludeColorSpaces(false);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 0);

    params->setIncludeNamedTransforms(true);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 0);

    params->setIncludeColorSpaces(true);
    params->setSearchReferenceSpaceType(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 0);

    params->setAppCategories("basic-2d");
    params->setSearchReferenceSpaceType(OCIO::SEARCH_REFERENCE_SPACE_SCENE);

    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 1);

    params->setIncludeColorSpaces(false);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 0);

    params->setIncludeColorSpaces(true);
    params->setSearchReferenceSpaceType(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 0);
}

OCIO_ADD_TEST(ColorSpaceMenuHelper, categories)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::ColorSpaceMenuHelperRcPtr menuHelper;

    // Use app-oriented categories with exact case.

    auto params = OCIO::ColorSpaceMenuParameters::Create(config);
    params->setAppCategories("file-io");

    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));

    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 4);

    // Use app-oriented categories with other case.

    params->setAppCategories("FILE-IO");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));

    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 4);

    // Use app-oriented categories, including named transforms.

    params->setAppCategories("file-io");
    params->setIncludeNamedTransforms(true);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 5);

    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("in_1"));
    OCIO_CHECK_EQUAL(menuHelper->getName(1), std::string("in_2"));
    OCIO_CHECK_EQUAL(menuHelper->getName(2), std::string("in_3"));
    OCIO_CHECK_EQUAL(menuHelper->getName(3), std::string("lut_input_3"));
    OCIO_CHECK_EQUAL(menuHelper->getName(4), std::string("nt3"));
    OCIO_CHECK_EQUAL(menuHelper->getName(5), std::string(""));

    OCIO_CHECK_EQUAL(menuHelper->getUIName(0), std::string("in_1"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(1), std::string("in_2"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(2), std::string("in_3"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(3), std::string("lut_input_3"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(4), std::string("nt3"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(5), std::string(""));

    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(0), 3);
    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(1), 0);
    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(2), 0);
    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(3), 0);
    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(4), 1);
    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(5), 0);

    OCIO_CHECK_EQUAL(menuHelper->getHierarchyLevel(0, 0), std::string("Input"));
    OCIO_CHECK_EQUAL(menuHelper->getHierarchyLevel(0, 1), std::string("Camera"));
    OCIO_CHECK_EQUAL(menuHelper->getHierarchyLevel(0, 2), std::string("Acme"));
    OCIO_CHECK_EQUAL(menuHelper->getHierarchyLevel(4, 0), std::string("Raw"));

    // Use null categories.

    params->setIncludeNamedTransforms(false);
    params->setAppCategories("");
    // All color spaces (scene and display).
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 16);

    // Non display color spaces only.
    params->setSearchReferenceSpaceType(OCIO::SEARCH_REFERENCE_SPACE_SCENE);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 13);

    // Display only.
    params->setSearchReferenceSpaceType(OCIO::SEARCH_REFERENCE_SPACE_DISPLAY);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 3);

    // Use null categories, including named transforms.

    params->setSearchReferenceSpaceType(OCIO::SEARCH_REFERENCE_SPACE_ALL);
    params->setIncludeNamedTransforms(true);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));

    // All color spaces and named transforms.
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 19);

    // Use app-oriented category, include roles.

    params->setIncludeNamedTransforms(false);
    params->setAppCategories("look-process-space");
    params->setIncludeRoles(true);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 7);
    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("lut_input_1"));
    OCIO_CHECK_EQUAL(menuHelper->getName(1), std::string("lut_input_2"));
    OCIO_CHECK_EQUAL(menuHelper->getName(2), std::string("lut_input_3"));
    OCIO_CHECK_EQUAL(menuHelper->getName(3), std::string("default"));
    OCIO_CHECK_EQUAL(menuHelper->getName(4), std::string("reference"));
    OCIO_CHECK_EQUAL(menuHelper->getName(5), std::string("rendering"));
    OCIO_CHECK_EQUAL(menuHelper->getName(6), std::string("scene_linear"));
    OCIO_CHECK_EQUAL(menuHelper->getName(7), std::string(""));

    OCIO_CHECK_EQUAL(menuHelper->getIndexFromName("lut_input_1"), 0);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromName("lut_input_2"), 1);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromName("lut_input_3"), 2);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromName("default"), 3);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromName("reference"), 4);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromName("rendering"), 5);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromName("scene_linear"), 6);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromName("default (lin_1)"), (size_t)(-1));

    OCIO_CHECK_EQUAL(menuHelper->getUIName(0), std::string("lut_input_1"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(1), std::string("lut_input_2"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(2), std::string("lut_input_3"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(3), std::string("default (raw)"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(4), std::string("reference (lin_1)"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(5), std::string("rendering (lin_1)"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(6), std::string("scene_linear (lin_1)"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(7), std::string(""));

    OCIO_CHECK_EQUAL(menuHelper->getIndexFromUIName("lut_input_1"), 0);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromUIName("lut_input_2"), 1);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromUIName("lut_input_3"), 2);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromUIName("default (raw)"), 3);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromUIName("reference (lin_1)"), 4);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromUIName("rendering (lin_1)"), 5);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromUIName("scene_linear (lin_1)"), 6);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromUIName("default"), (size_t)(-1));

    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(0), 0);
    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(1), 0);
    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(2), 0);
    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(3), 1);
    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(4), 1);
    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(5), 1);
    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(6), 1);
    OCIO_CHECK_EQUAL(menuHelper->getNumHierarchyLevels(7), 0);

    OCIO_CHECK_EQUAL(menuHelper->getHierarchyLevel(3, 0), std::string("Roles"));
    OCIO_CHECK_EQUAL(menuHelper->getHierarchyLevel(4, 0), std::string("Roles"));
    OCIO_CHECK_EQUAL(menuHelper->getHierarchyLevel(5, 0), std::string("Roles"));
    OCIO_CHECK_EQUAL(menuHelper->getHierarchyLevel(6, 0), std::string("Roles"));
    OCIO_CHECK_EQUAL(menuHelper->getHierarchyLevel(7, 1), std::string(""));
    OCIO_CHECK_EQUAL(menuHelper->getHierarchyLevel(6, 1), std::string(""));

    // Use an arbitrary (but existing) category only used by a named transform.

    {
        params->setIncludeRoles(false);
        params->setIncludeNamedTransforms(true);
        params->setAppCategories("");
        params->setUserCategories("basic-3d");
        OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
        OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 1);
        OCIO_CHECK_EQUAL(menuHelper->getUIName(0), std::string("nt1"));

        // No color space is found, using all color spaces and log a warning.
        OCIO::LogGuard guard;
        params->setIncludeNamedTransforms(false);
        OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
        OCIO_CHECK_EQUAL(guard.output(),
            "[OpenColorIO Info]: All parameters could not be used to create the menu: Found no "
            "color space using user categories. Categories have been ignored since they matched "
            "no color spaces.\n");
        guard.clear();
        OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 16);
    }

    // Use a role.

    params->setRole(OCIO::ROLE_RENDERING);
    params->setAppCategories("");
    params->setIncludeRoles(false);

    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(),  1);

    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("lin_1"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(0), std::string("rendering (lin_1)"));
    OCIO_CHECK_EQUAL(menuHelper->getFamily(0), std::string(""));

    // Use an existing role and app-oriented categories: categories are ignored.

    params->setAppCategories("file-io");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(),  1);

    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("lin_1"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(0), std::string("rendering (lin_1)"));
    OCIO_CHECK_EQUAL(menuHelper->getFamily(0), std::string(""));

    // Use an existing role and include roles: include roles is ignored.

    params->setAppCategories("");
    params->setIncludeRoles(true);

    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 1);

    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("lin_1"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(0), std::string("rendering (lin_1)"));
    OCIO_CHECK_EQUAL(menuHelper->getFamily(0), std::string(""));

    // Using an unknown category or role returns all the color spaces.
    {
        OCIO::LogGuard guard;

        params->setIncludeRoles(false);
        params->setRole("");
        params->setAppCategories("unknown_category");

        // Return all the color spaces.
        OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
        OCIO_CHECK_EQUAL(guard.output(),
            "[OpenColorIO Info]: All parameters could not be used to create the menu: Found no "
            "color space using app categories. Found no color space using user categories. "
            "Categories have been ignored since they matched no color spaces.\n");
        guard.clear();
        OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 16);

        params->setAppCategories("");
        params->setRole("unknown_role");

        // Return all the color spaces.
        OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
        OCIO_CHECK_EQUAL(guard.output(),
            "[OpenColorIO Info]: All parameters could not be used to create the menu: Found no "
            "color space using user categories. Categories have been ignored since they matched "
            "no color spaces.\n");
        guard.clear();
        OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 16);
    }
}


OCIO_ADD_TEST(ColorSpaceMenuHelper, user_categories)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::ColorSpaceMenuHelperRcPtr menuHelper;
    auto params = OCIO::ColorSpaceMenuParameters::Create(config);

    // User categories can be used instead of app-oriented categories.

    params->setUserCategories("basic-2d");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 3);

    params->setUserCategories("advanced-2d");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 4);

    params->setUserCategories("basic-2d, advanced-2d");
    params->setIncludeNamedTransforms(true);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 9);

    // Env. variable overrides parameter.

    OCIO::Platform::Setenv(OCIO::OCIO_USER_CATEGORIES_ENVVAR, "basic-3d");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 1);
    OCIO::Platform::Unsetenv(OCIO::OCIO_USER_CATEGORIES_ENVVAR);

    //
    // Using both app-oriented categories and user categories.
    //

    // Intersection is used if non empty.

    params->setIncludeNamedTransforms(false);
    params->setAppCategories("file-io, working-space");
    params->setUserCategories("advanced-2d");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 3);
    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("in_2"));

    // Intersection is used if non empty, named transforms can be included.

    params->setIncludeNamedTransforms(true);
    params->setAppCategories("working-space");
    params->setUserCategories("basic-3d");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("nt1"));
    params->setIncludeNamedTransforms(false);

    // Intersection is empty. App-oriented categories are used as the fall-back.

    OCIO::LogGuard guard;
    params->setAppCategories("look-process-space");
    params->setUserCategories("advanced-3d");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(guard.output(),
        "[OpenColorIO Info]: All parameters could not be used to create the menu: Intersection "
        "of color spaces with app categories and color spaces with user categories is empty. User "
        "categories have been ignored.\n");
    guard.clear();
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 3);

    // Intersection leads to no results and there are no app-oriented category results.  Fall back
    // to user categories.

    params->setAppCategories("not a category, not used");
    params->setUserCategories("basic-2d, not used");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(guard.output(),
        "[OpenColorIO Info]: All parameters could not be used to create the menu: Found no "
        "color space using app categories.\n");
    guard.clear();
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 3);
}

OCIO_ADD_TEST(ColorSpaceMenuHelper, encodings)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::ColorSpaceMenuHelperRcPtr menuHelper;
    auto params = OCIO::ColorSpaceMenuParameters::Create(config);
    params->setAppCategories("file-io");
    params->setEncodings("sdr-video");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 2);

    params->setIncludeNamedTransforms(true);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 3);
    params->setIncludeNamedTransforms(false);

    OCIO::LogGuard guard;
    params->setEncodings("not found encoding");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(guard.output(),
        "[OpenColorIO Info]: All parameters could not be used to create the menu: Encodings "
        "have been ignored since they matched no color spaces.\n");
    guard.clear();
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 4);

    // If intersection is empty, encodings are ignored.

    params->setIncludeNamedTransforms(true);
    params->setAppCategories("working-space");
    params->setUserCategories("basic-3d");
    params->setEncodings("log");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(guard.output(),
        "[OpenColorIO Info]: All parameters could not be used to create the menu: Encodings have"
        " been ignored since they matched no color spaces.\n");
    guard.clear();
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("nt1"));

    // If intersection is empty (with and without encoding), user categories are ignored and
    // encodings are used.

    params->setIncludeNamedTransforms(true);
    params->setAppCategories("file-io");
    params->setUserCategories("basic-3d");
    params->setEncodings("sdr-video");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(guard.output(),
        "[OpenColorIO Info]: All parameters could not be used to create the menu: Intersection "
        "of color spaces with app categories and color spaces with user categories is empty. "
        "User categories have been ignored.\n");
    guard.clear();
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 3);
    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("in_1"));
    OCIO_CHECK_EQUAL(menuHelper->getName(1), std::string("in_2"));
    OCIO_CHECK_EQUAL(menuHelper->getName(2), std::string("nt3"));

    // Categories give no color space, all categories are ignored but encodings are used.

    params->setIncludeNamedTransforms(true);
    params->setAppCategories("no");
    params->setUserCategories("no");
    params->setEncodings("sdr-video");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(guard.output(),
        "[OpenColorIO Info]: All parameters could not be used to create the menu: Found no color "
        "space using app categories. Found no color space using user categories. Categories have "
        "been ignored since they matched no color spaces.\n");
    guard.clear();
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 5);
    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("in_1"));
    OCIO_CHECK_EQUAL(menuHelper->getName(1), std::string("in_2"));
    OCIO_CHECK_EQUAL(menuHelper->getName(2), std::string("display_lin_2"));
    OCIO_CHECK_EQUAL(menuHelper->getName(3), std::string("nt1"));
    OCIO_CHECK_EQUAL(menuHelper->getName(4), std::string("nt3"));

    // App-oriented categories is empty, but encodings are used. Intersection with user categories.

    params->setAppCategories("");
    params->setEncodings("sdr-video");
    params->setUserCategories("advanced-2d");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_CHECK_EQUAL(guard.output(), "");
    guard.clear();
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 2);
}

OCIO_ADD_TEST(ColorSpaceMenuHelper, no_category)
{
    std::istringstream is{ R"(ocio_profile_version: 1

environment:
  {}

search_path: luts
strictparsing: true

roles:
  rendering: test_1
  default: raw

displays:
  DISP_1:
    - !<View> {name: VIEW_1, colorspace: test_1}
    - !<View> {name: VIEW_2, colorspace: test_2}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: raw
    family: Raw
    description: A raw color space. Conversions to and from this space are no-ops.
    isdata: true
    allocation: uniform

  - !<ColorSpace>
    name: test_1

  - !<ColorSpace>
    name: test_2
 )" };

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::ColorSpaceMenuHelperRcPtr menuHelper;
    auto params = OCIO::ColorSpaceMenuParameters::Create(config);

    // Categories are ignored when config is version 1 and no message is logged.
    {
        OCIO::LogGuard guard;

        params->setAppCategories("file-io");

        // Return all the color spaces.
        OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
        OCIO_CHECK_EQUAL(guard.output(), "");
        OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 3);
    }
}

OCIO_ADD_TEST(ColorSpaceMenuHelper, input_color_transformation)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    //
    // Step 1 - Validate the selected input color spaces.
    //

    auto params = OCIO::ColorSpaceMenuParameters::Create(config);
    OCIO::ColorSpaceMenuHelperRcPtr inputMenuHelper;

    params->setAppCategories("file-io");

    OCIO_CHECK_NO_THROW(inputMenuHelper = OCIO::ColorSpaceMenuHelper::Create(params));

    OCIO_REQUIRE_EQUAL(inputMenuHelper->getNumColorSpaces(), 4);

    OCIO_CHECK_EQUAL(inputMenuHelper->getName(0), std::string("in_1"));
    OCIO_CHECK_EQUAL(inputMenuHelper->getName(1), std::string("in_2"));
    OCIO_CHECK_EQUAL(inputMenuHelper->getName(2), std::string("in_3"));
    OCIO_CHECK_EQUAL(inputMenuHelper->getName(3), std::string("lut_input_3"));

    // Some extra validation.

    {
        OCIO_REQUIRE_EQUAL(inputMenuHelper->getNumHierarchyLevels(0), 3);
        OCIO_CHECK_EQUAL(std::string(inputMenuHelper->getHierarchyLevel(0, 0)), "Input");
        OCIO_CHECK_EQUAL(std::string(inputMenuHelper->getHierarchyLevel(0, 1)), "Camera");
        OCIO_CHECK_EQUAL(std::string(inputMenuHelper->getHierarchyLevel(0, 2)), "Acme");

        OCIO_CHECK_EQUAL(std::string(inputMenuHelper->getDescription(0)),
                         "An input color space.\nFor the Acme camera.");
    }

    {
        OCIO_REQUIRE_EQUAL(inputMenuHelper->getNumHierarchyLevels(1), 0);
        OCIO_REQUIRE_EQUAL(std::string(inputMenuHelper->getDescription(1)), "");
    }

    //
    // Step 2 - Validate the selected working color spaces.
    //

    params->setAppCategories("working-space");
    OCIO::ColorSpaceMenuHelperRcPtr workingMenuHelper;
    OCIO_CHECK_NO_THROW(workingMenuHelper = OCIO::ColorSpaceMenuHelper::Create(params));

    OCIO_REQUIRE_EQUAL(workingMenuHelper->getNumColorSpaces(), 7);

    OCIO_CHECK_EQUAL(std::string(workingMenuHelper->getName(0)), "lin_1");
    OCIO_CHECK_EQUAL(std::string(workingMenuHelper->getName(1)), "lin_2");
    OCIO_CHECK_EQUAL(std::string(workingMenuHelper->getName(2)), "log_1");
    OCIO_CHECK_EQUAL(std::string(workingMenuHelper->getName(3)), "in_3");
    OCIO_CHECK_EQUAL(std::string(workingMenuHelper->getName(4)), "display_lin_1");
    OCIO_CHECK_EQUAL(std::string(workingMenuHelper->getName(5)), "display_lin_2");
    OCIO_CHECK_EQUAL(std::string(workingMenuHelper->getName(6)), "display_log_1");

    //
    // Step 3 - Validate the color transformation from in_1 to lin_2.
    //

    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = config->getProcessor(inputMenuHelper->getName(0),
                                                         workingMenuHelper->getName(1)));

    OCIO::GroupTransformRcPtr groupTransform;
    OCIO_CHECK_NO_THROW(groupTransform = processor->createGroupTransform());

    OCIO_CHECK_NO_THROW(groupTransform->validate());

    OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 1);

    {
        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));

        OCIO::ConstExponentTransformRcPtr exp
            = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(tr);
        OCIO_REQUIRE_ASSERT(exp);

        OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

        double values[4] = { -1 };
        exp->getValue(values);

        OCIO_CHECK_EQUAL(values[0], 2.6);
        OCIO_CHECK_EQUAL(values[1], 2.6);
        OCIO_CHECK_EQUAL(values[2], 2.6);
        OCIO_CHECK_EQUAL(values[3], 1.);
    }
}

OCIO_ADD_TEST(ColorSpaceMenuHelper, additional_color_space)
{
    // The unit test validates that a custom color transformation (i.e. an inactive one
    // or a newly created one not in the config instance) are correctly handled.

    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    // Use an arbitrary menu helper.

    auto params = OCIO::ColorSpaceMenuParameters::Create(config);
    OCIO::ColorSpaceMenuHelperRcPtr menuHelper;

    params->setAppCategories("file-io");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 4);

    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(0)), "in_1");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(1)), "in_2");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(2)), "in_3");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(3)), "lut_input_3");

    //
    // Add an additional color space to the menu.
    //

    // Note that it could be an inactive color space or an active color space not having one
    // of the selected categories.

    params->addColorSpace("lin_1");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 5);

    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(0)), "in_1");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(1)), "in_2");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(2)), "in_3");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(3)), "lut_input_3");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(4)), "lin_1");

    //
    // Add an additional color space that is already there: nothing gets added.
    //

    params->addColorSpace("in_2");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 5);

    //
    // Delete a color space and recreate the menu helper.
    //

    OCIO::ConfigRcPtr cfg = config->createEditableCopy();
    OCIO_CHECK_NO_THROW(cfg->removeColorSpace("in_1"));
    params->setConfig(cfg);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 4);

    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(0)), "in_2");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(1)), "in_3");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(2)), "lut_input_3");
    // And the additional color space is still present.
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(3)), "lin_1");

    // Additional color space are case insensitive.
    params->clearAddedColorSpaces();
    OCIO_CHECK_EQUAL(params->getNumAddedColorSpaces(), 0);
    params->addColorSpace("LIN_1");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    // Still get 4 items.
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 4);

    //
    // Same color space can't be added twice.
    //

    params->addColorSpace("lin_1");
    OCIO_CHECK_EQUAL(params->getNumAddedColorSpaces(), 1);
    params->addColorSpace("LIN_1");
    OCIO_CHECK_EQUAL(params->getNumAddedColorSpaces(), 1);
    params->clearAddedColorSpaces();

    //
    // Add a named transform.
    //

    params->addColorSpace("lin_1");
    params->addColorSpace("nt1");
    OCIO_CHECK_EQUAL(params->getNumAddedColorSpaces(), 2);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 5);
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(4)), "nt1");

    //
    // Add a role (first that refers to color space already there or not).
    //

    params->addColorSpace(OCIO::ROLE_RENDERING);
    OCIO_CHECK_EQUAL(params->getNumAddedColorSpaces(), 3);
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    // Color space is already there: nothing is added.
    OCIO_CHECK_EQUAL(menuHelper->getNumColorSpaces(), 5);

    params->addColorSpace("default");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 6);
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(5)), "raw");

    //
    // Add inactive color space.
    //

    params->clearAddedColorSpaces();
    params->setAppCategories("file-io");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 3);
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(0)), "in_2");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(1)), "in_3");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(2)), "lut_input_3");
    cfg->setInactiveColorSpaces("in_3");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 2);
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(0)), "in_2");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(1)), "lut_input_3");
    params->addColorSpace("in_3");
    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params));
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 3);
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(0)), "in_2");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(1)), "lut_input_3");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(2)), "in_3");

    //
    // Add a color space that does not exist.
    //

    params->addColorSpace("doesNotExist");
    OCIO_CHECK_THROW_WHAT(menuHelper = OCIO::ColorSpaceMenuHelper::Create(params), OCIO::Exception,
                          "Element 'doesNotExist' is neither a color space not a named transform");
}
