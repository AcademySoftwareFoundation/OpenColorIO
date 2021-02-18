// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/LogTransform.h"

namespace OCIO_NAMESPACE
{
LogTransformRcPtr LogTransform::Create()
{
    return LogTransformRcPtr(new LogTransformImpl(), &LogTransformImpl::deleter);
}

void LogTransformImpl::deleter(LogTransform* t)
{
    delete static_cast<LogTransformImpl *>(t);
}

LogTransformImpl::LogTransformImpl()
    : m_data(2.0f, TRANSFORM_DIR_FORWARD)
{
}

TransformRcPtr LogTransformImpl::createEditableCopy() const
{
    LogTransformRcPtr transform = LogTransform::Create();
    dynamic_cast<LogTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection LogTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void LogTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void LogTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch (Exception & ex)
    {
        std::string errMsg("LogTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & LogTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & LogTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

bool LogTransformImpl::equals(const LogTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const LogTransformImpl*>(&other)->data();
}

double LogTransformImpl::getBase() const noexcept
{
    return data().getBase();
}

void LogTransformImpl::setBase(double val) noexcept
{
    data().setBase(val);
}

std::ostream & operator<< (std::ostream & os, const LogTransform & t)
{
    os << "<LogTransform";
    os << " direction=" << TransformDirectionToString(t.getDirection());
    os << ", base=" << t.getBase();
    os << ">";

    return os;
}

} // namespace OCIO_NAMESPACE

