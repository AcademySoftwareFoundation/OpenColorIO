..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Config
******

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

A config defines all the color spaces to be available at runtime.

The color configuration (:cpp:class:`Config`) is the main object for
interacting with this library. It encapsulates all of the information
necessary to use customized :cpp:class:`ColorSpaceTransform` and
:cpp:class:`DisplayViewTransform` operations.

See the :ref:`user-guide` for more information on
selecting, creating, and working with custom color configurations.

For applications interested in using only one color config at
a time (this is the vast majority of apps), their API would
traditionally get the global configuration and use that, as opposed to
creating a new one. This simplifies the use case for
plugins and bindings, as it alleviates the need to pass around configuration
handles.

An example of an application where this would not be sufficient would be
a multi-threaded image proxy server (daemon), which wished to handle
multiple show configurations in a single process concurrently. This
app would need to keep multiple configurations alive, and to manage them
appropriately.

Roughly speaking, a novice user should select a
default configuration that most closely approximates the use case
(animation, visual effects, etc.), and set the :envvar:`OCIO` environment
variable to point at the root of that configuration.

.. NOTE::
    Initialization using environment variables is typically preferable in
    a multi-app ecosystem, as it allows all applications to be
    consistently configured.


**class Config**


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: ConfigRcPtr createEditableCopy() const

      .. cpp:function:: unsigned int getMajorVersion() const

         Get the configuration major version.

      .. cpp:function:: void setMajorVersion(unsigned int major)

         Set the configuration major version.

      .. cpp:function:: unsigned int getMinorVersion() const

         Get the configuration minor version.

      .. cpp:function:: void setMinorVersion(unsigned int minor)

         Set the configuration minor version.

      .. cpp:function:: void upgradeToLatestVersion()

         Allows an older config to be serialized as the current
         version.

      .. cpp:function:: void sanityCheck() const

         Performs a thorough sanity check for the most common user
         errors.

         This will throw an exception if the config is malformed. The
         most common error occurs when references are made to
         colorspaces that do not exist.

      .. cpp:function:: char getFamilySeparator() const

         If not empty or null a single character to separate the
         family string in levels.

      .. cpp:function:: void setFamilySeparator(char separator)

         Succeeds if the characters is null or a valid character from
         the ASCII table i.e. from value 32 (i.e. space) to 126 (i.e.
         ‘~’); otherwise, it throws an exception.

      .. cpp:function:: const char *getDescription() const

      .. cpp:function:: void setDescription(const char *description)

      .. cpp:function:: void serialize(std::ostream &os) const

         Returns the string representation of the Config in YAML text
         form.

         This is typically stored on disk in a file with the extension
         .ocio.

      .. cpp:function:: const char *getCacheID() const

         This will produce a hash of the all colorspace definitions,
         etc. All external references, such as files used in
         FileTransforms, etc., will be incorporated into the cacheID.
         While the contents of the files are not read, the file system
         is queried for relevant information (mtime, inode) so that
         the config’s cacheID will change when the underlying luts are
         updated. If a context is not provided, the current Context
         will be used. If a null context is provided, file references
         will not be taken into account (this is essentially a hash of
         `Config::serialize`_).

      .. cpp:function:: const char *getCacheID(const ConstContextRcPtr &context) const
      

      .. cpp:function:: ConstContextRcPtr getCurrentContext() const

      .. cpp:function:: void addEnvironmentVar(const char *name, const char *defaultValue)

      .. cpp:function:: int getNumEnvironmentVars() const

      .. cpp:function:: const char *getEnvironmentVarNameByIndex(int index) const

      .. cpp:function:: const char *getEnvironmentVarDefault(const char *name) const
      

      .. cpp:function:: void clearEnvironmentVars()

      .. cpp:function:: void setEnvironmentMode(EnvironmentMode mode)

      .. cpp:function:: EnvironmentMode getEnvironmentMode() const

      .. cpp:function:: void loadEnvironment()

      .. cpp:function:: const char *getSearchPath() const

      .. cpp:function:: void setSearchPath(const char *path)

         Set all search paths as a concatenated string, ‘:’ to
         separate the paths.

         See `addSearchPath`_ for a more robust and platform-agnostic
         method of setting the search paths.

      .. cpp:function:: int getNumSearchPaths() const

      .. cpp:function:: const char *getSearchPath(int index) const

         Get a search path from the list.

         The paths are in the order they will be searched (that is,
         highest to lowest priority).

      .. cpp:function:: void clearSearchPaths()

      .. cpp:function:: void addSearchPath(const char *path)

         Add a single search path to the end of the list.

         Paths may be either absolute or relative. Relative paths are
         relative to the working directory. Forward slashes will be
         normalized to reverse for Windows. Environment (context)
         variables may be used in paths.

      .. cpp:function:: const char *getWorkingDir() const

      .. cpp:function:: void setWorkingDir(const char *dirname)

         The working directory defaults to the location of the config
         file. It is used to convert any relative paths to absolute.
         If no search paths have been set, the working directory will
         be used as the fallback search path. No environment (context)
         variables may be used in the working directory.

      .. cpp:function:: ColorSpaceSetRcPtr getColorSpaces(const char *category) const
      

         Get all active color spaces having a specific category in the
         order they appear in the config file.

         **Note**
         If the category is null or empty, the method returns all
         the active color spaces like
         :cpp:func:```Config::getNumColorSpaces`_`` and
         :cpp:func:```Config::getColorSpaceNameByIndex`_`` do.

         **Note**
         It’s worth noticing that the method returns a copy of the
         selected color spaces decoupling the result from the
         config. Hence, any changes on the config do not affect the
         existing color space sets, and vice-versa.

      .. cpp:function:: int getNumColorSpaces(SearchReferenceSpaceType searchReferenceType, ColorSpaceVisibility visibility) const

         Work on the color spaces selected by the reference color
         space type and visibility.

      .. cpp:function:: const char *getColorSpaceNameByIndex(SearchReferenceSpaceType searchReferenceType, ColorSpaceVisibility visibility, int index) const

         Work on the color spaces selected by the reference color
         space type and visibility (active or inactive).

         Return empty for invalid index.

      .. cpp:function:: ConstColorSpaceRcPtr getColorSpace(const char *name) const

         Get the color space from all the color spaces (i.e. active
         and inactive) and return null if the name is not found.

         **Note**
         The fcn accepts either a color space OR role name. (Color
         space names take precedence over roles.)

      .. cpp:function:: int getNumColorSpaces() const

         Work on the active color spaces only.

         **Note**
         Only works from the list of active color spaces.

      .. cpp:function:: const char *getColorSpaceNameByIndex(int index) const

         Work on the active color spaces only and return null for
         invalid index.

         **Note**
         Only works from the list of active color spaces.

      .. cpp:function:: int getIndexForColorSpace(const char *name) const

         Get an index from the active color spaces only and return -1
         if the name is not found.

         **Note**
         The fcn accepts either a color space OR role name. (Color
         space names take precedence over roles.)

      .. cpp:function:: void addColorSpace(const ConstColorSpaceRcPtr &cs)

         Add a color space to the configuration.

         **Note**
         If another color space is already present with the same
         name, this will overwrite it. This stores a copy of the
         specified color space.

         **Note**
         Adding a color space to a Config does not affect any
         ColorSpaceSet sets that have already been created.

      .. cpp:function:: void removeColorSpace(const char *name)

         Remove a color space from the configuration.

         **Note**
         It does not throw an exception if the color space is not
         present or used by an existing role. Role name arguments
         are ignored.

         **Note**
         Removing a color space to a Config does not affect any
         ColorSpaceSet sets that have already been created.

      .. cpp:function:: bool isColorSpaceUsed(const char *name) const noexcept

         Return true if the color space is used by a transform, a
         role, or a look.

      .. cpp:function:: void clearColorSpaces()

         Remove all the color spaces from the configuration.

         **Note**
         Removing color spaces from a Config does not affect any
         ColorSpaceSet sets that have already been created.

      .. cpp:function:: const char *parseColorSpaceFromString(const char *str) const
      

         Given the specified string, get the longest, right-most,
         colorspace substring that appears.

         * If strict parsing is enabled, and no color space is found,
         return an empty string.

         * If strict parsing is disabled, return ROLE_DEFAULT (if
         defined).

         * If the default role is not defined, return an empty
         string.

      .. cpp:function:: bool isStrictParsingEnabled() const

      .. cpp:function:: void setStrictParsingEnabled(bool enabled)

      .. cpp:function:: void setInactiveColorSpaces(const char *inactiveColorSpaces)
      

         Set/get a list of inactive color space names.

         * The inactive spaces are color spaces that should not
         appear in application menus.

         * These color spaces will still work in
         :cpp:func:``Config::getProcessor`` calls.

         * The argument is a comma-delimited string. A null or empty
         string empties the list.

         * The environment variable OCIO_INACTIVE_COLORSPACES may
         also be used to set the inactive color space list.

         * The env. var. takes precedence over the
         inactive_colorspaces list in the config file.

         * Setting the list via the API takes precedence over either
         the env. var. or the config file list.

         * Roles may not be used.

      .. cpp:function:: const char *getInactiveColorSpaces() const

      .. cpp:function:: void setRole(const char *role, const char *colorSpaceName)

         **Note**
         Setting the ``colorSpaceName`` name to a null string
         unsets it.

      .. cpp:function:: int getNumRoles() const

      .. cpp:function:: bool hasRole(const char *role) const

         Return true if the role has been defined.

      .. cpp:function:: const char *getRoleName(int index) const

         Get the role name at index, this will return values like
         ‘scene_linear’, ‘compositing_log’.

         Return empty string if index is out of range.

      .. cpp:function:: const char *getRoleColorSpace(int index) const

         Get the role color space at index.

         Return empty string if index is out of range.

      .. cpp:function:: void addSharedView(const char *view, const char *viewTransformName, const char *colorSpaceName, const char *looks, const char *ruleName, const char *description)

         Will throw if view or colorSpaceName are null or empty.

      .. cpp:function:: void removeSharedView(const char *view)

         Remove a shared view. Will throw if the view does not exist.

      .. cpp:function:: const char *getDefaultDisplay() const

      .. cpp:function:: int getNumDisplays() const

      .. cpp:function:: const char *getDisplay(int index) const

         Will return “” if the index is invalid.

      .. cpp:function:: const char *getDefaultView(const char *display) const

      .. cpp:function:: int getNumViews(const char *display) const

         Return the number of views attached to the display including
         the number of shared views if any. Return 0 if display does
         not exist.

      .. cpp:function:: const char *getView(const char *display, int index) const

      .. cpp:function:: int getNumViews(const char *display, const char *colorspaceName) const

         If the config has ViewingRules, get the number of active
         Views for this colorspace. (If there are no rules, it returns
         all of them.)

      .. cpp:function:: const char *getView(const char *display, const char *colorspaceName, int index) const

      .. cpp:function:: const char *getDisplayViewTransformName(const char *display, const char *view) const

         Returns the view_transform attribute of the (display, view)
         pair. View can be a shared view of the display. If display is
         null or empty, config shared views are used.

      .. cpp:function:: const char *getDisplayViewColorSpaceName(const char *display, const char *view) const

         Returns the colorspace attribute of the (display, view) pair.
         (Note that this may be either a color space or a display
         color space.)

      .. cpp:function:: const char *getDisplayViewLooks(const char *display, const char *view) const

         Returns the looks attribute of a (display, view) pair.

      .. cpp:function:: const char *getDisplayViewRule(const char *display, const char *view) const noexcept

         Returns the rule attribute of a (display, view) pair.

      .. cpp:function:: const char *getDisplayViewDescription(const char *display, const char *view) const noexcept

         Returns the description attribute of a (display, view) pair.

      .. cpp:function:: void addDisplayView(const char *display, const char *view, const char *colorSpaceName, const char *looks)

         For the (display, view) pair, specify which color space and
         look to use. If a look is not desired, then just pass a null
         or empty string.

      .. cpp:function:: void addDisplayView(const char *display, const char *view, const char *viewTransformName, const char *colorSpaceName, const char *looks, const char *ruleName, const char *description)

         For the (display, view) pair, specify the color space or
         alternatively specify the view transform and display color
         space. The looks, viewing rule, and description are optional.
         Pass a null or empty string for any optional arguments. If
         the view already exists, it is replaced.

         Will throw if:

         * Display, view or colorSpace are null or empty.

         * Display already has a shared view with the same name.

      .. cpp:function:: void addDisplaySharedView(const char *display, const char *sharedView)

         Add a (reference to a) shared view to a display.

         The shared view must be part of the config. See
         `Config::addSharedView`_

         This will throw if:

         * Display or view are null or empty.

         * Display already has a view (shared or not) with the same
         name.

      .. cpp:function:: void removeDisplayView(const char *display, const char *view)
      

         Remove the view and the display if no more views.

         It does not remove the associated color space. If the view
         name is a shared view, it only removes the reference to the
         view from the display but the shared view, remains in the
         config.

         Will throw if the view does not exist.

      .. cpp:function:: void clearDisplays()

      .. cpp:function:: void setActiveDisplays(const char *displays)

         $OCIO_ACTIVE_DISPLAYS envvar can, at runtime, optionally
         override the allowed displays. It is a comma or colon
         delimited list. Active displays that are not in the specified
         profile will be ignored, and the left-most defined display
         will be the default.

         Comma-delimited list of names to filter and order the active
         displays.

         **Note**
         The setter does not override the envvar. The getter does
         not take into account the envvar value and thus may not
         represent what the user is seeing.

      .. cpp:function:: const char *getActiveDisplays() const

      .. cpp:function:: void setActiveViews(const char *views)

         $OCIO_ACTIVE_VIEWS envvar can, at runtime, optionally
         override the allowed views. It is a comma or colon delimited
         list. Active views that are not in the specified profile will
         be ignored, and the left-most defined view will be the
         default.

         Comma-delimited list of names to filter and order the active
         views.

         **Note**
         The setter does not override the envvar. The getter does
         not take into account the envvar value and thus may not
         represent what the user is seeing.

      .. cpp:function:: const char *getActiveViews() const

      .. cpp:function:: int getNumDisplaysAll() const

         Get all displays in the config, ignoring the active_displays
         list.

      .. cpp:function:: const char *getDisplayAll(int index) const

      .. cpp:function:: int getNumViews(ViewType type, const char *display) const

         Get either the shared or display-defined views for a display.
         The active_views list is ignored. Passing a null or empty
         display (with type=VIEW_SHARED) returns the contents of the
         shared_views section of the config. Return 0 if display does
         not exist.

      .. cpp:function:: const char *getView(ViewType type, const char *display, int index) const**

      .. cpp:function:: ConstViewingRulesRcPtr getViewingRules() const noexcept

         Get read-only version of the viewing rules.

      .. cpp:function:: void setViewingRules(ConstViewingRulesRcPtr viewingRules)

         Set viewing rules.

         **Note**
         The argument is cloned.

      .. cpp:function:: void getDefaultLumaCoefs(double *rgb) const

         Get the default coefficients for computing luma.

         **Note**
         There is no “1 size fits all” set of luma coefficients.
         (The values are typically different for each colorspace,
         and the application of them may be nonsensical depending
         on the intensity coding anyways). Thus, the ‘right’ answer
         is to make these functions on the :cpp:class:``Config``
         class. However, it’s often useful to have a config-wide
         default so here it is. We will add the colorspace specific
         luma call if/when another client is interesting in using
         it.

      .. cpp:function:: void setDefaultLumaCoefs(const double *rgb)

         These should be normalized (sum to 1.0 exactly).

      .. cpp:function:: ConstLookRcPtr getLook(const char *name) const

      .. cpp:function:: int getNumLooks() const

      .. cpp:function:: const char *getLookNameByIndex(int index) const

      .. cpp:function:: void addLook(const ConstLookRcPtr &look)

      .. cpp:function:: void clearLooks()

      .. cpp:function:: int getNumViewTransforms() const noexcept

      .. cpp:function:: ConstViewTransformRcPtr getViewTransform(const char *name) const noexcept

      .. cpp:function:: const char *getViewTransformNameByIndex(int i) const noexcept
      

      .. cpp:function:: void addViewTransform(const ConstViewTransformRcPtr &viewTransform)

      .. cpp:function:: ConstViewTransformRcPtr getDefaultSceneToDisplayViewTransform() const

         The default transform to use for scene-referred to
         display-referred reference space conversions is the first
         scene-referred view transform listed in that section of the
         config (the one with the lowest index). Returns a null
         ConstTransformRcPtr if there isn’t one.

      .. cpp:function:: void clearViewTransforms()

      .. cpp:function:: ConstFileRulesRcPtr getFileRules() const noexcept

         Get read-only version of the file rules.

      .. cpp:function:: void setFileRules(ConstFileRulesRcPtr fileRules)

         Set file rules.

         **Note**
         The argument is cloned.

      .. cpp:function:: const char *getColorSpaceFromFilepath(const char *filePath) const

         Get the color space of the first rule that matched filePath.

      .. cpp:function:: const char *getColorSpaceFromFilepath(const char *filePath, size_t &ruleIndex) const

         Most applications will use the preceding method, but this
         method may be used for applications that want to know which
         was the highest priority rule to match filePath. The
         :cpp:func:```FileRules::getNumCustomKeys`_`` and custom keys
         methods may then be used to get additional information about
         the matching rule.

      .. cpp:function:: bool filepathOnlyMatchesDefaultRule(const char *filePath) const

         Returns true if the only rule matched by filePath is the
         default rule. This is a convenience method for applications
         that want to require the user to manually choose a color
         space when strictParsing is true and no other rules match.

      .. cpp:function:: ConstProcessorRcPtr getProcessor(const ConstContextRcPtr &context, const ConstColorSpaceRcPtr &srcColorSpace, const ConstColorSpaceRcPtr &dstColorSpace) const

      .. cpp:function:: ConstProcessorRcPtr getProcessor(const ConstColorSpaceRcPtr &srcColorSpace, const ConstColorSpaceRcPtr &dstColorSpace) const
      

      .. cpp:function:: ConstProcessorRcPtr getProcessor(const char *srcColorSpaceName, const char *dstColorSpaceName) const

         **Note**
         Names can be colorspace name, role name, or a combination
         of both.

      .. cpp:function:: ConstProcessorRcPtr getProcessor(const ConstContextRcPtr &context, const char *srcColorSpaceName, const char *dstColorSpaceName) const

      .. cpp:function:: ConstProcessorRcPtr getProcessor(const char *srcColorSpaceName, const char *display, const char *view) const
      

      .. cpp:function:: ConstProcessorRcPtr getProcessor(const ConstContextRcPtr &context, const char *srcColorSpaceName, const char *display, const char *view) const

      .. cpp:function:: ConstProcessorRcPtr getProcessor(const ConstTransformRcPtr &transform) const

         Get the processor for the specified transform.

         Not often needed, but will allow for the re-use of atomic
         OCIO functionality (such as to apply an individual LUT file).

      .. cpp:function:: ConstProcessorRcPtr getProcessor(const ConstTransformRcPtr &transform, TransformDirection direction) const

      .. cpp:function:: ConstProcessorRcPtr getProcessor(const ConstContextRcPtr &context, const ConstTransformRcPtr &transform, TransformDirection direction) const

      .. cpp:function:: Config(const Config&) = delete

      .. cpp:function:: `Config`_ &operator=(const Config&) = delete

      .. cpp:function:: ~Config()

      -[ Public Static Functions ]-

      .. cpp:function:: ConfigRcPtr Create()

         Create a default empty configuration.

      .. cpp:function:: ConstConfigRcPtr CreateRaw()

         Create a fall-back config.

         This may be useful to allow client apps to launch in cases
         when the supplied config path is not loadable.

      .. cpp:function:: ConstConfigRcPtr CreateFromEnv()

         Create a configuration using the OCIO environment variable.

         If the variable is missing or empty, returns the same result
         as `Config::CreateRaw`_ .

      .. cpp:function:: ConstConfigRcPtr CreateFromFile(const char *filename)

         Create a configuration using a specific config file.

      .. cpp:function:: ConstConfigRcPtr CreateFromStream(std::istream &istream)

         Create a configuration using a stream.

      .. cpp:function:: ConstProcessorRcPtr GetProcessor(const ConstConfigRcPtr &srcConfig, const char *srcColorSpaceName, const ConstConfigRcPtr &dstConfig, const char *dstColorSpaceName)

         Get a processor to convert between color spaces in two
         separate configs.

         This relies on both configs having the aces_interchange role
         (when srcName is scene-referred) or the role
         cie_xyz_d65_interchange (when srcName is display-referred)
         defined. An exception is thrown if that is not the case.

      .. cpp:function:: ConstProcessorRcPtr GetProcessor(const ConstContextRcPtr &srcContext, const ConstConfigRcPtr &srcConfig, const char *srcColorSpaceName, const ConstContextRcPtr &dstContext, const ConstConfigRcPtr &dstConfig, const char *dstColorSpaceName)

      .. cpp:function:: ConstProcessorRcPtr GetProcessor(const ConstConfigRcPtr &srcConfig, const char *srcColorSpaceName, const char *srcInterchangeName, const ConstConfigRcPtr &dstConfig, const char *dstColorSpaceName, const char *dstInterchangeName)

         The srcInterchangeName and dstInterchangeName must refer to a
         pair of color spaces in the two configs that are the same. A
         role name may also be used.

      .. cpp:function:: ConstProcessorRcPtr GetProcessor(const ConstContextRcPtr &srcContext, const ConstConfigRcPtr &srcConfig, const char *srcColorSpaceName, const char *srcInterchangeName, const ConstContextRcPtr &dstContext, const ConstConfigRcPtr &dstConfig, const char *dstColorSpaceName, const char *dstInterchangeName)


   .. group-tab:: Python

      .. py:class:: PyOpenColorIO.Config

      .. py:class:: ActiveColorSpaceIterator

      .. py:class:: ActiveColorSpaceNameIterator

      .. py:class:: ColorSpaceIterator

      .. py:class:: ColorSpaceNameIterator

      .. py:function:: CreateFromEnv() -> PyOpenColorIO.Config

      .. py:function:: CreateFromFile(fileName: str) -> PyOpenColorIO.Config

      .. py:function:: CreateFromStream(str: str) -> PyOpenColorIO.Config

      .. py:function:: CreateRaw() -> PyOpenColorIO.Config

      .. py:class:: DisplayIterator

      .. py:class:: EnvironmentVarNameIterator

      .. py:function:: GetProcessor(*args,**kwargs)

      Overloaded function.

         1. .. py:function:: GetProcessor(srcConfig: PyOpenColorIO.Config, srcColorSpaceName: str, dstConfig: PyOpenColorIO.Config, dstColorSpaceName: str) -> OpenColorIO_v2_0dev::Processor

         2. .. py:function:: GetProcessor(srcContext: OpenColorIO_v2_0dev::Context, srcConfig: PyOpenColorIO.Config, srcColorSpaceName: str, dstContext: OpenColorIO_v2_0dev::Context, dstConfig: PyOpenColorIO.Config, dstColorSpaceName: str) -> OpenColorIO_v2_0dev::Processor

         3. .. py:function:: GetProcessor(srcConfig: PyOpenColorIO.Config, srcColorSpaceName: str, srcInterchangeName: str, dstConfig: PyOpenColorIO.Config, dstColorSpaceName: str, dstInterchangeName: str) -> OpenColorIO_v2_0dev::Processor

         4. .. py:function:: GetProcessor(srcContext: OpenColorIO_v2_0dev::Context, srcConfig: PyOpenColorIO.Config, srcColorSpaceName: str, srcInterchangeName: str, dstContext: OpenColorIO_v2_0dev::Context, dstConfig: PyOpenColorIO.Config, dstColorSpaceName: str, dstInterchangeName: str) -> OpenColorIO_v2_0dev::Processor

      .. py:class:: LookIterator

      .. py:class:: LookNameIterator

      .. py:class:: RoleColorSpaceIterator

      .. py:class:: RoleNameIterator

      .. py:class:: SearchPathIterator

      .. py:class:: SharedViewIterator

      .. py:class:: ViewForColorSpaceIterator

      .. py:class:: ViewIterator

      .. py:class:: ViewTransformIterator

      .. py:class:: ViewTransformNameIterator

      .. py:method:: addColorSpace(self: PyOpenColorIO.Config, colorSpace: OpenColorIO_v2_0dev::ColorSpace) -> None

      .. py:method:: addDisplaySharedView(self: PyOpenColorIO.Config, display: str, view: str) -> None

      .. py:function:: addDisplayView(*args,**kwargs)

      Overloaded function.

         1. addDisplayView(self: PyOpenColorIO.Config, display: str,
         view: str, colorSpaceName: str, looks: str) -> None

         2. addDisplayView(self: PyOpenColorIO.Config, display: str,
         view: str, viewTransform: str, displayColorSpaceName:
         str, looks: str, ruleName: str, description: str) -> None

      .. py:method:: addEnvironmentVar(self: PyOpenColorIO.Config, name: str, defaultValue: str) -> None

      .. py:method:: addLook(self: PyOpenColorIO.Config, look: OpenColorIO_v2_0dev::Look) -> None

      .. py:method:: addSearchPath(self: PyOpenColorIO.Config, path: str) -> None

      .. py:method:: addSharedView(self: PyOpenColorIO.Config, view: str, viewTransformName: str, colorSpaceName: str, looks: str, ruleName: str, description: str) -> None

      .. py:method:: addViewTransform(self: PyOpenColorIO.Config, viewTransform: OpenColorIO_v2_0dev::ViewTransform) -> None

      .. py:method:: clearColorSpaces(self: PyOpenColorIO.Config) -> None

      .. py:method:: clearDisplays(self: PyOpenColorIO.Config) -> None

      .. py:method:: clearEnvironmentVars(self: PyOpenColorIO.Config) -> None

      .. py:method:: clearLooks(self: PyOpenColorIO.Config) -> None

      .. py:method:: clearSearchPaths(self: PyOpenColorIO.Config) -> None

      .. py:method:: clearViewTransforms(self: PyOpenColorIO.Config) -> None

      .. py:method:: filepathOnlyMatchesDefaultRule(self: PyOpenColorIO.Config, filePath: str) -> bool

      .. py:method:: getActiveDisplays(self: PyOpenColorIO.Config) -> str

      .. py:method:: getActiveViews(self: PyOpenColorIO.Config) -> str

      .. py:function:: getCacheID(*args,**kwargs)

         Overloaded function.

         1. getCacheID(self: PyOpenColorIO.Config) -> str

         2. getCacheID(self: PyOpenColorIO.Config, context: OpenColorIO_v2_0dev::Context) -> str

      .. py:function:: *getColorSpace(self: PyOpenColorIO.Config, name: str) -> OpenColorIO_v2_0dev::ColorSpace**

      .. py:function:: getColorSpaceFromFilepath(*args,**kwargs)

         Overloaded function.

         1. getColorSpaceFromFilepath(self: PyOpenColorIO.Config,
         filePath: str) -> str

         2. getColorSpaceFromFilepath(self: PyOpenColorIO.Config,
         filePath: str, ruleIndex: int) -> str

      .. py:function:: getColorSpaceNames(*args,**kwargs)

      Overloaded function.

         1. getColorSpaceNames(self: PyOpenColorIO.Config, searchReferenceType: PyOpenColorIO.SearchReferenceSpaceType, visibility: PyOpenColorIO.ColorSpaceVisibility) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 2, OpenColorIO_v2_0dev::SearchReferenceSpaceType, OpenColorIO_v2_0dev::ColorSpaceVisibility>

         2. getColorSpaceNames(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 4>

      .. py:function:: getColorSpaces(*args,**kwargs)

      Overloaded function.

         1. getColorSpaces(self: PyOpenColorIO.Config, category: str) -> OpenColorIO_v2_0dev::ColorSpaceSet

         2. getColorSpaces(self: PyOpenColorIO.Config, searchReferenceType: PyOpenColorIO.SearchReferenceSpaceType, visibility: PyOpenColorIO.ColorSpaceVisibility) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 3, OpenColorIO_v2_0dev::SearchReferenceSpaceType, OpenColorIO_v2_0dev::ColorSpaceVisibility>

         3. getColorSpaces(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 5>

      .. py:method:: getCurrentContext(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::Context

      .. py:method:: getDefaultDisplay(self: PyOpenColorIO.Config) -> str

      .. py:method:: getDefaultLumaCoefs(self: PyOpenColorIO.Config) -> List[float[3]]

      .. py:method:: getDefaultSceneToDisplayViewTransform(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::ViewTransform

      .. py:method:: getDefaultView(self: PyOpenColorIO.Config, display: str) -> str

      .. py:method:: getDescription(self: PyOpenColorIO.Config) -> str

      .. py:method:: getDisplayViewColorSpaceName(self: PyOpenColorIO.Config, display: str, view: str) -> str

      .. py:method:: getDisplayViewDescription(self: PyOpenColorIO.Config, display: str, view: str) -> str

      .. py:method:: getDisplayViewLooks(self: PyOpenColorIO.Config, display: str, view: str) -> str

      .. py:method:: getDisplayViewRule(self: PyOpenColorIO.Config, display: str, view: str) -> str

      .. py:method:: getDisplayViewTransformName(self: PyOpenColorIO.Config, display: str, view: str) -> str

      .. py:method:: getDisplays(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 8>

      .. py:method:: getEnvironmentMode(self: PyOpenColorIO.Config) -> PyOpenColorIO.EnvironmentMode

      .. py:method:: getEnvironmentVarDefault(self: PyOpenColorIO.Config, name: str) -> str

      .. py:method:: getEnvironmentVarNames(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 0>

      .. py:method:: getFamilySeparator(self: PyOpenColorIO.Config) -> str

      .. py:method:: getFileRules(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::FileRules

      .. py:method:: getInactiveColorSpaces(self: PyOpenColorIO.Config) -> str

      .. py:method:: getLook(self: PyOpenColorIO.Config, name: str) -> OpenColorIO_v2_0dev::Look

      .. py:method:: getLookNames(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 12>

      .. py:method:: getLooks(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 13>

      .. py:method:: getMajorVersion(self: PyOpenColorIO.Config) -> int

      .. py:method:: getMinorVersion(self: PyOpenColorIO.Config) -> int

      .. py:function:: getProcessor(*args,**kwargs)

         Overloaded function.

         1. getProcessor(self: PyOpenColorIO.Config, srcColorSpace: OpenColorIO_v2_0dev::ColorSpace, dstColorSpace: OpenColorIO_v2_0dev::ColorSpace) -> OpenColorIO_v2_0dev::Processor

         2. getProcessor(self: PyOpenColorIO.Config, context: OpenColorIO_v2_0dev::Context, srcColorSpace: OpenColorIO_v2_0dev::ColorSpace, dstColorSpace: OpenColorIO_v2_0dev::ColorSpace) -> OpenColorIO_v2_0dev::Processor

         3. getProcessor(self: PyOpenColorIO.Config, srcColorSpaceName: str, dstColorSpaceName: str) -> OpenColorIO_v2_0dev::Processor

         4. getProcessor(self: PyOpenColorIO.Config, context: OpenColorIO_v2_0dev::Context, srcColorSpaceName: str, dstColorSpaceName: str) -> OpenColorIO_v2_0dev::Processor

         5. getProcessor(self: PyOpenColorIO.Config, srcColorSpaceName: str, display: str, view: str) -> OpenColorIO_v2_0dev::Processor

         6. getProcessor(self: PyOpenColorIO.Config, context: OpenColorIO_v2_0dev::Context, srcColorSpaceName: str, display: str, view: str) -> OpenColorIO_v2_0dev::Processor

         7. getProcessor(self: PyOpenColorIO.Config, transform: PyOpenColorIO.Transform) -> OpenColorIO_v2_0dev::Processor

         8. getProcessor(self: PyOpenColorIO.Config, transform: PyOpenColorIO.Transform, direction: PyOpenColorIO.TransformDirection) -> OpenColorIO_v2_0dev::Processor

         9. getProcessor(self: PyOpenColorIO.Config, context: OpenColorIO_v2_0dev::Context, transform: PyOpenColorIO.Transform, direction: PyOpenColorIO.TransformDirection) -> OpenColorIO_v2_0dev::Processor

      .. py:method:: getRoleNames(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 6>

      .. py:method:: getRoles(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 7>

      .. py:method:: getSearchPath(self: PyOpenColorIO.Config) -> str

      .. py:method:: getSearchPaths(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 1>

      .. py:method:: getSharedViews(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 9>

      .. py:method:: getViewTransform(self: PyOpenColorIO.Config, name: str) -> OpenColorIO_v2_0dev::ViewTransform

      .. py:method:: getViewTransformNames(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 14>

      .. py:method:: getViewTransforms(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 15>

      .. py:method:: getViewingRules(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::ViewingRules

      .. py:function:: getViews(*args,**kwargs)

         Overloaded function.

         1. getViews(self: PyOpenColorIO.Config, display: str) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 10, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >

         2. getViews(self: PyOpenColorIO.Config, display: str, colorSpaceName: str) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 11, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >

      .. py:method:: getWorkingDir(self: PyOpenColorIO.Config) -> str

      .. py:method:: hasRole(self: PyOpenColorIO.Config, role: str) -> bool

      .. py:method:: isColorSpaceUsed(self: PyOpenColorIO.Config, name: str) -> bool

      .. py:method:: isStrictParsingEnabled(self: PyOpenColorIO.Config) -> bool

      .. py:method:: loadEnvironment(self: PyOpenColorIO.Config) -> None

      .. py:method:: parseColorSpaceFromString(self: PyOpenColorIO.Config, str: str) -> str

      .. py:method:: removeColorSpace(self: PyOpenColorIO.Config, name: str) -> None

      .. py:method:: removeDisplayView(self: PyOpenColorIO.Config, display: str, view: str) -> None

      .. py:method:: removeSharedView(self: PyOpenColorIO.Config, view: str) -> None

      .. py:method:: sanityCheck(self: PyOpenColorIO.Config) -> None**

      .. py:function:: serialize(*args,**kwargs)

         Overloaded function.

         1. serialize(self: PyOpenColorIO.Config, fileName: str) -> None

         2. serialize(self: PyOpenColorIO.Config) -> str

      .. py:method:: setActiveDisplays(self: PyOpenColorIO.Config, displays: str) -> None

      .. py:method:: setActiveViews(self: PyOpenColorIO.Config, views: str) -> None

      .. py:method:: setDefaultLumaCoefs(self: PyOpenColorIO.Config, rgb: List[float[3]]) -> None

      .. py:method:: setDescription(self: PyOpenColorIO.Config, description: str) -> None

      .. py:method:: setEnvironmentMode(self: PyOpenColorIO.Config, mode: PyOpenColorIO.EnvironmentMode) -> None

      .. py:method:: setFamilySeparator(self: PyOpenColorIO.Config, separator: str) -> None

      .. py:method:: setFileRules(self: PyOpenColorIO.Config, fileRules: OpenColorIO_v2_0dev::FileRules) -> None

      .. py:method:: setInactiveColorSpaces(self: PyOpenColorIO.Config, inactiveColorSpaces: str) -> None

      .. py:method:: setMajorVersion(self: PyOpenColorIO.Config, major: int) -> None

      .. py:method:: setMinorVersion(self: PyOpenColorIO.Config, minor: int) -> None

      .. py:method:: setRole(self: PyOpenColorIO.Config, role: str, colorSpaceName: str) -> None

      .. py:method:: setSearchPath(self: PyOpenColorIO.Config, path: str) -> None

      .. py:method:: setViewingRules(self: PyOpenColorIO.Config, ViewingRules: OpenColorIO_v2_0dev::ViewingRules) -> None

      .. py:method:: setWorkingDir(self: PyOpenColorIO.Config, dirName: str) -> None

      .. py:method:: upgradeToLatestVersion(self: PyOpenColorIO.Config) -> None
