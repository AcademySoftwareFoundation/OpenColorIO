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
    registry.addBuiltin(
        "cg-config-v1.0.0_aces-v1.3_ocio-v2.1",
        "Academy Color Encoding System - CG Config [COLORSPACES v1.0.0] [ACES v1.3] [OCIO v2.1]",
        CG_CONFIG_V100_ACES_V13_OCIO_V21,
        true
    );
}

} // namespace CGCONFIG
} // namespace OCIO_NAMESPACE
