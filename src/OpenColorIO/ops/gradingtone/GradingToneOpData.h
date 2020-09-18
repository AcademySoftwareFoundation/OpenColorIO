// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGTONE_OPDATA_H
#define INCLUDED_OCIO_GRADINGTONE_OPDATA_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


namespace OCIO_NAMESPACE
{

class GradingToneOpData;
typedef OCIO_SHARED_PTR<GradingToneOpData> GradingToneOpDataRcPtr;
typedef OCIO_SHARED_PTR<const GradingToneOpData> ConstGradingToneOpDataRcPtr;


class GradingToneOpData : public OpData
{
public:

    static void ConvertStringToStyleAndDir(const char * str, GradingStyle & style,
                                           TransformDirection & dir);
    static const char * ConvertStyleAndDirToString(GradingStyle style,
                                                   TransformDirection dir);

    explicit GradingToneOpData(GradingStyle style);
    GradingToneOpData() = delete;
    GradingToneOpData(const GradingToneOpData & rhs);
    GradingToneOpData & operator=(const GradingToneOpData & rhs);
    virtual ~GradingToneOpData();

    GradingToneOpDataRcPtr clone() const;

    void validate() const override;

    Type getType() const override { return GradingToneType; }

    bool isNoOp() const override;
    bool isIdentity() const override;

    bool hasChannelCrosstalk() const override { return false; }

    bool isInverse(ConstGradingToneOpDataRcPtr & r) const;
    GradingToneOpDataRcPtr inverse() const;

    std::string getCacheID() const override;

    GradingStyle getStyle() const noexcept { return m_style; }
    void setStyle(GradingStyle style) noexcept;

    const GradingTone & getValue() const { return m_value->getValue(); }
    void setValue(const GradingTone & values) { m_value->setValue(values); }

    TransformDirection getDirection() const noexcept;
    void setDirection(TransformDirection dir) noexcept;

    bool isDynamic() const noexcept;
    DynamicPropertyRcPtr getDynamicProperty() const noexcept;
    void replaceDynamicProperty(DynamicPropertyGradingToneImplRcPtr prop) noexcept;
    void removeDynamicProperty() noexcept;

    DynamicPropertyGradingToneImplRcPtr getDynamicPropertyInternal() const noexcept
    {
        return m_value;
    }

    bool operator==(const OpData & other) const override;

private:
    GradingStyle                        m_style;
    DynamicPropertyGradingToneImplRcPtr m_value;
    TransformDirection                  m_direction{ TRANSFORM_DIR_FORWARD };
};

} // namespace OCIO_NAMESPACE

#endif
