// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LUT3DOPCPU
#define INCLUDED_OCIO_LUT3DOPCPU

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/Lut3D/Lut3DOpData.h"

OCIO_NAMESPACE_ENTER
{

ConstOpCPURcPtr GetLut3DRenderer(ConstLut3DOpDataRcPtr & lut);

}
OCIO_NAMESPACE_EXIT


#endif