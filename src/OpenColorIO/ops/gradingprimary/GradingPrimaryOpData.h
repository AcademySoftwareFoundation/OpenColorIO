// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGPRIMARY_OPDATA_H
#define INCLUDED_OCIO_GRADINGPRIMARY_OPDATA_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


namespace OCIO_NAMESPACE
{

class GradingPrimaryOpData;
typedef OCIO_SHARED_PTR<GradingPrimaryOpData> GradingPrimaryOpDataRcPtr;
typedef OCIO_SHARED_PTR<const GradingPrimaryOpData> ConstGradingPrimaryOpDataRcPtr;


class GradingPrimaryOpData : public OpData
{
public:

    explicit GradingPrimaryOpData(GradingStyle style);
    GradingPrimaryOpData() = delete;
    GradingPrimaryOpData(const GradingPrimaryOpData & rhs);
    GradingPrimaryOpData & operator=(const GradingPrimaryOpData & rhs);
    virtual ~GradingPrimaryOpData();

    GradingPrimaryOpDataRcPtr clone() const;

    void validate() const override;

    Type getType() const override { return GradingPrimaryType; }

    bool isNoOp() const override;
    bool isIdentity() const override;
    OpDataRcPtr getIdentityReplacement() const override;

    bool hasChannelCrosstalk() const override;

    bool isInverse(ConstGradingPrimaryOpDataRcPtr & r) const;
    GradingPrimaryOpDataRcPtr inverse() const;

    std::string getCacheID() const override;

    GradingStyle getStyle() const noexcept { return m_style; }
    void setStyle(GradingStyle style) noexcept;

    const GradingPrimary & getValue() const { return m_value->getValue(); }
    void setValue(const GradingPrimary & values) { m_value->setValue(values); }

    TransformDirection getDirection() const noexcept;
    void setDirection(TransformDirection dir) noexcept;

    bool isDynamic() const noexcept;
    DynamicPropertyRcPtr getDynamicProperty() const noexcept;
    void replaceDynamicProperty(DynamicPropertyGradingPrimaryImplRcPtr prop) noexcept;
    void removeDynamicProperty() noexcept;

    DynamicPropertyGradingPrimaryImplRcPtr getDynamicPropertyInternal() const noexcept
    {
        return m_value;
    }

    bool operator==(const OpData & other) const override;

private:
    GradingStyle                           m_style;
    DynamicPropertyGradingPrimaryImplRcPtr m_value;
};

} // namespace OCIO_NAMESPACE

#endif
