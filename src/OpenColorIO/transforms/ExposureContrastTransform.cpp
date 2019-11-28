// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "ops/exposurecontrast/ExposureContrastOpData.h"

namespace OCIO_NAMESPACE
{

ExposureContrastStyle ConvertStyle(ExposureContrastOpData::Style style)
{
    switch (style)
    {
    case ExposureContrastOpData::STYLE_VIDEO:
    case ExposureContrastOpData::STYLE_VIDEO_REV:
        return EXPOSURE_CONTRAST_VIDEO;

    case ExposureContrastOpData::STYLE_LOGARITHMIC:
    case ExposureContrastOpData::STYLE_LOGARITHMIC_REV:
        return EXPOSURE_CONTRAST_LOGARITHMIC;

    case ExposureContrastOpData::STYLE_LINEAR:
    case ExposureContrastOpData::STYLE_LINEAR_REV:
        return EXPOSURE_CONTRAST_LINEAR;
    }

    std::stringstream ss("Unknown ExposureContrast style: ");
    ss << style;

    throw Exception(ss.str().c_str());
}

ExposureContrastTransformRcPtr ExposureContrastTransform::Create()
{
    return ExposureContrastTransformRcPtr(new ExposureContrastTransform(), &deleter);
}

void ExposureContrastTransform::deleter(ExposureContrastTransform* t)
{
    delete t;
}


///////////////////////////////////////////////////////////////////////////

class ExposureContrastTransform::Impl : public ExposureContrastOpData
{
public:
    Impl()
        : ExposureContrastOpData()
        , m_direction(TRANSFORM_DIR_FORWARD)
    {
    }

    Impl(const Impl &) = delete;

    ~Impl() {}

    Impl& operator=(const Impl & rhs)
    {
        if (this != &rhs)
        {
            ExposureContrastOpData::operator=(rhs);
            m_direction = rhs.m_direction;
        }
        return *this;
    }

    bool equals(const Impl & rhs) const
    {
        if (this == &rhs) return true;

        return ExposureContrastOpData::operator==(rhs)
               && m_direction == rhs.m_direction;
    }

    TransformDirection m_direction;
};

///////////////////////////////////////////////////////////////////////////

ExposureContrastTransform::ExposureContrastTransform()
    : m_impl(new ExposureContrastTransform::Impl)
{
}

ExposureContrastTransform::~ExposureContrastTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

ExposureContrastTransform & ExposureContrastTransform::operator= (const ExposureContrastTransform & rhs)
{
    if (this != &rhs)
    {
        *m_impl = *rhs.m_impl;
    }
    return *this;
}

TransformRcPtr ExposureContrastTransform::createEditableCopy() const
{
    ExposureContrastTransformRcPtr transform = ExposureContrastTransform::Create();
    *transform->m_impl = *m_impl;
    return transform;
}

TransformDirection ExposureContrastTransform::getDirection() const
{
    return getImpl()->m_direction;
}

void ExposureContrastTransform::setDirection(TransformDirection dir)
{
    getImpl()->m_direction = dir;
}

void ExposureContrastTransform::validate() const
{
    Transform::validate();
    getImpl()->validate();
}

FormatMetadata & ExposureContrastTransform::getFormatMetadata()
{
    return m_impl->getFormatMetadata();
}

const FormatMetadata & ExposureContrastTransform::getFormatMetadata() const
{
    return m_impl->getFormatMetadata();
}

ExposureContrastStyle ExposureContrastTransform::getStyle() const
{
    return ConvertStyle(getImpl()->getStyle());
}

void ExposureContrastTransform::setStyle(ExposureContrastStyle style)
{
    getImpl()->setStyle(ExposureContrastOpData::ConvertStyle(style, TRANSFORM_DIR_FORWARD));
}

double ExposureContrastTransform::getExposure() const
{
    return getImpl()->getExposure();
}

void ExposureContrastTransform::setExposure(double exposure)
{
    getImpl()->setExposure(exposure);
}

void ExposureContrastTransform::makeExposureDynamic()
{
    getImpl()->getExposureProperty()->makeDynamic();
}

bool ExposureContrastTransform::isExposureDynamic() const
{
    return getImpl()->getExposureProperty()->isDynamic();
}

double ExposureContrastTransform::getContrast() const
{
    return getImpl()->getContrast();
}

void ExposureContrastTransform::setContrast(double contrast)
{
    getImpl()->setContrast(contrast);
}

void ExposureContrastTransform::makeContrastDynamic()
{
    getImpl()->getContrastProperty()->makeDynamic();
}

bool ExposureContrastTransform::isContrastDynamic() const
{
    return getImpl()->getContrastProperty()->isDynamic();
}

double ExposureContrastTransform::getGamma() const
{
    return getImpl()->getGamma();
}

void ExposureContrastTransform::setGamma(double gamma)
{
    getImpl()->setGamma(gamma);
}

void ExposureContrastTransform::makeGammaDynamic()
{
    getImpl()->getGammaProperty()->makeDynamic();
}

bool ExposureContrastTransform::isGammaDynamic() const
{
    return getImpl()->getGammaProperty()->isDynamic();
}

double ExposureContrastTransform::getPivot() const
{
    return getImpl()->getPivot();
}

void ExposureContrastTransform::setPivot(double pivot)
{
    getImpl()->setPivot(pivot);
}

double ExposureContrastTransform::getLogExposureStep() const
{
    return getImpl()->getLogExposureStep();
}

void ExposureContrastTransform::setLogExposureStep(double logExposureStep)
{
    getImpl()->setLogExposureStep(logExposureStep);
}

double ExposureContrastTransform::getLogMidGray() const
{
    return getImpl()->getLogMidGray();
}

void ExposureContrastTransform::setLogMidGray(double logMidGray)
{
    return getImpl()->setLogMidGray(logMidGray);
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

