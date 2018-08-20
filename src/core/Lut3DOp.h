/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef INCLUDED_OCIO_LUT3DOP_H
#define INCLUDED_OCIO_LUT3DOP_H

#include <OpenColorIO/OpenColorIO.h>

#include "Mutex.h"
#include "Op.h"
#include "opdata/OpDataLut3D.h"
#include "opdata/OpDataInvLut3D.h"

#include <vector>

OCIO_NAMESPACE_ENTER
{
    // TODO: turn into a class instead of a struct?
    
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
    // cuberoot is not exact.
    int Get3DLutEdgeLenFromNumPixels(int numPixels);
    
    void CreateLut3DOp(OpRcPtrVec & ops,
                       Lut3DRcPtr lut,
                       Interpolation interpolation,
                       TransformDirection direction);

    void CreateLut3DOp(OpRcPtrVec & ops,
                       OpData::OpDataLut3DRcPtr & lut,
                       TransformDirection direction);

    void CreateInvLut3DOp(OpRcPtrVec & ops,
                          OpData::OpDataInvLut3DRcPtr & lut,
                          TransformDirection direction);

}
OCIO_NAMESPACE_EXIT

#endif
