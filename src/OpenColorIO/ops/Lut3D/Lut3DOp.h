// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LUT3DOP_H
#define INCLUDED_OCIO_LUT3DOP_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Lut3D/Lut3DOpData.h"

OCIO_NAMESPACE_ENTER
{
    // Temporaly restore old structure that is still used to store data
    // when loading files. After loading the structure is converted to 
    // Lut3DOpData by CreateLut3DOp.
    struct Lut3D;
    typedef OCIO_SHARED_PTR<Lut3D> Lut3DRcPtr;

    struct Lut3D
    {
        static Lut3DRcPtr Create();

        float from_min[3];
        float from_max[3];
        int size[3];

        typedef std::vector<float> fv_t;
        fv_t lut;

        std::string getCacheID() const;

    private:
        Lut3D();
        mutable std::string m_cacheID;
        mutable Mutex m_cacheidMutex;
    };

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
                       Lut3DRcPtr lut,
                       Interpolation interpolation,
                       TransformDirection direction);

    void CreateLut3DOp(OpRcPtrVec & ops,
                       Lut3DOpDataRcPtr & lut,
                       TransformDirection direction);

}
OCIO_NAMESPACE_EXIT

#endif
