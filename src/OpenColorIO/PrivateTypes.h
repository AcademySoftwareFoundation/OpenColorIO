// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_PRIVATE_TYPES_H
#define INCLUDED_OCIO_PRIVATE_TYPES_H

#include <OpenColorIO/OpenColorIO.h>

#include <map>
#include <set>
#include <vector>


namespace OCIO_NAMESPACE
{
// Stl types of OCIO classes
typedef std::map<std::string, std::string> StringMap;
typedef std::map<std::string, bool> StringBoolMap;
typedef std::set<std::string> StringSet;

typedef std::vector<ConstTransformRcPtr> ConstTransformVec;
typedef std::vector<LookRcPtr> LookVec;

typedef std::vector<TransformDirection> TransformDirectionVec;



} // namespace OCIO_NAMESPACE

#endif
