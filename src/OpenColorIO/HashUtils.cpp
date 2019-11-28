// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "HashUtils.h"
#include "md5/md5.h"

#include <sstream>
#include <iostream>

namespace OCIO_NAMESPACE
{
std::string CacheIDHash(const char * array, int size)
{
    md5_state_t state;
    md5_byte_t digest[16];

    md5_init(&state);
    md5_append(&state, (const md5_byte_t *)array, size);
    md5_finish(&state, digest);

    return GetPrintableHash(digest);
}

std::string GetPrintableHash(const md5_byte_t * digest)
{
    static char charmap[] = "0123456789abcdef";

    char printableResult[34];
    char *ptr = printableResult;

    // build a printable string from unprintable chars.  first character
    // of hashed cache IDs is '$', to later check if it's already been hashed
    *ptr++ = '$';
    for (int i=0;i<16;++i)
    {
        *ptr++ = charmap[(digest[i] & 0x0F)];
        *ptr++ = charmap[(digest[i] >> 4)];
    }
    *ptr++ = 0;

    return std::string(printableResult);
}

} // namespace OCIO_NAMESPACE
