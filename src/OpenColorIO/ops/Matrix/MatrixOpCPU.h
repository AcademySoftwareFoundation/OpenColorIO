// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_CPU_MATRIXOP
#define INCLUDED_OCIO_CPU_MATRIXOP

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/Matrix/MatrixOpData.h"

OCIO_NAMESPACE_ENTER
{

ConstOpCPURcPtr GetMatrixRenderer(ConstMatrixOpDataRcPtr & mat);

}
OCIO_NAMESPACE_EXIT

#endif