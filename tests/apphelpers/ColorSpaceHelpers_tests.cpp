// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "CategoryNames.h"
#include "ColorSpaceHelpers.h"
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
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    // Tests with 'in_1'.

    OCIO::ConstColorSpaceRcPtr cs;
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("in_1"));

    OCIO::ConstColorSpaceInfoRcPtr csInfo;
    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(config, cs));

    OCIO_CHECK_EQUAL(cs->getName(), std::string("in_1"));
    OCIO_CHECK_EQUAL(csInfo->getName(), std::string("in_1"));

    OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Input / Camera/Acme"));
    OCIO_REQUIRE_EQUAL(csInfo->getHierarchyLevels()->getNumString(), 3);
    OCIO_CHECK_EQUAL(csInfo->getHierarchyLevels()->getString(0), std::string("Input"));
    OCIO_CHECK_EQUAL(csInfo->getHierarchyLevels()->getString(1), std::string("Camera"));
    OCIO_CHECK_EQUAL(csInfo->getHierarchyLevels()->getString(2), std::string("Acme"));
    OCIO_CHECK_EQUAL(csInfo->getFamily(), std::string(cs->getFamily()));

    OCIO_CHECK_EQUAL(cs->getDescription(), std::string("An input color space.\nFor the Acme camera.\n"));
    OCIO_REQUIRE_EQUAL(csInfo->getDescriptions()->getNumString(), 2);
    OCIO_CHECK_EQUAL(csInfo->getDescriptions()->getString(0), std::string("An input color space."));
    OCIO_CHECK_EQUAL(csInfo->getDescriptions()->getString(1), std::string("For the Acme camera."));
    OCIO_CHECK_EQUAL(csInfo->getDescription(), std::string(cs->getDescription()));

    // Tests with 'lin_1'.

    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("lin_1"));
    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(config, cs));

    OCIO_CHECK_EQUAL(cs->getName(), std::string("lin_1"));
    OCIO_CHECK_EQUAL(csInfo->getName(), std::string("lin_1"));

    OCIO_CHECK_EQUAL(cs->getFamily(), std::string(""));
    OCIO_CHECK_EQUAL(csInfo->getHierarchyLevels()->getNumString(), 0);
    OCIO_CHECK_EQUAL(csInfo->getFamily(), std::string(""));

    OCIO_CHECK_EQUAL(cs->getDescription(), std::string(""));
    OCIO_CHECK_EQUAL(csInfo->getDescriptions()->getNumString(), 0);
    OCIO_CHECK_EQUAL(csInfo->getDescription(), std::string(""));
}

OCIO_ADD_TEST(ColorSpaceInfo, change_values)
{
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateRaw());
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO::ColorSpaceRcPtr cs;
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("raw")->createEditableCopy());

    OCIO::ConstColorSpaceInfoRcPtr csInfo;
    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(config, cs));

    OCIO_CHECK_EQUAL(cs->getName(), std::string("raw"));
    OCIO_CHECK_EQUAL(csInfo->getName(), std::string("raw"));

    OCIO_CHECK_EQUAL(cs->getFamily(), std::string("raw"));
    OCIO_REQUIRE_EQUAL(csInfo->getHierarchyLevels()->getNumString(), 1);
    OCIO_CHECK_EQUAL(csInfo->getHierarchyLevels()->getString(0), std::string("raw"));
    OCIO_CHECK_EQUAL(csInfo->getFamily(), std::string(cs->getFamily()));

    OCIO_CHECK_EQUAL(cs->getDescription(), 
                     std::string("A raw color space. Conversions to and from this space are no-ops."));
    OCIO_CHECK_EQUAL(csInfo->getDescriptions()->getNumString(), 1);
    OCIO_CHECK_EQUAL(csInfo->getDescriptions()->getString(0), 
                     std::string("A raw color space. Conversions to and from this space are no-ops."));
    OCIO_CHECK_EQUAL(csInfo->getDescription(), std::string(cs->getDescription()));

    // Change the family.

    OCIO_CHECK_NO_THROW(cs->setFamily(""));
    OCIO_CHECK_EQUAL(cs->getFamily(), std::string(""));

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(config, cs));
    OCIO_CHECK_EQUAL(csInfo->getHierarchyLevels()->getNumString(), 0);
    OCIO_CHECK_EQUAL(csInfo->getFamily(), std::string(""));

    OCIO_CHECK_NO_THROW(cs->setFamily("Acme     /   Camera"));
    OCIO_CHECK_EQUAL(cs->getFamily(), std::string("Acme     /   Camera"));

    // No family separator.

    OCIO_CHECK_EQUAL(config->getFamilySeparator(), 0x0);

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(config, cs));
    OCIO_REQUIRE_EQUAL(csInfo->getHierarchyLevels()->getNumString(), 1);
    OCIO_CHECK_EQUAL(csInfo->getHierarchyLevels()->getString(0), std::string(cs->getFamily()));


    OCIO::ConfigRcPtr cfg = config->createEditableCopy();

    // '/' is the new family separator.

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator('/'));

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, cs));
    OCIO_REQUIRE_EQUAL(csInfo->getHierarchyLevels()->getNumString(), 2);
    OCIO_CHECK_EQUAL(csInfo->getHierarchyLevels()->getString(0), std::string("Acme"));
    OCIO_CHECK_EQUAL(csInfo->getHierarchyLevels()->getString(1), std::string("Camera"));
    OCIO_CHECK_EQUAL(csInfo->getFamily(), std::string(cs->getFamily()));

    // '-' is the new family separator.

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator('-'));

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, cs));
    OCIO_REQUIRE_EQUAL(csInfo->getHierarchyLevels()->getNumString(), 1);
    OCIO_CHECK_EQUAL(csInfo->getHierarchyLevels()->getString(0), std::string(cs->getFamily()));

    // '' is the new family separator.

    OCIO_CHECK_NO_THROW(cfg->setFamilySeparator(0x0));

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, cs));
    OCIO_REQUIRE_EQUAL(csInfo->getHierarchyLevels()->getNumString(), 1);
    OCIO_CHECK_EQUAL(csInfo->getHierarchyLevels()->getString(0), std::string(cs->getFamily()));

    // Change the description.

    OCIO_CHECK_NO_THROW(cs->setDescription("desc 1\n\n\n desc 2\n"));
    OCIO_CHECK_EQUAL(cs->getDescription(), std::string("desc 1\n\n\n desc 2\n"));

    OCIO_CHECK_NO_THROW(csInfo = OCIO::ColorSpaceInfo::Create(cfg, cs));
    OCIO_REQUIRE_EQUAL(csInfo->getDescriptions()->getNumString(), 4);
    OCIO_CHECK_EQUAL(csInfo->getDescriptions()->getString(0), std::string("desc 1"));
    OCIO_CHECK_EQUAL(csInfo->getDescriptions()->getString(1), std::string(""));
    OCIO_CHECK_EQUAL(csInfo->getDescriptions()->getString(2), std::string(""));
    OCIO_CHECK_EQUAL(csInfo->getDescriptions()->getString(3), std::string("desc 2"));
    OCIO_CHECK_EQUAL(csInfo->getDescription(), std::string(cs->getDescription()));
}

OCIO_ADD_TEST(ColorSpaceMenuHelper, categories)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO::ColorSpaceMenuHelperRcPtr menuHelper;

    // Use the default categories.

    OCIO_CHECK_NO_THROW(
        menuHelper
            = OCIO::ColorSpaceMenuHelper::Create(config,
                                                 nullptr,
                                                 OCIO::ColorSpaceCategoryNames::Input));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 4);

    // Select 'rendering' and include roles.

    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(config, "rendering", nullptr));

    // Selected role supersedes the role adds.
    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(0), std::string("lin_1"));


    // Use custom categories.

    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(config, nullptr, "input"));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 4);

    // Use null categories & null role.

    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(config, nullptr, nullptr));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 12);  // All color spaces.

    // Use null categories with a role.

    OCIO_CHECK_NO_THROW(menuHelper = OCIO::ColorSpaceMenuHelper::Create(config, "rendering", nullptr));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 1);
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(0), std::string("lin_1"));

    // Use an arbitrary (but existing) category i.e. user could use some custom categories.

    OCIO_CHECK_NO_THROW(
        menuHelper = OCIO::ColorSpaceMenuHelper::Create(config, nullptr, "lut_input_space"));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 3);
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(0), std::string("lut_input_1"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(1), std::string("lut_input_2"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(2), std::string("lut_input_3"));

    // Use categories and a role.

    OCIO_CHECK_NO_THROW(
        menuHelper = OCIO::ColorSpaceMenuHelper::Create(config, "rendering", "lut_input_space"));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(),  1);

    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(0), std::string("lin_1"));

    // Using an unknown category returns all the color spaces.
    {
        OCIO::MuteLogging guard;

        // Return all the color spaces.
        OCIO_CHECK_NO_THROW(
            menuHelper = OCIO::ColorSpaceMenuHelper::Create(config, nullptr, "unknown_category"));
        OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 12);

        // Return all the color spaces.
        OCIO_CHECK_NO_THROW(
            menuHelper = OCIO::ColorSpaceMenuHelper::Create(config, "unknown_role", nullptr));
        OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 12);

        // Only return the color space associated to the role.
        OCIO_CHECK_NO_THROW(
            menuHelper = OCIO::ColorSpaceMenuHelper::Create(config, "rendering", "unknown_category"));
        OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 1);
    }
}

OCIO_ADD_TEST(ColorSpaceMenuHelper, input_color_transformation)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    //
    // Step 1 - Validate the selected input color spaces.
    //

    OCIO::ColorSpaceMenuHelperRcPtr inputMenuHelper;
    OCIO_CHECK_NO_THROW(
        inputMenuHelper
            = OCIO::ColorSpaceMenuHelper::Create(config,
                                                 nullptr,
                                                 OCIO::ColorSpaceCategoryNames::Input));

    OCIO_REQUIRE_EQUAL(inputMenuHelper->getNumColorSpaces(), 4);

    OCIO_CHECK_EQUAL(inputMenuHelper->getColorSpaceName(0), std::string("in_1"));
    OCIO_CHECK_EQUAL(inputMenuHelper->getColorSpaceName(1), std::string("in_2"));
    OCIO_CHECK_EQUAL(inputMenuHelper->getColorSpaceName(2), std::string("in_3"));
    OCIO_CHECK_EQUAL(inputMenuHelper->getColorSpaceName(3), std::string("lut_input_3"));

    // Some extra validation.

    {
        OCIO::ConstColorSpaceInfoRcPtr info = inputMenuHelper->getColorSpace(0);
        OCIO_CHECK_EQUAL(info->getName(), std::string("in_1"));
        OCIO::ConstStringsRcPtr hierarchyLevels = info->getHierarchyLevels();
        OCIO_REQUIRE_EQUAL(hierarchyLevels->getNumString(), 3);
        OCIO_CHECK_EQUAL(hierarchyLevels->getString(0), std::string("Input"));
        OCIO_CHECK_EQUAL(hierarchyLevels->getString(1), std::string("Camera"));
        OCIO_CHECK_EQUAL(hierarchyLevels->getString(2), std::string("Acme"));
        OCIO::ConstStringsRcPtr descriptions = info->getDescriptions();
        OCIO_REQUIRE_EQUAL(descriptions->getNumString(), 2);
        OCIO_CHECK_EQUAL(descriptions->getString(0), std::string("An input color space."));
        OCIO_CHECK_EQUAL(descriptions->getString(1), std::string("For the Acme camera."));
    }

    {
        OCIO::ConstColorSpaceInfoRcPtr info = inputMenuHelper->getColorSpace(1);
        OCIO_CHECK_EQUAL(info->getName(), std::string("in_2"));
        OCIO::ConstStringsRcPtr hierarchyLevels = info->getHierarchyLevels();
        OCIO_REQUIRE_EQUAL(hierarchyLevels->getNumString(), 0);
        OCIO::ConstStringsRcPtr descriptions = info->getDescriptions();
        OCIO_REQUIRE_EQUAL(descriptions->getNumString(), 0);
    }

    //
    // Step 2 - Validate the selected working color spaces.
    //

    OCIO::ColorSpaceMenuHelperRcPtr workingMenuHelper;
    OCIO_CHECK_NO_THROW(
        workingMenuHelper
            =   OCIO::ColorSpaceMenuHelper::Create(config,
                                                   nullptr,
                                                   OCIO::ColorSpaceCategoryNames::SceneLinearWorkingSpace));

    OCIO_REQUIRE_EQUAL(workingMenuHelper->getNumColorSpaces(), 2);

    OCIO_CHECK_EQUAL(workingMenuHelper->getColorSpaceName(0), std::string("lin_1"));
    OCIO_CHECK_EQUAL(workingMenuHelper->getColorSpaceName(1), std::string("lin_2"));

    //
    // Step 3 - Validate the color transformation from in_1 to lin_2.
    //

    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = config->getProcessor(inputMenuHelper->getColorSpaceName(0),
                                                         workingMenuHelper->getColorSpaceName(1)));

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
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    // Use an arbitrary menu helper.

    OCIO::ColorSpaceMenuHelperRcPtr menuHelper;
    OCIO_CHECK_NO_THROW(
        menuHelper
            = OCIO::ColorSpaceMenuHelper::Create(config,
                                                 nullptr,
                                                 OCIO::ColorSpaceCategoryNames::Input));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 4);

    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(0), std::string("in_1"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(1), std::string("in_2"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(2), std::string("in_3"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(3), std::string("lut_input_3"));

    //
    // Step 1 - Add an additional color space to the menu.
    //

    // Note that it could be an inactive color space or an active color space not having one
    // of the selected categories.
    OCIO::ConstColorSpaceRcPtr extraCS = config->getColorSpace("lin_1");
    OCIO::ConstColorSpaceInfoRcPtr extraCSInfo = OCIO::ColorSpaceInfo::Create(config, extraCS);

    OCIO_CHECK_NO_THROW(menuHelper->addColorSpaceToMenu(extraCSInfo));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 5);

    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(0), std::string("in_1"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(1), std::string("in_2"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(2), std::string("in_3"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(3), std::string("lut_input_3"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(4), std::string("lin_1"));

    //
    // Step 2 - Refresh the menu helper.
    //

    OCIO_CHECK_NO_THROW(menuHelper->refresh(config));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 5);

    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(0), std::string("in_1"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(1), std::string("in_2"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(2), std::string("in_3"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(3), std::string("lut_input_3"));
    // And the additional color space is still present.
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(4), std::string("lin_1"));

    //
    // Step 3 - Delete a color space and refresh the menu helper.
    //

    OCIO::ConfigRcPtr cfg = config->createEditableCopy();
    OCIO_CHECK_NO_THROW(cfg->removeColorSpace("in_1"));
    OCIO_CHECK_NO_THROW(menuHelper->refresh(cfg));

    OCIO_REQUIRE_EQUAL(menuHelper->getNumColorSpaces(), 4);

    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(0), std::string("in_2"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(1), std::string("in_3"));
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(2), std::string("lut_input_3"));
    // And the additional color space is still present.
    OCIO_CHECK_EQUAL(menuHelper->getColorSpaceName(3), std::string("lin_1"));
}

