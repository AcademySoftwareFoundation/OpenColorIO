// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_HUECURVE_OP_H
#define INCLUDED_OCIO_HUECURVE_OP_H


#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/gradinghuecurve/GradingHueCurveOpData.h"


namespace OCIO_NAMESPACE
{

void CreateHueCurveOp(OpRcPtrVec & ops,
                             HueCurveOpDataRcPtr & gpData,
                             TransformDirection direction);

// Create a copy of the rgb curve transform in the op and append it to
// the GroupTransform.
void CreateHueCurveTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

} // namespace OCIO_NAMESPACE

#endif
