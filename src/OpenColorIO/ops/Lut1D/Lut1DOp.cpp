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

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "HashUtils.h"
#include "GpuShaderUtils.h"
#include "MathUtils.h"
#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Lut1D/Lut1DOpCPU.h"
#include "ops/Lut1D/Lut1DOpGPU.h"
#include "ops/Matrix/MatrixOps.h"
#include "OpTools.h"
#include "SSE.h"

OCIO_NAMESPACE_ENTER
{
    // Code related to legacy struct will eventually go away.
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
        return std::make_shared<Lut1D>();
    }

    Lut1DRcPtr Lut1D::CreateIdentity(BitDepth inputBitDepth,
                                     BitDepth outputBitDepth)
    {
        Lut1DRcPtr lut = Lut1D::Create();
        lut->inputBitDepth = inputBitDepth;
        lut->outputBitDepth = outputBitDepth;

        const unsigned long idealSize = Lut1DOpData::GetLutIdealSize(inputBitDepth);

        lut->luts[0].resize(idealSize);
        lut->luts[1].resize(idealSize);
        lut->luts[2].resize(idealSize);

        const float stepValue
            = GetBitDepthMaxValue(outputBitDepth) / (float(idealSize) - 1.0f);

        for(unsigned long idx=0; idx<lut->luts[0].size(); ++idx)
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

            inputBitDepth = l.inputBitDepth;
            outputBitDepth = l.outputBitDepth;

            // Note: do not copy the mutex
        }

        return *this;
    }

    namespace
    {
        bool IsLut1DNoOp(const Lut1D & lut,
                         float maxerror,
                         Lut1D::ErrorType errortype)
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
                    
                    if(errortype == Lut1D::ERROR_ABSOLUTE)
                    {
                        if(!EqualWithAbsError(identval, lutval, maxerror))
                        {
                            return false;
                        }
                    }
                    else if(errortype == Lut1D::ERROR_RELATIVE)
                    {
                        if(!EqualWithRelError(identval, lutval, maxerror))
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

        if (luts[0].empty() || luts[1].empty() || luts[2].empty())
            throw Exception("Cannot compute cacheID of invalid Lut1D");

        if (!m_cacheID.empty())
            return m_cacheID;

        finalize();
        return m_cacheID;
    }

    bool Lut1D::isNoOp() const
    {
        AutoMutex lock(m_mutex);

        if (luts[0].empty() || luts[1].empty() || luts[2].empty())
            throw Exception("Cannot compute noOp of invalid Lut1D");

        if (!m_cacheID.empty())
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

        if (m_isNoOp)
        {
            m_cacheID = "<NULL 1D>";
        }
        else
        {
            md5_state_t state;
            md5_byte_t digest[16];

            md5_init(&state);
            md5_append(&state, (const md5_byte_t *)from_min, (int)(3 * sizeof(float)));
            md5_append(&state, (const md5_byte_t *)from_max, (int)(3 * sizeof(float)));

            for (int i = 0; i<3; ++i)
            {
                md5_append(&state, (const md5_byte_t *)&(luts[i][0]),
                    (int)(luts[i].size() * sizeof(float)));
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
                if(!IsNan(rgbaBuffer[0]))
                    rgbaBuffer[0] = lookupNearest_1D(mInv_x_maxIndex[0] * (rgbaBuffer[0] - b[0]), maxIndex[0], startPos[0]);
                if(!IsNan(rgbaBuffer[1]))
                    rgbaBuffer[1] = lookupNearest_1D(mInv_x_maxIndex[1] * (rgbaBuffer[1] - b[1]), maxIndex[1], startPos[1]);
                if(!IsNan(rgbaBuffer[2]))
                    rgbaBuffer[2] = lookupNearest_1D(mInv_x_maxIndex[2] * (rgbaBuffer[2] - b[2]), maxIndex[2], startPos[2]);
                
                rgbaBuffer += 4;
            }
        }
#endif
#if USE_SSE && OCIO_UNIT_TEST
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
                
                if(!IsNan(result[0]))
                    rgbaBuffer[0] = startPos[0][(int)(result[0])];
                if(!IsNan(result[1]))
                    rgbaBuffer[1] = startPos[1][(int)(result[1])];
                if(!IsNan(result[2]))
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
                if(!IsNan(rgbaBuffer[0]))
                    rgbaBuffer[0] = lookupLinear_1D(mInv_x_maxIndex[0] * (rgbaBuffer[0] - b[0]), maxIndex[0], startPos[0]);
                if(!IsNan(rgbaBuffer[1]))
                    rgbaBuffer[1] = lookupLinear_1D(mInv_x_maxIndex[1] * (rgbaBuffer[1] - b[1]), maxIndex[1], startPos[1]);
                if(!IsNan(rgbaBuffer[2]))
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
                if(!IsNan(rgbaBuffer[0]))
                    rgbaBuffer[0] = m[0] * reverseLookupNearest_1D(rgbaBuffer[0], startPos[0], endPos[0]) + b[0];
                if(!IsNan(rgbaBuffer[1]))
                    rgbaBuffer[1] = m[1] * reverseLookupNearest_1D(rgbaBuffer[1], startPos[1], endPos[1]) + b[1];
                if(!IsNan(rgbaBuffer[2]))
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
                if(!IsNan(rgbaBuffer[0]))
                    rgbaBuffer[0] = m[0] * reverseLookupLinear_1D(rgbaBuffer[0], startPos[0], endPos[0], invMaxIndex[0]) + b[0];
                if(!IsNan(rgbaBuffer[1]))
                    rgbaBuffer[1] = m[1] * reverseLookupLinear_1D(rgbaBuffer[1], startPos[1], endPos[1], invMaxIndex[0]) + b[1];
                if(!IsNan(rgbaBuffer[2]))
                    rgbaBuffer[2] = m[2] * reverseLookupLinear_1D(rgbaBuffer[2], startPos[2], endPos[2], invMaxIndex[0]) + b[2];
                
                rgbaBuffer += 4;
            }
        }
    
    }
    // End of code using the legacy Lut1D struct.

    namespace
    {
        class Lut1DOp;
        typedef OCIO_SHARED_PTR<Lut1DOp> Lut1DOpRcPtr;
        typedef OCIO_SHARED_PTR<const Lut1DOp> ConstLut1DOpRcPtr;

        class Lut1DOp : public Op
        {
        public:
            Lut1DOp(Lut1DOpDataRcPtr & lutData);
            virtual ~Lut1DOp();

            OpRcPtr clone() const override;

            std::string getInfo() const override;
            std::string getCacheID() const override;

            bool isNoOp() const override;
            bool isIdentity() const override;
            bool isSameType(ConstOpRcPtr & op) const override;
            bool isInverse(ConstOpRcPtr & op) const override;
            bool hasChannelCrosstalk() const override;
            void finalize() override;

            bool supportedByLegacyShader() const override { return false; }
            void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;

            ConstLut1DOpDataRcPtr lut1DData() const { return DynamicPtrCast<const Lut1DOpData>(data()); }
            Lut1DOpDataRcPtr lut1DData() { return DynamicPtrCast<Lut1DOpData>(data()); }

        private:

            ConstLut1DOpDataRcPtr m_lut_gpu_apply;
            
            Lut1DOp() = delete;
        };

        Lut1DOp::Lut1DOp(Lut1DOpDataRcPtr & lut1D)
        {
            data() = lut1D;
        }

        Lut1DOp::~Lut1DOp()
        {
        }

        OpRcPtr Lut1DOp::clone() const
        {
            Lut1DOpDataRcPtr lut = lut1DData()->clone();
            return std::make_shared<Lut1DOp>(lut);
        }

        std::string Lut1DOp::getInfo() const
        {
            return "<Lut1DOp>";
        }

        std::string Lut1DOp::getCacheID() const
        {
            return m_cacheID;
        }

        bool Lut1DOp::isNoOp() const
        {
            return lut1DData()->isNoOp();
        }

        bool Lut1DOp::isIdentity() const
        {
            return lut1DData()->isIdentity();
        }

        bool Lut1DOp::isSameType(ConstOpRcPtr & op) const
        {
            ConstLut1DOpRcPtr typedRcPtr = DynamicPtrCast<const Lut1DOp>(op);
            return (bool)typedRcPtr;
        }

        bool Lut1DOp::isInverse(ConstOpRcPtr & op) const
        {
            ConstLut1DOpRcPtr typedRcPtr = DynamicPtrCast<const Lut1DOp>(op);
            if (typedRcPtr)
            {
                ConstLut1DOpDataRcPtr lutData = typedRcPtr->lut1DData();
                return lut1DData()->isInverse(lutData);
            }

            return false;
        }

        bool Lut1DOp::hasChannelCrosstalk() const
        {
            return lut1DData()->hasChannelCrosstalk();
        }

        void Lut1DOp::finalize()
        {
            Lut1DOpDataRcPtr lutData = lut1DData();

            // TODO: Only the 32f processing is natively supported
            lutData->setInputBitDepth(BIT_DEPTH_F32);
            lutData->setOutputBitDepth(BIT_DEPTH_F32);

            lutData->validate();
            lutData->finalize();

            const Lut1DOp & constThis = *this;
            ConstLut1DOpDataRcPtr lutDataConst = constThis.lut1DData();

            m_cpuOp = GetLut1DRenderer(lutDataConst, BIT_DEPTH_F32, BIT_DEPTH_F32);

            // Rebuild the cache identifier
            std::ostringstream cacheIDStream;
            cacheIDStream << "<Lut1D ";
            cacheIDStream << lutDataConst->getCacheID() << " ";
            cacheIDStream << ">";

            m_cacheID = cacheIDStream.str();
        }

        void Lut1DOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            if (getInputBitDepth() != BIT_DEPTH_F32 || getOutputBitDepth() != BIT_DEPTH_F32)
            {
                throw Exception("Only 32F bit depth is supported for the GPU shader");
            }

            ConstLut1DOpDataRcPtr lutData = lut1DData();
            if (lutData->getDirection() == TRANSFORM_DIR_INVERSE)
            {
                Lut1DOpDataRcPtr newLut = Lut1DOpData::MakeFastLut1DFromInverse(lutData, true);
                if (!newLut)
                {
                    throw Exception("Cannot apply Lut1DOp, inversion failed.");
                }

                Lut1DOp invLut(newLut);
                invLut.finalize();
                invLut.extractGpuShaderInfo(shaderDesc);
            }
            else
            {
                GetLut1DGPUShaderProgram(shaderDesc, lutData);
            }
        }
    }

    void CreateLut1DOp(OpRcPtrVec & ops,
                       Lut1DRcPtr & lut,
                       Interpolation interpolation,
                       TransformDirection direction)
    {
        if (direction == TRANSFORM_DIR_UNKNOWN)
        {
            throw Exception("Cannot apply Lut1DOp op, "
                            "unspecified transform direction.");
        }

        if (lut->luts[0].empty() || lut->luts[1].empty() || lut->luts[2].empty())
        {
            throw Exception("Cannot apply lut1d op, no LUT data provided.");
        }

        if (lut->luts[0].size() != lut->luts[1].size()
            || lut->luts[0].size() != lut->luts[2].size())
        {
            throw Exception(
                "Cannot apply lut1d op, "
                "the LUT for each channel must have the same dimensions.");
        }

        if (lut->isNoOp()) return;

        // TODO: Detect if lut1d can be exactly approximated as y = mx + b
        // If so, return a mtx instead.

        Lut1DOpDataRcPtr data =
            std::make_shared<Lut1DOpData>(lut->inputBitDepth,
                                          lut->outputBitDepth,
                                          Lut1DOpData::LUT_STANDARD);

        switch (interpolation)
        {
        case INTERP_BEST:
        case INTERP_NEAREST:
        case INTERP_LINEAR:
            data->setInterpolation(interpolation);
            break;

        case INTERP_UNKNOWN:
            throw Exception("Cannot apply Lut1DOp, unspecified interpolation.");
            break;

        default:
            throw Exception("Cannot apply Lut1DOp op, the specified "
                            "interpolation is not allowed for 1D LUTs.");
            break;
        }

        data->getArray().setLength((unsigned long)lut->luts[0].size());
        data->getArray().setMaxColorComponents();

        float * values = &data->getArray().getValues()[0];
        for (unsigned long i = 0; i < lut->luts[0].size(); ++i)
        {
            values[3 * i    ] = lut->luts[0][i];
            values[3 * i + 1] = lut->luts[1][i];
            values[3 * i + 2] = lut->luts[2][i];
        }

        if (direction == TRANSFORM_DIR_INVERSE)
        {
            CreateLut1DOp(ops, data, TRANSFORM_DIR_INVERSE);
            CreateMinMaxOp(ops, lut->from_min, lut->from_max, TRANSFORM_DIR_INVERSE);
        }
        else
        {
            CreateMinMaxOp(ops, lut->from_min, lut->from_max, TRANSFORM_DIR_FORWARD);
            CreateLut1DOp(ops, data, TRANSFORM_DIR_FORWARD);
        }
    }
    
    void CreateLut1DOp(OpRcPtrVec & ops,
                       Lut1DOpDataRcPtr & lut,
                       TransformDirection direction)
    {
        if (lut->isNoOp()) return;

        // TODO: Detect if 1D LUT can be exactly approximated as y = mx + b
        // If so, return a mtx instead.

        if (direction != TRANSFORM_DIR_FORWARD
            && direction != TRANSFORM_DIR_INVERSE)
        {
            throw Exception("Cannot apply Lut1DOp op, "
                            "unspecified transform direction.");
        }

        if (direction == TRANSFORM_DIR_FORWARD)
        {
            ops.push_back(std::make_shared<Lut1DOp>(lut));
        }
        else
        {
            Lut1DOpDataRcPtr data = lut->inverse();
            ops.push_back(std::make_shared<Lut1DOp>(data));
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
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

#include <cstring>

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"
#include "UnitTestUtils.h"

OIIO_ADD_TEST(Lut1DOpStruct, no_op)
{
    // Make an identity LUT.
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();
    
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
    lut->errortype = OCIO::Lut1D::ERROR_RELATIVE;
    OIIO_CHECK_ASSERT(lut->isNoOp());
    
    lut->unfinalize();
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::Lut1D::ERROR_ABSOLUTE;
    OIIO_CHECK_ASSERT(lut->isNoOp());

    // Edit the LUT.
    // These should NOT be identity.
    lut->unfinalize();
    lut->luts[0][125] += 1e-3f;
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::Lut1D::ERROR_RELATIVE;
    OIIO_CHECK_ASSERT(!lut->isNoOp());

    lut->unfinalize();
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::Lut1D::ERROR_ABSOLUTE;
    OIIO_CHECK_ASSERT(!lut->isNoOp());
}

namespace
{
OCIO::Lut1DRcPtr CreateSquareLut()
{
    // Make a LUT that squares the input.
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();

    const int size = 256;
    lut->luts[0].resize(size);
    lut->luts[1].resize(size);
    lut->luts[2].resize(size);
    for (int i = 0; i < size; ++i)
    {
        float x = (float)i / (float)(size - 1);
        float x2 = x*x;

        for (int c = 0; c < 3; ++c)
        {
            lut->luts[c][i] = x2;
        }
    }
    return lut;
}
}

OIIO_ADD_TEST(Lut1DOpStruct, FiniteValue)
{
    OCIO::Lut1DRcPtr lut = CreateSquareLut();
    
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::Lut1D::ERROR_RELATIVE;
    bool isNoOp = true;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_ASSERT(!isNoOp);
    
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut, OCIO::INTERP_LINEAR,
                                      OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut, OCIO::INTERP_LINEAR,
                                      OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[0]->finalize());
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    float inputBuffer_linearforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    float inputBuffer_linearforward2[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    const float outputBuffer_linearforward[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    OCIO::Lut1D_Linear(inputBuffer_linearforward, 1, *lut);
    ops[0]->apply(inputBuffer_linearforward2, 1);
    for(int i=0; i <4; ++i)
    {
        OIIO_CHECK_CLOSE(inputBuffer_linearforward[i], outputBuffer_linearforward[i], 1e-5f);
        OIIO_CHECK_CLOSE(inputBuffer_linearforward2[i], outputBuffer_linearforward[i], 1e-5f);
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
    float outputBuffer_linearinverse2[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    OCIO::Lut1D_LinearInverse(outputBuffer_linearinverse, 1, *lut);
    ops[1]->apply(outputBuffer_linearinverse2, 1);
    for(int i=0; i <4; ++i)
    {
        OIIO_CHECK_CLOSE(inputBuffer_linearinverse[i], outputBuffer_linearinverse[i], 1e-5f);
        OIIO_CHECK_CLOSE(inputBuffer_linearinverse[i], outputBuffer_linearinverse2[i], 1e-5f);
    }
    
    const float inputBuffer_nearestinverse[4] = { 0.498039f, 0.6f, 0.698039f, 0.5f };
    float outputBuffer_nearestinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    OCIO::Lut1D_NearestInverse(outputBuffer_nearestinverse, 1, *lut);
    for(int i=0; i <4; ++i)
    {
        OIIO_CHECK_CLOSE(inputBuffer_nearestinverse[i], outputBuffer_nearestinverse[i], 1e-5f);
    }
}

OIIO_ADD_TEST(Lut1DOp, arbitrary_value)
{
    OCIO::Lut1DRcPtr lut = CreateSquareLut();
    lut->from_min[0] = -0.25f;
    lut->from_min[1] = -0.25f;
    lut->from_min[2] = -0.25f;

    lut->from_max[0] = +1.25f;
    lut->from_max[1] = +1.25f;
    lut->from_max[2] = +1.25f;

    const float inputBuffer_linearforward[16]
        = { -0.50f, -0.25f, -0.10f, 0.00f,
             0.50f,  0.60f,  0.70f, 0.80f,
             0.90f,  1.00f,  1.10f, 1.20f,
             1.30f,  1.40f,  1.50f, 1.60f };

    float outputBuffer_linearforward[16];
    for(unsigned long i=0; i<16; ++i) { outputBuffer_linearforward[i] = inputBuffer_linearforward[i]; }

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


OIIO_ADD_TEST(Lut1DOp, extrapolation_errors)
{
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();

    // Simple y=x+0.1 LUT.
    for(int c=0; c<3; ++c)
    {
        lut->luts[c].push_back(0.1f);
        lut->luts[c].push_back(0.6f);
        lut->luts[c].push_back(1.1f);
    }

    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::Lut1D::ERROR_RELATIVE;
    bool isNoOp = true;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_ASSERT(!isNoOp);

    const int PIXELS = 5;
    float inputBuffer_linearforward[PIXELS*4] = {
        -0.1f,   -0.2f, -10.0f, 0.0f,
         0.5f,    1.0f,   1.1f, 0.0f,
        10.1f,   55.0f,   2.3f, 0.0f,
         9.1f,  1.0e6f, 1.0e9f, 0.0f,
        4.0e9f, 9.5e7f,   0.5f, 0.0f };
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


OIIO_ADD_TEST(Lut1DOp, inverse)
{
    // Make a LUT that squares the input.
    OCIO::Lut1DRcPtr lut_a = CreateSquareLut();
    lut_a->maxerror = 1e-5f;
    lut_a->errortype = OCIO::Lut1D::ERROR_RELATIVE;

    // Make another LUT, same LUT but min & max are different.
    OCIO::Lut1DRcPtr lut_b = CreateSquareLut();
    lut_b->from_min[0] = 0.5f;
    lut_b->from_min[1] = 0.6f;
    lut_b->from_min[2] = 0.7f;
    lut_b->from_max[0] = 1.0f;
    lut_b->from_max[1] = 1.0f;
    lut_b->from_max[2] = 1.0f;
    lut_b->maxerror = 1e-5f;
    lut_b->errortype = OCIO::Lut1D::ERROR_RELATIVE;

    // Make a not identity LUT, and different from lut_a and lut_b.
    OCIO::Lut1DRcPtr lut_c = CreateSquareLut();
    {
        const int size = 256;
        for (int i = 0; i<size; ++i)
        {
            const float x = (float)i / (float)(size - 1);

            for (int c = 0; c<3; ++c)
            {
                lut_c->luts[c][i] -= x;
            }
        }
        lut_c->maxerror = 1e-5f;
        lut_c->errortype = OCIO::Lut1D::ERROR_RELATIVE;
    }

    OCIO::OpRcPtrVec ops;
    // Adding Lut1DOp.
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_a,
                                      OCIO::INTERP_LINEAR,
                                      OCIO::TRANSFORM_DIR_FORWARD));
    // Adding inverse Lut1DOp.
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_a,
                                      OCIO::INTERP_LINEAR,
                                      OCIO::TRANSFORM_DIR_INVERSE));
    // Adding MatrixOffsetOp (i.e. min & max) and Lut1DOp.
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_b,
                                      OCIO::INTERP_LINEAR,
                                      OCIO::TRANSFORM_DIR_FORWARD));
    // Adding inverse Lut1DOp and MatrixOffsetOp (i.e. min & max).
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_b,
                                      OCIO::INTERP_LINEAR,
                                      OCIO::TRANSFORM_DIR_INVERSE));

    OIIO_REQUIRE_EQUAL(ops.size(), 6);

    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<Lut1DOp>");
    OIIO_CHECK_EQUAL(ops[1]->getInfo(), "<Lut1DOp>");
    OIIO_CHECK_EQUAL(ops[2]->getInfo(), "<MatrixOffsetOp>");
    OIIO_CHECK_EQUAL(ops[3]->getInfo(), "<Lut1DOp>");
    OIIO_CHECK_EQUAL(ops[4]->getInfo(), "<Lut1DOp>");
    OIIO_CHECK_EQUAL(ops[5]->getInfo(), "<MatrixOffsetOp>");

    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op3 = ops[3];
    OCIO::ConstOpRcPtr op4 = ops[4];

    OIIO_CHECK_ASSERT(ops[0]->isInverse(op1));
    OIIO_CHECK_ASSERT(ops[3]->isInverse(op4));

    OCIO::ConstOpRcPtr clonedOp = ops[3]->clone();
    OIIO_CHECK_ASSERT(ops[3]->isSameType(clonedOp));

    OIIO_CHECK_EQUAL(ops[0]->isInverse(op1), true);
    OIIO_CHECK_EQUAL(ops[3]->isInverse(op4), true);

    OIIO_CHECK_EQUAL(ops[0]->isInverse(op3), false);
    OIIO_CHECK_EQUAL(ops[1]->isInverse(op4), false);

    // Add same as first.
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut_c,
        OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 7);

    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<Lut1DOp>");
    OIIO_CHECK_EQUAL(ops[1]->getInfo(), "<Lut1DOp>");
    OIIO_CHECK_EQUAL(ops[2]->getInfo(), "<MatrixOffsetOp>");
    OIIO_CHECK_EQUAL(ops[3]->getInfo(), "<Lut1DOp>");
    OIIO_CHECK_EQUAL(ops[4]->getInfo(), "<Lut1DOp>");
    OIIO_CHECK_EQUAL(ops[5]->getInfo(), "<MatrixOffsetOp>");
    OIIO_CHECK_EQUAL(ops[6]->getInfo(), "<Lut1DOp>");

    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, false));
    OIIO_REQUIRE_EQUAL(ops.size(), 7);

    OIIO_CHECK_EQUAL(ops[0]->getCacheID(), ops[3]->getCacheID());
    OIIO_CHECK_EQUAL(ops[1]->getCacheID(), ops[4]->getCacheID());

    OIIO_CHECK_NE(ops[0]->getCacheID(), ops[1]->getCacheID());
    OIIO_CHECK_NE(ops[0]->getCacheID(), ops[6]->getCacheID());
    OIIO_CHECK_NE(ops[1]->getCacheID(), ops[3]->getCacheID());
    OIIO_CHECK_NE(ops[1]->getCacheID(), ops[6]->getCacheID());

    // Optimize will remove LUT forward and inverse (0+1 and 3+4),
    // and remove matrix forward and inverse 2+5.
    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, true));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<Lut1DOp>");
}


#ifdef USE_SSE
OIIO_ADD_TEST(Lut1DOp, sse)
{
    // Make a LUT that squares the input.
    OCIO::Lut1DRcPtr lut = CreateSquareLut();
    
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::Lut1D::ERROR_RELATIVE;
    bool isNoOp = true;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_ASSERT(!isNoOp);

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


OIIO_ADD_TEST(Lut1DOp, nan_inf)
{
    // Make a LUT that squares the input.
    OCIO::Lut1DRcPtr lut = CreateSquareLut();
    
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::Lut1D::ERROR_RELATIVE;
    OIIO_CHECK_ASSERT(!lut->isNoOp());
    
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

OIIO_ADD_TEST(Lut1DOp, throw_no_op)
{
    // Make an identity LUT.
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();

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
    lut->errortype = (OCIO::Lut1D::ErrorType)0;
    OIIO_CHECK_THROW_WHAT(lut->isNoOp(),
                          OCIO::Exception, "Unknown error type");

    lut->errortype = OCIO::Lut1D::ERROR_RELATIVE;
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

OIIO_ADD_TEST(Lut1DOp, throw_op)
{
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();
    for (int c = 0; c<3; ++c)
    {
        lut->luts[c].push_back(0.1f);
        lut->luts[c].push_back(1.1f);
    }
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::Lut1D::ERROR_RELATIVE;
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_THROW_WHAT(CreateLut1DOp(ops, lut,
                                        OCIO::INTERP_NEAREST,
                                        OCIO::TRANSFORM_DIR_UNKNOWN),
                          OCIO::Exception, "unspecified transform direction");
    
    OIIO_CHECK_THROW_WHAT(CreateLut1DOp(ops, lut,
                                        OCIO::INTERP_UNKNOWN,
                                        OCIO::TRANSFORM_DIR_FORWARD),
                          OCIO::Exception, "unspecified interpolation");

    // INTERP_TETRAHEDRAL not allowed for 1D LUT.
    OIIO_CHECK_THROW_WHAT(CreateLut1DOp(ops, lut,
                                        OCIO::INTERP_TETRAHEDRAL,
                                        OCIO::TRANSFORM_DIR_FORWARD),
                          OCIO::Exception,
                          "interpolation is not allowed");
    ops.clear();

    lut->luts[0].clear();
    OIIO_CHECK_THROW_WHAT(CreateLut1DOp(ops, lut,
                                        OCIO::INTERP_BEST,
                                        OCIO::TRANSFORM_DIR_FORWARD),
                          OCIO::Exception, "no LUT data provided");
}

OIIO_ADD_TEST(Lut1DOp, gpu)
{
    OCIO::Lut1DRcPtr lut = OCIO::Lut1D::Create();
    for (int c = 0; c < 3; ++c)
    {
        lut->luts[c].push_back(0.1f);
        lut->luts[c].push_back(1.1f);
    }
    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::Lut1D::ERROR_RELATIVE;
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut,
                                      OCIO::INTERP_NEAREST,
                                      OCIO::TRANSFORM_DIR_FORWARD));
    
    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->supportedByLegacyShader(), false);
}

OIIO_ADD_TEST(Lut1DOp, identity_lut_1d)
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

OIIO_ADD_TEST(Lut1D, basic)
{
    const OCIO::BitDepth bitDepth = OCIO::BIT_DEPTH_F32;

    // By default, this constructor creates an 'identity LUT'.
    OCIO::Lut1DOpDataRcPtr lutData =
        std::make_shared<OCIO::Lut1DOpData>(bitDepth, bitDepth,
                                            "", OCIO::OpData::Descriptions(),
                                            OCIO::INTERP_LINEAR,
                                            OCIO::Lut1DOpData::LUT_STANDARD);

    OCIO::Lut1DOp lut(lutData);

    OIIO_CHECK_NO_THROW(lut.finalize());
    OIIO_CHECK_ASSERT(lutData->isIdentity());
    OIIO_CHECK_ASSERT(!lut.isNoOp());

    const float step = OCIO::GetBitDepthMaxValue(lutData->getInputBitDepth())
                       / ((float)lutData->getArray().getLength() - 1.0f);

    float myImage[8] = { 0.0f, 0.0f, 0.0f, 1.0f,
                         0.0f, 0.0f, step, 1.0f };

    const float error = 1e-6f;
    {
        OIIO_CHECK_NO_THROW(lut.apply(myImage, myImage, 2));

        OIIO_CHECK_CLOSE(myImage[0], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[1], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[2], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[3], 1.0f, error);

        OIIO_CHECK_CLOSE(myImage[4], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[5], 0.0f, error);
        OIIO_CHECK_CLOSE(myImage[6], step, error);
        OIIO_CHECK_CLOSE(myImage[7], 1.0f, error);
    }

    // No more an 'identity LUT 1D'.
    const float arbitraryVal = 0.123456f;

    lutData->getArray()[5] = arbitraryVal;

    OIIO_CHECK_NO_THROW(lut.finalize());
    OIIO_CHECK_ASSERT(!lutData->isIdentity());
    OIIO_CHECK_ASSERT(!lut.isNoOp());

    {
        OIIO_CHECK_NO_THROW(lut.apply(myImage, myImage, 2));

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

OIIO_ADD_TEST(Lut1D, half)
{
    OCIO::Lut1DOpDataRcPtr
        lutData(
            new OCIO::Lut1DOpData(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32,
                                  "", OCIO::OpData::Descriptions(),
                                  OCIO::INTERP_LINEAR,
                                  OCIO::Lut1DOpData::LUT_STANDARD));

    OCIO::Lut1DOp lut(lutData);

    const float step = OCIO::GetBitDepthMaxValue(lutData->getInputBitDepth())
                       / ((float)lutData->getArray().getLength() - 1.0f);

    // No more an 'identity LUT 1D'
    const float arbitraryVal = 0.123456f;
    lutData->getArray()[5] = arbitraryVal;
    OIIO_CHECK_ASSERT(!lutData->isIdentity());

    const half myImage[8] = { 0.1f, 0.3f, 0.4f, 1.0f,
                              0.0f, 0.9f, step, 0.0f };

    float resImage[8] = { 0.1f, 0.3f, 0.4f, 1.0f,
                          0.0f, 0.9f, step, 0.0f };

    // TODO: The SC test is intended to test half evaluation using myImage
    // as input.  Adjust after half support is added to apply.
    OIIO_CHECK_NO_THROW(lut.finalize());
    OIIO_CHECK_NO_THROW(lut.apply(resImage, resImage, 2));

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

OIIO_ADD_TEST(Lut1D, nan)
{
    const OCIO::BitDepth bitDepth = OCIO::BIT_DEPTH_F32;

    // By default, this constructor creates an 'identity LUT'
    OCIO::Lut1DOpDataRcPtr
        lutData(
            new OCIO::Lut1DOpData(bitDepth, bitDepth,
                                  "", OCIO::OpData::Descriptions(),
                                  OCIO::INTERP_LINEAR,
                                  OCIO::Lut1DOpData::LUT_STANDARD));

    OCIO::Lut1DOp lut(lutData);

    OIIO_CHECK_NO_THROW(lut.finalize());
    OIIO_CHECK_ASSERT(lut.isIdentity());
    OIIO_CHECK_ASSERT(!lut.isNoOp());

    const float step = OCIO::GetBitDepthMaxValue(lutData->getInputBitDepth())
                       / ((float)lutData->getArray().getLength() - 1.0f);

    float myImage[8] = {
        std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f, 1.0f,
                                           0.0f, 0.0f, step, 1.0f };

    const float error = 1e-6f;
    OIIO_CHECK_NO_THROW(lut.apply(myImage, myImage, 2));

    OIIO_CHECK_CLOSE(myImage[0], 0.0f, error);
    OIIO_CHECK_CLOSE(myImage[1], 0.0f, error);
    OIIO_CHECK_CLOSE(myImage[2], 0.0f, error);
    OIIO_CHECK_CLOSE(myImage[3], 1.0f, error);

    OIIO_CHECK_CLOSE(myImage[4], 0.0f, error);
    OIIO_CHECK_CLOSE(myImage[5], 0.0f, error);
    OIIO_CHECK_CLOSE(myImage[6], step, error);
    OIIO_CHECK_CLOSE(myImage[7], 1.0f, error);
}

OIIO_ADD_TEST(Lut1D, finite_value)
{
    // Make a LUT that squares the input.
    OCIO::Lut1DRcPtr lut = CreateSquareLut();

    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::Lut1D::ERROR_RELATIVE;
    bool isNoOp = true;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_ASSERT(!isNoOp);

    // Check lut1D with OCIO::Lut1D_Linear.
    {
        const float outputBuffer_linearforward[4] = { 0.25f, 0.36f, 0.49f, 0.5f };

        float lut1dlegacy_inputBuffer_linearforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
        OCIO::Lut1D_Linear(lut1dlegacy_inputBuffer_linearforward, 1, *lut);

        OCIO::OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(
            CreateLut1DOp(ops, lut, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));
        OIIO_REQUIRE_EQUAL(ops.size(), 1);

        float lut1d_inputBuffer_linearforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };

        OIIO_CHECK_NO_THROW(ops[0]->finalize());
        OIIO_CHECK_NO_THROW(ops[0]->apply(lut1d_inputBuffer_linearforward, 1));
        for (int i = 0; i <4; ++i)
        {
            OIIO_CHECK_CLOSE(lut1d_inputBuffer_linearforward[i],
                             outputBuffer_linearforward[i], 1e-5f);

            OIIO_CHECK_CLOSE(lut1dlegacy_inputBuffer_linearforward[i],
                             outputBuffer_linearforward[i], 1e-5f);

            OIIO_CHECK_CLOSE(lut1dlegacy_inputBuffer_linearforward[i],
                             lut1d_inputBuffer_linearforward[i], 1e-5f);
        }
    }

    // Check Invlut1D with OCIO::Lut1D_LinearInverse.
    {
        const float inputBuffer_linearinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };

        float lut1dlegacy_outputBuffer_linearinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
        OCIO::Lut1D_LinearInverse(lut1dlegacy_outputBuffer_linearinverse, 1, *lut);

        OCIO::OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(
            CreateLut1DOp(ops, lut, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_INVERSE));
        OIIO_REQUIRE_EQUAL(ops.size(), 1);

        float lut1d_outputBuffer_linearinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };

        OIIO_CHECK_NO_THROW(ops[0]->finalize());
        OIIO_CHECK_NO_THROW(ops[0]->apply(lut1d_outputBuffer_linearinverse, 1));
        for (int i = 0; i <4; ++i)
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

OIIO_ADD_TEST(Lut1D, finite_value_hue_adjust)
{
    // Make a LUT that squares the input.
    OCIO::Lut1DRcPtr lut = CreateSquareLut();

    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::Lut1D::ERROR_RELATIVE;
    bool isNoOp = true;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_ASSERT(!isNoOp);

    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(
        CreateLut1DOp(ops, lut, OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::OpRcPtr cloned = ops[0]->clone();
    OCIO::Lut1DOpRcPtr typedRcPtr = OCIO::DynamicPtrCast<OCIO::Lut1DOp>(cloned);
    typedRcPtr->lut1DData()->setHueAdjust(OCIO::Lut1DOpData::HUE_DW3);
    
    const float outputBuffer_linearforward[4] = { 0.25f,
                                                  0.37000f, // (Hue adj modifies green here.)
                                                  0.49f,
                                                  0.5f };
    float lut1d_inputBuffer_linearforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };

    OIIO_CHECK_NO_THROW(typedRcPtr->finalize());
    OIIO_CHECK_NO_THROW(typedRcPtr->apply(lut1d_inputBuffer_linearforward, 1));
    for (int i = 0; i <4; ++i)
    {
        OIIO_CHECK_CLOSE(lut1d_inputBuffer_linearforward[i],
                         outputBuffer_linearforward[i], 1e-5f);
    }

    OCIO::Lut1DOpDataRcPtr invData = typedRcPtr->lut1DData()->inverse();
    OCIO::Lut1DOpDataRcPtr invDataExact = invData->clone();
    invDataExact->setInversionQuality(OCIO::LUT_INVERSION_BEST);
    OIIO_CHECK_NO_THROW(
        CreateLut1DOp(ops, invData, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(
        CreateLut1DOp(ops, invDataExact, OCIO::TRANSFORM_DIR_FORWARD));

    OIIO_REQUIRE_EQUAL(ops.size(), 3);

    const float inputBuffer_linearinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    float lut1d_outputBuffer_linearinverse[4] = { 0.25f, 0.37f, 0.49f, 0.5f };
    float lut1d_outputBuffer_linearinverseEx[4] = { 0.25f, 0.37f, 0.49f, 0.5f };

    OIIO_CHECK_NO_THROW(ops[1]->finalize());
    OIIO_CHECK_NO_THROW(ops[2]->finalize());
    OIIO_CHECK_NO_THROW(ops[1]->apply(lut1d_outputBuffer_linearinverse, 1)); // fast
    OIIO_CHECK_NO_THROW(ops[2]->apply(lut1d_outputBuffer_linearinverseEx, 1)); // exact
    for (int i = 0; i <4; ++i)
    {
        OIIO_CHECK_CLOSE(lut1d_outputBuffer_linearinverse[i],
                         inputBuffer_linearinverse[i], 1e-5f);
        OIIO_CHECK_CLOSE(lut1d_outputBuffer_linearinverseEx[i],
                         inputBuffer_linearinverse[i], 1e-5f);
    }
}


//
// Unit tests using clf files.
//

namespace
{

void Apply(const OCIO::OpRcPtrVec & ops, float * img, long numPixels)
{
    for (auto op : ops)
    {
        op->apply(img, numPixels);
    }
}

}

OIIO_ADD_TEST(Lut1D, apply_half_domain_hue_adjust)
{
    const std::string ctfFile("lut1d_hd_hueAdjust.ctf");

    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OIIO_CHECK_NO_THROW(BuildOpsTest(ops, ctfFile, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    auto op = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData = op->data();
    OIIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);

    auto lut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);
    OIIO_REQUIRE_ASSERT(lut);

    float inputFrame[] = {
        0.05f, 0.18f, 1.1f, 0.5f,
        2.3f, 0.01f, 0.3f, 1.0f };

    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops));
    OIIO_CHECK_NO_THROW(Apply(ops, inputFrame, 2));

    const float rtol = 1e-6f;
    const float minExpected = 1e-3f;

    OIIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(inputFrame[0], 0.54780269f, rtol, minExpected));

    OIIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(inputFrame[1],
                                    9.57448578f,
                                    rtol, minExpected));  // would be 5.0 w/out hue adjust

    OIIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(inputFrame[2], 73.45562744f, rtol, minExpected));

    OIIO_CHECK_EQUAL(inputFrame[3], 0.5f);

    OIIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(inputFrame[4], 188.087067f, rtol, minExpected));
    OIIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(inputFrame[5], 0.0324990489f, rtol, minExpected));
    OIIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(inputFrame[6],
                                    23.8472710f,
                                    rtol, minExpected));  // would be 11.3372078 w/out hue adjust

    OIIO_CHECK_EQUAL(inputFrame[7], 1.0f);
}

OIIO_ADD_TEST(InvLut1D, apply_half)
{
    const OCIO::BitDepth inBD = OCIO::BIT_DEPTH_F32;
    const OCIO::BitDepth outBD = OCIO::BIT_DEPTH_F32;

    static const std::string ctfFile("lut1d_halfdom.ctf");

    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OIIO_CHECK_NO_THROW(BuildOpsTest(ops, ctfFile, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    auto op = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData = op->data();
    OIIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);

    auto lut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);
    OIIO_REQUIRE_ASSERT(lut);

    auto fwdLut = lut->clone();
    fwdLut->setInputBitDepth(outBD);
    fwdLut->setOutputBitDepth(inBD);

    OCIO::OpRcPtrVec ops1;
    auto fwdOp = std::make_shared<OCIO::Lut1DOp>(fwdLut);
    ops1.push_back(fwdOp);

    const float inImage[12] = {
         1.f,    1.f,   0.5f, 0.f,
         0.001f, 0.1f,  4.f,  0.5f,  // test positive half domain of R, G, B channels
        -0.08f, -1.f, -10.f,  1.f }; // test negative half domain of R, G, B channels

    float inImage1[12];
    memcpy(inImage1, inImage, 12 * sizeof(float));

    // Apply forward LUT.
    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops1));
    OIIO_CHECK_NO_THROW(Apply(ops1, inImage1, 3));

    // Apply inverse LUT.
    OCIO::OpRcPtrVec ops2;
    auto invLut = lut->inverse();
    invLut->setInversionQuality(OCIO::LUT_INVERSION_EXACT);
    auto invOp = std::make_shared<OCIO::Lut1DOp>(invLut);
    ops2.push_back(invOp);

    float inImage2[12];
    memcpy(inImage2, inImage1, 12 * sizeof(float));

    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops2));
    OIIO_CHECK_NO_THROW(Apply(ops2, inImage2, 3));

    // Compare the two applys
    for (unsigned i = 0; i < 12; ++i)
    {
        OIIO_CHECK_ASSERT(!OCIO::FloatsDiffer(inImage2[i], inImage[i], 50, false));
    }

    // Repeat with style = LUT_INVERSION_FAST.
    OCIO::OpRcPtrVec ops3;
    invLut = lut->inverse();
    invLut->setInversionQuality(OCIO::LUT_INVERSION_FAST);
    invLut->setFileBitDepth(inBD);
    invOp = std::make_shared<OCIO::Lut1DOp>(invLut);
    ops3.push_back(invOp);

    memcpy(inImage2, inImage1, 12 * sizeof(float));

    OIIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops3));
    OIIO_CHECK_NO_THROW(Apply(ops3, inImage2, 3));

    // Compare the two applys
    for (unsigned i = 0; i < 12; ++i)
    {
        OIIO_CHECK_ASSERT(!OCIO::FloatsDiffer(inImage2[i], inImage[i], 50, false));
    }
}

OIIO_ADD_TEST(Lut1D, lut_1d_compose_with_bit_depth)
{
    const std::string ctfFile("lut1d_comp.clf");

    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OIIO_CHECK_NO_THROW(BuildOpsTest(ops, ctfFile, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OIIO_REQUIRE_EQUAL(ops.size(), 3);
    auto op = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData = op->data();
    OIIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);
    auto lut1 = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);
    OIIO_REQUIRE_ASSERT(lut1);
    op = std::const_pointer_cast<const OCIO::Op>(ops[2]);
    opData = op->data();
    OIIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);
    auto lut2 = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);

    {
        auto lutComposed = lut1->clone();
        OCIO::Lut1DOpData::Compose(lutComposed, lut2, OCIO::Lut1DOpData::COMPOSE_RESAMPLE_NO);

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
        auto lutComposed = lut1->clone();
        OCIO::Lut1DOpData::Compose(lutComposed, lut2, OCIO::Lut1DOpData::COMPOSE_RESAMPLE_INDEPTH);

        const float error = 1e-5f;
        OIIO_CHECK_EQUAL(lutComposed->getArray().getLength(), 256);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[0], 0.00744791f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[1], 0.03172233f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[2], 0.07058375f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[383], 0.28073114f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[384], 0.09914176f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[385], 0.1866852f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[765], 0.3513808f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[766], 0.51819527f, error);
        OIIO_CHECK_CLOSE(lutComposed->getArray().getValues()[767], 0.67463773f, error);
    }
}

OIIO_ADD_TEST(Lut1D, inverse_twice)
{
    // Make a LUT that squares the input.
    OCIO::Lut1DRcPtr lut = CreateSquareLut();

    lut->maxerror = 1e-5f;
    lut->errortype = OCIO::Lut1D::ERROR_RELATIVE;
    bool isNoOp = true;
    OIIO_CHECK_NO_THROW(isNoOp = lut->isNoOp());
    OIIO_CHECK_ASSERT(!isNoOp);

    const float outputBuffer_linearinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };

    // Create inverse lut.
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut, OCIO::INTERP_LINEAR,
                                      OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);

    const float lut1d_inputBuffer_reference[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    float lut1d_inputBuffer_linearinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };

    OIIO_CHECK_NO_THROW(ops[0]->finalize());
    OIIO_CHECK_NO_THROW(ops[0]->apply(lut1d_inputBuffer_linearinverse, 1));
    for (int i = 0; i < 4; ++i)
    {
        OIIO_CHECK_CLOSE(lut1d_inputBuffer_linearinverse[i],
                         outputBuffer_linearinverse[i], 1e-5f);
    }

    // Inverse the inverse.
    OCIO::Lut1DOp * pLut = dynamic_cast<OCIO::Lut1DOp*>(ops[0].get());
    OIIO_CHECK_ASSERT(pLut);
    OCIO::Lut1DOpDataRcPtr lutData = pLut->lut1DData()->inverse();
    OIIO_CHECK_NO_THROW(OCIO::CreateLut1DOp(ops, lutData,
                                            OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);

    // Apply the inverse.
    OIIO_CHECK_NO_THROW(ops[1]->finalize());
    OIIO_CHECK_NO_THROW(ops[1]->apply(lut1d_inputBuffer_linearinverse, 1));

    // Verify we are back on the input.
    for (int i = 0; i < 4; ++i)
    {
        OIIO_CHECK_CLOSE(lut1d_inputBuffer_linearinverse[i],
                         lut1d_inputBuffer_reference[i], 1e-5f);
    }
}

#endif // OCIO_UNIT_TEST

// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Blue
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Green
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Red
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT1D_hue_adjust
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT1D_identity_half
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT1D_identity_half_to_integer
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT1D_identity_integer_to_half
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT1D_HALF_CODE
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_identity
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_increasing
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_decreasing_reversals
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_clamp_to_lut_range
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_flat_start_or_end
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_halfinput
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1DHalf_identity
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1DHalf_ctf
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1DHalf_fclut
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1D_hueAdjust
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererInvLUT1DHalf_hueAdjust
