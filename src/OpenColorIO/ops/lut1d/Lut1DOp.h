// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LUT1DOP_H
#define INCLUDED_OCIO_LUT1DOP_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/lut1d/Lut1DOpData.h"

namespace OCIO_NAMESPACE
{
// This generates an identity 1D LUT, from 0.0 to 1.0
void GenerateIdentityLut1D(float* img, int numElements, int numChannels);

void CreateLut1DOp(OpRcPtrVec & ops,
                    Lut1DOpDataRcPtr & lut,
                    TransformDirection direction);

// Create a Lut1DTransform decoupled from op and append it to the GroupTransform.
void CreateLut1DTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

} // namespace OCIO_NAMESPACE

#endif
