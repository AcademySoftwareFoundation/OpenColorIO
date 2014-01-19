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

#include <cstring>

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

    bool IsScalarEqualToZeroFlt(double v)
    {
        return equalWithAbsError(float(v), 0.0f, FLTMIN);
    }

    bool IsScalarEqualToOne(float v)
    {
        return equalWithAbsError(v, 1.0f, FLTMIN);
    }
    
    bool IsScalarEqualToOneFlt(double v)
    {
        return equalWithAbsError(float(v), 1.0f, FLTMIN);
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

    bool IsVecEqualToOneFlt(const double* v, int size)
    {
        for(int i=0; i<size; ++i)
        {
            if(!IsScalarEqualToOneFlt(v[i])) return false;
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
    
    // We use an intermediate double representation to make sure
    // there is minimal float precision error on the determinant's computation
    // (We have seen IsScalarEqualToZero sensitivities here on 32-bit
    // virtual machines)
    
    bool GetM44Inverse(float* inverse_out, const float* m_)
    {
        double m[16];
        for(unsigned int i=0; i<16; ++i) m[i] = (double)m_[i];
        
        double d10_21 = m[4]*m[9] - m[5]*m[8];
        double d10_22 = m[4]*m[10] - m[6]*m[8];
        double d10_23 = m[4]*m[11] - m[7]*m[8];
        double d11_22 = m[5]*m[10] - m[6]*m[9];
        double d11_23 = m[5]*m[11] - m[7]*m[9];
        double d12_23 = m[6]*m[11] - m[7]*m[10];
        
        double a00 = m[13]*d12_23 - m[14]*d11_23 + m[15]*d11_22;
        double a10 = m[14]*d10_23 - m[15]*d10_22 - m[12]*d12_23;
        double a20 = m[12]*d11_23 - m[13]*d10_23 + m[15]*d10_21;
        double a30 = m[13]*d10_22 - m[14]*d10_21 - m[12]*d11_22;
        
        double det = a00*m[0] + a10*m[1] + a20*m[2] + a30*m[3];
        
        if(IsScalarEqualToZero((float)det)) return false;
        
        det = 1.0/det;
        
        double d00_31 = m[0]*m[13] - m[1]*m[12];
        double d00_32 = m[0]*m[14] - m[2]*m[12];
        double d00_33 = m[0]*m[15] - m[3]*m[12];
        double d01_32 = m[1]*m[14] - m[2]*m[13];
        double d01_33 = m[1]*m[15] - m[3]*m[13];
        double d02_33 = m[2]*m[15] - m[3]*m[14];
        
        double a01 = m[9]*d02_33 - m[10]*d01_33 + m[11]*d01_32;
        double a11 = m[10]*d00_33 - m[11]*d00_32 - m[8]*d02_33;
        double a21 = m[8]*d01_33 - m[9]*d00_33 + m[11]*d00_31;
        double a31 = m[9]*d00_32 - m[10]*d00_31 - m[8]*d01_32;
        
        double a02 = m[6]*d01_33 - m[7]*d01_32 - m[5]*d02_33;
        double a12 = m[4]*d02_33 - m[6]*d00_33 + m[7]*d00_32;
        double a22 = m[5]*d00_33 - m[7]*d00_31 - m[4]*d01_33;
        double a32 = m[4]*d01_32 - m[5]*d00_32 + m[6]*d00_31;
        
        double a03 = m[2]*d11_23 - m[3]*d11_22 - m[1]*d12_23;
        double a13 = m[0]*d12_23 - m[2]*d10_23 + m[3]*d10_22;
        double a23 = m[1]*d10_23 - m[3]*d10_21 - m[0]*d11_23;
        double a33 = m[0]*d11_22 - m[1]*d10_22 + m[2]*d10_21;
        
        inverse_out[0] = (float) (a00*det);
        inverse_out[1] = (float) (a01*det);
        inverse_out[2] = (float) (a02*det);
        inverse_out[3] = (float) (a03*det);
        inverse_out[4] = (float) (a10*det);
        inverse_out[5] = (float) (a11*det);
        inverse_out[6] = (float) (a12*det);
        inverse_out[7] = (float) (a13*det);
        inverse_out[8] = (float) (a20*det);
        inverse_out[9] = (float) (a21*det);
        inverse_out[10] = (float) (a22*det);
        inverse_out[11] = (float) (a23*det);
        inverse_out[12] = (float) (a30*det);
        inverse_out[13] = (float) (a31*det);
        inverse_out[14] = (float) (a32*det);
        inverse_out[15] = (float) (a33*det);
        
        return true;
    }
    
    void GetM44M44Product(float* mout, const float* m1_, const float* m2_)
    {
        float m1[16];
        float m2[16];
        memcpy(m1, m1_, 16*sizeof(float));
        memcpy(m2, m2_, 16*sizeof(float));
        
        mout[ 0] = m1[ 0]*m2[0] + m1[ 1]*m2[4] + m1[ 2]*m2[ 8] + m1[ 3]*m2[12];
        mout[ 1] = m1[ 0]*m2[1] + m1[ 1]*m2[5] + m1[ 2]*m2[ 9] + m1[ 3]*m2[13];
        mout[ 2] = m1[ 0]*m2[2] + m1[ 1]*m2[6] + m1[ 2]*m2[10] + m1[ 3]*m2[14];
        mout[ 3] = m1[ 0]*m2[3] + m1[ 1]*m2[7] + m1[ 2]*m2[11] + m1[ 3]*m2[15];
        mout[ 4] = m1[ 4]*m2[0] + m1[ 5]*m2[4] + m1[ 6]*m2[ 8] + m1[ 7]*m2[12];
        mout[ 5] = m1[ 4]*m2[1] + m1[ 5]*m2[5] + m1[ 6]*m2[ 9] + m1[ 7]*m2[13];
        mout[ 6] = m1[ 4]*m2[2] + m1[ 5]*m2[6] + m1[ 6]*m2[10] + m1[ 7]*m2[14];
        mout[ 7] = m1[ 4]*m2[3] + m1[ 5]*m2[7] + m1[ 6]*m2[11] + m1[ 7]*m2[15];   
        mout[ 8] = m1[ 8]*m2[0] + m1[ 9]*m2[4] + m1[10]*m2[ 8] + m1[11]*m2[12];
        mout[ 9] = m1[ 8]*m2[1] + m1[ 9]*m2[5] + m1[10]*m2[ 9] + m1[11]*m2[13];
        mout[10] = m1[ 8]*m2[2] + m1[ 9]*m2[6] + m1[10]*m2[10] + m1[11]*m2[14];
        mout[11] = m1[ 8]*m2[3] + m1[ 9]*m2[7] + m1[10]*m2[11] + m1[11]*m2[15];
        mout[12] = m1[12]*m2[0] + m1[13]*m2[4] + m1[14]*m2[ 8] + m1[15]*m2[12];
        mout[13] = m1[12]*m2[1] + m1[13]*m2[5] + m1[14]*m2[ 9] + m1[15]*m2[13];
        mout[14] = m1[12]*m2[2] + m1[13]*m2[6] + m1[14]*m2[10] + m1[15]*m2[14];
        mout[15] = m1[12]*m2[3] + m1[13]*m2[7] + m1[14]*m2[11] + m1[15]*m2[15];
    }
    
    namespace
    {
    
    void GetM44V4Product(float* vout, const float* m, const float* v_)
    {
        float v[4];
        memcpy(v, v_, 4*sizeof(float));
        
        vout[0] = m[ 0]*v[0] + m[ 1]*v[1] + m[ 2]*v[2] + m[ 3]*v[3];
        vout[1] = m[ 4]*v[0] + m[ 5]*v[1] + m[ 6]*v[2] + m[ 7]*v[3];
        vout[2] = m[ 8]*v[0] + m[ 9]*v[1] + m[10]*v[2] + m[11]*v[3];
        vout[3] = m[12]*v[0] + m[13]*v[1] + m[14]*v[2] + m[15]*v[3];
    }
    
    void GetV4Sum(float* vout, const float* v1, const float* v2)
    {
        for(int i=0; i<4; ++i)
        {
            vout[i] = v1[i] + v2[i];
        }
    }
    
    } // anon namespace
    
    // All m(s) are 4x4.  All v(s) are size 4 vectors.
    // Return mout, vout, where mout*x+vout == m2*(m1*x+v1)+v2
    // mout = m2*m1
    // vout = m2*v1 + v2
    void GetMxbCombine(float* mout, float* vout,
                       const float* m1_, const float* v1_,
                       const float* m2_, const float* v2_)
    {
        float m1[16];
        float v1[4];
        float m2[16];
        float v2[4];
        memcpy(m1, m1_, 16*sizeof(float));
        memcpy(v1, v1_, 4*sizeof(float));
        memcpy(m2, m2_, 16*sizeof(float));
        memcpy(v2, v2_, 4*sizeof(float));
        
        GetM44M44Product(mout, m2, m1);
        GetM44V4Product(vout, m2, v1);
        GetV4Sum(vout, vout, v2);
    }
    
    namespace
    {
    
    void GetMxbResult(float* vout, float* m, float* x, float* v)
    {
        GetM44V4Product(vout, m, x);
        GetV4Sum(vout, vout, v);
    }
    
    } // anon namespace
    
    bool GetMxbInverse(float* mout, float* vout,
                       const float* m_, const float* v_)
    {
        float m[16];
        float v[4];
        memcpy(m, m_, 16*sizeof(float));
        memcpy(v, v_, 4*sizeof(float));

        if(!GetM44Inverse(mout, m)) return false;
        
        for(int i=0; i<4; ++i)
        {
            v[i] = -v[i];
        }
        GetM44V4Product(vout, mout, v);
        
        return true;
    }
    
}

OCIO_NAMESPACE_EXIT




///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

OCIO_NAMESPACE_USING

#include "UnitTest.h"

OIIO_ADD_TEST(MathUtils, M44_is_diagonal)
{
    {
        float m44[] = { 1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f };
        bool isdiag = IsM44Diagonal(m44);
        OIIO_CHECK_EQUAL(isdiag, true);

        m44[1] += 1e-8f;
        isdiag = IsM44Diagonal(m44);
        OIIO_CHECK_EQUAL(isdiag, false);
    }
}


OIIO_ADD_TEST(MathUtils, IsScalarEqualToZero)
{
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(0.0f), true);
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(-0.0f), true);
        
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(-1.072883670794056e-09f), false);
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(1.072883670794056e-09f), false);
        
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(-1.072883670794056e-03f), false);
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(1.072883670794056e-03f), false);
        
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(-1.072883670794056e-01f), false);
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(1.072883670794056e-01f), false);
}

OIIO_ADD_TEST(MathUtils, GetM44Inverse)
{
        // This is a degenerate matrix, and shouldnt be invertible.
        float m[] = { 0.3f, 0.3f, 0.3f, 0.0f,
                      0.3f, 0.3f, 0.3f, 0.0f,
                      0.3f, 0.3f, 0.3f, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f };
        
        float mout[16];
        bool invertsuccess = GetM44Inverse(mout, m);
        OIIO_CHECK_EQUAL(invertsuccess, false);
}


OIIO_ADD_TEST(MathUtils, M44_M44_product)
{
    {
        float mout[16];
        float m1[] = { 1.0f, 2.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 1.0f, 0.0f,
                       1.0f, 0.0f, 1.0f, 0.0f,
                       0.0f, 1.0f, 3.0f, 1.0f };
        float m2[] = { 1.0f, 1.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 1.0f, 0.0f,
                       2.0f, 0.0f, 0.0f, 1.0f };
        GetM44M44Product(mout, m1, m2);
        
        float mcorrect[] = { 1.0f, 3.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 1.0f, 0.0f,
                       1.0f, 1.0f, 1.0f, 0.0f,
                       2.0f, 1.0f, 3.0f, 1.0f };
        
        for(int i=0; i<16; ++i)
        {
            OIIO_CHECK_EQUAL(mout[i], mcorrect[i]);
        }
    }
}

OIIO_ADD_TEST(MathUtils, M44_V4_product)
{
    {
        float vout[4];
        float m[] = { 1.0f, 2.0f, 0.0f, 0.0f,
                      0.0f, 1.0f, 1.0f, 0.0f,
                      1.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 1.0f, 3.0f, 1.0f };
        float v[] = { 1.0f, 2.0f, 3.0f, 4.0f };
        GetM44V4Product(vout, m, v);
        
        float vcorrect[] = { 5.0f, 5.0f, 4.0f, 15.0f };
        
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_EQUAL(vout[i], vcorrect[i]);
        }
    }
}

OIIO_ADD_TEST(MathUtils, V4_add)
{
    {
        float vout[4];
        float v1[] = { 1.0f, 2.0f, 3.0f, 4.0f };
        float v2[] = { 3.0f, 1.0f, 4.0f, 1.0f };
        GetV4Sum(vout, v1, v2);
        
        float vcorrect[] = { 4.0f, 3.0f, 7.0f, 5.0f };
        
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_EQUAL(vout[i], vcorrect[i]);
        }
    }
}

OIIO_ADD_TEST(MathUtils, mxb_eval)
{
    {
        float vout[4];
        float m[] = { 1.0f, 2.0f, 0.0f, 0.0f,
                      0.0f, 1.0f, 1.0f, 0.0f,
                      1.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 1.0f, 3.0f, 1.0f };
        float x[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float v[] = { 1.0f, 2.0f, 3.0f, 4.0f };
        GetMxbResult(vout, m, x, v);
        
        float vcorrect[] = { 4.0f, 4.0f, 5.0f, 9.0f };
        
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_EQUAL(vout[i], vcorrect[i]);
        }
    }
}

OIIO_ADD_TEST(MathUtils, Combine_two_mxb)
{
    float m1[] = { 1.0f, 0.0f, 2.0f, 0.0f,
                   2.0f, 1.0f, 0.0f, 1.0f,
                   0.0f, 1.0f, 2.0f, 0.0f,
                   1.0f, 0.0f, 0.0f, 1.0f };
    float v1[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    float m2[] = { 2.0f, 1.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 0.0f, 0.0f,
                   1.0f, 0.0f, 3.0f, 0.0f,
                   1.0f,1.0f, 1.0f, 1.0f };
    float v2[] = { 0.0f, 2.0f, 1.0f, 0.0f };
    float tolerance = 1e-9f;

    {
        float x[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float vout[4];

        // Combine two mx+b operations, and apply to test point
        float mout[16];
        float vcombined[4];      
        GetMxbCombine(mout, vout, m1, v1, m2, v2);
        GetMxbResult(vcombined, mout, x, vout);
        
        // Sequentially apply the two mx+b operations.
        GetMxbResult(vout, m1, x, v1);
        GetMxbResult(vout, m2, vout, v2);
        
        // Compare outputs
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_CLOSE(vcombined[i], vout[i], tolerance);
        }
    }
    
    {
        float x[] = { 6.0f, 0.5f, -2.0f, -0.1f };
        float vout[4];

        float mout[16];
        float vcombined[4];
        GetMxbCombine(mout, vout, m1, v1, m2, v2);
        GetMxbResult(vcombined, mout, x, vout);
        
        GetMxbResult(vout, m1, x, v1);
        GetMxbResult(vout, m2, vout, v2);
        
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_CLOSE(vcombined[i], vout[i], tolerance);
        }
    }
    
    {
        float x[] = { 26.0f, -0.5f, 0.005f, 12.1f };
        float vout[4];

        float mout[16];
        float vcombined[4];
        GetMxbCombine(mout, vout, m1, v1, m2, v2);
        GetMxbResult(vcombined, mout, x, vout);
        
        GetMxbResult(vout, m1, x, v1);
        GetMxbResult(vout, m2, vout, v2);
        
        // We pick a not so small tolerance, as we're dealing with
        // large numbers, and the error for CHECK_CLOSE is absolute.
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_CLOSE(vcombined[i], vout[i], 1e-3);
        }
    }
}

OIIO_ADD_TEST(MathUtils, mxb_invert)
{
    {
        float m[] = { 1.0f, 2.0f, 0.0f, 0.0f,
                      0.0f, 1.0f, 1.0f, 0.0f,
                      1.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 1.0f, 3.0f, 1.0f };
        float x[] = { 1.0f, 0.5f, -1.0f, 60.0f };
        float v[] = { 1.0f, 2.0f, 3.0f, 4.0f };
        
        float vresult[4];
        float mout[16];
        float vout[4];
        
        GetMxbResult(vresult, m, x, v);
        bool invertsuccess = GetMxbInverse(mout, vout, m, v);
        OIIO_CHECK_EQUAL(invertsuccess, true);
        
        GetMxbResult(vresult, mout, vresult, vout);
        
        float tolerance = 1e-9f;
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_CLOSE(vresult[i], x[i], tolerance);
        }
    }
    
    {
        float m[] = { 0.3f, 0.3f, 0.3f, 0.0f,
                      0.3f, 0.3f, 0.3f, 0.0f,
                      0.3f, 0.3f, 0.3f, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f };
        float v[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        
        float mout[16];
        float vout[4];
        
        bool invertsuccess = GetMxbInverse(mout, vout, m, v);
        OIIO_CHECK_EQUAL(invertsuccess, false);
    }
}

#endif

