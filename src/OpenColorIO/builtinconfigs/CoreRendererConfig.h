// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CORE_RENDERER_CONFIG_H
#define INCLUDED_OCIO_CORE_RENDERER_CONFIG_H

namespace OCIO_NAMESPACE
{

class BuiltinConfigRegistryImpl;

namespace CORERENDERERCONFIG
{
    void Register(BuiltinConfigRegistryImpl & registry) noexcept;
} // namespace CORERENDERERCONFIG

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_CORE_RENDERER_CONFIG_H
