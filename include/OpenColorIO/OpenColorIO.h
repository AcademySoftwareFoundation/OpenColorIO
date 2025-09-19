// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_OPENCOLORIO_H
#define INCLUDED_OCIO_OPENCOLORIO_H

#include <cstddef>
#include <iosfwd>
#include <limits>
#include <stdexcept>
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <map>

#include "OpenColorABI.h"
#include "OpenColorTypes.h"
#include "OpenColorTransforms.h"
#include "OpenColorAppHelpers.h"

/*

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
// Exceptions

// Silence warning C4275 under Visual Studio:
// Exceptions derive from std::runtime_error but STL classes are not exportable.
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4275 )
#endif

/**
 * \brief An exception class to throw for errors detected at runtime.
 *
 * \warning
 *    All functions in the Config class can potentially throw this exception.
 */
class OCIOEXPORT Exception : public std::runtime_error
{
public:
    Exception() = delete;
    /// Constructor that takes a string as the exception message.
    explicit Exception(const char *);
    /// Constructor that takes an existing exception.
    Exception(const Exception &);
    Exception & operator= (const Exception &) = delete;

    ~Exception();
};

/**
 * \brief An exception class for errors detected at runtime.
 * 
 * Thrown when OCIO cannot find a file that is expected to exist. This is provided as a custom 
 * type to distinguish cases where one wants to continue looking for missing files, but wants 
 * to properly fail for other error conditions.
 */
class OCIOEXPORT ExceptionMissingFile : public Exception
{
public:
    ExceptionMissingFile() = delete;
    /// Constructor that takes a string as the exception message.
    explicit ExceptionMissingFile(const char *);
    /// Constructor that takes an existing exception.
    ExceptionMissingFile(const ExceptionMissingFile &);
    ExceptionMissingFile & operator= (const ExceptionMissingFile &) = delete;

    ~ExceptionMissingFile();
};

// Restore default warning behaviour for Visual Studio.
#ifdef _MSC_VER
#pragma warning( pop )
#endif

///////////////////////////////////////////////////////////////////////////
// Global
// ******

/**
 * During normal usage, OpenColorIO tends to cache certain global information (such
 * as the contents of LUTs on disk, intermediate results, etc.). Calling this function will flush
 * all such information. The global information are related to LUT file identifications, loaded LUT
 * file content and CDL transforms from loaded CDL files.
 *
 * Under normal usage, this is not necessary, but it can be helpful in particular instances,
 * such as designing OCIO profiles, and wanting to re-read luts without restarting.
 *
 * \note 
 *   This method does not apply to instance-specific caches such as the Processor cache in
 *   a Config instance or the GPU and CPU Processor caches in a Processor instance. So in cases
 *   where you still have a Config instance after calling ClearAllCaches, you should also call 
 *   the Config's clearProcessorCache method.
 */
extern OCIOEXPORT void ClearAllCaches();

/**
 * \brief Get the version number for the library, as a dot-delimited string 
 *     (e.g., "1.0.0").
 * 
 * This is also available at compile time as OCIO_VERSION_FULL_STR.
 */
extern OCIOEXPORT const char * GetVersion();

/**
 * \brief Get the version number for the library, as a
 * single 4-byte hex number (e.g., 0x01050200 for "1.5.2"), to be used
 * for numeric comparisons.
 * 
 * This is also at compile time as OCIO_VERSION_HEX.
 */
extern OCIOEXPORT int GetVersionHex();

/**
 * \brief Get the global logging level.
 * 
 * You can override this at runtime using the \ref OCIO_LOGGING_LEVEL
 * environment variable. The client application that sets this should use
 * \ref SetLoggingLevel, and not the environment variable. The default value is INFO.
 */
extern OCIOEXPORT LoggingLevel GetLoggingLevel();

/// Set the global logging level.
extern OCIOEXPORT void SetLoggingLevel(LoggingLevel level);

/**
 * \brief Set the logging function to use; otherwise, use the default
 * (i.e. std::cerr).
 * 
 * \note 
 *     The logging mechanism is thread-safe.
 */
extern OCIOEXPORT void SetLoggingFunction(LoggingFunction logFunction);
extern OCIOEXPORT void ResetToDefaultLoggingFunction();
/// Log a message using the library logging function.
extern OCIOEXPORT void LogMessage(LoggingLevel level, const char * message);

/**
 * \brief Set the Compute Hash Function to use; otherwise, use the default.
 * 
 * This is not used when using CreateFromFile with an OCIOZ archive or CreateFromConfigIOProxy.
 * 
 * \param ComputeHashFunction
 */
extern OCIOEXPORT void SetComputeHashFunction(ComputeHashFunction hashFunction);
extern OCIOEXPORT void ResetComputeHashFunction();

//
// Note that the following environment variable access methods are not thread safe.
//

/** 
 * Another call modifies the string obtained from a previous call as the method always uses the
 * same memory buffer.
 */
extern OCIOEXPORT const char * GetEnvVariable(const char * name);
/// \warning This method is not thread safe.
extern OCIOEXPORT void SetEnvVariable(const char * name, const char * value);
/// \warning This method is not thread safe.
extern OCIOEXPORT void UnsetEnvVariable(const char * name);
//!cpp:function::
extern OCIOEXPORT bool IsEnvVariablePresent(const char * name);

/// Get the current configuration.
extern OCIOEXPORT ConstConfigRcPtr GetCurrentConfig();

/// Set the current configuration. This will then store a copy of the specified config.
extern OCIOEXPORT void SetCurrentConfig(const ConstConfigRcPtr & config);

/**
 * \brief Make a config path forward-compatible by replacing special built-in config names 
 *        with the current name.
 * 
 * Application developers should call this function on any config path they intend to persist 
 * (e.g., to include in a file saved from a DCC).
 * 
 * As the built-in config collection evolves, special names such as "ocio://default" and 
 * "ocio://studio-config-latest" will point to newer versions of those configs. Therefore, it is 
 * recommended that application developers not save those strings and instead save the string that 
 * refers to the current version of that config. That way, it's guaranteed that there will be no 
 * change of behavior in the future. For example, as of OCIO 2.4, "ocio://default" should be saved
 * as "ocio://cg-config-v2.2.0_aces-v1.3_ocio-v2.4".
 * 
 * Note that there is no validation done on the path. That is left to the application since 
 * typically the application will load the config before attempting to save its path
 * and therefore catch, for example, a badly formed URI such as "ocio:default".
 * 
 * \return Resolved path if possible. Otherwise, the original path is returned unmodified.
 */
extern OCIOEXPORT const char * ResolveConfigPath(const char * originalPath) noexcept;

/**
 * \brief Extract an OCIO Config archive.
 * 
 * Converts an archived config file (.ocioz file) back to its original form as a config file
 * and associated LUT files.  This creates destinationDir and then creates a config.ocio file
 * at the root of that working directory and then unpacks the LUT files into their relative
 * locations relative to that working directory, creating any necessary sub-directories in the
 * process.  Note that configs which contain LUT files outside the working directory are not
 * archivable, and so this function will not create directories outside the working directory.
 * 
 * \param archivePath Absolute path to the .ocioz file.
 * \param destinationDir Absolute path of the directory you want to be created to contain the
 * contents of the unarchived config.
 * \throw Exception If the archive is not found or there is a problem extracting it.
 */
extern OCIOEXPORT void ExtractOCIOZArchive(
    const char * archivePath, 
    const char * destinationDir
);

/**
 * \brief
 * A config defines all the color spaces to be available at runtime.
 *
 * The color configuration (Config) is the main object for interacting with this library. It 
 * encapsulates all of the information necessary to use customized ColorSpaceTransform and
 * DisplayViewTransform operations.
 *
 * See the \ref user-guide for more information on selecting, creating, and working with custom 
 * color configurations.
 *
 * For applications interested in using only one color config at a time (this is the vast majority
 * of apps), their API would traditionally get the global configuration and use that, as opposed 
 * to creating a new one. This simplifies the use case for plugins and bindings, as it alleviates 
 * the need to pass around configuration handles.
 *
 * An example of an application where this would not be sufficient would be a multi-threaded image
 * proxy server (daemon), which wished to handle multiple show configurations in a single process 
 * concurrently. This app would need to keep multiple configurations alive, and to manage them
 * appropriately.
 *
 * Roughly speaking, a novice user should select a default configuration that most closely 
 * approximates the use case (animation, visual effects, etc.), and set the :envvar:`OCIO` 
 * environment variable to point at the root of that configuration.
 *
 * \note
 *    Initialization using environment variables is typically preferable in
 *    a multi-app ecosystem, as it allows all applications to be
 *    consistently configured.
 *
 * See \ref developers-usageexamples
 */
class OCIOEXPORT Config
{
public:

    // 
    // Initialization
    //

    /**
     * \brief Create an empty config of the current version.
     *
     * Note that an empty config will not pass validation since required elements will be missing.
     * \return The Config object.
     */
    static ConfigRcPtr Create();

    /**
     * \brief Create a fall-back config.
     * 
     * This may be useful to allow client apps to launch in cases when the
     * supplied config path is not loadable.
     * \return The Config object.
     */
    static ConstConfigRcPtr CreateRaw();

    /**
     * \brief Create a configuration using the OCIO environment variable.
     * 
     * Also supports the OCIO URI format for Built-in configs and supports archived configs.
     * See \ref Config::CreateFromFile.
     * 
     * If the variable is missing or empty, returns the same result as 
     * \ref Config::CreateRaw.
     * \return The Config object.
     */
    static ConstConfigRcPtr CreateFromEnv();

    /**
     * \brief Create a configuration using a specific config file.
     * 
     * Supports the OCIO URI format for Built-in configs.
     * See \ref Config::CreateFromBuiltinConfig.
     *
     * Supports archived configs (.ocioz files).
     * 
     * \throw Exception If the file may not be read or does not parse.
     * \return The Config object.
     */
    static ConstConfigRcPtr CreateFromFile(const char * filename);

    /**
     * \brief Create a configuration using a stream.
     * 
     * Note that CreateFromStream does not set the working directory so the caller would need to
     * set that separately in order to resolve FileTransforms.  This function is typically only
     * used for self-contained configs (no LUTs).
     *
     * Configs created from CreateFromStream can not be archived unless the working directory is 
     * set and contains any necessary LUT files.
     * 
     * \param istream Stream to the config.
     * \throw Exception If the stream does not parse.
     * \return The Config object.
     */
    static ConstConfigRcPtr CreateFromStream(std::istream & istream);

    /**
     * \brief Create a config from the supplied ConfigIOProxy object. This allows the calling
     * program to directly provide the config and associated LUTs rather than reading them from
     * the standard file system.
     * 
     * See the \ref ConfigIOProxy class documentation for more info.
     * 
     * \param ciop ConfigIOProxy object providing access to the config's files.
     * \throw Exception If the config may not be read from the proxy, or does not parse.
     * \return The Config object.
     */
    static ConstConfigRcPtr CreateFromConfigIOProxy(ConfigIOProxyRcPtr ciop);
    
    /**
     * \brief Create a configuration using an OCIO built-in config.
     * 
     * \param configName Built-in config name (with or without the "ocio://" URI prefix).
     * 
     * Also supports the following OCIO URI format for Built-in configs:
     *  "ocio://default"                - Default Built-in config.
     *  "ocio://cg-config-latest"       - Latest Built-in CG config.
     *  "ocio://studio-config-latest"   - Latest Built-in Studio config.
     *  "ocio://<CONFIG NAME>"          - A specific Built-in config.
     * 
     * The available configNames are:
     * 
     * ACES Studio config, contains a more complete collection of color spaces and displays:
     * "studio-config-v1.0.0_aces-v1.3_ocio-v2.1"
     * "studio-config-v2.1.0_aces-v1.3_ocio-v2.3"
     * "studio-config-v2.2.0_aces-v1.3_ocio-v2.4"
     * 
     * ACES CG config, basic color spaces for computer graphics apps:
     * "cg-config-v1.0.0_aces-v1.3_ocio-v2.1"
     * "cg-config-v2.1.0_aces-v1.3_ocio-v2.3"
     * "cg-config-v2.2.0_aces-v1.3_ocio-v2.4"
     * 
     * More information is available at: 
     * %https://github.com/AcademySoftwareFoundation/OpenColorIO-Config-ACES
     * 
     * Information about the available configs is available from the \ref BuiltinConfigRegistry.
     * 
     * \throw Exception If the configName is not recognized.
     * \return One of the configs built into the OCIO library.
     */
    static ConstConfigRcPtr CreateFromBuiltinConfig(const char * configName);

    ConfigRcPtr createEditableCopy() const;

    /// Get the configuration major version.
    unsigned int getMajorVersion() const;

    /**
     * Set the configuration major version.
     *
     * Throws if it is not supported. Resets minor to the most recent minor for the given major.
     */
    void setMajorVersion(unsigned int major);

    /// Get the configuration minor version.
    unsigned int getMinorVersion() const;

    /// Set the configuration minor version. Throws if it is not supported for the current major.
    void setMinorVersion(unsigned int minor);

    /// Set the configuration major and minor versions. Throws if version is not supported.
    void setVersion(unsigned int major, unsigned int minor);

    /// Allows an older config to be serialized as the current version.
    void upgradeToLatestVersion() noexcept;

    /**
     * \brief Performs a thorough validation for the most common user errors.
     * 
     * This will throw an exception if the config is malformed. The most
     * common error occurs when references are made to colorspaces that do not
     * exist.
     */
    void validate() const;

    /**
     * \brief Get/set a name string for the config.
     *
     * The name string may be used to communicate config update details or similar information
     * to workflows external to OCIO in cases where the config path/filename itself does not
     * provide adequate information.
     */
    const char * getName() const noexcept;
    void setName(const char * name) noexcept;

    /**
     * \brief Get the family separator
     * 
     * A single character used to separate the family string into tokens for use in hierarchical
     * menus.  Defaults to '/'.
     */
    char getFamilySeparator() const;
    /// Get the default family separator i.e. '/' .
    static char GetDefaultFamilySeparator() noexcept;
    /**
     * \brief Set the family separator
     *
     * Succeeds if the characters is null or a valid character from the ASCII table i.e. from
     * value 32 (i.e. space) to 126 (i.e. '~'); otherwise, it throws an exception.
     */
    void setFamilySeparator(char separator);

    const char * getDescription() const;
    void setDescription(const char * description);

    /**
     * \brief Returns the string representation of the Config in YAML text form.
     * 
     * This is typically stored on disk in a file with the extension .ocio.
     * NB: This does not validate the config.  Applications should validate before serializing.
     */
    void serialize(std::ostream & os) const;

    /**
     * This will produce a hash of the all colorspace definitions, etc. All external references, 
     * such as files used in FileTransforms, etc., will be incorporated into the cacheID. While 
     * the contents of the files are not read, the file system is queried for relevant information 
     * (mtime, inode) so that the config's cacheID will change when the underlying luts are updated.
     * 
     * If a context is not provided, the current Context will be used.
     * 
     * If a null context is provided, file references will not be taken into 
     * account (this is essentially a hash of Config::serialize).
     */
    const char * getCacheID() const;
    const char * getCacheID(const ConstContextRcPtr & context) const;

    // 
    // Resources
    //

    ConstContextRcPtr getCurrentContext() const;

    /// Add (or update) an environment variable with a default value.
    /// But it removes it if the default value is null.
    void addEnvironmentVar(const char * name, const char * defaultValue);
    int getNumEnvironmentVars() const;
    const char * getEnvironmentVarNameByIndex(int index) const;
    const char * getEnvironmentVarDefault(const char * name) const;
    void clearEnvironmentVars();

    /**
     * \brief The EnvironmentMode controls the behavior of loadEnvironment.
     *  * ENV_ENVIRONMENT_LOAD_PREDEFINED - Only update vars already added to the Context.
     *  * ENV_ENVIRONMENT_LOAD_ALL        - Load all env. vars into the Context.
     *
     * \note Loading ALL the env. vars may reduce performance and reduce cache efficiency.
     *
     * Client programs generally will not use these methods because the EnvironmentMode is
     * set automatically when a Config is loaded.  If the Config has an "environment"
     * section, the mode is set to LOAD_PREDEFINED, and otherwise set to LOAD_ALL.
     */
    void setEnvironmentMode(EnvironmentMode mode) noexcept;
    EnvironmentMode getEnvironmentMode() const noexcept;
    /// Initialize the environment/context variables in the Config's Context.
    void loadEnvironment() noexcept;

    const char * getSearchPath() const;
    /**
     * \brief Set all search paths as a concatenated string, use ':' to separate the paths. 
     * 
     * See \ref addSearchPath for a more robust and platform-agnostic method of
     * setting the search paths.
     */
    void setSearchPath(const char * path);

    int getNumSearchPaths() const;
    /**
     * Get a search path from the list.
     * 
     * The paths are in the order they will be searched (that is, highest to
     * lowest priority).
     */
    const char * getSearchPath(int index) const;
    void clearSearchPaths();
    /**
     * \brief Add a single search path to the end of the list.
     * 
     * Paths may be either absolute or relative. Relative paths are
     * relative to the working directory. Forward slashes will be
     * normalized to reverse for Windows. Environment (context) variables
     * may be used in paths.
     */
    void addSearchPath(const char * path);

    const char * getWorkingDir() const;
    /**
     * \brief
     * 
     * The working directory defaults to the location of the
     * config file. It is used to convert any relative paths to absolute.
     * If no search paths have been set, the working directory will be used
     * as the fallback search path. No environment (context) variables may
     * be used in the working directory.
     */
    void setWorkingDir(const char * dirname);

    // 
    // ColorSpaces
    //

    /**
     * \brief Get all active color spaces having a specific category
     *     in the order they appear in the config file.
     *
     * \note
     *     If the category is null or empty, the method returns
     *     all the active color spaces like \ref Config::getNumColorSpaces
     *     and \ref Config::getColorSpaceNameByIndex do.
     *
     * \note
     *     It's worth noticing that the method returns a copy of the
     *     selected color spaces decoupling the result from the config.
     *     Hence, any changes on the config do not affect the existing
     *     color space sets, and vice-versa.
     */
    ColorSpaceSetRcPtr getColorSpaces(const char * category) const;

    /**
     * \brief Work on the color spaces selected by the reference color space type
     * and visibility.
     */
    int getNumColorSpaces(SearchReferenceSpaceType searchReferenceType,
                          ColorSpaceVisibility visibility) const;

    /**
     * \brief Work on the color spaces selected by the reference color space
     *      type and visibility (active or inactive).
     * 
     * Return empty for invalid index.
     */
    const char * getColorSpaceNameByIndex(SearchReferenceSpaceType searchReferenceType,
                                          ColorSpaceVisibility visibility, int index) const;

    /**
     * \brief Work on the active color spaces only.
     * 
     * \note
     *     Only works from the list of active color spaces.
     */
    int getNumColorSpaces() const;

    /**
     * Work on the active color spaces only and return null for invalid index.
     * 
     * \note
     *      Only works from the list of active color spaces.
     */
    const char * getColorSpaceNameByIndex(int index) const;

    /**
     * \brief Get an index from the active color spaces only
     *      and return -1 if the name is not found.
     *
     * \note
     *    The fcn accepts either a color space name, role name, or alias.
     *    (Color space names take precedence over roles.)
     */
    int getIndexForColorSpace(const char * name) const;

    /**
     * \brief Get the color space from all the color spaces
     *      (i.e. active and inactive) and return null if the name is not found.
     *
     * \note
     *     The fcn accepts either a color space name, role name, or alias.
     *     (Color space names take precedence over roles.)
     */
    ConstColorSpaceRcPtr getColorSpace(const char * name) const;

    /**
     * Accepts an alias, role name, named transform name, or color space name and returns the
     * color space name or the named transform name.
     */
    const char * getCanonicalName(const char * name) const;

    /**
     * \brief Add a color space to the configuration.
     *
     * \note
     *    If another color space is already present with the same name,
     *    this will overwrite it. This stores a copy of the specified
     *    color space.
     * \note
     *    Adding a color space to a \ref Config does not affect any
     *    \ref ColorSpaceSet sets that have already been created.
     */
    void addColorSpace(const ConstColorSpaceRcPtr & cs);

    /**
     * \brief Remove a color space from the configuration.
     *
     * \note
     *    It does not throw an exception.  Name must be the canonical name.  If a role name or
     *    alias is provided or if the name is not in the config, nothing is done.
     * \note
     *    Removing a color space from a \ref Config does not affect any
     *    \ref ColorSpaceSet sets that have already been created.
     */
    void removeColorSpace(const char * name);

    /**
     * Return true if the color space is used by a transform, a role, or a look.
     *
     * \note
     *    Name must be the canonical name.
     */
    bool isColorSpaceUsed(const char * name) const noexcept;

    /**
     * \brief Remove all the color spaces from the configuration.
     *
     * \note
     *    Removing color spaces from a \ref Config does not affect
     *    any \ref ColorSpaceSet sets that have already been created.
     */
    void clearColorSpaces();

    /**
     * \brief Set/get a list of inactive color space or named transform names.
     *
     * Notes:
     * * List can contain color space and/or named transform names.
     * * The inactive spaces are color spaces that should not appear in application menus.
     * * These color spaces will still work in Config::getProcessor calls.
     * * The argument is a comma-delimited string.  A null or empty string empties the list.
     * * The environment variable OCIO_INACTIVE_COLORSPACES may also be used to set the
     *   inactive color space list.
     * * The env. var. takes precedence over the inactive_colorspaces list in the config file.
     * * Setting the list via the API takes precedence over either the env. var. or the
     *   config file list.
     */
    void setInactiveColorSpaces(const char * inactiveColorSpaces);
    const char * getInactiveColorSpaces() const;

    /// Return true if the color space name is present in the inactive_colorspaces list.
    bool isInactiveColorSpace(const char * colorspace) const noexcept;
    
    /**
     * \brief Return true if the specified color space is linear.
     * 
     * The determination of linearity is made with respect to one of the two reference spaces 
     * (i.e., either the scene-referred one or the display-referred one). If the reference space 
     * type of the color space is the opposite of the requested reference space type, false is 
     * returned immediately rather than trying to invoke the default view transform to convert 
     * between the reference spaces.
     * 
     * Note: This function relies on heuristics that may sometimes give an incorrect result. 
     * For example, if the encoding attribute is not set appropriately or the sampled values fail 
     * to detect non-linearity.
     * 
     * The algorithm proceeds as follows:
     * -- If the color space isdata attribute is true, return false.
     * -- If the reference space type of the color space differs from the requested reference 
     *    space type, return false.
     * -- If the color space's encoding attribute is present, return true if it matches the 
     *    expected reference space type (i.e., "scene-linear" for REFERENCE_SPACE_SCENE or 
     *    "display-linear" for REFERENCE_SPACE_DISPLAY) and false otherwise.
     * -- If the color space has no to_reference or from_reference transform, return true.
     * -- Evaluate several points through the color space's transform and check if the output only 
     *    differs by a scale factor (which may be different per channel, e.g. allowing an arbitrary
     *    matrix transform, with no offset).
     * 
     * Note that the encoding test happens before the sampled value test to give config authors 
     * ultimate control over the linearity determination. For example, they could set the encoding 
     * attribute to indicate linearity if they want to ignore some areas of non-linearity 
     * (e.g., at extreme values). Or they could set it to indicate that a color space should not 
     * be considered linear, even if it is, in a mathematical sense.
     * 
     * \param colorSpace Color space to evaluate.
     * \param referenceSpaceType Evaluate linearity with respect to the specified reference space 
     *                           (either scene-referred or display-referred).
     */
    bool isColorSpaceLinear(const char * colorSpace, ReferenceSpaceType referenceSpaceType) const;

    /**
     * \brief Find the name of the color space in the source config that is the same as 
     *        a color space in the default built-in config.  For example, setting the
     *        builtinColorSpaceName to "sRGB - Texture" (a color space name from that
     *        config), would return the name for the corresponding sRGB texture space in
     *        the current config (or empty if it was not found).  Note that this method
     *        relies on heuristics which may evolve over time and which may not work on
     *        all configs.
     *
     *        The method only looks at active color spaces.  If the interchange roles are
     *        missing and heuristics are used, only scene-referred color spaces are searched.
     * 
     * \param srcConfig The config to search for the desired color space.
     * \param builtinConfig The built-in config to use.  See \ref Config::CreateFromBuiltinConfig.
     * \param builtinColorSpaceName Color space name in the built-in default config.
     * \return Matching color space name from the source config. Empty if not found.
     * 
     * \throw Exception if an interchange space cannot be found or the equivalent space cannot be found.
     */
    static const char * IdentifyBuiltinColorSpace(const ConstConfigRcPtr & srcConfig,
                                                  const ConstConfigRcPtr & builtinConfig, 
                                                  const char * builtinColorSpaceName);

    /**
     * \brief Identify the two names of a common color space that exists in both the 
     *        given config and the provided built-in config that may be used for converting
     *        color spaces between the two configs.  If both configs have the interchange
     *        role set, than the color spaces set to that role will be returned.  Otherwise,
     *        heuristics will be used to try and identify a known color space in the source
     *        config.  These are the same heuristics that are used for other methods such as
     *        identifyBuiltinColorSpace and GetProcessorTo/FromBuiltinColorSpace.
     *
     *        Using this method in connection with GetProcessorFromConfigs is more efficient
     *        if you need to call GetProcessorTo/FromBuiltinColorSpace multiple times since it 
     *        is only necessary to run the heuristics once (to identify the interchange spaces).
     *
     *        The srcColorSpaceName and builtinColorSpace name are used to decide which
     *        interchange role to use (scene- or display-referred).  However, they are not
     *        used if the interchange roles are not present and the heuristics are used.
     *        It is actually only the ReferenceSpaceType of the color spaces that are used,
     *        so it is not necessary to call this function multiple times if all the spaces
     *        are of the same type.  (These are the same arguments that would also be set if
     *        you were instead calling GetProcessorTo/FromBuiltinColorSpace.)
     * 
     * \param[out] srcInterchangeName Color space name from the source config.
     * \param[out] builtinInterchangeName Corresponding color space name from the built-in config.
     * \param srcConfig The config to search for the desired color space.
     * \param srcColorSpaceName Color space name in the given config to convert to/from.
     * \param builtinConfig The built-in config to use.  See \ref Config::CreateFromBuiltinConfig.
     * \param builtinColorSpaceName Color space name in the default built-in config.
     * 
     * \throw Exception if either the srcInterchange or builtinInterchange cannot be identified.
     */
    static void IdentifyInterchangeSpace(const char ** srcInterchangeName,
                                         const char ** builtinInterchangeName,
                                         const ConstConfigRcPtr & srcConfig,
                                         const char * srcColorSpaceName,
                                         const ConstConfigRcPtr & builtinConfig,
                                         const char * builtinColorSpaceName);

    /**
     * Methods related to Roles.
     *
     * A role allows a config author to indicate that a given color space should be used
     * for a particular purpose.  
     *
     * Role names may be passed to most functions that accept color space names, such as 
     * getColorSpace.  So for example, you may find the name of the color space assigned 
     * to the scene_linear role by getting the color space object for "scene_linear" and 
     * then calling getName on the color space object.
     */

    /**
     * \brief
     *
     * \note
     *    Setting the ``colorSpaceName`` name to a null string unsets it.
     */
    void setRole(const char * role, const char * colorSpaceName);
    int getNumRoles() const;
    /// Return true if the role has been defined.
    bool hasRole(const char * role) const;
    /**
     * \brief Get the role name at index, this will return values
     * like 'scene_linear', 'compositing_log'.
     * 
     * Return empty string if index is out of range.
     */
    const char * getRoleName(int index) const;
    /**
     * \brief Get the role color space at index.
     * 
     * Return empty string if index is out of range.
     */
    const char * getRoleColorSpace(int index) const;
    /**
     * \brief Get the color space name used for the specified role.
     * 
     * Return an empty string if the role is not present
     */
    const char * getRoleColorSpace(const char * roleName) const noexcept;

    /**
     * Methods related to displays and views.
     *
     * The following methods only manipulate active displays and views. Active
     * displays and views are defined from an env. variable or from the config file.
     *
     * Looks is a potentially comma (or colon) delimited list of lookNames,
     * Where +/- prefixes are optionally allowed to denote forward/inverse
     * look specification. (And forward is assumed in the absence of either)

     * Add shared view (or replace existing one with same name).
     * Shared views are defined at config level and can be referenced by several
     * displays. Either provide a view transform and a display color space or
     * just a color space (and a null view transform).  Looks, rule and description
     * are optional, they can be null or empty.
     *
     * Shared views using a view transform may use the token <USE_DISPLAY_NAME>
     * for the color space (see :c:var:`OCIO_VIEW_USE_DISPLAY_NAME`).  In that
     * case, when the view is referenced in a display, the display color space
     * that is used will be the one matching the display name.  In other words,
     * the view will be customized based on the display it is used in.
     * \ref Config::validate will throw if the config does not contain
     * the matching display color space.
     */

    /// Check if a view within a given display is referencing one of the config's shared views.
    bool isViewShared(const char * dispName, const char * viewName) const;

    /// Will throw if view or colorSpaceName are null or empty.
    void addSharedView(const char * view, const char * viewTransformName,
                       const char * colorSpaceName, const char * looks,
                       const char * ruleName, const char * description);
    /// Remove a shared view.  Will throw if the view does not exist.
    void removeSharedView(const char * view);

    /// Clear all shared views. This will throw if any displays are still using the shared views.
    void clearSharedViews();

    const char * getDefaultDisplay() const;
    int getNumDisplays() const;
    /// Will return "" if the index is invalid.
    const char * getDisplay(int index) const;

    const char * getDefaultView(const char * display) const;
    // Get the default view for a given color space using the viewing rules.
    // This is the preferred call to use if the color space being viewed is known.
    const char * getDefaultView(const char * display, const char * colorspaceName) const;

    /**
     * Return the number of active views attached to the display including the number of
     * shared views if any. Return 0 if display does not exist.
     */
    int getNumViews(const char * display) const;
    const char * getView(const char * display, int index) const;

    /**
     * If the config has ViewingRules, get the number of active Views for this
     * colorspace. (If there are no rules, it returns all of them.)
     */
    int getNumViews(const char * display, const char * colorspaceName) const;
    const char * getView(const char * display, const char * colorspaceName, int index) const;

    /**
     * \brief Compare views in a pair of configs.
     *
     * Will return false if either of the views does not exist. This will return true even
     * if the view is display-defined in one config and a reference to a shared view in the
     * other config (both within the same display), as long as the contents match. The
     * description text (if any) is ignored, since it is not a functional difference.
     * 
     * Note that the comparison is only on the strings contained in the view definition,
     * the function does not attempt to compare that the color spaces or view transforms
     * being referenced are identical (only that they have the same name).
     */
    static bool AreViewsEqual(const ConstConfigRcPtr & first,
                              const ConstConfigRcPtr & second,
                              const char * dispName,
                              const char * viewName);

    /**
     * Returns the view_transform attribute of the (display, view) pair. View can
     * be a shared view of the display. If display is null or empty, config shared views are used.
     */
    const char * getDisplayViewTransformName(const char * display, const char * view) const;
    /**
     * Returns the colorspace attribute of the (display, view) pair.
     * (Note that this may be either a color space or a display color space.)
     */
    const char * getDisplayViewColorSpaceName(const char * display, const char * view) const;
    /// Returns the looks attribute of a (display, view) pair.
    const char * getDisplayViewLooks(const char * display, const char * view) const;
    /// Returns the rule attribute of a (display, view) pair.
    const char * getDisplayViewRule(const char * display, const char * view) const noexcept;
    /// Returns the description attribute of a (display, view) pair.
    const char * getDisplayViewDescription(const char * display, const char * view) const noexcept;

    /**
     * \brief Determine if a display and view exist.
     *
     * This returns false if either the display or view doesn't exist. It works regardless
     * of whether the display or view are active, and it works regardless of whether the
     * view is display-defined or if the display has this as a shared view. It will only
     * check config-level shared views if dispName is null. It will not check config level
     * shared views if dispName is not null.
     */
    bool hasView(const char * dispName, const char * viewName) const;

    /**
     * For the (display, view) pair, specify which color space and look to use.
     * If a look is not desired, then just pass a null or empty string.
     */
    void addDisplayView(const char * display, const char * view,
                        const char * colorSpaceName, const char * looks);

    /**
     * \brief
     * 
     * For the (display, view) pair, specify the color space or alternatively
     * specify the view transform and display color space.  The looks, viewing rule, and
     * description are optional.  Pass a null or empty string for any optional arguments.
     * If the view already exists, it is replaced.
     *
     * Will throw if:
     * * Display, view or colorSpace are null or empty.
     * * Display already has a shared view with the same name.
     */
    void addDisplayView(const char * display, const char * view, const char * viewTransformName,
                        const char * colorSpaceName, const char * looks,
                        const char * ruleName, const char * description);

    /**
     * \brief Add a (reference to a) shared view to a display.
     * 
     * The shared view must be part of the config. See \ref Config::addSharedView
     * 
     * This will throw if:
     * * Display or view are null or empty.
     * * Display already has a view (shared or not) with the same name.
     */
    void addDisplaySharedView(const char * display, const char * sharedView);

    /**
     * \brief Remove the view and the display if no more views.
     * 
     * It does not remove the associated color space. If the view name is a
     * shared view, it only removes the reference to the view from the display
     * but the shared view, remains in the config.
     * 
     * Will throw if the view does not exist.
     */
    void removeDisplayView(const char * display, const char * view);
    /// Clear all the displays.
    void clearDisplays();

    /**
     * Methods related to the Virtual Display.
     *
     *  ...  (See descriptions for the non-virtual methods above.)
     *
     * The virtual display is the way to incorporate the ICC monitor profile for a user's display
     * into OCIO. The views that are defined for the virtual display are the views that are used to
     * create a new display for an ICC profile. They serve as a kind of template that lets OCIO
     * know how to build the new display.
     *
     * Typically the views will define a View Transform and set the colorSpaceName to 
     * "<USE_DISPLAY_NAME>" so that it will use the display color space with the same name as the
     * display, in this case corresponding to the ICC profile.
     *
     */

    /**
     * \brief Determine if a virtual view exists.
     *
     * This returns false if the virtual view doesn't exist. It works regardless of
     * whether the virtual view is active, and it works regardless of whether the virtual
     * view is display-defined or if the display has this as a shared virtual view.
     */
    bool hasVirtualView(const char * viewName) const;

    /// Check if a given virtual view is referencing one of the config's shared views.
    bool isVirtualViewShared(const char * viewName) const;

    void addVirtualDisplayView(const char * view,
                               const char * viewTransformName,
                               const char * colorSpaceName,
                               const char * looks,
                               const char * ruleName,
                               const char * description);

    void addVirtualDisplaySharedView(const char * sharedView);

    /// Get the number of views associated to the virtual display.
    int getVirtualDisplayNumViews(ViewType type) const noexcept;
    /// Get the view name at a specific index.
    const char * getVirtualDisplayView(ViewType type, int index) const noexcept;

    /**
     * \brief Compare virtual views in a pair of configs.
     *
     * Will return false if either of the virtual views does not exist. This will return true
     * even if the virtual view is display-defined in one config and a reference to a shared
     * virtual view in the other config, as long as the contents match.
     * 
     * The description text (if any) is ignored, since it is not a functional difference.
     * 
     * Note that the comparison is only on the strings contained in the view definition,
     * the function does not attempt to compare that the color spaces or view transforms
     * being referenced are identical (only that they have the same name).
     */
    static bool AreVirtualViewsEqual(const ConstConfigRcPtr & first,
                                     const ConstConfigRcPtr & second,
                                     const char * viewName);

    const char * getVirtualDisplayViewTransformName(const char * view) const noexcept;
    const char * getVirtualDisplayViewColorSpaceName(const char * view) const noexcept;
    const char * getVirtualDisplayViewLooks(const char * view) const noexcept;
    const char * getVirtualDisplayViewRule(const char * view) const noexcept;
    const char * getVirtualDisplayViewDescription(const char * view) const noexcept;

    /// Remove the view from the virtual display.
    void removeVirtualDisplayView(const char * view) noexcept;

    /// Clear the virtual display.
    void clearVirtualDisplay() noexcept;

    /**
     * \brief Instantiate a new display from a virtual display, using the monitor name.
     * 
     * This method uses the virtual display to create an actual display for the given monitorName.
     * The new display will receive the views from the virtual display.
     *
     * After the ICC profile is read, a display name will be created by combining the description
     * text from the profile with the monitorName obtained from the OS. Use the SystemMonitors class
     * to obtain the list of monitorName strings for the displays connected to the computer.
     *
     * A new display color space will also be created using the display name. It will have a
     * from_display_reference transform that is a FileTransform pointing to the ICC profile.
     *
     * Any instantiated display color spaces for a virtual display are intended to be temporary
     * (i.e. last as long as the current session). By default, they are not saved when writing a
     * config file. If there is a need to make it a permanent color space, it may be desirable to
     * copy the ICC profile somewhere under the config search_path.
     *
     * Will throw if the config does not have a virtual display or if the monitorName does not exist.
     *
     * If there is already a display or a display color space with the name monitorName, it will be
     * replaced/updated.
     *
     * Returns the index of the display.
     */
    int instantiateDisplayFromMonitorName(const char * monitorName);

    /**
     * \brief Instantiate a new display from a virtual display, using an ICC profile.
     * 
     * On platforms such as Linux, where the SystemMonitors class is not able to obtain a list of
     * ICC profiles from the OS, this method may be used to manually specify a path to an ICC profile.
     * 
     * Will throw if the virtual display definition is missing from the config.
     *
     * Returns the index of the display.
     */
    int instantiateDisplayFromICCProfile(const char * ICCProfileFilepath);

    /**
     * \brief
     * 
     * The Active Displays list allows end users, config authors, and client apps to filter and
     * reorder of the list of displays available in a user-interface. The list may be left empty
     * to indicate all displays are active.
     *
     * The first active display is the config's Default Display.
     *
     * If the active list would remove all displays from a config, it is ignored (though the
     * config won't validate).
     *
     * When serialized in the config, commas are used as separators. However, if a display name
     * contains a comma, the name will be enclosed in quotes so its comma is not a separator.
     * 
     * The OCIO_ACTIVE_DISPLAYS environment variable will override the active list specified in
     * the config file as well as any modifications made by the client app. These functions
     * only get and set what is in the config object and do not take into account the override
     * and thus may not represent the actual user experience.
     */
    /// Set all active displays at once as a comma or colon delimited string. This replaces any
    /// previous contents of the list.
    void setActiveDisplays(const char * displays);
    /// Get a string with all active displays (as it would appear in a config file).
    /// Commas are always used as the separator.
    const char * getActiveDisplays() const;
    /// Get the number of active displays.
    int getNumActiveDisplays() const;
    /// Get a single active display, by index. Returns nullptr if the index is out of range.
    const char * getActiveDisplay(int index) const;
    /// Add a single active display to the end of the list. If the display is already present,
    /// no action is taken.
    void addActiveDisplay(const char * display);
    /// Remove a single display. Will throw if the display is not present.
    void removeActiveDisplay(const char * display);
    /// Clear the active displays list.
    void clearActiveDisplays();

    /**
     * \brief
     * 
     * The Active Views list allows end users, config authors, and client apps to filter and
     * reorder of the list of views available in a user-interface. The list may be left empty
     * to indicate all views are active.
     *
     * The first active view for a display is its Default View.
     *
     * If the active list would remove all views from a display, the list is ignored for that
     * display and all views are shown for it.
     *
     * When serialized in the config, commas are used as separators. However, if a view name
     * contains a comma, the name will be enclosed in quotes so its comma is not a separator.
     * 
     * The OCIO_ACTIVE_VIEWS environment variable will override the active list specified in
     * the config file as well as any modifications made by the client app. These functions
     * only get and set what is in the config object and do not take into account the override
     * and thus may not represent the actual user experience.
     */
    /// Set all active views at once as a comma or colon delimited string. This replaces any
    /// previous contents of the list.
    void setActiveViews(const char * views);
    /// Get a string with all active views (as it would appear in a config file).
    /// Commas are always used as the separator.
    const char * getActiveViews() const;
    /// Get the number of active views.
    int getNumActiveViews() const;
    /// Get a single active view, by index. Returns nullptr if the index is out of range.
    const char * getActiveView(int index) const;
    /// Add a single active view to the end of the list. If the view is already present,
    /// no action is taken.
    void addActiveView(const char * view);
    /// Remove a single view. Will throw if the view is not present.
    void removeActiveView(const char * view);
    /// Clear the active views list.
    void clearActiveViews();

    /// Get all displays in the config, ignoring the active_displays list.
    int getNumDisplaysAll() const noexcept;
    const char * getDisplayAll(int index) const noexcept;
    int getDisplayAllByName(const char *) const noexcept;
    /**
     * Will be true for a display that was instantiated from a virtual display. These displays are
     * intended to be temporary (i.e. for the current session) and are not saved to a config file.
     */
    bool isDisplayTemporary(int index) const noexcept;
    /**
     * Allows setting the flag that controls whether a display is temporary. This may be helpful,
     * for example, to share a config with a temporary instantiated display with an OFX plug-in.
     */
    void setDisplayTemporary(int index, bool isTemporary) noexcept;

    /**
     * Get either the shared or display-defined views for a display. The
     * active_views list is ignored.  Passing a null or empty display (with type=VIEW_SHARED)
     * returns the contents of the shared_views section of the config. Return 0 if display
     * does not exist.
     */
    int getNumViews(ViewType type, const char * display) const;
    const char * getView(ViewType type, const char * display, int index) const;

    // 
    // Viewing Rules
    //

    /// Get read-only version of the viewing rules.
    ConstViewingRulesRcPtr getViewingRules() const noexcept;

    /**
     * \brief Set viewing rules.
     * 
     * \note
     *    The argument is cloned.
     */
    void setViewingRules(ConstViewingRulesRcPtr viewingRules);

    // 
    // Luma
    // ^^^^

    /**
     * \brief Get the default coefficients for computing luma.
     *
     * \note
     *    There is no "1 size fits all" set of luma coefficients. (The
     *    values are typically different for each colorspace, and the
     *    application of them may be nonsensical depending on the
     *    intensity coding anyways). Thus, the 'right' answer is to make
     *    these functions on the ColorSpace class. However, it's
     *    often useful to have a config-wide default so here it is. We will
     *    add the colorspace specific luma call if/when another client is
     *    interesting in using it.
     */
    void getDefaultLumaCoefs(double * rgb) const;
    /// These should be normalized (sum to 1.0 exactly).
    void setDefaultLumaCoefs(const double * rgb);


    // 
    // Look
    //

    // Manager per-shot look settings.

    ConstLookRcPtr getLook(const char * name) const;

    int getNumLooks() const;

    const char * getLookNameByIndex(int index) const;

    void addLook(const ConstLookRcPtr & look);

    void clearLooks();


    //
    // View Transforms
    //

    // ViewTransform objects are used with the display reference space.

    int getNumViewTransforms() const noexcept;

    ConstViewTransformRcPtr getViewTransform(const char * name) const noexcept;

    const char * getViewTransformNameByIndex(int i) const noexcept;

    void addViewTransform(const ConstViewTransformRcPtr & viewTransform);

    /**
     * \brief
     * 
     * This view transform is the one that will be used by default if a ColorSpaceTransform is
     * needed between a scene-referred and display-referred color space.  The config author may
     * specify a transform to use via the default_view_transform entry in the config.  If that is
     * not present, or does not return a valid view transform from the scene-referred connection
     * space, the fall-back is to use the first valid view transform in the config.  Returns a
     * null ConstTransformRcPtr if there isn't one.
     */
    ConstViewTransformRcPtr getDefaultSceneToDisplayViewTransform() const;

    /**
     * Get or set the default_view_transform string from the config.
     * 
     * Note that if this is not the name of a valid view transform from the scene-referred
     * connection space, it will be ignored.
     */
    const char * getDefaultViewTransformName() const noexcept;
    void setDefaultViewTransformName(const char * defaultName) noexcept;


    void clearViewTransforms();

    /**
     * Methods related to named transforms.
     */

    /**
     * \brief Work on the named transforms selected by visibility.
     */
    int getNumNamedTransforms(NamedTransformVisibility visibility) const noexcept;

    /**
     * \brief Work on the named transforms selected by visibility (active or inactive).
     *
     * Return an empty string for invalid index.
     */
    const char * getNamedTransformNameByIndex(NamedTransformVisibility visibility,
                                              int index) const noexcept;

    /// Work on the active named transforms only.
    int getNumNamedTransforms() const noexcept;

    /// Work on the active named transforms only and return an empty string for invalid index.
    const char * getNamedTransformNameByIndex(int index) const noexcept;

    /// Get an index from the active named transforms only and return -1 if the name is not found.
    int getIndexForNamedTransform(const char * name) const noexcept;

    /**
     * \brief Get the named transform from all the named transforms (i.e. active and inactive) and
     *     return null if the name is not found.
     */
    ConstNamedTransformRcPtr getNamedTransform(const char * name) const noexcept;

    /**
     * \brief Add or replace named transform.
     *
     * \note
     *    Throws if namedTransform is null, name is missing, or no transform is set.  Also throws
     *    if the name or the aliases conflict with names or aliases already in the config.
     */
    void addNamedTransform(const ConstNamedTransformRcPtr & namedTransform);

    /// Clear all named transforms.
    void clearNamedTransforms();

    // 
    // File Rules
    //

    /// Get read-only version of the file rules.
    ConstFileRulesRcPtr getFileRules() const noexcept;

    /**
     * \brief Set file rules.
     * 
     * \note
     *    The argument is cloned.
     */
    void setFileRules(ConstFileRulesRcPtr fileRules);

    /// Get the color space of the first rule that matched filePath. (For v1 configs, this is
    /// equivalent to calling parseColorSpaceFromString with strictparsing set to false.)
    const char * getColorSpaceFromFilepath(const char * filePath) const;

    /**
     * Most applications will use the preceding method, but this method may be
     * used for applications that want to know which was the highest priority rule to match
     * filePath. \ref FileRules::getNumCustomKeys and custom keys methods
     * may then be used to get additional information about the matching rule.
     */
    const char * getColorSpaceFromFilepath(const char * filePath, size_t & ruleIndex) const;

    /**
     * \brief
     * 
     * Returns true if the only rule matched by filePath is the default rule.
     * This is a convenience method for applications that want to require the user to manually
     * choose a color space when strictParsing is true and no other rules match.
     */
    bool filepathOnlyMatchesDefaultRule(const char * filePath) const;

    /**
     * Given the specified string, get the longest, right-most, colorspace substring that
     * appears.
     *
     * * If strict parsing is enabled, and no color space is found, return
     *   an empty string.
     * * If strict parsing is disabled, return ROLE_DEFAULT (if defined).
     * * If the default role is not defined, return an empty string.
     */
    OCIO_DEPRECATED("This was marked as deprecated starting in v2.0, please use Config::getColorSpaceFromFilepath().")
    const char * parseColorSpaceFromString(const char * str) const;

    bool isStrictParsingEnabled() const;
    void setStrictParsingEnabled(bool enabled);

    //
    // Processors
    //

    // Create a Processor to assemble a transformation between two
    // color spaces.  It may then be used to create a CPUProcessor
    // or GPUProcessor to process/convert pixels.

    /// Get the processor to apply a ColorSpaceTransform from a source to a destination
    /// color space.
    ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                     const ConstColorSpaceRcPtr & srcColorSpace,
                                     const ConstColorSpaceRcPtr & dstColorSpace) const;
    ConstProcessorRcPtr getProcessor(const ConstColorSpaceRcPtr & srcColorSpace,
                                     const ConstColorSpaceRcPtr & dstColorSpace) const;

    /**
     * \brief
     *
     * \note
     *    Names can be colorspace name, role name, or a combination of both.
     */
    ConstProcessorRcPtr getProcessor(const char * srcColorSpaceName,
                                     const char * dstColorSpaceName) const;
    ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                     const char * srcColorSpaceName,
                                     const char * dstColorSpaceName) const;
    
    /// Get the processor to apply a DisplayViewTransform for a display and view.  Refer to the
    /// Display/View Registration section above for more info on the display and view arguments.
    ConstProcessorRcPtr getProcessor(const char * srcColorSpaceName,
                                     const char * display,
                                     const char * view,
                                     TransformDirection direction) const;
                                     
    ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                     const char * srcColorSpaceName,
                                     const char * display,
                                     const char * view,
                                     TransformDirection direction) const;

    /// Get the processor to apply a NamedTransform in the specified direction.
    ConstProcessorRcPtr getProcessor(const ConstNamedTransformRcPtr & namedTransform,
                                     TransformDirection direction) const;
    ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                     const ConstNamedTransformRcPtr & namedTransform,
                                     TransformDirection direction) const;

    ConstProcessorRcPtr getProcessor(const char * namedTransformName,
                                     TransformDirection direction) const;
    ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                     const char * namedTransformName,
                                     TransformDirection direction) const;

    /**
     * \brief Get the processor for the specified transform.
     * 
     *  Not often needed, but will allow for the re-use of atomic OCIO
     *  functionality (such as to apply an individual LUT file).
     */
    ConstProcessorRcPtr getProcessor(const ConstTransformRcPtr & transform) const;
    ConstProcessorRcPtr getProcessor(const ConstTransformRcPtr & transform,
                                     TransformDirection direction) const;
    ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                     const ConstTransformRcPtr & transform,
                                     TransformDirection direction) const;

    /**
     * \brief Get a Processor to or from a known external color space.
     * 
     * These methods provide a way to interface color spaces in a config with known standard 
     * external color spaces. The set of external color space are those contained in the current 
     * default Built-in config. This includes common spaces such as "Linear Rec.709 (sRGB)", 
     * "sRGB - Texture", "ACEScg", and "ACES2065-1".
     * 
     * If the source config defines the necessary Interchange Role (typically "aces_interchange"), 
     * then the conversion will be well-defined and equivalent to calling GetProcessorFromConfigs
     * with the source config and the Built-in config.
     * 
     * However, if the Interchange Roles are not present, heuristics will be used to try and 
     * identify a common color space in the source config that may be used to allow the conversion 
     * to proceed. If the heuristics fail to find a suitable space, an exception is thrown. 
     * The heuristics may evolve, so the results returned by this function for a given source config 
     * and color space may change in future releases of the library. However, the Interchange Roles 
     * are required in config versions 2.2 and higher, so it is hoped that the need for the heuristics 
     * will decrease over time.
     * 
     * \param srcConfig The user's source config.
     * \param srcColorSpaceName The name of the color space in the source config.
     * \param builtinColorSpaceName The name of the color space in the current default Built-in config.
     * 
     * \throw Exception if either the src or builtin interchange space cannot be identified.
     */
    static ConstProcessorRcPtr GetProcessorToBuiltinColorSpace(ConstConfigRcPtr srcConfig,
                                                               const char * srcColorSpaceName, 
                                                               const char * builtinColorSpaceName);
    /**
     * \brief See description of GetProcessorToBuiltinColorSpace.
     * 
     * \param builtinColorSpaceName The name of the color space in the current default Built-in config.
     * \param srcConfig The user's source config.
     * \param srcColorSpaceName The name of the color space in the source config. 
     */
    static ConstProcessorRcPtr GetProcessorFromBuiltinColorSpace(const char * builtinColorSpaceName,
                                                                 ConstConfigRcPtr srcConfig,
                                                                 const char * srcColorSpaceName);

    /**
     * \brief Get a processor to convert between color spaces in two separate configs.
     * 
     * This relies on both configs having the aces_interchange role (when srcName
     * is scene-referred) or the role cie_xyz_d65_interchange (when srcName is
     * display-referred) defined.  An exception is thrown if that is not the case.
     */
    static ConstProcessorRcPtr GetProcessorFromConfigs(const ConstConfigRcPtr & srcConfig,
                                                       const char * srcColorSpaceName,
                                                       const ConstConfigRcPtr & dstConfig,
                                                       const char * dstColorSpaceName);
    static ConstProcessorRcPtr GetProcessorFromConfigs(const ConstContextRcPtr & srcContext, 
                                                       const ConstConfigRcPtr & srcConfig,
                                                       const char * srcColorSpaceName,
                                                       const ConstContextRcPtr & dstContext,
                                                       const ConstConfigRcPtr & dstConfig,
                                                       const char * dstColorSpaceName);

    /**
     * The srcInterchangeName and dstInterchangeName must refer to a pair of
     * color spaces in the two configs that are the same.  A role name may also be used.
     *
     * Note: For all of the two-config GetProcessor functions, if either the source or 
     * destination color spaces are data spaces, the entire processor will be a no-op.
     */
    static ConstProcessorRcPtr GetProcessorFromConfigs(const ConstConfigRcPtr & srcConfig,
                                                       const char * srcColorSpaceName,
                                                       const char * srcInterchangeName,
                                                       const ConstConfigRcPtr & dstConfig,
                                                       const char * dstColorSpaceName,
                                                       const char * dstInterchangeName);

    static ConstProcessorRcPtr GetProcessorFromConfigs(const ConstContextRcPtr & srcContext,
                                                       const ConstConfigRcPtr & srcConfig,
                                                       const char * srcColorSpaceName,
                                                       const char * srcInterchangeName,
                                                       const ConstContextRcPtr & dstContext,
                                                       const ConstConfigRcPtr & dstConfig,
                                                       const char * dstColorSpaceName,
                                                       const char * dstInterchangeName);

    /**
     * \brief Get a processor to convert from a color space to a display and view in
     *      two separate configs.
     */
    static ConstProcessorRcPtr GetProcessorFromConfigs(const ConstConfigRcPtr & srcConfig,
                                                       const char * srcColorSpaceName,
                                                       const ConstConfigRcPtr & dstConfig,
                                                       const char * dstDisplay,
                                                       const char * dstView,
                                                       TransformDirection direction);

    static ConstProcessorRcPtr GetProcessorFromConfigs(const ConstContextRcPtr & srcContext,
                                                       const ConstConfigRcPtr & srcConfig,
                                                       const char * srcColorSpaceName,
                                                       const ConstContextRcPtr & dstContext,
                                                       const ConstConfigRcPtr & dstConfig,
                                                       const char * dstDisplay,
                                                       const char * dstView,
                                                       TransformDirection direction);

    /**
     * The srcInterchangeName and dstInterchangeName must refer to a pair of
     * color spaces in the two configs that are the same.  A role name may also be used.
     */
    static ConstProcessorRcPtr GetProcessorFromConfigs(const ConstConfigRcPtr & srcConfig,
                                                       const char * srcColorSpaceName,
                                                       const char * srcInterchangeName,
                                                       const ConstConfigRcPtr & dstConfig,
                                                       const char * dstDisplay,
                                                       const char * dstView,
                                                       const char * dstInterchangeName,
                                                       TransformDirection direction);

    static ConstProcessorRcPtr GetProcessorFromConfigs(const ConstContextRcPtr & srcContext,
                                                       const ConstConfigRcPtr & srcConfig,
                                                       const char * srcColorSpaceName,
                                                       const char * srcInterchangeName,
                                                       const ConstContextRcPtr & dstContext,
                                                       const ConstConfigRcPtr & dstConfig,
                                                       const char * dstDisplay,
                                                       const char * dstView,
                                                       const char * dstInterchangeName,
                                                       TransformDirection direction);

    /// Get the Processor Cache flags.
    ProcessorCacheFlags getProcessorCacheFlags() const noexcept;

    /// Control the caching of processors in the config instance.  By default, caching is on.  
    /// The flags allow turning caching off entirely or only turning it off if dynamic
    /// properties are being used by the processor.
    void setProcessorCacheFlags(ProcessorCacheFlags flags) const noexcept;

    /**
     * \brief Clears this config's cache of Processor, CPUProcessor, and GPUProcessor instances. 
     * 
     * This must be done if any of the LUT files used by these Processors have been modified. 
     * Note that setProcessorCacheFlags(PROCESSOR_CACHE_OFF) turns off caching but does not clear 
     * any existing cache.
     */
    void clearProcessorCache() noexcept;

    /// Set the ConfigIOProxy object used to provision the config and LUTs from somewhere other
    /// than the file system.  (This is set on the config's embedded Context object.)
    void setConfigIOProxy(ConfigIOProxyRcPtr ciop);
    ConfigIOProxyRcPtr getConfigIOProxy() const;

    /**
     * \brief Verify if the config is archivable.
     *
     * A config is not archivable if any of the following are true:
     * -- The working directory is not set
     * -- It contains FileTransforms with a src outside the working directory
     * -- The search path contains paths outside the working directory
     * -- The search path contains paths that start with a context variable
     * 
     * Context variables are allowed but the intent is that they may only resolve to paths that
     * are within or below the working directory.  This is because the archiving function will
     * only archive files that are within the working directory in order to ensure that if it is
     * later expanded, that it will not create any files outside this directory.
     *
     * For example, a context variable on the search path intended to contain the name of a 
     * sub-directory under the working directory must have the form "./$DIR_NAME" rather than just
     * "$DIR_NAME" to be considered archivable.  This is imperfect since there is no way to
     * prevent the context variable from creating a path outside the working dir, but it should
     * at least draw attention to the fact that the archive would fail if used with context vars
     * that try to abuse the intended functionality.
     * 
     * \return bool Archivable if true.
     */
    bool isArchivable() const;

    /**
     * \brief Archive the config and its LUTs into the specified output stream.
     * 
     * The config is archived by serializing the Config object into a file named "config.ocio" and
     * then walking through the current working directory and any sub-directories.  Any files that
     * have an extension matching a supported LUT file format are added to the archive.  Any files
     * that do not have an extension (or have some unsupported LUT extension, including .ocio),
     * will not be added to the archive. To reiterate, it is the in-memory Config object that is
     * archived, and not any .ocio file in the current working directory.  The directory structure
     * relative to the working directory is preserved.  No files outside the working directory are
     * archived so that if it is later expanded, no files will be created outside the working dir.
     *
     * The reason the archive is created using all supported LUT file extensions rather than by
     * trying to resolve all the FileTransforms in the Config to specific files is because of the
     * goal to allow context variables to continue to work.
     * 
     * If a Config is created with CreateFromStream, CreateFromFile with an OCIOZ archive, or 
     * CreateFromConfigIOProxy, it cannot be archived unless the working directory is manually set 
     * to a directory that contains any necessary LUT files. 
     * 
     * The provided output stream must be closed by the caller, if necessary (e.g., an ofstream).
     *
     * \param ostream The output stream to write to.
     */
    void archive(std::ostream & ostream) const;

    Config(const Config &) = delete;
    Config& operator= (const Config &) = delete;

    /// Do not use (needed only for pybind11).
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


/**
 * \brief
 * The File Rules are a set of filepath to color space mappings that are evaluated
 * from first to last. The first rule to match is what determines which color space is
 * returned. There are four types of rules available. Each rule type has a name key that may
 * be used by applications to refer to that rule. Name values must be unique i.e. using a
 * case insensitive comparison. The other keys depend on the rule type:
 *
 * * *Basic Rule*: This is the basic rule type that uses Unix glob style pattern matching and
 *   is thus very easy to use. It contains the keys:
 *     * name: Name of the rule
 *     * colorspace: Color space name to be returned.
 *     * pattern: Glob pattern to be used for the main part of the name/path.
 *     * extension: Glob pattern to be used for the file extension. Note that if glob tokens
 *       are not used, the extension will be used in a non-case-sensitive way by default.
 * 
 * * *Regex Rule*: This is similar to the basic rule but allows additional capabilities for
 *   power-users. It contains the keys:
 *     * name: Name of the rule
 *     * colorspace: Color space name to be returned.
 *     * regex: Regular expression to be evaluated.
 * 
 * * *OCIO v1 style Rule*: This rule allows the use of the OCIO v1 style, where the string
 *   is searched for color space names from the config. This rule may occur 0 or 1 times
 *   in the list. The position in the list prioritizes it with respect to the other rules.
 *   StrictParsing is not used. If no color space is found in the path, the rule will not
 *   match and the next rule will be considered.
 *   see \ref insertPathSearchRule.
 *   It has the key:
 *     * name: Must be "ColorSpaceNamePathSearch".
 * 
 * * *Default Rule*: The file_rules must always end with this rule. If no prior rules match,
 *   this rule specifies the color space applications will use.
 *   see \ref setDefaultRuleColorSpace.
 *   It has the keys:
 *     * name: must be "Default".
 *     * colorspace : Color space name to be returned.
 *
 * Custom string keys and associated string values may be used to convey app or
 * workflow-specific information, e.g. whether the color space should be left as is
 * or converted into a working space.
 *
 * Getters and setters are using the rule position, they will throw if the position is not
 * valid. If the rule at the specified position does not implement the requested property
 * getter will return NULL and setter will throw.
 *
 * When loading a v1 config, a set of FileRules are created with ColorSpaceNamePathSearch followed
 * by the Default rule pointing to the default role. This allows getColorSpaceFromFilepath to emulate
 * OCIO v1 code that used parseColorSpaceFromString with strictparsing set to false.
 */

class OCIOEXPORT FileRules
{
public:

    /// Reserved rule name for the default rule.
    static const char * DefaultRuleName;
    /// Reserved rule name for the file path search rule \see FileRules::insertPathSearchRule.
    static const char * FilePathSearchRuleName;

    /**
     * Creates FileRules for a Config. File rules will contain the default rule
     * using the default role. The default rule cannot be removed.
     */
    static FileRulesRcPtr Create();

    /// The method clones the content decoupling the two instances.
    FileRulesRcPtr createEditableCopy() const;

    /// Does include default rule. Result will be at least 1.
    size_t getNumEntries() const noexcept;

    /// Get the index from the rule name.
    size_t getIndexForRule(const char * ruleName) const;

    /// Get name of the rule.
    const char * getName(size_t ruleIndex) const;

    /// Setting pattern will erase regex.
    const char * getPattern(size_t ruleIndex) const;
    void setPattern(size_t ruleIndex, const char * pattern);

    /// Setting extension will erase regex.
    const char * getExtension(size_t ruleIndex) const;
    void setExtension(size_t ruleIndex, const char * extension);

    /// Setting a regex will erase pattern & extension.
    const char * getRegex(size_t ruleIndex) const;
    void setRegex(size_t ruleIndex, const char * regex);

    /// Set the rule's color space (may also be a role).
    const char * getColorSpace(size_t ruleIndex) const;
    void setColorSpace(size_t ruleIndex, const char * colorSpace);

    /// Get number of key/value pairs.
    size_t getNumCustomKeys(size_t ruleIndex) const;
    /// Get name of key.
    const char * getCustomKeyName(size_t ruleIndex, size_t key) const;
    /// Get value for the key.
    const char * getCustomKeyValue(size_t ruleIndex, size_t key) const;
    /**
     * Adds a key/value or replace value if key exists. Setting a NULL or an
     * empty value will erase the key.
     */
    void setCustomKey(size_t ruleIndex, const char * key, const char * value);

    /**
     * \brief Insert a rule at a given ruleIndex.
     * 
     * Rule currently at ruleIndex will be pushed to index: ruleIndex + 1.
     * Name must be unique.
     * - "Default" is a reserved name for the default rule. The default rule is automatically
     * added and can't be removed. (see \ref FileRules::setDefaultRuleColorSpace ).
     * - "ColorSpaceNamePathSearch" is also a reserved name
     * (see \ref FileRules::insertPathSearchRule ).
     *
     * Will throw if pattern, extension or regex is a null or empty string.
     *
     * Will throw if ruleIndex is not less than \ref FileRules::getNumEntries .
     */
    void insertRule(size_t ruleIndex, const char * name, const char * colorSpace,
                    const char * pattern, const char * extension);
    void insertRule(size_t ruleIndex, const char * name, const char * colorSpace,
                    const char * regex);
    /**
     * \brief Helper function to insert a rule. 
     * 
     * Uses \ref Config:parseColorSpaceFromString to search the path for any of
     * the color spaces named in the config (as per OCIO v1).
     */
    void insertPathSearchRule(size_t ruleIndex);
    /// Helper function to set the color space for the default rule.
    void setDefaultRuleColorSpace(const char * colorSpace);

    /**
     * \brief
     * 
     * \note
     *      Default rule can't be removed.
     * Will throw if ruleIndex + 1 is not less than \ref FileRules::getNumEntries .
     */
    void removeRule(size_t ruleIndex);

    /// Move a rule closer to the start of the list by one position.
    void increaseRulePriority(size_t ruleIndex);

    /// Move a rule closer to the end of the list by one position.
    void decreaseRulePriority(size_t ruleIndex);

    /**
     * Check if there is only the default rule using default role and no custom key. This is the
     * default FileRules state when creating a new config.
     */
    bool isDefault() const noexcept;

    FileRules(const FileRules &) = delete;
    FileRules & operator= (const FileRules &) = delete;

    /// Do not use (needed only for pybind11).
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

extern OCIOEXPORT std::ostream & operator<< (std::ostream &, const FileRules &);



/**
 * ViewingRules
 * 
 * Viewing Rules allow config authors to filter the list of views an application should offer
 * based on the color space of an image.  For example, a config may define a large number of
 * views but not all of them may be appropriate for use with all color spaces.  E.g., some views
 * may be intended for use with scene-linear color space encodings and others with video color
 * space encodings.
 * 
 * Each rule has a name key for applications to refer to the rule.  Name values must be unique
 * (using case insensitive comparison). Viewing Rules may also have the following keys:
 *
 * * colorspaces: Either a single colorspace name or a list of names.
 *
 * * encodings: One or more strings to be found in the colorspace's encoding attribute.
 *   Either this attribute or colorspaces must be present, but not both.
 *
 * * custom : Allows arbitrary key / value string pairs, similar to FileRules.
 *
 * Getters and setters are using the rule position, they will throw if the position is not
 * valid.
*/
class OCIOEXPORT ViewingRules
{
public:
    /// Creates ViewingRules for a Config.
    static ViewingRulesRcPtr Create();

    /// The method clones the content decoupling the two instances.
    ViewingRulesRcPtr createEditableCopy() const;

    size_t getNumEntries() const noexcept;

    /**
     * Get the index from the rule name. Will throw if there is no rule named
     * ruleName.
     */
    size_t getIndexForRule(const char * ruleName) const;

    /// Get name of the rule. Will throw if ruleIndex is invalid.
    const char * getName(size_t ruleIndex) const;

    /// Get number of colorspaces. Will throw if ruleIndex is invalid.
    size_t getNumColorSpaces(size_t ruleIndex) const;
    /// Get colorspace name. Will throw if ruleIndex or colorSpaceIndex is invalid.
    const char * getColorSpace(size_t ruleIndex, size_t colorSpaceIndex) const;
    /**
     * \brief
     * 
     * Add colorspace name. Will throw if:
     * * RuleIndex is invalid.
     * * \ref ViewingRules::getNumEncodings is not 0.
     */
    void addColorSpace(size_t ruleIndex, const char * colorSpace);
    /// Remove colorspace. Will throw if ruleIndex or colorSpaceIndex is invalid.
    void removeColorSpace(size_t ruleIndex, size_t colorSpaceIndex);

    /// Get number of encodings. Will throw if ruleIndex is invalid.
    size_t getNumEncodings(size_t ruleIndex) const;
    /// Get encoding name. Will throw if ruleIndex or encodingIndex is invalid.
    const char * getEncoding(size_t ruleIndex, size_t encodingIndex) const;

    /**
     * \brief
     * Add encoding name. Will throw if:
     * * RuleIndex is invalid.
     * * \ref ViewingRules::getNumColorSpaces is not 0.
     */
    void addEncoding(size_t ruleIndex, const char * encoding);
    /// Remove encoding. Will throw if ruleIndex or encodingIndex is invalid.
    void removeEncoding(size_t ruleIndex, size_t encodingIndex);

    /// Get number of key/value pairs. Will throw if ruleIndex is invalid.
    size_t getNumCustomKeys(size_t ruleIndex) const;
    /// Get name of key. Will throw if ruleIndex or keyIndex is invalid.
    const char * getCustomKeyName(size_t ruleIndex, size_t keyIndex) const;
    /// Get value for the key. Will throw if ruleIndex or keyIndex is invalid.
    const char * getCustomKeyValue(size_t ruleIndex, size_t keyIndex) const;
    /**
     * Adds a key/value or replace value if key exists. Setting a NULL or an
     * empty value will erase the key. Will throw if ruleIndex is invalid.
     */
    void setCustomKey(size_t ruleIndex, const char * key, const char * value);

    /**
     * \brief Insert a rule at a given ruleIndex.
     * 
     * Rule currently at ruleIndex will be pushed to index: ruleIndex + 1. If ruleIndex is 
     * \ref ViewingRules::getNumEntries, a new rule will be added at the end. Will throw if:
     * * RuleIndex is invalid (must be less than or equal to
     *   \ref ViewingRules::getNumEntries).
     * * RuleName already exists.
     */
    void insertRule(size_t ruleIndex, const char * ruleName);

    /// Remove a rule. Throws if ruleIndex is not valid.
    void removeRule(size_t ruleIndex);

    ViewingRules(const ViewingRules &) = delete;
    ViewingRules & operator= (const ViewingRules &) = delete;
    /// Do not use (needed only for pybind11).
    virtual ~ViewingRules();

private:
    ViewingRules();

    static void deleter(ViewingRules* c);

    friend class Config;

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

extern OCIOEXPORT std::ostream & operator<< (std::ostream &, const ViewingRules &);

//
// ColorSpace
//
/**
 * The *ColorSpace* is the state of an image with respect to colorimetry
 * and color encoding. Transforming images between different
 * *ColorSpaces* is the primary motivation for this library.
 *
 * While a complete discussion of color spaces is beyond the scope of
 * header documentation, traditional uses would be to have *ColorSpaces*
 * corresponding to: physical capture devices (known cameras, scanners),
 * and internal 'convenience' spaces (such as scene linear, logarithmic).
 */
class OCIOEXPORT ColorSpace
{
public:
    static ColorSpaceRcPtr Create();

    static ColorSpaceRcPtr Create(ReferenceSpaceType referenceSpace);

    ColorSpaceRcPtr createEditableCopy() const;

    const char * getName() const noexcept;
    /// If the name is already an alias, that alias is removed.
    void setName(const char * name) noexcept;

    size_t getNumAliases() const noexcept;
    /// Return empty string if idx is out of range.
    const char * getAlias(size_t idx) const noexcept;
    /// Return true if alias exists.
    bool hasAlias(const char * alias) const noexcept;
    /**
     * Add an alias for the color space name (the aliases may be used as a synonym for the
     * name).  Nothing will be added if the alias is already the color space name, one of its
     * aliases, or the argument is null.  The aliases must not conflict with existing roles,
     * color space names, named transform names, or other aliases.  This is verified when
     * adding the color space to the config.
     */
    void addAlias(const char * alias) noexcept;
    /// Does nothing if alias is not present.
    void removeAlias(const char * alias) noexcept;
    void clearAliases() noexcept;

    /**
     * Get the family, for use in user interfaces (optional)
     * The family string could use a '/' separator to indicate levels to be used
     * by hierarchical menus.
     */
    const char * getFamily() const noexcept;
    /// Set the family, for use in user interfaces (optional)
    void setFamily(const char * family);

    /**
     * Get the ColorSpace group name (used for equality comparisons)
     * This allows no-op transforms between different colorspaces.
     * If an equalityGroup is not defined (an empty string), it will be considered
     * unique (i.e., it will not compare as equal to other ColorSpaces with an
     * empty equality group).
     */
    const char * getEqualityGroup() const noexcept;
    void setEqualityGroup(const char * equalityGroup);

    const char * getDescription() const noexcept;
    void setDescription(const char * description);

    /**
     * Get/Set the interop ID for the color space. The interop ID is a
     * structured string defined by the Color Interop Forum. It is intended to
     * identify color spaces in a way that is portable across different configs,
     * making it suitable for use in various file formats. The Color Interop
     * Forum publishes ID strings for common color spaces. If you create your
     * own IDs, they must be preceded by a namespace string. The setter will
     * throw if the string does not follow certain rules (run ociocheck for a
     * more complete validation).
     */
    const char * getInteropID() const noexcept;
    void setInteropID(const char * interopID);

    /**
     * Get/Set the interchange attributes.
     *
     * Currently supported attribute names are "amf_transform_ids" and
     * "icc_profile_name". Using any other name will throw. If the attribute is
     * not defined, it'll return an empty string. Setting the value to an empty
     * string will effectively delete the attribute.
     *
     * The AMF transform IDs are used to identify specific transforms in the
     * ACES Metadata File. Multiple transform IDs can be specified in a
     * newline-separated string.
     *
     * The ICC profile name identifies the ICC color profile associated with
     * this color space. This can be used to link OCIO color spaces with
     * corresponding ICC profiles for applications that need to work with both
     * color management systems.
     */
    const char *getInterchangeAttribute(const char *attrName) const;
    void setInterchangeAttribute(const char* attrName, const char *value);
    std::map<std::string, std::string> getInterchangeAttributes() const noexcept;

    BitDepth getBitDepth() const noexcept;
    void setBitDepth(BitDepth bitDepth);

    /// A display color space will use the display-referred reference space.
    ReferenceSpaceType getReferenceSpaceType() const noexcept;

    //
    // Categories
    //

     /**
     * A category is used to allow applications to filter the list of color spaces
     * they display in menus based on what that color space is used for.
     *
     * Here is an example config entry that could appear under a ColorSpace:
     * 
     * \code{.yaml}
     *     categories: [ file-io, working-space, basic-3d ]
     * \endcode
     * 
     * The example contains three categories: 'file-io', 'working-space' and 'basic-3d'.
     * 
     * \note
     *     Category strings are not case-sensitive and the order is not significant.
     * 
     * There is no limit imposed on length or number. Although users may add their own categories,
     * the strings will typically come from a fixed set listed in the documentation (similar to 
     * roles).
     */
    /// Return true if the category is present.
    bool hasCategory(const char * category) const;
    /**
     * \brief Add a single category.
     * 
     * \note
     *     Will do nothing if the category already exists.
     */
    void addCategory(const char * category);
    /**
     * \brief Remove a category.
     * 
     * \note 
     *     Will do nothing if the category is missing.
     */
    void removeCategory(const char * category);
    /// Get the number of categories.
    int getNumCategories() const;
    /**
     * \brief Return the category name using its index
     * 
     * \note
     *     Will be null if the index is invalid.
     */
    const char * getCategory(int index) const;
    /// Clear all the categories.
    void clearCategories();

    /**
     * *Encodings*
     *
     * It is sometimes useful for applications to group color spaces based on how the color values
     * are digitally encoded.  For example, images in scene-linear, logarithmic, video, and data
     * color spaces could have different default views.  Unlike the Family and EqualityGroup
     * attributes of a color space, the list of Encodings is predefined in the OCIO documentation
     * (rather than being config-specific) to make it easier for applications to utilize.
     *
     * Here is an example config entry that could appear under a ColorSpace:
     * 
     * \code{.yaml}
     *     encoding: scene-linear
     * \endcode
     *
     * Encoding strings are not case-sensitive. Although users may add their own encodings, the
     * strings will typically come from a fixed set listed in the documentation (similar to roles).
     */
    const char * getEncoding() const noexcept;
    void setEncoding(const char * encoding);

    /**
     * *Data*
     * 
     * ColorSpaces that are data are treated a bit special. Basically, any colorspace transforms
     * you try to apply to them are ignored. (Think of applying a gamut mapping transform to an
     * ID pass). However, the setDataBypass method on ColorSpaceTransform and DisplayViewTransform
     * allow applications to process data when necessary.  (Think of sending mattes to an HDR
     * monitor.)
     *
     * This is traditionally used for pixel data that represents non-color
     * pixel data, such as normals, point positions, ID information, etc.
     */
    bool isData() const noexcept;
    void setIsData(bool isData) noexcept;

    /**
     * *Allocation*
     * 
     * If this colorspace needs to be transferred to a limited dynamic
     * range coding space (such as during display with a GPU path), use this
     * allocation to maximize bit efficiency.
     */
    Allocation getAllocation() const noexcept;
    void setAllocation(Allocation allocation) noexcept;

    /**
     * Specify the optional variable values to configure the allocation.
     * If no variables are specified, the defaults are used.
     *
     * ALLOCATION_UNIFORM::
     *
     *    2 vars: [min, max]
     *
     * ALLOCATION_LG2::
     *
     *    2 vars: [lg2min, lg2max]
     *    3 vars: [lg2min, lg2max, linear_offset]
     */
    int getAllocationNumVars() const;
    void getAllocationVars(float * vars) const;
    void setAllocationVars(int numvars, const float * vars);

    /**
     * *Transform*
     * 
     * If a transform in the specified direction has been specified,
     * return it. Otherwise return a null ConstTransformRcPtr
     */
    ConstTransformRcPtr getTransform(ColorSpaceDirection dir) const noexcept;
    /**
     * Specify the transform for the appropriate direction.
     * Setting the transform to null will clear it.
     */
    void setTransform(const ConstTransformRcPtr & transform, ColorSpaceDirection dir);

    ColorSpace(const ColorSpace &) = delete;
    ColorSpace& operator= (const ColorSpace &) = delete;
    /// Do not use (needed only for pybind11).
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




//
// ColorSpaceSet
//


/**
 * The *ColorSpaceSet* is a set of color spaces (i.e. no color space duplication)
 * which could be the result of \ref Config::getColorSpaces
 * or built from scratch.
 *
 * \note
 *    The color spaces are decoupled from the config ones, i.e., any
 *    changes to the set itself or to its color spaces do not affect the
 *    original color spaces from the configuration.  If needed,
 *    use \ref Config::addColorSpace to update the configuration.
 */
class OCIOEXPORT ColorSpaceSet
{
public:
    /// Create an empty set of color spaces.
    static ColorSpaceSetRcPtr Create();

    /// Create a set containing a copy of all the color spaces.
    ColorSpaceSetRcPtr createEditableCopy() const;

    /**
     * \brief Return true if the two sets are equal.
     * 
     * \note
     *    The comparison is done on the color space names (not a deep comparison).
     */
    bool operator==(const ColorSpaceSet & css) const;
    /// Return true if the two sets are different.
    bool operator!=(const ColorSpaceSet & css) const;

    /// Return the number of color spaces.
    int getNumColorSpaces() const;
    /**
     * Return the color space name using its index.
     * This will be null if an invalid index is specified.
     */
    const char * getColorSpaceNameByIndex(int index) const;
    /**
     * Return the color space using its index.
     * This will be empty if an invalid index is specified.
     */
    ConstColorSpaceRcPtr getColorSpaceByIndex(int index) const;

    /**
     * \brief
     * 
     * \note
     *   Only accepts color space names (i.e. no role name).
     * 
     * Will return null if the name is not found.
     */
    ConstColorSpaceRcPtr getColorSpace(const char * name) const;
    /**
     * Will return -1 if the name is not found.
     * 
     * \note
     *    Only accepts color space names (i.e. no role name).
     */
    int getColorSpaceIndex(const char * name) const;
    /**
     * \brief 
     * 
     * \note
     *     Only accepts color space names (i.e. no role name)
     * 
     * \param name 
     * \return true 
     * \return false 
     */
    bool hasColorSpace(const char * name) const;

    /**
     * \brief Add color space(s).
     *
     * \note
     *    If another color space is already registered with the same name,
     *    this will overwrite it. This stores a copy of the specified
     *    color space(s). Throws if one of the aliases is already assigned as
     *    a name or alias to an existing color space.
     */
    void addColorSpace(const ConstColorSpaceRcPtr & cs);
    void addColorSpaces(const ConstColorSpaceSetRcPtr & cs);

    /**
     * \brief Remove color space(s) using color space names (i.e. no role name).
     *
     * \note
     *    The removal of a missing color space does nothing.
     */
    void removeColorSpace(const char * name);
    void removeColorSpaces(const ConstColorSpaceSetRcPtr & cs);

    /// Clear all color spaces.
    void clearColorSpaces();

    /// Do not use (needed only for pybind11).
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

/** ColorSpaceSetOperators
 */

/**
 * \brief Perform the union of two sets.
 * 
 * \note
 *      This function provides operations on two color space sets
 *      where the result contains copied color spaces and no duplicates.
 * 
 * \param lcss 
 * \param rcss 
 */
extern OCIOEXPORT ConstColorSpaceSetRcPtr operator||(const ConstColorSpaceSetRcPtr & lcss,
                                                     const ConstColorSpaceSetRcPtr & rcss);
 /**
  * \brief Perform the intersection of two sets.
  * 
  * \note
  *      This function provides operations on two color space sets
  *      where the result contains copied color spaces and no duplicates.
  *
  * \param lcss 
  * \param rcss 
 */
extern OCIOEXPORT ConstColorSpaceSetRcPtr operator&&(const ConstColorSpaceSetRcPtr & lcss,
                                                     const ConstColorSpaceSetRcPtr & rcss);
/**
 * \brief Perform the difference of two sets. 
 * 
 * \note
 *      This function provides operations on two color space sets
 *      where the result contains copied color spaces and no duplicates.
 *
 * \param lcss 
 * \param rcss 
 */
extern OCIOEXPORT ConstColorSpaceSetRcPtr operator-(const ConstColorSpaceSetRcPtr & lcss,
                                                    const ConstColorSpaceSetRcPtr & rcss);


//
// Look
//

/**
 * The *Look* is an 'artistic' image modification, in a specified image
 * state.
 * The processSpace defines the ColorSpace the image is required to be
 * in, for the math to apply correctly.
 */
class OCIOEXPORT Look
{
public:
    static LookRcPtr Create();

    LookRcPtr createEditableCopy() const;

    const char * getName() const;
    void setName(const char * name);

    const char * getProcessSpace() const;
    void setProcessSpace(const char * processSpace);

    ConstTransformRcPtr getTransform() const;
    /// Setting a transform to a non-null call makes it allowed.
    void setTransform(const ConstTransformRcPtr & transform);

    ConstTransformRcPtr getInverseTransform() const;
    /// Setting a transform to a non-null call makes it allowed.
    void setInverseTransform(const ConstTransformRcPtr & transform);

    const char * getDescription() const;
    void setDescription(const char * description);

    Look(const Look &) = delete;
    Look& operator= (const Look &) = delete;
    /// Do not use (needed only for pybind11).
    ~Look();

private:
    Look();

    static void deleter(Look* c);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Look&);


/**
 * \brief NamedTransform.
 *
 * A NamedTransform provides a way for config authors to include a set of color
 * transforms that are independent of the color space being processed.  For example a "utility
 * curve" transform where there is no need to convert to or from a reference space.
 */

class OCIOEXPORT NamedTransform
{
public:
    static NamedTransformRcPtr Create();

    virtual NamedTransformRcPtr createEditableCopy() const = 0;

    virtual const char * getName() const noexcept = 0;
    virtual void setName(const char * name) noexcept = 0;

    /// Aliases can be used instead of the name. They must be unique within the config.
    virtual size_t getNumAliases() const noexcept = 0;
    /// Return empty string if idx is  out of range.
    virtual const char * getAlias(size_t idx) const noexcept = 0;
    /// Return true if alias exists.
    virtual bool hasAlias(const char* alias) const noexcept = 0;
    /**
    * Nothing is done if alias is NULL or empty, if it is already there, or if it is already
    * the named transform name.
    */
    virtual void addAlias(const char * alias) noexcept = 0;
    /// Does nothing if alias is not present.
    virtual void removeAlias(const char * alias) noexcept = 0;
    virtual void clearAliases() noexcept = 0;

    /// \see ColorSpace::getFamily
    virtual const char * getFamily() const noexcept = 0;
    /// \see ColorSpace::setFamily
    virtual void setFamily(const char * family) noexcept = 0;

    virtual const char * getDescription() const noexcept = 0;
    virtual void setDescription(const char * description) noexcept = 0;

    /// \see ColorSpace::hasCategory
    virtual bool hasCategory(const char * category) const noexcept = 0;
    /// \see ColorSpace::addCategory
    virtual void addCategory(const char * category) noexcept = 0;
    /// \see ColorSpace::removeCategory
    virtual void removeCategory(const char * category) noexcept = 0;
    /// \see ColorSpace::getNumCategories
    virtual int getNumCategories() const noexcept = 0;
    /// \see ColorSpace::getCategory
    virtual const char * getCategory(int index) const noexcept = 0;
    /// \see ColorSpace::clearCategories
    virtual void clearCategories() noexcept = 0;

    /**
     * A NamedTransform is not a color space and does not have an encoding in the same sense.
     * However, it may be useful to associate a color space encoding that the transform is intended
     * to be used with, for organizational purposes.
     */
    virtual const char * getEncoding() const noexcept = 0;
    virtual void setEncoding(const char * encoding) noexcept = 0;

    virtual ConstTransformRcPtr getTransform(TransformDirection dir) const = 0;
    virtual void setTransform(const ConstTransformRcPtr & transform, TransformDirection dir) = 0;

    /**
     * Will create the transform from the inverse direction if the transform for requested
     * direction is missing.
     */
    static ConstTransformRcPtr GetTransform(const ConstNamedTransformRcPtr & nt,
                                            TransformDirection dir);

    NamedTransform(const NamedTransform &) = delete;
    NamedTransform & operator= (const NamedTransform &) = delete;
    // Do not use (needed only for pybind11).
    virtual ~NamedTransform() = default;

protected:
    NamedTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<< (std::ostream &, const NamedTransform &);


/**
 * A *ViewTransform* provides a conversion from the main (usually scene-referred) reference space
 * to the display-referred reference space.  This allows splitting the conversion from the main
 * reference space to a display into two parts: the ViewTransform plus a display color space.
 *
 * It is also possible to provide a ViewTransform that converts from the display-referred
 * reference space back to that space.  This is useful in cases when a ViewTransform is needed
 * when converting between displays (such as HDR to SDR).
 *
 * The ReferenceSpaceType indicates whether the ViewTransform converts from scene-to-display
 * reference or display-to-display reference.
 *
 * The from_reference transform direction is the one that is used when going out towards a display.
 */
class OCIOEXPORT ViewTransform
{
public:
    static ViewTransformRcPtr Create(ReferenceSpaceType referenceSpace);

    ViewTransformRcPtr createEditableCopy() const;

    const char * getName() const noexcept;
    void setName(const char * name) noexcept;

    /// \see ColorSpace::getFamily
    const char * getFamily() const noexcept;
    /// \see ColorSpace::setFamily
    void setFamily(const char * family);

    const char * getDescription() const noexcept;
    void setDescription(const char * description);

    /// \see ColorSpace::hasCategory
    bool hasCategory(const char * category) const;
    /// \see ColorSpace::addCategory
    void addCategory(const char * category);
    /// \see ColorSpace::removeCategory
    void removeCategory(const char * category);
    /// \see ColorSpace::getNumCategories
    int getNumCategories() const;
    /// \see ColorSpace::getCategory
    const char * getCategory(int index) const;
    /// \see ColorSpace::clearCategories
    void clearCategories();

    ReferenceSpaceType getReferenceSpaceType() const noexcept;

    /**
     * If a transform in the specified direction has been specified, return it.
     * Otherwise return a null ConstTransformRcPtr
     */
    ConstTransformRcPtr getTransform(ViewTransformDirection dir) const noexcept;

    /**
     * Specify the transform for the appropriate direction. Setting the transform
     * to null will clear it.
     */
    void setTransform(const ConstTransformRcPtr & transform, ViewTransformDirection dir);

    ViewTransform(const ViewTransform &) = delete;
    ViewTransform & operator= (const ViewTransform &) = delete;
    /// Do not use (needed only for pybind11).
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

//
// Processor
//


/**
 * The *Processor* represents a specific color transformation which is
 * the result of \ref Config::getProcessor.
 */
class OCIOEXPORT Processor
{
public:
    bool isNoOp() const;

    /**
     * True if the image transformation is non-separable.
     * For example, if a change in red may also cause a change in green or blue.
     */
    bool hasChannelCrosstalk() const;

    /**
     * Returns a hash string generated by hashing the cachedIDs of the (unoptimized) list of ops
     * contained in the Processor. (This forms part of the key used by the config's processor cache.)
     */
    const char * getCacheID() const;

    /**
     * The ProcessorMetadata contains technical information
     * such as the number of files and looks used in the processor.
     */
    ConstProcessorMetadataRcPtr getProcessorMetadata() const;

    /**
     * Get a FormatMetadata containing the top level metadata
     * for the processor.  For a processor from a CLF file, this corresponds to
     * the ProcessList metadata.
     */
    const FormatMetadata & getFormatMetadata() const;

    /**
     * Get the number of transforms that comprise the processor.
     * Each transform has a (potentially empty) FormatMetadata.
     */
    int getNumTransforms() const;
    /**
     * Get a FormatMetadata containing the metadata for a
     * transform within the processor. For a processor from a CLF file, this 
     * corresponds to the metadata associated with an individual process node.
     */
    const FormatMetadata & getTransformFormatMetadata(int index) const;

    /**
     * Return a \ref GroupTransform that contains a copy of the transforms that comprise the
     * processor. (Changes to it will not modify the original processor.) Note that the
     * GroupTransform::write method may be used to serialize a Processor.  Serializing to
     * CTF format is a useful technique for debugging Processor contents.
     */
    GroupTransformRcPtr createGroupTransform() const;

    /**
     * The returned pointer may be used to set the default value of any dynamic
     * properties of the requested type.  Throws if the requested property is not found.  Note
     * that if the processor contains several ops that support the requested property, only one
     * can be dynamic and only this one will be controlled.
     *
     * \note The dynamic properties are a convenient way to change on-the-fly values without 
     * generating again and again a CPU or GPU processor instance. Color transformations can
     * contain dynamic properties from a ExposureContrastTransform for example.
     * So, Processor, CPUProcessor and GpuShaderCreator all have ways to manage dynamic
     * properties. However, the transform dynamic properties are decoupled between the types
     * of processor instances so that the same Processor can generate several independent CPU
     * and/or GPU processor instances i.e. changing the value of the exposure dynamic property
     * from a CPU processor  instance does not affect the corresponding GPU processor instance.
     * Processor creation will log a warning if there are more than one property of a given type.
     * There may be more than one property of a given type, but only one will respond to parameter
     * updates, the others will use their original parameter values.
     */
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;
    /// True if at least one dynamic property of that type exists.
    bool hasDynamicProperty(DynamicPropertyType type) const noexcept;
    /// True if at least one dynamic property of any type exists and is dynamic.
    bool isDynamic() const noexcept;

    /**
     * Run the optimizer on a Processor to create a new Processor.
     * It is usually not necessary to call this since getting a CPUProcessor or GPUProcessor
     * will also optimize.  However if you need both, calling this method first makes getting
     * a CPU and GPU Processor faster since the optimization is effectively only done once.
     */
    ConstProcessorRcPtr getOptimizedProcessor(OptimizationFlags oFlags) const;

    /**
     * Create a Processor that is optimized for a specific in and out bit-depth (as CPUProcessor 
     * would do).  This method is provided primarily for diagnostic purposes.
     */
    ConstProcessorRcPtr getOptimizedProcessor(BitDepth inBD, BitDepth outBD,
                                              OptimizationFlags oFlags) const;

    //
    // GPU Renderer
    //

    /// Get an optimized GPUProcessor instance.
    ConstGPUProcessorRcPtr getDefaultGPUProcessor() const;
    ConstGPUProcessorRcPtr getOptimizedGPUProcessor(OptimizationFlags oFlags) const;

    /** 
     * Get an optimized GPUProcessor instance that will emulate the OCIO v1 GPU path. This approach
     * bakes some of the ops into a single Lut3D and so is less accurate than the current GPU
     * processing methods.
     */
    ConstGPUProcessorRcPtr getOptimizedLegacyGPUProcessor(OptimizationFlags oFlags, 
                                                          unsigned edgelen) const;

    //
    // CPU Renderer
    //

    /**
     * Get an optimized CPUProcessor instance.
     *
     * \note
     *    This may provide higher fidelity than anticipated due to internal
     *    optimizations. For example, if the inputColorSpace and the
     *    outputColorSpace are members of the same equalitygroup, no conversion
     *    will be applied, even though strictly speaking quantization
     *    should be added.
     *
     * \note
     *    The typical use case to apply color processing to an image is:
     *
     * \code{.cpp}
     *
     *     OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
     *
     *     OCIO::ConstProcessorRcPtr processor
     *         = config->getProcessor(colorSpace1, colorSpace2);
     *
     *     OCIO::ConstCPUProcessorRcPtr cpuProcessor
     *         = processor->getDefaultCPUProcessor();
     *
     *     OCIO::PackedImageDesc img(imgDataPtr, imgWidth, imgHeight, imgChannels);
     *     cpuProcessor->apply(img);
     * 
     * \endcode
     */
    ConstCPUProcessorRcPtr getDefaultCPUProcessor() const;
    ConstCPUProcessorRcPtr getOptimizedCPUProcessor(OptimizationFlags oFlags) const;
    ConstCPUProcessorRcPtr getOptimizedCPUProcessor(BitDepth inBitDepth,
                                                    BitDepth outBitDepth,
                                                    OptimizationFlags oFlags) const;

    Processor(const Processor &) = delete;
    Processor & operator= (const Processor &) = delete;
    /// Do not use (needed only for pybind11).
    ~Processor();

private:
    Processor();

    static ProcessorRcPtr Create();

    static void deleter(Processor* c);

    friend class Config;

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};


///////////////////////////////////////////////////////////////////////////
// CPUProcessor

class OCIOEXPORT CPUProcessor
{
public:
    /// The in and out bit-depths must be equal for isNoOp to be true.
    bool isNoOp() const;

    /**
     * Equivalent to isNoOp from the underlying Processor, i.e., it ignores 
     * in/out bit-depth differences.
     */
    bool isIdentity() const;

    bool hasChannelCrosstalk() const;

    const char * getCacheID() const;

    /// Bit-depth of the input pixel buffer.
    BitDepth getInputBitDepth() const;
    /// Bit-depth of the output pixel buffer.
    BitDepth getOutputBitDepth() const;

    /**
     *  The returned pointer may be used to set the value of any dynamic properties
     * of the requested type.  Throws if the requested property is not found.  Note that if the
     * processor contains several ops that support the requested property, only one can be dynamic.
     *
     * \note The dynamic properties in this object are decoupled from the ones in the
     * \ref Processor it was generated from. For each dynamic property in the Processor,
     * there is one in the CPU processor.
     */
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;
    /// True if at least one dynamic property of that type exists.
    bool hasDynamicProperty(DynamicPropertyType type) const noexcept;
    /// True if at least one dynamic property of any type exists and is dynamic.
    bool isDynamic() const noexcept;

    /**
     * \brief Apply to an image with any kind of channel ordering while
     * respecting the input and output bit-depths.
     */
    void apply(const ImageDesc & imgDesc) const;
    void apply(const ImageDesc & srcImgDesc, ImageDesc & dstImgDesc) const;

    /**
     * Apply to a single pixel respecting that the input and output bit-depths
     * be 32-bit float and the image buffer be packed RGB/RGBA.
     *
     * \note
     *    This is not as efficient as applying to an entire image at once.
     *    If you are processing multiple pixels, and have the flexibility,
     *    use the above function instead.
     */
    void applyRGB(float * pixel) const;
    void applyRGBA(float * pixel) const;

    CPUProcessor(const CPUProcessor &) = delete;
    CPUProcessor& operator= (const CPUProcessor &) = delete;
    /// Do not use (needed only for pybind11).
    ~CPUProcessor();

private:
    CPUProcessor();

    static void deleter(CPUProcessor * c);

    friend class Processor;

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};


///////////////////////////////////////////////////////////////////////////
// GPUProcessor

class OCIOEXPORT GPUProcessor
{
public:
    bool isNoOp() const;

    bool hasChannelCrosstalk() const;

    const char * getCacheID() const;

    /// Extract & Store the shader information to implement the color processing.
    void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;

    /// Extract the shader information using a custom GpuShaderCreator class.
    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const;
    
    GPUProcessor(const GPUProcessor &) = delete;
    GPUProcessor& operator= (const GPUProcessor &) = delete;
    /// Do not use (needed only for pybind11).
    ~GPUProcessor();

private:
    GPUProcessor();

    static void deleter(GPUProcessor * c);

    friend class Processor;

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};


/**
 * \brief
 * 
 * This class contains meta information about the process that generated
 * this processor.  The results of these functions do not
 * impact the pixel processing.
 */
class OCIOEXPORT ProcessorMetadata
{
public:
    static ProcessorMetadataRcPtr Create();

    int getNumFiles() const;
    const char * getFile(int index) const;

    int getNumLooks() const;
    const char * getLook(int index) const;

    void addFile(const char * fname);
    void addLook(const char * look);

    ProcessorMetadata(const ProcessorMetadata &) = delete;
    ProcessorMetadata& operator= (const ProcessorMetadata &) = delete;
    /// Do not use (needed only for pybind11).
    ~ProcessorMetadata();

private:
    ProcessorMetadata();

    static void deleter(ProcessorMetadata* c);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};



/**
 * In certain situations it is necessary to serialize transforms into a variety
 * of application specific LUT formats. Note that not all file formats that may
 * be read also support baking.
 *
 * **Usage Example:** *Bake a CSP sRGB viewer LUT*
 *
 * \code{.cpp} 
 *
 *    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromEnv();
 *    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
 *    baker->setConfig(config);
 *    baker->setFormat("csp");
 *    baker->setInputSpace("lnf");
 *    baker->setShaperSpace("log");
 *    baker->setTargetSpace("sRGB");
 *    auto & metadata = baker->getFormatMetadata();
 *    metadata.addChildElement(OCIO::METADATA_DESCRIPTION, "A first comment");
 *    metadata.addChildElement(OCIO::METADATA_DESCRIPTION, "A second comment");
 *    std::ostringstream out;
 *    baker->bake(out); // fresh bread anyone!
 *    std::cout << out.str();
 * 
 * \endcode
 */
class OCIOEXPORT Baker
{
public:
    /// Create a new Baker.
    static BakerRcPtr Create();

    /// Create a copy of this Baker.
    BakerRcPtr createEditableCopy() const;

    ConstConfigRcPtr getConfig() const;
    /// Set the config to use.
    void setConfig(const ConstConfigRcPtr & config);

    const char * getFormat() const;
    /// Set the LUT output format.
    void setFormat(const char * formatName);

    const FormatMetadata & getFormatMetadata() const;
    /**
     * Get editable *optional* format metadata. The metadata that will be used
     * varies based on the capability of the given file format.  Formats such as CSP,
     * IridasCube, and ResolveCube will create comments in the file header using the value of
     * any first-level children elements of the formatMetadata.  The CLF/CTF formats will make
     * use of the top-level "id" and "name" attributes and children elements "Description",
     * "InputDescriptor", "OutputDescriptor", and "Info".
     */
    FormatMetadata & getFormatMetadata();

    const char * getInputSpace() const;
    /// Set the input ColorSpace that the LUT will be applied to.
    void setInputSpace(const char * inputSpace);

    const char * getShaperSpace() const;
    /**
     * Set an *optional* ColorSpace or NamedTransform to shape the incoming
     * values of the LUT.  When baking 3DLUT, this will correspond to the 1D
     * shaper used to normalise incoming values to the unit range.  When baking
     * 1D LUT, this will be used to determine the input range of the LUT.
     */
    void setShaperSpace(const char * shaperSpace);

    const char * getLooks() const;
    /**
     * Set the looks to be applied during baking. Looks is a potentially comma
     * (or colon) delimited list of lookNames, where +/- prefixes are optionally allowed to
     * denote forward/inverse look specification. (And forward is assumed in the absence of
     * either).
     */
    void setLooks(const char * looks);

    const char * getTargetSpace() const;
    /// Set the target (i.e., output) color space for the LUT. Must not be used if setDisplayView is used.
    void setTargetSpace(const char * targetSpace);

    const char * getDisplay() const;
    const char * getView() const;
    /// Set the display and view to apply during the baking. Must not be used if setTargetSpace is used.
    void setDisplayView(const char * display, const char * view);

    int getShaperSize() const;
    /**
     * Override the default shaper LUT size. Default value is -1, which allows
     * each format to use its own most appropriate size. For the CLF format, the default uses
     * a half-domain LUT1D (which is ideal for scene-linear inputs).
     */
    void setShaperSize(int shapersize);

    int getCubeSize() const;
    /**
     * Override the main LUT (3d or 1d) sample size. Default value is -1, which allows
     * each format to use its own most appropriate size.
     */
    void setCubeSize(int cubesize);

    /// Bake the LUT into the output stream.
    void bake(std::ostream & os) const;

    /// Get the number of LUT bakers.
    static int getNumFormats();

    /**
     * Get the LUT baker format name at index, return empty string if an invalid
     * index is specified.
     */
    static const char * getFormatNameByIndex(int index);
    /**
     * Get the LUT baker format extension at index, return empty string if an
     * invalid index is specified.
     */
    static const char * getFormatExtensionByIndex(int index);

    Baker(const Baker &) = delete;
    Baker& operator= (const Baker &) = delete;
    /// Do not use (needed only for pybind11).
    ~Baker();

private:
    Baker();

    static void deleter(Baker* o);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};


///////////////////////////////////////////////////////////////////////////
// ImageDesc

const ptrdiff_t AutoStride = std::numeric_limits<ptrdiff_t>::min();

/**
 * \brief
 * This is a light-weight wrapper around an image, that provides a context
 * for pixel access. This does NOT claim ownership of the pixels or copy
 * image data.
 */
class OCIOEXPORT ImageDesc
{
public:
    ImageDesc();
    virtual ~ImageDesc();

    /// Get a pointer to the red channel of the first pixel.
    virtual void * getRData() const = 0;
    /// Get a pointer to the green channel of the first pixel.
    virtual void * getGData() const = 0;
    /// Get a pointer to the blue channel of the first pixel.
    virtual void * getBData() const = 0;
    /**
     * Get a pointer to the alpha channel of the first pixel
     * or null as alpha channel is optional.
     */
    virtual void * getAData() const = 0;

    /// Get the bit-depth.
    virtual BitDepth getBitDepth() const = 0;

    /// Get the width to process (where x position starts at 0 and ends at width-1).
    virtual long getWidth() const = 0;
    /// Get the height to process (where y position starts at 0 and ends at height-1).
    virtual long getHeight() const = 0;

    /// Get the step in bytes to find the same color channel of the next pixel.
    virtual ptrdiff_t getXStrideBytes() const = 0;
    /**
     * Get the step in bytes to find the same color channel
     * of the pixel at the same position in the next line.
     */
    virtual ptrdiff_t getYStrideBytes() const = 0;

    /**
     * Is the image buffer in packed mode with the 4 color channels?
     * ("Packed" here means that XStrideBytes is 4x the bytes per channel, so it is more specific
     * than simply any PackedImageDesc.)
     */
    virtual bool isRGBAPacked() const = 0;
    /// Is the image buffer 32-bit float?
    virtual bool isFloat() const = 0;

private:
    ImageDesc(const ImageDesc &);
    ImageDesc & operator= (const ImageDesc &);
};

extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const ImageDesc&);


///////////////////////////////////////////////////////////////////////////
// PackedImageDesc

/**
 * All the constructors expect a pointer to packed image data (such as
 * rgbrgbrgb or rgbargbargba) starting at the first color channel of
 * the first pixel to process (which does not need to be the first pixel
 * of the image). The number of channels must be greater than or equal to 3.
 * If a 4th channel is specified, it is assumed to be alpha
 * information.  Channels > 4 will be ignored.
 *
 * \note
 *    The methods assume the CPUProcessor bit-depth type for the data pointer.
 */
class OCIOEXPORT PackedImageDesc : public ImageDesc
{
public:

    /**
     * \note
     *    numChannels must be 3 (RGB) or 4 (RGBA).
     */
    PackedImageDesc(void * data,
                    long width, long height,
                    long numChannels);

    /**
     * \note
     *    numChannels must be 3 (RGB) or 4 (RGBA).
     */
    PackedImageDesc(void * data,
                    long width, long height,
                    long numChannels,
                    BitDepth bitDepth,
                    ptrdiff_t chanStrideBytes,
                    ptrdiff_t xStrideBytes,
                    ptrdiff_t yStrideBytes);

    PackedImageDesc(void * data,
                    long width, long height,
                    ChannelOrdering chanOrder);

    PackedImageDesc(void * data,
                    long width, long height,
                    ChannelOrdering chanOrder,
                    BitDepth bitDepth,
                    ptrdiff_t chanStrideBytes,
                    ptrdiff_t xStrideBytes,
                    ptrdiff_t yStrideBytes);

    virtual ~PackedImageDesc();

    /// Get the channel ordering of all the pixels.
    ChannelOrdering getChannelOrder() const;

    /// Get the bit-depth.
    BitDepth getBitDepth() const override;

    /// Get a pointer to the first color channel of the first pixel.
    void * getData() const;

    void * getRData() const override;
    void * getGData() const override;
    void * getBData() const override;
    void * getAData() const override;

    long getWidth() const override;
    long getHeight() const override;
    long getNumChannels() const;

    ptrdiff_t getChanStrideBytes() const;
    ptrdiff_t getXStrideBytes() const override;
    ptrdiff_t getYStrideBytes() const override;

    bool isRGBAPacked() const override;
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
// PlanarImageDesc

/**
 * All the constructors expect pointers to the specified image planes
 * (i.e. rrrr gggg bbbb) starting at the first color channel of the
 * first pixel to process (which need not be the first pixel of the image).
 * Pass NULL for aData if no alpha exists (r/g/bData must not be NULL).
 *
 * \note
 *    The methods assume the CPUProcessor bit-depth type for the R/G/B/A data pointers.
 */
class OCIOEXPORT PlanarImageDesc : public ImageDesc
{
public:

    PlanarImageDesc(void * rData, void * gData, void * bData, void * aData,
                    long width, long height);

    /**
     *
     * Note that although PlanarImageDesc is powerful enough to also describe
     * all PackedImageDesc scenarios, it is recommended to use
     * a PackedImageDesc where possible since that allows for additional
     * optimizations.
     */
    PlanarImageDesc(void * rData, void * gData, void * bData, void * aData,
                    long width, long height,
                    BitDepth bitDepth,
                    ptrdiff_t xStrideBytes,
                    ptrdiff_t yStrideBytes);

    virtual ~PlanarImageDesc();

    void * getRData() const override;
    void * getGData() const override;
    void * getBData() const override;
    void * getAData() const override;

    /// Get the bit-depth.
    BitDepth getBitDepth() const override;

    long getWidth() const override;
    long getHeight() const override;

    ptrdiff_t getXStrideBytes() const override;
    ptrdiff_t getYStrideBytes() const override;

    bool isRGBAPacked() const override;
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
// GpuShaderCreator
/**
 * Inherit from the class to fully customize the implementation of a GPU shader program
 * from a color transformation. 
 *
 * When no customizations are needed and the intermediate in-memory step is acceptable then the
 * \ref GpuShaderDesc is a better choice.
 *
 * \note
 *   To better decouple the \ref DynamicProperties from their GPU implementation, the code provides
 *   several addUniform() methods i.e. one per access function types. For example, an
 *   \ref ExposureContrastTransform instance owns three \ref DynamicProperties and they are all
 *   implemented by a double. When creating the GPU fragment shader program, the addUniform() with
 *   GpuShaderCreator::DoubleGetter is called when property is dynamic, up to three times.
 * 
 * **An OCIO shader program could contain:**
 *
 * * A declaration part  e.g., uniform sampled3D tex3;
 *
 * * Some helper methods
 *
 * * The OCIO shader function may be broken down as:
 *
 *    * The function header  e.g., void OCIODisplay(in vec4 inColor) {
 *    * The function body    e.g.,   vec4 outColor.rgb = texture3D(tex3, inColor.rgb).rgb;
 *    * The function footer  e.g.,   return outColor; }
 * 
 * 
 * **Usage Example:**
 * 
 * Below is a code snippet to highlight the different parts of the OCIO shader program.
 * 
 * \code{.cpp}
 *     
 *     // All global declarations
 *     uniform sampled3D tex3;
 *   
 *     // All helper methods
 *     vec3 computePosition(vec3 color)
 *     {
 *        vec3 coords = color;
 *        // Some processing...
 *        return coords;
 *     }
 *     
 *     // The shader function
 *     vec4 OCIODisplay(in vec4 inColor)     //
 *     {                                     // Function Header
 *        vec4 outColor = inColor;           //
 *     
 *        outColor.rgb = texture3D(tex3, computePosition(inColor.rgb)).rgb;
 *     
 *        return outColor;                   // Function Footer
 *     }                                     //
 * 
 * \endcode
 */
class OCIOEXPORT GpuShaderCreator
{
public:

    virtual GpuShaderCreatorRcPtr clone() const = 0;

    const char * getUniqueID() const noexcept;
    void setUniqueID(const char * uid) noexcept;

    GpuLanguage getLanguage() const noexcept;
    /// Set the shader program language.
    void setLanguage(GpuLanguage lang) noexcept;

    const char * getFunctionName() const noexcept;
    // Set the function name of the shader program.
    void setFunctionName(const char * name) noexcept;

    const char * getPixelName() const noexcept;
    /// Set the pixel name variable holding the color values.
    void setPixelName(const char * name) noexcept;

    /**
     *
     * \note
     *   Some applications require that textures, uniforms,
     *   and helper methods be uniquely named because several
     *   processor instances could coexist.
     */
    const char * getResourcePrefix() const noexcept;
    ///  Set a prefix to the resource name
    void setResourcePrefix(const char * prefix) noexcept;

    virtual const char * getCacheID() const noexcept;

    /// Start to collect the shader data.
    virtual void begin(const char * uid);
    /// End to collect the shader data.
    virtual void end();

    /// Some graphic cards could have 1D & 2D textures with size limitations.
    virtual void setTextureMaxWidth(unsigned maxWidth) = 0;
    virtual unsigned getTextureMaxWidth() const noexcept = 0;

    /// Allow 1D GPU resource type, otherwise always using 2D resources for 1D LUTs.
    virtual void setAllowTexture1D(bool allowed) = 0;
    virtual bool getAllowTexture1D() const = 0;

    /**
     * To avoid global texture sampler and uniform name clashes always append an increasing index
     * to the resource name.
     */
    unsigned getNextResourceIndex() noexcept;

    /// Function returning a double, used by uniforms. GPU converts double to float.
    typedef std::function<double()> DoubleGetter;
    /// Function returning a bool, used by uniforms.
    typedef std::function<bool()> BoolGetter;
    /// Functions returning a Float3, used by uniforms.
    typedef std::function<const Float3 &()> Float3Getter;
    /// Function returning an int, used by uniforms.
    typedef std::function<int()> SizeGetter;
    /// Function returning a float *, used by uniforms.
    typedef std::function<const float *()> VectorFloatGetter;
    /// Function returning an int *, used by uniforms.
    typedef std::function<const int *()> VectorIntGetter;

    virtual bool addUniform(const char * name,
                            const DoubleGetter & getDouble) = 0;

    virtual bool addUniform(const char * name,
                            const BoolGetter & getBool) = 0;

    virtual bool addUniform(const char * name,
                            const Float3Getter & getFloat3) = 0;

    virtual bool addUniform(const char * name,
                            const SizeGetter & getSize,
                            const VectorFloatGetter & getVectorFloat) = 0;

    virtual bool addUniform(const char * name,
                            const SizeGetter & getSize,
                            const VectorIntGetter & getVectorInt) = 0;

    /// Adds the property (used internally).
    void addDynamicProperty(DynamicPropertyRcPtr & prop);

    /// Dynamic Property related methods.
    unsigned getNumDynamicProperties() const noexcept;
    DynamicPropertyRcPtr getDynamicProperty(unsigned index) const;

    bool hasDynamicProperty(DynamicPropertyType type) const;
    /**
     * Dynamic properties allow changes once the fragment shader program has been created. The
     * steps are to get the appropriate DynamicProperty instance, and then change its value.
     */
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;

    enum TextureType
    {
        TEXTURE_RED_CHANNEL, ///< Only need a red channel texture
        TEXTURE_RGB_CHANNEL  ///< Need a RGB texture
    };

    /**
     * Dimension enum used to differentiate between 1D and 2D object/resource types.
     */
    enum TextureDimensions : uint8_t {
        TEXTURE_1D = 1,
        TEXTURE_2D = 2,
    };

    /**
     *  Add a 1D or 2D texture
     *
     * \note
     *   The 'values' parameter contains the LUT data which must be used as-is as the dimensions and
     *   origin are hard-coded in the fragment shader program. So, it means one GPU texture per entry.
     **/
    virtual void addTexture(const char * textureName,
                            const char * samplerName,
                            unsigned width, unsigned height,
                            TextureType channel,
                            TextureDimensions dimensions,
                            Interpolation interpolation,
                            const float * values) = 0;

    /**
     *  Add a 3D texture with RGB channel type.
     * 
     * \note 
     *   The 'values' parameter contains the 3D LUT data which must be used as-is as the dimension
     *   and origin are hard-coded in the fragment shader program. So, it means one GPU 3D texture
     *   per entry.
     **/
    virtual void add3DTexture(const char * textureName,
                              const char * samplerName,
                              unsigned edgelen,
                              Interpolation interpolation,
                              const float * values) = 0;

    // Methods to specialize parts of a OCIO shader program
    virtual void addToDeclareShaderCode(const char * shaderCode);
    virtual void addToHelperShaderCode(const char * shaderCode);
    virtual void addToFunctionHeaderShaderCode(const char * shaderCode);
    virtual void addToFunctionShaderCode(const char * shaderCode);
    virtual void addToFunctionFooterShaderCode(const char * shaderCode);

    /**
     * \brief Create the OCIO shader program
     *
     * \note
     *   The OCIO shader program is decomposed to allow a specific implementation
     *   to change some parts. Some product integrations add the color processing
     *   within a client shader program, imposing constraints requiring this flexibility.
     */
    virtual void createShaderText(const char * shaderDeclarations,
                                  const char * shaderHelperMethods,
                                  const char * shaderFunctionHeader,
                                  const char * shaderFunctionBody,
                                  const char * shaderFunctionFooter);

    virtual void finalize();
    
    GpuShaderCreator(const GpuShaderCreator &) = delete;
    GpuShaderCreator & operator= (const GpuShaderCreator &) = delete;
    /// Do not use (needed only for pybind11).
    virtual ~GpuShaderCreator();

protected:
    GpuShaderCreator();

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

// GpuShaderDesc
/**
 * \brief This class holds the GPU-related information needed to build a shader program
 * from a specific processor.
 *
 * This class defines the interface and there are two implementations provided.
 * The "legacy" mode implements the OCIO v1 approach of baking certain ops
 * in order to have at most one 3D-LUT.  The "generic" mode is the v2 default and
 * allows all the ops to be processed as-is, without baking, like the CPU renderer.
 * Custom implementations could be written to accommodate the GPU needs of a
 * specific client app.
 *
 *
 * The complete fragment shader program is decomposed in two main parts:
 * the OCIO shader program for the color processing and the client shader
 * program which consumes the pixel color processing.
 *
 * The OCIO shader program is fully described by the GpuShaderDesc
 * independently from the client shader program. The only critical
 * point is the agreement on the OCIO function shader name.
 *
 * To summarize, the complete shader program is:
 *
 * \code{.cpp}
 *
 *  ////////////////////////////////////////////////////////////////////////
 *  //                                                                    //
 *  //               The complete fragment shader program                 //
 *  //                                                                    //
 *  ////////////////////////////////////////////////////////////////////////
 *  //                                                                    //
 *  //   //////////////////////////////////////////////////////////////   //
 *  //   //                                                          //   //
 *  //   //               The OCIO shader program                    //   //
 *  //   //                                                          //   //
 *  //   //////////////////////////////////////////////////////////////   //
 *  //   //                                                          //   //
 *  //   //   // All global declarations                             //   //
 *  //   //   uniform sampled3D tex3;                                //   //
 *  //   //                                                          //   //
 *  //   //   // All helper methods                                  //   //
 *  //   //   vec3 computePos(vec3 color)                            //   //
 *  //   //   {                                                      //   //
 *  //   //      vec3 coords = color;                                //   //
 *  //   //      ...                                                 //   //
 *  //   //      return coords;                                      //   //
 *  //   //   }                                                      //   //
 *  //   //                                                          //   //
 *  //   //   // The OCIO shader function                            //   //
 *  //   //   vec4 OCIODisplay(in vec4 inColor)                      //   //
 *  //   //   {                                                      //   //
 *  //   //      vec4 outColor = inColor;                            //   //
 *  //   //      ...                                                 //   //
 *  //   //      outColor.rbg                                        //   //
 *  //   //         = texture3D(tex3, computePos(inColor.rgb)).rgb;  //   //
 *  //   //      ...                                                 //   //
 *  //   //      return outColor;                                    //   //
 *  //   //   }                                                      //   //
 *  //   //                                                          //   //
 *  //   //////////////////////////////////////////////////////////////   //
 *  //                                                                    //
 *  //   //////////////////////////////////////////////////////////////   //
 *  //   //                                                          //   //
 *  //   //             The client shader program                    //   //
 *  //   //                                                          //   //
 *  //   //////////////////////////////////////////////////////////////   //
 *  //   //                                                          //   //
 *  //   //   uniform sampler2D image;                               //   //
 *  //   //                                                          //   //
 *  //   //   void main()                                            //   //
 *  //   //   {                                                      //   //
 *  //   //      vec4 inColor = texture2D(image, gl_TexCoord[0].st); //   //
 *  //   //      ...                                                 //   //
 *  //   //      vec4 outColor = OCIODisplay(inColor);               //   //
 *  //   //      ...                                                 //   //
 *  //   //      gl_FragColor = outColor;                            //   //
 *  //   //   }                                                      //   //
 *  //   //                                                          //   //
 *  //   //////////////////////////////////////////////////////////////   //
 *  //                                                                    //
 *  ////////////////////////////////////////////////////////////////////////
 * \endcode
 * 
 * **Usage Example:** *Building a GPU shader*
 *
 *   This example is based on the code in: src/apps/ociodisplay/main.cpp
 *
 * \code{.cpp}
 *
 *    // Get the processor
 *    //
 *    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromEnv();
 *    OCIO::ConstProcessorRcPtr processor
 *       = config->getProcessor("ACES - ACEScg", "Output - sRGB");
 *
 *    // Step 1: Create a GPU shader description
 *    //
 *    // The two potential scenarios are:
 *    //
 *    //   1. Instantiate the generic shader description.  The color processor
 *    //      is used as-is (i.e. without any baking step) and could contain
 *    //      any number of 1D & 3D luts.
 *    //
 *    //      This is the default OCIO v2 behavior and allows a much better
 *    //      match between the CPU and GPU renderers.
 *    //
 *    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::Create();
 *    //
 *    //   2. Instantiate a custom shader description.
 *    //
 *    //      Writing a custom shader description is a way to tailor the shaders
 *    //      to the needs of a given client program.  This involves writing a
 *    //      new class inheriting from the pure virtual class GpuShaderDesc.
 *    //
 *    //      Please refer to the GenericGpuShaderDesc class for an example.
 *    //
 *    OCIO::GpuShaderDescRcPtr shaderDesc = MyCustomGpuShader::Create();
 *
 *    shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_2);
 *    shaderDesc->setFunctionName("OCIODisplay");
 *
 *    // Step 2: Collect the shader program information for a specific processor
 *    //
 *    processor->extractGpuShaderInfo(shaderDesc);
 *
 *    // Step 3: Create a helper to build the shader. Here we use a helper for
 *    //         OpenGL but there will also be helpers for other languages.
 *    //
 *    OpenGLBuilderRcPtr oglBuilder = OpenGLBuilder::Create(shaderDesc);
 *
 *    // Step 4: Allocate & upload all the LUTs
 *    //
 *    oglBuilder->allocateAllTextures();
 *
 *    // Step 5: Build the complete fragment shader program using
 *    //         g_fragShaderText which is the client shader program.
 *    //
 *    g_programId = oglBuilder->buildProgram(g_fragShaderText);
 *
 *    // Step 6: Enable the fragment shader program, and all needed textures
 *    //
 *    glUseProgram(g_programId);
 *    glUniform1i(glGetUniformLocation(g_programId, "tex1"), 1);  // image texture
 *    oglBuilder->useAllTextures(g_programId);                    // LUT textures
 *
 *    // Step 7: Update uniforms from dynamic property instances.
 *    m_oglBuilder->useAllUniforms();
 * \endcode
 *
 */
class OCIOEXPORT GpuShaderDesc : public GpuShaderCreator
{
public:

    /// Create the default shader description.
    static GpuShaderDescRcPtr CreateShaderDesc();

    GpuShaderCreatorRcPtr clone() const override;

    /**
     * Used to retrieve uniform information. UniformData m_type indicates the type of uniform
     * and what member of the structure should be used:
     * * UNIFORM_DOUBLE: m_getDouble.
     * * UNIFORM_BOOL: m_getBool.
     * * UNIFORM_FLOAT3: m_getFloat3.
     * * UNIFORM_VECTOR_FLOAT: m_vectorFloat.
     * * UNIFORM_VECTOR_INT: m_vectorInt.
     */
    struct UniformData
    {
        UniformDataType m_type{ UNIFORM_UNKNOWN };
        DoubleGetter m_getDouble{};
        BoolGetter m_getBool{};
        Float3Getter m_getFloat3{};
        struct VectorFloat
        {
            SizeGetter m_getSize{};
            VectorFloatGetter m_getVector{};
        } m_vectorFloat{};
        struct VectorInt
        {
            SizeGetter m_getSize{};
            VectorIntGetter m_getVector{};
        } m_vectorInt{};
    };
    virtual unsigned getNumUniforms() const noexcept = 0;
    /// Returns name of uniform and data as parameter.
    virtual const char * getUniform(unsigned index, UniformData & data) const = 0;

    // 1D lut related methods
    virtual unsigned getNumTextures() const noexcept = 0;
    virtual void getTexture(unsigned index,
                            const char *& textureName,
                            const char *& samplerName,
                            unsigned & width,
                            unsigned & height,
                            TextureType & channel,
                            TextureDimensions & dimensions,
                            Interpolation & interpolation) const = 0;
    virtual void getTextureValues(unsigned index, const float *& values) const = 0;

    // 3D lut related methods
    virtual unsigned getNum3DTextures() const noexcept = 0;
    virtual void get3DTexture(unsigned index,
                              const char *& textureName,
                              const char *& samplerName,
                              unsigned & edgelen,
                              Interpolation & interpolation) const = 0;
    virtual void get3DTextureValues(unsigned index, const float *& values) const = 0;

    /// Get the complete OCIO shader program.
    const char * getShaderText() const noexcept;

    GpuShaderDesc(const GpuShaderDesc &) = delete;
    GpuShaderDesc& operator= (const GpuShaderDesc &) = delete;
    /// Do not use (needed only for pybind11).
    virtual ~GpuShaderDesc();

protected:
    GpuShaderDesc();
};


/**
 * Context
 * 
 * A context defines some overrides to a Config. For example, it can override the
 * search path or change the value of a context variable.
 *
 * \note 
 *    Only some \ref Config::getProcessor methods accept a custom context; otherwise,
 *    the default context instance is used (see \ref Config::getCurrentContext).
 *
 * Context Variables
 * 
 * The context variables allow changes at runtime using environment variables. For example,
 * a color space name (such as src & dst for the ColorSpaceTransform) or a file
 * name (such as LUT file name for the FileTransform) could be defined by context
 * variables. The color transformation is then customized based on some environment variables.
 *
 * In a config the context variables support three syntaxes (i.e. ${VAR}, $VAR and %VAR%) and
 * the parsing starts from longest to shortest. So, the resolve works like '$TEST_$TESTING_$TE'
 * expands in this order '2 1 3'.
 *
 * Config authors are recommended to include the "environment" section in their configs. This
 * improves performance as well as making the config more readable. When present, this section
 * must declare all context variables used in the config. It may also provide a default value,
 * in case the variable is not present in the user's environment.
 *
 * A context variable may only be used in the following places:
 * * the \ref `ColorSpaceTransform` to define the source and the destination color space names,
 * * the \ref `FileTransform` to define the source file name (e.g. a LUT file name),
 * * the search_path,
 * * the cccid of the \ref `FileTransform` to only extract one specific transform from
 *   the CDL & CCC files.
 *
 * Some specific restrictions are worth calling out:
 * * they cannot be used as either the name or value of a role,
 * * the context variable characters $ and % are prohibited in a color space name.
 */
class OCIOEXPORT Context
{
public:
    static ContextRcPtr Create();

    ContextRcPtr createEditableCopy() const;

    const char * getCacheID() const;

    void setSearchPath(const char * path);
    const char * getSearchPath() const;

    int getNumSearchPaths() const;
    const char * getSearchPath(int index) const;
    void clearSearchPaths();
    void addSearchPath(const char * path);
    void setWorkingDir(const char * dirname);
    const char * getWorkingDir() const;

    /// Add (or update) a context variable. But it removes it if the value argument is null.
    /// Note that a Context StringVar is the same thing as a Config EnvironmentVar and these
    /// are both often referred to as a "context var".
    void setStringVar(const char * name, const char * value) noexcept;
    /// Get the context variable value. It returns an empty string if the context 
    /// variable is null or does not exist.
    const char * getStringVar(const char * name) const noexcept;

    int getNumStringVars() const;
    const char * getStringVarNameByIndex(int index) const;
    
    const char * getStringVarByIndex(int index) const;

    void clearStringVars();

    /// Add to the instance all the context variables from ctx.
    void addStringVars(const ConstContextRcPtr & ctx) noexcept;

    /// See \ref Config::setEnvironmentMode.
    void setEnvironmentMode(EnvironmentMode mode) noexcept;
    EnvironmentMode getEnvironmentMode() const noexcept;

    /// Seed string vars with the current environment, based on the EnvironmentMode setting.
    void loadEnvironment() noexcept;

    /// Resolve all the context variables from the string. It could be color space
    /// names or file names. Note that it recursively applies the context variable resolution.
    /// Returns the string unchanged if it does not contain any context variable.  
    const char * resolveStringVar(const char * string) const noexcept;
    /// Resolve all the context variables from the string and return all the context
    /// variables used to resolve the string (empty if no context variables were used).
    const char * resolveStringVar(const char * string, ContextRcPtr & usedContextVars) const noexcept;

    /**
     * Build the resolved and expanded filepath using the search_path when needed,
     * and check if the filepath exists. If it cannot be resolved or found, an exception will be
     * thrown. The method argument is directly from the config file so it can be an absolute or
     * relative file path or a file name.
     *
     * \note The filepath existence check could add a performance hit.
     *
     * \note The context variable resolution is performed using :cpp:func:`resolveStringVar`.
     */
    const char * resolveFileLocation(const char * filename) const;
    /// Build the resolved and expanded filepath and return all the context variables
    /// used to resolve the filename (empty if no context variables were used).
    const char * resolveFileLocation(const char * filename, ContextRcPtr & usedContextVars) const;

    /// Set the ConfigIOProxy object used to provision the config and LUTs from somewhere other
    /// than the file system.
    void setConfigIOProxy(ConfigIOProxyRcPtr ciop);
    ConfigIOProxyRcPtr getConfigIOProxy() const;

    Context(const Context &) = delete;
    Context& operator= (const Context &) = delete;
    /// Do not use (needed only for pybind11).
    ~Context();

private:
    Context();

    static void deleter(Context* c);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Context&);


///////////////////////////////////////////////////////////////////////////
// BuiltinTransformRegistry

/**
 * The built-in transform registry contains all the existing built-in transforms which can
 * be used by a configuration (version 2 or higher only).
 */
class OCIOEXPORT BuiltinTransformRegistry
{
public:
    BuiltinTransformRegistry(const BuiltinTransformRegistry &) = delete;
    BuiltinTransformRegistry & operator= (const BuiltinTransformRegistry &) = delete;

    /// Get the current built-in transform registry.
    static ConstBuiltinTransformRegistryRcPtr Get() noexcept;

    /// Get the number of built-in transforms available.
    virtual size_t getNumBuiltins() const noexcept = 0;
    /**
     * Get the style string for the i-th built-in transform.
     * The style is the ID string that identifies a given transform.
     */
    virtual const char * getBuiltinStyle(size_t index) const = 0;
    /// Get the description string for the i-th built-in transform.
    virtual const char * getBuiltinDescription(size_t index) const = 0;

protected:
    BuiltinTransformRegistry() = default;
    virtual ~BuiltinTransformRegistry() = default;
};

///////////////////////////////////////////////////////////////////////////
// BuiltinConfigRegistry

/**
 * The built-in configs registry contains information about all the existing built-in configs.
 */
class OCIOEXPORT BuiltinConfigRegistry
{
public:
    BuiltinConfigRegistry(const BuiltinConfigRegistry &) = delete;
    BuiltinConfigRegistry & operator= (const BuiltinConfigRegistry &) = delete;

    /// Get the current built-in configs registry.
    static const BuiltinConfigRegistry & Get() noexcept;

    /// Get the number of built-in configs available.
    virtual size_t getNumBuiltinConfigs() const noexcept = 0;

    /// Get the name of the config at the specified (zero-based) index. 
    /// Throws for illegal index.
    virtual const char * getBuiltinConfigName(size_t configIndex) const = 0;

    // Get a user-friendly name for a built-in config, appropriate for displaying in a user interface.
    /// Throws for illegal index.
    virtual const char * getBuiltinConfigUIName(size_t configIndex) const = 0;

    /// Get Yaml text of the built-in config at the specified index.
    /// Throws for illegal index.
    virtual const char * getBuiltinConfig(size_t configIndex) const = 0;
    
    /// Get the Yaml text of the built-in config with the specified name. 
    /// Throws if the name is not found.
    virtual const char * getBuiltinConfigByName(const char * configName) const = 0;

    /**
     * @brief Check if a specific built-in config is recommended.
     * 
     * For backwards compatibility reasons, configs will remain in the registry even if they have
     * been superseded. If an app is presenting a list of configs to users, it should not include 
     * configs that are no longer recommended.
     * 
     * Throws if the name is not found.
     * 
     * @param configIndex Index of built-in config.
     * @return true if the config is recommended.
     */
    virtual bool isBuiltinConfigRecommended(size_t configIndex) const = 0;

    // Return the full forward-compatible name of the default built-in config.
    OCIO_DEPRECATED("This was marked as deprecated starting in v2.3, please use ResolveConfigPath(\"ocio://default\").")
    virtual const char * getDefaultBuiltinConfigName() const = 0;
protected:
    BuiltinConfigRegistry() = default;
    virtual ~BuiltinConfigRegistry() = default;
};


///////////////////////////////////////////////////////////////////////////
// SystemMonitors

/**
 * Provides access to the ICC monitor profile provided by the operating system for each active display.
 */
class OCIOEXPORT SystemMonitors
{
public:
    SystemMonitors(const SystemMonitors &) = delete;
    SystemMonitors & operator= (const SystemMonitors &) = delete;

    /// Get the existing instance. 
    static ConstSystemMonitorsRcPtr Get() noexcept;

    /** 
     * True if the OS is able to provide ICC profiles for the attached monitors (macOS, Windows)
     * and false otherwise.
     */
    virtual bool isSupported() const noexcept = 0;

    /**
     * Methods to access some information of the attached and active monitors.
     */

    /// Get the number of active monitors reported by the operating system.
    virtual size_t getNumMonitors() const noexcept = 0;

    /** 
     * \brief  Get the monitor profile name.
     *
     * Get the string describing the monitor. It is used as an argument to instantiateDisplay. It
     * may also be used in a UI to ask a user which of several monitors they want to instantiate a
     * display for.
     */
    virtual const char * getMonitorName(size_t idx) const = 0;
    /// Get the ICC profile path associated to the monitor.
    virtual const char * getProfileFilepath(size_t idx) const = 0;

protected:
    SystemMonitors() = default;
    virtual ~SystemMonitors() = default;
};


///////////////////////////////////////////////////////////////////////////
// ConfigIOProxy

/**
 * ConfigIOProxy is a proxy class to allow the calling program to supply the config and any
 * associated LUT files directly, without relying on the standard file system. 
 * 
 * The OCIOZ archive feature is implemented using this mechanism.
 */
class OCIOEXPORT ConfigIOProxy
{
public:
    ConfigIOProxy() = default;
    virtual ~ConfigIOProxy() = default;

    /**
     * \brief Provide the contents of a LUT file as a buffer of uint8_t data.
     * 
     * \param filepath Fully resolved path to the "file."
     * 
     * The file path is based on the Config's current working directory and is the same absolute
     * path that would have been provided to the file system.
     * 
     * \return Vector of uint8 with the content of the LUT.
     */
    virtual std::vector<uint8_t> getLutData(const char * filepath) const = 0;

    /**
     * \brief Provide the config file Yaml to be parsed.
     * 
     * \return String with the config Yaml.
     */
    virtual std::string getConfigData() const = 0;

    /**
     * \brief Provide a fast unique ID for a LUT file.
     * 
     * This is intended to supply the string that will be used in OCIO's FileCacheMap.
     * This should be highly performant and typically should not require extensive
     * computation such as calculating the md5 hash of the file, unless it is pre-computed.
     * 
     * If the "file" does not exist, in other words, if the proxy is unable to supply the requested
     * file contents, the function must return an empty string.
     * 
     * \param filepath Fully resolve the path to the "file."
     * 
     * The file path is based on the Config's current working directory and is the same absolute
     * path that would have been provided to the file system.
     *
     * \return The file hash string.
     */
    virtual std::string getFastLutFileHash(const char * filepath) const = 0;
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_OPENCOLORIO_H
