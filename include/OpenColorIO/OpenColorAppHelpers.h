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

/**
 * Parameters controlling which color spaces appear in menus.
 *
 * The ColorSpaceMenuHelper class is intended to be used by applications to get the list of items
 * to show in color space menus.
 *
 * The ColorSpaceMenuParameters class is used to configure the behavior as needed for any given
 * menu.  Here is the algorithm used to produce a list of "items" (or strings) that will appear in
 * a menu:
 *
 * 1)  Use setRole to identify a role that controls a given menu.  If the config has this role,
 *     then only that color space is returned.  The name is set to the color space name, the UIName
 *     is presented as "<role name> (<color space name>)". It may be useful for the application to
 *     then grey-out the menu or otherwise indicate to the user that the value for this menu is not
 *     user selectable since it was pre-determined by the config.  If the config does not have that
 *     role, the algorithm continues to the remaining steps.
 *
 * 2)  The IncludeColorSpaces, SearchReferenceSpaceType, and IncludeNamedTransforms parameters are
 *     used to identify a set of items from the config that are potential candidates for use in the
 *     menu, as follows:
 *        - IncludeColorSpaces: Set to true to include color spaces in the menu.
 *        - SearchReferenceSpaceType: Use this to control whether the menu should include all color
 *          spaces, only display color spaces, or only non-display color spaces.
 *        - IncludeNamedTransforms: Set to true to include named transforms in the menu.
 *
 * 3)  The set of items from step 2 is then filtered in step 3 using the following parameters:
 *        - AppCategories: A list of strings specified by the application based on the purpose of
 *          the menu.  For example, if the menu is used to select a color space for importing an
 *          image, the application might specify the 'file-io' category, whereas if it is to select
 *          a working color space, it might specify the 'working-space' category.  Application
 *          developers should document what strings they are using for each menu so that config
 *          authors know what categories to use in their configs.  Alternatively, an application
 *          could let advanced users customize the string to use for a given menu in the
 *          application.
 *        - Encodings: A list of strings used to further refine the items selected from the
 *          AppCategories.  For example, an application might specify 'working-space' as the
 *          category and then specify 'scene-linear' as the encoding to only use items that have
 *          both of those properties (e.g., only select scene-linear working color spaces).
 *        - UserCategories: A list of strings specified by the end-user of the application.  OCIO
 *          will check for these strings in an environment variable, or they may be passed in from
 *          the application.
 *        - TreatNoCategoryAsAny: By default, color spaces (or named transforms) that have no
 *          categories are handled as if they had any of the categories.  Config authors that want to
 *          hide color spaces without categories should either put them in the inactiveColorSpaces
 *          list or add a category that will never be searched for (e.g., "invisible" or "hidden").
 *          App developers may set this option to false to hide items without any categories.
 *
 * Basically the intent is for the filtering to return the intersection of the app categories,
 * encoding, and user categories.  However, some fall-backs are in place to ensure that the
 * filtering does not remove all menu items.  Here is the detailed description:
 *
 * 3a) The items from step 2 are filtered to generate a list of appItems containing only the ones
 *     that contain at least one of the AppCategories strings in their "categories" property and
 *     one of the encodings in their "encoding" property.   If this list is empty, an attempt is
 *     made to generate a non-empty appItems list by only filtering by AppCategories.  If that is
 *     empty, an attempt is made to only filter by Encodings.
 *
 * 3b) The items from step 2 are filtered to generate a list of userItems containing only the ones
 *     that have at least one of the UserCategories strings in their "categories" property.
 *
 * 3c) If both appItems and userItems are non-empty, a list of resultItems will be generated as
 *     the intersection of those two lists.
 *
 * 3d) If the resultItems list is empty, the appList will be expanded by only filtering by
 *     AppCategories and not encodings.  The resultItems will be formed again as the intersection
 *     of the appItems and userItems.
 *
 * 3e) If the resultItems is still empty, it will be set to just the appItems from step 3a.
 *
 * 3f) If the resultItems is still empty, it will be set to just the userItems.
 *
 * 3g) If the resultItems is still empty, the items are not filtered and all items from step 2 are
 *     returned.  The rationale is that if step 2 has produced any items, it is not acceptable for
 *     step 3 to remove all of them.  An application usually expects to have a non-zero number of
 *     items to display in the menu.  However, if step 2 produces no items (e.g. the application
 *     requests only named transforms and the config has no named transform), then no items will
 *     be returned.
 *
 *
 * 4)  If IncludeRoles is true, the items from step 3 are extended by including an item for each
 *     role.  The name is set to the role name, the UIName is presented as "<role name> (<color
 *     space name>)", and the family is set to "Roles".
 *
 * 5)  If AddColorSpace has been used to add any additional items, these are appended to the final
 *     list.
 */
class OCIOEXPORT ColorSpaceMenuParameters
{
public:
    static ColorSpaceMenuParametersRcPtr Create(ConstConfigRcPtr config);
    /// Config is required to be able to create a ColorSpaceMenuHelper.
    virtual void setConfig(ConstConfigRcPtr config) noexcept = 0;
    virtual ConstConfigRcPtr getConfig() const noexcept = 0;

    /// If role is a valid role, other parameters are ignored and menu will contain only that role.
    virtual void setRole(const char * role) noexcept = 0;
    virtual const char * getRole() const noexcept = 0;


    /**
     * Include all color spaces (or not) to ColorSpaceMenuHelper. Default is to include color
     * spaces.
     */
    virtual void setIncludeColorSpaces(bool include) noexcept = 0;
    virtual bool getIncludeColorSpaces() const noexcept = 0;

    /**
     * Can be used to restrict the search using the ReferenceSpaceType of the color spaces.
     * It has no effect on roles and named transforms.
     */
    virtual SearchReferenceSpaceType getSearchReferenceSpaceType() const noexcept = 0;
    virtual void setSearchReferenceSpaceType(SearchReferenceSpaceType colorSpaceType) noexcept = 0;

    /**
     * Include all named transforms (or not) to ColorSpaceMenuHelper. Default is not to include
     * named transforms.
     */
    virtual void setIncludeNamedTransforms(bool include) noexcept = 0;
    virtual bool getIncludeNamedTransforms() const noexcept = 0;

    /**
     * When searching for color spaces using app or user categories, treat color spaces or
     * named transforms that have no categories as if they had any of the categories.
     * Default is true.
     */
    virtual void setTreatNoCategoryAsAny(bool value) noexcept = 0;
    virtual bool getTreatNoCategoryAsAny() const noexcept = 0;

    /**
     * App categories is a comma separated list of categories. If appCategories is not NULL and
     * not empty, all color spaces that have one of the categories will be part of the menu.
     */
    virtual void setAppCategories(const char * appCategories) noexcept = 0;
    virtual const char * getAppCategories() const noexcept = 0;

    /**
     * Encodings is a comma separated list of encodings. When not empty, is retricting the search
     * to color spaces that are using one of the encodings.
     */
    virtual void setEncodings(const char * encodings) noexcept = 0;
    virtual const char * getEncodings() const noexcept = 0;

    /**
     * User categories is a comma separated list of categories. If OCIO_USER_CATEGORIES_ENVVAR
     * env. variable is defined and not empty, this parameter is ignored and the value of the
     * env. variable is used for user categories.
     */
    virtual void setUserCategories(const char * userCategories) noexcept = 0;
    virtual const char * getUserCategories() const noexcept = 0;


    /**
     * Include all roles (or not) to ColorSpaceMenuHelper. Default is not to include roles.
     * Roles are added after color spaces with an single hierarchy level named "Roles".
     */
    virtual void setIncludeRoles(bool include) noexcept = 0;
    virtual bool getIncludeRoles() const noexcept = 0;

    /**
     * Add an additional color space (or named transform) to the menu.
     *
     * Note that an additional color space could be:
     * * an inactive color space,
     * * an active color space not having at least one of the selected categories,
     * * a newly created color space.
     * Will throw when creating the menu if color space is not part of the config. Nothing is done
     * if it is already part of the menu.
     * It's ok to call this multiple times with the same color space, it will only be added to the
     * menu once.  If a role name is passed in, the name in the menu will be the color space name
     * the role points to.
     */
    virtual void addColorSpace(const char * name) noexcept = 0;

    virtual size_t getNumAddedColorSpaces() const noexcept = 0;
    virtual const char * getAddedColorSpace(size_t index) const noexcept = 0;
    virtual void clearAddedColorSpaces() noexcept = 0;

    /// Do not use (needed only for pybind11).
    virtual ~ColorSpaceMenuParameters() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const ColorSpaceMenuParameters &);

/**
 * Helper class to create menus for the content of a config.
 *
 * Menu can list color spaces, roles, named transforms. Each entry has a name, a UI name, a
 * description, and a family. Family can also be accessed as hierarchy levels; levels are created
 * by splitting the family using the 'family separator'. Hierarchy levels are meant to be used as
 * sub-menus.
 *
 * The UI name is what is intended to be put in application menus seen by the end-user.   However,
 * please note that the UI name is not guaranteed to remain stable between releases and so if
 * applications need to save something it should be the 'name' rather than the 'UI name'.
 * Currently, the only difference between the 'name' and 'UI name' is for roles.
 *
 * The overall ordering of items is: color spaces, named transforms, roles, and additional color
 * spaces.  The display color spaces will either come before or after the other color spaces based
 * on where that block of spaces appears in the config.  The order of items returned by the menu
 * helper preserves the order of items in the config itself for each type of elements, thus
 * preserving the intent of the config author.  For example, if you call getName at idx
 * and idx+1, the name returned at idx+1 will be from farther down in the config than the one at
 * idx as long as both are of the same type.  (An application may ask for only the items in one
 * of those blocks if it wants to handle them separately.)  If the application makes use of
 * hierarchical menus, that will obviously impose a different order on what the user sees in the
 * menu.  Though even with  hierarchical menus, applications should try to preserve config ordering
 * (which is equivalent to index ordering) for items within the same sub-menu.
 */
class OCIOEXPORT ColorSpaceMenuHelper
{
public:
    static ColorSpaceMenuHelperRcPtr Create(ConstColorSpaceMenuParametersRcPtr parameters);

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
    virtual const char * getUIName(size_t idx) const noexcept = 0;

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

    ColorSpaceMenuHelper(const ColorSpaceMenuHelper &) = delete;
    ColorSpaceMenuHelper & operator=(const ColorSpaceMenuHelper &) = delete;

    /// Do not use (needed only for pybind11).
    virtual ~ColorSpaceMenuHelper() = default;

protected:
    ColorSpaceMenuHelper() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const ColorSpaceMenuHelper &);

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

    LegacyViewingPipeline(const LegacyViewingPipeline &) = delete;
    LegacyViewingPipeline & operator=(const LegacyViewingPipeline &) = delete;

    /// Do not use (needed only for pybind11).
    virtual ~LegacyViewingPipeline() = default;

protected:
    LegacyViewingPipeline() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const LegacyViewingPipeline &);

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

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const MixingSlider &);

/**
 * Used to mix (or pick/choose) colors.
 */
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

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const MixingColorSpaceManager &);

/**
 * The ConfigMergingParameters class holds the options that control how a merge is done.
 * 
 * In terms of OCIOM file, it represent one of the merges in an OCIOM file.
 * 
 */
class OCIOEXPORT ConfigMergingParameters
{
public:

    enum MergeStrategies
    {
        /// Combine elements of the base and input configs, with the input taking priority.
        STRATEGY_PREFER_INPUT = 0,
        /// Combine elements of the base and input configs, with the base taking priority.
        STRATEGY_PREFER_BASE,
        /// Use only the input elements for that section of the config.
        STRATEGY_INPUT_ONLY,
        /// Use only the base elements for that section of the config.
        STRATEGY_BASE_ONLY,
        /// The elements in the input config are removed from the base config. (If the names
        /// match, the item is removed, even if the content is not identical.)
        STRATEGY_REMOVE,
        /// Strategy has not been set yet.
        STRATEGY_UNSPECIFIED
    };

    static ConfigMergingParametersRcPtr Create();

    ConfigMergingParametersRcPtr createEditableCopy() const;

    /// Set the file name of the base config. This is used along with the search path of 
    /// the ConfigMerger object.
    void setBaseConfigName(const char * baseConfig);
    const char * getBaseConfigName() const;

    /// Set the file name of the input config. This is used along with the search path of 
    /// the ConfigMerger object.
    void setInputConfigName(const char * inputConfig);
    const char * getInputConfigName() const;

    /// Set a name to use for this merger. This may be used as the input or base config name
    /// in subsequent mergers.
    void setOutputName(const char * outputName);
    const char * getOutputName() const;   

    // Options

    /// Set the default strategy. This will be used if the strategy for a given config section
    /// is not set, and will be used for basic attributes such as the config description.
    /// Default = STRATEGY_PREFER_INPUT.
    void setDefaultStrategy(const ConfigMergingParameters::MergeStrategies strategy);
    ConfigMergingParameters::MergeStrategies getDefaultStrategy() const;

    /// Set a prefix to add to the family of input config items. (It must use '/' as the
    /// separator and will be replaced by the actual family separator of the config.)
    void setInputFamilyPrefix(const char * prefix);
    const char * getInputFamilyPrefix() const;

    /// Set a prefix to add to the family of base config items. (It must use '/' as the
    /// separator and will be replaced by the actual family separator of the config.)
    void setBaseFamilyPrefix(const char * prefix);
    const char * getBaseFamilyPrefix() const;

    /// If true, items from the input config will be higher in the file than those of the
    /// base config. Default = true.
    void setInputFirst(bool enabled);
    bool isInputFirst() const;

    /// If true, throw an exception rather than log a warning when a conflict is detected.
    /// Default = false.
    void setErrorOnConflict(bool enabled);
    bool isErrorOnConflict() const;

    /// If true, a color space from the input config is compared against those of the base
    /// config. If it is mathematically equivalent, it is not added. Instead, its name and
    /// aliases are added to the original color space. Default = true.
    void setAvoidDuplicates(bool enabled);
    bool isAvoidDuplicates() const;

    /// If true, the reference spaces of the base and input config are compared and color
    /// spaces from the input config will be adjusted to use the reference space of the base.
    /// If the interchange roles are not set, heuristics will be used to try and determine
    /// the reference space. Default = true.
    void setAdjustInputReferenceSpace(bool enabled);
    bool isAdjustInputReferenceSpace() const;

    // Overrides

    /// Override the name of the merged config.
    void setName(const char * mergedConfigName);
    const char * getName() const;

    /// Override the description of the merged config.
    void setDescription(const char * mergedConfigDesc);
    const char * getDescription() const;

    /// Override a context variable in the merged config.
    void addEnvironmentVar(const char * name, const char * defaultValue);
    /// Get the number of context variable overrides.
    int getNumEnvironmentVars() const;
    const char * getEnvironmentVar(int index) const;
    const char * getEnvironmentVarValue(int index) const;

    /// Override the search_path of the merged config.
    void setSearchPath(const char * path);
    void addSearchPath(const char * path);
    const char * getSearchPath() const;

    /// Override the active_displays of the merged config.
    void setActiveDisplays(const char * displays);
    const char * getActiveDisplays() const;

    /// Override the active_views of the merged config.
    void setActiveViews(const char * views);
    const char * getActiveViews() const;

    /// Override the inactive_colorspaces of the merged config.
    void setInactiveColorSpaces(const char * colorspaces);
    const char * getInactiveColorSpaces() const;

    // Config section strategies

    /// Set the merge strategy for the roles section.
    void setRoles(MergeStrategies strategy);
    MergeStrategies getRoles() const;

    /// Set the merge strategy for the file_rules section.
    void setFileRules(MergeStrategies strategy);
    MergeStrategies getFileRules() const;

    /// Set the merge strategy for the displays/views section.
    /// This includes shared_views, displays, viewing_rules,
    /// virtual_display, active_display, and active_views.  
    void setDisplayViews(MergeStrategies strategy);
    MergeStrategies getDisplayViews() const;

    /// Set the merge strategy for the view_transforms section.
    /// This includes the view_transforms and default_view_transform.
    void setViewTransforms(MergeStrategies strategy);
    MergeStrategies getViewTransforms() const;
    
    /// Set the merge strategy for the looks section.
    void setLooks(MergeStrategies strategy);
    MergeStrategies getLooks() const;

    /// Set the merge strategy for the color spaces section.
    /// This includes colorspaces, display_colorspaces, environment, search_path,
    /// family_separator, and inactive_colorspaces.
    void setColorspaces(MergeStrategies strategy);
    MergeStrategies getColorspaces() const;
    
    /// Set the merge strategy for the named_transforms section.
    void setNamedTransforms(MergeStrategies strategy);
    MergeStrategies getNamedTransforms() const;

    ConfigMergingParameters(const ConfigMergingParameters &) = delete;
    ConfigMergingParameters& operator= (const ConfigMergingParameters &) = delete;

    /// Do not use (needed only for pybind11).
    ~ConfigMergingParameters();

private:
    ConfigMergingParameters();

    static void deleter(ConfigMergingParameters * c);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const ConfigMergingParameters &);

/**
 * The ConfigMerger class is the controller for the merging process.
 * 
 * It may be read from or serialized to an OCIOM file.
 *
 * It is controlling the search_path to find the base and input config, and the merge parameters.
 * 
 * It contains an instance of ConfigMergingParameters for each merge present under the "merge" 
 * section.
 * 
 * For example, consider the following OCIOM file contents:
 * 
 *   ociom_version: 1.0
 *   search_path:
 *     - /usr/local/configs
 *     - .
 *   merge:
 *     Merge_ADD_THIS:
 *       [...]
 *     Merge_ADD_THAT:
 *       [...]
 * 
 * For this OCIOM, there would be two instances of ConfigMergingParameters.
 * One for the merge with output name "Merge_ADD_THIS" and one for "Merge_ADD_THAT".
 * 
 * Where the [...] sections have the following structure:
 *
 *    Merge_ADD_THIS:
 *      base: base.ocio
 *      input: input.ocio
 *      options:
 *        input_family_prefix: ""
 *        base_family_prefix: ""
 *        input_first: true
 *        error_on_conflict: false
 *        default_strategy: PreferInput
 *        avoid_duplicates: true
 *        adjust_input_reference_space: true
 *      overrides:
 *        name: ""
 *        description: ""
 *        search_path: ""
 *        environment: {}
 *        active_displays: []
 *        active_views: []
 *        inactive_colorspaces: []
 *      params:
 *        roles:
 *          strategy: PreferBase
 *        file_rules:
 *          strategy: PreferInput
 *        display-views:
 *          strategy: InputOnly
 *        view_transforms:
 *          strategy: InputOnly
 *        looks:
 *          strategy: BaseOnly
 *        colorspaces:
 *          strategy: PreferInput
 *        named_transform:
 *          strategy: Remove
 * 
 * The indentation is significant and must be as shown. Default items may be omitted.
 *
 */
class OCIOEXPORT ConfigMerger
{
public:
    static ConfigMergerRcPtr Create();

    // Create by parsing an OCIOM file.
    static ConstConfigMergerRcPtr CreateFromFile(const char * filepath);

    ConfigMergerRcPtr createEditableCopy() const;

    /// These search paths are used to locate the input and base config.
    /// Set the entire search path. The ':' character is used to separate paths.
    void setSearchPath(const char * path);
    /// Add a single path to the search paths.
    void addSearchPath(const char * path);
    /// Get the number of search paths.
    int getNumSearchPaths() const;
    const char * getSearchPath(int index) const;

    /**
     * \brief Set the home directory used to resolve relative search paths.
     * 
     * The working directory defaults to the location of the OCIOM file. It is used to convert
     * any relative paths to absolute. If no search paths have been set, the working directory
     * will be used as the fallback search path. 
     */
    void setWorkingDir(const char * dirname);
    const char * getWorkingDir() const;

    /// Get the parameters for one of the merges. Returns null if index is out of range.
    ConfigMergingParametersRcPtr getParams(int index) const;
    int getNumConfigMergingParameters() const;
    void addParams(ConfigMergingParametersRcPtr params);

    /**
     * \brief Execute the merge(s) based on the merger object.
     * 
     * Execute the merge(s) based on the merger object that was previously populated by using 
     * ConfigMerger::CreateFromFile or created from scratch by using ConfigMerger::Create() and 
     * programmatically configuring it.
     * 
     * \return a merger object (call getMergedConfig to obtain the result)
     */
    ConstConfigMergerRcPtr mergeConfigs() const;

    /// Get the final merged config.
    ConstConfigRcPtr getMergedConfig() const;
    /// Get one of the merged configs (if there are a series of merges).  Returns null 
    /// if index is out of range.
    ConstConfigRcPtr getMergedConfig(int index) const;
    int getNumMergedConfigs() const;

    /// Serialize to the OCIOM file format.
    void serialize(std::ostream& os) const;

    /// Set the version of the OCIOM file format.
    void setVersion(unsigned int major, unsigned int minor);
    unsigned int getMajorVersion() const;
    unsigned int getMinorVersion() const;
    
    ConfigMerger(const ConfigMerger &) = delete;
    ConfigMerger & operator=(const ConfigMerger &) = delete;

    /// Do not use (needed only for pybind11).
    ~ConfigMerger();

private:
    ConfigMerger();

    static void deleter(ConfigMerger * c);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const ConfigMerger &);

namespace ConfigMergingHelpers
{

/**
 * \brief Merge the input into the base config, using the supplied merge parameters.
 * 
 * \param params ConfigMergingParameters controlling the merger.
 * \param params The base config.
 * \param params The input config to merge.
 * \return The merged config object.
 */
extern OCIOEXPORT ConfigRcPtr MergeConfigs(const ConfigMergingParametersRcPtr & params,
                                           const ConstConfigRcPtr & baseConfig,
                                           const ConstConfigRcPtr & inputConfig);

/**
 * \brief Merge a single color space into the base config, using the supplied merge parameters.
 * 
 * Note that the AdjustInputReferenceSpace merge parameter will be ignored and set to false.
 * To use automatic reference space conversion, add the color space to an input config that
 * has the necessary interchange role set.
 *
 * \param params ConfigMergingParameters controlling the merger.
 * \param params The base config.
 * \param params The input color space to merge.
 * \return The merged config object.
 */
extern OCIOEXPORT ConfigRcPtr MergeColorSpace(const ConfigMergingParametersRcPtr & params,
                                              const ConstConfigRcPtr & baseConfig,
                                              const ConstColorSpaceRcPtr & colorspace);

} // ConfigMergingHelpers

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_OPENCOLORAPPHELPERS_H
