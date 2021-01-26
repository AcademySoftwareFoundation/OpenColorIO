// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LUT1DTRANSFORM_H
#define INCLUDED_OCIO_LUT1DTRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/lut1d/Lut1DOpData.h"


namespace OCIO_NAMESPACE
{

class Lut1DTransformImpl : public Lut1DTransform
{
public:
    Lut1DTransformImpl();
    Lut1DTransformImpl(Lut1DOpData::HalfFlags halfFlag, unsigned long length);
    Lut1DTransformImpl(const Lut1DTransformImpl &) = delete;
    Lut1DTransformImpl& operator=(const Lut1DTransformImpl &) = delete;
    virtual ~Lut1DTransformImpl() = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    BitDepth getFileOutputBitDepth() const noexcept override;
    void setFileOutputBitDepth(BitDepth bitDepth) noexcept override;

    void validate() const override;

    bool equals(const Lut1DTransform & other) const noexcept override;

    unsigned long getLength() const override;
    void setLength(unsigned long length) override;

    void getValue(unsigned long index, float & r, float & g, float & b) const override;
    void setValue(unsigned long index, float r, float g, float b) override;

    bool getInputHalfDomain() const noexcept override;
    void setInputHalfDomain(bool isHalfDomain) noexcept override;

    bool getOutputRawHalfs() const noexcept override;
    void setOutputRawHalfs(bool isRawHalfs) noexcept override;

    Lut1DHueAdjust getHueAdjust() const noexcept override;
    void setHueAdjust(Lut1DHueAdjust algo) override;

    Interpolation getInterpolation() const override;
    void setInterpolation(Interpolation algo) override;

    Lut1DOpData & data() noexcept { return m_data; }
    const Lut1DOpData & data() const noexcept { return m_data; }

    static void deleter(Lut1DTransform * t);

private:
    Lut1DOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_LUT1DTRANSFORM_H
