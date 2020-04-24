// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_OPENCOLORIO_H
#define INCLUDED_OCIO_OPENCOLORIO_H

#include <cstddef>
#include <iosfwd>
#include <limits>
#include <stdexcept>
#include <string>

#include "OpenColorABI.h"
#include "OpenColorTypes.h"
#include "OpenColorTransforms.h"

/*!rst::
C++ API
=======

**Usage Example:** *Compositing plugin that converts from "log" to "lin"*

.. code-block:: cpp

   #include <OpenColorIO/OpenColorIO.h>
   namespace OCIO = OCIO_NAMESPACE;

   try
   {
       // Get the global OpenColorIO config
       // This will auto-initialize (using $OCIO) on first use
       OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

       // Get the processor corresponding to this transform.
       OCIO::ConstProcessorRcPtr processor = config->getProcessor(OCIO::ROLE_COMPOSITING_LOG,
                                                                  OCIO::ROLE_SCENE_LINEAR);

       // Get the corresponding CPU processor for 32-bit float image processing.
       OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();

       // Wrap the image in a light-weight ImageDescription
       OCIO::PackedImageDesc img(imageData, w, h, 4);

       // Apply the color transformation (in place)
       cpuProcessor->apply(img);
   }
   catch(OCIO::Exception & exception)
   {
       std::cerr << "OpenColorIO Error: " << exception.what() << std::endl;
   }

*/

namespace OCIO_NAMESPACE
{
///////////////////////////////////////////////////////////////////////////
//!rst::
// Exceptions
// **********

//!cpp:class:: An exception class to throw for errors detected at
// runtime.
//
// .. warning::
//    All functions in the Config class can potentially throw this exception.
class OCIOEXPORT Exception : public std::runtime_error
{
public:
    //!cpp:function::
    Exception() = delete;
    //!cpp:function:: Constructor that takes a string as the exception message.
    explicit Exception(const char *);
    //!cpp:function:: Constructor that takes an existing exception.
    Exception(const Exception &);
    //!cpp:function::
    Exception & operator= (const Exception &) = delete;

    //!cpp:function::
    ~Exception();
};

//!cpp:class:: An exception class for errors detected at
// runtime, thrown when OCIO cannot find a file that is expected to
// exist. This is provided as a custom type to
// distinguish cases where one wants to continue looking for
// missing files, but wants to properly fail
// for other error conditions.

class OCIOEXPORT ExceptionMissingFile : public Exception
{
public:
    //!cpp:function::
    ExceptionMissingFile() = delete;
    //!cpp:function:: Constructor that takes a string as the exception message.
    explicit ExceptionMissingFile(const char *);
    //!cpp:function:: Constructor that takes an existing exception.
    ExceptionMissingFile(const ExceptionMissingFile &);
    //!cpp:function::
    ExceptionMissingFile & operator= (const ExceptionMissingFile &) = delete;

    //!cpp:function::
    ~ExceptionMissingFile();
};

///////////////////////////////////////////////////////////////////////////
//!rst::
// Global
// ******

//!cpp:function::
// OpenColorIO, during normal usage, tends to cache certain information
// (such as the contents of LUTs on disk, intermediate results, etc.).
// Calling this function will flush all such information.
// Under normal usage, this is not necessary, but it can be helpful in particular instances,
// such as designing OCIO profiles, and wanting to re-read luts without
// restarting.
extern OCIOEXPORT void ClearAllCaches();

//!cpp:function:: Get the version number for the library, as a
// dot-delimited string (e.g., "1.0.0"). This is also available
// at compile time as OCIO_VERSION.
extern OCIOEXPORT const char * GetVersion();

//!cpp:function:: Get the version number for the library, as a
// single 4-byte hex number (e.g., 0x01050200 for "1.5.2"), to be used
// for numeric comparisons. This is also available
// at compile time as OCIO_VERSION_HEX.
extern OCIOEXPORT int GetVersionHex();

//!cpp:function:: Get the global logging level.
// You can override this at runtime using the :envvar:`OCIO_LOGGING_LEVEL`
// environment variable. The client application that sets this should use
// :cpp:func:`SetLoggingLevel`, and not the environment variable. The default value is INFO.
extern OCIOEXPORT LoggingLevel GetLoggingLevel();

//!cpp:function:: Set the global logging level.
extern OCIOEXPORT void SetLoggingLevel(LoggingLevel level);

//!cpp:function:: Set the logging function to use; otherwise, use the default (i.e. std::cerr).
// Note that the logging mechanism is thread-safe.
extern OCIOEXPORT void SetLoggingFunction(LoggingFunction logFunction);
//!cpp:function::
extern OCIOEXPORT void ResetToDefaultLoggingFunction();
//!cpp:function:: Log a message using the library logging function.
extern OCIOEXPORT void LogMessage(LoggingLevel level, const char * message);

//
// Note that the following env. variable access methods are not thread safe.
//

//!cpp:function:: Another call modifies the string obtained from a previous call
// as the method always uses the same memory buffer.
extern OCIOEXPORT const char * GetEnvVariable(const char * name);
//!cpp:function::
extern OCIOEXPORT void SetEnvVariable(const char * name, const char * value);


///////////////////////////////////////////////////////////////////////////
//!rst::
// Config
// ******
//
// A config defines all the color spaces to be available at runtime.
//
// The color configuration (:cpp:class:`Config`) is the main object for
// interacting with this library. It encapsulates all of the information
// necessary to use customized :cpp:class:`ColorSpaceTransform` and
// :cpp:class:`DisplayTransform` operations.
//
// See the :ref:`user-guide` for more information on
// selecting, creating, and working with custom color configurations.
//
// For applications interested in using only one color config at
// a time (this is the vast majority of apps), their API would
// traditionally get the global configuration and use that, as opposed to
// creating a new one. This simplifies the use case for
// plugins and bindings, as it alleviates the need to pass around configuration
// handles.
//
// An example of an application where this would not be sufficient would be
// a multi-threaded image proxy server (daemon), which wished to handle
// multiple show configurations in a single process concurrently. This
// app would need to keep multiple configurations alive, and to manage them
// appropriately.
//
// Roughly speaking, a novice user should select a
// default configuration that most closely approximates the use case
// (animation, visual effects, etc.), and set the :envvar:`OCIO` environment
// variable to point at the root of that configuration.
//
// .. note::
//    Initialization using environment variables is typically preferable in
//    a multi-app ecosystem, as it allows all applications to be
//    consistently configured.
//
// See :ref:`developers-usageexamples`

//!cpp:function:: Get the current configuration.

extern OCIOEXPORT ConstConfigRcPtr GetCurrentConfig();

//!cpp:function:: Set the current configuration. This will then store a copy of the specified config.
extern OCIOEXPORT void SetCurrentConfig(const ConstConfigRcPtr & config);


//!cpp:class::
class OCIOEXPORT Config
{
public:

    ///////////////////////////////////////////////////////////////////////////
    //!rst:: .. _cfginit_section:
    // 
    // Initialization
    // ^^^^^^^^^^^^^^

    //!cpp:function:: Create a default empty configuration.
    static ConfigRcPtr Create();
    //!cpp:function:: Create a fall-back config.  This may be useful to allow client apps
    // to launch in cases when the supplied config path is not loadable.
    static ConstConfigRcPtr CreateRaw();
    //!cpp:function:: Create a configuration using the OCIO environment variable.  If the
    // variable is missing or empty, returns the same result as :cpp:func:`Config::CreateRaw`.
    static ConstConfigRcPtr CreateFromEnv();
    //!cpp:function:: Create a configuration using a specific config file.
    static ConstConfigRcPtr CreateFromFile(const char * filename);
    //!cpp:function:: Create a configuration using a stream.
    static ConstConfigRcPtr CreateFromStream(std::istream & istream);

    //!cpp:function::
    ConfigRcPtr createEditableCopy() const;

    //!cpp:function:: Get the configuration major version
    unsigned int getMajorVersion() const;

    //!cpp:function:: Set the configuration major version
    void setMajorVersion(unsigned int major);

    //!cpp:function:: Get the configuration minor version
    unsigned int getMinorVersion() const;

    //!cpp:function:: Set the configuration minor version
    void setMinorVersion(unsigned int minor);

    //!cpp:function:: Allows an older config to be serialized as the current version.
    void upgradeToLatestVersion();

    //!cpp:function::
    // This will throw an exception if the config is malformed. The most
    // common error occurs when references are made to colorspaces that do not
    // exist.
    void sanityCheck() const;

    //!cpp:function:: If not empty or null a single character to separate
    // the family string in levels.
    char getFamilySeparator() const;
    //!cpp:function:: Succeeds if the characters is null or a valid character
    // from the ASCII table i.e. from value 32 (i.e. space) to 126 (i.e. '~');
    // otherwise, it throws an exception.
    void setFamilySeparator(char separator);

    //!cpp:function::
    const char * getDescription() const;
    //!cpp:function::
    void setDescription(const char * description);

    //!cpp:function::
    // Returns the string representation of the Config in YAML text form.
    // This is typically stored on disk in a file with the extension .ocio.
    void serialize(std::ostream & os) const;

    //!cpp:function::
    // This will produce a hash of the all colorspace definitions, etc.
    // All external references, such as files used in FileTransforms, etc.,
    // will be incorporated into the cacheID. While the contents of
    // the files are not read, the file system is queried for relevant
    // information (mtime, inode) so that the config's cacheID will
    // change when the underlying luts are updated.
    // If a context is not provided, the current Context will be used.
    // If a null context is provided, file references will not be taken into
    // account (this is essentially a hash of Config::serialize).
    const char * getCacheID() const;
    //!cpp:function::
    const char * getCacheID(const ConstContextRcPtr & context) const;

    ///////////////////////////////////////////////////////////////////////////
    //!rst:: .. _cfgresource_section:
    // 
    // Resources
    // ^^^^^^^^^
    // Given a lut src name, where should we find it?

    //!cpp:function::
    ConstContextRcPtr getCurrentContext() const;

    //!cpp:function::
    void addEnvironmentVar(const char * name, const char * defaultValue);
    //!cpp:function::
    int getNumEnvironmentVars() const;
    //!cpp:function::
    const char * getEnvironmentVarNameByIndex(int index) const;
    //!cpp:function::
    const char * getEnvironmentVarDefault(const char * name) const;
    //!cpp:function::
    void clearEnvironmentVars();
    //!cpp:function::
    void setEnvironmentMode(EnvironmentMode mode);
    //!cpp:function::
    EnvironmentMode getEnvironmentMode() const;
    //!cpp:function::
    void loadEnvironment();

    //!cpp:function::
    const char * getSearchPath() const;
    //!cpp:function:: Set all search paths as a concatenated string, using
    // ':' to separate the paths.  See addSearchPath for a more robust and
    // platform-agnostic method of setting the search paths.
    void setSearchPath(const char * path);

    //!cpp:function::
    int getNumSearchPaths() const;
    //!cpp:function:: Get a search path from the list. The paths are in the
    // order they will be searched (that is, highest to lowest priority).
    const char * getSearchPath(int index) const;
    //!cpp:function::
    void clearSearchPaths();
    //!cpp:function:: Add a single search path to the end of the list.
    // Paths may be either absolute or relative. Relative paths are
    // relative to the working directory. Forward slashes will be
    // normalized to reverse for Windows. Environment (context) variables
    // may be used in paths.
    void addSearchPath(const char * path);

    //!cpp:function::
    const char * getWorkingDir() const;
    //!cpp:function:: The working directory defaults to the location of the
    // config file. It is used to convert any relative paths to absolute.
    // If no search paths have been set, the working directory will be used
    // as the fallback search path. No environment (context) variables may
    // be used in the working directory.
    void setWorkingDir(const char * dirname);

    ///////////////////////////////////////////////////////////////////////////
    //!rst:: .. _cfgcolorspaces_section:
    // 
    // ColorSpaces
    // ^^^^^^^^^^^

    //!cpp:function:: Get all active color spaces having a specific category
    // in the order they appear in the config file.
    //
    // .. note::
    //    If the category is null or empty, the method returns
    //    all the active color spaces like :cpp:func:`Config::getNumColorSpaces`
    //    and :cpp:func:`Config::getColorSpaceNameByIndex` do.
    //
    // .. note::
    //    It's worth noticing that the method returns a copy of the
    //    selected color spaces decoupling the result from the config.
    //    Hence, any changes on the config do not affect the existing
    //    color space sets, and vice-versa.
    //
    ColorSpaceSetRcPtr getColorSpaces(const char * category) const;

    //!cpp:function:: Work on the color spaces selected by the reference color space type
    // and visibility.
    int getNumColorSpaces(SearchReferenceSpaceType searchReferenceType,
                          ColorSpaceVisibility visibility) const;

    //!cpp:function:: Work on the color spaces selected by the reference color space type
    // and visibility (active or inactive). Return empty for invalid index.
    const char * getColorSpaceNameByIndex(SearchReferenceSpaceType searchReferenceType,
                                          ColorSpaceVisibility visibility, int index) const;

    //!cpp:function:: Get the color space from all the color spaces
    // (i.e. active and inactive) and return null if the name is not found.
    //
    // .. note::
    //    The fcn accepts either a color space OR role name.
    //    (Color space names take precedence over roles.)
    ConstColorSpaceRcPtr getColorSpace(const char * name) const;

    // The following three methods only work from the list of active color spaces.

    //!cpp:function:: Work on the active color spaces only.
    int getNumColorSpaces() const;

    //!cpp:function:: Work on the active color spaces only and return null for invalid index.
    const char * getColorSpaceNameByIndex(int index) const;

    //!cpp:function:: Get an index from the active color spaces only
    // and return -1 if the name is not found.
    //
    // .. note::
    //    The fcn accepts either a color space OR role name.
    //    (Color space names take precedence over roles.)
    int getIndexForColorSpace(const char * name) const;

    //!cpp:function:: Add a color space to the configuration.
    //
    // .. note::
    //    If another color space is already present with the same name,
    //    this will overwrite it. This stores a copy of the specified
    //    color space.
    // .. note::
    //    Adding a color space to a :cpp:class:`Config` does not affect any
    //    :cpp:class:`ColorSpaceSet` sets that have already been created.
    void addColorSpace(const ConstColorSpaceRcPtr & cs);

    //!cpp:function:: Remove a color space from the configuration.
    //
    // .. note::
    //    It does not throw an exception if the color space is not present
    //    or used by an existing role.  Role name arguments are ignored.
    // .. note::
    //    Removing a color space to a :cpp:class:`Config` does not affect any
    //    :cpp:class:`ColorSpaceSet` sets that have already been created.
    void removeColorSpace(const char * name);

    //!cpp:function:: Return true if the color space is used by a transform, a role, or a look.
    bool isColorSpaceUsed(const char * name) const noexcept;

    //!cpp:function:: Remove all the color spaces from the configuration.
    //
    // .. note::
    //    Removing color spaces from a :cpp:class:`Config` does not affect
    //    any :cpp:class:`ColorSpaceSet` sets that have already been created.
    void clearColorSpaces();

    //!cpp:function:: Given the specified string, get the longest,
    // right-most, colorspace substring that appears.
    //
    // * If strict parsing is enabled, and no color space is found, return
    //   an empty string.
    // * If strict parsing is disabled, return ROLE_DEFAULT (if defined).
    // * If the default role is not defined, return an empty string.
    const char * parseColorSpaceFromString(const char * str) const;

    //!cpp:function::
    bool isStrictParsingEnabled() const;
    //!cpp:function::
    void setStrictParsingEnabled(bool enabled);

    //!cpp:function:: Set/get a list of inactive color space names.
    //
    // * The inactive spaces are color spaces that should not appear in application menus.
    // * These color spaces will still work in :cpp:func:`Config::getProcessor` calls.
    // * The argument is a comma-delimited string.  A null or empty string empties the list.
    // * The environment variable OCIO_INACTIVE_COLORSPACES may also be used to set the
    //   inactive color space list.
    // * The env. var. takes precedence over the inactive_colorspaces list in the config file.
    // * Setting the list via the API takes precedence over either the env. var. or the
    //   config file list.
    // * Roles may not be used.
    void setInactiveColorSpaces(const char * inactiveColorSpaces);
    //!cpp:function::
    const char * getInactiveColorSpaces() const;

    ///////////////////////////////////////////////////////////////////////////
    //!rst:: .. _cfgroles_section:
    //
    // Roles
    // ^^^^^
    // A role is like an alias for a colorspace. You can query the colorspace
    // corresponding to a role using the normal getColorSpace fcn.

    //!cpp:function::
    // .. note::
    //    Setting the ``colorSpaceName`` name to a null string unsets it.
    void setRole(const char * role, const char * colorSpaceName);
    //!cpp:function::
    int getNumRoles() const;
    //!cpp:function:: Return true if the role has been defined.
    bool hasRole(const char * role) const;
    //!cpp:function:: Get the role name at index, this will return values
    // like 'scene_linear', 'compositing_log'.
    // Return empty string if index is out of range.
    const char * getRoleName(int index) const;
    //!cpp:function:: Get the role color space at index.
    // Return empty string if index is out of range.
    const char * getRoleColorSpace(int index) const;

    ///////////////////////////////////////////////////////////////////////////
    //!rst:: .. _cfgdisplayview_section:
    //
    // Display/View Registration
    // ^^^^^^^^^^^^^^^^^^^^^^^^^
    //
    // The following methods only manipulate active displays and views. Active
    // displays and views are defined from an envvar or from the config file.
    //
    // Looks is a potentially comma (or colon) delimited list of lookNames,
    // Where +/- prefixes are optionally allowed to denote forward/inverse
    // look specification. (And forward is assumed in the absence of either)

    //!cpp:function::
    const char * getDefaultDisplay() const;
    //!cpp:function::
    int getNumDisplays() const;
    //!cpp:function::
    const char * getDisplay(int index) const;

    //!cpp:function::
    const char * getDefaultView(const char * display) const;
    //!cpp:function::
    int getNumViews(const char * display) const;
    //!cpp:function::
    const char * getView(const char * display, int index) const;

    //!cpp:function:: Returns the view_transform attribute of the (display, view) pair.
    const char * getDisplayViewTransformName(const char * display, const char * view) const;
    //!cpp:function:: Returns the colorspace attribute of the (display, view) pair.
    // (Note that this may be either a color space or a display color space.)
    const char * getDisplayColorSpaceName(const char * display, const char * view) const;
    //!cpp:function:: Returns the looks attribute of a (display, view) pair.
    const char * getDisplayLooks(const char * display, const char * view) const;

    //!cpp:function:: For the (display, view) pair, specify which color space and look to use.
    // If a look is not desired, then just pass a null or empty string.
    void addDisplay(const char * display, const char * view,
                    const char * colorSpaceName, const char * looks);

    //!cpp:function:: For the (display, view) pair, specify a viewTransform + displayColorSpace
    // to use.  (Looks work the same as above.)
    void addDisplay(const char * display, const char * view, const char * viewTransform,
                    const char * displayColorSpaceName, const char * looks);

    //!cpp:function:: Remove the view and the display if no more views. It does not remove
    // the associated color space.
    void removeDisplay(const char * display, const char * view);

    //!cpp:function::
    void clearDisplays();

    // $OCIO_ACTIVE_DISPLAYS envvar can, at runtime, optionally override the allowed displays.
    // It is a comma or colon delimited list.
    // Active displays that are not in the specified profile will be ignored, and the
    // left-most defined display will be the default.

    //!cpp:function:: Comma-delimited list of names to filter and order the active displays.
    // .. note:: The setter does not override the envvar.  The getter does not take into
    // account the envvar value and thus may not represent what the user is seeing.
    void setActiveDisplays(const char * displays);
    //!cpp:function::
    const char * getActiveDisplays() const;

    // $OCIO_ACTIVE_VIEWS envvar can, at runtime, optionally override the allowed views.
    // It is a comma or colon delimited list.
    // Active views that are not in the specified profile will be ignored, and the
    // left-most defined view will be the default.

    //!cpp:function:: Comma-delimited list of names to filter and order the active views.
    // .. note:: The setter does not override the envvar.  The getter does not take into
    // account the envvar value and thus may not represent what the user is seeing.
    void setActiveViews(const char * views);
    //!cpp:function::
    const char * getActiveViews() const;

    ///////////////////////////////////////////////////////////////////////////
    //!rst:: .. _cfgluma_section:
    // 
    // Luma
    // ^^^^
    //
    // Get the default coefficients for computing luma.
    //
    // .. note::
    //    There is no "1 size fits all" set of luma coefficients. (The
    //    values are typically different for each colorspace, and the
    //    application of them may be nonsensical depending on the
    //    intensity coding anyways). Thus, the 'right' answer is to make
    //    these functions on the :cpp:class:`Config` class. However, it's
    //    often useful to have a config-wide default so here it is. We will
    //    add the colorspace specific luma call if/when another client is
    //    interesting in using it.

    //!cpp:function::
    void getDefaultLumaCoefs(double * rgb) const;
    //!cpp:function:: These should be normalized (sum to 1.0 exactly).
    void setDefaultLumaCoefs(const double * rgb);


    ///////////////////////////////////////////////////////////////////////////
    //!rst:: .. _cflooka_section:
    // 
    // Look
    // ^^^^
    //
    // Manager per-shot look settings.
    //

    //!cpp:function::
    ConstLookRcPtr getLook(const char * name) const;

    //!cpp:function::
    int getNumLooks() const;

    //!cpp:function::
    const char * getLookNameByIndex(int index) const;

    //!cpp:function::
    void addLook(const ConstLookRcPtr & look);

    //!cpp:function::
    void clearLooks();


    ///////////////////////////////////////////////////////////////////////////
    //!rst:: .. _cfgview_transforms_section:
    //
    // View Transforms
    // ^^^^^^^^^^^^^^^
    //
    // :cpp:class:`ViewTransform` objects are used with the display reference space.

    //!cpp:function::
    int getNumViewTransforms() const noexcept;

    //!cpp:function::
    ConstViewTransformRcPtr getViewTransform(const char * name) const noexcept;

    //!cpp:function::
    const char * getViewTransformNameByIndex(int i) const noexcept;

    //!cpp:function::
    void addViewTransform(const ConstViewTransformRcPtr & viewTransform);

    //!cpp:function:: The default transform to use for scene-referred to display-referred
    // reference space conversions is the first scene-referred view transform listed in
    // that section of the config (the one with the lowest index).  Returns a null
    // ConstTransformRcPtr if there isn't one.
    ConstViewTransformRcPtr getDefaultSceneToDisplayViewTransform() const;

    //!cpp:function::
    void clearViewTransforms();

    ///////////////////////////////////////////////////////////////////////////
    //!rst:: .. _cfgfilerules_section:
    // 
    // File Rules
    // ^^^^^^^^^^
    //
    // See :cpp:class:`FileRules`.

    //!cpp:function:: Get read-only version of the file rules.
    ConstFileRulesRcPtr getFileRules() const noexcept;

    //!cpp:function:: Set file rules.
    // .. note::
    //    The argument is cloned.
    void setFileRules(ConstFileRulesRcPtr fileRules);

    // !cpp:function:: Get the color space of the first rule that matched filePath.
    const char * getColorSpaceFromFilepath(const char * filePath) const;

    //!cpp:function:: Most applications will use the preceding method, but this method may be
    // used for applications that want to know which was the highest priority rule to match
    // filePath.  The :cpp:func:`FileRules::getNumCustomKeys` and custom keys methods
    // may then be used to get additional information about the matching rule.
    const char * getColorSpaceFromFilepath(const char * filePath, size_t & ruleIndex) const;

    // !cpp:function:: Returns true if the only rule matched by filePath is the default rule.
    // This is a convenience method for applications that want to require the user to manually
    // choose a color space when strictParsing is true and no other rules match.
    bool filepathOnlyMatchesDefaultRule(const char * filePath) const;

    ///////////////////////////////////////////////////////////////////////////
    //!rst:: .. _cfgprocessors_section:
    //
    // Processors
    // ^^^^^^^^^^
    //
    // Create a :cpp:class:`Processor` to assemble a transformation between two
    // color spaces.  It may then be used to create a :cpp:class:`CPUProcessor`
    // or :cpp:class:`GPUProcessor` to process/convert pixels.

    //!rst:: Get the processor to apply a ColorSpaceTransform from a source to a destination
    // color space.

    //!cpp:function::
    ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                        const ConstColorSpaceRcPtr & srcColorSpace,
                                        const ConstColorSpaceRcPtr & dstColorSpace) const;
    //!cpp:function::
    ConstProcessorRcPtr getProcessor(const ConstColorSpaceRcPtr & srcColorSpace,
                                        const ConstColorSpaceRcPtr & dstColorSpace) const;

    //!cpp:function::
    // .. note::
    //    Names can be colorspace name, role name, or a combination of both.
    ConstProcessorRcPtr getProcessor(const char * srcName,
                                        const char * dstName) const;
    //!cpp:function::
    ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                        const char * srcName,
                                        const char * dstName) const;

    //!rst:: Get the processor to apply a DisplayTransform for a display and view.  Refer to the
    // Display/View Registration section above for more info on the display and view arguments.

    //!cpp:function::
    ConstProcessorRcPtr getProcessor(const char * inputColorSpaceName,
                                     const char * display, const char * view) const;
    //!cpp:function::
    ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                     const char * inputColorSpaceName,
                                     const char * display, const char * view) const;

    //!rst:: Get the processor for the specified transform.
    //
    // Not often needed, but will allow for the re-use of atomic OCIO
    // functionality (such as to apply an individual LUT file).

    //!cpp:function::
    ConstProcessorRcPtr getProcessor(const ConstTransformRcPtr& transform) const;
    //!cpp:function::
    ConstProcessorRcPtr getProcessor(const ConstTransformRcPtr& transform,
                                        TransformDirection direction) const;
    //!cpp:function::
    ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                        const ConstTransformRcPtr& transform,
                                        TransformDirection direction) const;

    //!rst: Get a processor to convert between color spaces in two separate configs.

    //!cpp:function:: This relies on both configs having the aces_interchange role (when srcName
    // is scene-referred) or the role cie_xyz_d65_interchange (when srcName is display-referred)
    // defined.  An exception is thrown if that is not the case.
    static ConstProcessorRcPtr GetProcessor(const ConstConfigRcPtr & srcConfig,
                                            const char * srcName,
                                            const ConstConfigRcPtr & dstConfig,
                                            const char * dstName);
    //!cpp:function::
    static ConstProcessorRcPtr GetProcessor(const ConstContextRcPtr & srcContext, 
                                            const ConstConfigRcPtr & srcConfig,
                                            const char * srcName,
                                            const ConstContextRcPtr & dstContext,
                                            const ConstConfigRcPtr & dstConfig,
                                            const char * dstName);

    //!cpp:function:: The srcInterchangeName and dstInterchangeName must refer to a pair of
    // color spaces in the two configs that are the same.  A role name may also be used.
    static ConstProcessorRcPtr GetProcessor(const ConstConfigRcPtr & srcConfig,
                                            const char * srcName,
                                            const char * srcInterchangeName,
                                            const ConstConfigRcPtr & dstConfig,
                                            const char * dstName,
                                            const char * dstInterchangeName);
    //!cpp:function::
    static ConstProcessorRcPtr GetProcessor(const ConstContextRcPtr & srcContext,
                                            const ConstConfigRcPtr & srcConfig,
                                            const char * srcName,
                                            const char * srcInterchangeName,
                                            const ConstContextRcPtr & dstContext,
                                            const ConstConfigRcPtr & dstConfig,
                                            const char * dstName,
                                            const char * dstInterchangeName);

    //!cpp:function::
    Config(const Config &) = delete;
    //!cpp:function::
    Config& operator= (const Config &) = delete;
    //!cpp:function::
    ~Config();

private:
    Config();

    static void deleter(Config* c);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Config&);

///////////////////////////////////////////////////////////////////////////
//!rst:: .. _filerules_section:
//
// FileRules
// *********
// The File Rules are a set of filepath to color space mappings that are evaluated
// from first to last. The first rule to match is what determines which color space is
// returned. There are four types of rules available. Each rule type has a name key that may
// be used by applications to refer to that rule. Name values must be unique i.e. using a
// case insensitive comparison. The other keys depend on the rule type:
//
// - Basic Rule: This is the basic rule type that uses Unix glob style pattern matching and
//   is thus very easy to use. It contains the keys:
//
//   * name: Name of the rule
//
//   * colorspace: Color space name to be returned.
//
//   * pattern: Glob pattern to be used for the main part of the name/path.
//
//   * extension: Glob pattern to be used for the file extension. Note that if glob tokens
//     are not used, the extension will be used in a non-case-sensitive way by default.
//
// - Regex Rule: This is similar to the basic rule but allows additional capabilities for
//   power-users. It contains the keys:
//
//   * name: Name of the rule
//
//   * colorspace: Color space name to be returned.
//
//   * regex: Regular expression to be evaluated.
//
// - OCIO v1 style Rule: This rule allows the use of the OCIO v1 style, where the string
//   is searched for color space names from the config. This rule may occur 0 or 1 times
//   in the list. The position in the list prioritizes it with respect to the other rules.
//   StrictParsing is not used. If no color space is found in the path, the rule will not
//   match and the next rule will be considered.
//   See :cpp:func:`FileRules::insertPathSearchRule`.
//   It has the key:
//
//   * name: Must be "ColorSpaceNamePathSearch".
//
// - Default Rule: The file_rules must always end with this rule. If no prior rules match,
//   this rule specifies the color space applications will use.
//   See :cpp:func:`FileRules::setDefaultRuleColorSpace`.
//   It has the keys:
//
//   * name: must be "Default".
//
//   * colorspace : Color space name to be returned.
//
// Custom string keys and associated string values may be used to convey app or
// workflow-specific information, e.g. whether the color space should be left as is
// or converted into a working space.
//
// Getters and setters are using the rule position, they will throw if the position is not
// valid. If the rule at the specified position does not implement the requested property
// getter will return NULL and setter will throw.
//

//!cpp:class::
class FileRules
{
public:
    //!cpp:function:: Creates FileRules for a Config. File rules will contain the default rule
    // using the default role. The default rule cannot be removed.
    static FileRulesRcPtr Create();

    //!cpp:function:: The method clones the content decoupling the two instances.
    FileRulesRcPtr createEditableCopy() const;

    //!cpp:function:: Does include default rule. Result will be at least 1.
    size_t getNumEntries() const noexcept;

    //!cpp:function:: Get the index from the rule name.
    size_t getIndexForRule(const char * ruleName) const;

    //!cpp:function:: Get name of the rule.
    const char * getName(size_t ruleIndex) const;

    //!cpp:function:: Setting pattern will erase regex.
    const char * getPattern(size_t ruleIndex) const;
    //!cpp:function::
    void setPattern(size_t ruleIndex, const char * pattern);

    //!cpp:function:: Setting extension will erase regex.
    const char * getExtension(size_t ruleIndex) const;
    //!cpp:function::
    void setExtension(size_t ruleIndex, const char * extension);

    //!cpp:function:: Setting a regex will erase pattern & extension.
    const char * getRegex(size_t ruleIndex) const;
    //!cpp:function::
    void setRegex(size_t ruleIndex, const char * regex);

    //!cpp:function:: Set the rule's color space (may also be a role).
    const char * getColorSpace(size_t ruleIndex) const;
    //!cpp:function::
    void setColorSpace(size_t ruleIndex, const char * colorSpace);

    //!cpp:function:: Get number of key/value pairs.
    size_t getNumCustomKeys(size_t ruleIndex) const;
    //!cpp:function:: Get name of key.
    const char * getCustomKeyName(size_t ruleIndex, size_t key) const;
    //!cpp:function:: Get name of value.
    const char * getCustomKeyValue(size_t ruleIndex, size_t key) const;
    //!cpp:function:: Adds a key/value or replace value if key exists. Setting a NULL or an
    // empty value will erase the key.
    void setCustomKey(size_t ruleIndex, const char * key, const char * value);

    //!cpp:function:: Insert a rule at a given ruleIndex. Rule currently at ruleIndex
    // will be pushed to index: ruleIndex + 1.
    // Name must be unique.
    // - "Default" is a reserved name for the default rule. The default rule is automatically
    // added and can't be removed. (see :cpp:func:`FileRules::setDefaultRuleColorSpace`).
    // - "ColorSpaceNamePathSearch" is also a reserved name
    // (see :cpp:func:`FileRules::insertPathSearchRule`).
    //
    // Will throw if ruleIndex is not less than :cpp:func:`FileRules::getNumEntries`.
    void insertRule(size_t ruleIndex, const char * name, const char * colorSpace,
                    const char * pattern, const char * extension);
    //!cpp:function::
    void insertRule(size_t ruleIndex, const char * name, const char * colorSpace,
                    const char * regex);
    //!cpp:function:: Helper function to insert a rule that uses
    // cpp:func:'Config:parseColorSpaceFromString` to search the path for any of the color spaces
    // named in the config (as per OCIO v1).
    void insertPathSearchRule(size_t ruleIndex);
    //!cpp:function:: Helper function tp set the color space for the default rule.
    void setDefaultRuleColorSpace(const char * colorSpace);

    //!cpp:function:: Default rule can't be removed.
    // Will throw if ruleIndex + 1 is not less than :cpp:func:`FileRules::getNumEntries`.
    void removeRule(size_t ruleIndex);

    //!cpp:function:: Move a rule closer to the start of the list by one position.
    void increaseRulePriority(size_t ruleIndex);

    //!cpp:function:: Move a rule closer to the end of the list by one position.
    void decreaseRulePriority(size_t ruleIndex);

    //!cpp:function::
    FileRules(const FileRules &) = delete;
    //!cpp:function::
    FileRules & operator= (const FileRules &) = delete;
    //!cpp:function::
    virtual ~FileRules();

private:
    FileRules();

    static void deleter(FileRules* c);

    friend class Config;

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};


///////////////////////////////////////////////////////////////////////////
//!rst:: .. _colorspace_section:
//
// ColorSpace
// **********
// The *ColorSpace* is the state of an image with respect to colorimetry
// and color encoding. Transforming images between different
// *ColorSpaces* is the primary motivation for this library.
//
// While a complete discussion of color spaces is beyond the scope of
// header documentation, traditional uses would be to have *ColorSpaces*
// corresponding to: physical capture devices (known cameras, scanners),
// and internal 'convenience' spaces (such as scene linear, logarithmic).
//
// *ColorSpaces* are specific to a particular image precision (float32,
// uint8, etc.), and the set of ColorSpaces that provide equivalent mappings
// (at different precisions) are referred to as a 'family'.

//!cpp:class::
class OCIOEXPORT ColorSpace
{
public:
    //!cpp:function::
    static ColorSpaceRcPtr Create();

    //!cpp:function::
    static ColorSpaceRcPtr Create(ReferenceSpaceType referenceSpace);

    //!cpp:function::
    ColorSpaceRcPtr createEditableCopy() const;

    //!cpp:function::
    const char * getName() const noexcept;
    //!cpp:function::
    void setName(const char * name);

    //!cpp:function::Get the family, for use in user interfaces (optional)
    // The family string could use a '/' separator to indicate levels to be used
    // by hierarchical menus.
    const char * getFamily() const noexcept;
    //!cpp:function::Set the family, for use in user interfaces (optional)
    void setFamily(const char * family);

    //!cpp:function::Get the ColorSpace group name (used for equality comparisons)
    // This allows no-op transforms between different colorspaces.
    // If an equalityGroup is not defined (an empty string), it will be considered
    // unique (i.e., it will not compare as equal to other ColorSpaces with an
    // empty equality group).  This is often, though not always, set to the
    // same value as 'family'.
    const char * getEqualityGroup() const noexcept;
    //!cpp:function::
    void setEqualityGroup(const char * equalityGroup);

    //!cpp:function::
    const char * getDescription() const noexcept;
    //!cpp:function::
    void setDescription(const char * description);

    //!cpp:function::
    BitDepth getBitDepth() const noexcept;
    //!cpp:function::
    void setBitDepth(BitDepth bitDepth);

    //!cpp:function:: A display color space will use the display-referred reference space.
    ReferenceSpaceType getReferenceSpaceType() const;

    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // Categories
    // ^^^^^^^^^^
    // A category is used to allow applications to filter the list of color spaces
    // they display in menus based on what that color space is used for.
    //
    // Here is an example config entry that could appear under a ColorSpace:
    // categories: [input, rendering]
    //
    // The example contains two categories: 'input' and 'rendering'.
    // Category strings are not case-sensitive and the order is not significant.
    // There is no limit imposed on length or number. Although users may add
    // their own categories, the strings will typically come from a fixed set
    // listed in the documentation (similar to roles).

    //!cpp:function:: Return true if the category is present.
    bool hasCategory(const char * category) const;
    //!cpp:function:: Add a single category.
    // .. note:: Will do nothing if the category already exists.
    void addCategory(const char * category);
    //!cpp:function:: Remove a category.
    // .. note:: Will do nothing if the category is missing.
    void removeCategory(const char * category);
    //!cpp:function:: Get the number of categories.
    int getNumCategories() const;
    //!cpp:function:: Return the category name using its index
    // .. note:: Will be null if the index is invalid.
    const char * getCategory(int index) const;
    //!cpp:function:: Clear all the categories.
    void clearCategories();

    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // Data
    // ^^^^
    // ColorSpaces that are data are treated a bit special. Basically, any
    // colorspace transforms you try to apply to them are ignored. (Think
    // of applying a gamut mapping transform to an ID pass). Also, the
    // :cpp:class:`DisplayTransform` process obeys special 'data min' and
    // 'data max' args.
    //
    // This is traditionally used for pixel data that represents non-color
    // pixel data, such as normals, point positions, ID information, etc.

    //!cpp:function::
    bool isData() const;
    //!cpp:function::
    void setIsData(bool isData);

    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // Allocation
    // ^^^^^^^^^^
    // If this colorspace needs to be transferred to a limited dynamic
    // range coding space (such as during display with a GPU path), use this
    // allocation to maximize bit efficiency.

    //!cpp:function::
    Allocation getAllocation() const;
    //!cpp:function::
    void setAllocation(Allocation allocation);

    //!rst::
    // Specify the optional variable values to configure the allocation.
    // If no variables are specified, the defaults are used.
    //
    // ALLOCATION_UNIFORM::
    //
    //    2 vars: [min, max]
    //
    // ALLOCATION_LG2::
    //
    //    2 vars: [lg2min, lg2max]
    //    3 vars: [lg2min, lg2max, linear_offset]

    //!cpp:function::
    int getAllocationNumVars() const;
    //!cpp:function::
    void getAllocationVars(float * vars) const;
    //!cpp:function::
    void setAllocationVars(int numvars, const float * vars);

    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // Transform
    // ^^^^^^^^^

    //!cpp:function::
    // If a transform in the specified direction has been specified,
    // return it. Otherwise return a null ConstTransformRcPtr
    ConstTransformRcPtr getTransform(ColorSpaceDirection dir) const;
    //!cpp:function::
    // Specify the transform for the appropriate direction.
    // Setting the transform to null will clear it.
    void setTransform(const ConstTransformRcPtr & transform,
                        ColorSpaceDirection dir);

    //!cpp:function::
    ColorSpace(const ColorSpace &) = delete;
    //!cpp:function::
    ColorSpace& operator= (const ColorSpace &) = delete;
    //!cpp:function::
    ~ColorSpace();

private:
    explicit ColorSpace(ReferenceSpaceType referenceSpace);
    ColorSpace();

    static void deleter(ColorSpace* c);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const ColorSpace&);




///////////////////////////////////////////////////////////////////////////
//!rst:: .. _colorspaceset_section:
//
// ColorSpaceSet
// *************
// The *ColorSpaceSet* is a set of color spaces (i.e. no color space duplication)
// which could be the result of :cpp:func:`Config::getColorSpaces`
// or built from scratch.
//
// .. note::
//    The color spaces are decoupled from the config ones, i.e., any
//    changes to the set itself or to its color spaces do not affect the
//    original color spaces from the configuration.  If needed,
//    use :cpp:func:`Config::addColorSpace` to update the configuration.

//!cpp:class::
class OCIOEXPORT ColorSpaceSet
{
public:
    //!cpp:function:: Create an empty set of color spaces.
    static ColorSpaceSetRcPtr Create();

    //!cpp:function:: Create a set containing a copy of all the color spaces.
    ColorSpaceSetRcPtr createEditableCopy() const;

    //!cpp:function:: Return true if the two sets are equal.
    // .. note:: The comparison is done on the color space names (not a deep comparison).
    bool operator==(const ColorSpaceSet & css) const;
    //!cpp:function:: Return true if the two sets are different.
    bool operator!=(const ColorSpaceSet & css) const;

    //!cpp:function:: Return the number of color spaces.
    int getNumColorSpaces() const;
    //!cpp:function:: Return the color space name using its index.
    // This will be null if an invalid index is specified.
    const char * getColorSpaceNameByIndex(int index) const;
    //!cpp:function:: Return the color space using its index.
    // This will be empty if an invalid index is specified.
    ConstColorSpaceRcPtr getColorSpaceByIndex(int index) const;

    //!rst::
    // .. note::
    //    These fcns only accept color space names (i.e. no role name).

    //!cpp:function:: Will return null if the name is not found.
    ConstColorSpaceRcPtr getColorSpace(const char * name) const;
    //!cpp:function:: Will return -1 if the name is not found.
    int getColorSpaceIndex(const char * name) const;
    //!cpp:function::
    bool hasColorSpace(const char * name) const;

    //!cpp:function:: Add color space(s).
    //
    // .. note::
    //    If another color space is already registered with the same name,
    //    this will overwrite it. This stores a copy of the specified
    //    color space(s).
    void addColorSpace(const ConstColorSpaceRcPtr & cs);
    void addColorSpaces(const ConstColorSpaceSetRcPtr & cs);

    //!cpp:function:: Remove color space(s) using color space names (i.e. no role name).
    //
    // .. note::
    //    The removal of a missing color space does nothing.
    void removeColorSpace(const char * name);
    void removeColorSpaces(const ConstColorSpaceSetRcPtr & cs);

    //!cpp:function:: Clear all color spaces.
    void clearColorSpaces();

    ~ColorSpaceSet();

private:
    ColorSpaceSet();

    ColorSpaceSet(const ColorSpaceSet &);
    ColorSpaceSet & operator= (const ColorSpaceSet &);

    static void deleter(ColorSpaceSet * c);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};


// .. note::
//    All these fcns provide some operations on two color space sets
//    where the result contains copied color spaces and no duplicates.

//!cpp:function:: Perform the union of two sets.
extern OCIOEXPORT ConstColorSpaceSetRcPtr operator||(const ConstColorSpaceSetRcPtr & lcss,
                                                        const ConstColorSpaceSetRcPtr & rcss);
//!cpp:function:: Perform the intersection of two sets.
extern OCIOEXPORT ConstColorSpaceSetRcPtr operator&&(const ConstColorSpaceSetRcPtr & lcss,
                                                        const ConstColorSpaceSetRcPtr & rcss);
//!cpp:function:: Perform the difference of two sets.
extern OCIOEXPORT ConstColorSpaceSetRcPtr operator-(const ConstColorSpaceSetRcPtr & lcss,
                                                    const ConstColorSpaceSetRcPtr & rcss);




///////////////////////////////////////////////////////////////////////////
//!rst:: .. _look_section:
//
// Look
// ****
// The *Look* is an 'artistic' image modification, in a specified image
// state.
// The processSpace defines the ColorSpace the image is required to be
// in, for the math to apply correctly.

//!cpp:class::
class OCIOEXPORT Look
{
public:
    //!cpp:function::
    static LookRcPtr Create();

    //!cpp:function::
    LookRcPtr createEditableCopy() const;

    //!cpp:function::
    const char * getName() const;
    //!cpp:function::
    void setName(const char * name);

    //!cpp:function::
    const char * getProcessSpace() const;
    //!cpp:function::
    void setProcessSpace(const char * processSpace);

    //!cpp:function::
    ConstTransformRcPtr getTransform() const;
    //!cpp:function:: Setting a transform to a non-null call makes it allowed.
    void setTransform(const ConstTransformRcPtr & transform);

    //!cpp:function::
    ConstTransformRcPtr getInverseTransform() const;
    //!cpp:function:: Setting a transform to a non-null call makes it allowed.
    void setInverseTransform(const ConstTransformRcPtr & transform);

    //!cpp:function::
    const char * getDescription() const;
    //!cpp:function::
    void setDescription(const char * description);

    ~Look();

private:
    Look();

    Look(const Look &);
    Look& operator= (const Look &);

    static void deleter(Look* c);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Look&);

///////////////////////////////////////////////////////////////////////////
//!rst:: .. _view_transform_section:
//
// ViewTransform
// *************
// A *ViewTransform* provides a conversion from the main (usually scene-referred) reference space
// to the display-referred reference space.  This allows splitting the conversion from the main
// reference space to a display into two parts: the ViewTransform plus a display color space.
//
// It is also possible to provide a ViewTransform that converts from the display-referred
// reference space back to that space.  This is useful in cases when a ViewTransform is needed
// when converting between displays (such as HDR to SDR).
//
// The ReferenceSpaceType indicates whether the ViewTransform converts from scene-to-display
// reference or display-to-display reference.
//
// The from_reference transform direction is the one that is used when going out towards a display.

//!cpp:class::
class OCIOEXPORT ViewTransform
{
public:
    //!cpp:function::
    static ViewTransformRcPtr Create(ReferenceSpaceType referenceSpace);

    //!cpp:function::
    ViewTransformRcPtr createEditableCopy() const;

    //!cpp:function::
    const char * getName() const noexcept;
    //!cpp:function::
    void setName(const char * name) noexcept;

    //!cpp:function:: See :cpp:func:`ColorSpace::getFamily`.
    const char * getFamily() const noexcept;
    //!cpp:function:: See :cpp:func:`ColorSpace::setFamily`.
    void setFamily(const char * family);

    //!cpp:function::
    const char * getDescription() const noexcept;
    //!cpp:function::
    void setDescription(const char * description);

    //!cpp:function:: See :cpp:func:`ColorSpace::hasCategory`.
    bool hasCategory(const char * category) const;
    //!cpp:function:: See :cpp:func:`ColorSpace::addCategory`.
    void addCategory(const char * category);
    //!cpp:function:: See :cpp:func:`ColorSpace::removeCategory`.
    void removeCategory(const char * category);
    //!cpp:function:: See :cpp:func:`ColorSpace::getNumCategories`.
    int getNumCategories() const;
    //!cpp:function:: See :cpp:func:`ColorSpace::getCategory`.
    const char * getCategory(int index) const;
    //!cpp:function:: See :cpp:func:`ColorSpace::clearCategories`.
    void clearCategories();

    //!cpp:function::
    ReferenceSpaceType getReferenceSpaceType() const noexcept;

    //!cpp:function:: If a transform in the specified direction has been specified, return it.
    // Otherwise return a null ConstTransformRcPtr
    ConstTransformRcPtr getTransform(ViewTransformDirection dir) const;

    //!cpp:function:: Specify the transform for the appropriate direction. Setting the transform
    // to null will clear it.
    void setTransform(const ConstTransformRcPtr & transform, ViewTransformDirection dir);

    //!cpp:function::
    ViewTransform(const ViewTransform &) = delete;
    //!cpp:function::
    ViewTransform & operator= (const ViewTransform &) = delete;
    //!cpp:function::
    ~ViewTransform();
    
private:
    ViewTransform();
    explicit ViewTransform(ReferenceSpaceType referenceSpace);

    static void deleter(ViewTransform * c);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const ViewTransform&);

///////////////////////////////////////////////////////////////////////////
//!rst::
// Processor
// *********
// The *Processor* represents a specific color transformation which is
// the result of :cpp:func:`Config::getProcessor`.

//!cpp:class::
class OCIOEXPORT Processor
{
public:
    //!cpp:function::
    bool isNoOp() const;

    //!cpp:function:: True if the image transformation is non-separable.
    // For example, if a change in red may also cause a change in green or blue.
    bool hasChannelCrosstalk() const;

    //!cpp:function:: 
    const char * getCacheID() const;

    //!cpp:function:: The ProcessorMetadata contains technical information
    //                such as the number of files and looks used in the processor.
    ConstProcessorMetadataRcPtr getProcessorMetadata() const;

    //!cpp:function:: Get a FormatMetadata containing the top level metadata
    //                for the processor.  For a processor from a CLF file,
    //                this corresponds to the ProcessList metadata.
    const FormatMetadata & getFormatMetadata() const;

    //!cpp:function:: Get the number of transforms that comprise the processor.
    //                Each transform has a (potentially empty) FormatMetadata.
    int getNumTransforms() const;
    //!cpp:function:: Get a FormatMetadata containing the metadata for a
    //                transform within the processor. For a processor from
    //                a CLF file, this corresponds to the metadata associated
    //                with an individual process node.
    const FormatMetadata & getTransformFormatMetadata(int index) const;

    //!cpp:function:: Return a cpp:class:`GroupTransform` that contains a
    //                copy of the transforms that comprise the processor.
    //                (Changes to it will not modify the original processor.)
    GroupTransformRcPtr createGroupTransform() const;

    //!cpp:function:: Write the transforms comprising the processor to the stream.
    //                Writing (as opposed to Baking) is a lossless process.
    //                An exception is thrown if the processor cannot be
    //                losslessly written to the specified file format.
    void write(const char * formatName, std::ostream & os) const;

    //!cpp:function:: Get the number of writers.
    static int getNumWriteFormats();

    //!cpp:function:: Get the writer at index, return empty string if
    //                an invalid index is specified.
    static const char * getFormatNameByIndex(int index);
    static const char * getFormatExtensionByIndex(int index);

    // TODO: Revisit the dynamic property access.
    //!cpp:function::
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;
    bool hasDynamicProperty(DynamicPropertyType type) const;

    //!cpp:function:: Run the optimizer on a Processor to create a new :cpp:class:`Processor`.
    // It is usually not necessary to call this since getting a CPUProcessor or GPUProcessor
    // will also optimize.  However if you need both, calling this method first makes getting
    // a CPU and GPU Processor faster since the optimization is effectively only done once.
    ConstProcessorRcPtr getOptimizedProcessor(OptimizationFlags oFlags) const;

    //!cpp:function:: Create a :cpp:class:`Processor` that is optimized for a specific in and out
    // bit-depth (as CPUProcessor would do).  This method is provided primarily for diagnostic
    // purposes.
    ConstProcessorRcPtr getOptimizedProcessor(BitDepth inBD, BitDepth outBD,
                                              OptimizationFlags oFlags) const;

    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // GPU Renderer
    // ^^^^^^^^^^^^
    // Get an optimized :cpp:class:`GPUProcessor` instance.

    //!cpp:function::
    ConstGPUProcessorRcPtr getDefaultGPUProcessor() const;
    //!cpp:function::
    ConstGPUProcessorRcPtr getOptimizedGPUProcessor(OptimizationFlags oFlags) const;

    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // CPU Renderer
    // ^^^^^^^^^^^^
    // Get an optimized :cpp:class:`CPUProcessor` instance.
    //
    // .. note::
    //    This may provide higher fidelity than anticipated due to internal
    //    optimizations. For example, if the inputColorSpace and the
    //    outputColorSpace are members of the same family, no conversion
    //    will be applied, even though strictly speaking quantization
    //    should be added.


    // .. note::
    //    The typical use case to apply color processing to an image is:
    //
    // .. code-block:: cpp
    //
    //     OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    //
    //     OCIO::ConstProcessorRcPtr processor
    //         = config->getProcessor(colorSpace1, colorSpace2);
    //
    //     OCIO::ConstCPUProcessorRcPtr cpuProcessor
    //         = processor->getDefaultCPUProcessor();
    //
    //     OCIO::PackedImageDesc img(imgDataPtr, imgWidth, imgHeight, imgChannels);
    //     cpuProcessor->apply(img);
    //

    //!cpp:function::
    ConstCPUProcessorRcPtr getDefaultCPUProcessor() const;
    //!cpp:function::
    ConstCPUProcessorRcPtr getOptimizedCPUProcessor(OptimizationFlags oFlags) const;
    //!cpp:function::
    ConstCPUProcessorRcPtr getOptimizedCPUProcessor(BitDepth inBitDepth,
                                                    BitDepth outBitDepth,
                                                    OptimizationFlags oFlags) const;

    ~Processor();

private:
    Processor();

    Processor(const Processor &);
    Processor& operator= (const Processor &);

    static ProcessorRcPtr Create();

    static void deleter(Processor* c);

    friend class Config;

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};


///////////////////////////////////////////////////////////////////////////
//!rst::
// CPUProcessor
// ************

//!cpp:class::
class OCIOEXPORT CPUProcessor
{
public:
    //!cpp:function:: The in and out bit-depths must be equal for isNoOp to be true.
    bool isNoOp() const;

    //!cpp:function:: Equivalent to isNoOp from the underlying Processor, i.e., it ignores 
    // in/out bit-depth differences.
    bool isIdentity() const;

    //!cpp:function::
    bool hasChannelCrosstalk() const;

    //!cpp:function::
    const char * getCacheID() const;

    //!cpp:function:: Bit-depth of the input pixel buffer.
    BitDepth getInputBitDepth() const;
    //!cpp:function:: Bit-depth of the output pixel buffer.
    BitDepth getOutputBitDepth() const;

    //!cpp:function:: Refer to :cpp:func:`GPUProcessor::getDynamicProperty`.
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;

    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // Apply to an image with any kind of channel ordering while respecting
    // the input and output bit-depths.

    //!cpp:function::
    void apply(ImageDesc & imgDesc) const;
    //!cpp:function::
    void apply(const ImageDesc & srcImgDesc, ImageDesc & dstImgDesc) const;

    //!rst::
    // Apply to a single pixel respecting that the input and output bit-depths
    // be 32-bit float and the image buffer be packed RGB/RGBA.
    //
    // .. note::
    //    This is not as efficient as applying to an entire image at once.
    //    If you are processing multiple pixels, and have the flexibility,
    //    use the above function instead.

    //!cpp:function::
    void applyRGB(float * pixel) const;
    //!cpp:function::
    void applyRGBA(float * pixel) const;

    ~CPUProcessor();

private:
    CPUProcessor();

    CPUProcessor(const CPUProcessor &);
    CPUProcessor& operator= (const CPUProcessor &);

    static void deleter(CPUProcessor * c);

    friend class Processor;

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};


///////////////////////////////////////////////////////////////////////////
//!rst::
// GPUProcessor
// ************

//!cpp:class::
class OCIOEXPORT GPUProcessor
{
public:
    //!cpp:function::
    bool isNoOp() const;

    //!cpp:function::
    bool hasChannelCrosstalk() const;

    //!cpp:function::
    const char * getCacheID() const;

    //!cpp:function:: The returned pointer may be used to set the value of any dynamic properties
    // of the requested type.  Throws if the requested property is not found.  Note that if the
    // processor contains several ops that support the requested property, only ones for which
    // dynamic has been enabled will be controlled.
    //
    // .. note::
    //    The dynamic properties in this object are decoupled from the ones
    //    in the :cpp:class:`Processor` it was generated from.
    //
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;

    //!cpp:function:: Extract & Store the shader information to implement the color processing.
    void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;

    //!cpp:function:: Extract the shader information using a custom :cpp:class:`GpuShaderCreator` class.
    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const;
    
    ~GPUProcessor();

private:
    GPUProcessor();

    GPUProcessor(const GPUProcessor &);
    GPUProcessor& operator= (const GPUProcessor &);

    static void deleter(GPUProcessor * c);

    friend class Processor;

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};


//!cpp:class::
// This class contains meta information about the process that generated
// this processor.  The results of these functions do not
// impact the pixel processing.

class OCIOEXPORT ProcessorMetadata
{
public:
    //!cpp:function::
    static ProcessorMetadataRcPtr Create();

    //!cpp:function::
    int getNumFiles() const;
    //!cpp:function::
    const char * getFile(int index) const;

    //!cpp:function::
    int getNumLooks() const;
    //!cpp:function::
    const char * getLook(int index) const;

    //!cpp:function::
    void addFile(const char * fname);
    //!cpp:function::
    void addLook(const char * look);

    ~ProcessorMetadata();

private:
    ProcessorMetadata();
    ProcessorMetadata(const ProcessorMetadata &);
    ProcessorMetadata& operator= (const ProcessorMetadata &);

    static void deleter(ProcessorMetadata* c);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};



///////////////////////////////////////////////////////////////////////////
//!rst::
// Baker
// *****
//
// In certain situations it is necessary to serialize transforms into a variety
// of application specific LUT formats. Note that not all file formats that may
// be read also support baking.
//
// **Usage Example:** *Bake a CSP sRGB viewer LUT*
//
// .. code-block:: cpp
//
//    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromEnv();
//    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
//    baker->setConfig(config);
//    baker->setFormat("csp");
//    baker->setInputSpace("lnf");
//    baker->setShaperSpace("log");
//    baker->setTargetSpace("sRGB");
//    auto & metadata = baker->getFormatMetadata();
//    metadata.addChildElement(OCIO::METADATA_DESCRIPTION, "A first comment");
//    metadata.addChildElement(OCIO::METADATA_DESCRIPTION, "A second comment");
//    std::ostringstream out;
//    baker->bake(out); // fresh bread anyone!
//    std::cout << out.str();

class OCIOEXPORT Baker
{
public:
    //!cpp:function:: Create a new Baker.
    static BakerRcPtr Create();

    //!cpp:function:: Create a copy of this Baker.
    BakerRcPtr createEditableCopy() const;

    //!cpp:function::
    ConstConfigRcPtr getConfig() const;
    //!cpp:function:: Set the config to use.
    void setConfig(const ConstConfigRcPtr & config);

    //!cpp:function::
    const char * getFormat() const;
    //!cpp:function:: Set the LUT output format.
    void setFormat(const char * formatName);


    //!cpp:function::
    const FormatMetadata & getFormatMetadata() const;
    //!cpp:function:: Get editable *optional* format metadata. The metadata that will be used
    // varies based on the capability of the given file format.  Formats such as CSP,
    // IridasCube, and ResolveCube will create comments in the file header using the value of
    // any first-level children elements of the formatMetadata.  The CLF/CTF formats will make
    // use of the top-level "id" and "name" attributes and children elements "Description",
    // "InputDescriptor", "OutputDescriptor", and "Info".
    FormatMetadata & getFormatMetadata();

    //!cpp:function::
    const char * getInputSpace() const;
    //!cpp:function:: Set the input ColorSpace that the LUT will be applied to.
    void setInputSpace(const char * inputSpace);

    //!cpp:function::
    const char * getShaperSpace() const;
    //!cpp:function:: Set an *optional* ColorSpace to be used to shape / transfer the input
    // colorspace. This is mostly used to allocate an HDR luminance range into an LDR one.
    // If a shaper space is not explicitly specified, and the file format supports one, the
    // ColorSpace Allocation will be used (not implemented for all formats).
    void setShaperSpace(const char * shaperSpace);

    //!cpp:function::
    const char * getLooks() const;
    //!cpp:function:: Set the looks to be applied during baking. Looks is a potentially comma
    // (or colon) delimited list of lookNames, where +/- prefixes are optionally allowed to
    // denote forward/inverse look specification. (And forward is assumed in the absence of
    // either).
    void setLooks(const char * looks);

    //!cpp:function::
    const char * getTargetSpace() const;
    //!cpp:function:: Set the target device colorspace for the LUT.
    void setTargetSpace(const char * targetSpace);

    //!cpp:function::
    int getShaperSize() const;
    //!cpp:function:: Override the default shaper LUT size. Default value is -1, which allows
    // each format to use its own most appropriate size. For the CLF format, the default uses
    // a half-domain LUT1D (which is ideal for scene-linear inputs).
    void setShaperSize(int shapersize);

    //!cpp:function::
    int getCubeSize() const;
    //!cpp:function:: Override the default cube sample size.
    // default: <format specific>
    void setCubeSize(int cubesize);

    //!cpp:function:: Bake the LUT into the output stream.
    void bake(std::ostream & os) const;

    //!cpp:function:: Get the number of LUT bakers.
    static int getNumFormats();

    //!cpp:function:: Get the LUT baker format name at index, return empty string if an invalid
    // index is specified.
    static const char * getFormatNameByIndex(int index);
    //!cpp:function:: Get the LUT baker format extension at index, return empty string if an
    // invalid index is specified.
    static const char * getFormatExtensionByIndex(int index);

    ~Baker();

private:
    Baker();

    Baker(const Baker &);
    Baker& operator= (const Baker &);

    static void deleter(Baker* o);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};


///////////////////////////////////////////////////////////////////////////
//!rst::
// ImageDesc
// *********

//!rst::
// .. c:var:: const ptrdiff_t AutoStride
//
//    AutoStride
const ptrdiff_t AutoStride = std::numeric_limits<ptrdiff_t>::min();

//!cpp:class::
// This is a light-weight wrapper around an image, that provides a context
// for pixel access. This does NOT claim ownership of the pixels or copy
// image data.

class OCIOEXPORT ImageDesc
{
public:
    //!cpp:function::
    ImageDesc();
    //!cpp:function::
    virtual ~ImageDesc();

    //!cpp:function:: Get a pointer to the red channel of the first pixel.
    virtual void * getRData() const = 0;
    //!cpp:function:: Get a pointer to the green channel of the first pixel.
    virtual void * getGData() const = 0;
    //!cpp:function:: Get a pointer to the blue channel of the first pixel.
    virtual void * getBData() const = 0;
    //!cpp:function:: Get a pointer to the alpha channel of the first pixel
    // or null as alpha channel is optional.
    virtual void * getAData() const = 0;

    //!cpp:function:: Get the bit-depth.
    virtual BitDepth getBitDepth() const = 0;

    //!cpp:function:: Get the width to process (where x position starts at 0 and ends at width-1).
    virtual long getWidth() const = 0;
    //!cpp:function:: Get the height to process (where y position starts at 0 and ends at height-1).
    virtual long getHeight() const = 0;

    //!cpp:function:: Get the step in bytes to find the same color channel of the next pixel.
    virtual ptrdiff_t getXStrideBytes() const = 0;
    //!cpp:function:: Get the step in bytes to find the same color channel
    // of the pixel at the same position in the next line.
    virtual ptrdiff_t getYStrideBytes() const = 0;

    //!cpp:function:: Is the image buffer in packed mode with the 4 color channels?
    // ("Packed" here means that XStrideBytes is 4x the bytes per channel, so it is more specific
    // than simply any PackedImageDesc.)
    virtual bool isRGBAPacked() const = 0;
    //!cpp:function:: Is the image buffer 32-bit float?
    virtual bool isFloat() const = 0;

private:
    ImageDesc(const ImageDesc &);
    ImageDesc & operator= (const ImageDesc &);
};

extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const ImageDesc&);


///////////////////////////////////////////////////////////////////////////
//!rst::
// PackedImageDesc
// ^^^^^^^^^^^^^^^

//!cpp:class::
class OCIOEXPORT PackedImageDesc : public ImageDesc
{
public:

    //!rst::
    // All the constructors expect a pointer to packed image data (such as
    // rgbrgbrgb or rgbargbargba) starting at the first color channel of
    // the first pixel to process (which does not need to be the first pixel
    // of the image). The number of channels must be greater than or equal to 3.
    // If a 4th channel is specified, it is assumed to be alpha
    // information.  Channels > 4 will be ignored.
    //
    // .. note::
    //    The methods assume the CPUProcessor bit-depth type for the data pointer.

    //!cpp:function::
    //
    // .. note::
    //    numChannels must be 3 (RGB) or 4 (RGBA).
    PackedImageDesc(void * data,
                    long width, long height,
                    long numChannels);

    //!cpp:function::
    //
    // .. note::
    //    numChannels must be 3 (RGB) or 4 (RGBA).
    PackedImageDesc(void * data,
                    long width, long height,
                    long numChannels,
                    BitDepth bitDepth,
                    ptrdiff_t chanStrideBytes,
                    ptrdiff_t xStrideBytes,
                    ptrdiff_t yStrideBytes);

    //!cpp:function::
    PackedImageDesc(void * data,
                    long width, long height,
                    ChannelOrdering chanOrder);

    //!cpp:function::
    PackedImageDesc(void * data,
                    long width, long height,
                    ChannelOrdering chanOrder,
                    BitDepth bitDepth,
                    ptrdiff_t chanStrideBytes,
                    ptrdiff_t xStrideBytes,
                    ptrdiff_t yStrideBytes);

    //!cpp:function::
    virtual ~PackedImageDesc();

    //!cpp:function:: Get the channel ordering of all the pixels.
    ChannelOrdering getChannelOrder() const;

    //!cpp:function:: Get the bit-depth.
    BitDepth getBitDepth() const override;

    //!cpp:function:: Get a pointer to the first color channel of the first pixel.
    void * getData() const;

    //!cpp:function::
    void * getRData() const override;
    //!cpp:function::
    void * getGData() const override;
    //!cpp:function::
    void * getBData() const override;
    //!cpp:function::
    void * getAData() const override;

    //!cpp:function::
    long getWidth() const override;
    //!cpp:function::
    long getHeight() const override;
    //!cpp:function::
    long getNumChannels() const;

    //!cpp:function::
    ptrdiff_t getChanStrideBytes() const;
    //!cpp:function::
    ptrdiff_t getXStrideBytes() const override;
    //!cpp:function::
    ptrdiff_t getYStrideBytes() const override;

    //!cpp:function::
    bool isRGBAPacked() const override;
    //!cpp:function::
    bool isFloat() const override;

private:
    struct Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }

    PackedImageDesc();
    PackedImageDesc(const PackedImageDesc &);
    PackedImageDesc& operator= (const PackedImageDesc &);
};


///////////////////////////////////////////////////////////////////////////
//!rst::
// PlanarImageDesc
// ^^^^^^^^^^^^^^^

//!cpp:class::
class OCIOEXPORT PlanarImageDesc : public ImageDesc
{
public:

    //!rst::
    // All the constructors expect pointers to the specified image planes
    // (i.e. rrrr gggg bbbb) starting at the first color channel of the
    // first pixel to process (which need not be the first pixel of the image).
    // Pass NULL for aData if no alpha exists (r/g/bData must not be NULL).
    //
    // .. note::
    //    The methods assume the CPUProcessor bit-depth type for the R/G/B/A data pointers.

    //!cpp:function::
    PlanarImageDesc(void * rData, void * gData, void * bData, void * aData,
                    long width, long height);

    //!cpp:function::
    // Note that although PlanarImageDesc is powerful enough to also describe
    // all :cpp:class:`PackedImageDesc` scenarios, it is recommended to use
    // a PackedImageDesc where possible since that allows for additional
    // optimizations.
    PlanarImageDesc(void * rData, void * gData, void * bData, void * aData,
                    long width, long height,
                    BitDepth bitDepth,
                    ptrdiff_t xStrideBytes,
                    ptrdiff_t yStrideBytes);

    //!cpp:function::
    virtual ~PlanarImageDesc();

    //!cpp:function::
    void * getRData() const override;
    //!cpp:function::
    void * getGData() const override;
    //!cpp:function::
    void * getBData() const override;
    //!cpp:function::
    void * getAData() const override;

    //!cpp:function:: Get the bit-depth.
    BitDepth getBitDepth() const override;

    //!cpp:function::
    long getWidth() const override;
    //!cpp:function::
    long getHeight() const override;

    //!cpp:function::
    ptrdiff_t getXStrideBytes() const override;
    //!cpp:function::
    ptrdiff_t getYStrideBytes() const override;

    //!cpp:function::
    bool isRGBAPacked() const override;
    //!cpp:function::
    bool isFloat() const override;

private:
    struct Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }

    PlanarImageDesc();
    PlanarImageDesc(const PlanarImageDesc &);
    PlanarImageDesc& operator= (const PlanarImageDesc &);
};


///////////////////////////////////////////////////////////////////////////
//!rst::
// GpuShaderCreator
// ****************
// Inherit from the class to fully customize the implementation of a GPU shader program
// from a color transformation.
//
// When no customizations are needed then the :cpp:class:`GpuShaderDesc` is a better choice.
//
//!cpp:class::
class OCIOEXPORT GpuShaderCreator
{
public:

    //!cpp:function::
    virtual GpuShaderCreatorRcPtr clone() const = 0;

    //!cpp:function::
    const char * getUniqueID() const noexcept;
    //!cpp:function::
    void setUniqueID(const char * uid) noexcept;

    //!cpp:function::
    GpuLanguage getLanguage() const noexcept;
    //!cpp:function:: Set the shader program language.
    void setLanguage(GpuLanguage lang) noexcept;

    //!cpp:function::
    const char * getFunctionName() const noexcept;
    //!cpp:function:: Set the function name of the shader program.
    void setFunctionName(const char * name) noexcept;

    //!cpp:function::
    const char * getPixelName() const noexcept;
    //!cpp:function:: Set the pixel name variable holding the color values.
    void setPixelName(const char * name) noexcept;

    //!cpp:function::
    //
    // .. note::
    //   Some applications require that textures, uniforms,
    //   and helper methods be uniquely named because several
    //   processor instances could coexist.
    //
    const char * getResourcePrefix() const noexcept;
    //!cpp:function::  Set a prefix to the resource name
    void setResourcePrefix(const char * prefix) noexcept;

    //!cpp:function::
    virtual const char * getCacheID() const noexcept;

    //!cpp:function:: Start to collect the shader data.
    virtual void begin(const char * uid);
    //!cpp:function:: End to collect the shader data.
    virtual void end();

    //!cpp:function:: Some graphic cards could have 1D & 2D textures with size limitations.
    virtual void setTextureMaxWidth(unsigned maxWidth) = 0;
    //!cpp:function::
    virtual unsigned getTextureMaxWidth() const noexcept = 0;

    //!cpp:function:: To avoid texture/unform name clashes always append
    // an increasing number to the resource name.
    unsigned getNextResourceIndex() noexcept;

    //!cpp:function::
    virtual bool addUniform(const char * name,
                            const DynamicPropertyRcPtr & value) = 0;

    enum TextureType
    {
        TEXTURE_RED_CHANNEL, // Only use the red channel of the texture
        TEXTURE_RGB_CHANNEL
    };

    //!cpp:function::
    virtual void addTexture(const char * textureName,
                            const char * samplerName,
                            const char * uid,
                            unsigned width, unsigned height,
                            TextureType channel,
                            Interpolation interpolation,
                            const float * values) = 0;

    //!cpp:function::
    virtual void add3DTexture(const char * textureName,
                              const char * samplerName,
                              const char * uid,
                              unsigned edgelen,
                              Interpolation interpolation,
                              const float * values) = 0;

    //!rst:: Methods to specialize parts of a OCIO shader program.
    //
    // **An OCIO shader program could contain:**
    //
    // 1. A declaration part  e.g., uniform sampled3D tex3;
    //
    // 2. Some helper methods
    //
    // 3. The OCIO shader function may be broken down as:
    //
    //    1. The function header  e.g., void OCIODisplay(in vec4 inColor) {
    //    2. The function body    e.g.,   vec4 outColor.rgb = texture3D(tex3, inColor.rgb).rgb;
    //    3. The function footer  e.g.,   return outColor; }
    //
    //
    // **Usage Example:**
    //
    // Below is a code snippet to highlight the different parts of the OCIO shader program.
    //
    // .. code-block:: cpp
    //
    //    // All global declarations
    //    uniform sampled3D tex3;
    //
    //    // All helper methods
    //    vec3 computePosition(vec3 color)
    //    {
    //       vec3 coords = color;
    //       // Some processing...
    //       return coords;
    //    }
    //
    //    // The shader function
    //    vec4 OCIODisplay(in vec4 inColor)     //
    //    {                                     // Function Header
    //       vec4 outColor = inColor;           //
    //
    //       outColor.rgb = texture3D(tex3, computePosition(inColor.rgb)).rgb;
    //
    //       return outColor;                   // Function Footer
    //    }                                     //
    //
    //!cpp:function::
    virtual void addToDeclareShaderCode(const char * shaderCode);
    //!cpp:function::
    virtual void addToHelperShaderCode(const char * shaderCode);
    //!cpp:function::
    virtual void addToFunctionHeaderShaderCode(const char * shaderCode);
    //!cpp:function::
    virtual void addToFunctionShaderCode(const char * shaderCode);
    //!cpp:function::
    virtual void addToFunctionFooterShaderCode(const char * shaderCode);

    //!cpp:function:: Create the OCIO shader program
    //
    // .. note::
    //
    //   The OCIO shader program is decomposed to allow a specific implementation
    //   to change some parts. Some product integrations add the color processing
    //   within a client shader program, imposing constraints requiring this flexibility.
    //
    virtual void createShaderText(const char * shaderDeclarations,
                                  const char * shaderHelperMethods,
                                  const char * shaderFunctionHeader,
                                  const char * shaderFunctionBody,
                                  const char * shaderFunctionFooter);

    //!cpp:function::
    virtual void finalize();
    
    //!cpp:function::
    virtual ~GpuShaderCreator();

protected:
    //!cpp:function::
    GpuShaderCreator();
    //!cpp:function::
    GpuShaderCreator(const GpuShaderCreator &) = delete;
    //!cpp:function::
    GpuShaderCreator & operator= (const GpuShaderCreator &) = delete;

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

///////////////////////////////////////////////////////////////////////////
//!rst::
// GpuShaderDesc
// *************
// This class holds the GPU-related information needed to build a shader program
// from a specific processor.
//
// This class defines the interface and there are two implementations provided.
// The "legacy" mode implements the OCIO v1 approach of baking certain ops
// in order to have at most one 3D-LUT.  The "generic" mode is the v2 default and
// allows all the ops to be processed as-is, without baking, like the CPU renderer.
// Custom implementations could be written to accommodate the GPU needs of a
// specific client app.
//
//
// The complete fragment shader program is decomposed in two main parts:
// the OCIO shader program for the color processing and the client shader
// program which consumes the pixel color processing.
//
// The OCIO shader program is fully described by the GpuShaderDesc
// independently from the client shader program. The only critical
// point is the agreement on the OCIO function shader name.
//
// To summarize, the complete shader program is:
//
// .. code-block:: cpp
//
//  ////////////////////////////////////////////////////////////////////////
//  //                                                                    //
//  //               The complete fragment shader program                 //
//  //                                                                    //
//  ////////////////////////////////////////////////////////////////////////
//  //                                                                    //
//  //   //////////////////////////////////////////////////////////////   //
//  //   //                                                          //   //
//  //   //               The OCIO shader program                    //   //
//  //   //                                                          //   //
//  //   //////////////////////////////////////////////////////////////   //
//  //   //                                                          //   //
//  //   //   // All global declarations                             //   //
//  //   //   uniform sampled3D tex3;                                //   //
//  //   //                                                          //   //
//  //   //   // All helper methods                                  //   //
//  //   //   vec3 computePos(vec3 color)                            //   //
//  //   //   {                                                      //   //
//  //   //      vec3 coords = color;                                //   //
//  //   //      ...                                                 //   //
//  //   //      return coords;                                      //   //
//  //   //   }                                                      //   //
//  //   //                                                          //   //
//  //   //   // The OCIO shader function                            //   //
//  //   //   vec4 OCIODisplay(in vec4 inColor)                      //   //
//  //   //   {                                                      //   //
//  //   //      vec4 outColor = inColor;                            //   //
//  //   //      ...                                                 //   //
//  //   //      outColor.rbg                                        //   //
//  //   //         = texture3D(tex3, computePos(inColor.rgb)).rgb;  //   //
//  //   //      ...                                                 //   //
//  //   //      return outColor;                                    //   //
//  //   //   }                                                      //   //
//  //   //                                                          //   //
//  //   //////////////////////////////////////////////////////////////   //
//  //                                                                    //
//  //   //////////////////////////////////////////////////////////////   //
//  //   //                                                          //   //
//  //   //             The client shader program                    //   //
//  //   //                                                          //   //
//  //   //////////////////////////////////////////////////////////////   //
//  //   //                                                          //   //
//  //   //   uniform sampler2D image;                               //   //
//  //   //                                                          //   //
//  //   //   void main()                                            //   //
//  //   //   {                                                      //   //
//  //   //      vec4 inColor = texture2D(image, gl_TexCoord[0].st); //   //
//  //   //      ...                                                 //   //
//  //   //      vec4 outColor = OCIODisplay(inColor);               //   //
//  //   //      ...                                                 //   //
//  //   //      gl_FragColor = outColor;                            //   //
//  //   //   }                                                      //   //
//  //   //                                                          //   //
//  //   //////////////////////////////////////////////////////////////   //
//  //                                                                    //
//  ////////////////////////////////////////////////////////////////////////
//
//
// **Usage Example:** *Building a GPU shader*
//
//   This example is based on the code in: src/apps/ociodisplay/main.cpp
//
// .. code-block:: cpp
//
//    // Get the processor
//    //
//    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromEnv();
//    OCIO::ConstProcessorRcPtr processor
//       = config->getProcessor("ACES - ACEScg", "Output - sRGB");
//
//    // Step 1: Create a GPU shader description
//    //
//    // The three potential scenarios are:
//    //
//    //   1. Instantiate the legacy shader description.  The color processor
//    //      is baked down to contain at most one 3D LUT and no 1D LUTs.
//    //
//    //      This is the v1 behavior and will remain part of OCIO v2
//    //      for backward compatibility.
//    //
//    OCIO::GpuShaderDescRcPtr shaderDesc
//          = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);
//    //
//    //   2. Instantiate the generic shader description.  The color processor
//    //      is used as-is (i.e. without any baking step) and could contain
//    //      any number of 1D & 3D luts.
//    //
//    //      This is the default OCIO v2 behavior and allows a much better
//    //      match between the CPU and GPU renderers.
//    //
//    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::Create();
//    //
//    //   3. Instantiate a custom shader description.
//    //
//    //      Writing a custom shader description is a way to tailor the shaders
//    //      to the needs of a given client program.  This involves writing a
//    //      new class inheriting from the pure virtual class GpuShaderDesc.
//    //
//    //      Please refer to the GenericGpuShaderDesc class for an example.
//    //
//    OCIO::GpuShaderDescRcPtr shaderDesc = MyCustomGpuShader::Create();
//
//    shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3);
//    shaderDesc->setFunctionName("OCIODisplay");
//
//    // Step 2: Collect the shader program information for a specific processor
//    //
//    processor->extractGpuShaderInfo(shaderDesc);
//
//    // Step 3: Create a helper to build the shader. Here we use a helper for
//    //         OpenGL but there will also be helpers for other languages.
//    //
//    OpenGLBuilderRcPtr oglBuilder = OpenGLBuilder::Create(shaderDesc);
//
//    // Step 4: Allocate & upload all the LUTs
//    //
//    oglBuilder->allocateAllTextures();
//
//    // Step 5: Build the complete fragment shader program using
//    //         g_fragShaderText which is the client shader program.
//    //
//    g_programId = oglBuilder->buildProgram(g_fragShaderText);
//
//    // Step 6: Enable the fragment shader program, and all needed textures
//    //
//    glUseProgram(g_programId);
//    glUniform1i(glGetUniformLocation(g_programId, "tex1"), 1);  // image texture
//    oglBuilder->useAllTextures(g_programId);                    // LUT textures
//

//!cpp:class::
class OCIOEXPORT GpuShaderDesc : public GpuShaderCreator
{
public:

    //!cpp:function:: Create the legacy shader description.
    static GpuShaderDescRcPtr CreateLegacyShaderDesc(unsigned edgelen);

    //!cpp:function:: Create the default shader description.
    static GpuShaderDescRcPtr CreateShaderDesc();

    //!cpp:function::
    GpuShaderCreatorRcPtr clone() const override;

    //!cpp:function:: Dynamic Property related methods.
    virtual unsigned getNumUniforms() const noexcept = 0;
    virtual void getUniform(unsigned index, const char *& name,
                            DynamicPropertyRcPtr & value) const = 0;

    //!cpp:function:: 1D lut related methods
    virtual unsigned getNumTextures() const noexcept = 0;
    virtual void getTexture(unsigned index,
                            const char *& textureName,
                            const char *& samplerName,
                            const char *& uid,
                            unsigned & width,
                            unsigned & height,
                            TextureType & channel,
                            Interpolation & interpolation) const = 0;
    virtual void getTextureValues(unsigned index, const float *& values) const = 0;

    //!cpp:function:: 3D lut related methods
    virtual unsigned getNum3DTextures() const noexcept = 0;
    virtual void get3DTexture(unsigned index,
                              const char *& textureName,
                              const char *& samplerName,
                              const char *& uid,
                              unsigned & edgelen,
                              Interpolation & interpolation) const = 0;
    virtual void get3DTextureValues(unsigned index, const float *& values) const = 0;

    //!cpp:function:: Get the complete OCIO shader program.
    const char * getShaderText() const noexcept;

    //!cpp:function::
    virtual ~GpuShaderDesc();

protected:
    //!cpp:function::
    GpuShaderDesc();
    //!cpp:function::
    GpuShaderDesc(const GpuShaderDesc &) = delete;
    //!cpp:function::
    GpuShaderDesc& operator= (const GpuShaderDesc &) = delete;
};


///////////////////////////////////////////////////////////////////////////
//!rst::
// Context
// *******

//!cpp:class::
class OCIOEXPORT Context
{
public:
    //!cpp:function::
    static ContextRcPtr Create();

    //!cpp:function::
    ContextRcPtr createEditableCopy() const;

    //!cpp:function::
    const char * getCacheID() const;

    //!cpp:function::
    void setSearchPath(const char * path);
    //!cpp:function::
    const char * getSearchPath() const;

    //!cpp:function::
    int getNumSearchPaths() const;
    //!cpp:function::
    const char * getSearchPath(int index) const;
    //!cpp:function::
    void clearSearchPaths();
    //!cpp:function::
    void addSearchPath(const char * path);

    //!cpp:function::
    void setWorkingDir(const char * dirname);
    //!cpp:function::
    const char * getWorkingDir() const;

    //!cpp:function::
    void setStringVar(const char * name, const char * value);
    //!cpp:function::
    const char * getStringVar(const char * name) const;

    //!cpp:function::
    int getNumStringVars() const;
    //!cpp:function::
    const char * getStringVarNameByIndex(int index) const;

    //!cpp:function::
    void clearStringVars();

    //!cpp:function::
    void setEnvironmentMode(EnvironmentMode mode);

    //!cpp:function::
    EnvironmentMode getEnvironmentMode() const;

    //!cpp:function:: Seed all string vars with the current environment.
    void loadEnvironment();

    //! Do a string lookup.
    //!cpp:function:: Do a file lookup.
    //
    // Evaluate the specified variable (as needed). Will not throw exceptions.
    const char * resolveStringVar(const char * val) const;

    //! Do a file lookup.
    //!cpp:function:: Do a file lookup.
    //
    // Evaluate all variables (as needed).
    // Also, walk the full search path until the file is found.
    // If the filename cannot be found, an exception will be thrown.
    const char * resolveFileLocation(const char * filename) const;

    ~Context();
    
private:
    Context();

    Context(const Context &);
    Context& operator= (const Context &);

    static void deleter(Context* c);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Context&);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_OPENCOLORIO_H
