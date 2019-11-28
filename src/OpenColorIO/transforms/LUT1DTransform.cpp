// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/lut1d/Lut1DOpData.h"

namespace OCIO_NAMESPACE
{
LUT1DTransformRcPtr LUT1DTransform::Create()
{
    return LUT1DTransformRcPtr(new LUT1DTransform(), &deleter);
}

LUT1DTransformRcPtr LUT1DTransform::Create(unsigned long length,
                                           bool isHalfDomain)
{
    return LUT1DTransformRcPtr(new LUT1DTransform(length, isHalfDomain), &deleter);
}

void LUT1DTransform::deleter(LUT1DTransform* t)
{
    delete t;
}

class LUT1DTransform::Impl : public Lut1DOpData
{
public:

    Impl()
        : Lut1DOpData(2)
    { }

    Impl(unsigned long length,
         Lut1DOpData::HalfFlags halfFlag)
        : Lut1DOpData(halfFlag, length)
    { }

    ~Impl()
    { }

    Impl& operator = (const Impl & rhs)
    {
        if (this != &rhs)
        {
            Lut1DOpData::operator=(rhs);
            m_direction = rhs.m_direction;
        }
        return *this;
    }

    void reset(unsigned long length)
    {
        auto & lutArray = getArray();
        lutArray = Lut3by1DArray(getHalfFlags(), length);
    }

    TransformDirection m_direction = TRANSFORM_DIR_FORWARD;

private:
    Impl(const Impl & rhs);
};


LUT1DTransform::LUT1DTransform()
    : m_impl(new LUT1DTransform::Impl)
{
}

LUT1DTransform::LUT1DTransform(unsigned long length,
                               bool isHalfDomain)
    : m_impl(new LUT1DTransform::Impl(
        length,
        (Lut1DOpData::HalfFlags)(isHalfDomain ? Lut1DOpData::LUT_INPUT_HALF_CODE :
                                                Lut1DOpData::LUT_STANDARD)))
{
}

LUT1DTransform::~LUT1DTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

LUT1DTransform& LUT1DTransform::operator= (const LUT1DTransform & rhs)
{
    if (this != &rhs)
    {
        *m_impl = *rhs.m_impl;
    }
    return *this;
}

TransformRcPtr LUT1DTransform::createEditableCopy() const
{
    LUT1DTransformRcPtr transform = LUT1DTransform::Create();
    *(transform->m_impl) = *m_impl;
    return transform;
}

void LUT1DTransform::validate() const
{
    try
    {
        Transform::validate();
        getImpl()->validate();
    }
    catch (Exception & ex)
    {
        std::ostringstream oss;
        oss << "LUT1DTransform validation failed: ";
        oss << ex.what();
        throw Exception(oss.str().c_str());
    }
}

TransformDirection LUT1DTransform::getDirection() const
{
    return getImpl()->m_direction;
}

void LUT1DTransform::setDirection(TransformDirection dir)
{
    getImpl()->m_direction = dir;
}

BitDepth LUT1DTransform::getFileOutputBitDepth() const
{
    return getImpl()->getFileOutputBitDepth();
}

void LUT1DTransform::setFileOutputBitDepth(BitDepth bitdepth)
{
    return getImpl()->setFileOutputBitDepth(bitdepth);
}

FormatMetadata & LUT1DTransform::getFormatMetadata()
{
    return m_impl->getFormatMetadata();
}

const FormatMetadata & LUT1DTransform::getFormatMetadata() const
{
    return m_impl->getFormatMetadata();
}

void LUT1DTransform::setLength(unsigned long length)
{
    m_impl->reset(length);
}


namespace
{
void CheckLUT1DIndex(const char * function, 
                     unsigned long index, unsigned long size)
{
    if (index >= size)
    {
        std::ostringstream oss;
        oss << "LUT1DTransform " << function << ": index (";
        oss << index << ") should be less than the length (";
        oss << size << ").";
        throw Exception(oss.str().c_str());
    }
}
}

void LUT1DTransform::setValue(unsigned long index, float r, float g, float b)
{
    CheckLUT1DIndex("setValue", index, getLength());
    m_impl->getArray()[3 * index] = r;
    m_impl->getArray()[3 * index + 1] = g;
    m_impl->getArray()[3 * index + 2] = b;
}

void LUT1DTransform::setInputHalfDomain(bool isHalfDomain)
{
    m_impl->setInputHalfDomain(isHalfDomain);
}

void LUT1DTransform::setOutputRawHalfs(bool isRawHalfs)
{
    m_impl->setOutputRawHalfs(isRawHalfs);
}

void LUT1DTransform::setHueAdjust(LUT1DHueAdjust algo)
{
    m_impl->setHueAdjust(algo);
}

void LUT1DTransform::setInterpolation(Interpolation algo)
{
    m_impl->setInterpolation(algo);
}

unsigned long LUT1DTransform::getLength() const
{
    return m_impl->getArray().getLength();
}

void LUT1DTransform::getValue(unsigned long index, float & r, float & g, float & b) const
{
    CheckLUT1DIndex("getValue", index, getLength());
    r = m_impl->getArray()[3 * index];
    g = m_impl->getArray()[3 * index + 1];
    b = m_impl->getArray()[3 * index + 2];
}

bool LUT1DTransform::getInputHalfDomain() const
{
    return m_impl->isInputHalfDomain();
}

bool LUT1DTransform::getOutputRawHalfs() const
{
    return m_impl->isOutputRawHalfs();
}

LUT1DHueAdjust LUT1DTransform::getHueAdjust() const
{
    return m_impl->getHueAdjust();
}

Interpolation LUT1DTransform::getInterpolation() const
{
    return m_impl->getInterpolation();
}

std::ostream& operator<< (std::ostream& os, const LUT1DTransform& t)
{
    os << "<LUT1DTransform ";
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
            rMax = std::max(rMin, r);
            gMax = std::max(gMin, g);
            bMax = std::max(bMin, b);
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

