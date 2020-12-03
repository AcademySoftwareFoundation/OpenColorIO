// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "apphelpers/MixingHelpers.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"


namespace OCIO = OCIO_NAMESPACE;


// The configuration file used by the unit tests.
#include "configs.data"


OCIO_ADD_TEST(MixingColorSpaceManager, basic)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::MixingColorSpaceManagerRcPtr mixingHelper;
    OCIO_CHECK_NO_THROW(mixingHelper = OCIO::MixingColorSpaceManager::Create(config));

    {
        OCIO::ConstProcessorRcPtr processor;
        OCIO_CHECK_NO_THROW(
            processor = mixingHelper->getProcessor("lin_1", "DISP_1", "VIEW_1",
                                                   OCIO::TRANSFORM_DIR_FORWARD));

        OCIO::GroupTransformRcPtr groupTransform;
        OCIO_CHECK_NO_THROW(groupTransform = processor->createGroupTransform());
        OCIO_CHECK_NO_THROW(groupTransform->validate());
        OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 1);

        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));

        OCIO_REQUIRE_ASSERT(OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr));
    }

    OCIO_CHECK_EQUAL(mixingHelper->getSelectedMixingEncodingIdx(), 0);
    OCIO_CHECK_EQUAL(mixingHelper->getNumMixingEncodings(), 2);

    OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingEncoding("HSV"));
    OCIO_CHECK_EQUAL(mixingHelper->getSelectedMixingEncodingIdx(), 1);
    OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingEncodingIdx(0));
    OCIO_CHECK_EQUAL(mixingHelper->getSelectedMixingEncodingIdx(), 0);

    OCIO_CHECK_THROW(mixingHelper->setSelectedMixingEncoding("HS"), OCIO::Exception);

    OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingEncodingIdx(1)); // i.e. HSV

    {
        OCIO::ConstProcessorRcPtr processor;
        OCIO_CHECK_NO_THROW(
            processor = mixingHelper->getProcessor("lin_1", "DISP_1", "VIEW_1",
                                                   OCIO::TRANSFORM_DIR_FORWARD));

        OCIO::GroupTransformRcPtr groupTransform;
        OCIO_CHECK_NO_THROW(groupTransform = processor->createGroupTransform());
        OCIO_CHECK_NO_THROW(groupTransform->validate());
        OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 2);

        {
            OCIO::ConstTransformRcPtr tr;
            OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));
            OCIO_REQUIRE_ASSERT(OCIO::DynamicPtrCast<const OCIO::MatrixTransform>(tr));

            OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(1));
            OCIO::ConstFixedFunctionTransformRcPtr ff
                = OCIO::DynamicPtrCast<const OCIO::FixedFunctionTransform>(tr);
            OCIO_REQUIRE_ASSERT(ff);

            OCIO_CHECK_EQUAL(ff->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
            OCIO_CHECK_EQUAL(ff->getStyle(), OCIO::FIXED_FUNCTION_RGB_TO_HSV);
        }
    }

    OCIO_CHECK_EQUAL(mixingHelper->getSelectedMixingSpaceIdx(), 0);
    OCIO_CHECK_EQUAL(mixingHelper->getNumMixingSpaces(), 2);

    OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingSpace("Display Space"));
    OCIO_CHECK_EQUAL(mixingHelper->getSelectedMixingSpaceIdx(), 1);
    OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingSpaceIdx(0));
    OCIO_CHECK_EQUAL(mixingHelper->getSelectedMixingSpaceIdx(), 0);

    OCIO_CHECK_THROW(mixingHelper->setSelectedMixingSpace("DisplaySpace"), OCIO::Exception);

    OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingSpaceIdx(1)); // i.e. 'Display Space'

    {
        OCIO::ConstProcessorRcPtr processor;
        OCIO_CHECK_NO_THROW(
            processor = mixingHelper->getProcessor("lin_1", "DISP_1", "VIEW_1",
                                                   OCIO::TRANSFORM_DIR_FORWARD));

        OCIO::GroupTransformRcPtr groupTransform;
        OCIO_CHECK_NO_THROW(groupTransform = processor->createGroupTransform());
        OCIO_CHECK_NO_THROW(groupTransform->validate());
        OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 2);

        OCIO::ConstTransformRcPtr tr;

        {
            OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));

            OCIO::ConstExponentTransformRcPtr exp
                = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(tr);
            OCIO_REQUIRE_ASSERT(exp);

            OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

            double values[4] = { -1 };
            exp->getValue(values);

            OCIO_CHECK_EQUAL(values[0], 2.6);
            OCIO_CHECK_EQUAL(values[1], 2.6);
            OCIO_CHECK_EQUAL(values[2], 2.6);
            OCIO_CHECK_EQUAL(values[3], 1.);
        }

        {
            OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(1));

            OCIO::ConstFixedFunctionTransformRcPtr ff
                = OCIO::DynamicPtrCast<const OCIO::FixedFunctionTransform>(tr);
            OCIO_REQUIRE_ASSERT(ff);

            OCIO_CHECK_EQUAL(ff->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
            OCIO_CHECK_EQUAL(ff->getStyle(), OCIO::FIXED_FUNCTION_RGB_TO_HSV);
        }
    }
}

OCIO_ADD_TEST(MixingColorSpaceManager, color_picker_role)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::MixingColorSpaceManagerRcPtr mixingHelper;
    OCIO_CHECK_NO_THROW(mixingHelper = OCIO::MixingColorSpaceManager::Create(config));
    OCIO_CHECK_EQUAL(mixingHelper->getNumMixingSpaces(), 2);

    // Add a color_picking role.
    OCIO::ConfigRcPtr cfg = config->createEditableCopy();
    OCIO_CHECK_ASSERT(!cfg->hasRole(OCIO::ROLE_COLOR_PICKING));
    OCIO_CHECK_NO_THROW(cfg->setRole(OCIO::ROLE_COLOR_PICKING, "log_1"));

    // The config changes so refresh the templates.
    OCIO_CHECK_NO_THROW(mixingHelper->refresh(cfg));
    OCIO_CHECK_EQUAL(mixingHelper->getNumMixingSpaces(), 1);
    OCIO_CHECK_EQUAL(mixingHelper->getMixingSpaceUIName(0),
                     std::string("color_picking (log_1)"));

    {
        OCIO::ConstProcessorRcPtr processor;
        OCIO_CHECK_NO_THROW(
            processor = mixingHelper->getProcessor("lin_1", "DISP_1", "VIEW_1",
                                                   OCIO::TRANSFORM_DIR_FORWARD));

        OCIO::GroupTransformRcPtr groupTransform;
        OCIO_CHECK_NO_THROW(groupTransform = processor->createGroupTransform());
        OCIO_CHECK_NO_THROW(groupTransform->validate());
        OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 1);

        {
            OCIO::ConstTransformRcPtr tr;
            OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));

            OCIO::ConstLogTransformRcPtr log = OCIO::DynamicPtrCast<const OCIO::LogTransform>(tr);
            OCIO_REQUIRE_ASSERT(log);

            OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
            OCIO_CHECK_EQUAL(log->getBase(), 2.);
        }
    }

    OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingEncodingIdx(1)); // i.e. HSV

    {
        OCIO::ConstProcessorRcPtr processor;
        OCIO_CHECK_NO_THROW(
            processor = mixingHelper->getProcessor("lin_1", "DISP_1", "VIEW_1",
                                                   OCIO::TRANSFORM_DIR_FORWARD));

        OCIO::GroupTransformRcPtr groupTransform;
        OCIO_CHECK_NO_THROW(groupTransform = processor->createGroupTransform());
        OCIO_CHECK_NO_THROW(groupTransform->validate());
        OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 2);

        {
            OCIO::ConstTransformRcPtr tr;
            OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));

            OCIO::ConstLogTransformRcPtr log
                = OCIO::DynamicPtrCast<const OCIO::LogTransform>(tr);
            OCIO_REQUIRE_ASSERT(log);

            OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
            OCIO_CHECK_EQUAL(log->getBase(), 2.);
        }

        {
            OCIO::ConstTransformRcPtr tr;
            OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(1));

            OCIO::ConstFixedFunctionTransformRcPtr ff
                = OCIO::DynamicPtrCast<const OCIO::FixedFunctionTransform>(tr);
            OCIO_REQUIRE_ASSERT(ff);

            OCIO_CHECK_EQUAL(ff->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
            OCIO_CHECK_EQUAL(ff->getStyle(), OCIO::FIXED_FUNCTION_RGB_TO_HSV);
        }
    }

    OCIO_CHECK_THROW_WHAT(mixingHelper->setSelectedMixingSpaceIdx(1), // i.e. Display
                          OCIO::Exception,
                          "Invalid idx for the mixing space index 1 where size is 1.");
}


// Helper macro.
#define FLOAT_CHECK_EQUAL(a, b) OCIO_CHECK_EQUAL(int(a), int((b)*100000.))


OCIO_ADD_TEST(MixingSlider, basic)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::MixingColorSpaceManagerRcPtr mixingHelper;
    OCIO_CHECK_NO_THROW(mixingHelper = OCIO::MixingColorSpaceManager::Create(config));

    OCIO::MixingSlider & slider = mixingHelper->getSlider(0.0f, 1.0f);

    OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingEncodingIdx(1)); // i.e. HSV
    {
        // Needs linear to perceptually linear adjustment.

        OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingSpaceIdx(0)); // i.e. Rendering Space
        OCIO_CHECK_EQUAL(mixingHelper->getSelectedMixingSpaceIdx(), 0);

        slider.setSliderMinEdge(0.0f);
        slider.setSliderMaxEdge(1.0f);

        FLOAT_CHECK_EQUAL(    0, slider.getSliderMinEdge());
        FLOAT_CHECK_EQUAL(83386, slider.getSliderMaxEdge());

        FLOAT_CHECK_EQUAL(37923, slider.mixingToSlider(0.1f));
        FLOAT_CHECK_EQUAL(80144, slider.mixingToSlider(0.5f));

        FLOAT_CHECK_EQUAL(10000, slider.sliderToMixing(0.379232f));
        FLOAT_CHECK_EQUAL(50000, slider.sliderToMixing(0.801448f));

        slider.setSliderMinEdge(-0.2f);
        slider.setSliderMaxEdge(5.f);

        FLOAT_CHECK_EQUAL( 3792, slider.mixingToSlider(-0.1f));
        FLOAT_CHECK_EQUAL(31573, slider.mixingToSlider( 0.1f));
        FLOAT_CHECK_EQUAL(58279, slider.mixingToSlider( 0.5f));
        FLOAT_CHECK_EQUAL(90744, slider.mixingToSlider( 3.0f));

        FLOAT_CHECK_EQUAL(-10000, slider.sliderToMixing(0.037927f));
        FLOAT_CHECK_EQUAL( 10000, slider.sliderToMixing(0.315733f));
        FLOAT_CHECK_EQUAL( 50000, slider.sliderToMixing(0.582797f));
        FLOAT_CHECK_EQUAL(300000, slider.sliderToMixing(0.907444f));

        // Does not need any linear to perceptually linear adjustment.

        OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingSpaceIdx(1)); // i.e. Display Space
        OCIO_CHECK_EQUAL(mixingHelper->getSelectedMixingSpaceIdx(), 1);

        slider.setSliderMinEdge(0.0f);
        slider.setSliderMaxEdge(1.0f);

        FLOAT_CHECK_EQUAL(     0, slider.getSliderMinEdge());
        FLOAT_CHECK_EQUAL(100000, slider.getSliderMaxEdge());

        FLOAT_CHECK_EQUAL(10000, slider.mixingToSlider(0.1f));
        FLOAT_CHECK_EQUAL(50000, slider.mixingToSlider(0.5f));

        FLOAT_CHECK_EQUAL(37923, slider.sliderToMixing(0.379232f));
        FLOAT_CHECK_EQUAL(80144, slider.sliderToMixing(0.801448f));

        slider.setSliderMinEdge(-0.2f);
        slider.setSliderMaxEdge(5.f);

        FLOAT_CHECK_EQUAL(     0, slider.mixingToSlider(slider.getSliderMinEdge()));
        FLOAT_CHECK_EQUAL(100000, slider.mixingToSlider(slider.getSliderMaxEdge()));

        FLOAT_CHECK_EQUAL( 1923, slider.mixingToSlider(-0.1f));
        FLOAT_CHECK_EQUAL( 5769, slider.mixingToSlider( 0.1f));
        FLOAT_CHECK_EQUAL(13461, slider.mixingToSlider( 0.5f));
        FLOAT_CHECK_EQUAL(61538, slider.mixingToSlider( 3.0f));

        FLOAT_CHECK_EQUAL( -277,  slider.sliderToMixing(0.037927f));
        FLOAT_CHECK_EQUAL(144181, slider.sliderToMixing(0.315733f));
        FLOAT_CHECK_EQUAL(283054, slider.sliderToMixing(0.582797f));
        FLOAT_CHECK_EQUAL(451870, slider.sliderToMixing(0.907444f));
    }

    OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingEncodingIdx(0)); // i.e. RGB
    {
        // Needs linear to perceptually linear adjustment.

        OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingSpaceIdx(0)); // i.e. Rendering Space
        OCIO_CHECK_EQUAL(mixingHelper->getSelectedMixingSpaceIdx(), 0);

        slider.setSliderMinEdge(0.0f);
        slider.setSliderMaxEdge(1.0f);

        FLOAT_CHECK_EQUAL(    0, slider.getSliderMinEdge());
        FLOAT_CHECK_EQUAL(83386, slider.getSliderMaxEdge());

        FLOAT_CHECK_EQUAL(37923, slider.mixingToSlider(0.1f));
        FLOAT_CHECK_EQUAL(80144, slider.mixingToSlider(0.5f));

        FLOAT_CHECK_EQUAL(10000, slider.sliderToMixing(0.379232f));
        FLOAT_CHECK_EQUAL(50000, slider.sliderToMixing(0.801448f));

        slider.setSliderMinEdge(-0.2f);
        slider.setSliderMaxEdge(5.f);

        FLOAT_CHECK_EQUAL( 3792, slider.mixingToSlider(-0.1f));
        FLOAT_CHECK_EQUAL(31573, slider.mixingToSlider( 0.1f));
        FLOAT_CHECK_EQUAL(58279, slider.mixingToSlider( 0.5f));
        FLOAT_CHECK_EQUAL(90744, slider.mixingToSlider( 3.0f));

        FLOAT_CHECK_EQUAL(-10000, slider.sliderToMixing(0.037927f));
        FLOAT_CHECK_EQUAL( 10000, slider.sliderToMixing(0.315733f));
        FLOAT_CHECK_EQUAL( 50000, slider.sliderToMixing(0.582797f));
        FLOAT_CHECK_EQUAL(300000, slider.sliderToMixing(0.907444f));

        // Does not need any linear to perceptually linear adjustment.

        OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingSpaceIdx(1)); // i.e. Display Space
        OCIO_CHECK_EQUAL(mixingHelper->getSelectedMixingSpaceIdx(), 1);

        slider.setSliderMinEdge(0.0f);
        slider.setSliderMaxEdge(1.0f);

        FLOAT_CHECK_EQUAL(     0, slider.getSliderMinEdge());
        FLOAT_CHECK_EQUAL(100000, slider.getSliderMaxEdge());

        FLOAT_CHECK_EQUAL(10000, slider.mixingToSlider(0.1f));
        FLOAT_CHECK_EQUAL(50000, slider.mixingToSlider(0.5f));

        FLOAT_CHECK_EQUAL(37923, slider.sliderToMixing(0.379232f));
        FLOAT_CHECK_EQUAL(80144, slider.sliderToMixing(0.801448f));

        slider.setSliderMinEdge(-0.2f);
        slider.setSliderMaxEdge(5.f);

        FLOAT_CHECK_EQUAL( 1923, slider.mixingToSlider(-0.1f));
        FLOAT_CHECK_EQUAL( 5769, slider.mixingToSlider( 0.1f));
        FLOAT_CHECK_EQUAL(13461, slider.mixingToSlider( 0.5f));
        FLOAT_CHECK_EQUAL(61538, slider.mixingToSlider( 3.0f));

        FLOAT_CHECK_EQUAL( -277,  slider.sliderToMixing(0.037927f));
        FLOAT_CHECK_EQUAL(144181, slider.sliderToMixing(0.315733f));
        FLOAT_CHECK_EQUAL(283054, slider.sliderToMixing(0.582797f));
        FLOAT_CHECK_EQUAL(451870, slider.sliderToMixing(0.907444f));
    }
}

OCIO_ADD_TEST(MixingSlider, color_picker_role)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO::MixingColorSpaceManagerRcPtr mixingHelper;
    OCIO_CHECK_NO_THROW(mixingHelper = OCIO::MixingColorSpaceManager::Create(config));

    // Add the color_picking role.
    OCIO::ConfigRcPtr cfg = config->createEditableCopy();
    OCIO_CHECK_ASSERT(!cfg->hasRole(OCIO::ROLE_COLOR_PICKING));
    OCIO_CHECK_NO_THROW(cfg->setRole(OCIO::ROLE_COLOR_PICKING, "lin_1"));

    // Refresh the templates as the config changed.
    OCIO_CHECK_NO_THROW(mixingHelper->refresh(cfg));

    OCIO_CHECK_EQUAL(mixingHelper->getNumMixingSpaces(), 1);
    OCIO_CHECK_EQUAL(std::string(mixingHelper->getMixingSpaceUIName(0)), 
                     std::string("color_picking (lin_1)"));

    OCIO_CHECK_THROW_WHAT(mixingHelper->setSelectedMixingSpaceIdx(1),
                          OCIO::Exception,
                          "Invalid idx for the mixing space index 1 where size is 1.");


    OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingEncodingIdx(1)); // i.e. HSV
    OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingSpaceIdx(0));    // i.e. Color Picker role

    OCIO::MixingSlider & slider = mixingHelper->getSlider(0.0f, 1.0f);
    FLOAT_CHECK_EQUAL(50501, slider.mixingToSlider(0.50501f));
    FLOAT_CHECK_EQUAL(50501, slider.sliderToMixing(0.50501f));

    OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingEncodingIdx(0)); // i.e. RGB
    OCIO_CHECK_NO_THROW(mixingHelper->setSelectedMixingSpaceIdx(0));    // i.e. Color Picker role

    FLOAT_CHECK_EQUAL(50501, slider.mixingToSlider(0.50501f));
    FLOAT_CHECK_EQUAL(50501, slider.sliderToMixing(0.50501f));
}
