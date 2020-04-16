// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_CPU_MATRIXOP_H
#define INCLUDED_OCIO_CPU_MATRIXOP_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/matrix/MatrixOpData.h"

namespace OCIO_NAMESPACE
{

ConstOpCPURcPtr GetMatrixRenderer(ConstMatrixOpDataRcPtr & mat);

} // namespace OCIO_NAMESPACE

#endif