// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_EXPOSURECONTRAST_H
#define INCLUDED_OCIO_EXPOSURECONTRAST_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/exposurecontrast/ExposureContrastOpData.h"

OCIO_NAMESPACE_ENTER
{

void CreateExposureContrastOp(OpRcPtrVec & ops,
                              ExposureContrastOpDataRcPtr & data,
                              TransformDirection direction);

}
OCIO_NAMESPACE_EXIT

#endif
