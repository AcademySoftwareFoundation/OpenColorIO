// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CDL_H
#define INCLUDED_OCIO_CDL_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/CDL/CDLOpData.h"


OCIO_NAMESPACE_ENTER
{

    // Create a CDL op from its parameters.
    void CreateCDLOp(OpRcPtrVec & ops,
                     const std::string & id,
                     const OpData::Descriptions & descr,
                     CDLOpData::Style style,
                     const double * slope3,
                     const double * offset3,
                     const double * power3,
                     double saturation,
                     TransformDirection direction);
    
    // Create a CDL op from an OpData CDL.
    void CreateCDLOp(OpRcPtrVec & ops, 
                     CDLOpDataRcPtr & cdlData,
                     TransformDirection direction);

}
OCIO_NAMESPACE_EXIT

#endif
