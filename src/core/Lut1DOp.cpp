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

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        bool IsLut1DNoOp(const Lut1D & lut,
                         float relativeIdentityTolerance)
        {
            // If tolerance is 0.0, or negative, skip check
            if(!(relativeIdentityTolerance > 0.0)) return false;
            
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
                    
                    if(!equalWithRelError(identval, lutval, relativeIdentityTolerance))
                    {
                        return false;
                    }
                }
            }
            
            return true;
        }
    }
    
    
    void Lut1D::finalize(float relativeIdentityTolerance)
    {
        if(isFinal) return;
        
        isFinal = true;
        isNoOp = IsLut1DNoOp(*this, relativeIdentityTolerance);
        
        if(isNoOp)
        {
            cacheID = "<NULL 1D>";
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
            
            cacheID = GetPrintableHash(digest);
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
        
#ifndef USE_SSE
        
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
                rgbaBuffer[0] = lookupNearest_1D(mInv_x_maxIndex[0] * (rgbaBuffer[0] - b[0]), maxIndex[0], startPos[0]);
                rgbaBuffer[1] = lookupNearest_1D(mInv_x_maxIndex[1] * (rgbaBuffer[1] - b[1]), maxIndex[1], startPos[1]);
                rgbaBuffer[2] = lookupNearest_1D(mInv_x_maxIndex[2] * (rgbaBuffer[2] - b[2]), maxIndex[2], startPos[2]);
                
                rgbaBuffer += 4;
            }
        }
#else
        void Lut1D_Nearest(float* rgbaBuffer, long numPixels, const Lut1D & lut)
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
                
                rgbaBuffer[0] = startPos[0][(int)(result[0])];
                rgbaBuffer[1] = startPos[1][(int)(result[1])];
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
                rgbaBuffer[0] = lookupLinear_1D(mInv_x_maxIndex[0] * (rgbaBuffer[0] - b[0]), maxIndex[0], startPos[0]);
                rgbaBuffer[1] = lookupLinear_1D(mInv_x_maxIndex[1] * (rgbaBuffer[1] - b[1]), maxIndex[1], startPos[1]);
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
                rgbaBuffer[0] = m[0] * reverseLookupNearest_1D(rgbaBuffer[0], startPos[0], endPos[0]) + b[0];
                rgbaBuffer[1] = m[1] * reverseLookupNearest_1D(rgbaBuffer[1], startPos[1], endPos[1]) + b[1];
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
            
            return ((float)(lowbound - start) + delta) * invMaxIndex;
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
                rgbaBuffer[0] = m[0] * reverseLookupLinear_1D(rgbaBuffer[0], startPos[0], endPos[0], invMaxIndex[0]) + b[0];
                rgbaBuffer[1] = m[1] * reverseLookupLinear_1D(rgbaBuffer[1], startPos[1], endPos[1], invMaxIndex[0]) + b[1];
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
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual bool supportsGpuShader() const;
            virtual void writeGpuShader(std::ostringstream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const;
            
            virtual bool definesGpuAllocation() const;
            virtual GpuAllocationData getGpuAllocation() const;
            
        private:
            const Lut1DRcPtr m_lut;
            Interpolation m_interpolation;
            TransformDirection m_direction;
            
            std::string m_cacheID;
        };
        
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
        
        void Lut1DOp::finalize()
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception("Cannot apply lut1d op, unspecified transform direction.");
            }
            if(m_interpolation == INTERP_UNKNOWN)
            {
                throw Exception("Cannot apply lut1d op, unspecified interpolation.");
            }
            if(m_lut->luts[0].empty() || m_lut->luts[1].empty() || m_lut->luts[2].empty())
            {
                throw Exception("Cannot apply lut1d op, no lut data provided.");
            }
            
            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<Lut1DOp ";
            cacheIDStream << m_lut->cacheID << " ";
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
                    Lut1D_Nearest(rgbaBuffer, numPixels, *m_lut);
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
        
        void Lut1DOp::writeGpuShader(std::ostringstream & /*shader*/,
                                     const std::string & /*pixelName*/,
                                     const GpuShaderDesc & /*shaderDesc*/) const
        {
            throw Exception("Lut1DOp does not support analytical shader generation.");
        }
        
        bool Lut1DOp::definesGpuAllocation() const
        {
            return false;
        }
        
        GpuAllocationData Lut1DOp::getGpuAllocation() const
        {
            throw Exception("Lut1DOp does not define a Gpu Allocation.");
        }
    }
    
    void CreateLut1DOp(OpRcPtrVec & ops,
                       const Lut1DRcPtr & lut,
                       Interpolation interpolation,
                       TransformDirection direction)
    {
        if(!lut->isFinal)
        {
            throw Exception("The specified Lut1D has not been finalized.");
        }
        
        if(lut->isNoOp) return;
        
        // TODO: Detect if lut1d can be exactly approximated as y = mx + b
        // If so, return a mtx instead.
        
        ops.push_back( OpRcPtr(new Lut1DOp(lut, interpolation, direction)) );
    }

}
OCIO_NAMESPACE_EXIT
