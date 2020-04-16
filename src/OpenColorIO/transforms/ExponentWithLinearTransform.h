// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_EXPONENTWITHLENEARTRANSFORM_H
#define INCLUDED_OCIO_EXPONENTWITHLENEARTRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gamma/GammaOpData.h"


namespace OCIO_NAMESPACE
{

class ExponentWithLinearTransformImpl : public ExponentWithLinearTransform
{
public:
    ExponentWithLinearTransformImpl();
    ExponentWithLinearTransformImpl(const ExponentWithLinearTransformImpl &) = delete;
    ExponentWithLinearTransformImpl & operator=(const ExponentWithLinearTransformImpl &) = delete;
    virtual ~ExponentWithLinearTransformImpl() = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    void validate() const override;

    bool equals(const ExponentWithLinearTransform & other) const noexcept override;

    void getGamma(double(&values)[4]) const noexcept override;
    void setGamma(const double(&values)[4]) noexcept override;

    void getOffset(double(&values)[4]) const noexcept override;
    void setOffset(const double(&values)[4]) noexcept override;

    NegativeStyle getNegativeStyle() const override;
    void setNegativeStyle(NegativeStyle style) override;

    GammaOpData & data() noexcept { return m_data; }
    const GammaOpData & data() const noexcept { return m_data; }

    static void deleter(ExponentWithLinearTransform * t);

private:
    GammaOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_EXPONENTWITHLENEARTRANSFORM_H
