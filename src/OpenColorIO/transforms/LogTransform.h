// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LOGTRANSFORM_H
#define INCLUDED_OCIO_LOGTRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/log/LogOpData.h"


namespace OCIO_NAMESPACE
{

class LogTransformImpl : public LogTransform
{
public:
    LogTransformImpl();
    LogTransformImpl(const LogTransformImpl &) = delete;
    LogTransformImpl & operator=(const LogTransformImpl &) = delete;
    virtual ~LogTransformImpl() = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    void validate() const override;

    bool equals(const LogTransform & other) const noexcept override;

    double getBase() const noexcept override;
    void setBase(double val) noexcept override;

    LogOpData & data() noexcept { return m_data; }
    const LogOpData & data() const noexcept { return m_data; }

    static void deleter(LogTransform * t);

private:
    LogOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_LOGTRANSFORM_H
