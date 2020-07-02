// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"
#include "transforms/ExponentWithLinearTransform.h"

namespace OCIO_NAMESPACE
{

ExponentWithLinearTransformRcPtr ExponentWithLinearTransform::Create()
{
    return ExponentWithLinearTransformRcPtr(new ExponentWithLinearTransformImpl(),
                                            &ExponentWithLinearTransformImpl::deleter);
}

void ExponentWithLinearTransformImpl::deleter(ExponentWithLinearTransform * t)
{
    delete static_cast<ExponentWithLinearTransformImpl *>(t);
}

ExponentWithLinearTransformImpl::ExponentWithLinearTransformImpl()
{
    data().setRedParams({ 1., 0. });
    data().setGreenParams({ 1., 0. });
    data().setBlueParams({ 1., 0. });
    data().setAlphaParams({ 1., 0. });

    data().setStyle(GammaOpData::MONCURVE_FWD);
}

TransformRcPtr ExponentWithLinearTransformImpl::createEditableCopy() const
{
    TransformRcPtr transform = ExponentWithLinearTransform::Create();
    dynamic_cast<ExponentWithLinearTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection ExponentWithLinearTransformImpl::getDirection() const noexcept
{
    const auto style = data().getStyle();
    if (GammaOpData::MONCURVE_FWD == style || GammaOpData::MONCURVE_MIRROR_FWD == style)
    {
        return TRANSFORM_DIR_FORWARD;
    }
    else
    {
        return TRANSFORM_DIR_INVERSE;
    }
}

void ExponentWithLinearTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void ExponentWithLinearTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch (Exception & ex)
    {
        std::string errMsg("ExponentWithLinearTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & ExponentWithLinearTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & ExponentWithLinearTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

bool ExponentWithLinearTransformImpl::equals(const ExponentWithLinearTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const ExponentWithLinearTransformImpl*>(&other)->data();
}

void ExponentWithLinearTransformImpl::setGamma(const double(&values)[4]) noexcept
{
    data().getRedParams()  [0] = values[0];
    data().getGreenParams()[0] = values[1];
    data().getBlueParams() [0] = values[2];
    data().getAlphaParams()[0] = values[3];
}

void ExponentWithLinearTransformImpl::getGamma(double(&values)[4]) const noexcept
{
    values[0] = data().getRedParams()  [0];
    values[1] = data().getGreenParams()[0];
    values[2] = data().getBlueParams() [0];
    values[3] = data().getAlphaParams()[0];
}

void ExponentWithLinearTransformImpl::setOffset(const double(&values)[4]) noexcept
{
    const GammaOpData::Params red = { data().getRedParams()  [0], values[0] };
    const GammaOpData::Params grn = { data().getGreenParams()[0], values[1] };
    const GammaOpData::Params blu = { data().getBlueParams() [0], values[2] };
    const GammaOpData::Params alp = { data().getAlphaParams()[0], values[3] };

    data().setRedParams(red);
    data().setGreenParams(grn);
    data().setBlueParams(blu);
    data().setAlphaParams(alp);
}

void ExponentWithLinearTransformImpl::getOffset(double(&values)[4]) const noexcept
{
    values[0] = data().getRedParams().size()  == 2 ? data().getRedParams()  [1] : 0.;
    values[1] = data().getGreenParams().size()== 2 ? data().getGreenParams()[1] : 0.;
    values[2] = data().getBlueParams().size() == 2 ? data().getBlueParams() [1] : 0.;
    values[3] = data().getAlphaParams().size()== 2 ? data().getAlphaParams()[1] : 0.;
}

NegativeStyle ExponentWithLinearTransformImpl::getNegativeStyle() const
{
    return GammaOpData::ConvertStyle(data().getStyle());
}

void ExponentWithLinearTransformImpl::setNegativeStyle(NegativeStyle style)
{
    const auto dir = getDirection();
    auto styleOp = GammaOpData::ConvertStyleMonCurve(style, dir);
    data().setStyle(styleOp);
}

std::ostream & operator<< (std::ostream & os, const ExponentWithLinearTransform & t)
{
    os << "<ExponentWithLinearTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";

    double gamma[4];
    t.getGamma(gamma);

    os << "gamma=" << gamma[0];
    for (int i = 1; i < 4; ++i)
    {
      os << " " << gamma[i];
    }

    double offset[4];
    t.getOffset(offset);

    os << ", offset=" << offset[0];
    for (int i = 1; i < 4; ++i)
    {
      os << " " << offset[i];
    }

    os << ", style=" << NegativeStyleToString(t.getNegativeStyle());
    os << ">";
    return os;
}

} // namespace OCIO_NAMESPACE

