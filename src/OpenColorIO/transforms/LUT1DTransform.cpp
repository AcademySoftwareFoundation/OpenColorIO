// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Lut1D/Lut1DOpData.h"

OCIO_NAMESPACE_ENTER
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
        : Lut1DOpData(BIT_DEPTH_F32, BIT_DEPTH_F32,
                      FormatMetadataImpl(METADATA_ROOT),
                      INTERP_DEFAULT, halfFlag, length)
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
        lutArray = Lut3by1DArray(BIT_DEPTH_F32, getHalfFlags(), length);
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

}
OCIO_NAMESPACE_EXIT

////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OCIO_ADD_TEST(LUT1DTransform, basic)
{
    const OCIO::LUT1DTransformRcPtr lut = OCIO::LUT1DTransform::Create();

    OCIO_CHECK_EQUAL(lut->getLength(), 2);
    OCIO_CHECK_EQUAL(lut->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(lut->getHueAdjust(), OCIO::HUE_NONE);
    OCIO_CHECK_EQUAL(lut->getInputHalfDomain(), false);
    OCIO_CHECK_EQUAL(lut->getOutputRawHalfs(), false);
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    lut->getValue(0, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 0.f);
    OCIO_CHECK_EQUAL(b, 0.f);
    lut->getValue(1, r, g, b);
    OCIO_CHECK_EQUAL(r, 1.f);
    OCIO_CHECK_EQUAL(g, 1.f);
    OCIO_CHECK_EQUAL(b, 1.f);

    lut->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(lut->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    lut->setLength(3);
    OCIO_CHECK_EQUAL(lut->getLength(), 3);
    lut->getValue(0, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 0.f);
    OCIO_CHECK_EQUAL(b, 0.f);
    lut->getValue(1, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.5f);
    OCIO_CHECK_EQUAL(g, 0.5f);
    OCIO_CHECK_EQUAL(b, 0.5f);
    lut->getValue(2, r, g, b);
    OCIO_CHECK_EQUAL(r, 1.f);
    OCIO_CHECK_EQUAL(g, 1.f);
    OCIO_CHECK_EQUAL(b, 1.f);

    r = 0.51f;
    g = 0.52f;
    b = 0.53f;
    lut->setValue(1, r, g, b);

    r = 0.f;
    g = 0.f;
    b = 0.f;
    lut->getValue(1, r, g, b);

    OCIO_CHECK_EQUAL(r, 0.51f);
    OCIO_CHECK_EQUAL(g, 0.52f);
    OCIO_CHECK_EQUAL(b, 0.53f);

    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);

    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    // File out bit-depth does not affect values.
    r = 0.f;
    g = 0.f;
    b = 0.f;
    lut->getValue(1, r, g, b);

    OCIO_CHECK_EQUAL(r, 0.51f);
    OCIO_CHECK_EQUAL(g, 0.52f);
    OCIO_CHECK_EQUAL(b, 0.53f);

    OCIO_CHECK_NO_THROW(lut->validate());

    OCIO_CHECK_THROW_WHAT(lut->setValue(3, 0.f, 0.f, 0.f), OCIO::Exception,
                          "should be less than the length");
    OCIO_CHECK_THROW_WHAT(lut->getValue(3, r, g, b), OCIO::Exception,
                          "should be less than the length");

    lut->setInputHalfDomain(true);
    OCIO_CHECK_THROW_WHAT(lut->validate(), OCIO::Exception,
                          "65536 required for halfDomain 1D LUT");

    OCIO_CHECK_THROW_WHAT(lut->setLength(1024*1024+1), OCIO::Exception,
                          "must not be greater than");

    lut->setInputHalfDomain(false);
    lut->setValue(0, -0.2f, 0.1f, -0.3f);
    lut->setValue(2, 1.2f, 1.3f, 0.8f);

    std::ostringstream oss;
    oss << *lut;
    OCIO_CHECK_EQUAL(oss.str(), "<LUT1DTransform direction=inverse, fileoutdepth=8ui,"
        " interpolation=default, inputhalf=0, outputrawhalf=0, hueadjust=0,"
        " length=3, minrgb=[-0.2 0.1 -0.3], maxrgb=[1.2 1.3 0.8]>");
}

OCIO_ADD_TEST(LUT1DTransform, create_with_parameters)
{
    const auto lut0 = OCIO::LUT1DTransform::Create(65536, true);

    OCIO_CHECK_EQUAL(lut0->getLength(), 65536);
    OCIO_CHECK_EQUAL(lut0->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(lut0->getHueAdjust(), OCIO::HUE_NONE);
    OCIO_CHECK_EQUAL(lut0->getInputHalfDomain(), true);
    OCIO_CHECK_NO_THROW(lut0->validate());

    const auto lut1 = OCIO::LUT1DTransform::Create(10, true);

    OCIO_CHECK_EQUAL(lut1->getLength(), 10);
    OCIO_CHECK_EQUAL(lut1->getInputHalfDomain(), true);
    OCIO_CHECK_THROW_WHAT(lut1->validate(), OCIO::Exception,
                          "65536 required for halfDomain 1D LUT");

    const auto lut2 = OCIO::LUT1DTransform::Create(8, false);

    OCIO_CHECK_EQUAL(lut2->getLength(), 8);
    OCIO_CHECK_EQUAL(lut2->getInputHalfDomain(), false);
    OCIO_CHECK_NO_THROW(lut2->validate());
}

#endif