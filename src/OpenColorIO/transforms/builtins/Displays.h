// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_DISPLAYS_TRANSFORMS_H
#define INCLUDED_OCIO_DISPLAYS_TRANSFORMS_H


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

class BuiltinTransformRegistryImpl;

namespace DISPLAY
{

void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept;

} // namespace DISPLAY

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_DISPLAYS_TRANSFORMS_H
