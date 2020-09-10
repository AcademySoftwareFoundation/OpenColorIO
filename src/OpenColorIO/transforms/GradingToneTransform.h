// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGTONETRANSFORM_H
#define INCLUDED_OCIO_GRADINGTONETRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradingtone/GradingToneOpData.h"


namespace OCIO_NAMESPACE
{

class GradingToneTransformImpl : public GradingToneTransform
{
public:
    explicit GradingToneTransformImpl(GradingStyle style);
    GradingToneTransformImpl() = delete;
    GradingToneTransformImpl(const GradingToneTransformImpl &) = delete;
    GradingToneTransformImpl& operator=(const GradingToneTransformImpl &) = delete;
    ~GradingToneTransformImpl() override = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    void validate() const override;

    bool equals(const GradingToneTransform & other) const noexcept override;

    GradingStyle getStyle() const noexcept override;

    void setStyle(GradingStyle style) noexcept override;

    const GradingTone & getValue() const override;
    void setValue(const GradingTone & values) override;

    bool isDynamic() const noexcept override;
    void makeDynamic() noexcept override;
    void makeNonDynamic() noexcept override;

    GradingToneOpData & data() noexcept { return m_data; }
    const GradingToneOpData & data() const noexcept { return m_data; }

    static void deleter(GradingToneTransform * t);

private:
    GradingToneOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_GRADINGTONETRANSFORM_H
