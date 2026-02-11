// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_HASHUTILS_H
#define INCLUDED_OCIO_HASHUTILS_H

#include <OpenColorIO/OpenColorIO.h>

#include <string>

namespace OCIO_NAMESPACE
{

std::string CacheIDHash(const char * array, std::size_t size);

// Generates 128 bit UUID in the form of 8-4-4-4-12 using the hash of the passed
// string.
std::string CacheIDHashUUID(const char * array, std::size_t size);

} // namespace OCIO_NAMESPACE

#endif
