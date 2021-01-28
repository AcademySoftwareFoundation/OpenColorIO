// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/log/LogOpData.h"
#include "transforms/LogCameraTransform.h"

namespace OCIO_NAMESPACE
{
LogCameraTransformRcPtr LogCameraTransform::Create(const double(&linSideBreakValues)[3])
{
    return LogCameraTransformRcPtr(new LogCameraTransformImpl(linSideBreakValues),
                                   &LogCameraTransformImpl::deleter);
}

void LogCameraTransformImpl::deleter(LogCameraTransform* t)
{
    delete static_cast<LogCameraTransformImpl *>(t);
}

LogCameraTransformImpl::LogCameraTransformImpl(const double(&linSideBreakValues)[3])
    : m_data(2.0f, TRANSFORM_DIR_FORWARD)
{
    data().setValue(LIN_SIDE_BREAK, linSideBreakValues);
}

TransformRcPtr LogCameraTransformImpl::createEditableCopy() const
{
    static constexpr double LIN_SB[]{ 0.1, 0.1, 0.1 };
    LogCameraTransformRcPtr transform = LogCameraTransform::Create(LIN_SB);
    dynamic_cast<LogCameraTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection LogCameraTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void LogCameraTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void LogCameraTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
        if (data().getRedParams().size() < 5)
        {
            throw Exception("LinSideBreak has to be defined.");
        }
    }
    catch (Exception & ex)
    {
        std::string errMsg("LogCameraTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & LogCameraTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & LogCameraTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

bool LogCameraTransformImpl::equals(const LogCameraTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const LogCameraTransformImpl*>(&other)->data();
}

void LogCameraTransformImpl::setBase(double base) noexcept
{
    data().setBase(base);
}

double LogCameraTransformImpl::getBase() const noexcept
{
    return data().getBase();
}

void LogCameraTransformImpl::setLogSideSlopeValue(const double(&values)[3]) noexcept
{
    data().setValue(LOG_SIDE_SLOPE, values);
}
void LogCameraTransformImpl::setLogSideOffsetValue(const double(&values)[3]) noexcept
{
    data().setValue(LOG_SIDE_OFFSET, values);
}
void LogCameraTransformImpl::setLinSideSlopeValue(const double(&values)[3]) noexcept
{
    data().setValue(LIN_SIDE_SLOPE, values);
}
void LogCameraTransformImpl::setLinSideOffsetValue(const double(&values)[3]) noexcept
{
    data().setValue(LIN_SIDE_OFFSET, values);
}

void LogCameraTransformImpl::getLogSideSlopeValue(double(&values)[3]) const noexcept
{
    data().getValue(LOG_SIDE_SLOPE, values);
}
void LogCameraTransformImpl::getLogSideOffsetValue(double(&values)[3]) const noexcept
{
    data().getValue(LOG_SIDE_OFFSET, values);
}
void LogCameraTransformImpl::getLinSideSlopeValue(double(&values)[3]) const noexcept
{
    data().getValue(LIN_SIDE_SLOPE, values);
}
void LogCameraTransformImpl::getLinSideOffsetValue(double(&values)[3]) const noexcept
{
    data().getValue(LIN_SIDE_OFFSET, values);
}

void LogCameraTransformImpl::setLinSideBreakValue(const double(&values)[3]) noexcept
{
    data().setValue(LIN_SIDE_BREAK, values);
}

void LogCameraTransformImpl::setLinearSlopeValue(const double(&values)[3])
{
    data().setValue(LINEAR_SLOPE, values);
}

void LogCameraTransformImpl::unsetLinearSlopeValue()
{
    data().unsetLinearSlope();
}

void LogCameraTransformImpl::getLinSideBreakValue(double(&values)[3]) const noexcept
{
    data().getValue(LIN_SIDE_BREAK, values);
}

bool LogCameraTransformImpl::getLinearSlopeValue(double(&values)[3]) const
{
    return data().getValue(LINEAR_SLOPE, values);
}

std::ostream & operator<< (std::ostream & os, const LogCameraTransform & t)
{
    os << "<LogCameraTransform";
    os << " direction=" << TransformDirectionToString(t.getDirection());
    os << ", base=" << t.getBase();
    double values[3];
    t.getLogSideSlopeValue(values);
    os << ", logSideSlope=" << values[0] << " " << values[1] << " " << values[2];
    t.getLogSideOffsetValue(values);
    os << ", logSideOffset=" << values[0] << " " << values[1] << " " << values[2];
    t.getLinSideSlopeValue(values);
    os << ", linSideSlope=" << values[0] << " " << values[1] << " " << values[2];
    t.getLinSideOffsetValue(values);
    os << ", linSideOffset=" << values[0] << " " << values[1] << " " << values[2];
    t.getLinSideBreakValue(values);
    os << ", linSideBreak=" << values[0] << " " << values[1] << " " << values[2];
    if (t.getLinearSlopeValue(values))
    {
        os << ", linearSlope=" << values[0] << " " << values[1] << " " << values[2];
    }
    os << ">";

    return os;
}

} // namespace OCIO_NAMESPACE

