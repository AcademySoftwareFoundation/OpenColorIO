// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "CategoryHelpers.h"
#include "utils/StringUtils.h"
#include "LegacyViewingPipeline.h"


namespace OCIO_NAMESPACE
{

namespace DisplayViewHelpers
{

ConstProcessorRcPtr GetProcessor(const ConstConfigRcPtr & config,
                                 const char * workingName,
                                 const char * displayName,
                                 const char * viewName,
                                 const ConstMatrixTransformRcPtr & channelView,
                                 TransformDirection direction)
{
    return GetProcessor(config,
                        config->getCurrentContext(),
                        workingName,
                        displayName,
                        viewName,
                        channelView,
                        direction);
}

ConstProcessorRcPtr GetProcessor(const ConstConfigRcPtr & config,
                                 const ConstContextRcPtr & context,
                                 const char * workingName,
                                 const char * displayName,
                                 const char * viewName,
                                 const ConstMatrixTransformRcPtr & channelView,
                                 TransformDirection direction)
{
    DisplayViewTransformRcPtr displayTransform = DisplayViewTransform::Create();
    displayTransform->setDirection(direction);
    displayTransform->setSrc(workingName);
    displayTransform->setDisplay(displayName);
    displayTransform->setView(viewName);

    ConstProcessorRcPtr processor
        = config->getProcessor(context, displayTransform, TRANSFORM_DIR_FORWARD);

    bool needExposure = true;
    bool needGamma    = true;

    if (processor->isDynamic())
    {
        GroupTransformRcPtr grpTransform = processor->createGroupTransform();

        const int maxTransform = grpTransform->getNumTransforms();
        for (int idx=0; idx<maxTransform; ++idx)
        {
            ConstTransformRcPtr tr = grpTransform->getTransform(idx);
            ConstExposureContrastTransformRcPtr ex = DynamicPtrCast<const ExposureContrastTransform>(tr);

            if (ex)
            {
                if (ex->isExposureDynamic())
                {
                    needExposure = false;
                }

                if (ex->isGammaDynamic())
                {
                    needGamma = false;
                }
            }
        }
    }

    if (!needExposure && !needGamma && !channelView)
    {
        return processor;
    }

    LegacyViewingPipelineRcPtr pipeline = LegacyViewingPipeline::Create();
    pipeline->setDisplayViewTransform(displayTransform);

    // TODO: Need to update to allow for apps that are viewing non-scene-linear images.
    if (needExposure)
    {
        ExposureContrastTransformRcPtr ex = ExposureContrastTransform::Create();

        ex->setStyle(EXPOSURE_CONTRAST_LINEAR);
        ex->setPivot(0.18);

        ex->makeExposureDynamic();
        ex->makeContrastDynamic();

        pipeline->setLinearCC(ex);
    }

    if (needGamma)
    {
        ExposureContrastTransformRcPtr ex = ExposureContrastTransform::Create();
        ex->setStyle(EXPOSURE_CONTRAST_VIDEO);
        ex->setPivot(1.0);

        ex->makeGammaDynamic();

        pipeline->setDisplayCC(ex);
    }

    if (channelView)
    {
        pipeline->setChannelView(channelView);
    }

    return pipeline->getProcessor(config, context);
}

ConstProcessorRcPtr GetIdentityProcessor(const ConstConfigRcPtr & config)
{
    GroupTransformRcPtr group = GroupTransform::Create();
    {
        ExposureContrastTransformRcPtr ex = ExposureContrastTransform::Create();

        ex->setStyle(EXPOSURE_CONTRAST_LINEAR);
        ex->setPivot(0.18);

        ex->makeExposureDynamic();
        ex->makeContrastDynamic();

        group->appendTransform(ex);
    }
    {
        ExposureContrastTransformRcPtr ex = ExposureContrastTransform::Create();
        ex->setStyle(EXPOSURE_CONTRAST_VIDEO);
        ex->setPivot(1.0);

        ex->makeGammaDynamic();

        group->appendTransform(ex);
    }
    return config->getProcessor(group);
}

void AddActiveDisplayView(ConfigRcPtr & config, const char * displayName, const char * viewName)
{
    if (!displayName || !viewName) return;

    // Add the display to the active display list only if possible.

    const char * envActiveDisplays = GetEnvVariable(OCIO_ACTIVE_DISPLAYS_ENVVAR);
    if (envActiveDisplays && *envActiveDisplays)
    {
        StringUtils::StringVec displays = StringUtils::Split(envActiveDisplays, ',');
        StringUtils::Trim(displays);

        const bool acceptAllDisplays = displays.size()==1 && displays[0]=="";
        if (!acceptAllDisplays)
        {
            std::stringstream err;
            err << "Forbidden to add an active display as '" 
                << OCIO_ACTIVE_DISPLAYS_ENVVAR
                << "' controls the active list.";

            throw Exception(err.str().c_str());
        }
    }
    else
    {
        const char * activeDisplays = config->getActiveDisplays();
        if (activeDisplays && *activeDisplays)
        {
            StringUtils::StringVec displays = StringUtils::Split(activeDisplays, ',');
            StringUtils::Trim(displays);

            const bool acceptAllDisplays = displays.size()==1 && displays[0]=="";

            if (!acceptAllDisplays && !StringUtils::Contain(displays, displayName))
            {
                displays.push_back(displayName);
                const std::string disp = StringUtils::Join(displays, ',');
                config->setActiveDisplays(disp.c_str());
            }
        }
    }

    // Add the view to the active view list, only if needed.

    const char * envActiveViews = GetEnvVariable(OCIO_ACTIVE_VIEWS_ENVVAR);
    if (envActiveViews && *envActiveViews)
    {
        StringUtils::StringVec views = StringUtils::Split(envActiveViews, ',');
        StringUtils::Trim(views);

        const bool acceptAllViews = views.size()==1 && views[0]=="";
        if (!acceptAllViews)
        {
            std::stringstream err;
            err << "Forbidden to add an active view as '" 
                << OCIO_ACTIVE_VIEWS_ENVVAR
                << "' controls the active list.";

            throw Exception(err.str().c_str());
        }
    }
    else
    {
        const char * activeViews = config->getActiveViews();
        if (activeViews && *activeViews)
        {
            StringUtils::StringVec views = StringUtils::Split(activeViews, ',');
            StringUtils::Trim(views);
    
            const bool acceptAllViews = views.size()==1 && views[0]=="";

            if (!acceptAllViews && !StringUtils::Contain(views, viewName))
            {
                views.push_back(viewName);
                const std::string v = StringUtils::Join(views, ',');
                config->setActiveViews(v.c_str());
            }
        }
    }
}

void RemoveActiveDisplayView(ConfigRcPtr & config, const char * displayName, const char * viewName)
{
    if (!displayName || !viewName) return;

    // Remove the display from the active display list only if possible.

    const char * envActiveDisplays = GetEnvVariable(OCIO_ACTIVE_DISPLAYS_ENVVAR);
    if (envActiveDisplays && *envActiveDisplays)
    {
        StringUtils::StringVec displays = StringUtils::Split(envActiveDisplays, ',');
        StringUtils::Trim(displays);

        const bool acceptAllDisplays = displays.size()==1 && displays[0]=="";
        if (!acceptAllDisplays)
        {
            std::stringstream err;
            err << "Forbidden to remove an active display as '" 
                << OCIO_ACTIVE_DISPLAYS_ENVVAR
                << "' controls the active list.";

            throw Exception(err.str().c_str());
        }
    }
    else
    {
        const char * activeDisplays = config->getActiveDisplays();
        if (activeDisplays && *activeDisplays)
        {
            StringUtils::StringVec displays = StringUtils::Split(activeDisplays, ',');
            StringUtils::Trim(displays);

            const bool acceptAllDisplays = displays.size()==1 && displays[0]=="";

            if (!acceptAllDisplays && StringUtils::Contain(displays, displayName))
            {
                // As the display is an active one, not finding it in the list
                // means that it could be removed from the 'active_displays' list.

                bool toRemove = true;
                const int numDisplays = config->getNumDisplays();
                for (int dispIdx = 0; dispIdx < numDisplays && toRemove; ++dispIdx)
                {
                    const char * dispName = config->getDisplay(dispIdx);
                    if (StringUtils::Compare(dispName, displayName))
                    {
                        toRemove = false;
                    }
                }

                if (toRemove)
                {
                    StringUtils::Remove(displays, displayName);
                    const std::string actives = StringUtils::Join(displays, ',');
                    config->setActiveDisplays(actives.c_str());
                }
            }
        }
    }

    // Remove the view from the active view list only if possible.

    const char * envActiveViews = GetEnvVariable(OCIO_ACTIVE_VIEWS_ENVVAR);
    if (envActiveViews && *envActiveViews)
    {
        StringUtils::StringVec views = StringUtils::Split(envActiveViews, ',');
        StringUtils::Trim(views);

        const bool acceptAllViews = views.size()==1 && views[0]=="";
        if (!acceptAllViews)
        {
            std::stringstream err;
            err << "Forbidden to remove an active view as '" 
                << OCIO_ACTIVE_VIEWS_ENVVAR
                << "' controls the active list.";

            throw Exception(err.str().c_str());
        }
    }
    else
    {
        const char * activeViews = config->getActiveViews();
        if (activeViews && *activeViews)
        {
            StringUtils::StringVec views = StringUtils::Split(activeViews, ',');
            StringUtils::Trim(views);

            const bool acceptAllViews = views.size()==1 && views[0]=="";
            if (!acceptAllViews && StringUtils::Contain(views, viewName))
            {
                // As the view is an active one, not finding it in the list means that it could
                // be removed. But the code needs to loop over all the displays i.e. active and
                // inactive ones, before validating the removal.

                struct EnableAllDisplays
                {
                    EnableAllDisplays() = delete;
                    EnableAllDisplays(const EnableAllDisplays &) = delete;

                    // Note that if the 'active_displays' list is controlled by the ennvar either
                    // the code throws before or the following guard is useless.
                    explicit EnableAllDisplays(ConfigRcPtr & config)
                        :   m_config(config)
                        ,   m_activeDisplays(config->getActiveDisplays())
                    {
                        m_config->setActiveDisplays("");
                    }

                    ~EnableAllDisplays()
                    {
                        m_config->setActiveDisplays(m_activeDisplays.c_str());
                    }

                    ConfigRcPtr       m_config;
                    const std::string m_activeDisplays;
                } guard(config);

                bool toRemove = true;

                const int numDisplays = config->getNumDisplays();
                for (int dispIdx = 0; dispIdx < numDisplays && toRemove; ++dispIdx)
                {
                    const char * dispName = config->getDisplay(dispIdx);

                    const int numViews = config->getNumViews(dispName);
                    for (int viewIdx = 0; viewIdx < numViews && toRemove; ++viewIdx)
                    {
                        const char * vwName = config->getView(dispName, viewIdx);
                        if (StringUtils::Compare(vwName, viewName))
                        {
                            toRemove = false;
                        }
                    }
                }

                if (toRemove)
                {
                    StringUtils::Remove(views, viewName);
                    const std::string actives = StringUtils::Join(views, ',');
                    config->setActiveViews(actives.c_str());
                }
            }
        }
    }
}

void AddDisplayView(ConfigRcPtr & config,
                    const char * displayName,
                    const char * viewName,
                    const char * lookDefinition,
                    const ColorSpaceRcPtr & colorSpace,
                    FileTransformRcPtr & userTransform,
                    const char * connectionColorSpaceName)
{
    if (!displayName || !*displayName)
    {
        throw Exception("Invalid display name.");
    }

    if (!viewName || !*viewName)
    {
        throw Exception("Invalid view name.");
    }

    // Step 1 - Create the color transformation.

    GroupTransformRcPtr grp =  GroupTransform::Create();

    // Add the 'reference' to connection color space.
    {
        ConstColorSpaceRcPtr connectionCS = config->getColorSpace(connectionColorSpaceName);

        // Check for an active or inactive color space.
        if (!connectionCS)
        {
            std::string errMsg;
            errMsg += "Connection color space name '";
            errMsg += connectionColorSpaceName;
            errMsg += "' does not exist.";

            throw Exception(errMsg.c_str());
        }

        ConstTransformRcPtr tr = connectionCS->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if (tr)
        {
            grp->appendTransform(tr->createEditableCopy());
        }
        else
        {
            tr = connectionCS->getTransform(COLORSPACE_DIR_TO_REFERENCE);
            if (tr)
            {
                TransformRcPtr t = tr->createEditableCopy();
                t->setDirection(CombineTransformDirections(tr->getDirection(), TRANSFORM_DIR_INVERSE));
                grp->appendTransform(t);
            }
        }
    }

    // Add the 'LUT' transform.
    grp->appendTransform(userTransform);

    grp->validate();

    // Step 2 - Add active display and view.

    AddActiveDisplayView(config, displayName, viewName);

    // Step 3 - Add the color space to the config.

    colorSpace->setTransform(DynamicPtrCast<const Transform>(grp), COLORSPACE_DIR_FROM_REFERENCE);
    config->addColorSpace(colorSpace);

    // Step 4 - Add a new active (display, view) pair.

    config->addDisplayView(displayName, viewName, colorSpace->getName(), lookDefinition);
}

void AddDisplayView(ConfigRcPtr & config,
                    const char * displayName,
                    const char * viewName,
                    const char * lookDefinition,
                    const char * colorSpaceName,
                    const char * colorSpaceFamily,
                    const char * colorSpaceDescription,
                    const char * categories,
                    const char * transformFilePath,
                    const char * connectionColorSpaceName)
{
    ColorSpaceRcPtr colorSpace = ColorSpace::Create();
    colorSpace->setName(colorSpaceName ? colorSpaceName : "");
    colorSpace->setFamily(colorSpaceFamily ? colorSpaceFamily : "");
    colorSpace->setDescription(colorSpaceDescription ? colorSpaceDescription : "");

    // Check if the name is already a color space or a role name.
    if (config->getColorSpace(colorSpace->getName()))
    {
        std::string errMsg;
        errMsg += "Color space name '";
        errMsg += colorSpace->getName();
        errMsg += "' already exists.";

        throw Exception(errMsg.c_str());
    }

    // Add categories if any.
    if (categories && *categories)
    {
        const Categories cats = ExtractItems(categories);

        // Only add the categories if already used.
        ColorSpaceNames names = FindColorSpaceNames(config, cats);
        if (!names.empty())
        {
            for (const auto & cat : cats)
            {
                colorSpace->addCategory(cat.c_str());
            }
        }
    }

    FileTransformRcPtr file = FileTransform::Create();
    file->setSrc(transformFilePath);

    AddDisplayView(config,
                   displayName, viewName, lookDefinition,
                   colorSpace, file,
                   connectionColorSpaceName);
}

void RemoveDisplayView(ConfigRcPtr & config, const char * displayName, const char * viewName)
{
    const std::string name{ config->getDisplayViewColorSpaceName(displayName, viewName) };
    const std::string csName{ name.empty() ? displayName : name };
    if (csName.empty())
    {
        std::string errMsg = "Missing color space for '";
        errMsg += displayName;
        errMsg += "' and '";
        errMsg += viewName;
        errMsg += "'.";
        throw Exception(errMsg.c_str());
    }

    // Step 1 - Remove the (display, view) pair.

    config->removeDisplayView(displayName, viewName);

    // Setp 2 - Remove the (display, view) pair from active lists if possible.

    RemoveActiveDisplayView(config, displayName, viewName);

    // Step 3 - Remove the associated color space if not used.

    if (!config->isColorSpaceUsed(csName.c_str()))
    {
        config->removeColorSpace(csName.c_str());
    }
}

} // DisplayViewHelpers


} // namespace OCIO_NAMESPACE
