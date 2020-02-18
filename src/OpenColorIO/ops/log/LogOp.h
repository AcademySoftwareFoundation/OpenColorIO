// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LOGOPS_H
#define INCLUDED_OCIO_LOGOPS_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/log/LogOpData.h"

namespace OCIO_NAMESPACE
{
// output = logSlope * log( linSlope * input + linOffset, base ) + logOffset
// This does not affect alpha.
// In the forward direction this is lin->log.
// All input vectors are size 3 (excluding base).

void CreateLogOp(OpRcPtrVec & ops,
                 double base,
                 const double(&logSlope)[3],
                 const double(&logOffset)[3],
                 const double(&linSlope)[3],
                 const double(&linOffset)[3],
                 TransformDirection direction);

void CreateLogOp(OpRcPtrVec & ops, double base, TransformDirection direction);

void CreateLogOp(OpRcPtrVec & ops,
                 LogOpDataRcPtr & logData,
                 TransformDirection direction);

// Create a copy of the log transform in the op and append it to the GroupTransform.
void CreateLogTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

} // namespace OCIO_NAMESPACE

#endif
