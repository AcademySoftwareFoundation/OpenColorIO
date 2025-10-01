// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_HUECURVE_OPDATA_H
#define INCLUDED_OCIO_HUECURVE_OPDATA_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


namespace OCIO_NAMESPACE
{

class GradingHueCurveOpData;
typedef OCIO_SHARED_PTR<GradingHueCurveOpData> GradingHueCurveOpDataRcPtr;
typedef OCIO_SHARED_PTR<const GradingHueCurveOpData> ConstGradingHueCurveOpDataRcPtr;


class GradingHueCurveOpData : public OpData
{
public:

    GradingHueCurveOpData(GradingStyle style);
    GradingHueCurveOpData(const GradingHueCurveOpData & rhs);
    GradingHueCurveOpData(GradingStyle style,
       ConstGradingBSplineCurveRcPtr hueHue,
       ConstGradingBSplineCurveRcPtr hueSat,
       ConstGradingBSplineCurveRcPtr hueLum,
       ConstGradingBSplineCurveRcPtr lumSat,
       ConstGradingBSplineCurveRcPtr satSat,
       ConstGradingBSplineCurveRcPtr lumLum,
       ConstGradingBSplineCurveRcPtr satLum,
       ConstGradingBSplineCurveRcPtr hueFx);
    GradingHueCurveOpData & operator=(const GradingHueCurveOpData & rhs);
    virtual ~GradingHueCurveOpData();

    GradingHueCurveOpDataRcPtr clone() const;

    void validate() const override;

    Type getType() const override { return GradingHueCurveType; }

    bool isNoOp() const override;
    bool isIdentity() const override;

    bool hasChannelCrosstalk() const override { return true; }

    bool isInverse(ConstGradingHueCurveOpDataRcPtr & r) const;
    GradingHueCurveOpDataRcPtr inverse() const;

    std::string getCacheID() const override;

    GradingStyle getStyle() const noexcept { return m_style; }
    void setStyle(GradingStyle style) noexcept;

    const ConstGradingHueCurveRcPtr getValue() const { return m_value->getValue(); }
    void setValue(const ConstGradingHueCurveRcPtr & values) { m_value->setValue(values); }

    float getSlope(HueCurveType c, size_t index) const;
    void setSlope(HueCurveType c, size_t index, float slope);
    bool slopesAreDefault(HueCurveType c) const;

    HSYTransformStyle getRGBToHSY() const noexcept { return m_RGBToHSY; }
    void setRGBToHSY(HSYTransformStyle style) noexcept { m_RGBToHSY = style; }

    TransformDirection getDirection() const noexcept;
    void setDirection(TransformDirection dir) noexcept;

    bool isDynamic() const noexcept;
    DynamicPropertyRcPtr getDynamicProperty() const noexcept;
    void replaceDynamicProperty(DynamicPropertyGradingHueCurveImplRcPtr prop) noexcept;
    void removeDynamicProperty() noexcept;

    DynamicPropertyGradingHueCurveImplRcPtr getDynamicPropertyInternal() const noexcept
    {
        return m_value;
    }

    bool equals(const OpData & other) const override;

private:
    GradingStyle                            m_style;
    DynamicPropertyGradingHueCurveImplRcPtr m_value;
    HSYTransformStyle                       m_RGBToHSY{ HSYTransformStyle::HSY_TRANSFORM_1 };
    TransformDirection                      m_direction{ TRANSFORM_DIR_FORWARD };
};

bool operator==(const GradingHueCurveOpData & lhs, const GradingHueCurveOpData & rhs);

} // namespace OCIO_NAMESPACE

#endif
