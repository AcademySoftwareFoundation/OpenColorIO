// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>
#include "CG.h"
#include "builtinconfigs/BuiltinConfigRegistry.h"
#include "builtinconfigs/CGConfig.h"

namespace OCIO_NAMESPACE
{
    //
    // Create the built-in transforms.
    //

    namespace CGCONFIG
    {
        // Register CG configs.
        void Register(BuiltinConfigRegistryImpl & registry) noexcept
        {
            registry.addBuiltin(
                "cg-config-v0.1.0_aces-v1.3_ocio-v2.1.1",
                CG_CONFIG_V010_ACES_V130_OCIO_V211,
                true
            );
        }

    } // namespace CGCONFIG

} // namespace OCIO_NAMESPACE
