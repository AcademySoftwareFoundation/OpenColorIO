// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_OPENCOLORAPPHELPERS_H
#define INCLUDED_OCIO_OPENCOLORAPPHELPERS_H

#include "OpenColorTypes.h"

#ifndef OCIO_NAMESPACE
#error This header cannot be used directly. Use <OpenColorIO/OpenColorIO.h> instead.
#endif


namespace OCIO_NAMESPACE
{

namespace Category
{
extern OCIOEXPORT const char * INPUT;
extern OCIOEXPORT const char * SCENE_LINEAR_WORKING_SPACE;
extern OCIOEXPORT const char * LOG_WORKING_SPACE;
extern OCIOEXPORT const char * VIDEO_WORKING_SPACE;
extern OCIOEXPORT const char * LUT_INPUT_SPACE;

} // Category


/**
 * Helper class to create menus for the content of a config. Menu can list color spaces, roles,
 * named transforms. Each entry has a name, a UI name, a description, and a family. Family can also
 * be accessed as hierarchy levels; levels are created by splitting the family using the 'family
 * separator'. Hierarchy levels are meant to be used as sub-menus.
 *
 * The UI name is what is intended to be put in application menus seen by the end-user.   However,
 * please note that the UI name is not guaranteed to remain stable between releases and so if
 * applications need to save something it should be the 'name' rather than the 'UI name'.
 * Currently, the only difference between the 'name' and 'UI name' is for roles.
 */
class OCIOEXPORT ColorSpaceMenuHelper
{
public:
    /**
     * Flag to control if roles and/or named transforms are added to the the list of color spaces.
     * If roles are added, the 'UI name' will be of the form "role (color space name)", the 'name'
     * will be the role name (not the color space name), and the family will be "Roles".
     */
    enum IncludeTypeFlag
    {
        INCLUDE_NO_EXTRAS       = 0x0000,
        INCLUDE_ROLES           = 0x0001,
        INCLUDE_NAMEDTRANSFORMS = 0x0002,
        INCLUDE_ALL_EXTRAS      = 0xFFFF
    };
    /**
     * Create a menu with config.
     *
     * If role is a valid role, other parameters are ignored and menu will contain only that role.
     * Categories is a comma separated list of categories. If categories is not NULL and not empty,
     * all color spaces that have one of the categories will be part of the menu. If no color space
     * uses one of the categories, all color spaces are added.  If include flag has INCLUDE_ROLES,
     * all roles are added with the "Roles" family (hierarchy level).
     */
    static ColorSpaceMenuHelperRcPtr Create(const ConstConfigRcPtr & config,
                                            const char * role,
                                            const char * categories,
                                            IncludeTypeFlag includeFlag);

    /// Access to the color spaces (or roles).
    virtual size_t getNumColorSpaces() const noexcept = 0;
    /**
     * Get the color space (or role) name used in the config for this menu item.  Will be empty
     * if the index is out of range.
     */
    virtual const char * getName(size_t idx) const noexcept = 0;
    /**
     * Get the name to use in the menu UI.  This might be different from the config name, for
     * example in the case of roles.  Will be empty if the index is out of range.
     */
    virtual const char * getUIName(size_t idx) const noexcept = 0;;

    /**
     * Get the index of the element of a given name. Return (size_t)-1 name if NULL or empty, or if
     * no element with that name is found.
     */
    virtual size_t getIndexFromName(const char * name) const noexcept = 0;
    virtual size_t getIndexFromUIName(const char * name) const noexcept = 0;

    virtual const char * getDescription(size_t idx) const noexcept = 0;
    virtual const char * getFamily(size_t idx) const noexcept = 0;

    /**
     * Hierarchy levels are created from the family string. It is split into levels using the
     * 'family separator'.
     */
    virtual size_t getNumHierarchyLevels(size_t idx) const noexcept = 0;
    virtual const char * getHierarchyLevel(size_t idx, size_t i) const noexcept = 0;

    /// Get the color space name from the UI name.
    virtual const char * getNameFromUIName(const char * uiName) const noexcept = 0;
    /// Get the color space UI name from the name.
    virtual const char * getUINameFromName(const char * name) const noexcept = 0;

    /**
     * Add an additional color space to the menu.
     *
     * Note that an additional color space could be:
     * * an inactive color space,
     * * an active color space not having at least one of the selected categories,
     * * a newly created color space.
     * Throws if color space is not part of the config. Nothing is done if it is already part of
     * the menu. 
     */
    virtual void addColorSpaceToMenu(const char * name) = 0;

    /**
     * Refresh the instance (i.e. needed following a configuration change for example). Note that
     * any added color spaces are preserved.
     */
    virtual void refresh(const ConstConfigRcPtr & config) = 0;

    ColorSpaceMenuHelper(const ColorSpaceMenuHelper &) = delete;
    ColorSpaceMenuHelper & operator=(const ColorSpaceMenuHelper &) = delete;

    /// Do not use (needed only for pybind11).
    virtual ~ColorSpaceMenuHelper() = default;

protected:
    ColorSpaceMenuHelper() = default;
};

namespace ColorSpaceHelpers
{
/**
 * Add a new color space to the config instance. The output of the userTransform must be in the
 * specified connectionColorSpace.
 *
 * Note: If the config does not already use categories, we do not add them since that would 
 * make a big change to how existing color spaces show up in menus.
 */
extern OCIOEXPORT void AddColorSpace(ConfigRcPtr & config,
                                     const char * name,
                                     const char * transformFilePath,
                                     const char * categories, // Could be null or empty.
                                     const char * connectionColorSpaceName);

} // ColorSpaceHelpers

namespace DisplayViewHelpers
{

/**
 * Get the processor from the working color space to (display, view) pair (forward) or (display,
 * view) pair to working (inverse). The working color space name could be a role name or a color
 * space name. ChannelView can be empty. If not already present, each of these functions adds
 * ExposureContrastTransforms to enable changing exposure, contrast, and gamma after the processor
 * has been created using dynamic properties.
 */
extern OCIOEXPORT ConstProcessorRcPtr GetProcessor(const ConstConfigRcPtr & config,
                                                   const ConstContextRcPtr & context,
                                                   const char * workingName,
                                                   const char * displayName,
                                                   const char * viewName,
                                                   const ConstMatrixTransformRcPtr & channelView,
                                                   TransformDirection direction);

extern OCIOEXPORT ConstProcessorRcPtr GetProcessor(const ConstConfigRcPtr & config,
                                                   const char * workingName,
                                                   const char * displayName,
                                                   const char * viewName,
                                                   const ConstMatrixTransformRcPtr & channelView,
                                                   TransformDirection direction);

/// Get an identity processor containing only the ExposureContrastTransforms.
extern OCIOEXPORT ConstProcessorRcPtr GetIdentityProcessor(const ConstConfigRcPtr & config);

/**
 * Add a new (display, view) pair and the new color space to a configuration instance.
 * The input to the userTransform must be in the specified connectionColorSpace.
 */
extern OCIOEXPORT void AddDisplayView(ConfigRcPtr & config,
                                      const char * displayName,
                                      const char * viewName,
                                      const char * lookDefinition,  // Could be empty or null
                                      const char * colorSpaceName,  // Could be empty or null
                                      const char * colorSpaceFamily, // Could be empty or null
                                      const char * colorSpaceDescription, // Could be empty or null
                                      const char * categories,      // Could be empty or null
                                      const char * transformFilePath,
                                      const char * connectionColorSpaceName);

/**
 * Remove a (display, view) pair including the associated color space (only if not used).
 * Note that the view is always removed but the display is only removed if empty.
 */
extern OCIOEXPORT void RemoveDisplayView(ConfigRcPtr & config,
                                         const char * displayName,
                                         const char * viewName);

} // DisplayViewHelpers


/**
 * Whereas the DisplayViewTransform simply applies a specific view from an OCIO display, the
 * LegacyViewingPipeline provides an example of a complete viewing pipeline of the sort that could
 * be used to implement a viewport in a typical application.  It therefore adds, around the
 * DisplayViewTransform, various optional color correction steps and RGBA channel view swizzling.
 * The direction of the DisplayViewTranform is used as the direction of the pipeline.
 * Note: The LegacyViewingPipeline class provides the same functionality as the OCIO v1
 * DisplayTransform.
 *
 * Legacy viewing pipeline:
 * * Start in display transform input color space.
 * * If linearCC is provided:
 *   * Go to scene_linear colorspace.
 *   * Apply linearCC transform.
 * * If colorTimingCC is provided:
 *   * Go to color_timing colorspace.
 *   * Apply colorTimingCC transform.
 * * Apply looks (from display transform or from looks override).
 *   * Go to first look color space.
 *   * Apply first look transform.
 *   * Iterate for all looks.
 * * Apply channelView transform.
 * * Apply display transform (without looks).
 * * Apply displayCC.
 * Note that looks are applied even if the display transform involves data color spaces.
 */
class OCIOEXPORT LegacyViewingPipeline
{
public:
    static LegacyViewingPipelineRcPtr Create();

    LegacyViewingPipeline(const LegacyViewingPipeline &) = delete;
    LegacyViewingPipeline & operator=(const LegacyViewingPipeline &) = delete;

    virtual ConstDisplayViewTransformRcPtr getDisplayViewTransform() const noexcept = 0;
    virtual void setDisplayViewTransform(const ConstDisplayViewTransformRcPtr & dt) noexcept = 0;

    virtual ConstTransformRcPtr getLinearCC() const noexcept = 0;
    virtual void setLinearCC(const ConstTransformRcPtr & cc) noexcept = 0;

    virtual ConstTransformRcPtr getColorTimingCC() const noexcept = 0;
    virtual void setColorTimingCC(const ConstTransformRcPtr & cc) noexcept = 0;

    virtual ConstTransformRcPtr getChannelView() const noexcept = 0;
    virtual void setChannelView(const ConstTransformRcPtr & transform) noexcept = 0;

    virtual ConstTransformRcPtr getDisplayCC() const noexcept = 0;
    virtual void setDisplayCC(const ConstTransformRcPtr & cc) noexcept = 0;

    /**
     * Specify whether the lookOverride should be used, or not. This is a separate flag, as
     * it's often useful to override "looks" to an empty string.
     */
    virtual void setLooksOverrideEnabled(bool enable) = 0;
    virtual bool getLooksOverrideEnabled() const = 0;

    /**
     * A user can optionally override the looks that are,  by default, used with the expected
     * display / view combination.  A common use case for this functionality is in an image
     * viewing app, where per-shot looks are supported.  If for some reason a per-shot look is
     * not defined for the current Context, the Config::getProcessor fcn will not succeed by
     * default.  Thus, with this mechanism the viewing app could override to looks = "", and
     * this will allow image display to continue (though hopefully) the interface would reflect
     * this fallback option.
     * 
     * Looks is a potentially comma (or colon) delimited list of lookNames, where +/- prefixes
     * are optionally allowed to denote forward/inverse look specification (and forward is
     * assumed in the absence of either).
     */
    virtual void setLooksOverride(const char * looks) = 0;
    virtual const char * getLooksOverride() const = 0;

    virtual ConstProcessorRcPtr getProcessor(const ConstConfigRcPtr & config,
                                             const ConstContextRcPtr & context) const = 0;

    virtual ConstProcessorRcPtr getProcessor(const ConstConfigRcPtr & config) const = 0;

    /// Do not use (needed only for pybind11).
    virtual ~LegacyViewingPipeline() = default;

protected:
    LegacyViewingPipeline() = default;
};


/**
 * The MixingSlider and MixingColorSpaceManager classes are to help applications implement correct 
 * color pickers.  The term "color mixing" is used here to describe what is done in a typical 
 * application "color picker" user interface.
 * 
 * A user may want to mix colors in different color spaces.  The two most common mixing space 
 * options are a scene-linear working space or the display space. 
 * 
 * Since scene-linear color spaces are not perceptually uniform, it is necessary to compensate UI 
 * widgets such as sliders.  For example, it is nice if mid-gray falls near the center of mixing
 * controls rather than way over near the black end.  This may be done by using a mapping from 
 * linear into an approximately perceptually uniform space. 
 * 
 * Also note that a color picking/mixing UI may want to present a given color space in several
 * different encodings.  The most common two encodings for color mixing are RGB and HSV.
 * 
 * Note that these helpers anticipate that a user may want to mix colors using values that extend
 * outside the typical [0,1] domain.
 */
class OCIOEXPORT MixingSlider
{
public:
    /// Set the minimum edge of a UI slider for conversion to mixing space.
    virtual void setSliderMinEdge(float sliderMixingMinEdge) noexcept = 0;

    /// Minimum edge of a UI slider for conversion to mixing space.
    virtual float getSliderMinEdge() const noexcept = 0;

    /// Set the maximum edge of a UI slider for conversion to mixing space.
    virtual void setSliderMaxEdge(float sliderMixingMaxEdge) noexcept = 0;

    /// Maximum edge of a UI slider for conversion to mixing space.
    virtual float getSliderMaxEdge() const noexcept = 0;

    /// Convert from units in distance along the slider to mixing space units.
    virtual float sliderToMixing(float sliderUnits) const noexcept = 0;

    /// Convert from mixing space units to distance along the slider.
    virtual float mixingToSlider(float mixingUnits) const noexcept = 0;

    MixingSlider(const MixingSlider &) = delete;
    MixingSlider & operator=(const MixingSlider &) = delete;

    /// Do not use (needed only for pybind11).
    virtual ~MixingSlider() = default;

protected:
    MixingSlider() = default;
};


class OCIOEXPORT MixingColorSpaceManager
{
public:

    static MixingColorSpaceManagerRcPtr Create(ConstConfigRcPtr & config);

    /// Access to the mixing spaces.
    virtual size_t getNumMixingSpaces() const noexcept = 0;
    virtual const char * getMixingSpaceUIName(size_t idx) const = 0;
    virtual size_t getSelectedMixingSpaceIdx() const noexcept = 0;
    virtual void setSelectedMixingSpaceIdx(size_t idx) = 0;
    virtual void setSelectedMixingSpace(const char * mixingSpace) = 0;

    virtual bool isPerceptuallyUniform() const noexcept = 0;

    /// Access to the mixing encodings.
    virtual size_t getNumMixingEncodings() const noexcept = 0;
    virtual const char * getMixingEncodingName(size_t idx) const = 0;
    virtual size_t getSelectedMixingEncodingIdx() const noexcept = 0;
    virtual void setSelectedMixingEncodingIdx(size_t idx) = 0;
    virtual void setSelectedMixingEncoding(const char * mixingEncoding) = 0;

    /// Refresh the instance (i.e. needed following a configuration change for example).
    virtual void refresh(ConstConfigRcPtr config) = 0;

    virtual ConstProcessorRcPtr getProcessor(const char * workingName,
                                             const char * displayName,
                                             const char * viewName,
                                             TransformDirection direction) const = 0;

    virtual MixingSlider & getSlider() noexcept = 0;
    virtual MixingSlider & getSlider(float sliderMixingMinEdge,
                                     float sliderMixingMaxEdge) noexcept = 0;

    MixingColorSpaceManager(const MixingColorSpaceManager &) = delete;
    MixingColorSpaceManager & operator=(const MixingColorSpaceManager &) = delete;

    /// Do not use (needed only for pybind11).
    virtual ~MixingColorSpaceManager() = default;

protected:
    MixingColorSpaceManager() = default;
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_OPENCOLORAPPHELPERS_H
