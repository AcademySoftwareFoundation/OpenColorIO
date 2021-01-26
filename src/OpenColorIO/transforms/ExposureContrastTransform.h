// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_EXPOSURECONTRASTTRANSFORM_H
#define INCLUDED_OCIO_EXPOSURECONTRASTTRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/exposurecontrast/ExposureContrastOpData.h"


namespace OCIO_NAMESPACE
{

class ExposureContrastTransformImpl : public ExposureContrastTransform
{
public:
    ExposureContrastTransformImpl() = default;
    ExposureContrastTransformImpl(const ExposureContrastTransformImpl &) = delete;
    ExposureContrastTransformImpl & operator=(const ExposureContrastTransformImpl &) = delete;
    virtual ~ExposureContrastTransformImpl() = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    void validate() const override;

    bool equals(const ExposureContrastTransform & other) const noexcept override;

    ExposureContrastStyle getStyle() const override;
    void setStyle(ExposureContrastStyle style) override;

    double getExposure() const override;
    void setExposure(double exposure) override;
    bool isExposureDynamic() const override;
    void makeExposureDynamic() override;
    void makeExposureNonDynamic() override;

    double getContrast() const override;
    void setContrast(double contrast) override;
    bool isContrastDynamic() const override;
    void makeContrastDynamic() override;
    void makeContrastNonDynamic() override;

    double getGamma() const override;
    void setGamma(double gamma) override;
    bool isGammaDynamic() const override;
    void makeGammaDynamic() override;
    void makeGammaNonDynamic() override;

    double getPivot() const override;
    void setPivot(double pivot) override;

    double getLogExposureStep() const override;
    void setLogExposureStep(double logExposureStep) override;

    double getLogMidGray() const override;
    void setLogMidGray(double logMidGray) override;

    ExposureContrastOpData & data() noexcept { return m_data; }
    const ExposureContrastOpData & data() const noexcept { return m_data; }

    static void deleter(ExposureContrastTransform * t);

private:
    ExposureContrastOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_EXPOSURECONTRASTTRANSFORM_H
