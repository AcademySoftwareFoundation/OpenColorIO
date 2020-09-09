// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/ExposureContrastTransform.h"

namespace OCIO_NAMESPACE
{

ExposureContrastTransformRcPtr ExposureContrastTransform::Create()
{
    return ExposureContrastTransformRcPtr(new ExposureContrastTransformImpl(),
                                          &ExposureContrastTransformImpl::deleter);
}

void ExposureContrastTransformImpl::deleter(ExposureContrastTransform * t)
{
    delete static_cast<ExposureContrastTransformImpl *>(t);
}

TransformRcPtr ExposureContrastTransformImpl::createEditableCopy() const
{
    TransformRcPtr transform = ExposureContrastTransform::Create();
    dynamic_cast<ExposureContrastTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection ExposureContrastTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void ExposureContrastTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void ExposureContrastTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch (Exception & ex)
    {
        std::string errMsg("ExposureContrastTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & ExposureContrastTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & ExposureContrastTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

bool ExposureContrastTransformImpl::equals(const ExposureContrastTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const ExposureContrastTransformImpl*>(&other)->data();
}

ExposureContrastStyle ExposureContrastTransformImpl::getStyle() const
{
    return ExposureContrastOpData::ConvertStyle(data().getStyle());
}

void ExposureContrastTransformImpl::setStyle(ExposureContrastStyle style)
{
    auto curDir = getDirection();
    data().setStyle(ExposureContrastOpData::ConvertStyle(style, curDir));
}

double ExposureContrastTransformImpl::getExposure() const
{
    return data().getExposure();
}

void ExposureContrastTransformImpl::setExposure(double exposure)
{
    data().setExposure(exposure);
}

void ExposureContrastTransformImpl::makeExposureDynamic()
{
    data().getExposureProperty()->makeDynamic();
}

void ExposureContrastTransformImpl::makeExposureNonDynamic()
{
    data().getExposureProperty()->makeNonDynamic();
}

bool ExposureContrastTransformImpl::isExposureDynamic() const
{
    return data().getExposureProperty()->isDynamic();
}

double ExposureContrastTransformImpl::getContrast() const
{
    return data().getContrast();
}

void ExposureContrastTransformImpl::setContrast(double contrast)
{
    data().setContrast(contrast);
}

void ExposureContrastTransformImpl::makeContrastDynamic()
{
    data().getContrastProperty()->makeDynamic();
}

void ExposureContrastTransformImpl::makeContrastNonDynamic()
{
    data().getContrastProperty()->makeNonDynamic();
}

bool ExposureContrastTransformImpl::isContrastDynamic() const
{
    return data().getContrastProperty()->isDynamic();
}

double ExposureContrastTransformImpl::getGamma() const
{
    return data().getGamma();
}

void ExposureContrastTransformImpl::setGamma(double gamma)
{
    data().setGamma(gamma);
}

void ExposureContrastTransformImpl::makeGammaDynamic()
{
    data().getGammaProperty()->makeDynamic();
}

void ExposureContrastTransformImpl::makeGammaNonDynamic()
{
    data().getGammaProperty()->makeNonDynamic();
}

bool ExposureContrastTransformImpl::isGammaDynamic() const
{
    return data().getGammaProperty()->isDynamic();
}

double ExposureContrastTransformImpl::getPivot() const
{
    return data().getPivot();
}

void ExposureContrastTransformImpl::setPivot(double pivot)
{
    data().setPivot(pivot);
}

double ExposureContrastTransformImpl::getLogExposureStep() const
{
    return data().getLogExposureStep();
}

void ExposureContrastTransformImpl::setLogExposureStep(double logExposureStep)
{
    data().setLogExposureStep(logExposureStep);
}

double ExposureContrastTransformImpl::getLogMidGray() const
{
    return data().getLogMidGray();
}

void ExposureContrastTransformImpl::setLogMidGray(double logMidGray)
{
    return data().setLogMidGray(logMidGray);
}


std::ostream& operator<< (std::ostream & os, const ExposureContrastTransform & t)
{
    os << "<ExposureContrast ";
    os << "direction=" << TransformDirectionToString(t.getDirection());
    os << ", style=" << ExposureContrastStyleToString(t.getStyle());

    os << ", exposure=" << t.getExposure();
    os << ", contrast=" << t.getContrast();
    os << ", gamma=" << t.getGamma();
    os << ", pivot=" << t.getPivot();
    os << ", logExposureStep=" << t.getLogExposureStep();
    os << ", logMidGray=" << t.getLogMidGray();
    if (t.isExposureDynamic())
    {
        os << ", exposureDynamic";
    }
    if (t.isContrastDynamic())
    {
        os << ", contrastDynamic";
    }
    if (t.isGammaDynamic())
    {
        os << ", gammaDynamic";
    }

    os << ">";
    return os;
}

} // namespace OCIO_NAMESPACE

