// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/log/LogOpData.h"
#include "transforms/LogAffineTransform.h"

namespace OCIO_NAMESPACE
{
LogAffineTransformRcPtr LogAffineTransform::Create()
{
    return LogAffineTransformRcPtr(new LogAffineTransformImpl(), &LogAffineTransformImpl::deleter);
}

void LogAffineTransformImpl::deleter(LogAffineTransform* t)
{
    delete static_cast<LogAffineTransformImpl *>(t);
}

LogAffineTransformImpl::LogAffineTransformImpl()
    : m_data(2.0f, TRANSFORM_DIR_FORWARD)
{
}

TransformRcPtr LogAffineTransformImpl::createEditableCopy() const
{
    LogAffineTransformRcPtr transform = LogAffineTransform::Create();
    dynamic_cast<LogAffineTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection LogAffineTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void LogAffineTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void LogAffineTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch (Exception & ex)
    {
        std::string errMsg("LogAffineTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & LogAffineTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & LogAffineTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

bool LogAffineTransformImpl::equals(const LogAffineTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const LogAffineTransformImpl*>(&other)->data();
}

void LogAffineTransformImpl::setBase(double base) noexcept
{
    data().setBase(base);
}

double LogAffineTransformImpl::getBase() const noexcept
{
    return data().getBase();
}

void LogAffineTransformImpl::setLogSideSlopeValue(const double(&values)[3]) noexcept
{
    data().setValue(LOG_SIDE_SLOPE, values);
}
void LogAffineTransformImpl::setLogSideOffsetValue(const double(&values)[3]) noexcept
{
    data().setValue(LOG_SIDE_OFFSET, values);
}
void LogAffineTransformImpl::setLinSideSlopeValue(const double(&values)[3]) noexcept
{
    data().setValue(LIN_SIDE_SLOPE, values);
}
void LogAffineTransformImpl::setLinSideOffsetValue(const double(&values)[3]) noexcept
{
    data().setValue(LIN_SIDE_OFFSET, values);
}

void LogAffineTransformImpl::getLogSideSlopeValue(double(&values)[3]) const noexcept
{
    data().getValue(LOG_SIDE_SLOPE, values);
}
void LogAffineTransformImpl::getLogSideOffsetValue(double(&values)[3]) const noexcept
{
    data().getValue(LOG_SIDE_OFFSET, values);
}
void LogAffineTransformImpl::getLinSideSlopeValue(double(&values)[3]) const noexcept
{
    data().getValue(LIN_SIDE_SLOPE, values);
}
void LogAffineTransformImpl::getLinSideOffsetValue(double(&values)[3]) const noexcept
{
    data().getValue(LIN_SIDE_OFFSET, values);
}

std::ostream & operator<< (std::ostream & os, const LogAffineTransform & t)
{
    os << "<LogAffineTransform";
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
    os << ">";

    return os;
}

} // namespace OCIO_NAMESPACE

