// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include <iomanip>

#include <OpenColorIO/OpenColorIO.h>

#include "HashUtils.h"

#define XXH_INLINE_ALL
#define XXH_STATIC_LINKING_ONLY   /* access advanced declarations */
#define XXH_IMPLEMENTATION   /* access definitions */
#include "xxhash.h"


namespace OCIO_NAMESPACE
{

std::string CacheIDHash(const char * array, std::size_t size)
{
    XXH128_hash_t hash = XXH3_128bits(array, size);

    std::stringstream oss;
    oss << std::hex << hash.low64 << hash.high64;
    return oss.str();
}

std::string CacheIDHashUUID(const char * array, std::size_t size)
{
    XXH128_hash_t hash = XXH3_128bits(array, size);

    // Make sure that we have full, zero-padded 32 chars.
    std::stringstream oss;
    oss << std::hex << std::setfill('0');
    oss << std::setw(16) << hash.high64;
    oss << std::setw(16) << hash.low64;

    // Format into 8-4-4-4-12 form.
    std::string hex = oss.str();    
    std::string uuid =
        hex.substr(0, 8)  + "-" +
        hex.substr(8, 4)  + "-" +
        hex.substr(12, 4) + "-" +
        hex.substr(16, 4) + "-" +
        hex.substr(20, 12);
    
    return uuid;
}


} // namespace OCIO_NAMESPACE
