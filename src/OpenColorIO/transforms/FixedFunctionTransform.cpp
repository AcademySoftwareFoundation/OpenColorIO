// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"
#include "transforms/FixedFunctionTransform.h"

namespace OCIO_NAMESPACE
{

FixedFunctionTransformRcPtr FixedFunctionTransform::Create(FixedFunctionStyle style)
{
    return FixedFunctionTransformRcPtr(new FixedFunctionTransformImpl(style),
                                       &FixedFunctionTransformImpl::deleter);
}

FixedFunctionTransformRcPtr FixedFunctionTransform::Create(FixedFunctionStyle style,
                                                           const double * params,
                                                           size_t num)
{
    FixedFunctionOpData::Params p(num);
    std::copy(params, params + num, p.begin());

    return FixedFunctionTransformRcPtr(new FixedFunctionTransformImpl(style, p),
                                       &FixedFunctionTransformImpl::deleter);
}

FixedFunctionTransformImpl::FixedFunctionTransformImpl(FixedFunctionStyle style)
    : m_data(FixedFunctionOpData::ConvertStyle(style, TRANSFORM_DIR_FORWARD))
{
}

FixedFunctionTransformImpl::FixedFunctionTransformImpl(FixedFunctionStyle style,
                                                       const FixedFunctionOpData::Params & p)
    : m_data(FixedFunctionOpData::ConvertStyle(style, TRANSFORM_DIR_FORWARD), p)
{
}

void FixedFunctionTransformImpl::deleter(FixedFunctionTransform * t)
{
    delete static_cast<FixedFunctionTransformImpl *>(t);
}

TransformRcPtr FixedFunctionTransformImpl::createEditableCopy() const
{
    TransformRcPtr transform = FixedFunctionTransform::Create(getStyle());
    dynamic_cast<FixedFunctionTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection FixedFunctionTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void FixedFunctionTransformImpl::setDirection(TransformDirection dir) noexcept
{
    // NB: The direction is set by modifying the OpData style.
    data().setDirection(dir);
}

void FixedFunctionTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch (Exception & ex)
    {
        std::string errMsg("FixedFunctionTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & FixedFunctionTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & FixedFunctionTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

bool FixedFunctionTransformImpl::equals(const FixedFunctionTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const FixedFunctionTransformImpl*>(&other)->data();
}

FixedFunctionStyle FixedFunctionTransformImpl::getStyle() const
{
    return FixedFunctionOpData::ConvertStyle(data().getStyle());
}

void FixedFunctionTransformImpl::setStyle(FixedFunctionStyle style)
{
    const auto curDir = getDirection();
    data().setStyle(FixedFunctionOpData::ConvertStyle(style, curDir));
}

size_t FixedFunctionTransformImpl::getNumParams() const
{
    return data().getParams().size();
}

void FixedFunctionTransformImpl::setParams(const double * params, size_t num)
{
    FixedFunctionOpData::Params p(num);
    std::copy(params, params+num, p.begin());
    data().setParams(p);
}

void FixedFunctionTransformImpl::getParams(double * params) const
{
    FixedFunctionOpData::Params p = data().getParams();
    std::copy(p.cbegin(), p.cend(), params);
}

std::ostream & operator<< (std::ostream & os, const FixedFunctionTransform & t)
{
    os << "<FixedFunction ";
    os << "direction=" << TransformDirectionToString(t.getDirection());
    os << ", style=" << FixedFunctionStyleToString(t.getStyle());

    const size_t numParams = t.getNumParams();
    if(numParams>0)
    {
        FixedFunctionOpData::Params params(numParams, 0.);
        t.getParams(&params[0]);

        os << ", params=" << params[0];
        for (size_t i = 1; i < numParams; ++i)
        {
          os << " " << params[i];
        }
    }

    os << ">";
    return os;
}

} // namespace OCIO_NAMESPACE

