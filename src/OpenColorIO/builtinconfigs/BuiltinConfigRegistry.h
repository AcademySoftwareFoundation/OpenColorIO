// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_BUILTIN_CONFIGS_REGISTRY_H
#define INCLUDED_OCIO_BUILTIN_CONFIGS_REGISTRY_H

#include <map>
#include <vector>
#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

class BuiltinConfigRegistryImpl : public BuiltinConfigRegistry
{
    // Structure size
    // 32 bytes (std::string), 32 bytes (std::string), 1 byte (bool) + 7 bytes of padding.
    // Total of 72 bytes of memory per config
    struct BuiltinConfigData
    {
        BuiltinConfigData(const char * name, const char * config, bool isRecommended = false)
            : m_config(config ? config : "")
            , m_name(name ? name : "")
            , m_isRecommended(isRecommended ? true : false)
        {
        }
        BuiltinConfigData() = delete;
        BuiltinConfigData(const BuiltinConfigData & o) = default;
        BuiltinConfigData(BuiltinConfigData && o) noexcept
            : m_config(std::move(o.m_config))
            , m_name(std::move(o.m_name))
            , m_isRecommended(std::move(o.m_isRecommended))
        {
        }
        BuiltinConfigData & operator= (const BuiltinConfigData & o)
        {
            m_config        = o.m_config;
            m_name          = o.m_name;
            m_isRecommended  = o.m_isRecommended;
            return *this;
        }

        std::string m_config;
        std::string m_name;
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
         * Loads the built-in configs from various config header file
         * that were generated from a template header file with cmake.
         * 
         * The init method is light-weight. 
         * It does not contain a copy of the config data strings or parse them into config objects.
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
         * @param config Config as string
         * @param isRecommended Is the built-in config recommended or not.
         */
        void addBuiltin(const char * name, const char * config, bool isRecommended);

        /// Get the current built-in configs registry.
        size_t getNumBuiltInConfigs() const noexcept override;

        /// Get the name of the config at the specified (zero-based) index. 
        /// Throws for illegal index.
        const char * getBuiltinConfigName(size_t configIndex) const override;

        /// Get Yaml text of the built-in config at the specified index.
        const char * getBuiltinConfig(size_t configIndex) const override;

        /// Get the Yaml text of the built-in config with the specified name. 
        /// Throws if the name is not found.
        const char * getBuiltinConfigByName(const char * configName) const noexcept override;

        /// Check if a specific built-in config is recommended.
        bool isBuiltinConfigRecommended(size_t configIndex) const override;

        /// Get the default recommended built-in config.
        const char * getDefaultBuiltinConfigName() const override;

        /// Set the default built-in config.
        void setDefaultBuiltinConfig(const char * configName);
    private:
        BuiltinConfigs m_builtinConfigs;
        std::string m_defaultBuiltinConfigName;
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_BUILTIN_CONFIGS_REGISTRY_H