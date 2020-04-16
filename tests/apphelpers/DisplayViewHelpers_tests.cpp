// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "CategoryNames.h"
#include "ColorSpaceHelpers.h"
#include "DisplayViewHelpers.h"
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


OCIO_ADD_TEST(DisplayViewHelpers, basic)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(cfg->sanityCheck());

    //
    // Step 1 - Validate the selected working color spaces.
    //

    OCIO::ColorSpaceMenuHelperRcPtr workingMenuHelper;
    OCIO_CHECK_NO_THROW(
        workingMenuHelper
            = OCIO::ColorSpaceMenuHelper::Create(cfg,
                                                 nullptr,
                                                 OCIO::ColorSpaceCategoryNames::SceneLinearWorkingSpace));

    OCIO_REQUIRE_EQUAL(workingMenuHelper->getNumColorSpaces(), 2);

    OCIO_CHECK_EQUAL(workingMenuHelper->getColorSpaceName(0), std::string("lin_1"));
    OCIO_CHECK_EQUAL(workingMenuHelper->getColorSpaceName(1), std::string("lin_2"));

    //
    // Step 2 - Validate the selected connection color spaces.
    //

    OCIO::ColorSpaceMenuHelperRcPtr connectionMenuHelper;
    OCIO_CHECK_NO_THROW(
        connectionMenuHelper
            = OCIO::ColorSpaceMenuHelper::Create(cfg,
                                                 nullptr,
                                                 OCIO::ColorSpaceCategoryNames::LutInputSpace));

    OCIO_REQUIRE_EQUAL(connectionMenuHelper->getNumColorSpaces(), 3);

    OCIO_CHECK_EQUAL(connectionMenuHelper->getColorSpaceName(0), std::string("lut_input_1"));
    OCIO_CHECK_EQUAL(connectionMenuHelper->getColorSpaceName(1), std::string("lut_input_2"));
    OCIO_CHECK_EQUAL(connectionMenuHelper->getColorSpaceName(2), std::string("lut_input_3"));

    //
    // Step 3 - Create a (display, view) pair.
    //

    OCIO::ConfigRcPtr config = cfg->createEditableCopy();

    OCIO::ConstColorSpaceInfoRcPtr csInfo
        = OCIO::ColorSpaceInfo::Create(config, "view_5", nullptr, nullptr);

    const std::string filePath(ocioTestFilesDir + "/lut1d_green.ctf");

    OCIO::FileTransformRcPtr userTransform;
    OCIO_CHECK_NO_THROW(userTransform = OCIO::FileTransform::Create());
    userTransform->setSrc(filePath.c_str());

    OCIO_CHECK_NO_THROW(
        OCIO::DisplayViewHelpers::AddDisplayView(config, 
                                                 "DISP_1", "VIEW_5", "look_3",
                                                 *csInfo, userTransform, 
                                                 "cat1, cat2", "lut_input_1"));
    // Keep in sync. the configs.
    cfg = config;

    //
    // Step 4 - Validate the new (display, view) pair.
    //

    const char * val = nullptr;
    OCIO_CHECK_NO_THROW(val = config->getView("DISP_1", 3));
    OCIO_CHECK_EQUAL(std::string(val), "VIEW_5");

    OCIO_CHECK_NO_THROW(val = config->getDisplayColorSpaceName("DISP_1", "VIEW_5"));
    OCIO_CHECK_EQUAL(std::string(val), "view_5");

    OCIO_CHECK_NO_THROW(val = config->getDisplayLooks("DISP_1", "VIEW_5"));
    OCIO_CHECK_EQUAL(std::string(val), "look_3");

    //
    // Step 5 - Check the newly created color space.
    //
    {
        OCIO::ConstColorSpaceRcPtr cs;
        OCIO_CHECK_NO_THROW(cs = config->getColorSpace(csInfo->getName()));
        OCIO_REQUIRE_ASSERT(cs);
        // These categories were not already used in the config, so AddDisplayView ignores them.
        OCIO_CHECK_ASSERT(!cs->hasCategory("cat1"));
        OCIO_CHECK_ASSERT(!cs->hasCategory("cat2"));
        OCIO_CHECK_EQUAL(cs->getFamily(), std::string(""));
        OCIO_CHECK_EQUAL(cs->getDescription(), std::string(""));
    }

    //
    // Step 6 - Create a processor for the new (display, view) pair.
    //
    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(
        processor = OCIO::DisplayViewHelpers::GetProcessor(cfg,
                                                           "lin_1", "DISP_1", "VIEW_5",
                                                           OCIO::TRANSFORM_DIR_FORWARD));

    OCIO::GroupTransformRcPtr groupTransform;
    OCIO_CHECK_NO_THROW(groupTransform = processor->createGroupTransform());

    OCIO_CHECK_NO_THROW(groupTransform->validate());

    OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 7);

    // The E/C op.
    {
        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(0));

        OCIO::ConstExposureContrastTransformRcPtr ex
            = OCIO::DynamicPtrCast<const OCIO::ExposureContrastTransform>(tr);
        OCIO_REQUIRE_ASSERT(ex);

        OCIO_CHECK_EQUAL(ex->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(ex->getStyle(), OCIO::EXPOSURE_CONTRAST_LINEAR);
        OCIO_CHECK_EQUAL(ex->getPivot(), 0.18);

        OCIO_CHECK_EQUAL(ex->getExposure(), 0.);
        OCIO_CHECK_ASSERT(ex->isExposureDynamic());

        OCIO_CHECK_EQUAL(ex->getContrast(), 1.);
        OCIO_CHECK_ASSERT(ex->isContrastDynamic());

        OCIO_CHECK_EQUAL(ex->getGamma(), 1.);
        OCIO_CHECK_ASSERT(!ex->isGammaDynamic());
    }

    // Working color space (i.e. lin_1) to the 'look' process color space (i.e. log_1).
    {
        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(1));

        OCIO::ConstLogTransformRcPtr log = OCIO::DynamicPtrCast<const OCIO::LogTransform>(tr);
        OCIO_REQUIRE_ASSERT(log);

        OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(log->getBase(), 2.);
    }

    // 'look' color processing i.e. look_3.
    {
        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(2));

        OCIO::ConstCDLTransformRcPtr cdl = OCIO::DynamicPtrCast<const OCIO::CDLTransform>(tr);
        OCIO_REQUIRE_ASSERT(cdl);

        OCIO_CHECK_EQUAL(cdl->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

        double rgb[3] = { -1 };
        OCIO_CHECK_NO_THROW(cdl->getSlope(rgb));
        OCIO_CHECK_EQUAL(rgb[0], 1.);
        OCIO_CHECK_EQUAL(rgb[1], 2.);
        OCIO_CHECK_EQUAL(rgb[2], 1.);

        OCIO_CHECK_NO_THROW(cdl->getPower(rgb));
        OCIO_CHECK_EQUAL(rgb[0], 1.);
        OCIO_CHECK_EQUAL(rgb[1], 1.);
        OCIO_CHECK_EQUAL(rgb[2], 1.);

        OCIO_CHECK_EQUAL(cdl->getSat(), 1.);
    }

    // 'look' process color space (i.e. log_1) to 'reference'.
    {
        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(3));

        OCIO::ConstLogTransformRcPtr log = OCIO::DynamicPtrCast<const OCIO::LogTransform>(tr);
        OCIO_REQUIRE_ASSERT(log);

        OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(log->getBase(), 2.);
    }

    // 'reference' to the display color space (i.e. view_3).
    {
        // The 'view_3' color space is a group transform containing:
        //  1. 'reference' to connection color space i.e. lut_1.
        //  2. The user 1D LUT.

        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(4));

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

        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(5));

        OCIO::ConstLut1DTransformRcPtr lut
            = OCIO::DynamicPtrCast<const OCIO::Lut1DTransform>(tr);
        OCIO_REQUIRE_ASSERT(lut);
        OCIO_CHECK_EQUAL(lut->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

        float r, g, b;
        OCIO_CHECK_NO_THROW(lut->getValue(0, r, g, b));
        OCIO_CHECK_EQUAL(r, 0.);
        OCIO_CHECK_EQUAL(g, 0.);
        OCIO_CHECK_EQUAL(b, 0.);

        OCIO_CHECK_NO_THROW(lut->getValue(1, r, g, b));
        OCIO_CHECK_EQUAL(r, 0.);
        OCIO_CHECK_CLOSE(g, 33.0f/1023.0f, 1e-8f);
        OCIO_CHECK_EQUAL(b, 0.);

        OCIO_CHECK_NO_THROW(lut->getValue(2, r, g, b));
        OCIO_CHECK_EQUAL(r, 0.);
        OCIO_CHECK_CLOSE(g, 66.0f/1023.0f, 1e-8f);
        OCIO_CHECK_EQUAL(b, 0.);
    }

    // The E/C op.
    {
        OCIO::ConstTransformRcPtr tr;
        OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(6));

        OCIO::ConstExposureContrastTransformRcPtr ex
            = OCIO::DynamicPtrCast<const OCIO::ExposureContrastTransform>(tr);
        OCIO_REQUIRE_ASSERT(ex);

        OCIO_CHECK_EQUAL(ex->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(ex->getStyle(), OCIO::EXPOSURE_CONTRAST_VIDEO);
        OCIO_CHECK_EQUAL(ex->getPivot(), 1.);

        OCIO_CHECK_EQUAL(ex->getExposure(), 0.);
        OCIO_CHECK_ASSERT(!ex->isExposureDynamic());

        OCIO_CHECK_EQUAL(ex->getContrast(), 1.);
        OCIO_CHECK_ASSERT(!ex->isContrastDynamic());

        OCIO_CHECK_EQUAL(ex->getGamma(), 1.);
        OCIO_CHECK_ASSERT(ex->isGammaDynamic());
    }

    //
    // Step 7 - Some faulty scenarios.
    //

    {
        // Color space already exists.
        OCIO_CHECK_THROW_WHAT(
            OCIO::DisplayViewHelpers::AddDisplayView(config, nullptr, "VIEW_4", "look_3",
                                                     *csInfo, userTransform, 
                                                     "cat1, cat2", "lut_input_1"),
            OCIO::Exception,
            "Color space name 'view_5' already exists.");
    }

    {
        OCIO::ConstColorSpaceInfoRcPtr info
            = OCIO::ColorSpaceInfo::Create(config, "view_51", nullptr, nullptr);

        // Display is null.
        OCIO_CHECK_THROW_WHAT(
            OCIO::DisplayViewHelpers::AddDisplayView(config, nullptr, "VIEW_4", "look_3",
                                                     *info, userTransform, 
                                                     "cat1, cat2", "lut_input_1"),
            OCIO::Exception,
            "Invalid display name.");
    }

    {
        OCIO::ConstColorSpaceInfoRcPtr info
            = OCIO::ColorSpaceInfo::Create(config, "view_51", nullptr, nullptr);

        // View is null.
        OCIO_CHECK_THROW_WHAT(
            OCIO::DisplayViewHelpers::AddDisplayView(config, "DISP_1", nullptr, "look_3",
                                                     *info, userTransform, 
                                                     "cat1, cat2", "lut_input_1"),
            OCIO::Exception,
            "Invalid view name.");
    }

    {
        OCIO::ConstColorSpaceInfoRcPtr info
            = OCIO::ColorSpaceInfo::Create(config, "view_51", nullptr, nullptr);

        // Connection CS does not exist.
        OCIO_CHECK_THROW_WHAT(
            OCIO::DisplayViewHelpers::AddDisplayView(config, "DISP_1", "VIEW_4", "look_3",
                                                     *info, userTransform, 
                                                     "cat1, cat2", "lut_unknown"),
            OCIO::Exception,
            "Connection color space name 'lut_unknown' does not exist.");
    }

    //
    // Step 8 - Remove the display/view.
    //

    OCIO_CHECK_NO_THROW(val = config->getView("DISP_1", 3));

    OCIO_CHECK_NO_THROW(OCIO::DisplayViewHelpers::RemoveDisplayView(config, "DISP_1", "VIEW_5"));

    OCIO_CHECK_ASSERT(!config->getColorSpace("view_5"));
    OCIO_CHECK_NO_THROW(val = config->getView("DISP_1", 2));
}

OCIO_ADD_TEST(DisplayViewHelpers, display_view_without_look)
{
    std::istringstream is(category_test_config);

    OCIO::ConstConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(cfg->sanityCheck());

    OCIO::ConstProcessorRcPtr processor;
    OCIO::GroupTransformRcPtr groupTransform;
    OCIO::ConstTransformRcPtr tr;
    OCIO::ConstExponentTransformRcPtr exp;


    // Forward direction.

    OCIO_CHECK_NO_THROW(
        processor = OCIO::DisplayViewHelpers::GetProcessor(cfg,
                                                           "lin_1", "DISP_1", "VIEW_1",
                                                           OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(groupTransform = processor->createGroupTransform());
    OCIO_CHECK_NO_THROW(groupTransform->validate());
    OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 3);

    OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(1));
    exp = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(tr);
    OCIO_REQUIRE_ASSERT(exp);
    OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    // Inverse direction.

    OCIO_CHECK_NO_THROW(
        processor = OCIO::DisplayViewHelpers::GetProcessor(cfg,
                                                           "lin_1", "DISP_1", "VIEW_1",
                                                           OCIO::TRANSFORM_DIR_INVERSE));

    OCIO_CHECK_NO_THROW(groupTransform = processor->createGroupTransform());
    OCIO_CHECK_NO_THROW(groupTransform->validate());
    OCIO_REQUIRE_EQUAL(groupTransform->getNumTransforms(), 3);

    OCIO_CHECK_NO_THROW(tr = groupTransform->getTransform(1));
    exp = OCIO::DynamicPtrCast<const OCIO::ExponentTransform>(tr);
    OCIO_REQUIRE_ASSERT(exp);
    OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
}

namespace
{

class ActiveGuard
{
public:
    ActiveGuard(const char * envvar, const char * value)
        :   m_envvar(envvar)
    {
        OCIO::SetEnvVariable(m_envvar.c_str(), value); 
    }
    
    ~ActiveGuard()
    {
        OCIO::SetEnvVariable(m_envvar.c_str(), "");
    }

private:
    std::string m_envvar;
};

}

OCIO_ADD_TEST(DisplayViewHelpers, active_display_view)
{
    std::istringstream is(category_test_config);

    OCIO::ConfigRcPtr cfg;
    OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    OCIO_CHECK_NO_THROW(cfg->sanityCheck());

    // Step 1 - Check the current status.

    OCIO_REQUIRE_EQUAL(cfg->getNumDisplays(), 2);
    OCIO_REQUIRE_EQUAL(cfg->getNumViews("DISP_1"), 3);
    OCIO_REQUIRE_EQUAL(cfg->getNumViews("DISP_2"), 4);

    // Step 2 - Add some active displays & views.

    OCIO_CHECK_NO_THROW(cfg->setActiveDisplays("DISP_1"));
    OCIO_CHECK_NO_THROW(cfg->setActiveViews("VIEW_3, VIEW_2"));

    OCIO_REQUIRE_EQUAL(cfg->getNumDisplays(), 1);
    OCIO_CHECK_EQUAL(cfg->getDisplay(0), std::string("DISP_1"));

    OCIO_REQUIRE_EQUAL(cfg->getNumViews("DISP_1"), 2);
    OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 0), std::string("VIEW_3"));
    OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 1), std::string("VIEW_2"));

    // Step 3 - Create a (display, view) pair.

    OCIO::ConstColorSpaceInfoRcPtr csInfo
        = OCIO::ColorSpaceInfo::Create(cfg, "VIEW_5", nullptr, nullptr);

    const std::string filePath(ocioTestFilesDir + "/lut1d_green.ctf");

    OCIO::FileTransformRcPtr userTransform;
    OCIO_CHECK_NO_THROW(userTransform = OCIO::FileTransform::Create());
    userTransform->setSrc(filePath.c_str());

    OCIO_CHECK_NO_THROW(
        OCIO::DisplayViewHelpers::AddDisplayView(cfg, 
                                                 "DISP_1", "VIEW_5", nullptr,
                                                 *csInfo, userTransform, 
                                                 "cat1, cat2", "lut_input_1"));

    // The active displays & views were correctly updated.
    OCIO_CHECK_EQUAL(cfg->getActiveDisplays(), std::string("DISP_1"));
    OCIO_CHECK_EQUAL(cfg->getActiveViews(), std::string("VIEW_3, VIEW_2, VIEW_5"));

    OCIO_REQUIRE_EQUAL(cfg->getNumDisplays(), 1);
    OCIO_CHECK_EQUAL(cfg->getDisplay(0), std::string("DISP_1"));

    OCIO_REQUIRE_EQUAL(cfg->getNumViews("DISP_1"), 3);
    OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 0), std::string("VIEW_3"));
    OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 1), std::string("VIEW_2"));
    OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 2), std::string("VIEW_5"));

    // Step 4 - Remove a (display, view) pair.

    OCIO_CHECK_NO_THROW(OCIO::DisplayViewHelpers::RemoveDisplayView(cfg, "DISP_1", "VIEW_5"));

    OCIO_CHECK_EQUAL(cfg->getActiveDisplays(), std::string("DISP_1"));
    OCIO_CHECK_EQUAL(cfg->getActiveViews(), std::string("VIEW_3, VIEW_2"));

    OCIO_REQUIRE_EQUAL(cfg->getNumDisplays(), 1);
    OCIO_CHECK_EQUAL(cfg->getDisplay(0), std::string("DISP_1"));

    OCIO_REQUIRE_EQUAL(cfg->getNumViews("DISP_1"), 2);
    OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 0), std::string("VIEW_3"));
    OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 1), std::string("VIEW_2"));

    // Step 5 - Reset active displays & views.

    OCIO_CHECK_NO_THROW(cfg->setActiveDisplays(""));
    OCIO_CHECK_NO_THROW(cfg->setActiveViews(""));

    OCIO_REQUIRE_EQUAL(cfg->getNumDisplays(), 2);
    OCIO_CHECK_EQUAL(cfg->getDisplay(0), std::string("DISP_1"));
    OCIO_CHECK_EQUAL(cfg->getDisplay(1), std::string("DISP_2"));

    OCIO_REQUIRE_EQUAL(cfg->getNumViews("DISP_1"), 3);
    OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 0), std::string("VIEW_1"));
    OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 1), std::string("VIEW_2"));
    OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 2), std::string("VIEW_3"));

    // Step 6 - Add some active displays.

    {
        ActiveGuard dispGuard("OCIO_ACTIVE_DISPLAYS", "DISP_1");

        // Grab the envvar value.
        is.seekg(std::ios_base::beg);
        OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(is)->createEditableCopy());

        OCIO_REQUIRE_EQUAL(cfg->getNumDisplays(), 1);
        OCIO_CHECK_EQUAL(cfg->getDisplay(0), std::string("DISP_1"));

        OCIO_REQUIRE_EQUAL(cfg->getNumViews("DISP_1"), 3);
        OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 0), std::string("VIEW_1"));
        OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 1), std::string("VIEW_2"));
        OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 2), std::string("VIEW_3"));

        OCIO_CHECK_THROW_WHAT(
            OCIO::DisplayViewHelpers::AddDisplayView(cfg, 
                                                     "DISP_5", "VIEW_5", nullptr,
                                                     *csInfo, userTransform, 
                                                     "cat1, cat2", "lut_input_1"),
            OCIO::Exception,
            "Forbidden to add an active display as 'OCIO_ACTIVE_DISPLAYS' controls the active list.");
    }

    // Step 7 - Add some active views.

    {
        ActiveGuard viewGuard("OCIO_ACTIVE_VIEWS", "VIEW_3, VIEW_2");

        // Grab the envvar value.
        is.seekg(std::ios_base::beg);
        OCIO_CHECK_NO_THROW(cfg = OCIO::Config::CreateFromStream(is)->createEditableCopy());

        OCIO_REQUIRE_EQUAL(cfg->getNumDisplays(), 2);
        OCIO_CHECK_EQUAL(cfg->getDisplay(0), std::string("DISP_1"));
        OCIO_CHECK_EQUAL(cfg->getDisplay(1), std::string("DISP_2"));

        OCIO_REQUIRE_EQUAL(cfg->getNumViews("DISP_1"), 2);
        OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 0), std::string("VIEW_3"));
        OCIO_CHECK_EQUAL(cfg->getView("DISP_1", 1), std::string("VIEW_2"));

        OCIO_CHECK_THROW_WHAT(
            OCIO::DisplayViewHelpers::AddDisplayView(cfg, 
                                                     "DISP_1", "VIEW_5", nullptr,
                                                     *csInfo, userTransform, 
                                                     "cat1, cat2", "lut_input_1"),
            OCIO::Exception,
            "Forbidden to add an active view as 'OCIO_ACTIVE_VIEWS' controls the active list.");
    }
}

OCIO_ADD_TEST(DisplayViewHelpers, remove_display_view)
{
    // Validate that a color space is removed or not depending of its usage i.e. color spaces used
    // by a ColorSpaceTransform for example. When removing a (display, view) pair the associated
    // color space is then removed only if not used.

    constexpr char CONFIG[] = 
        "ocio_profile_version: 2\n"
        "\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: cs1}\n"
        "    - !<View> {name: view2, colorspace: cs2}\n"
        "    - !<View> {name: view3, colorspace: cs3}\n"
        "    - !<View> {name: view4, colorspace: cs2}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs3\n"
        "    from_reference: !<ColorSpaceTransform> {src: cs2, dst: cs2}\n";

    std::istringstream iss;
    iss.str(CONFIG);

    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss)->createEditableCopy());
    OCIO_CHECK_NO_THROW(config->sanityCheck());
    OCIO_CHECK_EQUAL(config->getNumViews("disp1"), 4);

    // Remove a (display, view) pair.

    OCIO_CHECK_NO_THROW(OCIO::DisplayViewHelpers::RemoveDisplayView(config, "disp1", "view2"));
    OCIO_CHECK_EQUAL(config->getNumViews("disp1"), 3);
    // 'cs2' still exists because it's used by 'cs3' and the (disp1, view4) pair.
    OCIO::ConstColorSpaceRcPtr cs;
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("cs2"));
    OCIO_CHECK_ASSERT(cs);

    OCIO_CHECK_NO_THROW(OCIO::DisplayViewHelpers::RemoveDisplayView(config, "disp1", "view3"));
    OCIO_CHECK_EQUAL(config->getNumViews("disp1"), 2);
    // 'cs3' is removed because it was not used.
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("cs3"));
    OCIO_CHECK_ASSERT(!cs);

    OCIO_CHECK_NO_THROW(OCIO::DisplayViewHelpers::RemoveDisplayView(config, "disp1", "view4"));
    OCIO_CHECK_EQUAL(config->getNumViews("disp1"), 1);
    // 'cs2' is removed because it was not anymore used (i.e. 'cs3' is now removed).
    OCIO_CHECK_NO_THROW(cs = config->getColorSpace("cs2"));
    OCIO_CHECK_ASSERT(!cs);
}
