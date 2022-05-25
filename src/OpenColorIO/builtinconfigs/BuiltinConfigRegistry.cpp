// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

// TODO CED: remove
#include <iostream>
#include <memory>
#include <algorithm>
#include <sstream>

// OpenColorIO must be first - order is important.
#include <OpenColorIO/OpenColorIO.h>
#include "Mutex.h"

#include "Platform.h"

#include "CG.h"
#include "builtinconfigs/BuiltinConfigRegistry.h"
#include "builtinconfigs/CGConfig.h"

#define OUT_OF_RANGE_EXCEPTION_TEXT         "Config index is out of range."
#define INVALID_CONFIG_NAME_EXCEPTION_TEXT  "Config name is invalid."

namespace OCIO_NAMESPACE
{

namespace
{

static BuiltinConfigRegistryRcPtr globalRegistry;
static Mutex globalRegistryMutex;

} // anon.

ConstBuiltinConfigRegistryRcPtr BuiltinConfigRegistry::Get() noexcept
{
    AutoMutex guard(globalRegistryMutex);

    if (!globalRegistry)
    {
        globalRegistry = std::make_shared<BuiltinConfigRegistryImpl>();
        DynamicPtrCast<BuiltinConfigRegistryImpl>(globalRegistry)->init();
    }

    return globalRegistry;
}

////////////////////////////////
////BuiltinConfigRegistryImpl
////////////////////////////////
void BuiltinConfigRegistryImpl::init() noexcept
{
    m_builtinConfig.clear();
    CGCONFIG::Register(*this);
}

void BuiltinConfigRegistryImpl::addBuiltin(const char * name, const char * config, bool isDeprecated, bool isDefault)
{
    BuiltinConfigData data { name, config, isDeprecated, isDefault };

    for (auto & builtin : m_builtinConfig)
    {
        // Overwrite data if the config name is the same.
        if (Platform::Strcasecmp(data.m_name.c_str(), builtin.m_name.c_str()) == 0)
        {
            builtin = data;
            return;
        }
    }

    m_builtinConfig.push_back(data);
}

size_t BuiltinConfigRegistryImpl::getNumConfigs() const noexcept
{
    return m_builtinConfig.size();
}

const char * BuiltinConfigRegistryImpl::getConfigName(size_t configIndex) const
{
    if (configIndex >= m_builtinConfig.size())
    {
        throw Exception(OUT_OF_RANGE_EXCEPTION_TEXT);
    }

    return m_builtinConfig[configIndex].m_name.c_str();
}

const char * BuiltinConfigRegistryImpl::getConfig(size_t configIndex) const
{
    if (configIndex >= m_builtinConfig.size())
    {
        throw Exception(OUT_OF_RANGE_EXCEPTION_TEXT);
    }

    return m_builtinConfig[configIndex].m_config.c_str();
}

const char * BuiltinConfigRegistryImpl::getConfigByName(const char * configName) const noexcept
{
    // Search for config name.
    for (auto & builtin : m_builtinConfig)
    {
        if (Platform::Strcasecmp(configName, builtin.m_name.c_str()) == 0)
        {
            return builtin.m_config.c_str();
        }
    }

    return "";
}

bool BuiltinConfigRegistryImpl::isConfigRecommended(size_t configIndex) const
{
    if (configIndex >= m_builtinConfig.size())
    {
        throw Exception(OUT_OF_RANGE_EXCEPTION_TEXT);
    }

    return !m_builtinConfig[configIndex].m_isDeprecated;
}

bool BuiltinConfigRegistryImpl::isConfigDepecrated(size_t configIndex) const
{
    if (configIndex >= m_builtinConfig.size())
    {
        throw Exception(OUT_OF_RANGE_EXCEPTION_TEXT);
    }

    return m_builtinConfig[configIndex].m_isDeprecated;
}

const char * BuiltinConfigRegistryImpl::getDefaultConfigName() const
{
    std::string configName = "";

    // Search for the default config.
    for (auto & builtin : m_builtinConfig)
    {
        if (builtin.m_isDefault)
        {
            // Return after the first config found.
            // Only one default config out of all built-ins.
            configName = builtin.m_name;
        }
    }

    if (configName.empty())
    {
        // Make sure that at least one default built-ins config is present.
        throw Exception("Internal error - There is no default built-ins config.");
    }

    return configName.c_str();
}

} // namespace OCIO_NAMESPACE
