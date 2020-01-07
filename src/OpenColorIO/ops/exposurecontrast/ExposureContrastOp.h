// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_EXPOSURECONTRAST_H
#define INCLUDED_OCIO_EXPOSURECONTRAST_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/exposurecontrast/ExposureContrastOpData.h"

namespace OCIO_NAMESPACE
{

void CreateExposureContrastOp(OpRcPtrVec & ops,
                              ExposureContrastOpDataRcPtr & data,
                              TransformDirection direction);

// Create a copy of the exposure contrast transform in the op and append it to
// the GroupTransform.
void CreateExposureContrastTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

} // namespace OCIO_NAMESPACE

#endif
