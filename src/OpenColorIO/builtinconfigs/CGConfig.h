// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CGCONFIG_H
#define INCLUDED_OCIO_CGCONFIG_H


#include <OpenColorIO/OpenColorIO.h>
#include "builtinconfigs/BuiltinConfigRegistry.h"

namespace OCIO_NAMESPACE
{

class BuiltinConfigRegistryImpl;

namespace CGCONFIG
{
    void Register(BuiltinConfigRegistryImpl & registry) noexcept;
} // namespace CGCONFIG

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_CGCONFIG_H
