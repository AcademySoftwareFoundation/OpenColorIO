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
#include "GpuShaderUtils.h"
#include "BitDepthUtils.h"
#include "MatrixOps.h"

#include "cpu/SSE2.h"
#include "cpu/CPULut1DOp.h"
#include "cpu/CPUInvLut1DOp.h"
#include "cpu/CPULutUtils.h"

#include "opdata/OpDataTools.h"
#include "opdata/OpDataLut1D.h"
#include "opdata/OpDataInvLut1D.h"

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
        ,   halfFlags(OpData::Lut1D::LUT_STANDARD)

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

        const unsigned idealSize = OpData::GetLutIdealSize(inputBitDepth);

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

            halfFlags = l.halfFlags;

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
#ifdef OCIO_UNIT_TEST
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

#ifdef OCIO_UNIT_TEST
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
#endif       
        
        
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
        
#ifdef OCIO_UNIT_TEST
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
#endif
        
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
        
#ifdef OCIO_UNIT_TEST
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
#endif

        void PadLutChannels(unsigned width,
                            unsigned height,
                            const std::vector<float> & channel,
                            std::vector<float> & chn)
        {
            const unsigned currWidth = (unsigned)(channel.size() / 3);

            if (height>1)
            {
                // Fill the texture values
                //
                // Make the last texel of a given row the same as the first texel 
                // of its next row.  This will preserve the continuity along row breaks 
                // as long as the lookup position used by the sampler is based on (width-1) 
                // to account for the 1 texel padding at the end of each row.

                const unsigned step = width-1;
                for(unsigned i=0; i<(currWidth-step); i+=step)
                {
                    chn.insert(chn.end(), &channel[3*i], &channel[3*(i+step)]);

                    chn.push_back(channel[3*(i+step)+0]);
                    chn.push_back(channel[3*(i+step)+1]);
                    chn.push_back(channel[3*(i+step)+2]);
                }

                // If there are still texels to fill, add them to the texture data
                const unsigned leftover = currWidth%step;
                if (leftover > 0)
                {
                    chn.insert(chn.end(), &channel[3*(currWidth-leftover)], &channel[3*(currWidth-1)]);

                    chn.push_back(channel[3*(currWidth-1)+0]);
                    chn.push_back(channel[3*(currWidth-1)+1]);
                    chn.push_back(channel[3*(currWidth-1)+2]);
                }
            }
            else
            {
                chn = channel;
            }

            // Pad the remaining of the texture with the last LUT entry.
            //  Note: GPU Textures are expected a size of width*height

            for(unsigned idx=0; idx<(width*height-(chn.size()/3)); ++idx)
            {
                chn.push_back(channel[3*(currWidth-1)+0]);
                chn.push_back(channel[3*(currWidth-1)+1]);
                chn.push_back(channel[3*(currWidth-1)+2]);
            }
        }

#ifdef OCIO_UNIT_TEST
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
#endif		
    }
    
    namespace
    {
        class Lut1DOp : public Op
        {
        public:
            Lut1DOp(OpData::OpDataLut1DRcPtr & data);
            virtual ~Lut1DOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;
            
            virtual BitDepth getInputBitDepth() const;
            virtual BitDepth getOutputBitDepth() const;
            virtual void setInputBitDepth(BitDepth bitdepth);
            virtual void setOutputBitDepth(BitDepth bitdepth);

            virtual bool isNoOp() const;
            virtual bool isIdentity() const;
            virtual bool isSameType(const OpRcPtr & op) const;
            virtual bool isInverse(const OpRcPtr & op) const;
            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual bool supportedByLegacyShader() const { return false; }
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;
            
            OpData::OpDataLut1DRcPtr m_data;

        private:
            // Lut1DOp does not record the direction. It can only be forward.

            // The computed cache identifier
            std::string m_cacheID;
            // The CPU processor
            CPUOpRcPtr m_cpu;

            Lut1DOp();
        };

        typedef OCIO_SHARED_PTR<Lut1DOp> Lut1DOpRcPtr;


        class InvLut1DOp : public Op
        {
        public:
            InvLut1DOp(OpData::OpDataInvLut1DRcPtr & data);
            virtual ~InvLut1DOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;
            
            virtual BitDepth getInputBitDepth() const;
            virtual BitDepth getOutputBitDepth() const;
            virtual void setInputBitDepth(BitDepth bitdepth);
            virtual void setOutputBitDepth(BitDepth bitdepth);

            virtual bool isNoOp() const;
            virtual bool isIdentity() const;
            virtual bool isSameType(const OpRcPtr & op) const;
            virtual bool isInverse(const OpRcPtr & op) const;
            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual bool supportedByLegacyShader() const { return false; }
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;
            
            OpData::OpDataInvLut1DRcPtr m_data;

        private:
            // InvLut1DOp does not record the direction. It can only be forward
            // (ie an inverse Lut1DOp).

            // The computed cache identifier
            std::string m_cacheID;
            // The CPU processor
            CPUOpRcPtr m_cpu;

            InvLut1DOp();
        };
        
        typedef OCIO_SHARED_PTR<InvLut1DOp> InvLut1DOpRcPtr;

        Lut1DOp::Lut1DOp(OpData::OpDataLut1DRcPtr & data)
            :   Op()
            ,   m_data(data)
            ,   m_cpu(new CPUNoOp)
        {
        }

        Lut1DOp::~Lut1DOp()
        {
        }

        OpRcPtr Lut1DOp::clone() const
        {
            OpData::OpDataLut1DRcPtr
                lut(dynamic_cast<OpData::Lut1D*>(
                    m_data->clone(OpData::OpData::DO_DEEP_COPY)));

            return OpRcPtr(new Lut1DOp(lut));
        }

        std::string Lut1DOp::getInfo() const
        {
            return "<Lut1DOp>";
        }

        std::string Lut1DOp::getCacheID() const
        {
            return m_cacheID;
        }
       
        BitDepth Lut1DOp::getInputBitDepth() const
        {
            return m_data->getInputBitDepth();
        }

        BitDepth Lut1DOp::getOutputBitDepth() const
        {
            return m_data->getOutputBitDepth();
        }

        void Lut1DOp::setInputBitDepth(BitDepth bitdepth)
        {
            m_data->setInputBitDepth(bitdepth);
        }

        void Lut1DOp::setOutputBitDepth(BitDepth bitdepth)
        {
            m_data->setOutputBitDepth(bitdepth);
        }

        bool Lut1DOp::isNoOp() const
        {
            return m_data->isNoOp();
        }

        bool Lut1DOp::isIdentity() const
        {
            return m_data->isIdentity();
        }

        bool Lut1DOp::isSameType(const OpRcPtr & op) const
        {
            // NB: InvLut1D and Lut1D have the same type.
            //     One is the inverse of the other one.
            const Lut1DOpRcPtr fwdTypedRcPtr = DynamicPtrCast<Lut1DOp>(op);
            const InvLut1DOpRcPtr invTypedRcPtr = DynamicPtrCast<InvLut1DOp>(op);
            return (bool)fwdTypedRcPtr || (bool)invTypedRcPtr;
        }

        bool Lut1DOp::isInverse(const OpRcPtr & op) const
        {
            InvLut1DOpRcPtr typedRcPtr = DynamicPtrCast<InvLut1DOp>(op);
            if(typedRcPtr)
            {
                return m_data->isInverse(*typedRcPtr->m_data);
            }

            return false;
        }

        bool Lut1DOp::hasChannelCrosstalk() const
        {
            return m_data->hasChannelCrosstalk();
        }

        void Lut1DOp::finalize()
        {
            // Only the 32f processing is natively supported
            m_data->setInputBitDepth(BIT_DEPTH_F32);
            m_data->setOutputBitDepth(BIT_DEPTH_F32);

            m_data->validate();

            // Get the CPU engine
            m_cpu = Lut1DRenderer::GetRenderer(m_data);


            // Rebuild the cache identifier
            //

            md5_state_t state;
            md5_byte_t digest[16];
            
            md5_init(&state);
            md5_append(&state, 
                      (const md5_byte_t *)&(m_data->getArray().getValues()[0]),
                      (int) (m_data->getArray().getValues().size() * sizeof(float)));
            md5_finish(&state, digest);
           
            m_cacheID = GetPrintableHash(digest);

            std::ostringstream cacheIDStream;
            cacheIDStream << "<Lut1D ";
            cacheIDStream << GetPrintableHash(digest) << " ";
            cacheIDStream << BitDepthToString(m_data->getInputBitDepth()) << " ";
            cacheIDStream << BitDepthToString(m_data->getOutputBitDepth()) << " ";
            cacheIDStream << ">";
            
            m_cacheID = cacheIDStream.str();
        }

        void Lut1DOp::apply(float* rgbaBuffer, long numPixels) const
        {
            m_cpu->apply(rgbaBuffer, (unsigned int)numPixels);
        }

        void Lut1DOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            if(getInputBitDepth()!=BIT_DEPTH_F32 || getOutputBitDepth()!=BIT_DEPTH_F32)
            {
                throw Exception("Only 32F bit depth is supported for the GPU shader");
            }

            const unsigned defaultMaxWidth = shaderDesc->getTextureMaxWidth();

            const unsigned length = unsigned(m_data->getArray().getLength());
            const unsigned width  = std::min(length, defaultMaxWidth);
            const unsigned height = (length / defaultMaxWidth) + 1;

            // Adjust LUT texture to allow for correct 2d linear interpolation, if needed.

            std::vector<float> values;
            PadLutChannels(width, height, m_data->getArray().getValues(), values);

            // Register the RGB LUT

            std::ostringstream resName;
            resName << shaderDesc->getResourcePrefix()
                    << std::string("lut1d_")
                    << shaderDesc->getNumTextures();

            const std::string name(resName.str());

            shaderDesc->addTexture(GpuShaderText::getSamplerName(name).c_str(),
                                   m_cacheID.c_str(),
                                   width, height, 
                                   GpuShaderDesc::TEXTURE_RGB_CHANNEL, 
                                   m_data->getConcreteInterpolation(),
                                   &values[0]);

            // Add the LUT code to the OCIO shader program

            if(height>1 || m_data->isInputHalfDomain())
            {
                // In case the 1D LUT length exceeds the 1D texture maximum length
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

                    if(m_data->isInputHalfDomain())
                    {
                        static const float NEG_MIN_EXP = 15.0f;
                        static const float EXP_SCALE = 1024.0f;
                        static const float HALF_DENRM_MAX = 6.09755515e-05f;  // e.g. 2^-14 - 2^-24
 
                        ss.newLine() <<   "float dep;";
                        ss.newLine() <<   "float abs_f = abs(f);";
                        ss.newLine() <<   "if (abs_f > " << (float)HALF_NRM_MIN << ")";
                        ss.newLine() <<   "{";
                        ss.indent();
                                            ss.declareVec3f("fComp", NEG_MIN_EXP, NEG_MIN_EXP, NEG_MIN_EXP);
                        ss.newLine() <<       "float absarr = min( abs_f, " << (float)HALF_MAX << ");";
                                            // Compute the exponent, scaled [-14,15].
                        ss.newLine() <<       "fComp.x = floor( log2( absarr ) );";
                                            // Lower is the greatest power of 2 <= f.
                        ss.newLine() <<       "float lower = pow( 2.0, fComp.x );";
                                            // Compute the mantissa (scaled [0-1]).
                        ss.newLine() <<       "fComp.y = ( absarr - lower ) / lower;";
                                            // The dot product recombines the parts into a raw half without the sign component:
                                            //   dep = [ exponent + mantissa + NEG_MIN_EXP] * scale
                                            ss.declareVec3f("scale", EXP_SCALE, EXP_SCALE, EXP_SCALE);
                        ss.newLine() <<       "dep = dot( fComp, scale );";
                        ss.dedent();
                        ss.newLine() <<   "}";
                        ss.newLine() <<   "else";
                        ss.newLine() <<   "{";
                        ss.indent();
                                            // Extract bits from denormalized values
                        ss.newLine() <<       "dep = abs_f * 1023.0 / " << HALF_DENRM_MAX << ";";
                        ss.dedent();
                        ss.newLine() <<   "}";

                                        // Adjust position for negative values
                        ss.newLine() <<   "dep += step(f, 0.0) * 32768.0;";

                                        // At this point 'dep' contains the raw half
                                        // Note: Raw halfs for NaN floats cannot be computed using
                                        //       floating-point operations.
                        ss.newLine() <<   ss.vec2fDecl("retVal") << ";";
                        ss.newLine() <<   "retVal.y = floor(dep / " << float(width-1) << ");";       // floor( dep / (width-1) ))
                        ss.newLine() <<   "retVal.x = dep - retVal.y * " << float(width-1) << ";";   // dep - retVal.y * (width-1)

                        ss.newLine() <<   "retVal.x = (retVal.x + 0.5) / " << float(width) << ";";   // (retVal.x + 0.5) / width;
                        ss.newLine() <<   "retVal.y = (retVal.y + 0.5) / " << float(height) << ";";  // (retVal.x + 0.5) / height;
                    }
                    else
                    {
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
                    }

                    ss.newLine() <<   "return retVal;";
                    ss.dedent();
                    ss.newLine() << "}";

                    shaderDesc->addToHelperShaderCode(ss.string().c_str());
                }
            }
            else
            {
                GpuShaderText ss(shaderDesc->getLanguage());
                ss.declareTex1D(name);
                shaderDesc->addToDeclareShaderCode(ss.string().c_str());
            }

            GpuShaderText ss(shaderDesc->getLanguage());
            ss.indent();

            ss.newLine() << "";
            ss.newLine() << "// Add a LUT 1D processing for " << name;
            ss.newLine() << "";

            ss.newLine() << "{";
            ss.indent();

            if(m_data->getHueAdjust() == OpData::Lut1D::HUE_DW3)
            {
                ss.newLine() << "// Add the pre hue adjustment";
                ss.newLine() << ss.vec3fDecl("maxval") 
                             << " = max("<< shaderDesc->getPixelName() << ".rgb, max("
                             << shaderDesc->getPixelName() << ".gbr, "<< shaderDesc->getPixelName() << ".brg));";
                ss.newLine() << ss.vec3fDecl("minval") 
                             << " = min("<< shaderDesc->getPixelName() << ".rgb, min("
                             << shaderDesc->getPixelName() << ".gbr, "<< shaderDesc->getPixelName() << ".brg));";
                ss.newLine() << "float oldChroma = max(1e-8, maxval.r - minval.r);";
                ss.newLine() << ss.vec3fDecl("delta") << " = "<< shaderDesc->getPixelName() << ".rgb - minval;";
                ss.newLine() << "";
            }

            if(height>1 || m_data->isInputHalfDomain())
            {
                const std::string str = name + "_computePos(" + shaderDesc->getPixelName();

                ss.newLine() << shaderDesc->getPixelName() << ".r = " << ss.sampleTex2D(name, str + ".r)") << ".r;";
                ss.newLine() << shaderDesc->getPixelName() << ".g = " << ss.sampleTex2D(name, str + ".g)") << ".g;";
                ss.newLine() << shaderDesc->getPixelName() << ".b = " << ss.sampleTex2D(name, str + ".b)") << ".b;";
            }
            else
            {
                const float dim = (float)m_data->getArray().getLength();

                ss.newLine() << ss.vec3fDecl(name + "_coords") 
                             << " = ("<< shaderDesc->getPixelName() << ".rgb * " 
                             << ss.vec3fConst(dim-1) 
                             << " + " << ss.vec3fConst(0.5f) << " ) / " 
                             << ss.vec3fConst(dim) << ";";

                ss.newLine() << shaderDesc->getPixelName() << ".r = " 
                             << ss.sampleTex1D(name, name + "_coords.r") << ".r;";
                ss.newLine() << shaderDesc->getPixelName() << ".g = " 
                             << ss.sampleTex1D(name, name + "_coords.g") << ".g;";
                ss.newLine() << shaderDesc->getPixelName() << ".b = " 
                             << ss.sampleTex1D(name, name + "_coords.b") << ".b;";
            }

            if(m_data->getHueAdjust() == OpData::Lut1D::HUE_DW3)
            {
                ss.newLine() << "";
                ss.newLine() << "// Add the post hue adjustment";
                ss.newLine() << ss.vec3fDecl("maxval2") << " = max("<< shaderDesc->getPixelName() 
                             << ".rgb, max("<< shaderDesc->getPixelName() << ".gbr, "
                             << shaderDesc->getPixelName() << ".brg));";
                ss.newLine() << ss.vec3fDecl("minval2") << " = min("<< shaderDesc->getPixelName() 
                             << ".rgb, min("<< shaderDesc->getPixelName() << ".gbr, "
                             << shaderDesc->getPixelName() << ".brg));";
                ss.newLine() << "float newChroma = maxval2.r - minval2.r;";
                ss.newLine() << shaderDesc->getPixelName() 
                             << ".rgb = minval2.r + delta * newChroma / oldChroma;";
            }

            ss.dedent();
            ss.newLine() << "}";

            shaderDesc->addToFunctionShaderCode(ss.string().c_str());
        }

        InvLut1DOp::InvLut1DOp(OpData::OpDataInvLut1DRcPtr & data)
            :   Op()
            ,   m_data(data)
            ,   m_cpu(new CPUNoOp)
        {
        }

        InvLut1DOp::~InvLut1DOp()
        {
        }

        OpRcPtr InvLut1DOp::clone() const
        {
            OpData::OpDataInvLut1DRcPtr
                lut(dynamic_cast<OpData::InvLut1D*>(
                    m_data->clone(OpData::OpData::DO_DEEP_COPY)));

            return OpRcPtr(new InvLut1DOp(lut));
        }

        std::string InvLut1DOp::getInfo() const
        {
            return "<InvLut1DOp>";
        }

        std::string InvLut1DOp::getCacheID() const
        {
            return m_cacheID;
        }
       
        BitDepth InvLut1DOp::getInputBitDepth() const
        {
            return m_data->getInputBitDepth();
        }

        BitDepth InvLut1DOp::getOutputBitDepth() const
        {
            return m_data->getOutputBitDepth();
        }

        void InvLut1DOp::setInputBitDepth(BitDepth bitdepth)
        {
            m_data->setInputBitDepth(bitdepth);
        }

        void InvLut1DOp::setOutputBitDepth(BitDepth bitdepth)
        {
            m_data->setOutputBitDepth(bitdepth);
        }

        bool InvLut1DOp::isNoOp() const
        {
            return m_data->isNoOp();
        }

        bool InvLut1DOp::isIdentity() const
        {
            return m_data->isIdentity();
        }

        bool InvLut1DOp::isSameType(const OpRcPtr & op) const
        {
            // NB: InvLut1D and Lut1D have the same type.
            //     One is the inverse of the other one.
            const Lut1DOpRcPtr fwdTypedRcPtr = DynamicPtrCast<Lut1DOp>(op);
            const InvLut1DOpRcPtr invTypedRcPtr = DynamicPtrCast<InvLut1DOp>(op);
            return (bool)fwdTypedRcPtr || (bool)invTypedRcPtr;
        }

        bool InvLut1DOp::isInverse(const OpRcPtr & op) const
        {
            Lut1DOpRcPtr typedRcPtr = DynamicPtrCast<Lut1DOp>(op);
            if(typedRcPtr)
            {
                return m_data->isInverse(*typedRcPtr->m_data);
            }

            return false;
        }

        bool InvLut1DOp::hasChannelCrosstalk() const
        {
            return m_data->hasChannelCrosstalk();
        }

        void InvLut1DOp::finalize()
        {
            // Only the 32f processing is natively supported
            m_data->setInputBitDepth(BIT_DEPTH_F32);
            m_data->setOutputBitDepth(BIT_DEPTH_F32);

            m_data->validate();

            m_cpu = InvLut1DRenderer::GetRenderer(m_data);

            // Rebuild the cache identifier
            //
            
            md5_state_t state;
            md5_byte_t digest[16];
            
            md5_init(&state);
            md5_append(&state, 
                      (const md5_byte_t *)&(m_data->getArray().getValues()[0]),
                      (int) (m_data->getArray().getValues().size() * sizeof(float)));
            md5_finish(&state, digest);

            m_cacheID = GetPrintableHash(digest);

            std::ostringstream cacheIDStream;
            cacheIDStream << "<InvLut1D ";
            cacheIDStream << GetPrintableHash(digest) << " ";
            cacheIDStream << BitDepthToString(m_data->getInputBitDepth()) << " ";
            cacheIDStream << BitDepthToString(m_data->getOutputBitDepth()) << " ";
            cacheIDStream << ">";
            
            m_cacheID = cacheIDStream.str();
        }

        void InvLut1DOp::apply(float* rgbaBuffer, long numPixels) const
        {
            m_cpu->apply(rgbaBuffer, (unsigned int)numPixels);
        }

        void InvLut1DOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            OpData::OpDataLut1DRcPtr 
                newLut = InvLutUtil::makeFastLut1D(m_data, true);

            OpRcPtrVec ops;
            CreateLut1DOp(ops, newLut, TRANSFORM_DIR_FORWARD);
            if(ops.size()!=1)
            {
                throw Exception("Cannot apply Lut1DOp, optimization failed.");
            }
            ops[0]->finalize();
            ops[0]->extractGpuShaderInfo(shaderDesc);
        }
    }
    

    void CreateLut1DOp(OpRcPtrVec & ops,
                       const Lut1DRcPtr & lut,
                       Interpolation interpolation,
                       TransformDirection direction)
    {
        if(direction==TRANSFORM_DIR_UNKNOWN)
        {
            throw Exception("Cannot apply Lut1DOp op, "
                            "unspecified transform direction.");
        }

        if(lut->luts[0].empty() || lut->luts[1].empty() || lut->luts[2].empty())
        {
            throw Exception("Cannot apply lut1d op, no LUT data provided.");
        }

        if(lut->luts[0].size()!=lut->luts[1].size()
            || lut->luts[0].size()!=lut->luts[2].size())
        {
            throw Exception(
                "Cannot apply lut1d op, "
                "the LUT for each channel must have the same dimensions.");
        }
        
        if(lut->isNoOp()) return;

        // TODO: Detect if lut1d can be exactly approximated as y = mx + b
        // If so, return a mtx instead.

        OpData::OpDataLut1DRcPtr data(
            new OpData::Lut1D(lut->inputBitDepth, lut->outputBitDepth, lut->halfFlags));

        switch(interpolation)
        {
            case INTERP_BEST:
            case INTERP_NEAREST:
            case INTERP_LINEAR:
            case INTERP_CUBIC:
                data->setInterpolation(interpolation);
                break;

            case INTERP_UNKNOWN:
                throw Exception("Cannot apply Lut1DOp, unspecified interpolation.");
                break;
            
            default:
                throw Exception("Cannot apply Lut1DOp op, "
                                "interpolation is not allowed for 1d luts.");
                break;
        }

        data->getArray().setLength((unsigned)lut->luts[0].size());
        data->getArray().setNumColorComponents(3);

        float * values = &data->getArray().getValues()[0];
        for(unsigned i = 0; i < lut->luts[0].size(); ++i)
        {
            values[3*i]   = lut->luts[0][i];
            values[3*i+1] = lut->luts[1][i];
            values[3*i+2] = lut->luts[2][i];
        }

        if(direction==TRANSFORM_DIR_INVERSE)
        {
            OpData::OpDataInvLut1DRcPtr inv_data(new OpData::InvLut1D(*data));
            ops.push_back(OpRcPtr(new InvLut1DOp(inv_data)));
            CreateMatrixOp(ops, lut->from_min, lut->from_max, TRANSFORM_DIR_INVERSE);
        }
        else
        {
            CreateMatrixOp(ops, lut->from_min, lut->from_max, TRANSFORM_DIR_FORWARD);
            ops.push_back(OpRcPtr(new Lut1DOp(data)));
        }
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

    void CreateLut1DOp(OpRcPtrVec & ops, 
                       OpData::OpDataLut1DRcPtr & lut,
                       TransformDirection direction)
    {
        if(lut->isNoOp()) return;

        if(direction==TRANSFORM_DIR_UNKNOWN)
        {
            throw Exception("Cannot apply Lut1DOp op, "
                            "unspecified transform direction.");
        }

        if(lut->getOpType() != OpData::OpData::Lut1DType)
        {
            throw Exception("Cannot apply Lut1DOp op, "
                            "Not a forward LUT 1D data");
        }

        if(direction==TRANSFORM_DIR_FORWARD)
        {
            ops.push_back(OpRcPtr(new Lut1DOp(lut)));
        }
        else
        {
            OpData::OpDataInvLut1DRcPtr data(new OpData::InvLut1D(*lut));
            ops.push_back(OpRcPtr(new InvLut1DOp(data)));
        }
    }

    void CreateInvLut1DOp(OpRcPtrVec & ops,
                          OpData::OpDataInvLut1DRcPtr & lut,
                          TransformDirection direction)
    {
        if(lut->isNoOp()) return;

        if(direction==TRANSFORM_DIR_UNKNOWN)
        {
            throw Exception("Cannot apply InvLut1DOp op, "
                            "unspecified transform direction.");
        }

        if(lut->getOpType() != OpData::OpData::InvLut1DType)
        {
            throw Exception("Cannot apply InvLut1DOp op, "
                            "Not an inverse LUT 1D data");
        }

        if(direction==TRANSFORM_DIR_FORWARD)
        {
            ops.push_back(OpRcPtr(new InvLut1DOp(lut)));
        }
        else
        {
            OpData::OpDataLut1DRcPtr data(new OpData::Lut1D(*lut));
            ops.push_back(OpRcPtr(new Lut1DOp(data)));
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
    // Make an identity LUT
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
    
    // Edit the LUT
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
    // Make a LUT that squares the input
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
    // Make a LUT that squares the input
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
    
    // Make another LUT, same LUT but min & max are different
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
    
    // Make a not identity LUT, and different from lut_a and lut_b
    OCIO::Lut1DRcPtr lut_c = OCIO::Lut1D::Create();
    {
        lut_c->from_min[0] = 0.0f;
        lut_c->from_min[1] = 0.0f;
        lut_c->from_min[2] = 0.0f;
        lut_c->from_max[0] = 1.0f;
        lut_c->from_max[1] = 1.0f;
        lut_c->from_max[2] = 1.0f;
        const int size = 256;
        for(int i=0; i<size; ++i)
        {
            const float x = (float)i / (float)(size-1);
            const float x2 = x*x - x;
            
            for(int c=0; c<3; ++c)
            {
                lut_c->luts[c].push_back(x2);
            }
        }
        lut_c->maxerror = 1e-5f;
        lut_c->errortype = OCIO::ERROR_RELATIVE;
    }
    
    OCIO::OpRcPtrVec ops;
    // Adding Lut1DOp
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_a,
        OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));
    // Adding InvLut1DOp
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_a,
        OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE));
    // Adding MatrixOffsetOp (i.e. min & max) and Lut1DOp
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_b,
        OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));
    // Adding InvLut1DOp and MatrixOffsetOp (i.e. min & max)
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_b,
        OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE));
    
    OIIO_CHECK_EQUAL(ops.size(), 6);

    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<Lut1DOp>");
    OIIO_CHECK_EQUAL(ops[1]->getInfo(), "<InvLut1DOp>");
    OIIO_CHECK_EQUAL(ops[2]->getInfo(), "<MatrixOffsetOp>");
    OIIO_CHECK_EQUAL(ops[3]->getInfo(), "<Lut1DOp>");
    OIIO_CHECK_EQUAL(ops[4]->getInfo(), "<InvLut1DOp>");
    OIIO_CHECK_EQUAL(ops[5]->getInfo(), "<MatrixOffsetOp>");

    OIIO_CHECK_ASSERT(ops[0]->isInverse(ops[1]));
    OIIO_CHECK_ASSERT(ops[3]->isInverse(ops[4]));

    OCIO::OpRcPtr clonedOp;
    OIIO_CHECK_NO_THROW(clonedOp = ops[3]->clone());
    OIIO_CHECK_ASSERT(ops[3]->isSameType(clonedOp));

    OIIO_CHECK_EQUAL(ops[0]->isInverse(ops[1]), true);
    OIIO_CHECK_EQUAL(ops[3]->isInverse(ops[4]), true);

    OIIO_CHECK_EQUAL(ops[0]->isInverse(ops[3]), false);
    OIIO_CHECK_EQUAL(ops[1]->isInverse(ops[4]), false);


    // add same as first
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_c,
        OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 7);

    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<Lut1DOp>");
    OIIO_CHECK_EQUAL(ops[1]->getInfo(), "<InvLut1DOp>");
    OIIO_CHECK_EQUAL(ops[2]->getInfo(), "<MatrixOffsetOp>");
    OIIO_CHECK_EQUAL(ops[3]->getInfo(), "<Lut1DOp>");
    OIIO_CHECK_EQUAL(ops[4]->getInfo(), "<InvLut1DOp>");
    OIIO_CHECK_EQUAL(ops[5]->getInfo(), "<MatrixOffsetOp>");
    OIIO_CHECK_EQUAL(ops[6]->getInfo(), "<Lut1DOp>");

    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, false));
    OIIO_CHECK_EQUAL(ops.size(), 7);

    OIIO_CHECK_EQUAL(ops[0]->getCacheID(), ops[3]->getCacheID());
    OIIO_CHECK_EQUAL(ops[1]->getCacheID(), ops[4]->getCacheID());

    OIIO_CHECK_NE(ops[0]->getCacheID(), ops[1]->getCacheID());
    OIIO_CHECK_NE(ops[0]->getCacheID(), ops[6]->getCacheID());
    OIIO_CHECK_NE(ops[1]->getCacheID(), ops[3]->getCacheID());
    OIIO_CHECK_NE(ops[1]->getCacheID(), ops[6]->getCacheID());

    // Optimize will remove LUT forward and inverse (0+1 and 3+4),
    //  and remove matrix forward and inverse 2+5
    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, true));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<Lut1DOp>");
}


#ifdef USE_SSE
OIIO_ADD_TEST(Lut1DOp, SSE)
{
    // Make a LUT that squares the input
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
    // Make a LUT that squares the input
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
    // Make an identity LUT
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
    OIIO_CHECK_THROW_WHAT(CreateLut1DOp(ops, lut,
        OCIO::INTERP_NEAREST, OCIO::TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "unspecified transform direction");
    ops.clear();
    
    OIIO_CHECK_THROW_WHAT(CreateLut1DOp(ops, lut,
        OCIO::INTERP_UNKNOWN, OCIO::TRANSFORM_DIR_FORWARD),
        OCIO::Exception, "unspecified interpolation");
    ops.clear();

    // INTERP_TETRAHEDRAL not allowed for 1d LUT.
    OIIO_CHECK_THROW_WHAT(CreateLut1DOp(ops, lut,
        OCIO::INTERP_TETRAHEDRAL, OCIO::TRANSFORM_DIR_FORWARD),
        OCIO::Exception, "interpolation is not allowed for 1d luts");
    ops.clear();

    lut->luts[0].clear();
    OIIO_CHECK_THROW_WHAT(CreateLut1DOp(ops, lut,
        OCIO::INTERP_BEST, OCIO::TRANSFORM_DIR_FORWARD),
        OCIO::Exception, "no LUT data provided");
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
        OCIO::INTERP_BEST, OCIO::TRANSFORM_DIR_FORWARD));
    
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
    OIIO_CHECK_EQUAL(OCIO::OpData::GetLutIdealSize(OCIO::BIT_DEPTH_UINT8), 256);
    OIIO_CHECK_EQUAL(OCIO::OpData::GetLutIdealSize(OCIO::BIT_DEPTH_UINT16), 65536);

    OIIO_CHECK_EQUAL(OCIO::OpData::GetLutIdealSize(OCIO::BIT_DEPTH_F16), 65536);
    OIIO_CHECK_EQUAL(OCIO::OpData::GetLutIdealSize(OCIO::BIT_DEPTH_F32), 65536);
}

OIIO_ADD_TEST(Lut1D, Basic)
{
    const OCIO::BitDepth bitDepth = OCIO::BIT_DEPTH_F32;

    // By default, this constructor creates an 'identity LUT'
    OCIO::OpData::OpDataLut1DRcPtr 
        lutData( 
            new OCIO::OpData::Lut1D(bitDepth, bitDepth, 
                                    "", "", OCIO::OpData::Descriptions(),
                                    OCIO::INTERP_LINEAR, 
                                    OCIO::OpData::Lut1D::LUT_STANDARD));

    OCIO::Lut1DOp lut(lutData);

    OIIO_CHECK_NO_THROW(lut.finalize());
    OIIO_CHECK_ASSERT(lut.m_data->isIdentity());
    OIIO_CHECK_ASSERT(false == lut.isNoOp());

    const float step
        = OCIO::OpData::GetValueStepSize(lut.m_data->getInputBitDepth(),
                                         lut.m_data->getArray().getLength());

    float myImage[8] = { 0.0f, 0.0f, 0.0f, 1.0f,
                         0.0f, 0.0f, step, 1.0f };

    const float error = 1e-6f;
    {
        OIIO_CHECK_NO_THROW(lut.apply(myImage, 2));

        OIIO_CHECK_CLOSE(myImage[0], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[1], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[2], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[3], 1.0f, error);

        OIIO_CHECK_CLOSE(myImage[4], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[5], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[6], step, error);
        OIIO_CHECK_CLOSE(myImage[7], 1.0f, error);
    }

    // No more an 'identity LUT 1D'
    const float arbitraryVal = 0.123456f;

    lut.m_data->getArray()[5] = arbitraryVal;

    OIIO_CHECK_NO_THROW(lut.finalize());
    OIIO_CHECK_ASSERT(!lut.m_data->isIdentity());
    OIIO_CHECK_ASSERT(!lut.isNoOp());

    {
        OIIO_CHECK_NO_THROW(lut.apply(myImage, 2));

        OIIO_CHECK_CLOSE(myImage[0], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[1], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[2], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[3], 1.0f, error);

        OIIO_CHECK_CLOSE(myImage[4], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[5], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[6], arbitraryVal, error);
        OIIO_CHECK_CLOSE(myImage[7], 1.0f, error);
    }

}

OIIO_ADD_TEST(Lut1D, Half)
{
    OCIO::OpData::OpDataLut1DRcPtr 
        lutData( 
            new OCIO::OpData::Lut1D(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32, 
                                    "", "", OCIO::OpData::Descriptions(),
                                    OCIO::INTERP_LINEAR, 
                                    OCIO::OpData::Lut1D::LUT_STANDARD));

    OCIO::Lut1DOp lut(lutData);

    const float step
        = OCIO::OpData::GetValueStepSize(lut.m_data->getInputBitDepth(),
                                         lut.m_data->getArray().getLength());

    // No more an 'identity LUT 1D'
    const float arbitraryVal = 0.123456f;
    lut.m_data->getArray()[5] = arbitraryVal;
    OIIO_CHECK_ASSERT(!lut.m_data->isIdentity());

    const half myImage[8] = { 0.1f, 0.3f, 0.4f, 1.0f,
                              0.0f, 0.9f, step, 0.0f };

    float resImage[8] = { 0.1f, 0.3f, 0.4f, 1.0f,
                          0.0f, 0.9f, step, 0.0f };

    OIIO_CHECK_NO_THROW(lut.finalize());
    OIIO_CHECK_NO_THROW(lut.apply(resImage, 2));

    const float error = 1e-4f;

    OIIO_CHECK_CLOSE(resImage[0], (float)myImage[0], error);
    OIIO_CHECK_CLOSE(resImage[1], (float)myImage[1], error);
    OIIO_CHECK_CLOSE(resImage[2], (float)myImage[2], error);
    OIIO_CHECK_CLOSE(resImage[3], (float)myImage[3], error);

    OIIO_CHECK_CLOSE(resImage[4], (float)myImage[4], error);
    OIIO_CHECK_CLOSE(resImage[5], (float)myImage[5], error);
    OIIO_CHECK_CLOSE(resImage[6], arbitraryVal, error);
    OIIO_CHECK_CLOSE(resImage[7], (float)myImage[7], error);
}

OIIO_ADD_TEST(Lut1D, NaN)
{
    const OCIO::BitDepth bitDepth = OCIO::BIT_DEPTH_F32;

    // By default, this constructor creates an 'identity LUT'
    OCIO::OpData::OpDataLut1DRcPtr 
        lutData( 
            new OCIO::OpData::Lut1D(bitDepth, bitDepth, 
                                    "", "", OCIO::OpData::Descriptions(),
                                    OCIO::INTERP_LINEAR, 
                                    OCIO::OpData::Lut1D::LUT_STANDARD));

    OCIO::Lut1DOp lut(lutData);

    OIIO_CHECK_NO_THROW(lut.finalize());
    OIIO_CHECK_ASSERT(lut.m_data->isIdentity());
    OIIO_CHECK_ASSERT(false == lut.isNoOp());

    const float step
        = OCIO::OpData::GetValueStepSize(lut.m_data->getInputBitDepth(),
                                         lut.m_data->getArray().getLength());

    float myImage[8] = { std::numeric_limits<float>::quiet_NaN(),
                         0.0f, 0.0f, 1.0f,
                         0.0f, 0.0f, step, 1.0f };
    
    const float error = 1e-6f;
    {
        OIIO_CHECK_NO_THROW(lut.apply(myImage, 2));

        OIIO_CHECK_CLOSE(myImage[0], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[1], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[2], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[3], 1.0f, error);

        OIIO_CHECK_CLOSE(myImage[4], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[5], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[6], step, error);
        OIIO_CHECK_CLOSE(myImage[7], 1.0f, error);
    }
}

// NB: Unit test to validate that the new LUT processings 
//     is fine (by comparing with legacy processing and expected values).
//
OIIO_ADD_TEST(Lut1D, FiniteValue)
{
    // Make a LUT that squares the input
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

    // Check lut1D with OCIO::Lut1D_Linear
    {
        const float outputBuffer_linearforward[4] = { 0.25f, 0.36f, 0.49f, 0.5f };

        float lut1dlegacy_inputBuffer_linearforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
        OCIO::Lut1D_Linear(lut1dlegacy_inputBuffer_linearforward, 1, *lut);

        OCIO::OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(
            CreateLut1DOp(ops, lut, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_EQUAL(ops.size(), 1);

        float lut1d_inputBuffer_linearforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
        
        OIIO_CHECK_NO_THROW(ops[0]->finalize());
        OIIO_CHECK_NO_THROW(ops[0]->apply(lut1d_inputBuffer_linearforward, 1));
        for(int i=0; i <4; ++i)
        {
            OIIO_CHECK_CLOSE(lut1d_inputBuffer_linearforward[i], 
                             outputBuffer_linearforward[i], 1e-5f);

            OIIO_CHECK_CLOSE(lut1dlegacy_inputBuffer_linearforward[i], 
                             outputBuffer_linearforward[i], 1e-5f);

            OIIO_CHECK_CLOSE(lut1dlegacy_inputBuffer_linearforward[i], 
                             lut1d_inputBuffer_linearforward[i], 1e-5f);
        }
    }

    // Check Invlut1D with OCIO::Lut1D_LinearInverse
    {    
        const float inputBuffer_linearinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };

        float lut1dlegacy_outputBuffer_linearinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
        OCIO::Lut1D_LinearInverse(lut1dlegacy_outputBuffer_linearinverse, 1, *lut);

        OCIO::OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(
            CreateLut1DOp(ops, lut, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE));
        OIIO_CHECK_EQUAL(ops.size(), 1);

        float lut1d_outputBuffer_linearinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };

        OIIO_CHECK_NO_THROW(ops[0]->finalize());
        OIIO_CHECK_NO_THROW(ops[0]->apply(lut1d_outputBuffer_linearinverse, 1));
        for(int i=0; i <4; ++i)
        {
            OIIO_CHECK_CLOSE(lut1dlegacy_outputBuffer_linearinverse[i], 
                             inputBuffer_linearinverse[i], 1e-5f);

            OIIO_CHECK_CLOSE(lut1d_outputBuffer_linearinverse[i], 
                             inputBuffer_linearinverse[i], 1e-5f);

            OIIO_CHECK_CLOSE(lut1dlegacy_outputBuffer_linearinverse[i], 
                             lut1d_outputBuffer_linearinverse[i], 1e-5f);
        }
    }
}


//
// Unit tests using clf files
//

#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

#include "FileFormatCTF.h"

void Apply(const OCIO::OpData::OpDataVec & opDataVec, float * img, unsigned numPixels)
{
    // NB: Converting the OpData to Op is done using a clone(DO_DEEP_COPY)
    //     which means that the FinalizeOpVec does not affect OpData instances.
    //     This is mandatory when loading ctf files with bit depths other than 32f,
    //     to avoid changing the test conditions.

    OCIO::OpRcPtrVec ops;
    OCIO::CreateOpVecFromOpDataVec(ops, opDataVec, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::FinalizeOpVec(ops);

    for(unsigned idx=0; idx<ops.size(); ++idx)
    {
        ops[idx]->apply(img, numPixels);
    }
}


OIIO_ADD_TEST(Lut1D, Apply_Special_values)
{
    const std::string ctfFile("ACES_to_ACESproxy_sim_nd.ctf");

    OCIO::CTF::Reader::TransformPtr transform;
    OIIO_CHECK_NO_THROW(transform = OCIO::LoadCTFTestFile(ctfFile));

    const OCIO::OpData::OpDataVec & opList = transform->getOps();
    OIIO_CHECK_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::Lut1DType);

    const OCIO::OpData::Lut1D * lut = static_cast<const OCIO::OpData::Lut1D*>(opList[0]);
    OIIO_CHECK_ASSERT(lut);

    const OCIO::OpData::Array & lutArray = lut->getArray();
    const float step 
        = OCIO::OpData::GetValueStepSize(lut->getInputBitDepth(), lutArray.getLength());

    float inputFrame[] = {
        0.0f,         0.5f*step,     step,          1.0f,
        3000.0f*step, 32000.0f*step, 65535.0f*step, 1.0f  };

    OIIO_CHECK_NO_THROW(Apply(opList, inputFrame, 2));

    // -Inf is mapped to -MAX_FLOAT
    const float negmax = -std::numeric_limits<float>::max();

    const float lutElem_1 = -3216.000488281f;
    const float lutElem_3000 = -539.565734863f;

    const float rtol = powf(2.f, -14.f);

    // Account for rescaling artifacts that can occur when the output depth
    // of the 1d LUT operation in 'ACES_to_ACESproxy_sim_nd.ctf' is not 32f
    const float outRange = OCIO::GetBitDepthMaxValue(lut->getOutputBitDepth());

    OIIO_CHECK_ASSERT(OCIO::equalWithSafeRelError(inputFrame[0], negmax, rtol, 1.0f) );
    OIIO_CHECK_ASSERT(OCIO::equalWithSafeRelError(inputFrame[1], (lutElem_1 + negmax) * 0.5f, rtol, 1.0f) );
    OIIO_CHECK_ASSERT(OCIO::equalWithSafeRelError(inputFrame[2], lutElem_1 / outRange, rtol, 1.0f) );
    OIIO_CHECK_EQUAL(inputFrame[3], 1.0f);

    OIIO_CHECK_ASSERT(OCIO::equalWithSafeRelError(inputFrame[4], lutElem_3000 / outRange, rtol, 1.0f) );
    OIIO_CHECK_ASSERT(OCIO::equalWithSafeRelError(inputFrame[5], negmax, rtol, 1.0f) );
    OIIO_CHECK_ASSERT(OCIO::equalWithSafeRelError(inputFrame[6], negmax, rtol, 1.0f) );
    OIIO_CHECK_EQUAL(inputFrame[7], 1.0f);
}

OIIO_ADD_TEST(Lut1D, Apply_Half_domain_hue_adjust)
{
    const std::string ctfFile("ACESv0.2_RRT_LUT.ctf");

    OCIO::CTF::Reader::TransformPtr transform;
    OIIO_CHECK_NO_THROW(transform = OCIO::LoadCTFTestFile(ctfFile));

    const OCIO::OpData::OpDataVec & opList = transform->getOps();
    OIIO_CHECK_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::Lut1DType);


    float inputFrame[] = {
        0.05f, 0.18f, 1.1f, 0.5f,
        2.3f, 0.01f, 0.3f, 1.0f  };

    OIIO_CHECK_NO_THROW(Apply(opList, inputFrame, 2));

    const float rtol = 1e-6f;
    const float minExpected = 1e-3f;

    OIIO_CHECK_ASSERT(
        OCIO::equalWithSafeRelError(inputFrame[0], 0.54780269f, rtol, minExpected) );

    OIIO_CHECK_ASSERT(
        OCIO::equalWithSafeRelError(inputFrame[1],
                                    9.57448578f, 
                                    rtol, minExpected) );  // would be 5.0 w/out hue adjust

    OIIO_CHECK_ASSERT(
        OCIO::equalWithSafeRelError(inputFrame[2], 73.45562744f, rtol, minExpected) );

    OIIO_CHECK_EQUAL( inputFrame[3], 0.5f );

    OIIO_CHECK_ASSERT(
        OCIO::equalWithSafeRelError(inputFrame[4], 188.087067f, rtol, minExpected) );
    OIIO_CHECK_ASSERT(
        OCIO::equalWithSafeRelError(inputFrame[5], 0.0324990489f, rtol, minExpected) );
    OIIO_CHECK_ASSERT(
        OCIO::equalWithSafeRelError(inputFrame[6], 
                                    23.8472710f, 
                                    rtol, minExpected) );  // would be 11.3372078 w/out hue adjust

    OIIO_CHECK_EQUAL( inputFrame[7], 1.0f );
}

OIIO_ADD_TEST(InvLut1D, Apply_Half)
{
    const OCIO::BitDepth inBD  = OCIO::BIT_DEPTH_F32;
    const OCIO::BitDepth outBD = OCIO::BIT_DEPTH_F32;

    static const std::string ctfFile("lut1d_halfdom.ctf");

    OCIO::CTF::Reader::TransformPtr transform;
    OIIO_CHECK_NO_THROW(transform = OCIO::LoadCTFTestFile(ctfFile));

    OCIO::OpData::OpDataVec & opList = transform->getOps();
    OIIO_CHECK_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::Lut1DType);

    opList[0]->setInputBitDepth(outBD);
    opList[0]->setOutputBitDepth(inBD);

    const OCIO::OpData::Lut1D * fwdLut 
        = dynamic_cast<const OCIO::OpData::Lut1D*>(opList[0]);

    OCIO::OpData::InvLut1D invLut(*fwdLut);

    const float inImage[12] = {
         1.f,    1.f,   0.5f, 0.f,
         0.001f, 0.1f,  4.f,  0.5f,  // test positive half domain of R, G, B channels
        -0.08f, -1.f, -10.f,  1.f }; // test negative half domain of R, G, B channels

    float inImage1[12]; for(unsigned i=0;i<12;++i) { inImage1[i]=inImage[i]; }

    // Apply forward LUT.
    OCIO::OpData::OpDataVec ops1;
    ops1.append(fwdLut->clone(OCIO::OpData::Lut1D::DO_SHALLOW_COPY));

    OIIO_CHECK_NO_THROW(Apply(ops1, inImage1, 3));

    // Apply inverse LUT.
    OCIO::OpData::OpDataVec ops2;
    ops2.append(invLut.clone(OCIO::OpData::Lut1D::DO_SHALLOW_COPY));

    float inImage2[12]; for(unsigned i=0;i<12;++i) { inImage2[i]=inImage1[i]; }

    OIIO_CHECK_NO_THROW(Apply(ops2, inImage2, 3));

    // Compare the two applys
    for(unsigned i = 0; i < 12; ++i)
    {
        OIIO_CHECK_ASSERT(!OCIO::FloatsDiffer(inImage2[i], inImage[i], 50, false));
    }

    // Repeat with style = FAST.
    invLut.setInvStyle(OCIO::OpData::InvLut1D::FAST);
    ops1.replace(invLut.clone(OCIO::OpData::Lut1D::DO_SHALLOW_COPY), 0);

    for(unsigned i=0;i<12;++i) { inImage2[i]=inImage1[i]; }

    OIIO_CHECK_NO_THROW(Apply(ops1, inImage2, 3));

    // Compare the two applys
    for(unsigned i = 0; i < 12; ++i)
    {
        OIIO_CHECK_ASSERT(!OCIO::FloatsDiffer(inImage2[i], inImage[i], 50, false) );
    }
}

OIIO_ADD_TEST(Lut1D, Lut1D_compose_with_bit_depth)
{
    const std::string fileNameLut("lut1d_comp.clf");

    OCIO::CTF::Reader::TransformPtr transform;
    OIIO_CHECK_NO_THROW(transform = OCIO::LoadCTFTestFile(fileNameLut));

    OCIO::OpData::OpDataVec & opList = transform->getOps();
    OIIO_CHECK_EQUAL(opList.size(), 2);
    OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::Lut1DType);
    OIIO_CHECK_EQUAL(opList[1]->getOpType(), OCIO::OpData::OpData::Lut1DType);

    // Transform keeps raw pointers and will delete them. Need to copy them
    // to use shared pointers.
    OCIO::OpData::OpDataLut1DRcPtr pL1(new OCIO::OpData::Lut1D(
        *dynamic_cast<OCIO::OpData::Lut1D*>(opList[0])));
    OCIO::OpData::OpDataLut1DRcPtr pL2(new OCIO::OpData::Lut1D(
        *dynamic_cast<OCIO::OpData::Lut1D*>(opList[1])));

    {
        const OCIO::OpData::OpDataLut1DRcPtr lutComposed
            = OCIO::OpData::Compose(pL1, pL2, OCIO::OpData::COMPOSE_RESAMPLE_NO);

        const float error = 1e-5f;
        OIIO_CHECK_EQUAL(lutComposed->getArray().getLength(), 2);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[0], 0.00744791f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[1], 0.03172233f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[2], 0.07058375f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[3], 0.3513808f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[4], 0.51819527f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[5], 0.67463773f, error);
    }
    {
        const OCIO::OpData::OpDataLut1DRcPtr lutComposed
            = OCIO::OpData::Compose(pL1, pL2, OCIO::OpData::COMPOSE_RESAMPLE_INDEPTH);

        const float error = 1e-5f;
        OIIO_CHECK_EQUAL(lutComposed->getArray().getLength(), 256);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[  0], 0.00744791f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[  1], 0.03172233f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[  2], 0.07058375f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[383], 0.28073114f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[384], 0.09914176f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[385], 0.1866852f,  error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[765], 0.3513808f,  error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[766], 0.51819527f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[767], 0.67463773f, error);
    }
}

OIIO_ADD_TEST(Lut1D, InverseTwice)
{
    // Make a LUT that squares the input
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();
    lut->from_min[0] = 0.0f;
    lut->from_min[1] = 0.0f;
    lut->from_min[2] = 0.0f;

    lut->from_max[0] = 1.0f;
    lut->from_max[1] = 1.0f;
    lut->from_max[2] = 1.0f;

    const int size = 256;
    for (int i = 0; i < size; ++i)
    {
        const float x = (float)i / (float)(size - 1);
        const float x2 = x*x;

        for (int c = 0; c < 3; ++c)
        {
            lut->luts[c].push_back(x2);
        }
    }

    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::ERROR_RELATIVE;
    bool isNoOp = true;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_EQUAL(isNoOp, false);

    {
        const float outputBuffer_linearinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };

        // Create inverse lut
        OCIO::OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(
            CreateLut1DOp(ops, lut, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE));
        OIIO_CHECK_EQUAL(ops.size(), 1);

        const float lut1d_inputBuffer_reference[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
        float lut1d_inputBuffer_linearinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };

        OIIO_CHECK_NO_THROW(ops[0]->finalize());
        OIIO_CHECK_NO_THROW(ops[0]->apply(lut1d_inputBuffer_linearinverse, 1));
        for (int i = 0; i < 4; ++i)
        {
            OIIO_CHECK_CLOSE(lut1d_inputBuffer_linearinverse[i],
                             outputBuffer_linearinverse[i], 1e-5f);
        }

        // Inverse the inverse
        OCIO::InvLut1DOp * pLut = dynamic_cast<OCIO::InvLut1DOp*>(ops[0].get());
        OIIO_CHECK_ASSERT(pLut);
        OCIO::OpData::OpDataVec opsData;
        pLut->m_data->inverse(opsData);
        OIIO_CHECK_EQUAL(opsData.size(), 1);
        OCIO::OpData::Lut1D* pLutData = dynamic_cast<OCIO::OpData::Lut1D*>(opsData.remove(0));
        OCIO::OpData::OpDataLut1DRcPtr lutData(pLutData);
        OIIO_CHECK_NO_THROW(
            OCIO::CreateLut1DOp(ops, lutData, OCIO::TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_EQUAL(ops.size(), 2);

        // apply the inverse
        OIIO_CHECK_NO_THROW(ops[1]->finalize());
        OIIO_CHECK_NO_THROW(ops[1]->apply(lut1d_inputBuffer_linearinverse, 1));

        // verify we are back on the input
        for (int i = 0; i < 4; ++i)
        {
            OIIO_CHECK_CLOSE(lut1d_inputBuffer_linearinverse[i],
                             lut1d_inputBuffer_reference[i], 1e-5f);
        }
    }

}

#endif // OCIO_UNIT_TEST
