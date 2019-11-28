// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CDL_H
#define INCLUDED_OCIO_CDL_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/cdl/CDLOpData.h"


namespace OCIO_NAMESPACE
{

// Create a CDL op from its parameters and append it to ops.
void CreateCDLOp(OpRcPtrVec & ops,
                 const FormatMetadataImpl & info,
                 CDLOpData::Style style,
                 const double * slope3,
                 const double * offset3,
                 const double * power3,
                 double saturation,
                 TransformDirection direction);

// Create a CDL op from the CDL OpData and append it to ops. FormatMetadata is preserved.
void CreateCDLOp(OpRcPtrVec & ops, 
                 CDLOpDataRcPtr & cdlData,
                 TransformDirection direction);

// Create a copy of the CDL transform in the op and append it to the GroupTransform.
void CreateCDLTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

} // namespace OCIO_NAMESPACE

#endif
