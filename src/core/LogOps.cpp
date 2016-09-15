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
#include <cstring>
#include <iostream>

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
            
            float knew[3] = { k[0] / logf(base[0]),
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
            float kinv[3] = { 1.0f / k[0],
                               1.0f / k[1],
                               1.0f / k[2] };
            
            float minv[3] = { 1.0f / m[0],
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
                throw Exception("Cannot apply LogOp op, unspecified transform direction.");
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
        
        bool LogOp::isNoOp() const
        {
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
        
        bool LogOp::supportsGpuShader() const
        {
            return true;
        }
        
        void LogOp::writeGpuShader(std::ostream & shader,
                                   const std::string & pixelName,
                                   const GpuShaderDesc & shaderDesc) const
        {
            GpuLanguage lang = shaderDesc.getLanguage();
            
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                // Lin To Log
                // k * log(mx+b, base) + kb
                
                // We account for the change of base by rolling the multiplier
                // in with 'k'
                
                float knew[3] = { m_k[0] / logf(m_base[0]),
                                  m_k[1] / logf(m_base[1]),
                                  m_k[2] / logf(m_base[2]) };
                
                float clampMin[3] = { FLTMIN, FLTMIN, FLTMIN };
                
                // TODO: Switch to f32 for internal Cg processing?
                if(lang == GPU_LANGUAGE_CG)
                {
                    clampMin[0] = static_cast<float>(GetHalfNormMin());
                    clampMin[1] = static_cast<float>(GetHalfNormMin());
                    clampMin[2] = static_cast<float>(GetHalfNormMin());
                }
                
                // Decompose into 2 steps
                // 1) clamp(mx+b)
                // 2) knew * log(x) + kb
                
                shader << pixelName << ".rgb = ";
                shader << "max(" << GpuTextHalf3(clampMin,lang) << ", ";
                shader << GpuTextHalf3(m_m,lang) << " * ";
                shader << pixelName << ".rgb + ";
                shader << GpuTextHalf3(m_b,lang) << ");\n";
                
                shader << pixelName << ".rgb = ";
                shader << GpuTextHalf3(knew,lang) << " * ";
                shader << "log(" << pixelName << ".rgb) + ";
                shader << GpuTextHalf3(m_kb,lang) << ";\n";
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                float kinv[3] = { 1.0f / m_k[0],
                                  1.0f / m_k[1],
                                  1.0f / m_k[2] };
                
                float minv[3] = { 1.0f / m_m[0],
                                  1.0f / m_m[1],
                                  1.0f / m_m[2] };
                
                // Decompose into 3 steps
                // 1) kinv * ( x - kb)
                // 2) pow(base, x)
                // 3) minv * (x - b)
                
                shader << pixelName << ".rgb = ";
                shader << GpuTextHalf3(kinv,lang) << " * (";
                shader << pixelName << ".rgb - ";
                shader << GpuTextHalf3(m_kb,lang) << ");\n";
                
                shader << pixelName << ".rgb = pow(";
                shader << GpuTextHalf3(m_base,lang) << ", ";
                shader << pixelName << ".rgb);\n";
                
                shader << pixelName << ".rgb = ";
                shader << GpuTextHalf3(minv,lang) << " * (";
                shader << pixelName << ".rgb - ";
                shader << GpuTextHalf3(m_b,lang) << ");\n";
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
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OIIO_ADD_TEST(LogOps, LinToLog)
{
    float k[3] = { 0.18f, 0.18f, 0.18f };
    float m[3] = { 2.0f, 2.0f, 2.0f };
    float b[3] = { 0.1f, 0.1f, 0.1f };
    float base[3] = { 10.0f, 10.0f, 10.0f };
    float kb[3] = { 1.0f, 1.0f, 1.0f };
    
    
    float data[8] = { 0.01f, 0.1f, 1.0f, 1.0f,
                      10.0f, 100.0f, 1000.0f, 1.0f, };
    
    float result[8] = { 0.8342526242885725f,
                        0.90588182584953925f,
                        1.057999473052105462f,
                        1.0f,
                        1.23457529033568797f,
                        1.41422447595451795f,
                        1.59418930777214063f,
                        1.0f };
    
    OCIO::OpRcPtrVec ops;
    CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_FORWARD);
    
    FinalizeOpVec(ops);
    
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
    float k[3] = { 0.18f, 0.18f, 0.18f };
    float m[3] = { 2.0f, 2.0f, 2.0f };
    float b[3] = { 0.1f, 0.1f, 0.1f };
    float base[3] = { 10.0f, 10.0f, 10.0f };
    float kb[3] = { 1.0f, 1.0f, 1.0f };
    
    float data[8] = { 0.8342526242885725f,
                      0.90588182584953925f,
                      1.057999473052105462f,
                      1.0f,
                      1.23457529033568797f,
                      1.41422447595451795f,
                      1.59418930777214063f,
                      1.0f };
    
    float result[8] = { 0.01f, 0.1f, 1.0f, 1.0f,
                        10.0f, 100.0f, 1000.0f, 1.0f, };
    
    OCIO::OpRcPtrVec ops;
    CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_INVERSE);
    
    FinalizeOpVec(ops);
    
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
    float k[3] = { 0.18f, 0.5f, 0.3f };
    float m[3] = { 2.0f, 4.0f, 8.0f };
    float b[3] = { 0.1f, 0.1f, 0.1f };
    float base[3] = { 10.0f, 5.0f, 2.0f };
    float kb[3] = { 1.0f, 1.0f, 1.0f };
    
    OCIO::OpRcPtrVec ops;
    CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_FORWARD);
    
    CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_INVERSE);
    
    base[0] += 1e-5f;
    CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_INVERSE);
    CreateLogOp(ops, k, m, b, base, kb, OCIO::TRANSFORM_DIR_FORWARD);
    
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

    float result[12] = { 0.01f, 0.1f, 1.0f, 1.0f,
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

#endif // OCIO_UNIT_TEST
