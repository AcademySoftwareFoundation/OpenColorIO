// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGRGBCURVE_OPDATA_H
#define INCLUDED_OCIO_GRADINGRGBCURVE_OPDATA_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


namespace OCIO_NAMESPACE
{

class GradingRGBCurveOpData;
typedef OCIO_SHARED_PTR<GradingRGBCurveOpData> GradingRGBCurveOpDataRcPtr;
typedef OCIO_SHARED_PTR<const GradingRGBCurveOpData> ConstGradingRGBCurveOpDataRcPtr;


class GradingRGBCurveOpData : public OpData
{
public:

    explicit GradingRGBCurveOpData(GradingStyle style);
    GradingRGBCurveOpData(GradingStyle style,
                          ConstGradingBSplineCurveRcPtr red,
                          ConstGradingBSplineCurveRcPtr green,
                          ConstGradingBSplineCurveRcPtr blue,
                          ConstGradingBSplineCurveRcPtr master);
    GradingRGBCurveOpData() = delete;
    GradingRGBCurveOpData(const GradingRGBCurveOpData & rhs);
    GradingRGBCurveOpData & operator=(const GradingRGBCurveOpData & rhs);
    virtual ~GradingRGBCurveOpData();

    GradingRGBCurveOpDataRcPtr clone() const;

    void validate() const override;

    Type getType() const override { return GradingRGBCurveType; }

    bool isNoOp() const override;
    bool isIdentity() const override;

    bool hasChannelCrosstalk() const override { return false; }

    bool isInverse(ConstGradingRGBCurveOpDataRcPtr & r) const;
    GradingRGBCurveOpDataRcPtr inverse() const;

    std::string getCacheID() const override;

    GradingStyle getStyle() const noexcept { return m_style; }
    void setStyle(GradingStyle style) noexcept;

    const ConstGradingRGBCurveRcPtr getValue() const { return m_value->getValue(); }
    void setValue(const ConstGradingRGBCurveRcPtr & values) { m_value->setValue(values); }

    float getSlope(RGBCurveType c, size_t index) const;
    void setSlope(RGBCurveType c, size_t index, float slope);
    bool slopesAreDefault(RGBCurveType c) const;

    bool getBypassLinToLog() const noexcept { return m_bypassLinToLog; }
    void setBypassLinToLog(bool bypass) noexcept { m_bypassLinToLog = bypass; }

    TransformDirection getDirection() const noexcept;
    void setDirection(TransformDirection dir) noexcept;

    bool isDynamic() const noexcept;
    DynamicPropertyRcPtr getDynamicProperty() const noexcept;
    void replaceDynamicProperty(DynamicPropertyGradingRGBCurveImplRcPtr prop) noexcept;
    void removeDynamicProperty() noexcept;

    DynamicPropertyGradingRGBCurveImplRcPtr getDynamicPropertyInternal() const noexcept
    {
        return m_value;
    }

    bool operator==(const OpData & other) const override;

private:
    GradingStyle                            m_style;
    DynamicPropertyGradingRGBCurveImplRcPtr m_value;
    bool                                    m_bypassLinToLog{ false };
    TransformDirection                      m_direction{ TRANSFORM_DIR_FORWARD };
};



} // namespace OCIO_NAMESPACE

#endif
