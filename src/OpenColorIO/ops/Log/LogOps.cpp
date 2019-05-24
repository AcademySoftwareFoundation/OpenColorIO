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
#include "ops/Log/LogOpCPU.h"
#include "ops/Log/LogOpData.h"
#include "ops/Log/LogOpGPU.h"
#include "ops/Log/LogOps.h"
#include "MathUtils.h"


OCIO_NAMESPACE_ENTER
{
    namespace
    {
        class LogOp: public Op
        {
        public:

            LogOp(LogOpDataRcPtr & log);
            LogOp() = delete;

            virtual ~LogOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            
            virtual bool isSameType(ConstOpRcPtr & op) const;
            virtual bool isInverse(ConstOpRcPtr & op) const;
            virtual void finalize();
            
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;
            
        protected:
            ConstLogOpDataRcPtr logData() const { return DynamicPtrCast<const LogOpData>(data()); }
            LogOpDataRcPtr logData() { return DynamicPtrCast<LogOpData>(data()); }

        private:
            OpCPURcPtr m_cpu;
        };
        
        typedef OCIO_SHARED_PTR<LogOp> LogOpRcPtr;
        typedef OCIO_SHARED_PTR<const LogOp> ConstLogOpRcPtr;

        LogOp::LogOp(LogOpDataRcPtr & log)
            : Op()
        {
            data() = log;
        }
        
        OpRcPtr LogOp::clone() const
        {
            auto opData = logData()->clone();
            return std::make_shared<LogOp>(opData);
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

            ConstLogOpDataRcPtr logOpData = typedRcPtr->logData();
            return logData()->isInverse(logOpData);
        }
        
        void LogOp::finalize()
        {
            const LogOp & constThis = *this;

            // Only the 32f processing is natively supported
            logData()->setInputBitDepth(BIT_DEPTH_F32);
            logData()->setOutputBitDepth(BIT_DEPTH_F32);

            logData()->validate();
            logData()->finalize();

            ConstLogOpDataRcPtr logOpData = constThis.logData();
            m_cpuOp = GetLogRenderer(logOpData);

            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<LogOp ";
            cacheIDStream << logData()->getCacheID() << " ";
            cacheIDStream << ">";
            
            m_cacheID = cacheIDStream.str();
        }
        
        void LogOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            if (getInputBitDepth()!=BIT_DEPTH_F32 
                || getOutputBitDepth()!=BIT_DEPTH_F32)
            {
                throw Exception("Only 32F bit depth is supported for the GPU shader");
            }

            ConstLogOpDataRcPtr data = logData();
            GetLogGPUShaderProgram(shaderDesc, data);
        }
        
    }  // Anon namespace
    
    ///////////////////////////////////////////////////////////////////////////

    void CreateLogOp(OpRcPtrVec & ops,
                     double base,
                     const double(&logSlope)[3],
                     const double(&logOffset)[3],
                     const double(&linSlope)[3],
                     const double(&linOffset)[3],
                     TransformDirection direction)
    {
        auto opData = std::make_shared<LogOpData>(base, logSlope, logOffset,
                                                  linSlope, linOffset, direction);
        ops.push_back(std::make_shared<LogOp>(opData));
    }

    void CreateLogOp(OpRcPtrVec & ops, double base, TransformDirection direction)
    {
        auto opData = std::make_shared<LogOpData>(base, direction);
        ops.push_back(std::make_shared<LogOp>(opData));
    }

    void CreateLogOp(OpRcPtrVec & ops,
                     LogOpDataRcPtr & logData,
                     TransformDirection direction)
    {
        if (direction == TRANSFORM_DIR_UNKNOWN)
        {
            throw Exception("Cannot create Log op, unspecified transform direction.");
        }

        auto log = logData;
        if (direction == TRANSFORM_DIR_INVERSE)
        {
            log = log->inverse();
        }

        ops.push_back(std::make_shared<LogOp>(log));
    }

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(LogOps, lin_to_log)
{
    const double base = 10.0;
    const double logSlope[3] = { 0.18, 0.18, 0.18 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    const double logOffset[3] = { 1.0, 1.0, 1.0 };
    
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
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                    linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));

    // One operator has been created.
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_REQUIRE_ASSERT((bool)ops[0]);

    // No chache ID before operator has been finalized.
    std::string opCache = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(opCache.size(), 0);

    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));

    // Validate properties.
    opCache = ops[0]->getCacheID();
    OIIO_CHECK_NE(opCache.size(), 0);
    OIIO_CHECK_EQUAL(ops[0]->isNoOp(), false);
    OIIO_CHECK_EQUAL(ops[0]->hasChannelCrosstalk(), false);
    
    // Apply the result.
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 2);
    }
    
    for(int i=0; i<8; ++i)
    {
        OIIO_CHECK_CLOSE( data[i], result[i], 1.0e-3 );
    }
}

OIIO_ADD_TEST(LogOps, log_to_lin)
{
    const double base = 10.0;
    const double logSlope[3] = { 0.18, 0.18, 0.18 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    const double logOffset[3] = { 1.0, 1.0, 1.0 };
    
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
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                    linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_INVERSE));
    
    OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));
    
    // Apply the result.
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 2);
    }
    
    for(int i=0; i<8; ++i)
    {
        OIIO_CHECK_CLOSE( data[i], result[i], 2.0e-3f );
    }
}

OIIO_ADD_TEST(LogOps, inverse)
{
    double base = 10.0;
    const double logSlope[3] = { 0.5, 0.5, 0.5 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    const double logOffset[3] = { 1.0, 1.0, 1.0 };
    const double logSlope2[3] = { 0.5, 1.0, 1.5 };

    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));
    
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_INVERSE));
    
    base += 1.0;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));

    OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope2, logOffset, linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope2, logOffset, linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));

    OIIO_REQUIRE_EQUAL(ops.size(), 6);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];
    OCIO::ConstOpRcPtr op3 = ops[3];
    OCIO::ConstOpRcPtr op4 = ops[4];
    OCIO::ConstOpRcPtr op5 = ops[5];

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

    // When r, g & b are not equal, ops are not considered inverse 
    // even though they are.
    OIIO_CHECK_EQUAL(ops[4]->isInverse(op5), false);

    const float result[12] = { 0.01f, 0.1f, 1.0f, 1.0f,
                               1.0f, 10.0f, 100.0f, 1.0f,
                               1000.0f, 1.0f, 0.5f, 1.0f };
    float data[12];

    for(int i=0; i<12; ++i)
    {
        data[i] = result[i];
    }
    
    ops[0]->finalize();
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

    ops[1]->finalize();
    ops[1]->apply(data, 3);

#ifndef USE_SSE
    const float error = 1e-3f;
#else
    const float error = 1e-2f;
#endif // !USE_SSE

    for(int i=0; i<12; ++i)
    {
        OIIO_CHECK_CLOSE( data[i], result[i], error);
    }
}

OIIO_ADD_TEST(LogOps, cache_id)
{
    const double base = 10.0;
    const double logSlope[3] = { 0.18, 0.18, 0.18 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    double logOffset[3] = { 1.0, 1.0, 1.0 };

    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                    linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));
    logOffset[0] += 1.0f;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                    linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));
    logOffset[0] -= 1.0f;
    OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                    linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));

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

OIIO_ADD_TEST(LogOps, throw_direction)
{
    const double base = 10.0;
    const double logSlope[3] = { 0.18, 0.18, 0.18 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    const double logOffset[3] = { 1.0, 1.0, 1.0 };

    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_THROW_WHAT(
        CreateLogOp(ops, base, logSlope, logOffset,
                    linSlope, linOffset,
                    OCIO::TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "unspecified transform direction");
}

#endif // OCIO_UNIT_TEST
