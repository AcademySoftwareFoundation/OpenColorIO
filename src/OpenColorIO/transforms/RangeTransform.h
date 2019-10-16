// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_RANGETRANSFORM_H
#define INCLUDED_OCIO_RANGETRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Range/RangeOpData.h"


OCIO_NAMESPACE_ENTER
{

class RangeTransformImpl : public RangeTransform
{
public:
    RangeTransformImpl() = default;
    RangeTransformImpl(const RangeTransformImpl &) = delete;
    RangeTransformImpl& operator=(const RangeTransformImpl &) = delete;
    virtual ~RangeTransformImpl() = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const override;
    void setDirection(TransformDirection dir) override;

    FormatMetadata & getFormatMetadata() override;
    const FormatMetadata & getFormatMetadata() const override;

    BitDepth getFileInputBitDepth() const override;
    BitDepth getFileOutputBitDepth() const override;
    void setFileInputBitDepth(BitDepth bitDepth) override;
    void setFileOutputBitDepth(BitDepth bitDepth) override;

    RangeStyle getStyle() const override;
    void setStyle(RangeStyle style) override;

    void validate() const override;

    bool equals(const RangeTransform & other) const override;

    void setMinInValue(double val) override;
    double getMinInValue() const override;
    bool hasMinInValue() const override;
    void unsetMinInValue() override;

    void setMaxInValue(double val) override;
    double getMaxInValue() const override;
    bool hasMaxInValue() const override;
    void unsetMaxInValue() override;

    void setMinOutValue(double val) override;
    double getMinOutValue() const override;
    bool hasMinOutValue() const override;
    void unsetMinOutValue() override;

    void setMaxOutValue(double val) override;
    double getMaxOutValue() const override;
    bool hasMaxOutValue() const override;
    void unsetMaxOutValue() override;

    RangeOpData & data() { return m_data; }
    const RangeOpData & data() const { return m_data; }

    static void deleter(RangeTransformImpl * t)
    {
        delete t;
    }

private:
    TransformDirection m_direction = TRANSFORM_DIR_FORWARD;
    RangeStyle  m_style = RANGE_CLAMP;
    RangeOpData m_data;
};


}
OCIO_NAMESPACE_EXIT

#endif  // INCLUDED_OCIO_RANGETRANSFORM_H
