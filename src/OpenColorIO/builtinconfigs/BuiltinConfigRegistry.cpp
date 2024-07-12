// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <memory>
#include <algorithm>
#include <sstream>
#include <regex>

// OpenColorIO must be first - order is important.
#include <OpenColorIO/OpenColorIO.h>
#include "Mutex.h"

#include "Platform.h"

#include "builtinconfigs/BuiltinConfigRegistry.h"
#include "builtinconfigs/CGConfig.h"
#include "builtinconfigs/StudioConfig.h"

static constexpr char OUT_OF_RANGE_EXCEPTION_TEXT[] = "Config index is out of range.";

// TODO: Remove once getDefaultBuiltinConfigName is removed.
static constexpr char DEFAULT_BUILTIN_CONFIG[] = "cg-config-v2.1.0_aces-v1.3_ocio-v2.3";

// These are used for ResolveConfigPath function and we need to return a variable that still exists
// once the function finishes since we are returning a const char *.
static constexpr char DEFAULT_BUILTIN_CONFIG_URI[] = "ocio://cg-config-v2.1.0_aces-v1.3_ocio-v2.3";
static constexpr char LATEST_CG_BUILTIN_CONFIG_URI[] = "ocio://cg-config-v2.1.0_aces-v1.3_ocio-v2.3";
static constexpr char LATEST_STUDIO_BUILTIN_CONFIG_URI[] = "ocio://studio-config-v2.1.0_aces-v1.3_ocio-v2.3";

static constexpr char BUILTIN_DEFAULT_NAME[] = "default";
static constexpr char BUILTIN_LATEST_CG_NAME[] = "cg-config-latest";
static constexpr char BUILTIN_LATEST_STUDIO_NAME[] = "studio-config-latest";

namespace OCIO_NAMESPACE
{

// Note that this function does not require initializing the built-in config registry.
const char * ResolveConfigPath(const char * originalPath) noexcept
{
    static const std::regex uriPattern(R"(ocio:\/\/([^\s]+))");
    std::smatch match;
    const std::string uri = originalPath;
    // Check if original path starts with "ocio://".
    if (std::regex_search(uri, match, uriPattern))
    {
        if (Platform::Strcasecmp(match.str(1).c_str(), BUILTIN_DEFAULT_NAME) == 0)
        {
            return DEFAULT_BUILTIN_CONFIG_URI;
        }
        else if (Platform::Strcasecmp(match.str(1).c_str(), BUILTIN_LATEST_CG_NAME) == 0)
        {
            return LATEST_CG_BUILTIN_CONFIG_URI;
        }
        else if (Platform::Strcasecmp(match.str(1).c_str(), BUILTIN_LATEST_STUDIO_NAME) == 0)
        {
            return LATEST_STUDIO_BUILTIN_CONFIG_URI;
        }
    }

    // Return originalPath if no special path was used.
    return originalPath;
}

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
    }
}

void BuiltinConfigRegistryImpl::addBuiltin(const char * uiName, const char * const config, bool isRecommended, std::function<ConstConfigRcPtr()> creatorFn)
{
    BuiltinConfigData data { uiName, config, isRecommended, creatorFn };

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

ConstConfigRcPtr BuiltinConfigRegistryImpl::createBuiltinConfig(size_t configIndex) const
{
    if (configIndex >= m_builtinConfigs.size())
    {
        throw Exception(OUT_OF_RANGE_EXCEPTION_TEXT);
    }

    auto& builtin = m_builtinConfigs[configIndex];

    if (!builtin.m_creatorFn)
    {
        std::ostringstream os;
        os << "Creator function for the built-in config at index '" << configIndex << " is missing.'";
        throw Exception(os.str().c_str());
    }
    
    return builtin.m_creatorFn();
}

ConstConfigRcPtr BuiltinConfigRegistryImpl::createBuiltinConfigByName(const char* configName) const
{
    // Search for config name.
    for (auto & builtin : m_builtinConfigs)
    {
        if (Platform::Strcasecmp(configName, builtin.m_name.c_str()) == 0)
        {
            if (!builtin.m_creatorFn)
            {
                std::ostringstream os;
                os << "Creator function for the built-in config '" << configName << " is missing.'";
                throw Exception(os.str().c_str());
            }
            
            return builtin.m_creatorFn();
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
    return DEFAULT_BUILTIN_CONFIG;
}

} // namespace OCIO_NAMESPACE
