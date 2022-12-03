// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_STUDIOCONFIG_H
#define INCLUDED_OCIO_STUDIOCONFIG_H


#include <OpenColorIO/OpenColorIO.h>
#include "builtinconfigs/BuiltinConfigRegistry.h"

namespace OCIO_NAMESPACE
{

class BuiltinConfigRegistryImpl;

namespace STUDIOCONFIG
{
    void Register(BuiltinConfigRegistryImpl & registry) noexcept;
} // namespace STUDIOCONFIG

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_STUDIOCONFIG_H
