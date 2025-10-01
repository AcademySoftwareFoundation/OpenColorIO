// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>
#include "builtinconfigs/BuiltinConfigRegistry.h"
#include "builtinconfigs/CGConfig.h"

#include "CG.cpp"

namespace OCIO_NAMESPACE
{
// Create the built-in configs for all versions of the OCIO CG config for ACES.
// For backwards compatibility, previous versions are kept in the registry but the
// isRecommended flag should be set to false.

namespace CGCONFIG
{
void Register(BuiltinConfigRegistryImpl & registry) noexcept
{
    // If a new built-in config is added, do not forget to update the LATEST_CG_BUILTIN_CONFIG_URI
    // variable (in BuiltinConfigRegistry.cpp).

    registry.addBuiltin(
        "cg-config-v1.0.0_aces-v1.3_ocio-v2.1",
        "Academy Color Encoding System - CG Config [COLORSPACES v1.0.0] [ACES v1.3] [OCIO v2.1]",
        CG_CONFIG_V100_ACES_V13_OCIO_V21,
        false
    );

    registry.addBuiltin(
        "cg-config-v2.1.0_aces-v1.3_ocio-v2.3",
        "Academy Color Encoding System - CG Config [COLORSPACES v2.0.0] [ACES v1.3] [OCIO v2.3]",
        CG_CONFIG_V210_ACES_V13_OCIO_V23,
        false
    );

    registry.addBuiltin(
        "cg-config-v2.2.0_aces-v1.3_ocio-v2.4",
        "Academy Color Encoding System - CG Config [COLORSPACES v2.2.0] [ACES v1.3] [OCIO v2.4]",
        CG_CONFIG_V220_ACES_V13_OCIO_V24,
        false
    );

    registry.addBuiltin(
        "cg-config-v4.0.0_aces-v2.0_ocio-v2.5",
        "Academy Color Encoding System - CG Config [COLORSPACES v4.0.0] [ACES v2.0] [OCIO v2.5]",
        CG_CONFIG_V400_ACES_V20_OCIO_V25,
        true
    );
}

} // namespace CGCONFIG
} // namespace OCIO_NAMESPACE
