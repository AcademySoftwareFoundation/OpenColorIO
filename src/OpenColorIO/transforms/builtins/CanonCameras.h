// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CANON_CAMERAS_H
#define INCLUDED_OCIO_CANON_CAMERAS_H


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

class BuiltinTransformRegistryImpl;

namespace CAMERA
{

namespace CANON
{

void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept;

} // namespace CANON

} // namespace CAMERA

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_CANON_CAMERAS_H
