// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_HUECURVE_OPDATA_H
#define INCLUDED_OCIO_HUECURVE_OPDATA_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


namespace OCIO_NAMESPACE
{

class HueCurveOpData;
typedef OCIO_SHARED_PTR<HueCurveOpData> HueCurveOpDataRcPtr;
typedef OCIO_SHARED_PTR<const HueCurveOpData> ConstHueCurveOpDataRcPtr;


class HueCurveOpData : public OpData
{
public:

    HueCurveOpData(GradingStyle style);
    HueCurveOpData(const HueCurveOpData & rhs);
    HueCurveOpData(GradingStyle style,
                   const std::array<ConstGradingBSplineCurveRcPtr, HUE_NUM_CURVES> & curve);
    HueCurveOpData & operator=(const HueCurveOpData & rhs);
    virtual ~HueCurveOpData();

    HueCurveOpDataRcPtr clone() const;

    void validate() const override;

    Type getType() const override { return GradingRGBCurveType; }

    bool isNoOp() const override;
    bool isIdentity() const override;

    bool hasChannelCrosstalk() const override { return false; }

    bool isInverse(ConstHueCurveOpDataRcPtr & r) const;
    HueCurveOpDataRcPtr inverse() const;

    std::string getCacheID() const override;

    GradingStyle getStyle() const noexcept { return m_style; }
    void setStyle(GradingStyle style) noexcept;

    const ConstHueCurveRcPtr getValue() const { return m_value->getValue(); }
    void setValue(const ConstHueCurveRcPtr & values) { m_value->setValue(values); }

    float getSlope(HueCurveType c, size_t index) const;
    void setSlope(HueCurveType c, size_t index, float slope);
    bool slopesAreDefault(HueCurveType c) const;

    bool getBypassLinToLog() const noexcept { return m_bypassLinToLog; }
    void setBypassLinToLog(bool bypass) noexcept { m_bypassLinToLog = bypass; }

    TransformDirection getDirection() const noexcept;
    void setDirection(TransformDirection dir) noexcept;

    bool isDynamic() const noexcept;
    DynamicPropertyRcPtr getDynamicProperty() const noexcept;
    void replaceDynamicProperty(DynamicPropertyHueCurveImplRcPtr prop) noexcept;
    void removeDynamicProperty() noexcept;

    DynamicPropertyHueCurveImplRcPtr getDynamicPropertyInternal() const noexcept
    {
        return m_value;
    }

    bool equals(const OpData & other) const override;

private:
    GradingStyle                            m_style;
    DynamicPropertyHueCurveImplRcPtr        m_value;
    bool                                    m_bypassLinToLog{ false };
    TransformDirection                      m_direction{ TRANSFORM_DIR_FORWARD };
};

bool operator==(const HueCurveOpData & lhs, const HueCurveOpData & rhs);

} // namespace OCIO_NAMESPACE

#endif
