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

#include "OpDataIndexMapping.h"

OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace OpData
{
IndexMapping::IndexMapping(unsigned dimension)
    : m_Dimension(dimension)
{
    resize(m_Dimension);
}

IndexMapping::~IndexMapping()
{
}

void IndexMapping::resize(unsigned dimension)
{
    // TODO: Should we try to disallow really large dimensions?
    m_Dimension = dimension;
    // Currently only supporting one index map per LUT.
    m_Indices[0].resize(dimension);
}

unsigned IndexMapping::getDimension() const
{
    return m_Dimension;
}

const IndexMapping::Indices& IndexMapping::getIndices() const
{
    return m_Indices;
}

unsigned IndexMapping::getComponents() const
{
    return 3;
}

void IndexMapping::getPair(unsigned index, float& first, float& second) const
{
    // TODO: Have an assert here to check valid index?
    first  = m_Indices[0][index].first;
    second = m_Indices[0][index].second;
}

void IndexMapping::setPair(unsigned index, float first, float second)
{
    m_Indices[0][index].first  = first;
    m_Indices[0][index].second = second;
}

void IndexMapping::validate() const
{
    // Check that both halves of the index map is increasing.
    // For now we are not validating that the index is within the length
    // of the LUT since the LUT renderer will safely handle that situation.
    float first, second, prevFirst, prevSecond;
    for (unsigned i = 1; i < m_Dimension; ++i)
    {
        getPair(i, first, second);
        getPair(i - 1, prevFirst, prevSecond);
        if (first <= prevFirst || second <= prevSecond)
        {
            throw Exception("Index values must be increasing");
        }
    }
}

bool IndexMapping::operator==(const IndexMapping& other) const
{
    if (this == &other) return true;

    if (m_Dimension != other.m_Dimension) return false;

    for (unsigned i = 0; i < getComponents(); i++)
    {
        if (m_Indices[i] != other.m_Indices[i]) return false;
    }

    return true;
}
}
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "../UnitTest.h"

OIIO_ADD_TEST(OpDataIndexMapping, TestAccessors)
{
    OCIO::OpData::IndexMapping ref(4);
    ref.setPair(0u, 0.f, 0.f);
    ref.setPair(1u, 100.f, 1.f);
    ref.setPair(2u, 200.f, 2.f);
    ref.setPair(3u, 300.f, 3.f);
    ref.validate();

    OIIO_CHECK_EQUAL(ref.getDimension(), 4u);
    float first, second;
    ref.getPair(0u, first, second);
    OIIO_CHECK_EQUAL(first, 0.f);  OIIO_CHECK_EQUAL(second, 0.f);
    ref.getPair(1u, first, second);
    OIIO_CHECK_EQUAL(first, 100.f);  OIIO_CHECK_EQUAL(second, 1.f);
    ref.getPair(2u, first, second);
    OIIO_CHECK_EQUAL(first, 200.f);  OIIO_CHECK_EQUAL(second, 2.f);
    ref.getPair(3u, first, second);
    OIIO_CHECK_EQUAL(first, 300.f);  OIIO_CHECK_EQUAL(second, 3.f);

    ref.resize(8);
    OIIO_CHECK_EQUAL(ref.getDimension(), 8u);

}


OIIO_ADD_TEST(OpDataIndexMapping, RangeValidation)
{
    OCIO::OpData::IndexMapping ref(4);
    ref.setPair(0u, 0.f, 0.f);
    ref.setPair(1u, 100.f, 1.f);
    ref.setPair(2u, 200.f, 2.f);
    ref.setPair(3u, 200.f, 3.f);  // illegal

    OIIO_CHECK_THROW_WHAT(ref.validate(), 
                          OCIO::Exception, 
                          "Index values must be increasing");

    ref.setPair(3u, 300.f, 2.f);  // illegal
    OIIO_CHECK_THROW_WHAT(ref.validate(), 
                          OCIO::Exception, 
                          "Index values must be increasing");
}


OIIO_ADD_TEST(OpDataIndexMapping, Equality)
{
    OCIO::OpData::IndexMapping ref1(4);
    ref1.setPair(0u, 0.f, 0.f);
    ref1.setPair(1u, 100.f, 1.f);
    ref1.setPair(2u, 200.f, 2.f);
    ref1.setPair(3u, 300.f, 3.f);

    OCIO::OpData::IndexMapping ref2(4);
    ref2.setPair(0u, 0.f, 0.f);
    ref2.setPair(1u, 100.f, 1.f);
    ref2.setPair(2u, 200.f, 2.f);
    ref2.setPair(3u, 300.f, 3.f);

    OIIO_CHECK_ASSERT(ref1 == ref2);

    ref2.setPair(2u, 200.f, 2.1f);

    OIIO_CHECK_ASSERT(!(ref1 == ref2));
}

#endif
