// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_CONFIG_UTILS_H
#define INCLUDED_OCIO_CONFIG_UTILS_H

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

namespace ConfigUtils
{

bool GetInterchangeRolesForColorSpaceConversion(const char ** srcInterchangeCSName,
                                                const char ** dstInterchangeCSName,
                                                ReferenceSpaceType & interchangeType,
                                                const ConstConfigRcPtr & srcConfig,
                                                const char * srcName,
                                                const ConstConfigRcPtr & dstConfig,
                                                const char * dstName);

void IdentifyInterchangeSpace(const char ** srcInterchange, 
                              const char ** builtinInterchange,
                              const ConstConfigRcPtr & srcConfig,
                              const char * srcColorSpaceName, 
                              const ConstConfigRcPtr & builtinConfig, 
                              const char * builtinColorSpaceName);

const char * IdentifyBuiltinColorSpace(const ConstConfigRcPtr & srcConfig,
                                       const ConstConfigRcPtr & builtinConfig, 
                                       const char * builtinColorSpaceName);

// Temporarily deactivate the Processor cache on a Config object.
// Currently, this also clears the cache.
//
class SuspendCacheGuard
{
public:
    SuspendCacheGuard();
    SuspendCacheGuard(const SuspendCacheGuard &) = delete;
    SuspendCacheGuard & operator=(const SuspendCacheGuard &) = delete;

    SuspendCacheGuard(const ConstConfigRcPtr & config)
        : m_config(config), m_origCacheFlags(config->getProcessorCacheFlags())
    {
        m_config->setProcessorCacheFlags(PROCESSOR_CACHE_OFF);
    }

    ~SuspendCacheGuard()
    {
        m_config->setProcessorCacheFlags(m_origCacheFlags);
    }

private:
    ConstConfigRcPtr m_config = nullptr;
    ProcessorCacheFlags m_origCacheFlags;
};

} // namespace ConfigUtils

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_CONFIG_UTILS_H
