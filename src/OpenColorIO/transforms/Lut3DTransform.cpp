// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/Lut3DTransform.h"

namespace OCIO_NAMESPACE
{
Lut3DTransformRcPtr Lut3DTransform::Create()
{
    return Lut3DTransformRcPtr(new Lut3DTransformImpl(), &Lut3DTransformImpl::deleter);
}

Lut3DTransformRcPtr Lut3DTransform::Create(unsigned long gridSize)
{
    return Lut3DTransformRcPtr(new Lut3DTransformImpl(gridSize), &Lut3DTransformImpl::deleter);
}

void Lut3DTransformImpl::deleter(Lut3DTransform* t)
{
    delete static_cast<Lut3DTransformImpl *>(t);
}

Lut3DTransformImpl::Lut3DTransformImpl()
    : m_data(2)
{
}

Lut3DTransformImpl::Lut3DTransformImpl(unsigned long gridSize)
    : m_data(gridSize)
{
}

TransformRcPtr Lut3DTransformImpl::createEditableCopy() const
{
    Lut3DTransformRcPtr transform = Lut3DTransform::Create();
    dynamic_cast<Lut3DTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection Lut3DTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void Lut3DTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void Lut3DTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch (Exception & ex)
    {
        std::string errMsg("Lut3DTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

BitDepth Lut3DTransformImpl::getFileOutputBitDepth() const noexcept
{
    return data().getFileOutputBitDepth();
}

void Lut3DTransformImpl::setFileOutputBitDepth(BitDepth bitDepth) noexcept
{
    data().setFileOutputBitDepth(bitDepth);
}

FormatMetadata & Lut3DTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & Lut3DTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

bool Lut3DTransformImpl::equals(const Lut3DTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const Lut3DTransformImpl*>(&other)->data();
}

unsigned long Lut3DTransformImpl::getGridSize() const
{
    return m_data.getArray().getLength();
}

void Lut3DTransformImpl::setGridSize(unsigned long gridSize)
{
    auto & lutArray = m_data.getArray();
    lutArray = Lut3DOpData::Lut3DArray(gridSize);
}

namespace
{
static constexpr char COMPONENT_R[] = "Red";
static constexpr char COMPONENT_G[] = "Green";
static constexpr char COMPONENT_B[] = "Blue";
void CheckLUT3DIndex(const char * function, 
                     const char * component,
                     unsigned long index, unsigned long size)
{
    if (index >= size)
    {
        std::ostringstream oss;
        oss << "Lut3DTransform " << function << ": " << component << " index (";
        oss << index << ") should be less than the grid size (";
        oss << size << ").";
        throw Exception(oss.str().c_str());
    }
}
}

void Lut3DTransformImpl::setValue(unsigned long indexR,
                                  unsigned long indexG,
                                  unsigned long indexB,
                                  float r, float g, float b)
{
    const unsigned long gs = getGridSize();
    CheckLUT3DIndex("setValue", COMPONENT_R, indexR, gs);
    CheckLUT3DIndex("setValue", COMPONENT_G, indexG, gs);
    CheckLUT3DIndex("setValue", COMPONENT_B, indexB, gs);

    // Array is stored in blue-fastest order.
    const unsigned long arrayIdx = 3 * ((indexR*gs + indexG)*gs + indexB);
    m_data.getArray()[arrayIdx] = r;
    m_data.getArray()[arrayIdx + 1] = g;
    m_data.getArray()[arrayIdx + 2] = b;
}


void Lut3DTransformImpl::getValue(unsigned long indexR,
                                  unsigned long indexG,
                                  unsigned long indexB,
                                  float & r, float & g, float & b) const
{
    const unsigned long gs = getGridSize();
    CheckLUT3DIndex("getValue", COMPONENT_R, indexR, gs);
    CheckLUT3DIndex("getValue", COMPONENT_G, indexG, gs);
    CheckLUT3DIndex("getValue", COMPONENT_B, indexB, gs);

    // Array is stored in blue-fastest order.
    const unsigned long arrayIdx = 3 * ((indexR*gs + indexG)*gs + indexB);
    r = m_data.getArray()[arrayIdx];
    g = m_data.getArray()[arrayIdx + 1];
    b = m_data.getArray()[arrayIdx + 2];
}

void Lut3DTransformImpl::setInterpolation(Interpolation algo)
{
    m_data.setInterpolation(algo);
}

Interpolation Lut3DTransformImpl::getInterpolation() const
{
    return m_data.getInterpolation();
}

std::ostream & operator<< (std::ostream & os, const Lut3DTransform & t)
{
    os << "<Lut3DTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
    os << "fileoutdepth=" << BitDepthToString(t.getFileOutputBitDepth()) << ", ";
    os << "interpolation=" << InterpolationToString(t.getInterpolation()) << ", ";
    const unsigned long l = t.getGridSize();
    os << "gridSize=" << l << ", ";
    if (l > 0)
    {
        float rMin = std::numeric_limits<float>::max();
        float gMin = std::numeric_limits<float>::max();
        float bMin = std::numeric_limits<float>::max();
        float rMax = -rMin;
        float gMax = -gMin;
        float bMax = -bMin;

        for (unsigned long r = 0; r < l; ++r)
        {
            for (unsigned long g = 0; g < l; ++g)
            {
                for (unsigned long b = 0; b < l; ++b)
                {
                    float rv = 0.f;
                    float gv = 0.f;
                    float bv = 0.f;
                    t.getValue(r, g , b, rv, gv, bv);
                    rMin = std::min(rMin, rv);
                    gMin = std::min(gMin, gv);
                    bMin = std::min(bMin, bv);
                    rMax = std::max(rMin, rv);
                    gMax = std::max(gMin, gv);
                    bMax = std::max(bMin, bv);
                }
            }
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

