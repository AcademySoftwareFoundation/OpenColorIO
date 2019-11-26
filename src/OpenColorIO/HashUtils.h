// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_HASHUTILS_H
#define INCLUDED_OCIO_HASHUTILS_H

#include <OpenColorIO/OpenColorIO.h>

#include "md5/md5.h"
#include <string>

namespace OCIO_NAMESPACE
{
std::string CacheIDHash(const char * array, int size);

// TODO: get rid of md5.h include, make this a generic byte array
std::string GetPrintableHash(const md5_byte_t * digest);

} // namespace OCIO_NAMESPACE

#endif

