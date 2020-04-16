// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_ALLOCATIONOP_H
#define INCLUDED_OCIO_ALLOCATIONOP_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"

namespace OCIO_NAMESPACE
{
void CreateAllocationOps(OpRcPtrVec & ops,
                            const AllocationData & data,
                            TransformDirection dir);

} // namespace OCIO_NAMESPACE

#endif
