
Config
******

.. class:: Config


Initialization
==============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: ConfigRcPtr OpenColorIO::Config::Create()

         Create a default empty configuration. 

      .. cpp:function:: ConstConfigRcPtr OpenColorIO::Config::CreateRaw()

         Create a fall-back config. 

         This may be useful to allow client apps to launch in cases when the supplied config path is not loadable. 

      .. cpp:function:: ConstConfigRcPtr OpenColorIO::Config::CreateFromEnv()

         Create a configuration using the OCIO environment variable. 

         If the variable is missing or empty, returns the same result as Config::CreateRaw . 

      .. cpp:function:: ConstConfigRcPtr OpenColorIO::Config::CreateFromFile(const char *filename)

         Create a configuration using a specific config file. 

      .. cpp:function:: ConstConfigRcPtr OpenColorIO::Config::CreateFromStream(std::istream &istream)

         Create a configuration using a stream. 

      .. cpp:function:: ConfigRcPtr OpenColorIO::Config::createEditableCopy() const

   .. group-tab:: Python

      .. py:method:: Config.__init__(self: PyOpenColorIO.Config) -> None

      .. py:method:: Config.CreateRaw() -> PyOpenColorIO.Config

      .. py:method:: Config.CreateFromEnv() -> PyOpenColorIO.Config

      .. py:method:: Config.CreateFromFile(fileName: str) -> PyOpenColorIO.Config

      .. py:method:: Config.CreateFromStream(str: str) -> PyOpenColorIO.Config


Methods
=======

.. tabs::

   .. group-tab:: C++

      **Versioning**

      .. cpp:function:: unsigned int OpenColorIO::Config::getMajorVersion() const

         Get the configuration major version. 

      .. cpp:function:: void OpenColorIO::Config::setMajorVersion(unsigned int major)

         Set the configuration major version. 

      .. cpp:function:: unsigned int OpenColorIO::Config::getMinorVersion() const

         Get the configuration minor version. 

      .. cpp:function:: void OpenColorIO::Config::setMinorVersion(unsigned int minor)

         Set the configuration minor version. 

      .. cpp:function:: void OpenColorIO::Config::upgradeToLatestVersion()

         Allows an older config to be serialized as the current version. 

      **Serialization**

      .. cpp:function:: void OpenColorIO::Config::sanityCheck() const

         Performs a thorough sanity check for the most common user errors. 

         This will throw an exception if the config is malformed. The most common error occurs when references are made to colorspaces that do not exist. 

      .. cpp:function:: void OpenColorIO::Config::serialize(std::ostream &os) const

         Returns the string representation of the Config in YAML text form. 

         This is typically stored on disk in a file with the extension .ocio. 

      **Family Separator**

      .. cpp:function:: char OpenColorIO::Config::getFamilySeparator() const

         If not empty or null a single character to separate the family string in levels. 

      .. cpp:function:: void OpenColorIO::Config::setFamilySeparator(char separator)

         Succeeds if the characters is null or a valid character from the ASCII table i.e. from value 32 (i.e. space) to 126 (i.e. ‘~’); otherwise, it throws an exception. 

      **Description**

      .. cpp:function:: const char *OpenColorIO::Config::getDescription() const

      .. cpp:function:: void OpenColorIO::Config::setDescription(const char *description)

      **Cache**

      Warning: doxygenfunction: Unable to resolve multiple matches for function “OpenColorIO::Config::getCacheID” with arguments () in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml.
         Potential matches:
         ::

            - const char *getCacheID() const
            - const char *getCacheID(const ConstContextRcPtr &context) const

      **Resources**

      .. cpp:function:: ConstContextRcPtr OpenColorIO::Config::getCurrentContext() const

      .. cpp:function:: void OpenColorIO::Config::addEnvironmentVar(const char *name, const char *defaultValue)

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::addEnvironmentVarNames” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::addEnvironmentVarDefault” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: void OpenColorIO::Config::clearEnvironmentVars()

      .. cpp:function:: EnvironmentMode OpenColorIO::Config::getEnvironmentMode() const

      .. cpp:function:: void OpenColorIO::Config::setEnvironmentMode(EnvironmentMode mode)

      .. cpp:function:: void OpenColorIO::Config::loadEnvironment()

      Warning: doxygenfunction: Unable to resolve multiple matches for function “OpenColorIO::Config::getSearchPath” with arguments () in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml.
         Potential matches:
         ::

            - const char *getSearchPath() const
            - const char *getSearchPath(int index) const

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::getSearchPaths” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: void OpenColorIO::Config::setSearchPath(const char *path)

         Set all search paths as a concatenated string, ‘:’ to separate the paths. 

         See addSearchPath for a more robust and platform-agnostic method of setting the search paths. 

      .. cpp:function:: void OpenColorIO::Config::clearSearchPaths()

      .. cpp:function:: void OpenColorIO::Config::addSearchPath(const char *path)

         Add a single search path to the end of the list. 

         Paths may be either absolute or relative. Relative paths are relative to the working directory. Forward slashes will be normalized to reverse for Windows. Environment (context) variables may be used in paths. 

      .. cpp:function:: const char *OpenColorIO::Config::getWorkingDir() const

      .. cpp:function:: void OpenColorIO::Config::setWorkingDir(const char *dirname)

         The working directory defaults to the location of the config file. It is used to convert any relative paths to absolute. If no search paths have been set, the working directory will be used as the fallback search path. No environment (context) variables may be used in the working directory. 

      **Color Spaces**

      .. cpp:function:: ConstColorSpaceRcPtr OpenColorIO::Config::getColorSpace(const char *name) const

         Get the color space from all the color spaces (i.e. active and inactive) and return null if the name is not found. 

         **Note**
            The fcn accepts either a color space OR role name. (Color space names take precedence over roles.) 

      .. cpp:function:: ColorSpaceSetRcPtr OpenColorIO::Config::getColorSpaces(const char *category) const

         Get all active color spaces having a specific category in the order they appear in the config file. 

         **Note**
            If the category is null or empty, the method returns all the active color spaces like :cpp:func:`Config::getNumColorSpaces` and :cpp:func:`Config::getColorSpaceNameByIndex` do.

         **Note**
            It’s worth noticing that the method returns a copy of the selected color spaces decoupling the result from the config. Hence, any changes on the config do not affect the existing color space sets, and vice-versa. 

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::getColorSpaceNames” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: void OpenColorIO::Config::addColorSpace(const ConstColorSpaceRcPtr &cs)

         Add a color space to the configuration. 

         **Note**
            If another color space is already present with the same name, this will overwrite it. This stores a copy of the specified color space. 

         **Note**
            Adding a color space to a Config does not affect any `ColorSpaceSet` sets that have already been created. 

      .. cpp:function:: void OpenColorIO::Config::removeColorSpace(const char *name)

         Remove a color space from the configuration. 

         **Note**
            It does not throw an exception if the color space is not present or used by an existing role. Role name arguments are ignored. 

         **Note**
            Removing a color space to a Config does not affect any `ColorSpaceSet` sets that have already been created. 

      .. cpp:function:: void OpenColorIO::Config::clearColorSpaces()

         Remove all the color spaces from the configuration. 

         **Note**
            Removing color spaces from a Config does not affect any `ColorSpaceSet` sets that have already been created. 

      .. cpp:function:: const char *OpenColorIO::Config::parseColorSpaceFromString(const char *str) const

         Given the specified string, get the longest, right-most, colorspace substring that appears. 

         * If strict parsing is enabled, and no color space is found, return an empty string.

         * If strict parsing is disabled, return ROLE_DEFAULT (if defined).

         * If the default role is not defined, return an empty string. 

      .. cpp:function:: bool OpenColorIO::Config::isStrictParsingEnabled() const

      **bool OpenColorIO::Config::isColorSpaceUsed(const char *name) const noexcept
      **

         Return true if the color space is used by a transform, a role, or a look. 

      .. cpp:function:: const char *OpenColorIO::Config::getInactiveColorSpaces() const

      .. cpp:function:: void OpenColorIO::Config::setInactiveColorSpaces(const char *inactiveColorSpaces)

         Set/get a list of inactive color space names. 

         * The inactive spaces are color spaces that should not appear in application menus.

         * These color spaces will still work in :cpp:func:``Config::getProcessor`` calls.

         * The argument is a comma-delimited string. A null or empty string empties the list.

         * The environment variable OCIO_INACTIVE_COLORSPACES may also be used to set the inactive color space list.

         * The env. var. takes precedence over the inactive_colorspaces list in the config file.

         * Setting the list via the API takes precedence over either the env. var. or the config file list.

         * Roles may not be used. 

      **Roles**

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::getRoles” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::getRoleNames” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: bool OpenColorIO::Config::hasRole(const char *role) const

         Return true if the role has been defined. 

      .. cpp:function:: void OpenColorIO::Config::setRole(const char *role, const char *colorSpaceName)

         **Note**
            Setting the ``colorSpaceName`` name to a null string unsets it. 

      **Display**

      .. cpp:function:: const char *OpenColorIO::Config::getDefaultDisplay() const

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::getDisplays” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: const char *OpenColorIO::Config::getDefaultView(const char *display) const

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::getViews” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: const char *OpenColorIO::Config::getDisplayViewTransformName(const char *display, const char *view) const

         Returns the view_transform attribute of the (display, view) pair. View can be a shared view of the display. If display is null or empty, config shared views are used. 

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::getDisplayColorSpaceName” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::getDisplayLooks” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::addDisplay” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::removeDisplay” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: void OpenColorIO::Config::clearDisplays()

      .. cpp:function:: void OpenColorIO::Config::setActiveDisplays(const char *displays)

         $OCIO_ACTIVE_DISPLAYS envvar can, at runtime, optionally override the allowed displays. It is a comma or colon delimited list. Active displays that are not in the specified profile will be ignored, and the left-most defined display will be the default. 

         Comma-delimited list of names to filter and order the active displays.

         **Note**
            The setter does not override the envvar. The getter does not take into account the envvar value and thus may not represent what the user is seeing. 

      .. cpp:function:: const char *OpenColorIO::Config::getActiveDisplays() const

      .. cpp:function:: void OpenColorIO::Config::setActiveViews(const char *views)

         $OCIO_ACTIVE_VIEWS envvar can, at runtime, optionally override the allowed views. It is a comma or colon delimited list. Active views that are not in the specified profile will be ignored, and the left-most defined view will be the default. 

         Comma-delimited list of names to filter and order the active views.

         **Note**
            The setter does not override the envvar. The getter does not take into account the envvar value and thus may not represent what the user is seeing. 

      .. cpp:function:: const char *OpenColorIO::Config::getActiveViews() const

      **Views**

      .. cpp:function:: ConstViewTransformRcPtr OpenColorIO::Config::getViewTransform(const char *name) const noexcept

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::getViewTransforms” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::getViewTransformNames” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: void OpenColorIO::Config::addViewTransform(const ConstViewTransformRcPtr &viewTransform)

      .. cpp:function:: ConstViewTransformRcPtr OpenColorIO::Config::getDefaultSceneToDisplayViewTransform() const

         The default transform to use for scene-referred to display-referred reference space conversions is the first scene-referred view transform listed in that section of the config (the one with the lowest index). Returns a null ConstTransformRcPtr if there isn’t one. 

      .. cpp:function:: void OpenColorIO::Config::clearViewTransforms()

      **Looks**

      .. cpp:function:: ConstLookRcPtr OpenColorIO::Config::getLook(const char *name) const

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::getLooks” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::getLookNames” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: void OpenColorIO::Config::addLook(const ConstLookRcPtr &look)

      .. cpp:function:: void OpenColorIO::Config::clearLooks()

      **Luma**

      Warning: doxygenfunction: Cannot find function “OpenColorIO::Config::getDeafultLumaCoefs” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: void OpenColorIO::Config::setDefaultLumaCoefs(const double *rgb)

         These should be normalized (sum to 1.0 exactly). 

      **File Rules**

      .. cpp:function:: ConstFileRulesRcPtr OpenColorIO::Config::getFileRules() const noexcept

         Get read-only version of the file rules. 

      .. cpp:function:: void OpenColorIO::Config::setFileRules(ConstFileRulesRcPtr fileRules)

         Set file rules. 

         **Note**
            The argument is cloned. 

      Warning: doxygenfunction: Unable to resolve multiple matches for function “OpenColorIO::Config::getColorSpaceFromFilepath” with arguments () in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml.
         Potential matches:
         ::

            - const char *getColorSpaceFromFilepath(const char *filePath) const
            - const char *getColorSpaceFromFilepath(const char *filePath, size_t &ruleIndex) const

      .. cpp:function:: bool OpenColorIO::Config::filepathOnlyMatchesDefaultRule(const char *filePath) const

         Returns true if the only rule matched by filePath is the default rule. This is a convenience method for applications that want to require the user to manually choose a color space when strictParsing is true and no other rules match. 

      **Processors**

      Warning: doxygenfunction: Unable to resolve multiple matches for function “OpenColorIO::Config::getProcessor” with arguments () in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml.
         Potential matches:
         ::

            - ConstProcessorRcPtr getProcessor(const ConstColorSpaceRcPtr &srcColorSpace, const ConstColorSpaceRcPtr &dstColorSpace) const
            - ConstProcessorRcPtr getProcessor(const ConstContextRcPtr &context, const ConstColorSpaceRcPtr &srcColorSpace, const ConstColorSpaceRcPtr &dstColorSpace) const
            - ConstProcessorRcPtr getProcessor(const ConstContextRcPtr &context, const ConstTransformRcPtr &transform, TransformDirection direction) const
            - ConstProcessorRcPtr getProcessor(const ConstContextRcPtr &context, const char *srcColorSpaceName, const char *display, const char *view) const
            - ConstProcessorRcPtr getProcessor(const ConstContextRcPtr &context, const char *srcColorSpaceName, const char *dstColorSpaceName) const
            - ConstProcessorRcPtr getProcessor(const ConstTransformRcPtr &transform) const
            - ConstProcessorRcPtr getProcessor(const ConstTransformRcPtr &transform, TransformDirection direction) const
            - ConstProcessorRcPtr getProcessor(const char *srcColorSpaceName, const char *display, const char *view) const
            - ConstProcessorRcPtr getProcessor(const char *srcColorSpaceName, const char *dstColorSpaceName) const

      Warning: doxygenfunction: Unable to resolve multiple matches for function “OpenColorIO::Config::GetProcessor” with arguments () in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml.
         Potential matches:
         ::

            - ConstProcessorRcPtr GetProcessor(const ConstConfigRcPtr &srcConfig, const char *srcColorSpaceName, const ConstConfigRcPtr &dstConfig, const char *dstColorSpaceName)
            - ConstProcessorRcPtr GetProcessor(const ConstConfigRcPtr &srcConfig, const char *srcColorSpaceName, const char *srcInterchangeName, const ConstConfigRcPtr &dstConfig, const char *dstColorSpaceName, const char *dstInterchangeName)
            - ConstProcessorRcPtr GetProcessor(const ConstContextRcPtr &srcContext, const ConstConfigRcPtr &srcConfig, const char *srcColorSpaceName, const ConstContextRcPtr &dstContext, const ConstConfigRcPtr &dstConfig, const char *dstColorSpaceName)
            - ConstProcessorRcPtr GetProcessor(const ConstContextRcPtr &srcContext, const ConstConfigRcPtr &srcConfig, const char *srcColorSpaceName, const char *srcInterchangeName, const ConstContextRcPtr &dstContext, const ConstConfigRcPtr &dstConfig, const char *dstColorSpaceName, const char *dstInterchangeName)

   .. group-tab:: Python

      **Versioning**

      .. py:method:: Config.getMajorVersion(self: PyOpenColorIO.Config) -> int

      .. py:method:: Config.setMajorVersion(self: PyOpenColorIO.Config, major: int) -> None

      .. py:method:: Config.getMinorVersion(self: PyOpenColorIO.Config) -> int

      .. py:method:: Config.setMinorVersion(self: PyOpenColorIO.Config, minor: int) -> None

      .. py:method:: Config.upgradeToLatestVersion(self: PyOpenColorIO.Config) -> None

      **Serialization**

      .. py:method:: Config.sanityCheck(self: PyOpenColorIO.Config) -> None

      .. py:method:: Config.serialize(*args, **kwargs)

         Overloaded function.

         1. serialize(self: PyOpenColorIO.Config, fileName: str) -> None

         2. serialize(self: PyOpenColorIO.Config) -> str

      **Family Separator**

      .. py:method:: Config.getFamilySeparator(self: PyOpenColorIO.Config) -> str

      .. py:method:: Config.setFamilySeparator(self: PyOpenColorIO.Config, separator: str) -> None

      **Description**

      .. py:method:: Config.getDescription(self: PyOpenColorIO.Config) -> str

      .. py:method:: Config.setDescription(self: PyOpenColorIO.Config, description: str) -> None

      **Cache**

      .. py:method:: Config.getCacheID(*args, **kwargs)

         Overloaded function.

         1. getCacheID(self: PyOpenColorIO.Config) -> str

         2. getCacheID(self: PyOpenColorIO.Config, context: OpenColorIO_v2_0dev::Context) -> str

      **Resources**

      .. py:method:: Config.getCurrentContext(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::Context

      .. py:method:: Config.addEnvironmentVar(self: PyOpenColorIO.Config, name: str, defaultValue: str) -> None

      .. py:method:: Config.clearEnvironmentVars(self: PyOpenColorIO.Config) -> None

      .. py:method:: Config.getEnvironmentMode(self: PyOpenColorIO.Config) -> PyOpenColorIO.EnvironmentMode

      .. py:method:: Config.setEnvironmentMode(self: PyOpenColorIO.Config, mode: PyOpenColorIO.EnvironmentMode) -> None

      .. py:method:: Config.loadEnvironment(self: PyOpenColorIO.Config) -> None

      .. py:method:: Config.getSearchPath(self: PyOpenColorIO.Config) -> str

      .. py:method:: Config.getSearchPaths(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 1>

      .. py:method:: Config.setSearchPath(self: PyOpenColorIO.Config, path: str) -> None

      .. py:method:: Config.clearSearchPaths(self: PyOpenColorIO.Config) -> None

      .. py:method:: Config.addSearchPath(self: PyOpenColorIO.Config, path: str) -> None

      .. py:method:: Config.getWorkingDir(self: PyOpenColorIO.Config) -> str

      .. py:method:: Config.setWorkingDir(self: PyOpenColorIO.Config, dirName: str) -> None

      **Color Spaces**

      .. py:method:: Config.getColorSpace(self: PyOpenColorIO.Config, name: str) -> OpenColorIO_v2_0dev::ColorSpace

      .. py:method:: Config.getColorSpaces(*args, **kwargs)

         Overloaded function.

         1. getColorSpaces(self: PyOpenColorIO.Config, category: str) -> OpenColorIO_v2_0dev::ColorSpaceSet

         2. getColorSpaces(self: PyOpenColorIO.Config, searchReferenceType: PyOpenColorIO.SearchReferenceSpaceType, visibility: PyOpenColorIO.ColorSpaceVisibility) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 3, OpenColorIO_v2_0dev::SearchReferenceSpaceType, OpenColorIO_v2_0dev::ColorSpaceVisibility>

         3. getColorSpaces(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 5>

      .. py:method:: Config.getColorSpaceNames(*args, **kwargs)

         Overloaded function.

         1. getColorSpaceNames(self: PyOpenColorIO.Config, searchReferenceType: PyOpenColorIO.SearchReferenceSpaceType, visibility: PyOpenColorIO.ColorSpaceVisibility) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 2, OpenColorIO_v2_0dev::SearchReferenceSpaceType, OpenColorIO_v2_0dev::ColorSpaceVisibility>

         2. getColorSpaceNames(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 4>

      .. py:method:: Config.addColorSpace(self: PyOpenColorIO.Config, colorSpace: OpenColorIO_v2_0dev::ColorSpace) -> None

      .. py:method:: Config.removeColorSpace(self: PyOpenColorIO.Config, name: str) -> None

      .. py:method:: Config.clearColorSpaces(self: PyOpenColorIO.Config) -> None

      .. py:method:: Config.parseColorSpaceFromString(self: PyOpenColorIO.Config, str: str) -> str

      .. py:method:: Config.isStrictParsingEnabled(self: PyOpenColorIO.Config) -> bool

      .. py:method:: Config.isColorSpaceUsed(self: PyOpenColorIO.Config, name: str) -> bool

      .. py:method:: Config.getInactiveColorSpaces(self: PyOpenColorIO.Config) -> str

      .. py:method:: Config.setInactiveColorSpaces(self: PyOpenColorIO.Config, inactiveColorSpaces: str) -> None

      **Roles**

      .. py:method:: Config.getRoles(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 7>

      .. py:method:: Config.getRoleNames(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 6>

      .. py:method:: Config.hasRole(self: PyOpenColorIO.Config, role: str) -> bool

      .. py:method:: Config.setRole(self: PyOpenColorIO.Config, role: str, colorSpaceName: str) -> None

      **Display**

      .. py:method:: Config.getDefaultDisplay(self: PyOpenColorIO.Config) -> str

      .. py:method:: Config.getDisplays(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 8>

      .. py:method:: Config.getDefaultView(self: PyOpenColorIO.Config, display: str) -> str

      .. py:method:: Config.getViews(*args, **kwargs)

         Overloaded function.

         1. getViews(self: PyOpenColorIO.Config, display: str) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 10, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >

         2. getViews(self: PyOpenColorIO.Config, display: str, colorSpaceName: str) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 11, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >

      .. py:method:: Config.getDisplayViewTransformName(self: PyOpenColorIO.Config, display: str, view: str) -> str

      .. py:method:: Config.clearDisplays(self: PyOpenColorIO.Config) -> None

      .. py:method:: Config.setActiveDisplays(self: PyOpenColorIO.Config, displays: str) -> None

      .. py:method:: Config.getActiveDisplays(self: PyOpenColorIO.Config) -> str

      .. py:method:: Config.setActiveViews(self: PyOpenColorIO.Config, views: str) -> None

      .. py:method:: Config.getActiveViews(self: PyOpenColorIO.Config) -> str**

      **Views**

      .. py:method:: Config.getViewTransform(self: PyOpenColorIO.Config, name: str) -> OpenColorIO_v2_0dev::ViewTransform

      .. py:method:: Config.getViewTransforms(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 15>

      .. py:method:: Config.getViewTransformNames(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 14>

      .. py:method:: Config.addViewTransform(self: PyOpenColorIO.Config, viewTransform: OpenColorIO_v2_0dev::ViewTransform) -> None

      .. py:method:: Config.getDefaultSceneToDisplayViewTransform(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::ViewTransform

      .. py:method:: Config.clearViewTransforms(self: PyOpenColorIO.Config) -> None

      **Looks**

      .. py:method:: Config.getLook(self: PyOpenColorIO.Config, name: str) -> OpenColorIO_v2_0dev::Look

      .. py:method:: Config.getLooks(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 13>

      .. py:method:: Config.getLookNames(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::Config>, 12>

      .. py:method:: Config.addLook(self: PyOpenColorIO.Config, look: OpenColorIO_v2_0dev::Look) -> None

      .. py:method:: Config.clearLooks(self: PyOpenColorIO.Config) -> None

      **Luma**

      .. py:method:: Config.setDefaultLumaCoefs(self: PyOpenColorIO.Config, rgb: List[float[3]]) -> None

      **File Rules**

      .. py:method:: Config.getFileRules(self: PyOpenColorIO.Config) -> OpenColorIO_v2_0dev::FileRules

      .. py:method:: Config.setFileRules(self: PyOpenColorIO.Config, fileRules: OpenColorIO_v2_0dev::FileRules) -> None

      .. py:method:: Config.getColorSpaceFromFilepath(*args, **kwargs)

         Overloaded function.

         1. getColorSpaceFromFilepath(self: PyOpenColorIO.Config, filePath: str) -> str

         2. getColorSpaceFromFilepath(self: PyOpenColorIO.Config, filePath: str, ruleIndex: int) -> str

      .. py:method:: Config.filepathOnlyMatchesDefaultRule(self: PyOpenColorIO.Config, filePath: str) -> bool

      **Processors**

      .. py:method:: Config.getProcessor(*args, **kwargs)

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

      .. py:method:: static Config.GetProcessor(*args, **kwargs)

         Overloaded function.

         1. GetProcessor(srcConfig: PyOpenColorIO.Config, srcColorSpaceName: str, dstConfig: PyOpenColorIO.Config, dstColorSpaceName: str) -> OpenColorIO_v2_0dev::Processor

         2. GetProcessor(srcContext: OpenColorIO_v2_0dev::Context, srcConfig: PyOpenColorIO.Config, srcColorSpaceName: str, dstContext: OpenColorIO_v2_0dev::Context, dstConfig: PyOpenColorIO.Config, dstColorSpaceName: str) -> OpenColorIO_v2_0dev::Processor

         3. GetProcessor(srcConfig: PyOpenColorIO.Config, srcColorSpaceName: str, srcInterchangeName: str, dstConfig: PyOpenColorIO.Config, dstColorSpaceName: str, dstInterchangeName: str) -> OpenColorIO_v2_0dev::Processor

         4. GetProcessor(srcContext: OpenColorIO_v2_0dev::Context, srcConfig: PyOpenColorIO.Config, srcColorSpaceName: str, srcInterchangeName: str, dstContext: OpenColorIO_v2_0dev::Context, dstConfig: PyOpenColorIO.Config, dstColorSpaceName: str, dstInterchangeName: str) -> OpenColorIO_v2_0dev::Processor
