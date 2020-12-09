// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "Display.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(SharedViews, basic)
{
    // Shared views can not be used with v1 config.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(1);
    OCIO_CHECK_NO_THROW(config->addSharedView("shared1", "", "colorspace", "", "", ""));
    std::ostringstream oss;
    OCIO_CHECK_THROW_WHAT(config->serialize(oss), OCIO::Exception,
                          "Only version 2 (or higher) can have shared views");

    // Using a v2 config.
    config = OCIO::Config::CreateRaw()->createEditableCopy();
    OCIO_CHECK_NO_THROW(config->validate());

    // Shared views need to refer to existing colorspaces.
    OCIO_CHECK_NO_THROW(config->addSharedView("shared1", "", "colorspace1", "", "", ""));
    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                          "color space or a named transform, 'colorspace1', which is not defined");

    OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
    cs->setName("colorspace1");
    config->addColorSpace(cs);
    OCIO_CHECK_NO_THROW(config->validate());

    // Shared views need to refer to existing looks.
    cs->setName("colorspace2");
    config->addColorSpace(cs);
    OCIO_CHECK_NO_THROW(config->addSharedView("shared2", "", "colorspace2", "look1", "", ""));
    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                          "refers to a look, 'look1', which is not defined.");

    OCIO::LookRcPtr lk = OCIO::Look::Create();
    lk->setName("look1");
    lk->setProcessSpace("look1_process");
    cs->setName("look1_process");
    config->addColorSpace(cs);
    config->addLook(lk);
    OCIO_CHECK_NO_THROW(config->validate());

    // Shared views need to refer to existing view transforms.
    cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    cs->setName("colorspace3");
    config->addColorSpace(cs);
    OCIO_CHECK_NO_THROW(config->addSharedView("shared3", "viewTransform1", "colorspace3", "", "",
                                              "shared view description"));
    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                          "refers to a view transform, 'viewTransform1', which is neither a view "
                          "transform nor a named transform");

    OCIO::ViewTransformRcPtr vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    vt->setName("viewTransform1");
    OCIO_CHECK_NO_THROW(vt->setTransform(OCIO::MatrixTransform::Create(),
                                         OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE));
    OCIO_CHECK_NO_THROW(config->addViewTransform(vt));
    OCIO_CHECK_NO_THROW(config->validate());

    // Shared views need to refer to existing rules.
    OCIO_CHECK_NO_THROW(config->addSharedView("shared4", "", "colorspace1", "", "rule1", ""));
    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                          "viewing rule, 'rule1', which is not defined");

    OCIO::ViewingRulesRcPtr vrules = OCIO::ViewingRules::Create();
    OCIO_CHECK_NO_THROW(vrules->insertRule(0, "rule1"));
    OCIO_CHECK_NO_THROW(vrules->addColorSpace(0, "colorspace3"));

    OCIO_CHECK_NO_THROW(config->setViewingRules(vrules));
    OCIO_CHECK_NO_THROW(config->validate());

    // Add shared view with description.
    OCIO_CHECK_NO_THROW(config->addSharedView("shared5", "", "colorspace2", "", "",
                                              "Sample description"));
    OCIO_CHECK_NO_THROW(config->validate());

    // Add another view to the sRGB display (CreateRaw creates an sRGB display with a Raw view).
    OCIO_CHECK_NO_THROW(config->addDisplayView("sRGB", "view1", "colorspace1", ""));
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_CHECK_NO_THROW(config->addDisplaySharedView("sRGB", "shared2"));
    OCIO_CHECK_NO_THROW(config->addDisplaySharedView("sRGB", "shared3"));
    OCIO_CHECK_NO_THROW(config->addDisplaySharedView("sRGB", "shared4"));
    OCIO_CHECK_NO_THROW(config->validate());

    // Expecting five views: two DISPLAY_DEFINED views plus three SHARED views.
    OCIO_REQUIRE_EQUAL(5, config->getNumViews("sRGB"));
    OCIO_CHECK_EQUAL(std::string("Raw"), config->getView("sRGB", 0));
    OCIO_CHECK_EQUAL(std::string("view1"), config->getView("sRGB", 1));
    OCIO_CHECK_EQUAL(std::string("shared2"), config->getView("sRGB", 2));
    OCIO_CHECK_EQUAL(std::string("shared3"), config->getView("sRGB", 3));
    OCIO_CHECK_EQUAL(std::string("shared4"), config->getView("sRGB", 4));
    OCIO_REQUIRE_EQUAL(2, config->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "sRGB"));
    OCIO_CHECK_EQUAL(std::string("Raw"), config->getView(OCIO::VIEW_DISPLAY_DEFINED, "sRGB", 0));
    OCIO_CHECK_EQUAL(std::string("view1"), config->getView(OCIO::VIEW_DISPLAY_DEFINED, "sRGB", 1));
    OCIO_REQUIRE_EQUAL(3, config->getNumViews(OCIO::VIEW_SHARED, "sRGB"));
    OCIO_CHECK_EQUAL(std::string("shared2"), config->getView(OCIO::VIEW_SHARED, "sRGB", 0));
    OCIO_CHECK_EQUAL(std::string("shared3"), config->getView(OCIO::VIEW_SHARED, "sRGB", 1));
    OCIO_CHECK_EQUAL(std::string("shared4"), config->getView(OCIO::VIEW_SHARED, "sRGB", 2));

    // Access view properties (either display-defined or shared views).
    OCIO_CHECK_EQUAL(std::string("colorspace1"),
                     config->getDisplayViewColorSpaceName("sRGB", "view1"));
    OCIO_CHECK_EQUAL(std::string("colorspace2"),
                     config->getDisplayViewColorSpaceName("sRGB", "shared2"));
    OCIO_CHECK_EQUAL(std::string("viewTransform1"),
                     config->getDisplayViewTransformName("sRGB", "shared3"));
    OCIO_CHECK_EQUAL(std::string("look1"),
                     config->getDisplayViewLooks("sRGB", "shared2"));
    OCIO_CHECK_EQUAL(std::string("rule1"),
                     config->getDisplayViewRule("sRGB", "shared4"));
    OCIO_CHECK_EQUAL(std::string("shared view description"),
                     config->getDisplayViewDescription("sRGB", "shared3"));

    // A null or empty display name may be used to access shared views (regardless of whether
    // they are used in any displays).
    OCIO_CHECK_EQUAL(std::string("colorspace1"),
                     config->getDisplayViewColorSpaceName(nullptr, "shared1"));
    OCIO_CHECK_EQUAL(std::string("colorspace2"),
                     config->getDisplayViewColorSpaceName("", "shared2"));
    OCIO_CHECK_EQUAL(std::string("look1"),
                     config->getDisplayViewLooks(nullptr, "shared2"));
    OCIO_CHECK_EQUAL(std::string("viewTransform1"),
                     config->getDisplayViewTransformName(nullptr, "shared3"));
    OCIO_CHECK_EQUAL(std::string("colorspace3"),
                     config->getDisplayViewColorSpaceName(nullptr, "shared3"));
    OCIO_CHECK_EQUAL(std::string("rule1"),
                     config->getDisplayViewRule(nullptr, "shared4"));
    OCIO_CHECK_EQUAL(std::string("Sample description"),
                     config->getDisplayViewDescription(nullptr, "shared5"));

    // Use active views.
    config->setActiveViews("view1, shared3");
    OCIO_REQUIRE_EQUAL(2, config->getNumViews("sRGB"));
    OCIO_CHECK_EQUAL(std::string("view1"), config->getView("sRGB", 0));
    OCIO_CHECK_EQUAL(std::string("shared3"), config->getView("sRGB", 1));

    // Even if not active, view properties can be queried.
    OCIO_CHECK_EQUAL(std::string("look1"),
                     config->getDisplayViewLooks("sRGB", "shared2"));

    // These are not affected by active views.
    OCIO_REQUIRE_EQUAL(2, config->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "sRGB"));
    OCIO_REQUIRE_EQUAL(3, config->getNumViews(OCIO::VIEW_SHARED, "sRGB"));

    // Save and reload.
    std::ostringstream os;
    os << *config.get();
    auto configStr = os.str();
    std::istringstream back;
    back.str(configStr);
    auto configBack = OCIO::Config::CreateFromStream(back);

    // Verify reloaded version of config.
    OCIO_CHECK_EQUAL(config->getNumViews(OCIO::VIEW_SHARED, nullptr),
                     configBack->getNumViews(OCIO::VIEW_SHARED, nullptr));
    OCIO_CHECK_EQUAL(std::string("viewTransform1"),
                     configBack->getDisplayViewTransformName(nullptr, "shared3"));
    OCIO_CHECK_EQUAL(std::string("colorspace3"),
                     configBack->getDisplayViewColorSpaceName(nullptr, "shared3"));
    OCIO_CHECK_EQUAL(std::string("rule1"),
                     configBack->getDisplayViewRule(nullptr, "shared4"));
    OCIO_CHECK_EQUAL(std::string("Sample description"),
                     configBack->getDisplayViewDescription(nullptr, "shared5"));

    // Add view to display with name of existing shared view will throw.
    OCIO_CHECK_THROW_WHAT(config->addDisplayView("sRGB", "shared2", "colorspace1", ""), OCIO::Exception,
                          "There is already a shared view named 'shared2' in the display 'sRGB'");

    // Add shared view to a display with name of existing view will throw.
    // Shared1 is a shared view, but it is not used by sRGB, so a view with that name
    // can be added as a display-defined view.
    OCIO_CHECK_NO_THROW(config->addDisplayView("sRGB", "shared1", "colorspace1", ""));
    OCIO_CHECK_NO_THROW(config->validate());
    OCIO_CHECK_THROW_WHAT(config->addDisplaySharedView("sRGB", "shared1"), OCIO::Exception,
                          "There is already a view named 'shared1' in the display 'sRGB'");

    OCIO_CHECK_EQUAL(3, config->getNumViews(OCIO::VIEW_SHARED, "sRGB"));
    OCIO_CHECK_NO_THROW(config->validate());

    // Add undefined shared view.
    OCIO_CHECK_NO_THROW(config->addDisplaySharedView("sRGB", "shared42"));
    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                          "contains a shared view 'shared42' that is not defined");

    // Remove faulty view.
    OCIO_CHECK_NO_THROW(config->removeDisplayView("sRGB", "shared42"));
    OCIO_CHECK_NO_THROW(config->validate());

    // Remove unused shared view.
    OCIO_CHECK_NO_THROW(config->removeSharedView("shared1"));
    OCIO_CHECK_NO_THROW(config->validate());

    // Replace one of the existing shared views.  This time, it uses only a view transform and
    // special color space name. However, the config is missing a display color space having the
    // same name as the display.
    OCIO_CHECK_NO_THROW(config->addSharedView("shared3", "viewTransform1",
                                              OCIO::OCIO_VIEW_USE_DISPLAY_NAME,
                                              "", "", "shared view description"));

    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                          "The display 'sRGB' contains a shared view 'shared3' "
                          "which does not define a color space and there is no color space "
                          "that matches the display name");

    cs->setName("sRGB");
    config->addColorSpace(cs);
    OCIO_CHECK_NO_THROW(config->validate());

    // Verify that shared views with no color space are saved with a special display
    // color space name, and that they are properly loaded.
    os.clear();
    os.str("");

    os << *config.get();
    configStr = os.str();

    OCIO_CHECK_NE(std::string::npos, configStr.find(OCIO::OCIO_VIEW_USE_DISPLAY_NAME));

    back.clear();
    back.str(configStr);
    configBack = OCIO::Config::CreateFromStream(back);

    OCIO_CHECK_EQUAL(std::string(OCIO::OCIO_VIEW_USE_DISPLAY_NAME),
                     configBack->getDisplayViewColorSpaceName(nullptr, "shared3"));
}
