// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_HUECURVETRANSFORM_H
#define INCLUDED_OCIO_HUECURVETRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradinghuecurve/GradingHueCurveOpData.h"


namespace OCIO_NAMESPACE
{

class GradingHueCurveTransformImpl : public GradingHueCurveTransform
{
public:
    GradingHueCurveTransformImpl(GradingStyle style);
    GradingHueCurveTransformImpl() = delete;
    GradingHueCurveTransformImpl(const GradingHueCurveTransformImpl &) = delete;
    GradingHueCurveTransformImpl& operator=(const GradingHueCurveTransformImpl &) = delete;
    ~GradingHueCurveTransformImpl() override = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    void validate() const override;

    bool equals(const GradingHueCurveTransform & other) const noexcept override;

    GradingStyle getStyle() const noexcept override;

    void setStyle(GradingStyle style) noexcept override;

    const ConstGradingHueCurveRcPtr getValue() const override;
    
    void setValue(const ConstGradingHueCurveRcPtr & values) override;

    float getSlope(HueCurveType c, size_t index) const override;
    void setSlope(HueCurveType c, size_t index, float slope) override;
    bool slopesAreDefault(HueCurveType c) const override;

    HSYTransformStyle getRGBToHSY() const noexcept override;
    void setRGBToHSY(HSYTransformStyle style) noexcept override;
    
    bool isDynamic() const noexcept override;
    void makeDynamic() noexcept override;
    void makeNonDynamic() noexcept override;

    GradingHueCurveOpData & data() noexcept { return m_data; }
    const GradingHueCurveOpData & data() const noexcept { return m_data; }

    static void deleter(GradingHueCurveTransform* t);

private:
    GradingHueCurveOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_HUECURVETRANSFORM_H
