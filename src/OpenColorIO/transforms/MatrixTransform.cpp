// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "transforms/MatrixTransform.h"

namespace OCIO_NAMESPACE
{
MatrixTransformRcPtr MatrixTransform::Create()
{
    return MatrixTransformRcPtr(new MatrixTransformImpl(), &MatrixTransformImpl::deleter);
}

void MatrixTransformImpl::deleter(MatrixTransform * t)
{
    delete static_cast<MatrixTransformImpl *>(t);
}

TransformRcPtr MatrixTransformImpl::createEditableCopy() const
{
    MatrixTransformRcPtr transform = MatrixTransform::Create();
    dynamic_cast<MatrixTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection MatrixTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void MatrixTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void MatrixTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch(Exception & ex)
    {
        std::string errMsg("MatrixTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

BitDepth MatrixTransformImpl::getFileInputBitDepth() const noexcept
{
    return data().getFileInputBitDepth();
}
BitDepth MatrixTransformImpl::getFileOutputBitDepth() const noexcept
{
    return data().getFileOutputBitDepth();
}
void MatrixTransformImpl::setFileInputBitDepth(BitDepth bitDepth) noexcept
{
    data().setFileInputBitDepth(bitDepth);
}
void MatrixTransformImpl::setFileOutputBitDepth(BitDepth bitDepth) noexcept
{
    data().setFileOutputBitDepth(bitDepth);
}

FormatMetadata & MatrixTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & MatrixTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

bool MatrixTransformImpl::equals(const MatrixTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const MatrixTransformImpl*>(&other)->data();
}

void MatrixTransformImpl::setMatrix(const double * m44)
{
    if (m44) data().setRGBA(m44);
}

template<typename T>
void GetMatrix(const ArrayDouble::Values & vals, T * m44)
{
    static_assert(std::is_floating_point<T>::value, 
                    "Only single and double precision floats are supported");

    if (m44)
    {
        m44[0]  = (T)vals[0];
        m44[1]  = (T)vals[1];
        m44[2]  = (T)vals[2];
        m44[3]  = (T)vals[3];
        m44[4]  = (T)vals[4];
        m44[5]  = (T)vals[5];
        m44[6]  = (T)vals[6];
        m44[7]  = (T)vals[7];
        m44[8]  = (T)vals[8];
        m44[9]  = (T)vals[9];
        m44[10] = (T)vals[10];
        m44[11] = (T)vals[11];
        m44[12] = (T)vals[12];
        m44[13] = (T)vals[13];
        m44[14] = (T)vals[14];
        m44[15] = (T)vals[15];
    }
}

void MatrixTransformImpl::getMatrix(double * m44) const
{
    const ArrayDouble::Values & vals = data().getArray().getValues();
    GetMatrix(vals, m44);
}

void MatrixTransformImpl::setOffset(const double * offset4)
{
    if (offset4) data().setRGBAOffsets(offset4);
}

template<typename T>
void GetOffset(const double * vals, T * offset4)
{
    static_assert(std::is_floating_point<T>::value, 
                    "Only single and double precision floats are supported");

    if(offset4)
    {
        offset4[0] = (T)vals[0];
        offset4[1] = (T)vals[1];
        offset4[2] = (T)vals[2];
        offset4[3] = (T)vals[3];
    }
}

void MatrixTransformImpl::getOffset(double * offset4) const
{
    const double * vals = data().getOffsets().getValues();
    GetOffset(vals, offset4);
}

/*
Fit is canonically formulated as:
out = newmin + ((value-oldmin)/(oldmax-oldmin)*(newmax-newmin))
I.e., subtract the old offset, descale into the [0,1] range,
        scale into the new range, and add the new offset

We algebraiclly manipulate the terms into y = mx + b form as:
m = (newmax-newmin)/(oldmax-oldmin)
b = (newmin*oldmax - newmax*oldmin) / (oldmax-oldmin)
*/
void MatrixTransform::Fit(double * m44, double * offset4,
                            const double * oldmin4, const double * oldmax4,
                            const double * newmin4, const double * newmax4)
{
    if(!oldmin4 || !oldmax4) return;
    if(!newmin4 || !newmax4) return;

    if(m44) memset(m44, 0, 16*sizeof(double));
    if(offset4) memset(offset4, 0, 4*sizeof(double));

    for(int i=0; i<4; ++i)
    {
        double denom = oldmax4[i] - oldmin4[i];
        if(IsScalarEqualToZero(denom))
        {
            std::ostringstream os;
            os << "Cannot create Fit operator. ";
            os << "Max value equals min value '";
            os << oldmax4[i] << "' in channel index ";
            os << i << ".";
            throw Exception(os.str().c_str());
        }

        if(m44) m44[5*i] = (newmax4[i]-newmin4[i]) / denom;
        if(offset4) offset4[i] = (newmin4[i]*oldmax4[i] - newmax4[i]*oldmin4[i]) / denom;
    }
}

void MatrixTransform::Identity(double * m44, double * offset4)
{
    if (m44)
    {
        memset(m44, 0, 16 * sizeof(double));
        m44[0] = 1.0;
        m44[5] = 1.0;
        m44[10] = 1.0;
        m44[15] = 1.0;
    }

    if (offset4)
    {
        offset4[0] = 0.0;
        offset4[1] = 0.0;
        offset4[2] = 0.0;
        offset4[3] = 0.0;
    }
}

void MatrixTransform::Sat(double * m44, double * offset4,
                            double sat, const double * lumaCoef3)
{
    if(!lumaCoef3) return;

    if(m44)
    {
        m44[0] = (1. - sat) * lumaCoef3[0] + sat;
        m44[1] = (1. - sat) * lumaCoef3[1];
        m44[2] = (1. - sat) * lumaCoef3[2];
        m44[3] = 0.0;

        m44[4] = (1. - sat) * lumaCoef3[0];
        m44[5] = (1. - sat) * lumaCoef3[1] + sat;
        m44[6] = (1. - sat) * lumaCoef3[2];
        m44[7] = 0.0;

        m44[8] =  (1. - sat) * lumaCoef3[0];
        m44[9] =  (1. - sat) * lumaCoef3[1];
        m44[10] = (1. - sat) * lumaCoef3[2] + sat;
        m44[11] = 0.0;

        m44[12] = 0.0;
        m44[13] = 0.0;
        m44[14] = 0.0;
        m44[15] = 1.0;
    }

    if(offset4)
    {
        offset4[0] = 0.0;
        offset4[1] = 0.0;
        offset4[2] = 0.0;
        offset4[3] = 0.0;
    }
}

void MatrixTransform::Scale(double * m44, double * offset4,
                            const double * scale4)
{
    if(!scale4) return;

    if(m44)
    {
        memset(m44, 0, 16*sizeof(double));
        m44[0] = scale4[0];
        m44[5] = scale4[1];
        m44[10] = scale4[2];
        m44[15] = scale4[3];
    }

    if(offset4)
    {
        offset4[0] = 0.0;
        offset4[1] = 0.0;
        offset4[2] = 0.0;
        offset4[3] = 0.0;
    }
}
void MatrixTransform::View(double * m44, double * offset4,
                            int * channelHot4,
                            const double * lumaCoef3)
{
    if(!channelHot4 || !lumaCoef3) return;

    if(offset4)
    {
        offset4[0] = 0.0;
        offset4[1] = 0.0;
        offset4[2] = 0.0;
        offset4[3] = 0.0;
    }

    if(m44)
    {
        memset(m44, 0, 16*sizeof(double));

        // All channels are hot, return identity
        if(channelHot4[0] && channelHot4[1] &&
            channelHot4[2] && channelHot4[3])
        {
            Identity(m44, 0x0);
        }
        // If not all the channels are hot, but alpha is,
        // just show it.
        else if(channelHot4[3])
        {
            for(int i=0; i<4; ++i)
            {
                    m44[4*i+3] = 1.0;
            }
        }
        // Blend rgb as specified, place it in all 3 output
        // channels (to make a grayscale final image)
        else
        {
            double values[3] = { 0.0, 0.0, 0.0 };

            for(int i = 0; i < 3; ++i)
            {
                values[i] += lumaCoef3[i] * (channelHot4[i] ? 1.0 : 0.0);
            }

            double sum = values[0] + values[1] + values[2];
            if(!IsScalarEqualToZero(sum))
            {
                values[0] /= sum;
                values[1] /= sum;
                values[2] /= sum;
            }

            // Copy rgb into rgb rows
            for(int row=0; row<3; ++row)
            {
                for(int i=0; i<3; i++)
                {
                    m44[4*row+i] = values[i];
                }
            }

            // Preserve alpha
            m44[15] = 1.0;
        }
    }
}

namespace
{
const int DOUBLE_DECIMALS = 16;
}

std::ostream& operator<< (std::ostream& os, const MatrixTransform& t) noexcept
{
    double matrix[16], offset[4];

    t.getMatrix(matrix);
    t.getOffset(offset);

    os.precision(DOUBLE_DECIMALS);

    os << "<MatrixTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection());
    os << ", fileindepth=" << BitDepthToString(t.getFileInputBitDepth());
    os << ", fileoutdepth=" << BitDepthToString(t.getFileOutputBitDepth());
    os << ", matrix=" << matrix[0];
    for (int i = 1; i < 16; ++i)
    {
        os << " " << matrix[i];
    }
    os << ", offset=" << offset[0];
    for (int i = 1; i < 4; ++i)
    {
        os << " " << offset[i];
    }
    os << ">";
    return os;
}

} // namespace OCIO_NAMESPACE

