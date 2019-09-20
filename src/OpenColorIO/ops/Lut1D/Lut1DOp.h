// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LUT1DOP_H
#define INCLUDED_OCIO_LUT1DOP_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Lut1D/Lut1DOpData.h"

OCIO_NAMESPACE_ENTER
{
    // This generates an identity 1D LUT, from 0.0 to 1.0
    void GenerateIdentityLut1D(float* img, int numElements, int numChannels);

    void CreateLut1DOp(OpRcPtrVec & ops,
                       Lut1DOpDataRcPtr & lut,
                       TransformDirection direction);

    // Create a LUT1DTransform decoupled from op and append it to the GroupTransform.
    void CreateLut1DTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

}
OCIO_NAMESPACE_EXIT

#endif
