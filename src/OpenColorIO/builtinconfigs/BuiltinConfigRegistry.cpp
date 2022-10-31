// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <memory>
#include <algorithm>
#include <sstream>

// OpenColorIO must be first - order is important.
#include <OpenColorIO/OpenColorIO.h>
#include "Mutex.h"

#include "Platform.h"

#include "builtinconfigs/BuiltinConfigRegistry.h"
#include "builtinconfigs/CGConfig.h"
#include "builtinconfigs/StudioConfig.h"

#define OUT_OF_RANGE_EXCEPTION_TEXT         "Config index is out of range."

namespace OCIO_NAMESPACE
{

const BuiltinConfigRegistry & BuiltinConfigRegistry::Get() noexcept
{
    // Meyer's Singleton pattern.

    static BuiltinConfigRegistryImpl globalRegistry;
    static Mutex globalRegistryMutex;
    
    AutoMutex guard(globalRegistryMutex);

    globalRegistry.init();

    return globalRegistry;
}

////////////////////////////////
////BuiltinConfigRegistryImpl
////////////////////////////////

void BuiltinConfigRegistryImpl::init() noexcept
{
    if (m_builtinConfigs.empty())
    {
        m_builtinConfigs.clear();
        
        CGCONFIG::Register(*this);
        STUDIOCONFIG::Register(*this);

        this->setDefaultBuiltinConfig("cg-config-v1.0.0_aces-v1.3_ocio-v2.1");
    }
}

void BuiltinConfigRegistryImpl::addBuiltin(const char * name, const char * uiName, const char * const config, bool isRecommended)
{
    BuiltinConfigData data { name, uiName, config, isRecommended };

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

size_t BuiltinConfigRegistryImpl::getNumBuiltinConfigs() const noexcept
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

const char * BuiltinConfigRegistryImpl::getBuiltinConfigUIName(size_t configIndex) const
{
    if (configIndex >= m_builtinConfigs.size())
    {
        throw Exception(OUT_OF_RANGE_EXCEPTION_TEXT);
    }

    return m_builtinConfigs[configIndex].m_uiName.c_str();
}

const char * BuiltinConfigRegistryImpl::getBuiltinConfig(size_t configIndex) const
{
    if (configIndex >= m_builtinConfigs.size())
    {
        throw Exception(OUT_OF_RANGE_EXCEPTION_TEXT);
    }

    return m_builtinConfigs[configIndex].m_config;
}

const char * BuiltinConfigRegistryImpl::getBuiltinConfigByName(const char * configName) const
{
    // Search for config name.
    for (auto & builtin : m_builtinConfigs)
    {
        if (Platform::Strcasecmp(configName, builtin.m_name.c_str()) == 0)
        {
            return builtin.m_config;
        }
    }

    std::ostringstream os;
    os << "Could not find '" << configName << "' in the built-in configurations.";
    throw Exception(os.str().c_str());
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
        throw Exception("Internal error - The default built-in config name has not been set yet.");
    }

    return m_defaultBuiltinConfigName.c_str();
}

void BuiltinConfigRegistryImpl::setDefaultBuiltinConfig(const char * configName)
{
    // Search for config name.
    for (auto & builtin : m_builtinConfigs)
    {
        if (Platform::Strcasecmp(configName, builtin.m_name.c_str()) == 0)
        {
            m_defaultBuiltinConfigName = configName;
            return;
        }
    }

    throw Exception("Internal error - Config name does not exist."); 
}

} // namespace OCIO_NAMESPACE
