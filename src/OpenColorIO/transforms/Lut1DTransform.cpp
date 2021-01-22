// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/Lut1DTransform.h"

namespace OCIO_NAMESPACE
{
Lut1DTransformRcPtr Lut1DTransform::Create()
{
    return Lut1DTransformRcPtr(new Lut1DTransformImpl(), &Lut1DTransformImpl::deleter);
}

Lut1DTransformRcPtr Lut1DTransform::Create(unsigned long length, bool isHalfDomain)
{
    const auto halfFlag = isHalfDomain ? Lut1DOpData::LUT_INPUT_HALF_CODE :
                                         Lut1DOpData::LUT_STANDARD;
    return Lut1DTransformRcPtr(new Lut1DTransformImpl(halfFlag, length),
                               &Lut1DTransformImpl::deleter);
}

void Lut1DTransformImpl::deleter(Lut1DTransform* t)
{
    delete static_cast<Lut1DTransformImpl *>(t);
}

Lut1DTransformImpl::Lut1DTransformImpl()
    : m_data(2)
{
}

Lut1DTransformImpl::Lut1DTransformImpl(Lut1DOpData::HalfFlags halfFlag, unsigned long length)
    : m_data(halfFlag, length, false)
{
}

TransformRcPtr Lut1DTransformImpl::createEditableCopy() const
{
    Lut1DTransformRcPtr transform = Lut1DTransform::Create();
    dynamic_cast<Lut1DTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection Lut1DTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void Lut1DTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void Lut1DTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch (Exception & ex)
    {
        std::ostringstream oss;
        oss << "Lut1DTransform validation failed: ";
        oss << ex.what();
        throw Exception(oss.str().c_str());
    }
}

BitDepth Lut1DTransformImpl::getFileOutputBitDepth() const noexcept
{
    return data().getFileOutputBitDepth();
}

void Lut1DTransformImpl::setFileOutputBitDepth(BitDepth bitdepth) noexcept
{
    return data().setFileOutputBitDepth(bitdepth);
}

FormatMetadata & Lut1DTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & Lut1DTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

bool Lut1DTransformImpl::equals(const Lut1DTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const Lut1DTransformImpl*>(&other)->data();
}

void Lut1DTransformImpl::setLength(unsigned long length)
{
    auto & lutArray = m_data.getArray();
    // Use NaNs for the 2048 NaN values in the domain.
    lutArray = Lut1DOpData::Lut3by1DArray(m_data.getHalfFlags(), 3, length, false);
}

namespace
{
void CheckLUT1DIndex(const char * function, unsigned long index, unsigned long size)
{
    if (index >= size)
    {
        std::ostringstream oss;
        oss << "Lut1DTransform " << function << ": index (";
        oss << index << ") should be less than the length (";
        oss << size << ").";
        throw Exception(oss.str().c_str());
    }
}
}

void Lut1DTransformImpl::setValue(unsigned long index, float r, float g, float b)
{
    CheckLUT1DIndex("setValue", index, getLength());
    data().getArray()[3 * index] = r;
    data().getArray()[3 * index + 1] = g;
    data().getArray()[3 * index + 2] = b;
}

void Lut1DTransformImpl::setInputHalfDomain(bool isHalfDomain) noexcept
{
    data().setInputHalfDomain(isHalfDomain);
}

void Lut1DTransformImpl::setOutputRawHalfs(bool isRawHalfs) noexcept
{
    data().setOutputRawHalfs(isRawHalfs);
}

void Lut1DTransformImpl::setHueAdjust(Lut1DHueAdjust algo)
{
    data().setHueAdjust(algo);
}

void Lut1DTransformImpl::setInterpolation(Interpolation algo)
{
    data().setInterpolation(algo);
}

unsigned long Lut1DTransformImpl::getLength() const
{
    return data().getArray().getLength();
}

void Lut1DTransformImpl::getValue(unsigned long index, float & r, float & g, float & b) const
{
    CheckLUT1DIndex("getValue", index, getLength());
    r = data().getArray()[3 * index];
    g = data().getArray()[3 * index + 1];
    b = data().getArray()[3 * index + 2];
}

bool Lut1DTransformImpl::getInputHalfDomain() const noexcept
{
    return data().isInputHalfDomain();
}

bool Lut1DTransformImpl::getOutputRawHalfs() const noexcept
{
    return data().isOutputRawHalfs();
}

Lut1DHueAdjust Lut1DTransformImpl::getHueAdjust() const noexcept
{
    return data().getHueAdjust();
}

Interpolation Lut1DTransformImpl::getInterpolation() const
{
    return data().getInterpolation();
}

std::ostream & operator<< (std::ostream & os, const Lut1DTransform & t)
{
    os << "<Lut1DTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
    os << "fileoutdepth=" << BitDepthToString(t.getFileOutputBitDepth()) << ", ";
    os << "interpolation=" << InterpolationToString(t.getInterpolation()) << ", ";
    os << "inputhalf=" << t.getInputHalfDomain() << ", ";
    os << "outputrawhalf=" << t.getOutputRawHalfs() << ", ";
    os << "hueadjust=" << t.getHueAdjust() << ", ";
    const unsigned long l = t.getLength();
    os << "length=" << l << ", ";
    if (l > 0)
    {
        float rMin = std::numeric_limits<float>::max();
        float gMin = std::numeric_limits<float>::max();
        float bMin = std::numeric_limits<float>::max();
        float rMax = -rMin;
        float gMax = -gMin;
        float bMax = -bMin;
        for (unsigned long i = 0; i < l; ++i)
        {
            float r = 0.f;
            float g = 0.f;
            float b = 0.f;
            t.getValue(i, r, g, b);
            rMin = std::min(rMin, r);
            gMin = std::min(gMin, g);
            bMin = std::min(bMin, b);
            rMax = std::max(rMax, r);
            gMax = std::max(gMax, g);
            bMax = std::max(bMax, b);
        }
        os << "minrgb=[";
        os << rMin << " " << gMin << " " << bMin << "], ";
        os << "maxrgb=[";
        os << rMax << " " << gMax << " " << bMax << "]";
    }
    os << ">";

    return os;
}

} // namespace OCIO_NAMESPACE

