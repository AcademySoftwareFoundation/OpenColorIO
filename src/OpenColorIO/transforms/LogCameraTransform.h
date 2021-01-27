// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LOGCAMERATRANSFORM_H
#define INCLUDED_OCIO_LOGCAMERATRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/log/LogOpData.h"

namespace OCIO_NAMESPACE
{

class LogCameraTransformImpl : public LogCameraTransform
{
public:
    LogCameraTransformImpl() = delete;
    LogCameraTransformImpl(const double(&linSideBreakValues)[3]);
    LogCameraTransformImpl(const LogCameraTransformImpl &) = delete;
    LogCameraTransformImpl & operator=(const LogCameraTransformImpl &) = delete;
    virtual ~LogCameraTransformImpl() = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    void validate() const override;

    bool equals(const LogCameraTransform & other) const noexcept override;

    double getBase() const noexcept override;
    void setBase(double val) noexcept override;

    void getLogSideSlopeValue(double(&values)[3]) const noexcept override;
    void setLogSideSlopeValue(const double(&values)[3]) noexcept override;
    void getLogSideOffsetValue(double(&values)[3]) const noexcept override;
    void setLogSideOffsetValue(const double(&values)[3]) noexcept override;
    void getLinSideSlopeValue(double(&values)[3]) const noexcept override;
    void setLinSideSlopeValue(const double(&values)[3]) noexcept override;
    void getLinSideOffsetValue(double(&values)[3]) const noexcept override;
    void setLinSideOffsetValue(const double(&values)[3]) noexcept override;

    void getLinSideBreakValue(double(&values)[3]) const noexcept override;
    void setLinSideBreakValue(const double(&values)[3]) noexcept override;
    
    bool getLinearSlopeValue(double(&values)[3]) const override;
    void setLinearSlopeValue(const double(&values)[3]) override;
    void unsetLinearSlopeValue() override;

    LogOpData & data() noexcept { return m_data; }
    const LogOpData & data() const noexcept { return m_data; }

    static void deleter(LogCameraTransform * t);

private:
    LogOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_LOGCAMERATRANSFORM_H
