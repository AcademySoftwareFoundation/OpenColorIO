// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_APPLE_CAMERAS_H
#define INCLUDED_OCIO_APPLE_CAMERAS_H

#include <OpenColorIO/OpenColorIO.h>

#if OCIO_LUT_AND_FILETRANSFORM_SUPPORT

namespace OCIO_NAMESPACE
{

class BuiltinTransformRegistryImpl;

namespace CAMERA
{

namespace APPLE
{

void RegisterAll(BuiltinTransformRegistryImpl & registry) noexcept;

} // namespace APPLE

} // namespace CAMERA

} // namespace OCIO_NAMESPACE
#endif // OCIO_LUT_AND_FILETRANSFORM_SUPPORT

#endif // INCLUDED_OCIO_APPLE_CAMERAS_H
