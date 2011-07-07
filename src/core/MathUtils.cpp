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

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"


OCIO_NAMESPACE_ENTER
{
    namespace
    {
        const float FLTMIN = std::numeric_limits<float>::min();
    }
    
    bool IsScalarEqualToZero(float v)
    {
        return equalWithAbsError(v, 0.0f, FLTMIN);
    }
    
    bool IsScalarEqualToOne(float v)
    {
        return equalWithAbsError(v, 1.0f, FLTMIN);
    }
    
    float GetSafeScalarInverse(float v, float defaultValue)
    {
        if(IsScalarEqualToZero(v)) return defaultValue;
        return 1.0f / v;
    }
    
    bool IsVecEqualToZero(const float* v, int size)
    {
        for(int i=0; i<size; ++i)
        {
            if(!IsScalarEqualToZero(v[i])) return false;
        }
        return true;
    }
    
    bool IsVecEqualToOne(const float* v, int size)
    {
        for(int i=0; i<size; ++i)
        {
            if(!IsScalarEqualToOne(v[i])) return false;
        }
        return true;
    }
    
    bool VecContainsZero(const float* v, int size)
    {
        for(int i=0; i<size; ++i)
        {
            if(IsScalarEqualToZero(v[i])) return true;
        }
        return false;
    }
    
    bool VecContainsOne(const float* v, int size)
    {
        for(int i=0; i<size; ++i)
        {
            if(IsScalarEqualToOne(v[i])) return true;
        }
        return false;
    }
    
    bool VecsEqualWithRelError(const float* v1, int size1,
                               const float* v2, int size2,
                               float e)
    {
        if(size1 != size2) return false;
        for(int i=0; i<size1; ++i)
        {
            if(!equalWithRelError(v1[i], v2[i], e)) return false;
        }
        
        return true;
    }
    
    double ClampToNormHalf(double val)
    {
        if(val < -GetHalfMax())
        {
            return -GetHalfMax();
        }

        if(val > -GetHalfNormMin() && val<GetHalfNormMin())
        {
            return 0.0;
        }

        if(val > GetHalfMax())
        {
            return GetHalfMax();
        }

        return val;
    }

    bool IsM44Identity(const float* m44)
    {
        int index=0;

        for(unsigned int j=0; j<4; ++j)
        {
            for(unsigned int i=0; i<4; ++i)
            {
                index = 4*j+i;

                if(i==j)
                {
                    if(!IsScalarEqualToOne(m44[index])) return false;
                }
                else
                {
                    if(!IsScalarEqualToZero(m44[index])) return false;
                }
            }
        }
        
        return true;
    }
    
    bool IsM44Diagonal(const float* m44)
    {
        for(int i=0; i<16; ++i)
        {
            if((i%5)==0) continue; // If we're on the diagonal, skip it
            if(!IsScalarEqualToZero(m44[i])) return false;
        }
        
        return true;
    }
    
    void GetM44Diagonal(float* out4, const float* m44)
    {
        for(int i=0; i<4; ++i)
        {
            out4[i] = m44[i*5];
        }
    }
    
    
    bool GetM44Inverse(float* inverse_out, const float* m)
    {
        float d10_21 = m[4]*m[9] - m[5]*m[8];
        float d10_22 = m[4]*m[10] - m[6]*m[8];
        float d10_23 = m[4]*m[11] - m[7]*m[8];
        float d11_22 = m[5]*m[10] - m[6]*m[9];
        float d11_23 = m[5]*m[11] - m[7]*m[9];
        float d12_23 = m[6]*m[11] - m[7]*m[10];
        
        float a00 = m[13]*d12_23 - m[14]*d11_23 + m[15]*d11_22;
        float a10 = m[14]*d10_23 - m[15]*d10_22 - m[12]*d12_23;
        float a20 = m[12]*d11_23 - m[13]*d10_23 + m[15]*d10_21;
        float a30 = m[13]*d10_22 - m[14]*d10_21 - m[12]*d11_22;
        
        float det = a00*m[0] + a10*m[1] + a20*m[2] + a30*m[3];
        
        if(equalWithAbsError(0.0, det, FLTMIN)) return false;
        
        det = 1.0f/det;
        
        float d00_31 = m[0]*m[13] - m[1]*m[12];
        float d00_32 = m[0]*m[14] - m[2]*m[12];
        float d00_33 = m[0]*m[15] - m[3]*m[12];
        float d01_32 = m[1]*m[14] - m[2]*m[13];
        float d01_33 = m[1]*m[15] - m[3]*m[13];
        float d02_33 = m[2]*m[15] - m[3]*m[14];
        
        float a01 = m[9]*d02_33 - m[10]*d01_33 + m[11]*d01_32;
        float a11 = m[10]*d00_33 - m[11]*d00_32 - m[8]*d02_33;
        float a21 = m[8]*d01_33 - m[9]*d00_33 + m[11]*d00_31;
        float a31 = m[9]*d00_32 - m[10]*d00_31 - m[8]*d01_32;
        
        float a02 = m[6]*d01_33 - m[7]*d01_32 - m[5]*d02_33;
        float a12 = m[4]*d02_33 - m[6]*d00_33 + m[7]*d00_32;
        float a22 = m[5]*d00_33 - m[7]*d00_31 - m[4]*d01_33;
        float a32 = m[4]*d01_32 - m[5]*d00_32 + m[6]*d00_31;
        
        float a03 = m[2]*d11_23 - m[3]*d11_22 - m[1]*d12_23;
        float a13 = m[0]*d12_23 - m[2]*d10_23 + m[3]*d10_22;
        float a23 = m[1]*d10_23 - m[3]*d10_21 - m[0]*d11_23;
        float a33 = m[0]*d11_22 - m[1]*d10_22 + m[2]*d10_21;
        
        inverse_out[0] = a00*det;
        inverse_out[1] = a01*det;
        inverse_out[2] = a02*det;
        inverse_out[3] = a03*det;
        inverse_out[4] = a10*det;
        inverse_out[5] = a11*det;
        inverse_out[6] = a12*det;
        inverse_out[7] = a13*det;
        inverse_out[8] = a20*det;
        inverse_out[9] = a21*det;
        inverse_out[10] = a22*det;
        inverse_out[11] = a23*det;
        inverse_out[12] = a30*det;
        inverse_out[13] = a31*det;
        inverse_out[14] = a32*det;
        inverse_out[15] = a33*det;
        
        return true;
    }


}
OCIO_NAMESPACE_EXIT
