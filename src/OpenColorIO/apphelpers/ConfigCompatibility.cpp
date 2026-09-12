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

bool AllDisplayColorSpacesHaveAttributes(const ConstConfigRcPtr & config)
{
    const int numCS = config->getNumColorSpaces(SEARCH_REFERENCE_SPACE_DISPLAY, COLORSPACE_ALL);
    if (numCS == 0)
    {
        LogDebug("ConfigCompatibility: Config has no display-referred color spaces; "
                 "CONFIG_HDR_DISPLAY_SUPPORT_26 requires at least one with an interop ID set.");
        return false;
    }

    bool allHaveInteropID = true;

    for (int i = 0; i < numCS; ++i)
    {
        const char * csName = config->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_DISPLAY,
                                                               COLORSPACE_ALL, i);
        ConstColorSpaceRcPtr cs = config->getColorSpace(csName);
        if (!cs || cs->getReferenceSpaceType() != REFERENCE_SPACE_DISPLAY ||
            !cs->getInteropID() || !*cs->getInteropID())
        {
            LogDebug(std::string("ConfigCompatibility: Display color space '") + csName +
                     "' does not have an interop ID set; CONFIG_HDR_DISPLAY_SUPPORT_26 requires "
                     "all display color spaces to have one.");
            allHaveInteropID = false;
        }

        if (!cs || !cs->getEncoding() || !*cs->getEncoding())
        {
            LogDebug(std::string("ConfigCompatibility: Display color space '") + csName +
                     "' does not have an encoding set; CONFIG_HDR_DISPLAY_SUPPORT_26 requires "
                     "all display color spaces to have one.");
            allHaveInteropID = false;
        }
    }

    return allHaveInteropID;
}

bool AllViewsHaveViewTransform(const ConstConfigRcPtr & config)
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

            const char * view = config->getView(display, v);
            const char * transformName = config->getDisplayViewTransformName(display, view);
            if (transformName && *transformName)
            {
                hasViewTransform = true;
                continue;
            }

            const char * csName = config->getDisplayViewColorSpaceName(display, view);
            ConstColorSpaceRcPtr cs = csName && *csName ? config->getColorSpace(csName) :
                                                          ConstColorSpaceRcPtr();
            if (cs && cs->isData())
            {
                continue;
            }

            LogDebug(std::string("ConfigCompatibility: (display, view) pair ('") + display +
                     "', '" + view + "') does not qualify; CONFIG_HDR_DISPLAY_SUPPORT_26 requires "
                     "all active (display, view) pairs to reference a view transform unless their "
                     "color space is a data color space.");
            allHaveViewTransform = false;
        }
    }

    if (numViewsTotal == 0)
    {
        LogDebug("ConfigCompatibility: Config has no (display, view) pairs; "
                 "CONFIG_HDR_DISPLAY_SUPPORT_26 requires at least one.");
        return false;
    }

    if (!hasViewTransform)
    {
        LogDebug("ConfigCompatibility: No (display, view) pair references a view transform; "
                 "CONFIG_HDR_DISPLAY_SUPPORT_26 requires at least one.");
    }

    return allHaveViewTransform && hasViewTransform;
}

bool CheckHDRDisplaySupport26(const ConstConfigRcPtr & config)
{
    bool compatible = true;

    if (!config->hasRole(ROLE_INTERCHANGE_DISPLAY))
    {
        LogDebug("ConfigCompatibility: Config is missing the ROLE_INTERCHANGE_DISPLAY role "
                 "required for CONFIG_HDR_DISPLAY_SUPPORT_26.");
        compatible = false;
    }

    if (!AllDisplayColorSpacesHaveAttributes(config))
    {
        compatible = false;
    }

    if (!AllViewsHaveViewTransform(config))
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
