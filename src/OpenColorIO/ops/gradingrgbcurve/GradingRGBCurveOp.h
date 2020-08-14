// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGRGBCURVE_OP_H
#define INCLUDED_OCIO_GRADINGRGBCURVE_OP_H


#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/gradingrgbcurve/GradingRGBCurveOpData.h"


namespace OCIO_NAMESPACE
{

void CreateGradingRGBCurveOp(OpRcPtrVec & ops,
                             GradingRGBCurveOpDataRcPtr & gpData,
                             TransformDirection direction);

// Create a copy of the rgb curve transform in the op and append it to
// the GroupTransform.
void CreateGradingRGBCurveTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

} // namespace OCIO_NAMESPACE

#endif
