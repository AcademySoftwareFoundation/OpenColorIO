// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_FILERULES_H
#define INCLUDED_OCIO_FILERULES_H

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

namespace FileRuleUtils
{

// Reserved rule names.
constexpr char DefaultName[] { "Default" };
constexpr char ParseName[]   { "ColorSpaceNamePathSearch" };

constexpr char Name[]       { "name" };
constexpr char ColorSpace[] { "colorspace" };
constexpr char Pattern[]    { "pattern" };
constexpr char Extension[]  { "extension" };
constexpr char Regex[]      { "regex" };
constexpr char CustomKey[]  { "custom" };

}

} // namespace OCIO_NAMESPACE

#endif
