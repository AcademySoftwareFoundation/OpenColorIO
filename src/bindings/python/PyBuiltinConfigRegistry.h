// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYBUILTINCONFIGREGISTRY_H
#define INCLUDED_OCIO_PYBUILTINCONFIGREGISTRY_H

#include "PyOpenColorIO.h"

namespace OCIO_NAMESPACE
{

// Wrapper to preserve the BuiltinConfigRegistry singleton.
class OCIOHIDDEN PyBuiltinConfigRegistry
{
public:
    PyBuiltinConfigRegistry() = default;
    ~PyBuiltinConfigRegistry() = default;

    size_t getNumBuiltinConfigs() const noexcept
    {
        return BuiltinConfigRegistry::Get().getNumBuiltinConfigs();
    }

    const char * getBuiltinConfigName(size_t configIndex) const
    {
        return BuiltinConfigRegistry::Get().getBuiltinConfigName(configIndex);
    }

    const char * getBuiltinConfigUIName(size_t configIndex) const
    {
        return BuiltinConfigRegistry::Get().getBuiltinConfigUIName(configIndex);
    }

    const char * getBuiltinConfig(size_t configIndex) const
    {
        return BuiltinConfigRegistry::Get().getBuiltinConfig(configIndex);
    }

    const char * getBuiltinConfigByName(const char * configName) const
    {
        return BuiltinConfigRegistry::Get().getBuiltinConfigByName(configName);
    }

    bool isBuiltinConfigRecommended(size_t configIndex) const
    {
        return BuiltinConfigRegistry::Get().isBuiltinConfigRecommended(configIndex);
    }
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PYBUILTINCONFIGREGISTRY_H
