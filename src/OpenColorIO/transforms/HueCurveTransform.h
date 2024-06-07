// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_HUECURVETRANSFORM_H
#define INCLUDED_OCIO_HUECURVETRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradinghuecurve/GradingHueCurveOpData.h"


namespace OCIO_NAMESPACE
{

class HueCurveTransformImpl : public HueCurveTransform
{
public:
    HueCurveTransformImpl(GradingStyle style);
    HueCurveTransformImpl() = delete;
    HueCurveTransformImpl(const HueCurveTransformImpl &) = delete;
    HueCurveTransformImpl& operator=(const HueCurveTransformImpl &) = delete;
    ~HueCurveTransformImpl() override = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    void validate() const override;

    bool equals(const HueCurveTransform & other) const noexcept override;

    GradingStyle getStyle() const noexcept override;

    void setStyle(GradingStyle style) noexcept override;

    const ConstGradingHueCurveRcPtr getValue() const override;
    
    void setValue(const ConstGradingHueCurveRcPtr & values) override;

    float getSlope(HueCurveType c, size_t index) const override;
    void setSlope(HueCurveType c, size_t index, float slope) override;
    bool slopesAreDefault(HueCurveType c) const override;

    bool getBypassLinToLog() const noexcept override;
    void setBypassLinToLog(bool bypass) noexcept override;

    bool isDynamic() const noexcept override;
    void makeDynamic() noexcept override;
    void makeNonDynamic() noexcept override;

    HueCurveOpData & data() noexcept { return m_data; }
    const HueCurveOpData & data() const noexcept { return m_data; }

    static void deleter(HueCurveTransform* t);

private:
    HueCurveOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_HUECURVETRANSFORM_H
