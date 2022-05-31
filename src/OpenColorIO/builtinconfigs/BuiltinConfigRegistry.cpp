// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
    m_builtinConfigs.clear();
    CGCONFIG::Register(*this);

    this->setDefaultBuiltinConfig("cg-config-v0.1.0_aces-v1.3_ocio-v2.1.1");
}

void BuiltinConfigRegistryImpl::addBuiltin(const char * name, const char * config, bool isRecommended)
{
    BuiltinConfigData data { name, config, isRecommended };

    for (auto & builtin : m_builtinConfigs)
    {
        // Overwrite data if the config name is the same.
        if (Platform::Strcasecmp(data.m_name.c_str(), builtin.m_name.c_str()) == 0)
        {
            builtin = data;
            return;
        }
    }

    m_builtinConfigs.push_back(data);
}

size_t BuiltinConfigRegistryImpl::getNumBuiltInConfigs() const noexcept
{
    return m_builtinConfigs.size();
}

const char * BuiltinConfigRegistryImpl::getBuiltinConfigName(size_t configIndex) const
{
    if (configIndex >= m_builtinConfigs.size())
    {
        throw Exception(OUT_OF_RANGE_EXCEPTION_TEXT);
    }

    return m_builtinConfigs[configIndex].m_name.c_str();
}

const char * BuiltinConfigRegistryImpl::getBuiltinConfig(size_t configIndex) const
{
    if (configIndex >= m_builtinConfigs.size())
    {
        throw Exception(OUT_OF_RANGE_EXCEPTION_TEXT);
    }

    return m_builtinConfigs[configIndex].m_config.c_str();
}

const char * BuiltinConfigRegistryImpl::getBuiltinConfigByName(const char * configName) const noexcept
{
    // Search for config name.
    for (auto & builtin : m_builtinConfigs)
    {
        if (Platform::Strcasecmp(configName, builtin.m_name.c_str()) == 0)
        {
            return builtin.m_config.c_str();
        }
    }

    return "";
}

bool BuiltinConfigRegistryImpl::isBuiltinConfigRecommended(size_t configIndex) const
{
    if (configIndex >= m_builtinConfigs.size())
    {
        throw Exception(OUT_OF_RANGE_EXCEPTION_TEXT);
    }

    return m_builtinConfigs[configIndex].m_isRecommended;
}

const char * BuiltinConfigRegistryImpl::getDefaultBuiltinConfigName() const
{
    if (m_defaultBuiltinConfigName.empty())
    {
        // Make sure that at least one default built-ins config is present.
        throw Exception("Internal error - There is no default built-ins config.");
    }

    return m_defaultBuiltinConfigName.c_str();
}

void BuiltinConfigRegistryImpl::setDefaultBuiltinConfig(const char * configName)
{
    BuiltinConfigName builtinConfigName = "";
    // Search for config name.
    for (auto & builtin : m_builtinConfigs)
    {
        if (Platform::Strcasecmp(configName, builtin.m_name.c_str()) == 0)
        {
            builtinConfigName = builtin.m_name;
        }
    }

    if (builtinConfigName.empty())
    {
        throw Exception("Internal error - Config name do not exist.");
    }

    m_defaultBuiltinConfigName = builtinConfigName;
}

} // namespace OCIO_NAMESPACE
