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

#include <cmath>
#include <cstring>
#include <iostream>
#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "LogOps.h"
#include "MathUtils.h"


OCIO_NAMESPACE_ENTER
{
    namespace
    {
        const float FLTMIN = std::numeric_limits<float>::min();
        
        const int FLOAT_DECIMALS = 7;
        
        // k * log(mx+b, base) + kb
        // the caller is responsible for base != 1.0
        // TODO: pull the precomputation into the caller?
        
        void ApplyLinToLog(float* rgbaBuffer, long numPixels,
                           const float * k,
                           const float * m,
                           const float * b,
                           const float * base,
                           const float * kb)
        {
            // We account for the change of base by rolling the multiplier
            // in with 'k'
            
            const float knew[3] = { k[0] / logf(base[0]),
                                    k[1] / logf(base[1]),
                                    k[2] / logf(base[2]) };
            
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                rgbaBuffer[0] = knew[0] * logf(std::max(m[0]*rgbaBuffer[0] + b[0], FLTMIN)) + kb[0];
                rgbaBuffer[1] = knew[1] * logf(std::max(m[1]*rgbaBuffer[1] + b[1], FLTMIN)) + kb[1];
                rgbaBuffer[2] = knew[2] * logf(std::max(m[2]*rgbaBuffer[2] + b[2], FLTMIN)) + kb[2];
                
                rgbaBuffer += 4;
            }
        }
        
        // the caller is responsible for m != 0
        // the caller is responsible for k != 0
        // TODO: pull the precomputation into the caller?
        
        void ApplyLogToLin(float* rgbaBuffer, long numPixels,
                           const float * k,
                           const float * m,
                           const float * b,
                           const float * base,
                           const float * kb)
        {
            const float kinv[3] = { 1.0f / k[0],
                                    1.0f / k[1],
                                    1.0f / k[2] };
            
            const float minv[3] = { 1.0f / m[0],
                                    1.0f / m[1],
                                    1.0f / m[2] };
            
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                rgbaBuffer[0] = minv[0] * (powf(base[0], kinv[0]*(rgbaBuffer[0]-kb[0])) - b[0]);
                rgbaBuffer[1] = minv[1] * (powf(base[1], kinv[1]*(rgbaBuffer[1]-kb[1])) - b[1]);
                rgbaBuffer[2] = minv[2] * (powf(base[2], kinv[2]*(rgbaBuffer[2]-kb[2])) - b[2]);
                
                rgbaBuffer += 4;
            }
        }
        
    }
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    namespace
    {
        class LogOp : public Op
        {
        public:
            LogOp(const float * k,
                  const float * m,
                  const float * b,
                  const float * base,
                  const float * kb,
                  TransformDirection direction);
            virtual ~LogOp();
            
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
            
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;
            
        private:
            float m_k[3];
            float m_m[3];
            float m_b[3];
            float m_base[3];
            float m_kb[3];
            TransformDirection m_direction;
            
            std::string m_cacheID;
        };
        
        typedef OCIO_SHARED_PTR<LogOp> LogOpRcPtr;
        
        LogOp::LogOp(const float * k,
                     const float * m,
                     const float * b,
                     const float * base,
                     const float * kb,
                     TransformDirection direction):
                                       Op(),
                                       m_direction(direction)
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception(
                    "Cannot create LogOp with unspecified transform direction.");
            }

            memcpy(m_k, k, sizeof(float)*3);
            memcpy(m_m, m, sizeof(float)*3);
            memcpy(m_b, b, sizeof(float)*3);
            memcpy(m_base, base, sizeof(float)*3);
            memcpy(m_kb, kb, sizeof(float)*3);
        }
        
        OpRcPtr LogOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new LogOp(m_k, m_m, m_b, m_base, m_kb, m_direction));
            return op;
        }
        
        LogOp::~LogOp()
        { }
        
        std::string LogOp::getInfo() const
        {
            return "<LogOp>";
        }
        
        std::string LogOp::getCacheID() const
        {
            return m_cacheID;
        }
        
        BitDepth LogOp::getInputBitDepth() const
        {
            // TODO: To be implemented when OpDataLog will be in
            return BIT_DEPTH_F32;
        }

        BitDepth LogOp::getOutputBitDepth() const
        {
            // TODO: To be implemented when OpDataLog will be in
            return BIT_DEPTH_F32;
        }

        void LogOp::setInputBitDepth(BitDepth /*bitdepth*/)
        {
            // TODO: To be implemented when OpDataLog will be in
        }

        void LogOp::setOutputBitDepth(BitDepth /*bitdepth*/)
        {
            // TODO: To be implemented when OpDataLog will be in
        }

        bool LogOp::isNoOp() const
        {
            // TODO: To be implemented when OpDataLog is ready
            return false;
        }

        bool LogOp::isIdentity() const
        {
            // TODO: To be implemented when OpDataLog is ready
            return false;
        }

        bool LogOp::isSameType(const OpRcPtr & op) const
        {
            LogOpRcPtr typedRcPtr = DynamicPtrCast<LogOp>(op);
            if(!typedRcPtr) return false;
            return true;
        }
        
        bool LogOp::isInverse(const OpRcPtr & op) const
        {
            LogOpRcPtr typedRcPtr = DynamicPtrCast<LogOp>(op);
            if(!typedRcPtr) return false;
            
            if(GetInverseTransformDirection(m_direction) != typedRcPtr->m_direction)
                return false;
            
            float error = std::numeric_limits<float>::min();
            if(!VecsEqualWithRelError(m_k, 3, typedRcPtr->m_k, 3, error))
                return false;
            if(!VecsEqualWithRelError(m_m, 3, typedRcPtr->m_m, 3, error))
                return false;
            if(!VecsEqualWithRelError(m_b, 3, typedRcPtr->m_b, 3, error))
                return false;
            if(!VecsEqualWithRelError(m_base, 3, typedRcPtr->m_base, 3, error))
                return false;
            if(!VecsEqualWithRelError(m_kb, 3, typedRcPtr->m_kb, 3, error))
                return false;
            
            return true;
        }
        
        bool LogOp::hasChannelCrosstalk() const
        {
            return false;
        }
        
        void LogOp::finalize()
        {
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                if(VecContainsOne(m_base, 3))
                    throw Exception("LogOp Exception, base cannot be 1.");
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                if(VecContainsZero(m_m, 3))
                    throw Exception("LogOp Exception, m (slope) cannot be 0.");
                if(VecContainsZero(m_k, 3))
                    throw Exception("LogOp Exception, k (multiplier) cannot be 0.");
            }
            
            std::ostringstream cacheIDStream;
            cacheIDStream << "<LogOp ";
            cacheIDStream.precision(FLOAT_DECIMALS);
            for(int i=0; i<3; ++i)
            {
                cacheIDStream << m_k[i] << " ";
                cacheIDStream << m_m[i] << " ";
                cacheIDStream << m_b[i] << " ";
                cacheIDStream << m_base[i] << " ";
                cacheIDStream << m_kb[i] << " ";
            }
            
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << BitDepthToString(getInputBitDepth()) << " ";
            cacheIDStream << BitDepthToString(getOutputBitDepth()) << " ";
            cacheIDStream << ">";
            
            m_cacheID = cacheIDStream.str();
        }
        
        void LogOp::apply(float* rgbaBuffer, long numPixels) const
        {
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                ApplyLinToLog(rgbaBuffer, numPixels,
                              m_k, m_m, m_b, m_base, m_kb);
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                ApplyLogToLin(rgbaBuffer, numPixels,
                              m_k, m_m, m_b, m_base, m_kb);
            }
        } // Op::process
        
        void LogOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            if(getInputBitDepth()!=BIT_DEPTH_F32 
                || getOutputBitDepth()!=BIT_DEPTH_F32)
            {
                throw Exception("Only 32F bit depth is supported for the GPU shader");
            }


            GpuShaderText ss(shaderDesc->getLanguage());
            ss.indent();

            ss.newLine() << "";
            ss.newLine() << "// Add a Log processing";
            ss.newLine() << "";

            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                // Lin To Log
                // k * log(mx+b, base) + kb
                
                // We account for the change of base by rolling the multiplier
                // in with 'k'
                
                const float knew[3] = { m_k[0] / logf(m_base[0]),
                                        m_k[1] / logf(m_base[1]),
                                        m_k[2] / logf(m_base[2]) };
                
                float clampMin = FLTMIN;
                
                // TODO: Switch to f32 for internal Cg processing?
                if(shaderDesc->getLanguage() == GPU_LANGUAGE_CG)
                {
                    clampMin = static_cast<float>(HALF_NRM_MIN);
                }
                
                // Decompose into 2 steps
                // 1) clamp(mx+b)
                // 2) knew * log(x) + kb
                
                ss.newLine() << shaderDesc->getPixelName() << ".rgb = "
                             << "max(" << ss.vec3fConst(clampMin) << ", "
                             << ss.vec3fConst(m_m[0], m_m[1], m_m[2]) << " * "
                             << shaderDesc->getPixelName() << ".rgb + "
                             << ss.vec3fConst(m_b[0], m_b[1], m_b[2]) << ");";
                
                ss.newLine() << shaderDesc->getPixelName() << ".rgb = "
                             << ss.vec3fConst(knew[0], knew[1], knew[2]) << " * "
                             << "log(" << shaderDesc->getPixelName() << ".rgb) + "
                             << ss.vec3fConst(m_kb[0], m_kb[1], m_kb[2]) << ";";
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                const float kinv[3] = { 1.0f / m_k[0],
                                        1.0f / m_k[1],
                                        1.0f / m_k[2] };
                
                const float minv[3] = { 1.0f / m_m[0],
                                        1.0f / m_m[1],
                                        1.0f / m_m[2] };

                // Decompose into 3 steps
                // 1) kinv * ( x - kb)
                // 2) pow(base, x)
                // 3) minv * (x - b)

                ss.newLine() << shaderDesc->getPixelName() << ".rgb = "
                             << ss.vec3fConst(kinv[0], kinv[1], kinv[2]) << " * ("
                             << shaderDesc->getPixelName() << ".rgb - "
                             << ss.vec3fConst(m_kb[0], m_kb[1], m_kb[2]) << ");";
                
                ss.newLine() << shaderDesc->getPixelName() << ".rgb = pow("
                             << ss.vec3fConst(m_base[0], m_base[1], m_base[2]) << ", "
                             << shaderDesc->getPixelName() << ".rgb);";
                
                ss.newLine() << shaderDesc->getPixelName() << ".rgb = "
                             << ss.vec3fConst(minv[0], minv[1], minv[2]) << " * ("
                             << shaderDesc->getPixelName() << ".rgb - "
                             << ss.vec3fConst(m_b[0], m_b[1], m_b[2]) << ");";
            }

            shaderDesc->addToFunctionShaderCode(ss.string().c_str());
        }
        
    }  // Anon namespace
    
    ///////////////////////////////////////////////////////////////////////////
    
    void CreateLogOp(OpRcPtrVec & ops,
                     const float * k,
                     const float * m,
                     const float * b,
                     const float * base,
                     const float * kb,
                     TransformDirection direction)
    {
        ops.push_back( LogOpRcPtr(new LogOp(k, m, b, base, kb, direction)) );
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OIIO_ADD_TEST(LogOps, LinToLog)
{
    const float k[3] = { 0.18f, 0.18f, 0.18f };
    const float m[3] = { 2.0f, 2.0f, 2.0f };
    const float b[3] = { 0.1f, 0.1f, 0.1f };
    const float base[3] = { 10.0f, 10.0f, 10.0f };
    const float kb[3] = { 1.0f, 1.0f, 1.0f };
    
    
    float data[8] = { 0.01f, 0.1f, 1.0f, 1.0f,
                      10.0f, 100.0f, 1000.0f, 1.0f, };
    
    const float result[8] = { 0.8342526242885725f,
                        0.90588182584953925f,
                        1.057999473052105462f,
                        1.0f,
                        1.23457529033568797f,
                        1.41422447595451795f,
                        1.59418930777214063f,
                        1.0f };
    
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_FORWARD));

    // one operator has been created
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_NE(ops[0].get(), NULL);

    // no chache ID before operator has been finalized
    std::string opCache = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(opCache.size(), 0);

    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));

    // validate properties
    opCache = ops[0]->getCacheID();
    OIIO_CHECK_NE(opCache.size(), 0);
    OIIO_CHECK_EQUAL(ops[0]->isNoOp(), false);
    OIIO_CHECK_EQUAL(ops[0]->hasChannelCrosstalk(), false);
    
    // Apply the result
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 2);
    }
    
    for(int i=0; i<8; ++i)
    {
        OIIO_CHECK_CLOSE( data[i], result[i], 1.0e-3 );
    }
}

OIIO_ADD_TEST(LogOps, LogToLin)
{
    const float k[3] = { 0.18f, 0.18f, 0.18f };
    const float m[3] = { 2.0f, 2.0f, 2.0f };
    const float b[3] = { 0.1f, 0.1f, 0.1f };
    const float base[3] = { 10.0f, 10.0f, 10.0f };
    const float kb[3] = { 1.0f, 1.0f, 1.0f };
    
    float data[8] = { 0.8342526242885725f,
                      0.90588182584953925f,
                      1.057999473052105462f,
                      1.0f,
                      1.23457529033568797f,
                      1.41422447595451795f,
                      1.59418930777214063f,
                      1.0f };
    
    const float result[8] = { 0.01f, 0.1f, 1.0f, 1.0f,
                        10.0f, 100.0f, 1000.0f, 1.0f, };
    
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_INVERSE));
    
    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));
    
    // Apply the result
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 2);
    }
    
    for(int i=0; i<8; ++i)
    {
        OIIO_CHECK_CLOSE( data[i], result[i], 1.0e-3 );
    }
}

OIIO_ADD_TEST(LogOps, Inverse)
{
    const float k[3] = { 0.18f, 0.5f, 0.3f };
    const float m[3] = { 2.0f, 4.0f, 8.0f };
    const float b[3] = { 0.1f, 0.1f, 0.1f };
    float base[3] = { 10.0f, 5.0f, 2.0f };
    const float kb[3] = { 1.0f, 1.0f, 1.0f };
    
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_FORWARD));
    
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_INVERSE));
    
    base[0] += 1e-5f;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_FORWARD));
    
    OIIO_CHECK_EQUAL(ops.size(), 4);
    
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[1]));
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[2]));
    OIIO_CHECK_ASSERT(ops[0]->isSameType(ops[3]->clone()));
    
    OIIO_CHECK_EQUAL(ops[0]->isInverse(ops[0]), false);
    OIIO_CHECK_EQUAL(ops[0]->isInverse(ops[1]), true);
    OIIO_CHECK_EQUAL(ops[0]->isInverse(ops[2]), false);
    OIIO_CHECK_EQUAL(ops[0]->isInverse(ops[3]), false);
    
    OIIO_CHECK_EQUAL(ops[1]->isInverse(ops[0]), true);
    OIIO_CHECK_EQUAL(ops[1]->isInverse(ops[2]), false);
    OIIO_CHECK_EQUAL(ops[1]->isInverse(ops[3]), false);
    
    OIIO_CHECK_EQUAL(ops[2]->isInverse(ops[2]), false);
    OIIO_CHECK_EQUAL(ops[2]->isInverse(ops[3]), true);
    
    OIIO_CHECK_EQUAL(ops[3]->isInverse(ops[3]), false);

    const float result[12] = { 0.01f, 0.1f, 1.0f, 1.0f,
                        1.0f, 10.0f, 100.0f, 1.0f,
                        1000.0f, 1.0f, 0.5f, 1.0f
                      };
    float data[12];

    for(int i=0; i<12; ++i)
    {
        data[i] = result[i];
    }
     
    ops[0]->apply(data, 3);
    // Note: Skip testing alpha channels.
    OIIO_CHECK_NE( data[0], result[0] );
    OIIO_CHECK_NE( data[1], result[1] );
    OIIO_CHECK_NE( data[2], result[2] );
    OIIO_CHECK_NE( data[4], result[4] );
    OIIO_CHECK_NE( data[5], result[5] );
    OIIO_CHECK_NE( data[6], result[6] );
    OIIO_CHECK_NE( data[8], result[8] );
    OIIO_CHECK_NE( data[9], result[9] );
    OIIO_CHECK_NE( data[10], result[10] );

    ops[1]->apply(data, 3);
    for(int i=0; i<12; ++i)
    {
        OIIO_CHECK_CLOSE( data[i], result[i], 1.0e-3 );
    }
}

OIIO_ADD_TEST(LogOps, CacheID)
{
    const float k[3] = { 0.18f, 0.18f, 0.18f };
    const float m[3] = { 2.0f, 2.0f, 2.0f };
    const float b[3] = { 0.1f, 0.1f, 0.1f };
    const float base[3] = { 10.0f, 10.0f, 10.0f };
    float kb[3] = { 1.0f, 1.0f, 1.0f };

    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_FORWARD));
    kb[0] += 1.0f;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_FORWARD));
    kb[0] -= 1.0f;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_FORWARD));

    // 3 operators have been created
    OIIO_CHECK_EQUAL(ops.size(), 3);
    OIIO_CHECK_NE(ops[0].get(), NULL);
    OIIO_CHECK_NE(ops[1].get(), NULL);
    OIIO_CHECK_NE(ops[2].get(), NULL);

    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));

    const std::string opCacheID0 = ops[0]->getCacheID();
    const std::string opCacheID1 = ops[1]->getCacheID();
    const std::string opCacheID2 = ops[2]->getCacheID();
    OIIO_CHECK_EQUAL(opCacheID0, opCacheID2);
    OIIO_CHECK_NE(opCacheID0, opCacheID1);
}

OIIO_ADD_TEST(LogOps, ThrowDirection)
{
    const float k[3] = { 0.18f, 0.18f, 0.18f };
    const float m[3] = { 2.0f, 2.0f, 2.0f };
    const float b[3] = { 0.1f, 0.1f, 0.1f };
    const float base[3] = { 10.0f, 10.0f, 10.0f };
    const float kb[3] = { 1.0f, 1.0f, 1.0f };

    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_THROW_WHAT(
        CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "unspecified transform direction");
}

OIIO_ADD_TEST(LogOps, ThrowBase)
{
    const float k[3] = { 0.18f, 0.18f, 0.18f };
    const float m[3] = { 2.0f, 2.0f, 2.0f };
    const float b[3] = { 0.1f, 0.1f, 0.1f };
    const float base0[3] = { 1.0f, 10.0f, 10.0f };
    const float base1[3] = { 10.0f, 1.0f, 10.0f };
    const float base2[3] = { 10.0f, 10.0f, 1.0f };
    const float kb[3] = { 1.0f, 1.0f, 1.0f };

    // Can't use base 1 for forward log transform
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base0, kb, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_THROW_WHAT(FinalizeOpVec(ops),
        OCIO::Exception, "base cannot be 1");

    ops.clear();
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base1, kb, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_THROW_WHAT(FinalizeOpVec(ops),
        OCIO::Exception, "base cannot be 1");

    ops.clear();
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base2, kb, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_THROW_WHAT(FinalizeOpVec(ops),
        OCIO::Exception, "base cannot be 1");

    // Base 1 is valid for inverse log transform
    ops.clear();
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base0, kb, OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));

    const std::string opCacheID = ops[0]->getCacheID();
    OIIO_CHECK_NE(opCacheID.size(), 0);
}

OIIO_ADD_TEST(LogOps, ThrowSlope)
{
    const float k[3] = { 0.18f, 0.18f, 0.18f };
    const float m0[3] = { 0.0f, 2.0f, 2.0f };
    const float m1[3] = { 2.0f, 0.0f, 2.0f };
    const float m2[3] = { 2.0f, 2.0f, 0.0f };
    const float b[3] = { 0.1f, 0.1f, 0.1f };
    const float base[3] = { 10.0f, 10.0f, 10.0f };
    const float kb[3] = { 1.0f, 1.0f, 1.0f };

    // Can't use slope 0 for inverse log transform
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m0, b, base, kb, OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_THROW_WHAT(FinalizeOpVec(ops),
        OCIO::Exception, "m (slope) cannot be 0");
    ops.clear();

    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m1, b, base, kb, OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_THROW_WHAT(FinalizeOpVec(ops),
        OCIO::Exception, "m (slope) cannot be 0");
    ops.clear();

    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m2, b, base, kb, OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_THROW_WHAT(FinalizeOpVec(ops),
        OCIO::Exception, "m (slope) cannot be 0");
    ops.clear();

    // Slope 0 is valid for forward log transform
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m0, b, base, kb, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));

    const std::string opCacheID = ops[0]->getCacheID();
    OIIO_CHECK_NE(opCacheID.size(), 0);
}

OIIO_ADD_TEST(LogOps, ThrowMultiplier)
{
    const float k0[3] = { 0.0f, 0.18f, 0.18f };
    const float k1[3] = { 0.18f, 0.0f, 0.18f };
    const float k2[3] = { 0.18f, 0.18f, 0.0f };
    const float m[3] = { 2.0f, 2.0f, 2.0f };
    const float b[3] = { 0.1f, 0.1f, 0.1f };
    const float base[3] = { 10.0f, 10.0f, 10.0f };
    const float kb[3] = { 1.0f, 1.0f, 1.0f };

    // Can't use multiplier 0 for inverse log transform
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k0, m, b, base, kb, OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_THROW_WHAT(FinalizeOpVec(ops),
        OCIO::Exception, "k (multiplier) cannot be 0");
    ops.clear();

    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k1, m, b, base, kb, OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_THROW_WHAT(FinalizeOpVec(ops),
        OCIO::Exception, "k (multiplier) cannot be 0");
    ops.clear();

    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k2, m, b, base, kb, OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_THROW_WHAT(FinalizeOpVec(ops),
        OCIO::Exception, "k (multiplier) cannot be 0");
    ops.clear();

    // Multiplier 0 is valid for forward log transform
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, k0, m, b, base, kb, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));

    const std::string opCacheID = ops[0]->getCacheID();
    OIIO_CHECK_NE(opCacheID.size(), 0);
}

#endif // OCIO_UNIT_TEST
