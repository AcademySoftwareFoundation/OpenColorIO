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

#include "HashUtils.h"
#include "GpuShaderUtils.h"
#include "ops/Log/LogOps.h"
#include "MathUtils.h"


OCIO_NAMESPACE_ENTER
{
    namespace DefaultValues
    {
        const int FLOAT_DECIMALS = 7;

        float k[3] = { 1.0f, 1.0f, 1.0f };
        float m[3] = { 1.0f, 1.0f, 1.0f };
        float b[3] = { 0.0f, 0.0f, 0.0f };
        float kb[3] = { 0.0f, 0.0f, 0.0f };
    }

    LogOpData::LogOpData(float base)
        :   OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    {           
        memcpy(m_k,    DefaultValues::k,    sizeof(float)*3);
        memcpy(m_m,    DefaultValues::m,    sizeof(float)*3);
        memcpy(m_b,    DefaultValues::b,    sizeof(float)*3);
        memcpy(m_kb,   DefaultValues::kb,   sizeof(float)*3);

        m_base[0] = base;
        m_base[1] = base;
        m_base[2] = base;
    }

    LogOpData::LogOpData(const float * k,
                         const float * m,
                         const float * b,
                         const float * base,
                         const float * kb)
        :   OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    {           
        memcpy(m_k,    k,    sizeof(float)*3);
        memcpy(m_m,    m,    sizeof(float)*3);
        memcpy(m_b,    b,    sizeof(float)*3);
        memcpy(m_base, base, sizeof(float)*3);
        memcpy(m_kb,   kb,   sizeof(float)*3);
    }

    LogOpData & LogOpData::operator = (const LogOpData & rhs)
    {
        if(this!=&rhs)
        {
            memcpy(m_k,    rhs.m_k,    sizeof(float)*3);
            memcpy(m_m,    rhs.m_m,    sizeof(float)*3);
            memcpy(m_b,    rhs.m_b,    sizeof(float)*3);
            memcpy(m_base, rhs.m_base, sizeof(float)*3);
            memcpy(m_kb,   rhs.m_kb,   sizeof(float)*3);
        }

        return *this;
    }

    void LogOpData::finalize()
    {
        AutoMutex lock(m_mutex);

        std::ostringstream cacheIDStream;
        cacheIDStream << getID();

        cacheIDStream.precision(DefaultValues::FLOAT_DECIMALS);
        for(int i=0; i<3; ++i)
        {
            cacheIDStream << m_k[i] << " ";
            cacheIDStream << m_m[i] << " ";
            cacheIDStream << m_b[i] << " ";
            cacheIDStream << m_base[i] << " ";
            cacheIDStream << m_kb[i] << " ";
        }

        m_cacheID = cacheIDStream.str();
    }
        

    namespace
    {
        const float FLTMIN = std::numeric_limits<float>::min();
        
        // k * log(mx+b, base) + kb
        // the caller is responsible for base != 1.0
        // TODO: pull the precomputation into the caller?
        
        void ApplyLinToLog(float* rgbaBuffer, long numPixels, ConstLogOpDataRcPtr & log)
        {
            // We account for the change of base by rolling the multiplier
            // in with 'k'
            
            const float knew[3] = { log->m_k[0] / logf(log->m_base[0]),
                                    log->m_k[1] / logf(log->m_base[1]),
                                    log->m_k[2] / logf(log->m_base[2]) };
            
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                rgbaBuffer[0] = knew[0] * logf(std::max(log->m_m[0]*rgbaBuffer[0] + log->m_b[0], FLTMIN)) + log->m_kb[0];
                rgbaBuffer[1] = knew[1] * logf(std::max(log->m_m[1]*rgbaBuffer[1] + log->m_b[1], FLTMIN)) + log->m_kb[1];
                rgbaBuffer[2] = knew[2] * logf(std::max(log->m_m[2]*rgbaBuffer[2] + log->m_b[2], FLTMIN)) + log->m_kb[2];
                
                rgbaBuffer += 4;
            }
        }
        
        // the caller is responsible for m != 0
        // the caller is responsible for k != 0
        // TODO: pull the precomputation into the caller?
        
        void ApplyLogToLin(float* rgbaBuffer, long numPixels, ConstLogOpDataRcPtr & log)
        {
            const float kinv[3] = { 1.0f / log->m_k[0],
                                    1.0f / log->m_k[1],
                                    1.0f / log->m_k[2] };
            
            const float minv[3] = { 1.0f / log->m_m[0],
                                    1.0f / log->m_m[1],
                                    1.0f / log->m_m[2] };
            
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                rgbaBuffer[0] 
                    = minv[0] * (powf(log->m_base[0], kinv[0]*(rgbaBuffer[0]-log->m_kb[0])) 
                        - log->m_b[0]);
                rgbaBuffer[1] 
                    = minv[1] * (powf(log->m_base[1], kinv[1]*(rgbaBuffer[1]-log->m_kb[1]))
                        - log->m_b[1]);
                rgbaBuffer[2] 
                    = minv[2] * (powf(log->m_base[2], kinv[2]*(rgbaBuffer[2]-log->m_kb[2]))
                        - log->m_b[2]);
                
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
            LogOp(float base, TransformDirection direction);

            LogOp(const float * k,
                  const float * m,
                  const float * b,
                  const float * base,
                  const float * kb,
                  TransformDirection direction);
            virtual ~LogOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            
            virtual bool isSameType(ConstOpRcPtr & op) const;
            virtual bool isInverse(ConstOpRcPtr & op) const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;
            
        protected:
            ConstLogOpDataRcPtr logData() const { return DynamicPtrCast<const LogOpData>(data()); }
            LogOpDataRcPtr logData() { return DynamicPtrCast<LogOpData>(data()); }

        private:
            TransformDirection m_direction;
        };
        
        typedef OCIO_SHARED_PTR<LogOp> LogOpRcPtr;
        typedef OCIO_SHARED_PTR<const LogOp> ConstLogOpRcPtr;

        LogOp::LogOp(float base, TransformDirection direction):
                                       Op(),
                                       m_direction(direction)
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception("Cannot apply LogOp op, unspecified transform direction.");
            }

            data().reset(new LogOpData(base));
        }
        
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
                throw Exception("Cannot apply LogOp op, unspecified transform direction.");
            }

            data().reset(new LogOpData(k, m, b, base, kb));
        }
        
        OpRcPtr LogOp::clone() const
        {
            return std::make_shared<LogOp>(
                logData()->m_k, logData()->m_m, logData()->m_b,
                logData()->m_base, logData()->m_kb,
                m_direction);
        }
        
        LogOp::~LogOp()
        { }
        
        std::string LogOp::getInfo() const
        {
            return "<LogOp>";
        }
        
        bool LogOp::isSameType(ConstOpRcPtr & op) const
        {
            ConstLogOpRcPtr typedRcPtr = DynamicPtrCast<const LogOp>(op);
            if(!typedRcPtr) return false;
            return true;
        }
        
        bool LogOp::isInverse(ConstOpRcPtr & op) const
        {
            ConstLogOpRcPtr typedRcPtr = DynamicPtrCast<const LogOp>(op);
            if(!typedRcPtr) return false;
            
            if(GetInverseTransformDirection(m_direction) != typedRcPtr->m_direction)
                return false;
            
            float error = std::numeric_limits<float>::min();
            if(!VecsEqualWithRelError(logData()->m_k, 3, typedRcPtr->logData()->m_k, 3, error))
                return false;
            if(!VecsEqualWithRelError(logData()->m_m, 3, typedRcPtr->logData()->m_m, 3, error))
                return false;
            if(!VecsEqualWithRelError(logData()->m_b, 3, typedRcPtr->logData()->m_b, 3, error))
                return false;
            if(!VecsEqualWithRelError(logData()->m_base, 3, typedRcPtr->logData()->m_base, 3, error))
                return false;
            if(!VecsEqualWithRelError(logData()->m_kb, 3, typedRcPtr->logData()->m_kb, 3, error))
                return false;
            
            return true;
        }
        
        void LogOp::finalize()
        {
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                if(VecContainsOne(logData()->m_base, 3))
                    throw Exception("LogOp Exception, base cannot be 1.");
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                if(VecContainsZero(logData()->m_m, 3))
                    throw Exception("LogOp Exception, m (slope) cannot be 0.");
                if(VecContainsZero(logData()->m_k, 3))
                    throw Exception("LogOp Exception, k (multiplier) cannot be 0.");
            }

            logData()->finalize();
            
            std::ostringstream cacheIDStream;
            cacheIDStream << "<LogOp ";
            cacheIDStream << logData()->getCacheID() << " ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << ">";
            
            m_cacheID = cacheIDStream.str();
        }
        
        void LogOp::apply(float* rgbaBuffer, long numPixels) const
        {
            ConstLogOpDataRcPtr logOpData = logData();
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                ApplyLinToLog(rgbaBuffer, numPixels, logOpData);
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                ApplyLogToLin(rgbaBuffer, numPixels, logOpData);
            }
        } // Op::process
        
        void LogOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            if(getInputBitDepth()!=BIT_DEPTH_F32 
                || getOutputBitDepth()!=BIT_DEPTH_F32)
            {
                throw Exception("Only 32F bit depth is supported for the GPU shader");
            }

            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                // Lin To Log
                // k * log(mx+b, base) + kb
                
                // We account for the change of base by rolling the multiplier
                // in with 'k'
                
                const float knew[3] = { logData()->m_k[0] / logf(logData()->m_base[0]),
                                        logData()->m_k[1] / logf(logData()->m_base[1]),
                                        logData()->m_k[2] / logf(logData()->m_base[2]) };
                
                float clampMin = FLTMIN;
                
                // TODO: Switch to f32 for internal Cg processing?
                if(shaderDesc->getLanguage() == GPU_LANGUAGE_CG)
                {
                    clampMin = static_cast<float>(GetHalfNormMin());
                }
                
                // Decompose into 2 steps
                // 1) clamp(mx+b)
                // 2) knew * log(x) + kb
                
                GpuShaderText ss(shaderDesc->getLanguage());
                ss.indent();

                ss.newLine() << shaderDesc->getPixelName() << ".rgb = "
                             << "max(" << ss.vec3fConst(clampMin) << ", "
                             << ss.vec3fConst(logData()->m_m[0], 
                                              logData()->m_m[1], 
                                              logData()->m_m[2]) 
                             << " * "
                             << shaderDesc->getPixelName() << ".rgb + "
                             << ss.vec3fConst(logData()->m_b[0], 
                                              logData()->m_b[1], 
                                              logData()->m_b[2]) 
                             << ");";
                
                ss.newLine() << shaderDesc->getPixelName() << ".rgb = "
                             << ss.vec3fConst(knew[0], knew[1], knew[2]) << " * "
                             << "log(" << shaderDesc->getPixelName() << ".rgb) + "
                             << ss.vec3fConst(logData()->m_kb[0], 
                                              logData()->m_kb[1], 
                                              logData()->m_kb[2]) 
                             << ";";

                shaderDesc->addToFunctionShaderCode(ss.string().c_str());
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                const float kinv[3] = { 1.0f / logData()->m_k[0],
                                        1.0f / logData()->m_k[1],
                                        1.0f / logData()->m_k[2] };
                
                const float minv[3] = { 1.0f / logData()->m_m[0],
                                        1.0f / logData()->m_m[1],
                                        1.0f / logData()->m_m[2] };

                // Decompose into 3 steps
                // 1) kinv * ( x - kb)
                // 2) pow(base, x)
                // 3) minv * (x - b)

                GpuShaderText ss(shaderDesc->getLanguage());
                ss.indent();

                ss.newLine() << shaderDesc->getPixelName() << ".rgb = "
                             << ss.vec3fConst(kinv[0], kinv[1], kinv[2]) 
                             << " * ("
                             << shaderDesc->getPixelName() << ".rgb - "
                             << ss.vec3fConst(logData()->m_kb[0], 
                                              logData()->m_kb[1], 
                                              logData()->m_kb[2]) 
                             << ");";
                
                ss.newLine() << shaderDesc->getPixelName() << ".rgb = pow("
                             << ss.vec3fConst(logData()->m_base[0], 
                                              logData()->m_base[1], 
                                              logData()->m_base[2]) 
                             << ", "
                             << shaderDesc->getPixelName() << ".rgb);";
                
                ss.newLine() << shaderDesc->getPixelName() << ".rgb = "
                             << ss.vec3fConst(minv[0], minv[1], minv[2]) 
                             << " * ("
                             << shaderDesc->getPixelName() << ".rgb - "
                             << ss.vec3fConst(logData()->m_b[0], 
                                              logData()->m_b[1], 
                                              logData()->m_b[2]) 
                             << ");";

                shaderDesc->addToFunctionShaderCode(ss.string().c_str());
            }
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

    void CreateLogOp(OpRcPtrVec & ops, float base, TransformDirection direction)
    {
        ops.push_back( LogOpRcPtr(new LogOp(base, direction)) );
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

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
    OIIO_REQUIRE_ASSERT((bool)ops[0]);

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
    
    OIIO_REQUIRE_EQUAL(ops.size(), 4);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];
    OCIO::ConstOpRcPtr op3 = ops[3];

    OIIO_CHECK_ASSERT(ops[0]->isSameType(op1));
    OIIO_CHECK_ASSERT(ops[0]->isSameType(op2));
    OCIO::ConstOpRcPtr op3Cloned = ops[3]->clone();
    OIIO_CHECK_ASSERT(ops[0]->isSameType(op3Cloned));
    
    OIIO_CHECK_EQUAL(ops[0]->isInverse(op0), false);
    OIIO_CHECK_EQUAL(ops[0]->isInverse(op1), true);
    OIIO_CHECK_EQUAL(ops[0]->isInverse(op2), false);
    OIIO_CHECK_EQUAL(ops[0]->isInverse(op3), false);
    
    OIIO_CHECK_EQUAL(ops[1]->isInverse(op0), true);
    OIIO_CHECK_EQUAL(ops[1]->isInverse(op2), false);
    OIIO_CHECK_EQUAL(ops[1]->isInverse(op3), false);
    
    OIIO_CHECK_EQUAL(ops[2]->isInverse(op2), false);
    OIIO_CHECK_EQUAL(ops[2]->isInverse(op3), true);
    
    OIIO_CHECK_EQUAL(ops[3]->isInverse(op3), false);

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
    OIIO_REQUIRE_ASSERT((bool)ops[0]);
    OIIO_REQUIRE_ASSERT((bool)ops[1]);
    OIIO_REQUIRE_ASSERT((bool)ops[2]);

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
