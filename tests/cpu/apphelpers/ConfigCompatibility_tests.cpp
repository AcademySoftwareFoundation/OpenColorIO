// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "apphelpers/ConfigCompatibility.cpp"
#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{

OCIO::ColorSpaceRcPtr CreateDisplayColorSpace(const char * name)
{
    auto cs = OCIO::ColorSpace::Create(OCIO::REFERENCE_SPACE_DISPLAY);
    cs->setName(name);
    return cs;
}

} // namespace

OCIO_ADD_TEST(ConfigCompatibility, hdr_display_support_26)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Add the role, but the referenced display color space has no interop ID or encoding.

    auto interchange = CreateDisplayColorSpace("CIE XYZ-D65");
    OCIO_CHECK_NO_THROW(config->addColorSpace(interchange));
    OCIO_CHECK_NO_THROW(config->setRole(OCIO::ROLE_INTERCHANGE_DISPLAY,
                                        "CIE XYZ-D65"));

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::ActiveDisplayColorSpacesHaveAttributes(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Give the interchange color space an interop ID, but it still has no encoding.

    interchange->setInteropID("ocio:lin_ciexyzd65_display");
    OCIO_CHECK_NO_THROW(config->addColorSpace(interchange));

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::ActiveDisplayColorSpacesHaveAttributes(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Give the interchange color space an encoding as well. It is now the only display color
    // space in the config and it has both an interop ID and an encoding.

    interchange->setEncoding("display-linear");
    OCIO_CHECK_NO_THROW(config->addColorSpace(interchange));

    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::ActiveDisplayColorSpacesHaveAttributes(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Add another display color space without an interop ID or encoding. Since not all
    // display color spaces satisfy the requirement anymore, the check fails again.

    auto dcs = CreateDisplayColorSpace("display_cs");
    OCIO_CHECK_NO_THROW(config->addColorSpace(dcs));

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::ActiveDisplayColorSpacesHaveAttributes(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // It does not have the interop ID and encoding, but making it inactive allows it to pass.

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces("display_cs"));

    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::ActiveDisplayColorSpacesHaveAttributes(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Add a view that does not use a view transform and whose color space is not a data
    // color space. A view transform is required unless the color space is a data color space,
    // so the check fails.

    auto scs = OCIO::ColorSpace::Create();
    scs->setName("scene_cs");
    OCIO_CHECK_NO_THROW(config->addColorSpace(scs));

    OCIO_CHECK_NO_THROW(config->addDisplayView("display", "view_no_vt", "scene_cs", ""));

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::ActiveViewsHaveViewTransform(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Mark scene_cs as a data color space. The view with no view transform is now exempt from
    // the requirement, but the check still fails since no view uses a view transform at all.

    scs->setIsData(true);
    OCIO_CHECK_NO_THROW(config->addColorSpace(scs));

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::ActiveViewsHaveViewTransform(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Add another view that does use a view transform. All views now satisfy the view
    // transform requirement, but the config is not yet compatible since the active display
    // "display" has no color space matching its name.

    auto vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    vt->setName("view_transform");
    auto cdl = OCIO::CDLTransform::Create();
    cdl->setSat(1.2);
    vt->setTransform(cdl, OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    OCIO_CHECK_NO_THROW(config->addViewTransform(vt));

    OCIO_CHECK_NO_THROW(config->addDisplayView("display", "view_with_vt", "view_transform",
                                               "display_cs", "", "", ""));

    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::ActiveViewsHaveViewTransform(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::ActiveDisplaysHaveColorSpace(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Add a color space matching the display's name (with an interop ID and encoding, since
    // it is active and so must also satisfy ActiveDisplayColorSpacesHaveAttributes). The config
    // is now compatible.

    auto displayCS = CreateDisplayColorSpace("display");
    displayCS->setInteropID("foo:pq_rec6000_display");
    displayCS->setEncoding("hdr-video");
    OCIO_CHECK_NO_THROW(config->addColorSpace(displayCS));

    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::ActiveDisplayColorSpacesHaveAttributes(config));
    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::ActiveDisplaysHaveColorSpace(config));
    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Add a second display that has no color space matching its name. Since it is active,
    // the check fails. Deactivating it via setActiveDisplays makes the config compatible
    // again, showing that an inactive display does not need to pass the check.

    OCIO_CHECK_NO_THROW(config->addDisplayView("display2", "view", "view_transform",
                                               "display_cs", "", "", ""));

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::ActiveDisplaysHaveColorSpace(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    OCIO_CHECK_NO_THROW(config->setActiveDisplays("display"));

    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::ActiveDisplaysHaveColorSpace(config));
    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Add a view that does not qualify (no view transform, non-data color space) to the
    // active display. Since it is active, the check fails. Deactivating it via setActiveViews
    // makes the config compatible again, showing that an inactive view does not need to pass
    // the check.

    OCIO_CHECK_NO_THROW(config->addDisplayView("display", "bad_view", "display_cs", ""));

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::ActiveViewsHaveViewTransform(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    OCIO_CHECK_NO_THROW(config->setActiveViews("view_with_vt"));

    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::ActiveViewsHaveViewTransform(config));
    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));
}
