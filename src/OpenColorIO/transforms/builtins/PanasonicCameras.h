// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_PANASONIC_CAMERAS_H
#define INCLUDED_OCIO_PANASONIC_CAMERAS_H


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

class BuiltinTransformRegistryImpl;

namespace CAMERA
{

namespace PANASONIC
{

void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept;

} // namespace PANASONIC

} // namespace CAMERA

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PANASONIC_CAMERAS_H
