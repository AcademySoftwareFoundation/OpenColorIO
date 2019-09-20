// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Lut3D/Lut3DOpData.h"

OCIO_NAMESPACE_ENTER
{
LUT3DTransformRcPtr LUT3DTransform::Create()
{
    return LUT3DTransformRcPtr(new LUT3DTransform(), &deleter);
}

LUT3DTransformRcPtr LUT3DTransform::Create(unsigned long gridSize)
{
    return LUT3DTransformRcPtr(new LUT3DTransform(gridSize), &deleter);
}

void LUT3DTransform::deleter(LUT3DTransform* t)
{
    delete t;
}

class LUT3DTransform::Impl : public Lut3DOpData
{
public:

    Impl()
        : Lut3DOpData(2)
    { }

    Impl(unsigned long gridSize)
        : Lut3DOpData(gridSize)
    { }

    ~Impl()
    { }

    Impl& operator = (const Impl & rhs)
    {
        if (this != &rhs)
        {
            Lut3DOpData::operator=(rhs);
            m_direction = rhs.m_direction;
        }
        return *this;
    }

    void reset(unsigned long gridSize)
    {
        auto & lutArray = getArray();
        lutArray = Lut3DArray(gridSize, getOutputBitDepth());
    }

    TransformDirection m_direction = TRANSFORM_DIR_FORWARD;

private:
    Impl(const Impl & rhs);
};


LUT3DTransform::LUT3DTransform()
    : m_impl(new LUT3DTransform::Impl)
{
}

LUT3DTransform::LUT3DTransform(unsigned long gridSize)
    : m_impl(new LUT3DTransform::Impl(gridSize))
{
}

LUT3DTransform::~LUT3DTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

LUT3DTransform& LUT3DTransform::operator= (const LUT3DTransform & rhs)
{
    if (this != &rhs)
    {
        *m_impl = *rhs.m_impl;
    }
    return *this;
}

TransformRcPtr LUT3DTransform::createEditableCopy() const
{
    LUT3DTransformRcPtr transform = LUT3DTransform::Create();
    *(transform->m_impl) = *m_impl;
    return transform;
}

void LUT3DTransform::validate() const
{
    try
    {
        Transform::validate();
        getImpl()->validate();
    }
    catch (Exception & ex)
    {
        std::ostringstream oss;
        oss << "LUT3DTransform validation failed: ";
        oss << ex.what();
        throw Exception(oss.str().c_str());
    }
}

TransformDirection LUT3DTransform::getDirection() const
{
    return getImpl()->m_direction;
}

void LUT3DTransform::setDirection(TransformDirection dir)
{
    getImpl()->m_direction = dir;
}

BitDepth LUT3DTransform::getFileOutputBitDepth() const
{
    return getImpl()->getFileOutputBitDepth();
}

void LUT3DTransform::setFileOutputBitDepth(BitDepth bitdepth)
{
    return getImpl()->setFileOutputBitDepth(bitdepth);
}

FormatMetadata & LUT3DTransform::getFormatMetadata()
{
    return m_impl->getFormatMetadata();
}

const FormatMetadata & LUT3DTransform::getFormatMetadata() const
{
    return m_impl->getFormatMetadata();
}

void LUT3DTransform::setGridSize(unsigned long gridSize)
{
    m_impl->reset(gridSize);
}

namespace
{
static constexpr const char * COMPONENT_R = "Red";
static constexpr const char * COMPONENT_G = "Green";
static constexpr const char * COMPONENT_B = "Blue";
void CheckLUT3DIndex(const char * function, 
                     const char * component,
                     unsigned long index, unsigned long size)
{
    if (index >= size)
    {
        std::ostringstream oss;
        oss << "LUT3DTransform " << function << ": " << component << " index (";
        oss << index << ") should be less than the grid size (";
        oss << size << ").";
        throw Exception(oss.str().c_str());
    }
}
}

void LUT3DTransform::setValue(unsigned long indexR,
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
    m_impl->getArray()[arrayIdx] = r;
    m_impl->getArray()[arrayIdx + 1] = g;
    m_impl->getArray()[arrayIdx + 2] = b;
}

void LUT3DTransform::setInterpolation(Interpolation algo)
{
    m_impl->setInterpolation(algo);
}

unsigned long LUT3DTransform::getGridSize() const
{
    return m_impl->getArray().getLength();
}

void LUT3DTransform::getValue(unsigned long indexR,
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
    r = m_impl->getArray()[arrayIdx];
    g = m_impl->getArray()[arrayIdx + 1];
    b = m_impl->getArray()[arrayIdx + 2];
}

Interpolation LUT3DTransform::getInterpolation() const
{
    return m_impl->getInterpolation();
}

std::ostream& operator<< (std::ostream& os, const LUT3DTransform& t)
{
    os << "<LUT3DTransform ";
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

}
OCIO_NAMESPACE_EXIT

////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OCIO_ADD_TEST(LUT3DTransform, basic)
{
    const OCIO::LUT3DTransformRcPtr lut = OCIO::LUT3DTransform::Create();

    OCIO_CHECK_EQUAL(lut->getGridSize(), 2);
    OCIO_CHECK_EQUAL(lut->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    lut->getValue(0, 0, 0, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 0.f);
    OCIO_CHECK_EQUAL(b, 0.f);
    lut->getValue(0, 1, 1, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 1.f);
    OCIO_CHECK_EQUAL(b, 1.f);
    lut->getValue(1, 0, 0, r, g, b);
    OCIO_CHECK_EQUAL(r, 1.f);
    OCIO_CHECK_EQUAL(g, 0.f);
    OCIO_CHECK_EQUAL(b, 0.f);

    lut->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(lut->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    lut->setGridSize(3);
    OCIO_CHECK_EQUAL(lut->getGridSize(), 3);
    lut->getValue(0, 0, 0, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 0.f);
    OCIO_CHECK_EQUAL(b, 0.f);
    lut->getValue(0, 1, 1, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 0.5f);
    OCIO_CHECK_EQUAL(b, 0.5f);
    lut->getValue(2, 0, 2, r, g, b);
    OCIO_CHECK_EQUAL(r, 1.f);
    OCIO_CHECK_EQUAL(g, 0.f);
    OCIO_CHECK_EQUAL(b, 1.f);

    lut->getValue(0, 1, 2, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 0.5f);
    OCIO_CHECK_EQUAL(b, 1.f);

    r = 0.1f;
    g = 0.52f;
    b = 0.93f;
    lut->setValue(0, 1, 2, r, g, b);
    
    r = 0.f;
    g = 0.f;
    b = 0.f;
    lut->getValue(0, 1, 2, r, g, b);

    OCIO_CHECK_EQUAL(r, 0.1f);
    OCIO_CHECK_EQUAL(g, 0.52f);
    OCIO_CHECK_EQUAL(b, 0.93f);

    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);

    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    // File out bit-depth does not affect values.
    r = 0.f;
    g = 0.f;
    b = 0.f;
    lut->getValue(0, 1, 2, r, g, b);

    OCIO_CHECK_EQUAL(r, 0.1f);
    OCIO_CHECK_EQUAL(g, 0.52f);
    OCIO_CHECK_EQUAL(b, 0.93f);

    OCIO_CHECK_THROW_WHAT(lut->setValue(3, 1, 1, 0.f, 0.f, 0.f), OCIO::Exception,
                          "should be less than the grid size");
    OCIO_CHECK_THROW_WHAT(lut->getValue(0, 0, 4, r, g, b), OCIO::Exception,
                          "should be less than the grid size");

    OCIO_CHECK_THROW_WHAT(lut->setGridSize(200), OCIO::Exception,
                          "must not be greater than '129'");

    OCIO_CHECK_NO_THROW(lut->validate());

    lut->setValue(0, 0, 0, -0.2f, -0.1f, -0.3f);
    lut->setValue(2, 2, 2, 1.2f, 1.3f, 1.8f);

    std::ostringstream oss;
    oss << *lut;
    OCIO_CHECK_EQUAL(oss.str(), "<LUT3DTransform direction=inverse, fileoutdepth=8ui,"
        " interpolation=default, gridSize=3, minrgb=[-0.2 -0.1 -0.3], maxrgb=[1.2 1.3 1.8]>");
}

OCIO_ADD_TEST(LUT3DTransform, create_with_parameters)
{
    const auto lut = OCIO::LUT3DTransform::Create(8);

    OCIO_CHECK_EQUAL(lut->getGridSize(), 8);
    OCIO_CHECK_EQUAL(lut->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(lut->getInterpolation(), OCIO::INTERP_DEFAULT);

    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    lut->getValue(7, 7, 7, r, g, b);
    OCIO_CHECK_EQUAL(r, 1.0f);
    OCIO_CHECK_EQUAL(g, 1.0f);
    OCIO_CHECK_EQUAL(b, 1.0f);
}

#endif