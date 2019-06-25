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


#ifndef INCLUDED_OCIO_LUT1DOP_H
#define INCLUDED_OCIO_LUT1DOP_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Lut1D/Lut1DOpData.h"

OCIO_NAMESPACE_ENTER
{
    // Legacy struct, still used by file formats, will eventually go away.
    struct Lut1D;
    typedef OCIO_SHARED_PTR<Lut1D> Lut1DRcPtr;

    struct Lut1D
    {
        enum ErrorType
        {
            ERROR_ABSOLUTE = 1,
            ERROR_RELATIVE
        };   
    
        static Lut1DRcPtr Create();
        static Lut1DRcPtr CreateIdentity(BitDepth inputBitDepth, BitDepth outBitDepth);
        
        // This will compute the cacheid, and also
        // determine if the LUT is a no-op.
        // If this LUT is being read in from float ASCII text
        // a value of 1e-5 is preferable.
        // If this LUT is being read in from integer ASCII
        // representation, the value will depend on the LSB
        // at the specified integer precision.
        // Example: reading 10-bit ints? Use 2/1023.0
        // If you dont want to do the noop computation,
        // specify a 0.0 tolerance.
        //
        // TODO: Instead of having each user compute the error
        // individually, maybe they should specify the original file bitdepth?
        // (with appropriate precision tokens?)
        float maxerror;
        ErrorType errortype;
        
        float from_min[3];
        float from_max[3];
        
        typedef std::vector<float> fv_t;
        fv_t luts[3];

        BitDepth inputBitDepth;
        BitDepth outputBitDepth;

        std::string getCacheID() const;
        bool isNoOp() const;

        void unfinalize();

        Lut1D & operator=(const Lut1D & l);

        Lut1D();

        ~Lut1D() {}

        mutable std::string m_cacheID;
        mutable bool m_isNoOp;
        mutable Mutex m_mutex;

        void finalize() const;
    };
    
    // This generates an identity 1D LUT, from 0.0 to 1.0
    void GenerateIdentityLut1D(float* img, int numElements, int numChannels);
    
    void CreateLut1DOp(OpRcPtrVec & ops,
                       Lut1DRcPtr & lut,
                       Interpolation interpolation,
                       TransformDirection direction);

    void CreateLut1DOp(OpRcPtrVec & ops,
                       Lut1DOpDataRcPtr & lut,
                       TransformDirection direction);

}
OCIO_NAMESPACE_EXIT

#endif
