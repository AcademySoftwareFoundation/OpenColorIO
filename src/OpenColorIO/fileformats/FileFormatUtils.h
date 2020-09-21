// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_FILEFORMAT_UTILS_H
#define INCLUDED_OCIO_FILEFORMAT_UTILS_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/lut1d/Lut1DOpData.h"
#include "ops/lut3d/Lut3DOpData.h"

namespace OCIO_NAMESPACE
{
Lut1DOpDataRcPtr HandleLUT1D(const Lut1DOpDataRcPtr & fileLut1D,
                             Interpolation fileInterp,
                             Interpolation cachedInterp,
                             bool & fileInterpUsed);

Lut3DOpDataRcPtr HandleLUT3D(const Lut3DOpDataRcPtr & fileLut1D,
                             Interpolation fileInterp,
                             Interpolation cachedInterp,
                             bool & fileInterpUsed);

void LogWarningInterpolationNotUsed(Interpolation interp, const FileTransform & fileTransform);

TransformDirection CombineDirections(TransformDirection dir, const FileTransform & fileTransform);
} // OCIO_NAMESPACE

#endif // INCLUDED_OCIO_FILEFORMAT_UTILS_H