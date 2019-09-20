// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/CDLTransform.h"
#include "PathUtils.h"
#include "transforms/FileTransform.h"

OCIO_NAMESPACE_ENTER
{
    // TODO: Processors which the user hangs onto have local caches.
    // Should these be cleared?
    
    void ClearAllCaches()
    {
        ClearPathCaches();
        ClearFileTransformCaches();
        ClearCDLTransformFileCache();
    }
}
OCIO_NAMESPACE_EXIT
