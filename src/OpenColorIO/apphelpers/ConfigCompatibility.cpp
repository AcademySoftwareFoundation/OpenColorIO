// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "ConfigCompatibility.h"
#include "Logging.h"


namespace OCIO_NAMESPACE
{

namespace ConfigCompatibilityHelpers
{

bool DisplayColorSpacesHaveAttributes(const ConstConfigRcPtr & config)
{
    // Search all (active & inactive) display color spaces.

    const int numCS = config->getNumColorSpaces(SEARCH_REFERENCE_SPACE_DISPLAY, COLORSPACE_ALL);
    if (numCS == 0)
    {
        LogDebug("HDR Display Support (2.6): No display-referred color spaces found.");
        return false;
    }

    bool allHaveInteropID = true;

    for (int i = 0; i < numCS; ++i)
    {
        const char * csName = config->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_DISPLAY,
                                                               COLORSPACE_ALL, i);
        ConstColorSpaceRcPtr cs = config->getColorSpace(csName);

        // Having the interop ID allows applications to match up a display color space with
        // external color space definitions, such as operating system color space enums.

        if (!cs || !cs->getInteropID() || !*cs->getInteropID())
        {
            LogDebug(std::string("HDR Display Support (2.6): Display color space '") + csName +
                     "' has no interop ID.");
            allHaveInteropID = false;
        }

        // The encoding attribute allows applications to know whether this color space is
        // for an SDR or HDR monitor and whether it is a linear or gamma-corrected encoding.

        if (!cs || !cs->getEncoding() || !*cs->getEncoding())
        {
            LogDebug(std::string("HDR Display Support (2.6): Display color space '") + csName +
                     "' has no encoding.");
            allHaveInteropID = false;
        }
    }

    return allHaveInteropID;
}

bool ActiveDisplaysHaveColorSpace(const ConstConfigRcPtr & config)
{
    // Only check active displays.
    const int numDisplays = config->getNumDisplays();
    if (numDisplays == 0)
    {
        LogDebug("HDR Display Support (2.6): No active displays.");
        return false;
    }

    bool allHaveColorSpace = true;

    // Having a display color space for each display allows applications to convert the
    // output from a DisplayViewTransform back to the display-referred reference space
    // and from there to any color space that may be required by the operating system.

    for (int d = 0; d < numDisplays; ++d)
    {
        const char * display = config->getDisplay(d);
        ConstColorSpaceRcPtr cs = config->getColorSpace(display);
        if (!cs || cs->getReferenceSpaceType() != REFERENCE_SPACE_DISPLAY)
        {
            LogDebug(std::string("HDR Display Support (2.6): Display '") + display +
                     "' has no matching display-referred color space.");
            allHaveColorSpace = false;
        }
    }

    return allHaveColorSpace;
}

bool ActiveViewsHaveViewTransform(const ConstConfigRcPtr & config)
{
    // Only check active displays.
    const int numDisplays = config->getNumDisplays();

    int numViewsTotal = 0;
    bool allHaveViewTransform = true;
    bool hasViewTransform = false;

    for (int d = 0; d < numDisplays; ++d)
    {
        const char * display = config->getDisplay(d);
        // Only check active views.
        const int numViews = config->getNumViews(display);
        for (int v = 0; v < numViews; ++v)
        {
            ++numViewsTotal;

            // Being structured as a ViewTransform plus a display color space is the
            // best-practice for configs and structures the processing to give the
            // application the most flexibility to conform to the OS requirements.

            const char * view = config->getView(display, v);
            const char * transformName = config->getDisplayViewTransformName(display, view);
            if (transformName && *transformName)
            {
                hasViewTransform = true;
                continue;
            }

            // However, sometimes a config will include a few "utility" views, such as a
            // "Raw" view, implemented as a color space where isData is true, and this
            // should not disqualify a config.

            const char * csName = config->getDisplayViewColorSpaceName(display, view);
            ConstColorSpaceRcPtr cs = csName && *csName ? config->getColorSpace(csName) :
                                                          ConstColorSpaceRcPtr();
            if (cs && cs->isData())
            {
                continue;
            }

            // Similarly, configs sometimes will structure "utility" views using a
            // NamedTransform as the color space. End-user expectations for such views
            // is not that they will provide exact colorimetry. As long as the application
            // has a display color space for the view, that should be sufficient context
            // for how to handle the resulting output.

            ConstNamedTransformRcPtr nt = csName && *csName ? config->getNamedTransform(csName) :
                                                              ConstNamedTransformRcPtr();
            if (nt)
            {
                continue;
            }

            LogDebug(std::string("HDR Display Support (2.6): Active (display, view) pair ('") + display +
                     "', '" + view + "') has no view transform and is not a data color space "
                     "or a named transform.");
            allHaveViewTransform = false;
        }
    }

    if (numViewsTotal == 0)
    {
        LogDebug("HDR Display Support (2.6): No active (display, view) pairs.");
        return false;
    }

    if (!hasViewTransform)
    {
        LogDebug("HDR Display Support (2.6): No active (display, view) pair references a view "
                 "transform.");
    }

    return allHaveViewTransform && hasViewTransform;
}

bool CheckHDRDisplaySupport26(const ConstConfigRcPtr & config)
{
    bool compatible = true;

    // Ideally, an application will be able to use the interop IDs in the display color
    // spaces to identify one that corresponds to the color space required by the OS.
    // An OCIO ColorSpaceTransform may then be used to convert from the display color
    // space produced by the DisplayViewTransform into the required OS color space.
    //
    // However, if that is not available, the display-referred interchange role is a key
    // piece of information for applications, allowing them to invert the display color
    // spaces back to CIE XYZ D65 and then convert forward to whatever color space is
    // required. For example, the interchange role facilitates using GetProcessorFromConfigs
    // to convert from a display color space in a user's config to a display color space
    // in one of the built-in configs.

    if (!config->hasRole(ROLE_INTERCHANGE_DISPLAY))
    {
        LogDebug("HDR Display Support (2.6): Missing the 'cie_xyz_d65_interchange' role.");
        compatible = false;
    }

    if (!DisplayColorSpacesHaveAttributes(config))
    {
        compatible = false;
    }

    if (!ActiveDisplaysHaveColorSpace(config))
    {
        compatible = false;
    }

    if (!ActiveViewsHaveViewTransform(config))
    {
        compatible = false;
    }

    return compatible;
}

bool CheckCompatibility(const ConstConfigRcPtr & config, ConfigCompatibility compatibility)
{
    switch (compatibility)
    {
    case CONFIG_HDR_DISPLAY_SUPPORT_26:
        return CheckHDRDisplaySupport26(config);
    }

    return false;
}

} // namespace ConfigCompatibilityHelpers

} // namespace OCIO_NAMESPACE
