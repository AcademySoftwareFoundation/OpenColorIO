// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_EXPONENTTRANSFORM_H
#define INCLUDED_OCIO_EXPONENTTRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gamma/GammaOpData.h"


namespace OCIO_NAMESPACE
{

class ExponentTransformImpl : public ExponentTransform
{
public:
    ExponentTransformImpl() = default;
    ExponentTransformImpl(const ExponentTransformImpl &) = delete;
    ExponentTransformImpl & operator=(const ExponentTransformImpl &) = delete;
    virtual ~ExponentTransformImpl() = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    void validate() const override;

    bool equals(const ExponentTransform & other) const noexcept override;

    void getValue(double(&values)[4]) const noexcept override;
    void setValue(const double(&values)[4]) noexcept override;

    NegativeStyle getNegativeStyle() const override;
    void setNegativeStyle(NegativeStyle style) override;

    GammaOpData & data() noexcept { return m_data; }
    const GammaOpData & data() const noexcept { return m_data; }

    static void deleter(ExponentTransform * t);

private:
    GammaOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_EXPONENTTRANSFORM_H
