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

    // Step 1 - No interchange display role.

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Step 2 - Has the role, but the referenced display color space has no interop ID or
    // encoding.

    auto interchange = CreateDisplayColorSpace("CIE XYZ-D65");
    OCIO_CHECK_NO_THROW(config->addColorSpace(interchange));
    OCIO_CHECK_NO_THROW(config->setRole(OCIO::ROLE_INTERCHANGE_DISPLAY,
                                        "CIE XYZ-D65"));

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::AllDisplayColorSpacesHaveAttributes(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Step 3 - Give the interchange color space an interop ID, but it still has no encoding.

    interchange->setInteropID("ocio:lin_ciexyzd65_display");
    OCIO_CHECK_NO_THROW(config->addColorSpace(interchange));

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::AllDisplayColorSpacesHaveAttributes(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Step 4 - Give the interchange color space an encoding as well. It is now the only display
    // color space in the config and it has both an interop ID and an encoding.

    interchange->setEncoding("display-linear");
    OCIO_CHECK_NO_THROW(config->addColorSpace(interchange));

    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::AllDisplayColorSpacesHaveAttributes(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Step 5 - Add another display color space without an interop ID or encoding. Since not
    // all display color spaces satisfy the requirement anymore, the check fails again.

    auto dcs = CreateDisplayColorSpace("display_cs");
    OCIO_CHECK_NO_THROW(config->addColorSpace(dcs));

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::AllDisplayColorSpacesHaveAttributes(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Step 6 - Give the second display color space an interop ID, but it still has no encoding.

    dcs->setInteropID("foo:pq_rec6000_display");
    OCIO_CHECK_NO_THROW(config->addColorSpace(dcs));

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::AllDisplayColorSpacesHaveAttributes(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Step 7 - Give the second display color space an encoding as well, but there is still
    // no (display, view) pair defined.

    dcs->setEncoding("hdr-video");
    OCIO_CHECK_NO_THROW(config->addColorSpace(dcs));

    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::AllDisplayColorSpacesHaveAttributes(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::AllViewsHaveViewTransform(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Step 8 - Add a view that does not use a view transform and whose color space is not a
    // data color space. A view transform is required unless the color space is a data color
    // space, so the check fails.

    auto scs = OCIO::ColorSpace::Create();
    scs->setName("scene_cs");
    OCIO_CHECK_NO_THROW(config->addColorSpace(scs));

    OCIO_CHECK_NO_THROW(config->addDisplayView("display", "view_no_vt", "scene_cs", ""));

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::AllViewsHaveViewTransform(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Step 9 - Mark scene_cs as a data color space. The view with no view transform is now
    // exempt from the requirement, but the check still fails since no view uses a view
    // transform at all.

    scs->setIsData(true);
    OCIO_CHECK_NO_THROW(config->addColorSpace(scs));

    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::AllViewsHaveViewTransform(config));
    OCIO_CHECK_ASSERT(!OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));

    // Step 10 - Add another view that does use a view transform. The config is now
    // compatible.

    auto vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    vt->setName("view_transform");
    auto cdl = OCIO::CDLTransform::Create();
    cdl->setSat(1.2);
    vt->setTransform(cdl, OCIO::VIEWTRANSFORM_DIR_FROM_REFERENCE);
    OCIO_CHECK_NO_THROW(config->addViewTransform(vt));

    OCIO_CHECK_NO_THROW(config->addDisplayView("display", "view_with_vt", "view_transform",
                                               "display_cs", "", "", ""));

    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::AllViewsHaveViewTransform(config));
    OCIO_CHECK_ASSERT(OCIO::ConfigCompatibilityHelpers::CheckCompatibility(
        config, OCIO::CONFIG_HDR_DISPLAY_SUPPORT_26));
}
