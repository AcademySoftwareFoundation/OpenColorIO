// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGRGBCURVETRANSFORM_H
#define INCLUDED_OCIO_GRADINGRGBCURVETRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradingrgbcurve/GradingRGBCurveOpData.h"


namespace OCIO_NAMESPACE
{

class GradingRGBCurveTransformImpl : public GradingRGBCurveTransform
{
public:
    explicit GradingRGBCurveTransformImpl(GradingStyle style);
    GradingRGBCurveTransformImpl() = delete;
    GradingRGBCurveTransformImpl(const GradingRGBCurveTransformImpl &) = delete;
    GradingRGBCurveTransformImpl& operator=(const GradingRGBCurveTransformImpl &) = delete;
    ~GradingRGBCurveTransformImpl() override = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    void validate() const override;

    bool equals(const GradingRGBCurveTransform & other) const noexcept override;

    GradingStyle getStyle() const noexcept override;

    void setStyle(GradingStyle style) noexcept override;

    const ConstGradingRGBCurveRcPtr getValue() const override;
    void setValue(const ConstGradingRGBCurveRcPtr & values) override;

    float getSlope(RGBCurveType c, size_t index) const override;
    void setSlope(RGBCurveType c, size_t index, float slope) override;
    bool slopesAreDefault(RGBCurveType c) const override;

    bool getBypassLinToLog() const override;
    void setBypassLinToLog(bool bypass) override;

    bool isDynamic() const noexcept override;
    void makeDynamic() noexcept override;
    void makeNonDynamic() noexcept override;

    GradingRGBCurveOpData & data() noexcept { return m_data; }
    const GradingRGBCurveOpData & data() const noexcept { return m_data; }

    static void deleter(GradingRGBCurveTransform * t);

private:
    GradingRGBCurveOpData m_data;
};
} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_GRADINGRGBCURVETRANSFORM_H
