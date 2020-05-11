// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_RANGETRANSFORM_H
#define INCLUDED_OCIO_RANGETRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/range/RangeOpData.h"


namespace OCIO_NAMESPACE
{

class RangeTransformImpl : public RangeTransform
{
public:
    RangeTransformImpl() = default;
    RangeTransformImpl(const RangeTransformImpl &) = delete;
    RangeTransformImpl& operator=(const RangeTransformImpl &) = delete;
    ~RangeTransformImpl() override = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    BitDepth getFileInputBitDepth() const noexcept override;
    BitDepth getFileOutputBitDepth() const noexcept override;
    void setFileInputBitDepth(BitDepth bitDepth) noexcept override;
    void setFileOutputBitDepth(BitDepth bitDepth) noexcept override;

    RangeStyle getStyle() const noexcept override;
    void setStyle(RangeStyle style) noexcept override;

    void validate() const override;

    bool equals(const RangeTransform & other) const noexcept override;

    void setMinInValue(double val) noexcept override;
    double getMinInValue() const noexcept override;
    bool hasMinInValue() const noexcept override;
    void unsetMinInValue() noexcept override;

    void setMaxInValue(double val) noexcept override;
    double getMaxInValue() const noexcept override;
    bool hasMaxInValue() const noexcept override;
    void unsetMaxInValue() noexcept override;

    void setMinOutValue(double val) noexcept override;
    double getMinOutValue() const noexcept override;
    bool hasMinOutValue() const noexcept override;
    void unsetMinOutValue() noexcept override;

    void setMaxOutValue(double val) noexcept override;
    double getMaxOutValue() const noexcept override;
    bool hasMaxOutValue() const noexcept override;
    void unsetMaxOutValue() noexcept override;

    RangeOpData & data() noexcept { return m_data; }
    const RangeOpData & data() const noexcept { return m_data; }

    static void deleter(RangeTransform * t);

private:
    RangeStyle  m_style = RANGE_CLAMP;
    RangeOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_RANGETRANSFORM_H
