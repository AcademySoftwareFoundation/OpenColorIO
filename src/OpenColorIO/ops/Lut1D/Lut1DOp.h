// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


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
