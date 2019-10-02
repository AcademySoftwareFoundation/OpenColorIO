// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
            LogOp() = delete;
            LogOp(const LogOp &) = delete;
            explicit LogOp(LogOpDataRcPtr & log);

            virtual ~LogOp();
            
            TransformDirection getDirection() const noexcept override { return logData()->getDirection(); }

            OpRcPtr clone() const override;
            
            std::string getInfo() const override;
            
            bool isSameType(ConstOpRcPtr & op) const override;
            bool isInverse(ConstOpRcPtr & op) const override;
            void finalize(FinalizationFlags fFlags) override;

            ConstOpCPURcPtr getCPUOp() const override;
            
            void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;
            
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
        
        void LogOp::finalize(FinalizationFlags /*fFlags*/)
        {
            logData()->finalize();

            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<LogOp ";
            cacheIDStream << logData()->getCacheID() << " ";
            cacheIDStream << ">";
            
            m_cacheID = cacheIDStream.str();
        }
        
        ConstOpCPURcPtr LogOp::getCPUOp() const
        {
            ConstLogOpDataRcPtr data = logData();
            return GetLogRenderer(data);
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


    ///////////////////////////////////////////////////////////////////////////

    void CreateLogTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
    {
        auto log = DynamicPtrCast<const LogOp>(op);
        if (!log)
        {
            throw Exception("CreateRangeTransform: op has to be a RangeOp");
        }
        auto logTransform = LogAffineTransform::Create();

        auto logData = DynamicPtrCast<const LogOpData>(op->data());
        logTransform->setDirection(logData->getDirection());

        auto & formatMetadata = logTransform->getFormatMetadata();
        auto & metadata = dynamic_cast<FormatMetadataImpl &>(formatMetadata);
        metadata = logData->getFormatMetadata();

        logTransform->setBase(logData->getBase());
        double logSlope[3]{ 0.0 };
        double logOffset[3]{ 0.0 };
        double linSlope[3]{ 0.0 };
        double linOffset[3]{ 0.0 };
        logData->getParameters(logSlope, logOffset, linSlope, linOffset);
        logTransform->setLogSideSlopeValue(logSlope);
        logTransform->setLogSideOffsetValue(logOffset);
        logTransform->setLinSideSlopeValue(linSlope);
        logTransform->setLinSideOffsetValue(linOffset);
        
        group->push_back(logTransform);
    }

    void BuildLogOps(OpRcPtrVec & ops,
                     const Config & /*config*/,
                     const LogAffineTransform& transform,
                     TransformDirection dir)
    {
        TransformDirection combinedDir =
            CombineTransformDirections(dir,
                                       transform.getDirection());

        double base = transform.getBase();
        double logSlope[3] = { 1.0, 1.0, 1.0 };
        double linSlope[3] = { 1.0, 1.0, 1.0 };
        double linOffset[3] = { 0.0, 0.0, 0.0 };
        double logOffset[3] = { 0.0, 0.0, 0.0 };

        transform.getLogSideSlopeValue(logSlope);
        transform.getLogSideOffsetValue(logOffset);
        transform.getLinSideSlopeValue(linSlope);
        transform.getLinSideOffsetValue(linOffset);

        auto opData = std::make_shared<LogOpData>(base, logSlope, logOffset,
                                                  linSlope, linOffset, TRANSFORM_DIR_FORWARD);

        CreateLogOp(ops, opData, combinedDir);
    }

    void BuildLogOps(OpRcPtrVec & ops,
                     const Config& /*config*/,
                     const LogTransform& transform,
                     TransformDirection dir)
    {
        TransformDirection combinedDir =
            CombineTransformDirections(dir,
                                       transform.getDirection());
        CreateLogOp(ops, transform.getBase(), combinedDir);
    }

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OCIO_ADD_TEST(LogOps, lin_to_log)
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
    OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                    linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));

    // One operator has been created.
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT((bool)ops[0]);

    // No chacheID before operator has been finalized.
    std::string opCache = ops[0]->getCacheID();
    OCIO_CHECK_EQUAL(opCache.size(), 0);

    OCIO_CHECK_NO_THROW(OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));

    // Validate properties.
    opCache = ops[0]->getCacheID();
    OCIO_CHECK_NE(opCache.size(), 0);
    OCIO_CHECK_EQUAL(ops[0]->isNoOp(), false);
    OCIO_CHECK_EQUAL(ops[0]->hasChannelCrosstalk(), false);
    
    // Apply the result.
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 2);
    }
    
    for(int i=0; i<8; ++i)
    {
        OCIO_CHECK_CLOSE( data[i], result[i], 1.0e-3 );
    }
}

OCIO_ADD_TEST(LogOps, log_to_lin)
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
    OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                    linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_INVERSE));
    
    OCIO_CHECK_NO_THROW(OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
    
    // Apply the result.
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 2);
    }
    
    for(int i=0; i<8; ++i)
    {
        OCIO_CHECK_CLOSE( data[i], result[i], 2.0e-3f );
    }
}

OCIO_ADD_TEST(LogOps, inverse)
{
    double base = 10.0;
    const double logSlope[3] = { 0.5, 0.5, 0.5 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    const double logOffset[3] = { 1.0, 1.0, 1.0 };
    const double logSlope2[3] = { 0.5, 1.0, 1.5 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));
    
    OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_INVERSE));
    
    base += 1.0;
    OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope2, logOffset, linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope2, logOffset, linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 6);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];
    OCIO::ConstOpRcPtr op3 = ops[3];
    OCIO::ConstOpRcPtr op4 = ops[4];
    OCIO::ConstOpRcPtr op5 = ops[5];

    OCIO_CHECK_ASSERT(ops[0]->isSameType(op1));
    OCIO_CHECK_ASSERT(ops[0]->isSameType(op2));
    OCIO::ConstOpRcPtr op3Cloned = ops[3]->clone();
    OCIO_CHECK_ASSERT(ops[0]->isSameType(op3Cloned));
    
    OCIO_CHECK_EQUAL(ops[0]->isInverse(op0), false);
    OCIO_CHECK_EQUAL(ops[0]->isInverse(op1), true);
    OCIO_CHECK_EQUAL(ops[0]->isInverse(op2), false);
    OCIO_CHECK_EQUAL(ops[0]->isInverse(op3), false);
    
    OCIO_CHECK_EQUAL(ops[1]->isInverse(op0), true);
    OCIO_CHECK_EQUAL(ops[1]->isInverse(op2), false);
    OCIO_CHECK_EQUAL(ops[1]->isInverse(op3), false);
    
    OCIO_CHECK_EQUAL(ops[2]->isInverse(op2), false);
    OCIO_CHECK_EQUAL(ops[2]->isInverse(op3), true);
    
    OCIO_CHECK_EQUAL(ops[3]->isInverse(op3), false);

    // When r, g & b are not equal, ops are not considered inverse 
    // even though they are.
    OCIO_CHECK_EQUAL(ops[4]->isInverse(op5), false);

    const float result[12] = { 0.01f, 0.1f, 1.0f, 1.0f,
                               1.0f, 10.0f, 100.0f, 1.0f,
                               1000.0f, 1.0f, 0.5f, 1.0f };
    float data[12];

    for(int i=0; i<12; ++i)
    {
        data[i] = result[i];
    }
    
    ops[0]->finalize(OCIO::FINALIZATION_EXACT);
    ops[0]->apply(data, 3);
    // Note: Skip testing alpha channels.
    OCIO_CHECK_NE( data[0], result[0] );
    OCIO_CHECK_NE( data[1], result[1] );
    OCIO_CHECK_NE( data[2], result[2] );
    OCIO_CHECK_NE( data[4], result[4] );
    OCIO_CHECK_NE( data[5], result[5] );
    OCIO_CHECK_NE( data[6], result[6] );
    OCIO_CHECK_NE( data[8], result[8] );
    OCIO_CHECK_NE( data[9], result[9] );
    OCIO_CHECK_NE( data[10], result[10] );

    ops[1]->finalize(OCIO::FINALIZATION_EXACT);
    ops[1]->apply(data, 3);

#ifndef USE_SSE
    const float error = 1e-3f;
#else
    const float error = 1e-2f;
#endif // !USE_SSE

    for(int i=0; i<12; ++i)
    {
        OCIO_CHECK_CLOSE( data[i], result[i], error);
    }
}

OCIO_ADD_TEST(LogOps, cache_id)
{
    const double base = 10.0;
    const double logSlope[3] = { 0.18, 0.18, 0.18 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    double logOffset[3] = { 1.0, 1.0, 1.0 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                    linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));
    logOffset[0] += 1.0f;
    OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                    linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));
    logOffset[0] -= 1.0f;
    OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                    linSlope, linOffset,
                                    OCIO::TRANSFORM_DIR_FORWARD));

    // 3 operators have been created
    OCIO_CHECK_EQUAL(ops.size(), 3);
    OCIO_REQUIRE_ASSERT((bool)ops[0]);
    OCIO_REQUIRE_ASSERT((bool)ops[1]);
    OCIO_REQUIRE_ASSERT((bool)ops[2]);

    OCIO_CHECK_NO_THROW(OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));

    const std::string opCacheID0 = ops[0]->getCacheID();
    const std::string opCacheID1 = ops[1]->getCacheID();
    const std::string opCacheID2 = ops[2]->getCacheID();
    OCIO_CHECK_EQUAL(opCacheID0, opCacheID2);
    OCIO_CHECK_NE(opCacheID0, opCacheID1);
}

OCIO_ADD_TEST(LogOps, throw_direction)
{
    const double base = 10.0;
    const double logSlope[3] = { 0.18, 0.18, 0.18 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    const double logOffset[3] = { 1.0, 1.0, 1.0 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_THROW_WHAT(
        CreateLogOp(ops, base, logSlope, logOffset,
                    linSlope, linOffset,
                    OCIO::TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "unspecified transform direction");
}

OCIO_ADD_TEST(LogOps, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;

    const double base = 1.0;
    const double logSlope[] = { 1.5, 1.6, 1.7 };
    const double linSlope[] = { 1.1, 1.2, 1.3 };
    const double linOffset[] = { 1.0, 2.0, 3.0 };
    const double logOffset[] = { 10.0, 20.0, 30.0 };

    OCIO::LogOpDataRcPtr log
        = std::make_shared<OCIO::LogOpData>(base, logSlope, logOffset, linSlope, linOffset, direction);

    auto & metadataSource = log->getFormatMetadata();
    metadataSource.addAttribute("name", "test");

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateLogOp(ops, log, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateLogTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->size(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto lTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::LogAffineTransform>(transform);
    OCIO_REQUIRE_ASSERT(lTransform);

    const auto & metadata = lTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeName(0)), "name");
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeValue(0)), "test");

    OCIO_CHECK_EQUAL(lTransform->getDirection(), direction);
    OCIO_CHECK_EQUAL(lTransform->getBase(), base);
    double values[3]{ 0.0 };
    lTransform->getLogSideSlopeValue(values);
    OCIO_CHECK_EQUAL(values[0], logSlope[0]);
    OCIO_CHECK_EQUAL(values[1], logSlope[1]);
    OCIO_CHECK_EQUAL(values[2], logSlope[2]);
    lTransform->getLogSideOffsetValue(values);
    OCIO_CHECK_EQUAL(values[0], logOffset[0]);
    OCIO_CHECK_EQUAL(values[1], logOffset[1]);
    OCIO_CHECK_EQUAL(values[2], logOffset[2]);
    lTransform->getLinSideSlopeValue(values);
    OCIO_CHECK_EQUAL(values[0], linSlope[0]);
    OCIO_CHECK_EQUAL(values[1], linSlope[1]);
    OCIO_CHECK_EQUAL(values[2], linSlope[2]);
    lTransform->getLinSideOffsetValue(values);
    OCIO_CHECK_EQUAL(values[0], linOffset[0]);
    OCIO_CHECK_EQUAL(values[1], linOffset[1]);
    OCIO_CHECK_EQUAL(values[2], linOffset[2]);
}

#endif // OCIO_UNIT_TEST
