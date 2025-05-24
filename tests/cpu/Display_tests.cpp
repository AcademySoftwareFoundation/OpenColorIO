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

    // Remove all shared views
    OCIO_REQUIRE_EQUAL(4, config->getNumViews(OCIO::VIEW_SHARED, nullptr));
    OCIO_CHECK_NO_THROW(config->clearSharedViews());
    OCIO_REQUIRE_EQUAL(0, config->getNumViews(OCIO::VIEW_SHARED, nullptr));
}

OCIO_ADD_TEST(Config, compare_displays) {
    static constexpr char CONFIG1[]{ R"(ocio_profile_version: 2

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

shared_views:
  - !<View> {name: sview1, colorspace: raw}

displays:
  Raw:
    - !<View> {name: Raw, colorspace: raw}
  sRGB:
    - !<View> {name: Raw, colorspace: raw}
    - !<View> {name: view, view_transform: display_vt, display_colorspace: display_cs}
    - !<Views> [sview1]

active_displays: [sRGB]
active_views: [view, sview1]

view_transforms:
  - !<ViewTransform>
    name: default_vt
    to_scene_reference: !<CDLTransform> {sat: 1.5}

  - !<ViewTransform>
    name: display_vt
    to_display_reference: !<CDLTransform> {sat: 1.5}

display_colorspaces:
  - !<ColorSpace>
    name: display_cs
    to_display_reference: !<CDLTransform> {sat: 1.5}

colorspaces:
  - !<ColorSpace>
    name: raw
)" };

    static constexpr char CONFIG2[]{ R"(ocio_profile_version: 2

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

shared_views:
  - !<View> {name: view, view_transform: display_vt, display_colorspace: display_cs}
  - !<View> {name: sview1, colorspace: raw}

displays:
  Raw:
    - !<View> {name: Raw, colorspace: raw}
  sRGB:
    - !<View> {name: Raw, colorspace: raw}
    - !<Views> [view, sview1]

active_displays: [Raw]
active_views: [Raw]

view_transforms:
  - !<ViewTransform>
    name: default_vt
    to_scene_reference: !<CDLTransform> {sat: 1.5}

  - !<ViewTransform>
    name: display_vt
    to_display_reference: !<CDLTransform> {sat: 1.5}

display_colorspaces:
  - !<ColorSpace>
    name: display_cs
    to_display_reference: !<CDLTransform> {sat: 1.5}

colorspaces:
  - !<ColorSpace>
    name: raw
)" };

    std::istringstream is;
    is.str(CONFIG1);
    OCIO::ConstConfigRcPtr config1, config2;
    OCIO_CHECK_NO_THROW(config1 = OCIO::Config::CreateFromStream(is));
    is.clear();
    is.str(CONFIG2);
    OCIO_CHECK_NO_THROW(config2 = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config1->validate());
    OCIO_CHECK_NO_THROW(config2->validate());

    {
        // Test that Config::ViewsAreEqual works for a matching (display, view) pair across separate configs.
        // Works regardless of if the view is display-defined in one config and shared in the other.
        // Works regardless of if the (display, view) pair is active in one config and inactive in another.

        // Active (display, view) pair where the view is display-defined.
        OCIO_REQUIRE_EQUAL(1, config1->getNumDisplays());
        OCIO_REQUIRE_EQUAL(std::string("sRGB"), config1->getDefaultDisplay());

        OCIO_CHECK_EQUAL(2, config1->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "sRGB"));
        OCIO_CHECK_EQUAL(std::string("Raw"), config1->getView(OCIO::VIEW_DISPLAY_DEFINED, "sRGB", 0));
        OCIO_CHECK_EQUAL(std::string("view"), config1->getView(OCIO::VIEW_DISPLAY_DEFINED, "sRGB", 1));

        OCIO_CHECK_ASSERT(!config1->viewIsShared("sRGB", "view"));

        OCIO_CHECK_EQUAL(2, config1->getNumViews("sRGB"));
        OCIO_CHECK_EQUAL(std::string("view"), config1->getView("sRGB", 0));

        // Inactive (display, view) pair where the view is a reference to a shared view.
        OCIO_REQUIRE_EQUAL(1, config2->getNumDisplays());
        OCIO_CHECK_EQUAL(std::string("Raw"), config2->getDefaultDisplay());
        OCIO_CHECK_EQUAL(1, config2->getNumViews("Raw"));
        OCIO_CHECK_EQUAL(std::string("Raw"), config2->getDefaultView("Raw"));

        OCIO_REQUIRE_EQUAL(config2->getNumDisplaysAll(), 2);
        OCIO_CHECK_EQUAL(config2->getDisplayAll(1), std::string("sRGB"));

        OCIO_CHECK_EQUAL(2, config2->getNumViews(OCIO::VIEW_SHARED, "sRGB"));
        OCIO_CHECK_EQUAL(std::string("view"), config2->getView(OCIO::VIEW_SHARED, "sRGB", 0));
        OCIO_CHECK_EQUAL(std::string("sview1"), config2->getView(OCIO::VIEW_SHARED, "sRGB", 1));

        OCIO_CHECK_ASSERT(config2->viewIsShared("sRGB", "view"));

        OCIO_CHECK_ASSERT(OCIO::Config::ViewsAreEqual(config1, config2, "sRGB", "view"));
        
    }

    {
        // Test that Config::ViewsAreEqual works for a matching (display, view) pair across separate configs,
        // even if the pair is active in one config and inactive in another. Both views are display-defined.

        // Inactive (display, view) pair where the view is display-defined.
        OCIO_REQUIRE_EQUAL(1, config1->getNumDisplays());
        OCIO_CHECK_EQUAL(std::string("sRGB"), config1->getDefaultDisplay());
        OCIO_CHECK_EQUAL(config1->getDisplayAll(0), std::string("Raw"));
        OCIO_CHECK_EQUAL(1, config1->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "Raw"));
        OCIO_CHECK_EQUAL(std::string("Raw"), config1->getView(OCIO::VIEW_DISPLAY_DEFINED, "Raw", 0));
        OCIO_CHECK_ASSERT(!config1->viewIsShared("Raw", "Raw"));

        // Active (display, view) pair where the view is display-defined.
        OCIO_REQUIRE_EQUAL(1, config2->getNumDisplays());
        OCIO_CHECK_EQUAL(std::string("Raw"), config2->getDefaultDisplay());
        OCIO_CHECK_EQUAL(1, config2->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "Raw"));
        OCIO_CHECK_EQUAL(std::string("Raw"), config2->getView(OCIO::VIEW_DISPLAY_DEFINED, "Raw", 0));
        OCIO_CHECK_ASSERT(!config2->viewIsShared("Raw", "Raw"));

        OCIO_CHECK_ASSERT(OCIO::Config::ViewsAreEqual(config1, config2, "Raw", "Raw"));
    }

    {
        // Test that Config::ViewsAreEqual works for a matching (display, view) pair across separate configs, even
        // if the pair is active in one config and inactive in another. Both views are reference to a shared view.
        
        // Active (display, view) pair where the view is a reference to a shared view.
        OCIO_CHECK_EQUAL(1, config1->getNumViews(OCIO::VIEW_SHARED, "sRGB"));
        OCIO_CHECK_EQUAL(std::string("sview1"), config1->getView(OCIO::VIEW_SHARED, "sRGB", 0));
        OCIO_CHECK_ASSERT(config1->viewIsShared("sRGB", "sview1"));

        // Inactive (display, view) pair where the view is a reference to a shared view.
        OCIO_CHECK_EQUAL(2, config2->getNumViews(OCIO::VIEW_SHARED, "sRGB"));
        OCIO_CHECK_EQUAL(std::string("view"), config2->getView(OCIO::VIEW_SHARED, "sRGB", 0));
        OCIO_CHECK_EQUAL(std::string("sview1"), config2->getView(OCIO::VIEW_SHARED, "sRGB", 1));
        OCIO_CHECK_ASSERT(config2->viewIsShared("sRGB", "sview1"));

        OCIO_CHECK_ASSERT(OCIO::Config::ViewsAreEqual(config1, config2, "sRGB", "sview1"));
    }

    {
        // Check that displayHasView method works if (display, view) pair exists regardless of whether the display or view
        // are active and regardless of whether the view is display-defined or if the view is a reference to a shared view.

        OCIO::ConfigRcPtr cfg1 = config1->createEditableCopy();
        OCIO_CHECK_ASSERT(cfg1->displayHasView("sRGB", "Raw"   )); // Active display has inactive view (display-defined).
        OCIO_CHECK_ASSERT(cfg1->displayHasView("sRGB", "view"  )); // Active display has active view (display-defined).
        OCIO_CHECK_ASSERT(cfg1->displayHasView("sRGB", "sview1")); // Active display has active view (shared).
        OCIO_CHECK_ASSERT(cfg1->displayHasView("Raw" , "Raw"   )); // Inactive display has inactive view (display-defined).

        OCIO_CHECK_NO_THROW(cfg1->setActiveDisplays("Raw"));
        OCIO_CHECK_EQUAL(1, cfg1->getNumDisplays());
        OCIO_CHECK_EQUAL(std::string("Raw"), cfg1->getDefaultDisplay());

        OCIO_CHECK_ASSERT(cfg1->displayHasView("sRGB", "sview1")); // Inactive display has active view (shared).

        OCIO_CHECK_NO_THROW(cfg1->setActiveViews("Raw"));
        OCIO_REQUIRE_EQUAL(cfg1->getNumViews("sRGB"), 1);
        OCIO_CHECK_EQUAL(cfg1->getView("sRGB", 0), std::string("Raw"));

        OCIO_CHECK_ASSERT(cfg1->displayHasView("sRGB" , "Raw"  )); // Inactive display has active view (display-defined).
        OCIO_CHECK_ASSERT(cfg1->displayHasView("sRGB", "sview1")); // Inactive display has inactive view (shared).

        OCIO_CHECK_NO_THROW(cfg1->setActiveDisplays("sRGB"));
        OCIO_CHECK_EQUAL(1, cfg1->getNumDisplays());
        OCIO_CHECK_EQUAL(std::string("sRGB"), cfg1->getDefaultDisplay());

        OCIO_CHECK_ASSERT(cfg1->displayHasView("sRGB", "sview1")); // Active display has inactive view (shared).
    }

    {
        // Test when a display exists, but a view doesn't exist.

        OCIO::ConfigRcPtr cfg1 = config1->createEditableCopy();

        OCIO_CHECK_EQUAL(std::string("sRGB"), cfg1->getDefaultDisplay());
        OCIO_CHECK_EQUAL(2, cfg1->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "sRGB"));
        OCIO_CHECK_EQUAL(std::string("Raw"), cfg1->getView(OCIO::VIEW_DISPLAY_DEFINED, "sRGB", 0));
        OCIO_CHECK_EQUAL(std::string("view"), cfg1->getView(OCIO::VIEW_DISPLAY_DEFINED, "sRGB", 1));

        OCIO_CHECK_ASSERT(cfg1->displayHasView("sRGB", "Raw"));
        OCIO_CHECK_ASSERT(OCIO::Config::ViewsAreEqual(config1, cfg1, "sRGB", "Raw"));

        // Remove the view from the display.
        OCIO_CHECK_NO_THROW(cfg1->removeDisplayView("sRGB", "Raw"));
        OCIO_CHECK_EQUAL(1, cfg1->getNumViews(OCIO::VIEW_DISPLAY_DEFINED, "sRGB"));
        OCIO_CHECK_EQUAL(std::string("view"), cfg1->getView(OCIO::VIEW_DISPLAY_DEFINED, "sRGB", 0));

        OCIO_CHECK_ASSERT(!cfg1->displayHasView("sRGB", "Raw"));
        OCIO_CHECK_ASSERT(!OCIO::Config::ViewsAreEqual(config1, cfg1, "sRGB", "Raw"));
    }

    {
        // Test when a view exists, but a display doesn't exist.
        
        OCIO::ConfigRcPtr cfg2 = config2->createEditableCopy();

        OCIO_CHECK_EQUAL(std::string("Raw"), cfg2->getDefaultDisplay());
        OCIO_REQUIRE_EQUAL(1, cfg2->getNumViews("Raw"));
        OCIO_REQUIRE_EQUAL(std::string("Raw"), cfg2->getView("Raw", 0));
        OCIO_CHECK_EQUAL(std::string("Raw"), cfg2->getActiveViews());

        OCIO_CHECK_ASSERT(cfg2->displayHasView("Raw", "Raw"));
        OCIO_CHECK_ASSERT(OCIO::Config::ViewsAreEqual(config2, cfg2, "Raw", "Raw"));

        // Remove the view from the display and the display itself since it only has no more views.
        OCIO_REQUIRE_EQUAL(2, cfg2->getNumDisplaysAll());
        OCIO_CHECK_NO_THROW(cfg2->removeDisplayView("Raw", "Raw"));
        OCIO_REQUIRE_EQUAL(1, cfg2->getNumDisplaysAll());

        // The view is still active.
        OCIO_REQUIRE_EQUAL(std::string("Raw"), cfg2->getActiveViews());

        OCIO_CHECK_ASSERT(!cfg2->displayHasView("Raw", "Raw"));
        OCIO_CHECK_ASSERT(!OCIO::Config::ViewsAreEqual(config2, cfg2, "Raw", "Raw"));
    }

    {
        // Test access of config-level shared views for displayHasView method.

        OCIO::ConfigRcPtr cfg1 = config1->createEditableCopy();

        OCIO_CHECK_EQUAL(1, cfg1->getNumViews(OCIO::VIEW_SHARED, "sRGB"));
        OCIO_CHECK_EQUAL(std::string("sview1"), cfg1->getView(OCIO::VIEW_SHARED, "sRGB", 0));
        OCIO_CHECK_ASSERT(cfg1->viewIsShared("sRGB", "sview1"));

        OCIO_CHECK_ASSERT(cfg1->displayHasView("sRGB", "sview1"));

        // Remove the shared view from the display.
        OCIO_CHECK_NO_THROW(cfg1->removeDisplayView("sRGB", "sview1"));
        OCIO_REQUIRE_EQUAL(std::string("sRGB"), config1->getDefaultDisplay());
        OCIO_CHECK_EQUAL(0, cfg1->getNumViews(OCIO::VIEW_SHARED, "sRGB"));

        // Shared view still exists in the config.
        OCIO_CHECK_EQUAL(1, cfg1->getNumViews(OCIO::VIEW_SHARED, nullptr));
        OCIO_CHECK_EQUAL(std::string("sview1"), cfg1->getView(OCIO::VIEW_SHARED, nullptr, 0));
        OCIO_CHECK_ASSERT(cfg1->viewIsShared(nullptr, "sview1"));

        OCIO_CHECK_ASSERT(!cfg1->displayHasView("sRGB", "sview1"));

        // When display name is null, displayHasView will only check config level shared views.
        OCIO_CHECK_ASSERT(cfg1->displayHasView(nullptr, "sview1"));
    }
}

OCIO_ADD_TEST(Config, compare_virtual_displays) {
    static constexpr char CONFIG1[]{ R"(ocio_profile_version: 2

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

viewing_rules:
  - !<Rule> {name: Linear, colorspaces: default}

shared_views:
  - !<View> {name: Film, view_transform: display_vt, display_colorspace: <USE_DISPLAY_NAME>, looks: look1, rule: Linear, description: Test view}
  - !<View> {name: view, view_transform: display_vt, display_colorspace: display_cs}

displays:
  Raw:
    - !<View> {name: Raw, colorspace: raw}
  sRGB:
    - !<View> {name: Raw, colorspace: raw}

virtual_display:
  - !<View> {name: Raw, colorspace: raw}
  - !<Views> [Film, view]

looks:
  - !<Look>
    name: look1
    process_space: default

view_transforms:
  - !<ViewTransform>
    name: default_vt
    to_scene_reference: !<CDLTransform> {sat: 1.5}

  - !<ViewTransform>
    name: display_vt
    to_display_reference: !<CDLTransform> {sat: 1.5}

display_colorspaces:
  - !<ColorSpace>
    name: display_cs
    to_display_reference: !<CDLTransform> {sat: 1.5}

colorspaces:
  - !<ColorSpace>
    name: raw
)" };

    static constexpr char CONFIG2[]{ R"(ocio_profile_version: 2

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

viewing_rules:
  - !<Rule> {name: Linear, colorspaces: default}

shared_views:
  - !<View> {name: view, view_transform: display_vt, display_colorspace: display_cs}

displays:
  Raw:
    - !<View> {name: Raw, colorspace: raw}
  sRGB:
    - !<View> {name: Raw, colorspace: raw}
    - !<Views> [view]

virtual_display:
  - !<View> {name: Raw, colorspace: raw}
  - !<View> {name: Film, view_transform: display_vt, display_colorspace: <USE_DISPLAY_NAME>, looks: look1, rule: Linear, description: Test view}
  - !<Views> [view]

looks:
  - !<Look>
    name: look1
    process_space: default

view_transforms:
  - !<ViewTransform>
    name: default_vt
    to_scene_reference: !<CDLTransform> {sat: 1.5}

  - !<ViewTransform>
    name: display_vt
    to_display_reference: !<CDLTransform> {sat: 1.5}

display_colorspaces:
  - !<ColorSpace>
    name: display_cs
    to_display_reference: !<CDLTransform> {sat: 1.5}

colorspaces:
  - !<ColorSpace>
    name: raw
)" };

    std::istringstream is;
    is.str(CONFIG1);
    OCIO::ConstConfigRcPtr config1, config2;
    OCIO_CHECK_NO_THROW(config1 = OCIO::Config::CreateFromStream(is));
    is.clear();
    is.str(CONFIG2);
    OCIO_CHECK_NO_THROW(config2 = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config1->validate());
    OCIO_CHECK_NO_THROW(config2->validate());

    {
        // Test that Config::VirtualViewsAreEqual works for a matching virtual view pair across separate configs.
        // Works regardless of if the virtual view is display-defined in one config and shared in the other.

        // Virtual view is a reference to a shared view.
        OCIO_REQUIRE_EQUAL(2, config1->getVirtualDisplayNumViews(OCIO::VIEW_SHARED));

        const char * viewName1 = config1->getVirtualDisplayView(OCIO::VIEW_SHARED, 0);

        OCIO_CHECK_EQUAL(std::string("Film"), viewName1);
        OCIO_CHECK_EQUAL(std::string("display_vt"), config1->getVirtualDisplayViewTransformName(viewName1));
        OCIO_CHECK_EQUAL(std::string("<USE_DISPLAY_NAME>"), config1->getVirtualDisplayViewColorSpaceName(viewName1));
        OCIO_CHECK_EQUAL(std::string("look1"), config1->getVirtualDisplayViewLooks(viewName1));
        OCIO_CHECK_EQUAL(std::string("Linear"), config1->getVirtualDisplayViewRule(viewName1));
        OCIO_CHECK_EQUAL(std::string("Test view"), config1->getVirtualDisplayViewDescription(viewName1));

        // Virtual view is a reference to a display-defined view.
        OCIO_REQUIRE_EQUAL(2, config2->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED));

        const char * viewName2 = config2->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 1);

        OCIO_CHECK_EQUAL(std::string("Film"), viewName2);
        OCIO_CHECK_EQUAL(std::string("display_vt"), config2->getVirtualDisplayViewTransformName(viewName2));
        OCIO_CHECK_EQUAL(std::string("<USE_DISPLAY_NAME>"), config2->getVirtualDisplayViewColorSpaceName(viewName2));
        OCIO_CHECK_EQUAL(std::string("look1"), config2->getVirtualDisplayViewLooks(viewName2));
        OCIO_CHECK_EQUAL(std::string("Linear"), config2->getVirtualDisplayViewRule(viewName2));
        OCIO_CHECK_EQUAL(std::string("Test view"), config2->getVirtualDisplayViewDescription(viewName2));

        OCIO_CHECK_EQUAL(std::string(viewName1), std::string(viewName2));
        OCIO_CHECK_ASSERT(OCIO::Config::VirtualViewsAreEqual(config1, config2, viewName1));
    }
    {
        // Virtual views are both display-defined.
        OCIO_REQUIRE_EQUAL(1, config1->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED));

        const char * viewName1 = config1->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 0);

        OCIO_CHECK_EQUAL(std::string("Raw"), viewName1);
        OCIO_CHECK_EQUAL(std::string(""), config1->getVirtualDisplayViewTransformName(viewName1));
        OCIO_CHECK_EQUAL(std::string("raw"), config1->getVirtualDisplayViewColorSpaceName(viewName1));
        OCIO_CHECK_EQUAL(std::string(""), config1->getVirtualDisplayViewLooks(viewName1));
        OCIO_CHECK_EQUAL(std::string(""), config1->getVirtualDisplayViewRule(viewName1));
        OCIO_CHECK_EQUAL(std::string(""), config1->getVirtualDisplayViewDescription(viewName1));

        const char * viewName2 = config2->getVirtualDisplayView(OCIO::VIEW_DISPLAY_DEFINED, 0);

        OCIO_CHECK_EQUAL(std::string("Raw"), viewName2);
        OCIO_CHECK_EQUAL(std::string(""), config2->getVirtualDisplayViewTransformName(viewName2));
        OCIO_CHECK_EQUAL(std::string("raw"), config2->getVirtualDisplayViewColorSpaceName(viewName2));
        OCIO_CHECK_EQUAL(std::string(""), config2->getVirtualDisplayViewLooks(viewName2));
        OCIO_CHECK_EQUAL(std::string(""), config2->getVirtualDisplayViewRule(viewName2));
        OCIO_CHECK_EQUAL(std::string(""), config2->getVirtualDisplayViewDescription(viewName2));

        OCIO_CHECK_EQUAL(std::string(viewName1), std::string(viewName2));
        OCIO_CHECK_ASSERT(OCIO::Config::VirtualViewsAreEqual(config1, config2, viewName1));
    }
    {
        // Virtual views are both shared.
        const char * viewName1 = config1->getVirtualDisplayView(OCIO::VIEW_SHARED, 1);

        OCIO_CHECK_EQUAL(std::string("view"), viewName1);
        OCIO_CHECK_EQUAL(std::string("display_vt"), config1->getVirtualDisplayViewTransformName(viewName1));
        OCIO_CHECK_EQUAL(std::string("display_cs"), config1->getVirtualDisplayViewColorSpaceName(viewName1));
        OCIO_CHECK_EQUAL(std::string(""), config1->getVirtualDisplayViewLooks(viewName1));
        OCIO_CHECK_EQUAL(std::string(""), config1->getVirtualDisplayViewRule(viewName1));
        OCIO_CHECK_EQUAL(std::string(""), config1->getVirtualDisplayViewDescription(viewName1));

        OCIO_REQUIRE_EQUAL(1, config2->getVirtualDisplayNumViews(OCIO::VIEW_SHARED));

        const char * viewName2 = config2->getVirtualDisplayView(OCIO::VIEW_SHARED, 0);

        OCIO_CHECK_EQUAL(std::string("view"), viewName2);
        OCIO_CHECK_EQUAL(std::string("display_vt"), config2->getVirtualDisplayViewTransformName(viewName2));
        OCIO_CHECK_EQUAL(std::string("display_cs"), config2->getVirtualDisplayViewColorSpaceName(viewName2));
        OCIO_CHECK_EQUAL(std::string(""), config2->getVirtualDisplayViewLooks(viewName2));
        OCIO_CHECK_EQUAL(std::string(""), config2->getVirtualDisplayViewRule(viewName2));
        OCIO_CHECK_EQUAL(std::string(""), config2->getVirtualDisplayViewDescription(viewName2));

        OCIO_CHECK_EQUAL(std::string(viewName1), std::string(viewName2));
        OCIO_CHECK_ASSERT(OCIO::Config::VirtualViewsAreEqual(config1, config2, viewName1));

        OCIO_CHECK_EQUAL(std::string(viewName1), std::string(viewName2));
        OCIO_CHECK_ASSERT(OCIO::Config::VirtualViewsAreEqual(config1, config2, viewName1));
    }
    {
        // Test when a shared virtual view exists in one config but not the other.
        OCIO::ConfigRcPtr cfg = config1->createEditableCopy();

        OCIO_CHECK_ASSERT(config1->hasVirtualView("Film"));
        OCIO_CHECK_ASSERT(config1->virtualViewIsShared("Film"));

        OCIO_REQUIRE_EQUAL(2, cfg->getVirtualDisplayNumViews(OCIO::VIEW_SHARED));
        OCIO_CHECK_ASSERT(cfg->hasVirtualView("Film"));
        OCIO_CHECK_ASSERT(cfg->virtualViewIsShared("Film"));

        OCIO_CHECK_ASSERT(OCIO::Config::VirtualViewsAreEqual(config1, cfg, "Film"));

        // Check against another config where the virtual view is display-defined.
        OCIO_CHECK_ASSERT(OCIO::Config::VirtualViewsAreEqual(config2, cfg, "Film"));

        // Remove a shared view from the virtual display.
        cfg->removeVirtualDisplayView("Film");

        OCIO_REQUIRE_EQUAL(1, cfg->getVirtualDisplayNumViews(OCIO::VIEW_SHARED));
        OCIO_CHECK_ASSERT(!cfg->hasVirtualView("Film"));
        OCIO_CHECK_ASSERT(!cfg->virtualViewIsShared("Film"));

        OCIO_CHECK_ASSERT(!OCIO::Config::VirtualViewsAreEqual(config1, cfg, "Film"));        
        OCIO_CHECK_ASSERT(!OCIO::Config::VirtualViewsAreEqual(config2, cfg, "Film"));
    }
    {
        // Test when a display-defined virtual view exists in one config but not the other.
        OCIO::ConfigRcPtr cfg = config2->createEditableCopy();

        // Remove a display-defined view from the virtual display.
        OCIO_CHECK_ASSERT(config2->hasVirtualView("Film"));
        OCIO_CHECK_ASSERT(!config2->virtualViewIsShared("Film"));   // Confirm display-defined

        OCIO_REQUIRE_EQUAL(2, cfg->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED));
        OCIO_CHECK_ASSERT(cfg->hasVirtualView("Film"));
        OCIO_CHECK_ASSERT(!cfg->virtualViewIsShared("Film"));       // Confirm display-defined

        OCIO_CHECK_ASSERT(OCIO::Config::VirtualViewsAreEqual(config2, cfg, "Film"));

        // Check against another config where the virtual view is a reference to a shared view.
        OCIO_CHECK_ASSERT(OCIO::Config::VirtualViewsAreEqual(config1, cfg, "Film"));

        // Remove a display-defined view from the virtual display.
        cfg->removeVirtualDisplayView("Film");

        OCIO_REQUIRE_EQUAL(1, cfg->getVirtualDisplayNumViews(OCIO::VIEW_DISPLAY_DEFINED));
        OCIO_CHECK_ASSERT(!cfg->hasVirtualView("Film"));
        
        OCIO_CHECK_ASSERT(!OCIO::Config::VirtualViewsAreEqual(config2, cfg, "Film"));        
        OCIO_CHECK_ASSERT(!OCIO::Config::VirtualViewsAreEqual(config1, cfg, "Film"));
    }
}
