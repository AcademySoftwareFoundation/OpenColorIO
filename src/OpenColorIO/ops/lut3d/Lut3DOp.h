// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LUT3DOP_H
#define INCLUDED_OCIO_LUT3DOP_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/lut3d/Lut3DOpData.h"

namespace OCIO_NAMESPACE
{
// RGB channel ordering.
// LUT entries ordered in such a way that the red coordinate changes fastest,
// then the green coordinate, and finally, the blue coordinate changes slowest

inline int GetLut3DIndex_RedFast(int indexR, int indexG, int indexB,
    int sizeR,  int sizeG,  int /*sizeB*/)
{
    return 3 * (indexR + sizeR * (indexG + sizeG * indexB));
}


// RGB channel ordering.
// LUT entries ordered in such a way that the blue coordinate changes fastest,
// then the green coordinate, and finally, the red coordinate changes slowest

inline int GetLut3DIndex_BlueFast(int indexR, int indexG, int indexB,
                                    int /*sizeR*/,  int sizeG,  int sizeB)
{
    return 3 * (indexB + sizeB * (indexG + sizeG * indexR));
}

// What is the preferred order for the 3D LUT?
// I.e., are the first two entries change along
// the blue direction, or the red direction?
// OpenGL expects 'red'

enum Lut3DOrder
{
    LUT3DORDER_FAST_RED = 0,
    LUT3DORDER_FAST_BLUE
};

void GenerateIdentityLut3D(float* img, int edgeLen, int numChannels,
                            Lut3DOrder lut3DOrder);

// Essentially the cube root, but will throw an exception if the
// cube root is not exact.
int Get3DLutEdgeLenFromNumPixels(int numPixels);

void CreateLut3DOp(OpRcPtrVec & ops,
                    Lut3DOpDataRcPtr & lut,
                    TransformDirection direction);

// Create a Lut3DTransform decoupled from op and append it to the GroupTransform.
void CreateLut3DTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

} // namespace OCIO_NAMESPACE

#endif
