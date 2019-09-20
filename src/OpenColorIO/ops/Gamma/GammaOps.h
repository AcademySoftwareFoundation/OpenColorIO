// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GAMMA_H
#define INCLUDED_OCIO_GAMMA_H


#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/Gamma/GammaOpData.h"


OCIO_NAMESPACE_ENTER
{

    void CreateGammaOp(OpRcPtrVec & ops,
                       const FormatMetadataImpl & info,
                       GammaOpData::Style style,
                       const double * gamma4,
                       const double * offset4);

    void CreateGammaOp(OpRcPtrVec & ops, 
                       GammaOpDataRcPtr & gammaData,
                       TransformDirection direction);

    // Create a copy of the gamma transform in the op and append it to the GroupTransform.
    void CreateGammaTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

}
OCIO_NAMESPACE_EXIT

#endif
