// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "HashUtils.h"

#define XXH_STATIC_LINKING_ONLY   /* access advanced declarations */
#define XXH_IMPLEMENTATION   /* access definitions */
#include "xxhash.h"


namespace OCIO_NAMESPACE
{

std::string CacheIDHash(const char * array, std::size_t size)
{
    auto hash = XXH3_64bits(array, size);

    std::stringstream oss;
    oss << std::hex << hash;
    return oss.str();
}

} // namespace OCIO_NAMESPACE
