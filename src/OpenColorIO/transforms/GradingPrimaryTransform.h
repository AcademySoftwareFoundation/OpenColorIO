// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGPRIMARYTRANSFORM_H
#define INCLUDED_OCIO_GRADINGPRIMARYTRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradingprimary/GradingPrimaryOpData.h"


namespace OCIO_NAMESPACE
{

class GradingPrimaryTransformImpl : public GradingPrimaryTransform
{
public:
    explicit GradingPrimaryTransformImpl(GradingStyle style);
    GradingPrimaryTransformImpl() = delete;
    GradingPrimaryTransformImpl(const GradingPrimaryTransformImpl &) = delete;
    GradingPrimaryTransformImpl& operator=(const GradingPrimaryTransformImpl &) = delete;
    ~GradingPrimaryTransformImpl() override = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    void validate() const override;

    bool equals(const GradingPrimaryTransform & other) const noexcept override;

    GradingStyle getStyle() const noexcept override;

    void setStyle(GradingStyle style) noexcept override;

    const GradingPrimary & getValue() const override;
    void setValue(const GradingPrimary & values) override;

    bool isDynamic() const noexcept override;
    void makeDynamic() noexcept override;
    void makeNonDynamic() noexcept override;

    GradingPrimaryOpData & data() noexcept { return m_data; }
    const GradingPrimaryOpData & data() const noexcept { return m_data; }

    static void deleter(GradingPrimaryTransform * t);

private:
    GradingPrimaryOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_GRADINGPRIMARYTRANSFORM_H
