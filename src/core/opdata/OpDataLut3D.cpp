/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <OpenColorIO/OpenColorIO.h>

#include "../Platform.h"
#include "../BitDepthUtils.h"
#include "../MathUtils.h"

#include "OpDataLut3D.h"
#include "OpDataInvLut3D.h"
#include "OpDataRange.h"
#include "OpDataTools.h"

#include <sstream>


OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace OpData
{
unsigned Lut3D::maxSupportedLength = 129;

Interpolation Lut3D::getInterpolation(const char* str)
{
    if (str && *str)
    {
        if (0 == strcmp(str, "trilinear"))
        {
            return INTERP_LINEAR;
        }
        else if (0 == strcmp(str, "tetrahedral"))
        {
            return INTERP_TETRAHEDRAL;
        }
        else if (0 == strcmp(str, "4pt tetrahedral"))
        {
            return INTERP_TETRAHEDRAL;
        }
        else if (0 == strcmp(str, "default"))
        {
            return INTERP_DEFAULT;
        }

        std::ostringstream oss;
        oss << "3D LUT interpolation not recongnized: ";
        oss << str;

        throw Exception(oss.str().c_str());
    }

    throw Exception("3D LUT missing interpolation value.");
}

Lut3D::Lut3DArray::Lut3DArray(unsigned length,
                                BitDepth outBitDepth)
{
    resize(length, getMaxColorComponents());
    fill(outBitDepth);
}

Lut3D::Lut3DArray::~Lut3DArray()
{
}

Lut3D::Lut3DArray& Lut3D::Lut3DArray::operator=(const Array& a)
{
    if (this != &a)
    {
        *(Array*)this = a;
    }
    return *this;
}

void Lut3D::Lut3DArray::fill(BitDepth outBitDepth)
{
    const unsigned length = getLength();
    const unsigned maxChannels = getMaxColorComponents();

    Array::Values& values = getValues();

    const float stepValue 
        = GetBitDepthMaxValue(outBitDepth) / ((float)length - 1.0f);

    const unsigned maxEntries = length*length*length;

    for (unsigned idx = 0; idx<maxEntries; idx++)
    {
        values[maxChannels*idx + 0] = (float)((idx / length / length) % length) * stepValue;
        values[maxChannels*idx + 1] = (float)((idx / length) % length) * stepValue;
        values[maxChannels*idx + 2] = (float)(idx%length) * stepValue;
    }
}

bool Lut3D::Lut3DArray::isIdentity(BitDepth outBitDepth) const
{
    const unsigned length = getLength();
    const Array::Values & values = getValues();

    // As an identity LUT shall not change color component values,
    // aside from possibly a scaling for bit-depth conversion.

    const float stepSize = GetValueStepSize(outBitDepth, length);

    const unsigned maxEntries = length*length*length;

    for(unsigned i=0; i<maxEntries; i++)
    {
        // TODO: Use equalWithSafeRelError to account for outBitDepth
        if(!equalWithAbsError(values[3*i+0],
                                (float)((i/ length / length)% length) * stepSize,
                                0.0001f))
        {
            return false;
        }

        if(!equalWithAbsError(values[3*i+1],
                                (float)((i/ length)% length) * stepSize,
                                0.0001f))
        {
            return false;
        }

        if(!equalWithAbsError(values[3*i+2],
                                (float)(i%length) * stepSize,
                                0.0001f))
        {
            return false;
        }
    }
    return true;
}

unsigned Lut3D::Lut3DArray::getNumValues() const
{
    const unsigned numEntries = getLength()*getLength()*getLength();
    return numEntries*getMaxColorComponents();
}

void Lut3D::Lut3DArray::getRGB(unsigned i, unsigned j, unsigned k, float* RGB) const
{
    const unsigned length = getLength();
    const unsigned maxChannels = getMaxColorComponents();
    const Array::Values& values = getValues();
    // Array order matches ctf order: channels vary most rapidly, then B, G, R.
    unsigned offset = (i*length*length + j*length + k) * maxChannels;
    RGB[0] = values[offset];
    RGB[1] = values[++offset];
    RGB[2] = values[++offset];
}

void Lut3D::Lut3DArray::setRGB(unsigned i, unsigned j, unsigned k, float* RGB)
{
    const unsigned length = getLength();
    const unsigned maxChannels = getMaxColorComponents();
    Array::Values& values = getValues();
    // Array order matches ctf order: channels vary most rapidly, then B, G, R.
    unsigned offset = (i*length*length + j*length + k) * maxChannels;
    values[offset] = RGB[0];
    values[++offset] = RGB[1];
    values[++offset] = RGB[2];
}

Lut3D::Lut3D(unsigned gridSize)
    :   OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    ,   m_interpolation(INTERP_DEFAULT)
    ,   m_array(gridSize, getOutputBitDepth())
{
}

Lut3D::Lut3D(BitDepth inBitDepth,
                BitDepth outBitDepth,
                const std::string& id,
                const std::string& name,
                const Descriptions& descriptions,
                Interpolation interpolation,
                unsigned gridSize)
    :   OpData(inBitDepth, outBitDepth, id, name, descriptions)
    ,   m_interpolation(interpolation)
    ,   m_array(gridSize, getOutputBitDepth())
{
}

Lut3D::~Lut3D()
{
}

const std::string& Lut3D::getOpTypeName() const
{
    static const std::string type("LUT 3D");
    return type;
}

void Lut3D::setInterpolation(Interpolation algo)
{
    m_interpolation = algo;
}

Interpolation Lut3D::getConcreteInterpolation() const
{
    if (m_interpolation == INTERP_BEST
        || m_interpolation == INTERP_TETRAHEDRAL)
    {
        return INTERP_TETRAHEDRAL;
    }
    if (m_interpolation == INTERP_DEFAULT
        || m_interpolation == INTERP_LINEAR)
    {
        return INTERP_LINEAR;
    }

    // TODO: currently INTERP_NEAREST is not implemented.
    // This is a regression for OCIO v1.
    if (m_interpolation == INTERP_NEAREST)
    {
        return INTERP_LINEAR;
    }
            
    return m_interpolation;
}

void Lut3D::validate() const
{
    if(getInputBitDepth()==BIT_DEPTH_UNKNOWN)
    {
        throw Exception("Unknown bit depth");
    }

    if(getOutputBitDepth()==BIT_DEPTH_UNKNOWN)
    {
        throw Exception("Unknown bit depth");
    }

    Interpolation interp = getConcreteInterpolation();
    if (interp != INTERP_LINEAR && interp != INTERP_TETRAHEDRAL)
    {
        const std::string i = InterpolationToString(getInterpolation());
        std::string err = "Unsupported interpolation algorithm: " + i;
        throw Exception(err.c_str());
    }

    OpData::validate();

    try
    {
        getArray().validate();
    }
    catch (Exception& e)
    {
        std::ostringstream oss;
        oss << "Lut3D content array issue: ";
        oss << e.what();

        throw Exception(oss.str().c_str());
    }

    if (getArray().getNumColorComponents() != 3)
    {
        throw Exception("Lut3D has an incorrect number of color components. ");
    }

    if (getArray().getLength()>maxSupportedLength)
    {
        std::ostringstream oss;
        oss << "Lut3D length: ";
        oss << getArray().getLength();
        oss << " is not supported. ";

        throw Exception(oss.str().c_str());
    }
}

bool Lut3D::isIdentity() const
{
    return m_array.isIdentity(getOutputBitDepth());
}

void Lut3D::setOutputBitDepth(BitDepth out) 
{
    // Scale factor is max_new_depth/max_old_depth
    const float scaleFactor
        = GetBitDepthMaxValue(out)
            / GetBitDepthMaxValue(getOutputBitDepth());

    // Call parent to set the outputbitdepth
    OpData::setOutputBitDepth(out);

    // Scale array by scaleFactor,
    // don't scale if scaleFactor = 1.0f
    if (!equalWithAbsError(scaleFactor, 1.0f, 0.00001f))
    {
        Array::Values& arrayVals = m_array.getValues();
        const size_t size = arrayVals.size();

        for (unsigned i = 0; i < size; i++)
        {
            arrayVals[i] *= scaleFactor;
        }
    }
}

std::auto_ptr<OpData> Lut3D::getIdentityReplacement() const
{
    const BitDepth inBD = getInputBitDepth();
    const BitDepth outBD = getOutputBitDepth();

    return 
        std::auto_ptr<OpData>(new Range(inBD,
                                        outBD,
                                        0.,
                                        GetBitDepthMaxValue(inBD),
                                        0.,
                                        GetBitDepthMaxValue(outBD)));
}

bool Lut3D::haveEqualBasics(const Lut3D & B) const
{
    // Question:  Should interpolation style be considered?
    return (m_array == B.m_array);
}

bool Lut3D::operator==(const OpData & other) const
{
    if (this == &other) return true;

    if (getOpType() != other.getOpType()) return false;

    const Lut3D* lop = static_cast<const Lut3D*>(&other);

    if ( ! haveEqualBasics(*lop) ) return false;

    return (OpData::operator==(other) &&
            m_interpolation == lop->m_interpolation);
}

OpData * Lut3D::clone(CloneType /*type*/) const
{
    return new Lut3D(*this);
}

void Lut3D::inverse(OpDataVec & ops) const
{
    std::auto_ptr<InvLut3D> invOp(new InvLut3D(*this));
    ops.append(invOp.release());
}

bool Lut3D::isInverse(const Lut3D& B) const
{
    if (getOpType() == OpData::Lut3DType && B.getOpType() == OpData::InvLut3DType)
    {
        // Need to check bit-depth because the array scaling is relative to it.
        // (For LUT it is the out-depth, for INVLUT it is the in-depth.)
        // Note that we use MaxValue so that 16f and 32f are considered the same.

        // In the LUT-->INVLUT case this will typically be true anyway.
        if (GetBitDepthMaxValue(getOutputBitDepth()) !=
            GetBitDepthMaxValue(B.getInputBitDepth()))
            return false;

        // Test the core parts such as array, half domain, and hue adjust while
        // ignoring superficial differences such as in/out bit-depth.
        return haveEqualBasics(B);
    }
    else if (getOpType() == OpData::InvLut3DType && B.getOpType() == OpData::Lut3DType)
    {
        // TODO: Harmonize array bit-depths to allow a proper array comparison.

        // In the INVLUT-->LUT case this could easily not be true even if the
        // pair are actually inverses.
        if (GetBitDepthMaxValue(getInputBitDepth()) !=
            GetBitDepthMaxValue(B.getOutputBitDepth()))
            return false;

        return haveEqualBasics(B);
    }
    return false;
}

} // exit OpData namespace
}
OCIO_NAMESPACE_EXIT



#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "../UnitTest.h"


OIIO_ADD_TEST(OpDataLut3D, TestEmpty)
{
    OCIO::OpData::Lut3D l(2);
    OIIO_CHECK_NO_THROW(l.validate());
    OIIO_CHECK_ASSERT(l.getOpTypeName() == "LUT 3D");
    OIIO_CHECK_ASSERT(l.isIdentity());
    OIIO_CHECK_ASSERT(l.isClamping());
    OIIO_CHECK_ASSERT(!l.isNoOp());
}

OIIO_ADD_TEST(OpDataLut3D, TestAccessors)
{
    OCIO::Interpolation interpol = OCIO::INTERP_LINEAR;

    OCIO::OpData::Lut3D l(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
        "uid", "name", OCIO::OpData::Descriptions(), interpol, 33);

    OIIO_CHECK_EQUAL(l.getInterpolation(), interpol);
    OIIO_CHECK_ASSERT(l.isIdentity());

    OIIO_CHECK_NO_THROW(l.validate());

    l.getArray()[0] = 1.0f;

    OIIO_CHECK_ASSERT(!l.isIdentity());
    OIIO_CHECK_NO_THROW(l.validate());

    interpol = OCIO::INTERP_TETRAHEDRAL;
    l.setInterpolation(interpol);
    OIIO_CHECK_EQUAL(l.getInterpolation(), interpol);

    OIIO_CHECK_EQUAL(l.getArray().getLength(), (unsigned)33);
    OIIO_CHECK_EQUAL(l.getArray().getNumValues(), (unsigned)33 * 33 * 33 * 3);
    OIIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), (unsigned)3);

    l.getArray().resize(17, 3);

    OIIO_CHECK_EQUAL(l.getArray().getLength(), (unsigned)17);
    OIIO_CHECK_EQUAL(l.getArray().getNumValues(), (unsigned)17 * 17 * 17 * 3);
    OIIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), (unsigned)3);
    OIIO_CHECK_NO_THROW(l.validate());
}

OIIO_ADD_TEST(OpDataLut3D, TestDiffBitdepth)
{
    OCIO::Interpolation interpol = OCIO::INTERP_LINEAR;
    OCIO::OpData::Lut3D l1(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8,
        "uid", "name", OCIO::OpData::Descriptions(), interpol, 33);
    OCIO::OpData::Lut3D l2(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10,
        "uid", "name", OCIO::OpData::Descriptions(), interpol, 33);

    OIIO_CHECK_NO_THROW(l1.validate());
    OIIO_CHECK_NO_THROW(l2.validate());

    const float coeff
        = OCIO::GetBitDepthMaxValue(l2.getOutputBitDepth())
            / OCIO::GetBitDepthMaxValue(l2.getInputBitDepth());

    // To validate the result, compute the expected results
    // not using the Lut3DOp algorithm
    // TODO: verify testing function used
    const float error = 1e-4f;
    for (unsigned idx = 0; idx<l2.getArray().getLength(); ++idx)
    {
        OIIO_CHECK_CLOSE(l1.getArray().getValues()[idx * 3 + 0] * coeff,
                         l2.getArray().getValues()[idx * 3 + 0],
                         error);
        OIIO_CHECK_CLOSE(l1.getArray().getValues()[idx * 3 + 1] * coeff,
                         l2.getArray().getValues()[idx * 3 + 1],
                         error);
        OIIO_CHECK_CLOSE(l1.getArray().getValues()[idx * 3 + 2] * coeff,
                         l2.getArray().getValues()[idx * 3 + 2],
                         error);
    }
}

OIIO_ADD_TEST(OpDataLut3D, Clone)
{
    OCIO::OpData::Lut3D ref(33);
    ref.getArray()[1] = 0.1f;

    std::auto_ptr< OCIO::OpData::Lut3D> pClone(
       dynamic_cast< OCIO::OpData::Lut3D*>(
            ref.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY)));

    OIIO_CHECK_ASSERT(pClone.get());
    OIIO_CHECK_ASSERT(!pClone->isNoOp());
    OIIO_CHECK_ASSERT(!pClone->isIdentity());
    OIIO_CHECK_NO_THROW(pClone->validate());
    OIIO_CHECK_EQUAL(pClone->getOpTypeName(), "LUT 3D");
    OIIO_CHECK_ASSERT(pClone->getArray()==ref.getArray());
}

OIIO_ADD_TEST(OpDataLut3D, NotSupportedLength)
{
    const OCIO::OpData::Lut3D
        ref1(OCIO::OpData::Lut3D::maxSupportedLength);

    OIIO_CHECK_NO_THROW(ref1.validate());

    const OCIO::OpData::Lut3D
        ref2(OCIO::OpData::Lut3D::maxSupportedLength + 1);

    OIIO_CHECK_THROW_WHAT(ref2.validate(), OCIO::Exception, "is not supported");

}

OIIO_ADD_TEST(OpDataLut3D, OuputDepthScaling)
{
    OCIO::Interpolation interpol = OCIO::INTERP_LINEAR;
    OCIO::OpData::Lut3D ref(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10,
        "uid", "name", OCIO::OpData::Descriptions(), interpol, 33);

    //get the array values and keep them to compare later
    const OCIO::OpData::Array::Values 
        initialArrValues = ref.getArray().getValues();
    const OCIO::BitDepth initialBitdepth = ref.getOutputBitDepth();

    const OCIO::BitDepth newBitdepth = OCIO::BIT_DEPTH_UINT16;

    const float factor
        = OCIO::GetBitDepthMaxValue(newBitdepth)
            / OCIO::GetBitDepthMaxValue(initialBitdepth);

    ref.setOutputBitDepth(newBitdepth);
    // now we need to make sure that the bitdepth was changed from the overriden
    // method.
    OIIO_CHECK_EQUAL(ref.getOutputBitDepth(), newBitdepth);

    // now we need to check if the scaling between the new array and initial array
    // matches the factor computed above.
    const OCIO::OpData::Array::Values& newArrValues = ref.getArray().getValues();

    // sanity check first
    OIIO_CHECK_EQUAL(initialArrValues.size(), newArrValues.size());

    float expectedValue = 0.0f;

    // TODO: verify testing function used
    for (unsigned i = 0; i < newArrValues.size(); i++)
    {
        expectedValue = initialArrValues[i] * factor;
        OIIO_CHECK_ASSERT(OCIO::equalWithAbsError(expectedValue,
                                                  newArrValues[i],
                                                  0.0001f));
    }
}

OIIO_ADD_TEST(OpDataLut3D, Equality)
{
    OCIO::OpData::Lut3D l1(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                           "", "", OCIO::OpData::Descriptions(), 
                           OCIO::INTERP_LINEAR, 33);

    OCIO::OpData::Lut3D l2(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                           "", "", OCIO::OpData::Descriptions(), 
                           OCIO::INTERP_BEST, 33);  

    OIIO_CHECK_ASSERT(!(l1 == l2));

    OCIO::OpData::Lut3D l3(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32,
                           "", "", OCIO::OpData::Descriptions(), 
                           OCIO::INTERP_LINEAR, 33);

    OIIO_CHECK_ASSERT(!(l1 == l3) && !(l2 == l3));

    OCIO::OpData::Lut3D l4(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                           "", "", OCIO::OpData::Descriptions(),
                           OCIO::INTERP_LINEAR, 33);  

    OIIO_CHECK_ASSERT(l1 == l4);
}


OIIO_ADD_TEST(OpDataLut3D, Interpolation)
{
    OCIO::OpData::Lut3D l(2);
    l.setInputBitDepth(OCIO::BIT_DEPTH_F32);
    l.setOutputBitDepth(OCIO::BIT_DEPTH_F32);
    
    l.setInterpolation(OCIO::INTERP_LINEAR);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_DEFAULT);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_DEFAULT);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_BEST);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_BEST);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_NO_THROW(l.validate());

    // TODO: INTERP_NEAREST is currently implemented as INTERP_LINEAR
    l.setInterpolation(OCIO::INTERP_NEAREST);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_NEAREST);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(l.validate());

    // Invalid interpolation type duo not get translated
    // by getConcreteInterpolation
    l.setInterpolation(OCIO::INTERP_UNKNOWN);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_UNKNOWN);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_UNKNOWN);
    OIIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "Unsupported interpolation algorithm");

    l.setInterpolation(OCIO::INTERP_CUBIC);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_CUBIC);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_CUBIC);
    OIIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "Unsupported interpolation algorithm");
}

#endif

