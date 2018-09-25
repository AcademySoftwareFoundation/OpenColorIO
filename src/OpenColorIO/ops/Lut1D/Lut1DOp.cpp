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
#include "ops/Lut1D/Lut1DOp.h"
#include "MathUtils.h"
#include "SSE.h"
#include "GpuShaderUtils.h"
#include "BitDepthUtils.h"
#include "OpTools.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <iostream>

OCIO_NAMESPACE_ENTER
{
    Lut1D::Lut1D()
        :   maxerror(std::numeric_limits<float>::min())
        ,   errortype(ERROR_RELATIVE)
        ,   inputBitDepth(BIT_DEPTH_F32)
        ,   outputBitDepth(BIT_DEPTH_F32)

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

    Lut1DRcPtr Lut1D::CreateIdentity(BitDepth inputBitDepth, BitDepth outputBitDepth)
    {
        Lut1DRcPtr lut      = Lut1D::Create();
        lut->inputBitDepth  = inputBitDepth;
        lut->outputBitDepth = outputBitDepth;

        const unsigned idealSize = GetLutIdealSize(inputBitDepth);

        lut->luts[0].resize(idealSize);
        lut->luts[1].resize(idealSize);
        lut->luts[2].resize(idealSize);

        const float stepValue
            = GetBitDepthMaxValue(outputBitDepth) / (float(idealSize) - 1.0f);

        for(unsigned idx=0; idx<lut->luts[0].size(); ++idx)
        {
            const float ftemp = float(idx) * stepValue;
            lut->luts[0][idx] = ftemp;
            lut->luts[1][idx] = ftemp;
            lut->luts[2][idx] = ftemp;
        }

        return lut;
    }

    Lut1D & Lut1D::operator=(const Lut1D & l)
    {
        if(this!=&l)
        {
            maxerror  = l.maxerror;
            errortype = l.errortype;
            
            from_min[0] = l.from_min[0];
            from_min[1] = l.from_min[1];
            from_min[2] = l.from_min[2];

            from_max[0] = l.from_max[0];
            from_max[1] = l.from_max[1];
            from_max[2] = l.from_max[2];
            
            luts[0] = l.luts[0];
            luts[1] = l.luts[1];
            luts[2] = l.luts[2];

            inputBitDepth  = l.inputBitDepth;
            outputBitDepth = l.outputBitDepth;

            // Note: do not copy the mutex
        }

        return *this;
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
            md5_append(&state, (const md5_byte_t *)from_min, (int)(3*sizeof(float)));
            md5_append(&state, (const md5_byte_t *)from_max, (int)(3*sizeof(float)));
            
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

#if defined(OCIO_UNIT_TEST) || !defined(USE_SSE)
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
#endif
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
            return simple_lut[indexLow] + delta * (simple_lut[indexHigh] - simple_lut[indexLow]);
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
    
        void PadLutChannel(unsigned width,
                           unsigned height,
                           const std::vector<float> & channel,
                           std::vector<float> & chn)
        {
            const unsigned currWidth = (unsigned)channel.size();

            if (height>1)
            {
                // Fill the texture values
                //
                // Make the last texel of a given row the same as the first texel 
                // of its next row.  This will preserve the continuity along row breaks 
                // as long as the lookup position used by the sampler is based on (width-1) 
                // to account for the 1 texel padding at the end of each row.

                const unsigned step = width-1;
                for(unsigned i=0 ; i<currWidth-step ; i+=step)
                {
                    chn.insert(chn.end(), &channel[i], &channel[i+step]);
                    chn.push_back(channel[i+step]);
                }

                // If there are still texels to fill, add them to the texture data
                const unsigned leftover = currWidth%step;
                if (leftover > 0)
                {
                    chn.insert(chn.end(), &channel[currWidth-leftover], &channel[currWidth-1]);
                    chn.push_back(channel[currWidth-1]);
                }
            }
            else
            {
                chn = channel;
            }

            // Pad the remaining of the texture with the last LUT entry.
            chn.insert(chn.end(), width*height-chn.size(), channel[currWidth-1]);
        }

        bool HasExtendedDomain(const Lut1DRcPtr & lut)
        {
            // The forward LUT is allowed to have entries outside the outDepth 
            // (e.g. a 10i LUT is allowed to have values on [-20,1050] if it wants).
            // This is called an extended range LUT and helps maximize accuracy 
            // by allowing clamping to happen (if necessary) after the interpolation.
            // The implication is that the inverse LUT needs to evaluate over 
            // an extended domain.  Since this potentially requires a slower rendering 
            // method for the Fast style, this method allows the renderers 
            // to determine if this is necessary.

            // TODO: To enhance when adding bit depth

            // TODO: To enhance when adding half domain luts

            // TODO: To enhance to support not monotonic luts

            // The input bit depth describes the scaling of the LUT entries.
            const float normalMin = 0.0f;
            const float normalMax = GetBitDepthMaxValue(lut->inputBitDepth);

            return lut->from_min[0]<normalMin
                || lut->from_min[1]<normalMin 
                || lut->from_min[2]<normalMin

                || lut->from_max[0]>normalMax
                || lut->from_max[1]>normalMax 
                || lut->from_max[2]>normalMax;
        }

        // TODO: This function is not fully ported yet from SynColor 
        //       and probably needs to be reworked for OCIO.
        //
        Lut1DRcPtr MakeLookupDomain(BitDepth incomingBitDepth)
        {
            const unsigned idealSize = GetLutIdealSize(incomingBitDepth);

            Lut1DRcPtr lut(Lut1D::Create());
            lut->inputBitDepth = BIT_DEPTH_F32;
            lut->outputBitDepth = BIT_DEPTH_F32;
            lut->luts[0].resize(idealSize);
            lut->luts[1].resize(idealSize);
            lut->luts[2].resize(idealSize);

            const float stepValue 
                = GetBitDepthMaxValue(BIT_DEPTH_F32) / (float(idealSize) - 1.0f);

            for(unsigned idx=0; idx<idealSize; ++idx)
            {
                const float ftemp = float(idx) * stepValue;

                lut->luts[0][idx] = ftemp;
                lut->luts[1][idx] = ftemp;
                lut->luts[2][idx] = ftemp;
            }

            return lut;
        }

    }
    
    namespace
    {
        class Lut1DOp;
        typedef OCIO_SHARED_PTR<Lut1DOp> Lut1DOpRcPtr;
        

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
            
            virtual bool supportedByLegacyShader() const { return false; }
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;

            virtual bool canCombineWith(const OpRcPtr & op) const;
            virtual void combineWith(OpRcPtrVec & ops, const OpRcPtr & secondOp) const;

            Lut1DRcPtr makeFastLut1D(bool forGPU);

        private:
            const Lut1DRcPtr m_lut;
            Interpolation m_interpolation;
            TransformDirection m_direction;

            Lut1DRcPtr m_lut_gpu_apply;
            std::string m_cacheID;
        };
        
        
        Lut1DOp::Lut1DOp(const Lut1DRcPtr & lut,
                         Interpolation interpolation,
                         TransformDirection direction)
            :   Op(lut->inputBitDepth, lut->outputBitDepth),
                m_lut(lut),
                m_interpolation(interpolation),
                m_direction(direction),
                m_lut_gpu_apply(lut)
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

        bool Lut1DOp::canCombineWith(const OpRcPtr & /*op*/) const
        {
            // TODO: To implement

            return false;
        }
        
        void Lut1DOp::combineWith(OpRcPtrVec & /*ops*/,
                                  const OpRcPtr & /*secondOp*/) const
        {
            // TODO: To implement

            std::ostringstream os;
            os << "Op: " << getInfo() << " cannot be combined. ";
            os << "A type-specific combining function is not defined.";
            throw Exception(os.str().c_str());
        }   

        // TODO: This function is not fully ported yet from SynColor 
        //       and probably needs to be reworked for OCIO.
        //
        // The domain to use for the FastLut is a challenging problem since we don't
        // know the input and output color space of the LUT.  In particular, we don't
        // know if a half or normal domain would be better.  For now, we use a
        // heuristic which is based on the original input bit-depth of the inverse LUT
        // (the output bit-depth of the forward LUT).  (We preserve the original depth
        // as a member since typically by the time this routine is called, the depth
        // has been reset to 32f.)  However, there are situations where the origDepth
        // is not reliable (e.g. a user creates a transform in Custom mode and exports it).
        // Ultimately, the goal is to replace this with an automated algorithm that
        // computes the best domain based on analysis of the curvature of the LUT.
        //
        Lut1DRcPtr Lut1DOp::makeFastLut1D(bool forGPU)
        {
            BitDepth depth(getInputBitDepth());

            // For typical LUTs (e.g. gamma tables from ICC monitor profiles)
            // we can use a smaller FastLUT on the GPU.
            // Currently allowing 16f to be subsampled for GPU but using 16i as a way
            // to indicate not to subsample certain LUTs (e.g. float-conversion LUTs).
            if(forGPU && depth != BIT_DEPTH_UINT16)
            {
                // GPU will always interpolate rather than look-up.
                // Use a smaller table for better efficiency.

                // TODO: Investigate performance/quality trade-off.

                depth = BIT_DEPTH_UINT12;
            }

            // But if the LUT has values outside [0,1], use a half-domain fastLUT.
            if(HasExtendedDomain(m_lut))
            {
                depth = BIT_DEPTH_F16;
            }

            // Make a domain for the composed Lut1D.
            Lut1DRcPtr newDomainLut = MakeLookupDomain(depth);

            // Regardless of what depth is used to build the domain, set the in & out 
            // to the actual depth so that scaling is done correctly.
            newDomainLut->inputBitDepth  = getInputBitDepth();
            newDomainLut->outputBitDepth = getInputBitDepth();

            // To avoid impacting the current Op, clone it (i.e. the const data is shared)
            OpRcPtr b = clone();
            return Compose(newDomainLut, b, COMPOSE_RESAMPLE_NO);
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
                    throw Exception(
                        "Cannot apply Lut1DOp, tetrahedral interpolation is not allowed for 1d luts.");
                    break;
                default:
                    throw Exception("Cannot apply Lut1DOp, invalid interpolation specified.");
            }
            
            if(m_lut->luts[0].empty() || m_lut->luts[1].empty() || m_lut->luts[2].empty())
            {
                throw Exception("Cannot apply lut1d op, no lut data provided.");
            }

            if(m_lut->luts[0].size()!=m_lut->luts[1].size()
                || m_lut->luts[0].size()!=m_lut->luts[2].size())
            {
                throw Exception(
                    "Cannot apply lut1d op, the LUT for each channel must have the same dimensions.");
            }

            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<Lut1DOp ";
            cacheIDStream << m_lut->getCacheID() << " ";
            cacheIDStream << InterpolationToString(m_interpolation) << " ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << BitDepthToString(getInputBitDepth()) << " ";
            cacheIDStream << BitDepthToString(getOutputBitDepth()) << " ";
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();

            if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                // Compute a fast forward Lut 1D from an inverse Lut 1D
                m_lut_gpu_apply = makeFastLut1D(true);
            }
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

        void Lut1DOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            if(getInputBitDepth()!=BIT_DEPTH_F32 || getOutputBitDepth()!=BIT_DEPTH_F32)
            {
                throw Exception("Only 32F bit depth is supported for the GPU shader");
            }

            const unsigned defaultMaxWidth = shaderDesc->getTextureMaxWidth();

            const unsigned length = unsigned(m_lut_gpu_apply->luts[0].size());
            const unsigned width  = std::min(length, defaultMaxWidth);
            const unsigned height = (length / defaultMaxWidth) + 1;

            // Adjust LUT texture to allow for correct 2d linear interpolation, if needed.

            std::vector<float> red;
            PadLutChannel(width, height, m_lut_gpu_apply->luts[0], red);

            std::vector<float> grn;
            PadLutChannel(width, height, m_lut_gpu_apply->luts[1], grn);

            std::vector<float> blu;
            PadLutChannel(width, height, m_lut_gpu_apply->luts[2], blu);

            const unsigned maxValues = width * height;

            std::vector<float> rgb;
            rgb.resize(maxValues * 3);

            for(unsigned idx=0; idx<maxValues; ++idx)
            {
                rgb[3*idx+0] = red[idx];
                rgb[3*idx+1] = grn[idx];
                rgb[3*idx+2] = blu[idx];
            }

            // Register the RGB lut

            std::ostringstream resName;
            resName << shaderDesc->getResourcePrefix()
                    << std::string("lut1d_")
                    << shaderDesc->getNumTextures();

            const std::string name(resName.str());

            shaderDesc->addTexture(GpuShaderText::getSamplerName(name).c_str(),
                                   m_cacheID.c_str(),
                                   width, height, 
                                   GpuShaderDesc::TEXTURE_RGB_CHANNEL, 
                                   m_interpolation, 
                                   &rgb[0]);


            float scale[3], offset[3];
            bool somethingToDo = false;
            for(unsigned i = 0; i < 3; ++i)
            {
                scale[i]  = 1.0f / (m_lut_gpu_apply->from_max[i] - m_lut_gpu_apply->from_min[i]);
                offset[i] = -m_lut_gpu_apply->from_min[i] * scale[i];

                if(m_direction == TRANSFORM_DIR_INVERSE)
                {
                    scale[i]  = 1.0f / scale[i];
                    offset[i] = -offset[i];
                }

                somethingToDo |= (scale[i] != 1.f || offset[i] != 0.f);
            }

            // Add the lut code to the OCIO shader program

            if(height>1)
            {
                // In case the 1D lut length exceeds the 1D texture maximum length
                // a 2D texture is used.

                {
                    GpuShaderText ss(shaderDesc->getLanguage());
                    ss.declareTex2D(name);
                    shaderDesc->addToDeclareShaderCode(ss.string().c_str());
                }

                {
                    GpuShaderText ss(shaderDesc->getLanguage());

                    ss.newLine() << ss.vec2fKeyword() << " " << name << "_computePos(float f)";
                    ss.newLine() << "{";
                    ss.indent();

                    // Need min() to protect against f > 1 causing a bogus x value.
                    // min( f, 1.) * (dim - 1)
                    ss.newLine() <<   "float dep = min(f, 1.0) * " << float(length-1) << ";";

                    ss.newLine() <<   ss.vec2fDecl("retVal") << ";";
                    // float(int( dep / (width-1) ))
                    ss.newLine() <<   "retVal.y = float(int(dep / " << float(width-1) << "));";
                    // dep - retVal.y * (width-1)
                    ss.newLine() <<   "retVal.x = dep - retVal.y * " << float(width-1) << ";";

                    // (retVal.x + 0.5) / width;
                    ss.newLine() <<   "retVal.x = (retVal.x + 0.5) / " << float(width) << ";";
                    // (retVal.x + 0.5) / height;
                    ss.newLine() <<   "retVal.y = (retVal.y + 0.5) / " << float(height) << ";";
                    ss.newLine() <<   "return retVal;";

                    ss.dedent();
                    ss.newLine() << "}";

                    shaderDesc->addToHelperShaderCode(ss.string().c_str());
                }

                {
                    GpuShaderText ss(shaderDesc->getLanguage());
                    ss.indent();

                    const std::string str(name + "_computePos(" + shaderDesc->getPixelName());

                    if(somethingToDo && m_direction == TRANSFORM_DIR_FORWARD)
                    {
                        ss.newLine() << shaderDesc->getPixelName() << ".rgb = " 
                                     << ss.vec3fConst(offset[0], offset[1], offset[2])
                                     << " + " << shaderDesc->getPixelName() << ".rgb * " 
                                     << ss.vec3fConst(scale[0], scale[1], scale[2]) 
                                     << ";" ;
                    }

                    ss.newLine() << shaderDesc->getPixelName() << ".r = " 
                                 << ss.sampleTex2D(name, str + ".r)") 
                                 << ".r;";

                    ss.newLine() << shaderDesc->getPixelName() << ".g = "
                                 << ss.sampleTex2D(name, str + ".g)")
                                 << ".g;";

                    ss.newLine() << shaderDesc->getPixelName() << ".b = " 
                                 << ss.sampleTex2D(name, str + ".b)")
                                 << ".b;";

                    if(somethingToDo && m_direction == TRANSFORM_DIR_INVERSE)
                    {
                        ss.newLine() << shaderDesc->getPixelName() << ".rgb = " 
                                     << ss.vec3fConst(offset[0], offset[1], offset[2])
                                     << " + " << shaderDesc->getPixelName() << ".rgb * " 
                                     << ss.vec3fConst(scale[0], scale[1], scale[2]) 
                                     << ";" ;
                    }

                    shaderDesc->addToFunctionShaderCode(ss.string().c_str());
                }
            }
            else
            {
                {
                    GpuShaderText ss(shaderDesc->getLanguage());
                    ss.declareTex1D(name);
                    shaderDesc->addToDeclareShaderCode(ss.string().c_str());
                }

                {
                    const float m = (float(length)-1.0f) / float(length);
                    const float b = 1.0f / (2.0f * float(length));

                    GpuShaderText ss(shaderDesc->getLanguage());
                    ss.indent();

                    if(somethingToDo && m_direction == TRANSFORM_DIR_FORWARD)
                    {
                        ss.newLine() << shaderDesc->getPixelName() << ".rgb = " 
                                     << ss.vec3fConst(offset[0], offset[1], offset[2])
                                     << " + " << shaderDesc->getPixelName() << ".rgb * " 
                                     << ss.vec3fConst(scale[0], scale[1], scale[2]) 
                                     << ";" ;
                    }

                    // vec3 coords = (inPixel.rgb * (dim-1.0f) + 0.5f) / dim

                    ss.newLine() << ss.vec3fDecl(name + "_coords") 
                                 << " = " << shaderDesc->getPixelName() << ".rgb * " 
                                 << ss.vec3fConst(m) << " + " << ss.vec3fConst(b) << ";";

                    ss.newLine() << shaderDesc->getPixelName() << ".r = " 
                                 << ss.sampleTex1D(name, name + "_coords.r") << ".r;";
                    ss.newLine() << shaderDesc->getPixelName() << ".g = " 
                                 << ss.sampleTex1D(name, name + "_coords.g") << ".g;";
                    ss.newLine() << shaderDesc->getPixelName() << ".b = " 
                                 << ss.sampleTex1D(name, name + "_coords.b") << ".b;";

                    if(somethingToDo && m_direction == TRANSFORM_DIR_INVERSE)
                    {
                        ss.newLine() << shaderDesc->getPixelName() << ".rgb = " 
                                     << ss.vec3fConst(offset[0], offset[1], offset[2])
                                     << " + " << shaderDesc->getPixelName() << ".rgb * " 
                                     << ss.vec3fConst(scale[0], scale[1], scale[2]) 
                                     << ";" ;
                    }

                    shaderDesc->addToFunctionShaderCode(ss.string().c_str());
                }
            }
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
#include "unittest.h"

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
    
    const int size = 256;
    for(int i=0; i<size; ++i)
    {
        const float x = (float)i / (float)(size-1);
        for(int c=0; c<3; ++c)
        {
            lut->luts[c].push_back(x);
        }
    }
    
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_RELATIVE;
    bool isNoOp = false;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_EQUAL(isNoOp, true);
    
    lut->unfinalize();
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_ABSOLUTE;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_EQUAL(isNoOp, true);

    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut,
        OCIO::INTERP_NEAREST, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 0);
    
    // Edit the lut
    // These should NOT be identity
    lut->unfinalize();
    lut->luts[0][125] += 1e-3f;
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_RELATIVE;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_EQUAL(isNoOp, false);
    
    lut->unfinalize();
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_ABSOLUTE;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_EQUAL(isNoOp, false);
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
    
    const int size = 256;
    for(int i=0; i<size; ++i)
    {
        const float x = (float)i / (float)(size-1);
        const float x2 = x*x;
        
        for(int c=0; c<3; ++c)
        {
            lut->luts[c].push_back(x2);
        }
    }
    
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_RELATIVE;
    bool isNoOp = true;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_EQUAL(isNoOp, false);
    
    float inputBuffer_linearforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    const float outputBuffer_linearforward[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    OCIO::Lut1D_Linear(inputBuffer_linearforward, 1, *lut);
    for(int i=0; i <4; ++i)
    {
        OIIO_CHECK_CLOSE(inputBuffer_linearforward[i], outputBuffer_linearforward[i], 1e-5f);
    }
    
    float inputBuffer_nearestforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    const float outputBuffer_nearestforward[4] = { 0.2519647f, 0.36f, 0.492749f, 0.5f };
    OCIO::Lut1D_Nearest(inputBuffer_nearestforward, 1, *lut);
    for(int i=0; i <4; ++i)
    {
        OIIO_CHECK_CLOSE(inputBuffer_nearestforward[i], outputBuffer_nearestforward[i], 1e-5f);
    }
    
    const float inputBuffer_linearinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    float outputBuffer_linearinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    OCIO::Lut1D_LinearInverse(outputBuffer_linearinverse, 1, *lut);
    for(int i=0; i <4; ++i)
    {
        OIIO_CHECK_CLOSE(inputBuffer_linearinverse[i], outputBuffer_linearinverse[i], 1e-5f);
    }
    
    const float inputBuffer_nearestinverse[4] = { 0.498039f, 0.6f, 0.698039f, 0.5f };
    float outputBuffer_nearestinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    OCIO::Lut1D_NearestInverse(outputBuffer_nearestinverse, 1, *lut);
    for(int i=0; i <4; ++i)
    {
        OIIO_CHECK_CLOSE(inputBuffer_nearestinverse[i], outputBuffer_nearestinverse[i], 1e-5f);
    }
}


OIIO_ADD_TEST(Lut1DOp, ArbitraryValue)
{
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();
    lut->from_min[0] = -0.25f;
    lut->from_min[1] = -0.25f;
    lut->from_min[2] = -0.25f;

    lut->from_max[0] = +1.25f;
    lut->from_max[1] = +1.25f;
    lut->from_max[2] = +1.25f;

    const int size = 256;
    lut->luts[0].resize(size);
    lut->luts[1].resize(size);
    lut->luts[2].resize(size);

    for(int i=0; i<size; ++i)
    {
        const float x = (float)i / (float)(size-1);
        const float x2 = x*x;
        
        for(int c=0; c<3; ++c)
        {
            lut->luts[c][i] = x2;
        }
    }

    const float inputBuffer_linearforward[16] 
        = { -0.50f, -0.25f, -0.10f, 0.00f,
             0.50f,  0.60f,  0.70f, 0.80f,
             0.90f,  1.00f,  1.10f, 1.20f,
             1.30f,  1.40f,  1.50f, 1.60f };

    float outputBuffer_linearforward[16];
    for(unsigned i=0; i<16; ++i) { outputBuffer_linearforward[i] = inputBuffer_linearforward[i]; }

    const float outputBuffer_inv_linearforward[16] 
        = { -0.25f, -0.25f, -0.10f, 0.00f,
             0.50f,  0.60f,  0.70f, 0.80f,
             0.90f,  1.00f,  1.10f, 1.20f,
             1.25f,  1.25f,  1.25f, 1.60f };

    OCIO::Lut1D_Linear(outputBuffer_linearforward, 4, *lut);
    OCIO::Lut1D_LinearInverse(outputBuffer_linearforward, 4, *lut);

    for(int i=0; i<16; ++i)
    {
        OIIO_CHECK_CLOSE(outputBuffer_linearforward[i], outputBuffer_inv_linearforward[i], 1e-5f);
    }
}


OIIO_ADD_TEST(Lut1DOp, ExtrapolationErrors)
{
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();
    lut->from_min[0] = 0.0f;
    lut->from_min[2] = 0.0f;
    lut->from_min[2] = 0.0f;

    lut->from_max[0] = 1.0f;
    lut->from_max[1] = 1.0f;
    lut->from_max[2] = 1.0f;

    // Simple y=x+0.1 LUT
    for(int c=0; c<3; ++c)
    {
        lut->luts[c].push_back(0.1f);
        lut->luts[c].push_back(0.6f);
        lut->luts[c].push_back(1.1f);
    }

    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_RELATIVE;
    bool isNoOp = true;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_EQUAL(isNoOp, false);

    const int PIXELS = 5;
    float inputBuffer_linearforward[PIXELS*4] = {
        -0.1f, -0.2f, -10.0f, 0.0f,
        0.5f, 1.0f, 1.1f, 0.0f,
        10.1f, 55.0f, 2.3f, 0.0f,
        9.1f, 1.0e6f, 1.0e9f, 0.0f,
        4.0e9f, 9.5e7f, 0.5f, 0.0f };
    const float outputBuffer_linearforward[PIXELS*4] = {
        0.1f, 0.1f, 0.1f, 0.0f,
        0.6f, 1.1f, 1.1f, 0.0f,
        1.1f, 1.1f, 1.1f, 0.0f,
        1.1f, 1.1f, 1.1f, 0.0f,
        1.1f, 1.1f, 0.6f, 0.0f };
    OCIO::Lut1D_Linear(inputBuffer_linearforward, PIXELS, *lut);
    for(size_t i=0; i <sizeof(inputBuffer_linearforward)/sizeof(inputBuffer_linearforward[0]); ++i)
    {
        OIIO_CHECK_CLOSE(inputBuffer_linearforward[i], outputBuffer_linearforward[i], 1e-5f);
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
    const int size = 256;
    for(int i=0; i<size; ++i)
    {
        const float x = (float)i / (float)(size-1);
        const float x2 = x*x;
        
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
    const int size = 256;
    for(int i=0; i<size; ++i)
    {
        const float x = (float)i / (float)(size-1);
        const float x2 = x*x;
        
        for(int c=0; c<3; ++c)
        {
            lut_b->luts[c].push_back(x2);
        }
    }
    lut_b->maxerror = 1e-5f;
    lut_b->errortype = OCIO::ERROR_RELATIVE;
    }
    
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_a,
        OCIO::INTERP_NEAREST, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_a,
        OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_b,
        OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_b,
        OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE));
    
    OIIO_CHECK_EQUAL(ops.size(), 4);
    
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[1]));
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[2]));
    OCIO::OpRcPtr clonedOp;
    OIIO_CHECK_NO_THROW(clonedOp = ops[3]->clone());
    OIIO_CHECK_ASSERT(ops[0]->isSameType(clonedOp));

    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[1]), true);
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[2]), false);
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[2]), false);
    OIIO_CHECK_EQUAL( ops[0]->isInverse(ops[3]), false);
    OIIO_CHECK_EQUAL( ops[2]->isInverse(ops[3]), true);

    // add same as first
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_a,
        OCIO::INTERP_NEAREST, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 5);

    OCIO::FinalizeOpVec(ops, false);
    OIIO_CHECK_EQUAL(ops.size(), 5);

    OIIO_CHECK_EQUAL(ops[0]->getCacheID(), ops[4]->getCacheID());
    OIIO_CHECK_NE(ops[2]->getCacheID(), ops[3]->getCacheID());

    // optimize will remove forward and inverse (0+1 and 2+3)
    OCIO::FinalizeOpVec(ops, true);
    OIIO_CHECK_EQUAL(ops.size(), 1);
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
    
    const int size = 256;
    for(int i=0; i<size; ++i)
    {
        const float x = (float)i / (float)(size-1);
        const float x2 = x*x;
        
        for(int c=0; c<3; ++c)
        {
            lut->luts[c].push_back(x2);
        }
    }
    
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_RELATIVE;
    bool isNoOp = true;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_EQUAL(isNoOp, false);

    const int NUM_TEST_PIXELS = 1024;
    std::vector<float> testValues(NUM_TEST_PIXELS*4);
    std::vector<float> outputBuffer_cpu(NUM_TEST_PIXELS*4);
    std::vector<float> outputBuffer_sse(NUM_TEST_PIXELS*4);
    
    float val = -1.0f;
    const float delta = 0.00123456789f;
    
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

OIIO_ADD_TEST(Lut1DOp, ThrowNoOp)
{
    // Make an identity lut
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();
    lut->from_min[0] = 0.0f;
    lut->from_min[1] = 0.0f;
    lut->from_min[2] = 0.0f;

    lut->from_max[0] = 1.0f;
    lut->from_max[1] = 1.0f;
    lut->from_max[2] = 1.0f;

    const int size = 2;
    for (int i = 0; i < size; ++i)
    {
        const float x = (float)i / (float)(size - 1);
        for (int c = 0; c < 3; ++c)
        {
            lut->luts[c].push_back(x);
        }
    }

    lut->maxerror = 1e-5f;
    lut->errortype = (OCIO::ErrorType)0;
    OIIO_CHECK_THROW_WHAT(lut->isNoOp(),
        OCIO::Exception, "Unknown error type");

    lut->errortype = OCIO::ERROR_RELATIVE;
    OIIO_CHECK_NO_THROW(lut->isNoOp());
    lut->unfinalize();

    lut->luts[0].clear();
    OIIO_CHECK_THROW_WHAT(lut->isNoOp(),
        OCIO::Exception, "invalid Lut1D");

    lut->luts[0] = lut->luts[1];
    lut->luts[1].clear();
    OIIO_CHECK_THROW_WHAT(lut->isNoOp(),
        OCIO::Exception, "invalid Lut1D");

    lut->luts[1] = lut->luts[2];
    lut->luts[2].clear();
    OIIO_CHECK_THROW_WHAT(lut->isNoOp(),
        OCIO::Exception, "invalid Lut1D");

    lut->luts[2] = lut->luts[0];
    OIIO_CHECK_NO_THROW(lut->isNoOp());
}

OIIO_ADD_TEST(Lut1DOp, ThrowOp)
{
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();
    lut->from_min[0] = 0.0f;
    lut->from_min[1] = 0.0f;
    lut->from_min[2] = 0.0f;
    lut->from_max[0] = 1.0f;
    lut->from_max[1] = 1.0f;
    lut->from_max[2] = 1.0f;
    for (int c = 0; c<3; ++c)
    {
        lut->luts[c].push_back(0.1f);
        lut->luts[c].push_back(1.1f);
    }
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_RELATIVE;
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut,
        OCIO::INTERP_NEAREST, OCIO::TRANSFORM_DIR_UNKNOWN));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_THROW_WHAT(ops[0]->finalize(),
        OCIO::Exception, "unspecified transform direction");
    ops.clear();
    
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut,
        OCIO::INTERP_UNKNOWN, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_THROW_WHAT(ops[0]->finalize(),
        OCIO::Exception, "unspecified interpolation");
    ops.clear();

    // INTERP_TETRAHEDRAL not allowed for 1d lut.
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut,
        OCIO::INTERP_TETRAHEDRAL, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_THROW_WHAT(ops[0]->finalize(),
        OCIO::Exception, "tetrahedral interpolation is not allowed");
    ops.clear();

    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut,
        OCIO::INTERP_BEST, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    lut->luts[0].clear();
    OIIO_CHECK_THROW_WHAT(ops[0]->finalize(),
        OCIO::Exception, "no lut data provided");
}

OIIO_ADD_TEST(Lut1DOp, GPU)
{
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();
    lut->from_min[0] = 0.0f;
    lut->from_min[1] = 0.0f;
    lut->from_min[2] = 0.0f;
    lut->from_max[0] = 1.0f;
    lut->from_max[1] = 1.0f;
    lut->from_max[2] = 1.0f;
    for (int c = 0; c < 3; ++c)
    {
        lut->luts[c].push_back(0.1f);
        lut->luts[c].push_back(1.1f);
    }
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_RELATIVE;
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut,
        OCIO::INTERP_NEAREST, OCIO::TRANSFORM_DIR_FORWARD));
    
    OCIO::FinalizeOpVec(ops);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->supportedByLegacyShader(), false);
}

OIIO_ADD_TEST(Lut1DOp, IdentityLut1D)
{
    int size = 3;
    int channels = 2;
    std::vector<float> data(size*channels);
    OCIO::GenerateIdentityLut1D(&data[0], size, channels);
    OIIO_CHECK_EQUAL(data[0], 0.0f);
    OIIO_CHECK_EQUAL(data[1], 0.0f);
    OIIO_CHECK_EQUAL(data[2], 0.5f);
    OIIO_CHECK_EQUAL(data[3], 0.5f);
    OIIO_CHECK_EQUAL(data[4], 1.0f);
    OIIO_CHECK_EQUAL(data[5], 1.0f);

    size = 4;
    channels = 3;
    data.resize(size*channels);
    OCIO::GenerateIdentityLut1D(&data[0], size, channels);
    for (int c = 0; c < channels; ++c)
    {
        OIIO_CHECK_EQUAL(data[0+c], 0.0f);
        OIIO_CHECK_EQUAL(data[channels+c], 0.33333333f);
        OIIO_CHECK_EQUAL(data[2*channels+c], 0.66666667f);
        OIIO_CHECK_EQUAL(data[3*channels+c], 1.0f);
    }
}

OIIO_ADD_TEST(Lut1DOp, padLutChannel_one_dimension)
{
    const unsigned width  = 15;

    // Create a channel multi row and smaller than the expected texture size

    std::vector<float> channel;
    channel.resize(width - 5);

    // Fill the channel

    for(unsigned idx=0; idx<channel.size(); ++idx)
    {
        channel[idx] = float(idx);
    }

    // Pad the texture values

    std::vector<float> chn;
    OIIO_CHECK_NO_THROW( OCIO::PadLutChannel(width, 1, channel, chn) );

    // Check the values

    const float res[15] = { 0.0f,  1.0f,  2.0f,  3.0f,  4.0f,
                            5.0f,  6.0f,  7.0f,  8.0f,  9.0f,
                            9.0f,  9.0f,  9.0f,  9.0f,  9.0f, };
    OIIO_CHECK_EQUAL(chn.size(), 15);
    for(unsigned idx=0; idx<chn.size(); ++idx)
    {
        OIIO_CHECK_EQUAL(chn[idx], res[idx]);
    }
}

OIIO_ADD_TEST(Lut1DOp, padLutChannel_two_dimensions)
{
    const unsigned width  = 5;
    const unsigned height = 3;

    // Create a channel multi row and smaller than the expected texture size

    std::vector<float> channel;
    channel.resize(width * (height-1) + 1);

    // Fill the channel

    for(unsigned idx=0; idx<channel.size(); ++idx)
    {
        channel[idx] = float(idx);
    }

    // Pad the texture values

    std::vector<float> chn;
    OIIO_CHECK_NO_THROW( OCIO::PadLutChannel(width, height, channel, chn) );

    // Check the values

    const float res[15] = { 0.0f,  1.0f,  2.0f,  3.0f,  4.0f,
                            4.0f,  5.0f,  6.0f,  7.0f,  8.0f,
                            8.0f,  9.0f, 10.0f, 10.0f, 10.0f };
    OIIO_CHECK_EQUAL(chn.size(), 15);
    for(unsigned idx=0; idx<15; ++idx)
    {
        OIIO_CHECK_EQUAL(chn[idx], res[idx]);
    }
}

OIIO_ADD_TEST(Lut1DOp, GetLutIdealSize)
{
    OIIO_CHECK_EQUAL(OCIO::GetLutIdealSize(OCIO::BIT_DEPTH_UINT8), 256);
    OIIO_CHECK_EQUAL(OCIO::GetLutIdealSize(OCIO::BIT_DEPTH_UINT16), 65536);

    OIIO_CHECK_EQUAL(OCIO::GetLutIdealSize(OCIO::BIT_DEPTH_F16), 65536);
    OIIO_CHECK_EQUAL(OCIO::GetLutIdealSize(OCIO::BIT_DEPTH_F32), 65536);
}

OIIO_ADD_TEST(Lut1DOp, makeFastLut)
{
    OCIO::Lut1DRcPtr lut1  = OCIO::Lut1D::Create();
    lut1->luts[0].resize(8);
    lut1->luts[1].resize(8);
    lut1->luts[2].resize(8);

    lut1->luts[0][0] = 0.00f; lut1->luts[1][0] = 0.12f; lut1->luts[2][0] = 0.25f;
    lut1->luts[0][1] = 0.11f; lut1->luts[1][1] = 0.22f; lut1->luts[2][1] = 0.35f;
    lut1->luts[0][2] = 0.21f; lut1->luts[1][2] = 0.32f; lut1->luts[2][2] = 0.45f;
    lut1->luts[0][3] = 0.31f; lut1->luts[1][3] = 0.42f; lut1->luts[2][3] = 0.55f;
    lut1->luts[0][4] = 0.41f; lut1->luts[1][4] = 0.52f; lut1->luts[2][4] = 0.65f;
    lut1->luts[0][5] = 0.51f; lut1->luts[1][5] = 0.52f; lut1->luts[2][5] = 0.75f;
    lut1->luts[0][6] = 0.61f; lut1->luts[1][6] = 0.72f; lut1->luts[2][6] = 0.85f;
    lut1->luts[0][7] = 0.71f; lut1->luts[1][7] = 0.82f; lut1->luts[2][7] = 0.95f;

    {
        OCIO::OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW( 
            CreateLut1DOp(ops, lut1, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD) );
        OIIO_CHECK_EQUAL( ops.size(), 1 );

        OCIO::Lut1DRcPtr res1 
            = static_cast<OCIO::Lut1DOp*>(ops[0].get())->makeFastLut1D(true);

        OIIO_CHECK_EQUAL( res1->luts[0].size(), 4096 );

        OIIO_CHECK_CLOSE( res1->luts[0][0], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][0], 0.119999997f,    1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][0], 0.25f,           1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][1], 0.000188037753f, 1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][1], 0.120170936f,    1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][1], 0.250170946f,    1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][2], 0.000376068056f, 1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][2], 0.120341875f,    1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][2], 0.250341892f,    1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][100], 0.0188034177f, 1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][100], 0.137094021f,  1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][100], 0.267094016f,  1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][300], 0.056410253f,  1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][300], 0.171282053f,  1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][300], 0.301282048f,  1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][400], 0.0752136782f, 1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][400], 0.188376069f,  1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][400], 0.318376064f,  1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][600], 0.112564094f,  1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][600], 0.222564101f,  1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][600], 0.352564096f,  1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][900], 0.16384615f,   1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][900], 0.273846149f,  1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][900], 0.403846145f,  1e-6f );
    }

    {
        OCIO::OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW( 
            CreateLut1DOp(ops, lut1, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE) );
        OIIO_CHECK_EQUAL( ops.size(), 1 );

        OCIO::Lut1DRcPtr res1 
            = static_cast<OCIO::Lut1DOp*>(ops[0].get())->makeFastLut1D(true);

        OIIO_CHECK_EQUAL( res1->luts[0].size(), 4096 );

        OIIO_CHECK_CLOSE( res1->luts[0][0], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][0], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][0], 0.0f,            1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][1], 0.000317143218f, 1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][1], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][1], 0.0f,            1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][2], 0.000634286436f, 1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][2], 0.0f,            1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][2], 0.0f,            1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][100], 0.0317143202f, 1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][100], 0.0f,          1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][100], 0.0f,          1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][300], 0.095142968f,  1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][300], 0.0f,          1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][300], 0.0f,          1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][400], 0.126857281f,  1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][400], 0.0f,          1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][400], 0.0f,          1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][600], 0.195028812f,  1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][600], 0.0378859341f, 1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][600], 0.0f,          1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][900], 0.299686074f,  1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][900], 0.142543197f,  1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][900], 0.0f,          1e-6f );

        OIIO_CHECK_CLOSE( res1->luts[0][2000], 0.68342936f,  1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[1][2000], 0.526286483f, 1e-6f );
        OIIO_CHECK_CLOSE( res1->luts[2][2000], 0.340572208f, 1e-6f );
    }
}

#endif // OCIO_UNIT_TEST
