// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_MATRIXTRANSFORM_H
#define INCLUDED_OCIO_MATRIXTRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/matrix/MatrixOpData.h"


namespace OCIO_NAMESPACE
{

class MatrixTransformImpl : public MatrixTransform
{
public:
    MatrixTransformImpl() = default;
    MatrixTransformImpl(const MatrixTransformImpl &) = delete;
    MatrixTransformImpl& operator=(const MatrixTransformImpl &) = delete;
    ~MatrixTransformImpl() override = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    BitDepth getFileInputBitDepth() const noexcept override;
    BitDepth getFileOutputBitDepth() const noexcept override;
    void setFileInputBitDepth(BitDepth bitDepth) noexcept override;
    void setFileOutputBitDepth(BitDepth bitDepth) noexcept override;

    void validate() const override;

    bool equals(const MatrixTransform & other) const noexcept override;

    void setMatrix(const double * m44) override;
    void getMatrix(double * m44) const override;

    void setOffset(const double * offset4) override;
    void getOffset(double * offset4) const override;

    MatrixOpData & data() noexcept { return m_data; }
    const MatrixOpData & data() const noexcept { return m_data; }

    static void deleter(MatrixTransform * t);

private:
    MatrixOpData m_data;
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_MATRIXTRANSFORM_H
