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
    using BuiltinConfigName = std::string;
    struct BuiltinConfigData
    {
        BuiltinConfigData(const char * name, const char * config, bool isDeprecated = false, bool isDefault = false)
            : m_config(config ? config : "")
            , m_name(name ? name : "")
            , m_isDeprecated(isDeprecated ? true : false)
            , m_isDefault(isDefault ? true : false)
        {
        }
        BuiltinConfigData() = delete;
        BuiltinConfigData(const BuiltinConfigData & o) = default;
        BuiltinConfigData(BuiltinConfigData && o) noexcept
            : m_config(std::move(o.m_config))
            , m_name(std::move(o.m_name))
            , m_isDeprecated(std::move(o.m_isDeprecated))
            , m_isDefault(std::move(o.m_isDefault))
        {
        }
        BuiltinConfigData & operator= (const BuiltinConfigData & o)
        {
            m_config        = o.m_config;
            m_name          = o.m_name;
            m_isDeprecated  = o.m_isDeprecated;
            m_isDefault     = o.m_isDefault;
            return *this;
        }

        std::string m_config;
        std::string m_name;
        bool m_isDefault;
        bool m_isDeprecated;
    };
    using BuiltinConfig = std::vector<BuiltinConfigData>;

    public:
        BuiltinConfigRegistryImpl() = default;
        BuiltinConfigRegistryImpl(const BuiltinConfigRegistryImpl &) = delete;
        BuiltinConfigRegistryImpl & operator=(const BuiltinConfigRegistryImpl &) = delete;
        ~BuiltinConfigRegistryImpl() = default;

        // config working group: Based on ACES: 
        // cg (simple basic one, minimum set), studio, 
        void init() noexcept;

        void addBuiltin(const char * name, const char * config, bool isDefault = false, bool isDeprecated = false);

        // Get the number of built-in configs available.
        size_t getNumConfigs() const noexcept override;

        // Get built-in config name at specified index.
        const char * getConfigName(size_t configIndex) const override;
        // Get built-in config at specified index as text.
        const char * getConfig(size_t configIndex) const override;
        // Get built-in config by name.
        const char * getConfigByName(const char * configName) const noexcept override;

        // Check if built-in config at specified index is still recommended.
        bool isConfigRecommended(size_t configIndex) const override;
        // Check if built-in config at specified index is deprecated.
        bool isConfigDepecrated(size_t configIndex) const override;

        // Get default built-in config name.
        const char * getDefaultConfigName() const override;
    private:
        BuiltinConfig m_builtinConfig;
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_BUILTIN_CONFIGS_REGISTRY_H
