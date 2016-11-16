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

#include "HashUtils.h"
#include "Lut1DOp.h"
#include "MathUtils.h"
#include "SSE.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <iostream>

OCIO_NAMESPACE_ENTER
{
    Lut1D::Lut1D() :
        maxerror(std::numeric_limits<float>::min()),
        errortype(ERROR_RELATIVE)
    {
        for(int i=0; i<3; ++i)
        {
            from_min[i] = 0.0f;
            from_max[i] = 1.0f;
        }
    }
    
    Lut1DRcPtr Lut1D::Create()
    {
        return Lut1DRcPtr(new Lut1D());
    }
    
    
    namespace
    {
        bool IsLut1DNoOp(const Lut1D & lut,
                         float maxerror,
                         ErrorType errortype)
        {
            // If tolerance not positive, skip the check.
            if(!(maxerror > 0.0)) return false;
            
            for(int channel = 0; channel<3; ++channel)
            {
                if(lut.luts[channel].size() == 0) continue;
                
                float inorm = 1.0f / (static_cast<float>(lut.luts[channel].size()) - 1.0f);
                
                float m = lut.from_max[channel] - lut.from_min[channel];
                float b = lut.from_min[channel];
                
                for(unsigned int i=0; i<lut.luts[channel].size(); ++i)
                {
                    float x = static_cast<float>(i) * inorm;
                    float identval = m*x+b;
                    float lutval = lut.luts[channel][i];
                    
                    if(errortype == ERROR_ABSOLUTE)
                    {
                        if(!equalWithAbsError(identval, lutval, maxerror))
                        {
                            return false;
                        }
                    }
                    else if(errortype == ERROR_RELATIVE)
                    {
                        if(!equalWithRelError(identval, lutval, maxerror))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        throw Exception("Unknown error type.");
                    }
                }
            }
            
            return true;
        }
    }
    
    std::string Lut1D::getCacheID() const
    {
        AutoMutex lock(m_mutex);
        
        if(luts[0].empty() || luts[1].empty() || luts[2].empty())
            throw Exception("Cannot compute cacheID of invalid Lut1D");
        
        if(!m_cacheID.empty())
            return m_cacheID;
        
        finalize();
        return m_cacheID;
    }
    
    bool Lut1D::isNoOp() const
    {
        AutoMutex lock(m_mutex);
        
        if(luts[0].empty() || luts[1].empty() || luts[2].empty())
            throw Exception("Cannot compute noOp of invalid Lut1D");
        
        if(!m_cacheID.empty())
            return m_isNoOp;
        
        finalize();
        
        return m_isNoOp;
    }
    
    void Lut1D::unfinalize()
    {
        AutoMutex lock(m_mutex);
        m_cacheID = "";
        m_isNoOp = false;
    }
    
    void Lut1D::finalize() const
    {
        m_isNoOp = IsLut1DNoOp(*this, maxerror, errortype);
        
        if(m_isNoOp)
        {
            m_cacheID = "<NULL 1D>";
        }
        else
        {
            md5_state_t state;
            md5_byte_t digest[16];
            
            md5_init(&state);
            md5_append(&state, (const md5_byte_t *)from_min, 3*sizeof(float));
            md5_append(&state, (const md5_byte_t *)from_max, 3*sizeof(float));
            
            for(int i=0; i<3; ++i)
            {
                md5_append( &state, (const md5_byte_t *)&(luts[i][0]),
                    (int) (luts[i].size()*sizeof(float)) );
            }
            
            md5_finish(&state, digest);
            
            m_cacheID = GetPrintableHash(digest);
        }
    }
    
    
    namespace
    {
        // Note: This function assumes that minVal is less than maxVal
        inline int clamp(float k, float minVal, float maxVal)
        {
            return static_cast<int>(roundf(std::max(std::min(k, maxVal), minVal)));
        }
        
        
        ///////////////////////////////////////////////////////////////////////
        // Nearest Forward
        
        inline float lookupNearest_1D(float index, float maxIndex, const float * simple_lut)
        {
            return simple_lut[clamp(index, 0.0f, maxIndex)];
        }
        
        void Lut1D_Nearest(float* rgbaBuffer, long numPixels, const Lut1D & lut)
        {
            float maxIndex[3];
            float mInv[3];
            float b[3];
            float mInv_x_maxIndex[3];
            const float* startPos[3];
            
            for(int i=0; i<3; ++i)
            {
                maxIndex[i] = (float) (lut.luts[i].size() - 1);
                mInv[i] = 1.0f / (lut.from_max[i] - lut.from_min[i]);
                b[i] = lut.from_min[i];
                mInv_x_maxIndex[i] = (float) (mInv[i] * maxIndex[i]);
                startPos[i] = &(lut.luts[i][0]);
            }
            
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                if(!isnan(rgbaBuffer[0]))
                    rgbaBuffer[0] = lookupNearest_1D(mInv_x_maxIndex[0] * (rgbaBuffer[0] - b[0]), maxIndex[0], startPos[0]);
                if(!isnan(rgbaBuffer[1]))
                    rgbaBuffer[1] = lookupNearest_1D(mInv_x_maxIndex[1] * (rgbaBuffer[1] - b[1]), maxIndex[1], startPos[1]);
                if(!isnan(rgbaBuffer[2]))
                    rgbaBuffer[2] = lookupNearest_1D(mInv_x_maxIndex[2] * (rgbaBuffer[2] - b[2]), maxIndex[2], startPos[2]);
                
                rgbaBuffer += 4;
            }
        }
#ifdef USE_SSE
        void Lut1D_Nearest_SSE(float* rgbaBuffer, long numPixels, const Lut1D & lut)
        {
            // orig: 546 ms
            // curr: 91 ms
            
            // These are all sized 4, to allow simpler sse loading
            float maxIndex[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            float mInv[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            float b[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            float mInv_x_maxIndex[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            const float* startPos[3];
            
            for(int i=0; i<3; ++i)
            {
                maxIndex[i] = (float) (lut.luts[i].size() - 1);
                mInv[i] = 1.0f / (lut.from_max[i] - lut.from_min[i]);
                b[i] = lut.from_min[i];
                mInv_x_maxIndex[i] = (float) (mInv[i] * maxIndex[i]);
                startPos[i] = &(lut.luts[i][0]);
            }
            
            const __m128 _zero = _mm_setzero_ps();
            const __m128 _mInv_x_maxIndex = _mm_loadu_ps(mInv_x_maxIndex);
            const __m128 _b = _mm_loadu_ps(b);
            const __m128 _maxIndex = _mm_loadu_ps(maxIndex);
            const __m128 _half = _mm_set1_ps(0.5f);
            
            float result[4];
            
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                // TODO: SSE Optimized nancheck
                
                __m128 p = _mm_loadu_ps(rgbaBuffer);
                
                // mInv_x_maxIndex * (p - b)
                p = _mm_sub_ps(p, _b);
                p = _mm_mul_ps(p, _mInv_x_maxIndex);
                
                // clamp _zero <= b <= _maxIndex
                p = _mm_max_ps(p, _zero);
                p = _mm_min_ps(p, _maxIndex);
                
                // add 0.5f for rounding
                p = _mm_add_ps(p, _half);
                
                _mm_storeu_ps(result, p);
                
                
                // TODO: use native SSE to convert to an int?
                // _mm_cvttss_si32
                // Converts the lower single-precision, floating-point value of
                // a to a 32-bit integer with truncation
                //
                // _mm_cvttps_pi32 converts 2 floats to 2 32-bit packed ints,
                // with truncation
                
                if(!isnan(result[0]))
                    rgbaBuffer[0] = startPos[0][(int)(result[0])];
                if(!isnan(result[1]))
                    rgbaBuffer[1] = startPos[1][(int)(result[1])];
                if(!isnan(result[2]))
                    rgbaBuffer[2] = startPos[2][(int)(result[2])];
                
                rgbaBuffer += 4;
            }
        }
#endif
        
        
        ///////////////////////////////////////////////////////////////////////
        // Linear Forward
        
        inline float lookupLinear_1D(float index, float maxIndex, const float * simple_lut)
        {
            int indexLow = clamp(std::floor(index), 0.0f, maxIndex);
            int indexHigh = clamp(std::ceil(index), 0.0f, maxIndex);
            float delta = index - (float)indexLow;
            return (1.0f-delta) * simple_lut[indexLow] + delta * simple_lut[indexHigh];
        }
        
        void Lut1D_Linear(float* rgbaBuffer, long numPixels, const Lut1D & lut)
        {
            float maxIndex[3];
            float mInv[3];
            float b[3];
            float mInv_x_maxIndex[3];
            const float* startPos[3];
            
            for(int i=0; i<3; ++i)
            {
                maxIndex[i] = (float) (lut.luts[i].size() - 1);
                mInv[i] = 1.0f / (lut.from_max[i] - lut.from_min[i]);
                b[i] = lut.from_min[i];
                mInv_x_maxIndex[i] = (float) (mInv[i] * maxIndex[i]);
                startPos[i] = &(lut.luts[i][0]);
            }
            
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                if(!isnan(rgbaBuffer[0]))
                    rgbaBuffer[0] = lookupLinear_1D(mInv_x_maxIndex[0] * (rgbaBuffer[0] - b[0]), maxIndex[0], startPos[0]);
                if(!isnan(rgbaBuffer[1]))
                    rgbaBuffer[1] = lookupLinear_1D(mInv_x_maxIndex[1] * (rgbaBuffer[1] - b[1]), maxIndex[1], startPos[1]);
                if(!isnan(rgbaBuffer[2]))
                    rgbaBuffer[2] = lookupLinear_1D(mInv_x_maxIndex[2] * (rgbaBuffer[2] - b[2]), maxIndex[2], startPos[2]);
                
                rgbaBuffer += 4;
            }
        }
        
        
        
        ///////////////////////////////////////////////////////////////////////
        // Nearest Inverse
        
        inline float reverseLookupNearest_1D(const float v, const float *start, const float *end)
        {
            const float *lowbound = std::lower_bound(start, end, v);
            if (lowbound != start) --lowbound;
            
            const float *highbound = lowbound;
            if (highbound < end - 1) ++highbound;
            
            // NOTE: Not dividing result by /(size-1) anymore
            if (fabsf(v - *lowbound) < fabsf(v - *highbound))
            {
                return (float)(lowbound-start);
            }
            else
            {
                return (float)(highbound-start);
            }
        }
        
        void Lut1D_NearestInverse(float* rgbaBuffer, long numPixels, const Lut1D & lut)
        {
            float m[3];
            float b[3];
            const float* startPos[3];
            const float* endPos[3];
            
            for(int i=0; i<3; ++i)
            {
                m[i] = (lut.from_max[i] - lut.from_min[i]);
                b[i] = lut.from_min[i];
                
                startPos[i] = &(lut.luts[i][0]);
                endPos[i] = startPos[i] + lut.luts[i].size();
                
                // Roll the size division into m as an optimization
                m[i] /= (float) (lut.luts[i].size() - 1);
            }
            
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                if(!isnan(rgbaBuffer[0]))
                    rgbaBuffer[0] = m[0] * reverseLookupNearest_1D(rgbaBuffer[0], startPos[0], endPos[0]) + b[0];
                if(!isnan(rgbaBuffer[1]))
                    rgbaBuffer[1] = m[1] * reverseLookupNearest_1D(rgbaBuffer[1], startPos[1], endPos[1]) + b[1];
                if(!isnan(rgbaBuffer[2]))
                    rgbaBuffer[2] = m[2] * reverseLookupNearest_1D(rgbaBuffer[2], startPos[2], endPos[2]) + b[2];
                
                rgbaBuffer += 4;
            }
        }
        
        ///////////////////////////////////////////////////////////////////////
        // Linear Inverse
        
        inline float reverseLookupLinear_1D(const float v, const float *start, const float *end, float invMaxIndex)
        {
            const float *lowbound = std::lower_bound(start, end, v);
            if (lowbound != start) --lowbound;
            
            const float *highbound = lowbound;
            if (highbound < end - 1) ++highbound;
            
            // lowbound is the lower bound, highbound is the upper bound.
            float delta = 0.0;
            if (*highbound > *lowbound)
            {
                delta = (v - *lowbound) / (*highbound - *lowbound);
            }
            
            return std::max(((float)(lowbound - start) + delta) * invMaxIndex, 0.0f);
        }
        
        void Lut1D_LinearInverse(float* rgbaBuffer, long numPixels, const Lut1D & lut)
        {
            float m[3];
            float b[3];
            const float* startPos[3];
            const float* endPos[3];
            float invMaxIndex[3];
            
            for(int i=0; i<3; ++i)
            {
                m[i] = (lut.from_max[i] - lut.from_min[i]);
                b[i] = lut.from_min[i];
                
                startPos[i] = &(lut.luts[i][0]);
                endPos[i] = startPos[i] + lut.luts[i].size();
                
                invMaxIndex[i] = 1.0f / (float) (lut.luts[i].size() - 1);
            }
            
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                if(!isnan(rgbaBuffer[0]))
                    rgbaBuffer[0] = m[0] * reverseLookupLinear_1D(rgbaBuffer[0], startPos[0], endPos[0], invMaxIndex[0]) + b[0];
                if(!isnan(rgbaBuffer[1]))
                    rgbaBuffer[1] = m[1] * reverseLookupLinear_1D(rgbaBuffer[1], startPos[1], endPos[1], invMaxIndex[0]) + b[1];
                if(!isnan(rgbaBuffer[2]))
                    rgbaBuffer[2] = m[2] * reverseLookupLinear_1D(rgbaBuffer[2], startPos[2], endPos[2], invMaxIndex[0]) + b[2];
                
                rgbaBuffer += 4;
            }
        }
    
    
    }
    
    namespace
    {
        class Lut1DOp : public Op
        {
        public:
            Lut1DOp(const Lut1DRcPtr & lut,
                    Interpolation interpolation,
                    TransformDirection direction);
            virtual ~Lut1DOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;
            
            virtual bool isNoOp() const;
            virtual bool isSameType(const OpRcPtr & op) const;
            virtual bool isInverse(const OpRcPtr & op) const;
            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual bool supportsGpuShader() const;
            virtual void writeGpuShader(std::ostream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const;
            
        private:
            const Lut1DRcPtr m_lut;
            Interpolation m_interpolation;
            TransformDirection m_direction;
            
            std::string m_cacheID;
        };
        
        typedef OCIO_SHARED_PTR<Lut1DOp> Lut1DOpRcPtr;
        
        
        Lut1DOp::Lut1DOp(const Lut1DRcPtr & lut,
                         Interpolation interpolation,
                         TransformDirection direction):
                            Op(),
                            m_lut(lut),
                            m_interpolation(interpolation),
                            m_direction(direction)
        {
        }
        
        OpRcPtr Lut1DOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new Lut1DOp(m_lut, m_interpolation, m_direction));
            return op;
        }
        
        Lut1DOp::~Lut1DOp()
        { }
        
        std::string Lut1DOp::getInfo() const
        {
            return "<Lut1DOp>";
        }
        
        std::string Lut1DOp::getCacheID() const
        {
            return m_cacheID;
        }
        
        // TODO: compute real value for isNoOp
        bool Lut1DOp::isNoOp() const
        {
            return false;
        }
        
        bool Lut1DOp::isSameType(const OpRcPtr & op) const
        {
            Lut1DOpRcPtr typedRcPtr = DynamicPtrCast<Lut1DOp>(op);
            if(!typedRcPtr) return false;
            return true;
        }
        
        bool Lut1DOp::isInverse(const OpRcPtr & op) const
        {
            Lut1DOpRcPtr typedRcPtr = DynamicPtrCast<Lut1DOp>(op);
            if(!typedRcPtr) return false;
            
            if(GetInverseTransformDirection(m_direction) != typedRcPtr->m_direction)
                return false;
            
            return (m_lut->getCacheID() == typedRcPtr->m_lut->getCacheID());
        }
    
        bool Lut1DOp::hasChannelCrosstalk() const
        {
            return false;
        }
        
        void Lut1DOp::finalize()
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception("Cannot apply lut1d op, unspecified transform direction.");
            }
            
            // Validate the requested interpolation type
            switch(m_interpolation)
            {
                 // These are the allowed values.
                case INTERP_NEAREST:
                case INTERP_LINEAR:
                    break;
                case INTERP_BEST:
                    m_interpolation = INTERP_LINEAR;
                    break;
                case INTERP_UNKNOWN:
                    throw Exception("Cannot apply Lut1DOp, unspecified interpolation.");
                    break;
                case INTERP_TETRAHEDRAL:
                    throw Exception("Cannot apply Lut1DOp, tetrahedral interpolation is not allowed for 1d luts.");
                    break;
                default:
                    throw Exception("Cannot apply Lut1DOp, invalid interpolation specified.");
            }
            
            if(m_lut->luts[0].empty() || m_lut->luts[1].empty() || m_lut->luts[2].empty())
            {
                throw Exception("Cannot apply lut1d op, no lut data provided.");
            }
            
            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<Lut1DOp ";
            cacheIDStream << m_lut->getCacheID() << " ";
            cacheIDStream << InterpolationToString(m_interpolation) << " ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();
        }
        
        void Lut1DOp::apply(float* rgbaBuffer, long numPixels) const
        {
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                if(m_interpolation == INTERP_NEAREST)
                {
#ifdef USE_SSE
                    Lut1D_Nearest_SSE(rgbaBuffer, numPixels, *m_lut);
#else
                    Lut1D_Nearest(rgbaBuffer, numPixels, *m_lut);
#endif
                }
                else if(m_interpolation == INTERP_LINEAR)
                {
                    Lut1D_Linear(rgbaBuffer, numPixels, *m_lut);
                }
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                if(m_interpolation == INTERP_NEAREST)
                {
                    Lut1D_NearestInverse(rgbaBuffer, numPixels, *m_lut);
                }
                else if(m_interpolation == INTERP_LINEAR)
                {
                    Lut1D_LinearInverse(rgbaBuffer, numPixels, *m_lut);
                }
            }
        }
        
        bool Lut1DOp::supportsGpuShader() const
        {
            return false;
        }
        
        void Lut1DOp::writeGpuShader(std::ostream & /*shader*/,
                                     const std::string & /*pixelName*/,
                                     const GpuShaderDesc & /*shaderDesc*/) const
        {
            throw Exception("Lut1DOp does not support analytical shader generation.");
        }
    }
    
    void CreateLut1DOp(OpRcPtrVec & ops,
                       const Lut1DRcPtr & lut,
                       Interpolation interpolation,
                       TransformDirection direction)
    {
        if(lut->isNoOp()) return;
        
        // TODO: Detect if lut1d can be exactly approximated as y = mx + b
        // If so, return a mtx instead.
        
        ops.push_back( OpRcPtr(new Lut1DOp(lut, interpolation, direction)) );
    }
    
    
    void GenerateIdentityLut1D(float* img, int numElements, int numChannels)
    {
        if(!img) return;
        int numChannelsToFill = std::min(3, numChannels);
        
        float scale = 1.0f / ((float) numElements - 1.0f);
        for(int i=0; i<numElements; i++)
        {
            for(int c=0; c<numChannelsToFill; ++c)
            {
                img[numChannels*i+c] = scale * (float)(i);
            }
        }
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

#include <cstring>

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OIIO_ADD_TEST(Lut1DOp, NoOp)
{
    // Make an identity lut
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();
    lut->from_min[0] = 0.0f;
    lut->from_min[1] = 0.0f;
    lut->from_min[2] = 0.0f;
    
    lut->from_max[0] = 1.0f;
    lut->from_max[1] = 1.0f;
    lut->from_max[2] = 1.0f;
    
    int size = 256;
    for(int i=0; i<size; ++i)
    {
        float x = (float)i / (float)(size-1);
        for(int c=0; c<3; ++c)
        {
            lut->luts[c].push_back(x);
        }
    }
    
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_RELATIVE;
    OIIO_CHECK_EQUAL(lut->isNoOp(), true);
    
    lut->unfinalize();
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_ABSOLUTE;
    OIIO_CHECK_EQUAL(lut->isNoOp(), true);
    
    // Edit the lut
    // These should NOT be identity
    lut->unfinalize();
    lut->luts[0][125] += 1e-3f;
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_RELATIVE;
    OIIO_CHECK_EQUAL(lut->isNoOp(), false);
    
    lut->unfinalize();
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_ABSOLUTE;
    OIIO_CHECK_EQUAL(lut->isNoOp(), false);
}


OIIO_ADD_TEST(Lut1DOp, FiniteValue)
{
    // Make a lut that squares the input
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();
    lut->from_min[0] = 0.0f;
    lut->from_min[1] = 0.0f;
    lut->from_min[2] = 0.0f;
    
    lut->from_max[0] = 1.0f;
    lut->from_max[1] = 1.0f;
    lut->from_max[2] = 1.0f;
    
    int size = 256;
    for(int i=0; i<size; ++i)
    {
        float x = (float)i / (float)(size-1);
        float x2 = x*x;
        
        for(int c=0; c<3; ++c)
        {
            lut->luts[c].push_back(x2);
        }
    }
    
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_RELATIVE;
    OIIO_CHECK_EQUAL(lut->isNoOp(), false);
    
    float inputBuffer_linearforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    float outputBuffer_linearforward[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    OCIO::Lut1D_Linear(inputBuffer_linearforward, 1, *lut);
    for(int i=0; i <4; ++i)
    {
        OIIO_CHECK_CLOSE(inputBuffer_linearforward[i], outputBuffer_linearforward[i], 1e-5f);
    }
    
    float inputBuffer_nearestforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    float outputBuffer_nearestforward[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    OCIO::Lut1D_Nearest(inputBuffer_nearestforward, 1, *lut);
    for(int i=0; i <4; ++i)
    {
        OIIO_CHECK_CLOSE(inputBuffer_nearestforward[i], outputBuffer_nearestforward[i], 1e-2f);
    }
    
    float inputBuffer_linearinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    float outputBuffer_linearinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    OCIO::Lut1D_LinearInverse(outputBuffer_linearinverse, 1, *lut);
    for(int i=0; i <4; ++i)
    {
        OIIO_CHECK_CLOSE(inputBuffer_linearinverse[i], outputBuffer_linearinverse[i], 1e-5f);
    }
    
    float inputBuffer_nearestinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    float outputBuffer_nearestinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    OCIO::Lut1D_NearestInverse(outputBuffer_nearestinverse, 1, *lut);
    for(int i=0; i <4; ++i)
    {
        OIIO_CHECK_CLOSE(inputBuffer_nearestinverse[i], outputBuffer_nearestinverse[i], 1e-2f);
    }
}


OIIO_ADD_TEST(Lut1DOp, Inverse)
{
    // Make a lut that squares the input
    OCIO::Lut1DRcPtr lut_a = OCIO::Lut1D::Create();
    {
    lut_a->from_min[0] = 0.0f;
    lut_a->from_min[1] = 0.0f;
    lut_a->from_min[2] = 0.0f;
    lut_a->from_max[0] = 1.0f;
    lut_a->from_max[1] = 1.0f;
    lut_a->from_max[2] = 1.0f;
    int size = 256;
    for(int i=0; i<size; ++i)
    {
        float x = (float)i / (float)(size-1);
        float x2 = x*x;
        
        for(int c=0; c<3; ++c)
        {
            lut_a->luts[c].push_back(x2);
        }
    }
    lut_a->maxerror = 1e-5f;
    lut_a->errortype = OCIO::ERROR_RELATIVE;
    }
    
    // Make another lut
    OCIO::Lut1DRcPtr lut_b = OCIO::Lut1D::Create();
    {
    lut_b->from_min[0] = 0.5f;
    lut_b->from_min[1] = 0.6f;
    lut_b->from_min[2] = 0.7f;
    lut_b->from_max[0] = 1.0f;
    lut_b->from_max[1] = 1.0f;
    lut_b->from_max[2] = 1.0f;
    int size = 256;
    for(int i=0; i<size; ++i)
    {
        float x = (float)i / (float)(size-1);
        float x2 = x*x;
        
        for(int c=0; c<3; ++c)
        {
            lut_b->luts[c].push_back(x2);
        }
    }
    lut_b->maxerror = 1e-5f;
    lut_b->errortype = OCIO::ERROR_RELATIVE;
    }
    
    OCIO::OpRcPtrVec ops;
    CreateLut1DOp(ops, lut_a, OCIO::INTERP_NEAREST, OCIO::TRANSFORM_DIR_FORWARD);
    CreateLut1DOp(ops, lut_a, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE);
    CreateLut1DOp(ops, lut_b, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD);
    CreateLut1DOp(ops, lut_b, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE);
    
    OIIO_CHECK_EQUAL(ops.size(), 4);
    
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[1]));
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[2]));
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[3]->clone()));
    
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[1]), true);
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[2]), false);
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[2]), false);
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[3]), false);
    OIIO_CHECK_EQUAL( ops[2]->isInverse(ops[3]), true);
}


#ifdef USE_SSE
OIIO_ADD_TEST(Lut1DOp, SSE)
{
    // Make a lut that squares the input
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();
    lut->from_min[0] = 0.0f;
    lut->from_min[1] = 0.0f;
    lut->from_min[2] = 0.0f;
    
    lut->from_max[0] = 1.0f;
    lut->from_max[1] = 1.0f;
    lut->from_max[2] = 1.0f;
    
    int size = 256;
    for(int i=0; i<size; ++i)
    {
        float x = (float)i / (float)(size-1);
        float x2 = x*x;
        
        for(int c=0; c<3; ++c)
        {
            lut->luts[c].push_back(x2);
        }
    }
    
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_RELATIVE;
    OIIO_CHECK_EQUAL(lut->isNoOp(), false);
    
    int NUM_TEST_PIXELS = 1024;
    std::vector<float> testValues(NUM_TEST_PIXELS*4);
    std::vector<float> outputBuffer_cpu(NUM_TEST_PIXELS*4);
    std::vector<float> outputBuffer_sse(NUM_TEST_PIXELS*4);
    
    float val = -1.0f;
    float delta = 0.00123456789f;
    
    for(int i=0; i<NUM_TEST_PIXELS*4; ++i)
    {
        testValues[i] = val;
        val += delta;
    }
    
    memcpy(&outputBuffer_cpu[0], &testValues[0], testValues.size()*sizeof(float));
    memcpy(&outputBuffer_sse[0], &testValues[0], testValues.size()*sizeof(float));
    
    OCIO::Lut1D_Nearest(&outputBuffer_cpu[0], NUM_TEST_PIXELS, *lut);
    OCIO::Lut1D_Nearest_SSE(&outputBuffer_sse[0], NUM_TEST_PIXELS, *lut);
    
    for(int i=0; i<NUM_TEST_PIXELS*4; ++i)
    {
        OIIO_CHECK_CLOSE(outputBuffer_cpu[i], outputBuffer_sse[i], 1e-7f);
        //OIIO_CHECK_EQUAL(outputBuffer_cpu[i], outputBuffer_sse[i]);
    }
    
    
    // Test special values
    /*
    NUM_TEST_PIXELS = 2;
    testValues.resize(NUM_TEST_PIXELS*4);
    outputBuffer_cpu.resize(NUM_TEST_PIXELS*4);
    outputBuffer_sse.resize(NUM_TEST_PIXELS*4);
    
    testValues[0] = std::numeric_limits<float>::signaling_NaN();
    testValues[1] = std::numeric_limits<float>::quiet_NaN();
    testValues[2] = -std::numeric_limits<float>::signaling_NaN();
    testValues[3] = -std::numeric_limits<float>::signaling_NaN();
    
    testValues[4] = std::numeric_limits<float>::infinity();
    testValues[5] = -std::numeric_limits<float>::infinity();
    testValues[6] = 0.0f;
    
    
    memcpy(&outputBuffer_cpu[0], &testValues[0], testValues.size()*sizeof(float));
    memcpy(&outputBuffer_sse[0], &testValues[0], testValues.size()*sizeof(float));
    
    OCIO::Lut1D_Nearest(&outputBuffer_cpu[0], NUM_TEST_PIXELS, lut);
    OCIO::Lut1D_Nearest_SSE(&outputBuffer_sse[0], NUM_TEST_PIXELS, lut);
    
    for(int i=0; i<NUM_TEST_PIXELS*4; ++i)
    {
        //OIIO_CHECK_CLOSE(outputBuffer_cpu[i], outputBuffer_sse[i], 1e-7f);
        OIIO_CHECK_EQUAL(outputBuffer_cpu[i], outputBuffer_sse[i]);
    }
    
    */
}
#endif


OIIO_ADD_TEST(Lut1DOp, NanInf)
{
    // Make a lut that squares the input
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();
    lut->from_min[0] = 0.0f;
    lut->from_min[1] = 0.0f;
    lut->from_min[2] = 0.0f;
    
    lut->from_max[0] = 1.0f;
    lut->from_max[1] = 1.0f;
    lut->from_max[2] = 1.0f;
    
    int size = 256;
    for(int i=0; i<size; ++i)
    {
        float x = (float)i / (float)(size-1);
        float x2 = x*x;
        
        for(int c=0; c<3; ++c)
        {
            lut->luts[c].push_back(x2);
        }
    }
    
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_RELATIVE;
    OIIO_CHECK_EQUAL(lut->isNoOp(), false);
    
    const float reference[4] = {  std::numeric_limits<float>::signaling_NaN(),
                                  std::numeric_limits<float>::quiet_NaN(),
                                  std::numeric_limits<float>::infinity(),
                                  -std::numeric_limits<float>::infinity() };
    /*
    float output[4] = { std::numeric_limits<float>::signaling_NaN(),
                        std::numeric_limits<float>::quiet_NaN(),
                        1.0f,
                        -std::numeric_limits<float>::infinity()  };
    */
    float color[4];
    
    memcpy(color, reference, 4*sizeof(float));
    OCIO::Lut1D_Linear(color, 1, *lut);
    /*
    for(int i=0; i<4; ++i)
    {
        if(isnan(color[i]))
        {
            std::cerr << color[i] << " " << output[i] << std::endl;
            OIIO_CHECK_EQUAL(isnan(color[i]), isnan(output[i]));
        }
        else
        {
            OIIO_CHECK_EQUAL(color[i], output[i]);
        }
    }
    */
    memcpy(color, reference, 4*sizeof(float));
    OCIO::Lut1D_Nearest(color, 1, *lut);
    /*
    for(int i=0; i <4; ++i)
    {
        if(isnan(color[i]))
        {
            OIIO_CHECK_EQUAL(isnan(color[i]), isnan(output[i]));
        }
        else
        {
            OIIO_CHECK_EQUAL(color[i], output[i]);
        }
    }
    */
    memcpy(color, reference, 4*sizeof(float));
    OCIO::Lut1D_LinearInverse(color, 1, *lut);
    /*
    for(int i=0; i <4; ++i)
    {
        if(isnan(color[i]))
        {
            OIIO_CHECK_EQUAL(isnan(color[i]), isnan(output[i]));
        }
        else
        {
            OIIO_CHECK_EQUAL(color[i], output[i]);
        }
    }
    */
    memcpy(color, reference, 4*sizeof(float));
    OCIO::Lut1D_NearestInverse(color, 1, *lut);
    /*
    for(int i=0; i <4; ++i)
    {
        if(isnan(color[i]))
        {
            OIIO_CHECK_EQUAL(isnan(color[i]), isnan(output[i]));
        }
        else
        {
            OIIO_CHECK_EQUAL(color[i], output[i]);
        }
    }
    */
}

#endif // OCIO_UNIT_TEST
