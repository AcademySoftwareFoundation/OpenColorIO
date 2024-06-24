// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_LUT3DOP_CPU_H
#define INCLUDED_OCIO_LUT3DOP_CPU_H

#include <OpenColorIO/OpenColorIO.h>

#if OCIO_LUT_AND_FILETRANSFORM_SUPPORT

#include "Op.h"
#include "ops/lut3d/Lut3DOpData.h"

namespace OCIO_NAMESPACE
{

ConstOpCPURcPtr GetLut3DRenderer(ConstLut3DOpDataRcPtr & lut);

} // namespace OCIO_NAMESPACE

#endif // OCIO_LUT_AND_FILETRANSFORM_SUPPORT

#endif