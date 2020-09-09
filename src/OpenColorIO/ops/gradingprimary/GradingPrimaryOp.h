// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGPRIMARY_OP_H
#define INCLUDED_OCIO_GRADINGPRIMARY_OP_H


#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/gradingprimary/GradingPrimaryOpData.h"


namespace OCIO_NAMESPACE
{

void CreateGradingPrimaryOp(OpRcPtrVec & ops,
                             GradingPrimaryOpDataRcPtr & gpData,
                             TransformDirection direction);

// Create a copy of the primary transform in the op and append it to
// the GroupTransform.
void CreateGradingPrimaryTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

} // namespace OCIO_NAMESPACE

#endif
