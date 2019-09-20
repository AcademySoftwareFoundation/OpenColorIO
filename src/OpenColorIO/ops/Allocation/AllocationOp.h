// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_ALLOCATIONOP_H
#define INCLUDED_OCIO_ALLOCATIONOP_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"

OCIO_NAMESPACE_ENTER
{
    void CreateAllocationOps(OpRcPtrVec & ops,
                             const AllocationData & data,
                             TransformDirection dir);
}
OCIO_NAMESPACE_EXIT

#endif
