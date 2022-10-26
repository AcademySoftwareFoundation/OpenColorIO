// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>
#include "builtinconfigs/BuiltinConfigRegistry.h"
#include "builtinconfigs/StudioConfig.h"

#include "Studio.cpp"

namespace OCIO_NAMESPACE
{
// Create the built-in configs for all versions of the OCIO Studio config for ACES.
// For backwards compatibility, previous versions are kept in the registry but the
// isRecommended flag should be set to false.

namespace STUDIOCONFIG
{
void Register(BuiltinConfigRegistryImpl & registry) noexcept
{
    registry.addBuiltin(
        "studio-config-v1.0.0_aces-v1.3_ocio-v2.1",
        "Academy Color Encoding System - Studio Config [COLORSPACES v1.0.0] [ACES v1.3] [OCIO v2.1]",
        STUDIO_CONFIG_V100_ACES_V13_OCIO_V21,
        true
    );
}

} // namespace STUDIOCONFIG
} // namespace OCIO_NAMESPACE
