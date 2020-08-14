// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGTONE_OP_H
#define INCLUDED_OCIO_GRADINGTONE_OP_H


#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/gradingtone/GradingToneOpData.h"


namespace OCIO_NAMESPACE
{

void CreateGradingToneOp(OpRcPtrVec & ops,
                         GradingToneOpDataRcPtr & gtData,
                         TransformDirection direction);

// Create a copy of the tone transform in the op and append it to
// the GroupTransform.
void CreateGradingToneTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

} // namespace OCIO_NAMESPACE

#endif
