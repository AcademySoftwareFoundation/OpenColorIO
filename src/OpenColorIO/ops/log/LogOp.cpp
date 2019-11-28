// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cmath>
#include <cstring>
#include <iostream>
#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "HashUtils.h"
#include "GpuShaderUtils.h"
#include "ops/log/LogOpCPU.h"
#include "ops/log/LogOpData.h"
#include "ops/log/LogOpGPU.h"
#include "ops/log/LogOp.h"
#include "MathUtils.h"

namespace OCIO_NAMESPACE
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

    TransformDirection getDirection() const noexcept { return logData()->getDirection(); }

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    void finalize(OptimizationFlags oFlags) override;

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

void LogOp::finalize(OptimizationFlags /*oFlags*/)
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

    group->appendTransform(logTransform);
}

void BuildLogOp(OpRcPtrVec & ops,
                const Config & /*config*/,
                const LogAffineTransform & transform,
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

void BuildLogOp(OpRcPtrVec & ops,
                const Config & /*config*/,
                const LogTransform & transform,
                TransformDirection dir)
{
    TransformDirection combinedDir =
        CombineTransformDirections(dir,
                                    transform.getDirection());
    CreateLogOp(ops, transform.getBase(), combinedDir);
}

} // namespace OCIO_NAMESPACE

