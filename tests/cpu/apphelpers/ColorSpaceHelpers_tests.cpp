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
    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(config, cs));

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
    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(config, cs));

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
    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(config, cs));

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

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(config, cs));
    OCIO_CHECK_EQUAL(csInfo->getNumHierarchyLevels(), 0);
    OCIO_CHECK_EQUAL(std::string(csInfo->getFamily()), "");

    OCIO_CHECK_NO_THROW(cs->setFamily("Acme     /   Camera"));
    OCIO_CHECK_EQUAL(std::string(cs->getFamily()), "Acme     /   Camera");

    OCIO::ConfigRcPtr cfg = config->createEditableCopy();

    // No family separator.

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator(0)); // i.e. that's the null character

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, cs));
    OCIO_REQUIRE_EQUAL(csInfo->getNumHierarchyLevels(), 1);
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(0)), cs->getFamily());


    // '/' is the new family separator.

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator('/'));

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, cs));
    OCIO_REQUIRE_EQUAL(csInfo->getNumHierarchyLevels(), 2);
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(0)), "Acme");
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(1)), "Camera");
    OCIO_CHECK_EQUAL(std::string(csInfo->getFamily()), cs->getFamily());

    // '-' is the new family separator.

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator('-'));

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, cs));
    OCIO_REQUIRE_EQUAL(csInfo->getNumHierarchyLevels(), 1);
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(0)), cs->getFamily());

    // Reset to the v2 default family separator i.e. default to '/'.

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator(OCIO::Config::GetDefaultFamilySeparator()));

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, cs));
    OCIO_REQUIRE_EQUAL(csInfo->getNumHierarchyLevels(), 2);
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(0)), "Acme");
    OCIO_CHECK_EQUAL(std::string(csInfo->getHierarchyLevel(1)), "Camera");
    OCIO_CHECK_EQUAL(std::string(csInfo->getFamily()), cs->getFamily());

    // Change the description.

    OCIO_CHECK_NO_THROW(cs->setDescription("desc 1\n\n\n desc 2"));
    OCIO_CHECK_EQUAL(cs->getDescription(), std::string("desc 1\n\n\n desc 2"));

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, cs));
    OCIO_CHECK_EQUAL(std::string(csInfo->getDescription()), cs->getDescription());
}

OCIO_ADD_TEST(ColorSpaceMenuHelper, categories)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::ColorSpaceMenuHelperRcPtr menuHelper;

    // Use the default categories.

    OCIO_CHECK_NO_THROW(menuHelper =
        OCIO::ColorSpaceMenuHelper::Create(config, nullptr,
                                           OCIO::Category::INPUT,
                                           OCIO::ColorSpaceMenuHelper::INCLUDE_NO_EXTRAS));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 4);

    // Use custom categories.

    OCIO_CHECK_NO_THROW(menuHelper =
        OCIO::ColorSpaceMenuHelper::Create(config, nullptr, "input",
                                           OCIO::ColorSpaceMenuHelper::INCLUDE_NO_EXTRAS));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 4);

    // Use null categories.

    OCIO_CHECK_NO_THROW(menuHelper =
        OCIO::ColorSpaceMenuHelper::Create(config, nullptr, nullptr,
                                           OCIO::ColorSpaceMenuHelper::INCLUDE_NO_EXTRAS));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 12);  // All color spaces.

    // Use an arbitrary (but existing) category i.e. user could use some custom categories.

    OCIO_CHECK_NO_THROW(menuHelper =
        OCIO::ColorSpaceMenuHelper::Create(config, nullptr, "lut_input_space",
                                           OCIO::ColorSpaceMenuHelper::INCLUDE_NO_EXTRAS));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 3);
    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("lut_input_1"));
    OCIO_CHECK_EQUAL(menuHelper->getName(1), std::string("lut_input_2"));
    OCIO_CHECK_EQUAL(menuHelper->getName(2), std::string("lut_input_3"));

    // Use an arbitrary category and include roles.

    OCIO_CHECK_NO_THROW(menuHelper =
        OCIO::ColorSpaceMenuHelper::Create(config, nullptr, "lut_input_space",
                                           OCIO::ColorSpaceMenuHelper::INCLUDE_ROLES));

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
    OCIO_CHECK_EQUAL(menuHelper->getUIName(3), std::string("default (lin_1)"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(4), std::string("reference (lin_1)"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(5), std::string("rendering (lin_1)"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(6), std::string("scene_linear (lin_1)"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(7), std::string(""));

    OCIO_CHECK_EQUAL(menuHelper->getIndexFromUIName("lut_input_1"), 0);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromUIName("lut_input_2"), 1);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromUIName("lut_input_3"), 2);
    OCIO_CHECK_EQUAL(menuHelper->getIndexFromUIName("default (lin_1)"), 3);
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

    // Use a role.

    OCIO_CHECK_NO_THROW(menuHelper =
        OCIO::ColorSpaceMenuHelper::Create(config, "rendering", nullptr,
                                           OCIO::ColorSpaceMenuHelper::INCLUDE_NO_EXTRAS));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(),  1);

    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("rendering"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(0), std::string("rendering (lin_1)"));
    OCIO_CHECK_EQUAL(menuHelper->getFamily(0), std::string(""));

    // Use an existing role and categories: categories are ignored.

    OCIO_CHECK_NO_THROW(menuHelper =
        OCIO::ColorSpaceMenuHelper::Create(config, "rendering", "lut_input_space",
                                           OCIO::ColorSpaceMenuHelper::INCLUDE_NO_EXTRAS));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(),  1);

    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("rendering"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(0), std::string("rendering (lin_1)"));
    OCIO_CHECK_EQUAL(menuHelper->getFamily(0), std::string(""));

    // Use an existing role and include flag: include flag is ignored.

    OCIO_CHECK_NO_THROW(menuHelper =
        OCIO::ColorSpaceMenuHelper::Create(config, "rendering", nullptr,
                                           OCIO::ColorSpaceMenuHelper::INCLUDE_ROLES));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 1);

    OCIO_CHECK_EQUAL(menuHelper->getName(0), std::string("rendering"));
    OCIO_CHECK_EQUAL(menuHelper->getUIName(0), std::string("rendering (lin_1)"));
    OCIO_CHECK_EQUAL(menuHelper->getFamily(0), std::string(""));


    // Using an unknown category or role returns all the color spaces.
    {
        OCIO::MuteLogging guard;

        // Return all the color spaces.
        OCIO_CHECK_NO_THROW(menuHelper =
            OCIO::ColorSpaceMenuHelper::Create(config, nullptr, "unknown_category",
                                               OCIO::ColorSpaceMenuHelper::INCLUDE_NO_EXTRAS));
        OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 12);

        // Return all the color spaces.
        OCIO_CHECK_NO_THROW(menuHelper =
            OCIO::ColorSpaceMenuHelper::Create(config, "unknown_role", nullptr,
                                               OCIO::ColorSpaceMenuHelper::INCLUDE_NO_EXTRAS));
        OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 12);
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

    OCIO::ColorSpaceMenuHelperRcPtr inputMenuHelper;
    OCIO_CHECK_NO_THROW(inputMenuHelper =
        OCIO::ColorSpaceMenuHelper::Create(config, nullptr, OCIO::Category::INPUT,
                                           OCIO::ColorSpaceMenuHelper::INCLUDE_NO_EXTRAS));

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

    OCIO::ColorSpaceMenuHelperRcPtr workingMenuHelper;
    OCIO_CHECK_NO_THROW(workingMenuHelper =
        OCIO::ColorSpaceMenuHelper::Create(config, nullptr,
                                           OCIO::Category::SCENE_LINEAR_WORKING_SPACE,
                                           OCIO::ColorSpaceMenuHelper::INCLUDE_NO_EXTRAS));

    OCIO_REQUIRE_EQUAL(workingMenuHelper->getNumColorSpaces(), 2);

    OCIO_CHECK_EQUAL(std::string(workingMenuHelper->getName(0)), "lin_1");
    OCIO_CHECK_EQUAL(std::string(workingMenuHelper->getName(1)), "lin_2");

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

    OCIO::ColorSpaceMenuHelperRcPtr menuHelper;

    OCIO_CHECK_NO_THROW(menuHelper =
        OCIO::ColorSpaceMenuHelper::Create(config, nullptr, OCIO::Category::INPUT,
                                           OCIO::ColorSpaceMenuHelper::INCLUDE_NO_EXTRAS));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 4);

    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(0)), "in_1");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(1)), "in_2");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(2)), "in_3");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(3)), "lut_input_3");

    //
    // Step 1 - Add an additional color space to the menu.
    //

    // Note that it could be an inactive color space or an active color space not having one
    // of the selected categories.

    OCIO_CHECK_NO_THROW(menuHelper->addColorSpaceToMenu("lin_1"));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 5);

    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(0)), "in_1");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(1)), "in_2");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(2)), "in_3");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(3)), "lut_input_3");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(4)), "lin_1");

    //
    // Step 2 - Refresh the menu helper.
    //

    OCIO_CHECK_NO_THROW(menuHelper->refresh(config));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 5);

    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(0)), "in_1");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(1)), "in_2");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(2)), "in_3");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(3)), "lut_input_3");
    // And the additional color space is still present.
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(4)), "lin_1");

    //
    // Step 3 - Delete a color space and refresh the menu helper.
    //

    OCIO::ConfigRcPtr cfg = config->createEditableCopy();
    OCIO_CHECK_NO_THROW(cfg->removeColorSpace("in_1"));
    OCIO_CHECK_NO_THROW(menuHelper->refresh(cfg));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 4);

    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(0)), "in_2");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(1)), "in_3");
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(2)), "lut_input_3");
    // And the additional color space is still present.
    OCIO_CHECK_EQUAL(std::string(menuHelper->getName(3)), "lin_1");
}
