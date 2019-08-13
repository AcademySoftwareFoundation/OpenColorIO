// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_RANGE_H
#define INCLUDED_OCIO_RANGE_H


#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/Range/RangeOpData.h"


OCIO_NAMESPACE_ENTER
{
    
// Create a range op from its input and output bounds.
void CreateRangeOp(OpRcPtrVec & ops, 
                   double minInValue, double maxInValue,
                   double minOutValue, double maxOutValue);

void CreateRangeOp(OpRcPtrVec & ops, 
                   double minInValue, double maxInValue,
                   double minOutValue, double maxOutValue,
                   TransformDirection direction);

// Create a range op from an OpData Range.
void CreateRangeOp(OpRcPtrVec & ops, 
                   RangeOpDataRcPtr & rangeData,
                   TransformDirection direction);

}
OCIO_NAMESPACE_EXIT

#endif
