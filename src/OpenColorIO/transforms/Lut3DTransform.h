// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LUT3DTRANSFORM_H
#define INCLUDED_OCIO_LUT3DTRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/lut3d/Lut3DOpData.h"


namespace OCIO_NAMESPACE
{

class Lut3DTransformImpl : public Lut3DTransform
{
public:
    Lut3DTransformImpl();
    explicit Lut3DTransformImpl(unsigned long gridSize);
    Lut3DTransformImpl(const Lut3DTransformImpl &) = delete;
    Lut3DTransformImpl& operator=(const Lut3DTransformImpl &) = delete;
    virtual ~Lut3DTransformImpl() = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    BitDepth getFileOutputBitDepth() const noexcept override;
    void setFileOutputBitDepth(BitDepth bitDepth) noexcept override;

    void validate() const override;

    bool equals(const Lut3DTransform & other) const noexcept override;

    unsigned long getGridSize() const override;
    void setGridSize(unsigned long gridSize) override;

    void getValue(unsigned long indexR,
                  unsigned long indexG,
                  unsigned long indexB,
                  float & r, float & g, float & b) const override;
    void setValue(unsigned long indexR,
                  unsigned long indexG,
                  unsigned long indexB,
                  float r, float g, float b) override;

    Interpolation getInterpolation() const override;
    void setInterpolation(Interpolation algo) override;

    Lut3DOpData & data() noexcept { return m_data; }
    const Lut3DOpData & data() const noexcept { return m_data; }

    static void deleter(Lut3DTransform * t);

private:
    Lut3DOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_LUT3DTRANSFORM_H
