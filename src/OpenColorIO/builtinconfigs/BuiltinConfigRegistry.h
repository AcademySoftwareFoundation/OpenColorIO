// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_BUILTIN_CONFIGS_REGISTRY_H
#define INCLUDED_OCIO_BUILTIN_CONFIGS_REGISTRY_H

#include <string>
#include <vector>
#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

// Returns true if sanitizedConfigName (the result of ConfigUtils::SanitizeIDToken on a config
// name) is the name of one of the ASWF OCIO configs (e.g. "cg-config-v4.0.0_aces-v2.0_ocio-v2.5")
// version 4.0.0 or higher, since those configs already have interop_id attributes populated
// and so must not be used as the namespace of a locally-generated interop ID.  A name that
// happens to start with "cg-config-"/"studio-config-" followed by a version but continues with
// something other than the expected "_aces-<version>_ocio-<version>" suffix is not one of these
// built-in configs and so is not reserved. See GenerateLocalIDForColorSpace.
bool IsReservedConfigName(const std::string & sanitizedConfigName);

class BuiltinConfigRegistryImpl : public BuiltinConfigRegistry
{
struct BuiltinConfigData
{
    BuiltinConfigData(const char * name, const char * uiName, const char * config, bool isRecommended)
        : m_config(config ? config : "")
        , m_name(name ? name : "")
        , m_uiName(uiName ? uiName : "")
        , m_isRecommended(isRecommended)
    {
    }

    BuiltinConfigData() = delete;
    BuiltinConfigData(const BuiltinConfigData & o) = default;
    BuiltinConfigData(BuiltinConfigData &&) = default;
    BuiltinConfigData & operator= (const BuiltinConfigData &) = default;

    // m_config is accessing a global static pointer so there is not need to manage it.
    const char * m_config;
    std::string m_name;
    std::string m_uiName;
    bool m_isRecommended;
};
using BuiltinConfigs = std::vector<BuiltinConfigData>;

public:
    BuiltinConfigRegistryImpl() = default;
    BuiltinConfigRegistryImpl(const BuiltinConfigRegistryImpl &) = delete;
    BuiltinConfigRegistryImpl & operator=(const BuiltinConfigRegistryImpl &) = delete;
    ~BuiltinConfigRegistryImpl() = default;

    /**
     * @brief Loads built-in configs into the registry.
     * 
     * Loads the built-in configs from various config header file that were generated from 
     * a template header file with cmake.
     * 
     * The init method is light-weight. It does not contain a copy of the config data strings 
     * or parse them into config objects.
     */
    void init() noexcept;

    /**
     * @brief Add a built-in config into the registry.
     * 
     * Adding a built-in config using an existing name will overwrite the current built-in
     * config associated with that name.
     * 
     * For backward compatibility, built-in configs can be set as NOT recommended. They will
     * still be available, but not recommended for the current version of OCIO.
     * 
     * @param name Name for the built-in config.
     * @param uiName User-friendly config name.
     * @param config Config as string
     * @param isRecommended Is the built-in config recommended or not.
     */
    void addBuiltin(const char * name, const char * uiName, const char * config, bool isRecommended);

    /// Get the number of built-in configs available.
    size_t getNumBuiltinConfigs() const noexcept override;

    /// Get the name of the config at the specified (zero-based) index. 
    /// Throws for illegal index.
    const char * getBuiltinConfigName(size_t configIndex) const override;

    // Get a user-friendly name for a built-in config, appropriate for displaying in a user 
    // interface.
    /// Throws for illegal index.
    const char * getBuiltinConfigUIName(size_t configIndex) const override;

    /// Get Yaml text of the built-in config at the specified index.
    /// Throws for illegal index.
    const char * getBuiltinConfig(size_t configIndex) const override;

    /// Get the Yaml text of the built-in config with the specified name. 
    /// Throws if the name is not found.
    const char * getBuiltinConfigByName(const char * configName) const override;

    /// Check if a specific built-in config is recommended.
    /// Throws for illegal index.
    bool isBuiltinConfigRecommended(size_t configIndex) const override;

private:
    BuiltinConfigs m_builtinConfigs;
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_BUILTIN_CONFIGS_REGISTRY_H
