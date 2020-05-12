// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_ACES_H
#define INCLUDED_OCIO_ACES_H


#include <OpenColorIO/OpenColorIO.h>

#include "transforms/builtins/ColorMatrixHelpers.h"


namespace OCIO_NAMESPACE
{

class BuiltinTransformRegistryImpl;

namespace ACES
{

void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept;

} // namespace ACES

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_ACES_H
