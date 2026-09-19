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

bool ActiveDisplayColorSpacesHaveAttributes(const ConstConfigRcPtr & config)
{
    const int numCS = config->getNumColorSpaces(SEARCH_REFERENCE_SPACE_DISPLAY, COLORSPACE_ACTIVE);
    if (numCS == 0)
    {
        LogDebug("HDR Display Support (2.6): No active display-referred color spaces found.");
        return false;
    }

    bool allHaveInteropID = true;

    for (int i = 0; i < numCS; ++i)
    {
        const char * csName = config->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_DISPLAY,
                                                               COLORSPACE_ACTIVE, i);
        ConstColorSpaceRcPtr cs = config->getColorSpace(csName);
        if (!cs || cs->getReferenceSpaceType() != REFERENCE_SPACE_DISPLAY ||
            !cs->getInteropID() || !*cs->getInteropID())
        {
            LogDebug(std::string("HDR Display Support (2.6): Active display color space '") + csName +
                     "' has no interop ID.");
            allHaveInteropID = false;
        }

        if (!cs || !cs->getEncoding() || !*cs->getEncoding())
        {
            LogDebug(std::string("HDR Display Support (2.6): Active display color space '") + csName +
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

    for (int d = 0; d < numDisplays; ++d)
    {
        const char * display = config->getDisplay(d);
        ConstColorSpaceRcPtr cs = config->getColorSpace(display);
        if (!cs)
        {
            LogDebug(std::string("HDR Display Support (2.6): Display '") + display +
                     "' has no matching color space.");
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

            LogDebug(std::string("HDR Display Support (2.6): Active (display, view) pair ('") + display +
                     "', '" + view + "') has no view transform and is not a data color space.");
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

    if (!config->hasRole(ROLE_INTERCHANGE_DISPLAY))
    {
        LogDebug("HDR Display Support (2.6): Missing the 'cie_xyz_d65_interchange' role.");
        compatible = false;
    }

    if (!ActiveDisplayColorSpacesHaveAttributes(config))
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
