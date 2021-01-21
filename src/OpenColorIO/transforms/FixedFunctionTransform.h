// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_FIXEDFUNCTIONTRANSFORM_H
#define INCLUDED_OCIO_FIXEDFUNCTIONTRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/fixedfunction/FixedFunctionOpData.h"


namespace OCIO_NAMESPACE
{

class FixedFunctionTransformImpl : public FixedFunctionTransform
{
public:
    FixedFunctionTransformImpl() = delete;
    FixedFunctionTransformImpl(FixedFunctionStyle style);
    FixedFunctionTransformImpl(FixedFunctionStyle style, const FixedFunctionOpData::Params & p);
    FixedFunctionTransformImpl(const FixedFunctionTransformImpl &) = delete;
    FixedFunctionTransformImpl & operator=(const FixedFunctionTransformImpl &) = delete;
    virtual ~FixedFunctionTransformImpl() = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    void validate() const override;

    bool equals(const FixedFunctionTransform & other) const noexcept override;

    FixedFunctionStyle getStyle() const override;
    void setStyle(FixedFunctionStyle style) override;

    size_t getNumParams() const override;
    void getParams(double * params) const override;
    void setParams(const double * params, size_t num) override;

    FixedFunctionOpData & data() noexcept { return m_data; }
    const FixedFunctionOpData & data() const noexcept { return m_data; }

    static void deleter(FixedFunctionTransform * t);

private:
    FixedFunctionOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_FIXEDFUNCTIONTRANSFORM_H
