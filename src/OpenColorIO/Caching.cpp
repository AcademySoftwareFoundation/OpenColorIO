// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "Caching.h"
#include "transforms/CDLTransform.h"
#include "PathUtils.h"
#include "transforms/FileTransform.h"


namespace OCIO_NAMESPACE
{

const char * OCIO_DISABLE_ALL_CACHES       = "OCIO_DISABLE_ALL_CACHES";
const char * OCIO_DISABLE_PROCESSOR_CACHES = "OCIO_DISABLE_PROCESSOR_CACHES";
const char * OCIO_DISABLE_CACHE_FALLBACK   = "OCIO_DISABLE_CACHE_FALLBACK";


// TODO: Processors which the user hangs onto have local caches.
// Should these be cleared?

void ClearAllCaches()
{
    ClearPathCaches();
    ClearFileTransformCaches();
}
} // namespace OCIO_NAMESPACE
