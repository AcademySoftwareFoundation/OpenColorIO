/*
Copyright (c) 2018 Autodesk Inc., et al.
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

#include <sstream>

#include "ops/IndexMapping.h"

OCIO_NAMESPACE_ENTER
{

IndexMapping::IndexMapping(size_t dimension)
    : m_dimension(dimension)
{
    resize(m_dimension);
}

IndexMapping::~IndexMapping()
{
}

void IndexMapping::resize(size_t dimension)
{
    // TODO: Should we try to disallow really large dimensions?
    m_dimension = dimension;
    // Currently only supporting one index map per LUT.
    m_indices[0].resize(dimension);
}

size_t IndexMapping::getDimension() const
{
    return m_dimension;
}

const IndexMapping::Indices & IndexMapping::getIndices() const
{
    return m_indices;
}

unsigned int IndexMapping::getNumComponents() const
{
    return 3;
}

void IndexMapping::validateIndex(size_t index) const
{
    if (index >= m_indices[0].size())
    {
        std::ostringstream oss;
        oss << "IndexMapping: Index ";
        oss << index << " is invalid. Should be less than ";
        oss << m_indices[0].size() << ".";
        throw Exception(oss.str().c_str());
    }
}

void IndexMapping::getPair(size_t index, float & first, float & second) const
{
    validateIndex(index);

    first  = m_indices[0][index].first;
    second = m_indices[0][index].second;
}

void IndexMapping::setPair(size_t index, float first, float second)
{
    validateIndex(index);

    m_indices[0][index].first  = first;
    m_indices[0][index].second = second;
}

void IndexMapping::validate() const
{
    // Check that both halves of the index map is increasing.
    // For now we are not validating that the index is within the length
    // of the LUT since the LUT renderer will safely handle that situation.
    float first, second, prevFirst, prevSecond;
    for (unsigned i = 1; i < m_dimension; ++i)
    {
        getPair(i, first, second);
        getPair(i - 1, prevFirst, prevSecond);
        if (first <= prevFirst || second <= prevSecond)
        {
            throw Exception("Index values must be increasing.");
        }
    }
}

bool IndexMapping::operator==(const IndexMapping & other) const
{
    if (this == &other) return true;

    if (m_dimension != other.m_dimension) return false;

    for (unsigned i = 0; i < getNumComponents(); i++)
    {
        if (m_indices[i] != other.m_indices[i]) return false;
    }

    return true;
}

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OCIO_ADD_TEST(IndexMapping, test_accessors)
{
    OCIO::IndexMapping ref(4);
    ref.setPair(0u, 0.f, 0.f);
    ref.setPair(1u, 100.f, 1.f);
    ref.setPair(2u, 200.f, 2.f);
    ref.setPair(3u, 300.f, 3.f);

    OCIO_CHECK_THROW_WHAT(ref.setPair(5u, 300.f, 3.f),
                          OCIO::Exception,
                          "invalid. Should be less than");

    ref.validate();

    OCIO_CHECK_EQUAL(ref.getDimension(), 4u);
    float first, second;
    ref.getPair(0u, first, second);
    OCIO_CHECK_EQUAL(first, 0.f);  OCIO_CHECK_EQUAL(second, 0.f);
    ref.getPair(1u, first, second);
    OCIO_CHECK_EQUAL(first, 100.f);  OCIO_CHECK_EQUAL(second, 1.f);
    ref.getPair(2u, first, second);
    OCIO_CHECK_EQUAL(first, 200.f);  OCIO_CHECK_EQUAL(second, 2.f);
    ref.getPair(3u, first, second);
    OCIO_CHECK_EQUAL(first, 300.f);  OCIO_CHECK_EQUAL(second, 3.f);

    ref.resize(8);
    OCIO_CHECK_EQUAL(ref.getDimension(), 8u);

}

OCIO_ADD_TEST(IndexMapping, range_validation)
{
    OCIO::IndexMapping ref(4);
    ref.setPair(0u, 0.f, 0.f);
    ref.setPair(1u, 100.f, 1.f);
    ref.setPair(2u, 200.f, 2.f);
    ref.setPair(3u, 200.f, 3.f);  // illegal

    OCIO_CHECK_THROW_WHAT(ref.validate(), 
                          OCIO::Exception, 
                          "Index values must be increasing");

    ref.setPair(3u, 300.f, 2.f);  // illegal
    OCIO_CHECK_THROW_WHAT(ref.validate(), 
                          OCIO::Exception, 
                          "Index values must be increasing");
}

OCIO_ADD_TEST(IndexMapping, equality)
{
    OCIO::IndexMapping ref1(4);
    ref1.setPair(0u, 0.f, 0.f);
    ref1.setPair(1u, 100.f, 1.f);
    ref1.setPair(2u, 200.f, 2.f);
    ref1.setPair(3u, 300.f, 3.f);

    OCIO::IndexMapping ref2(4);
    ref2.setPair(0u, 0.f, 0.f);
    ref2.setPair(1u, 100.f, 1.f);
    ref2.setPair(2u, 200.f, 2.f);
    ref2.setPair(3u, 300.f, 3.f);

    OCIO_CHECK_ASSERT(ref1 == ref2);

    ref2.setPair(2u, 200.f, 2.1f);

    OCIO_CHECK_ASSERT(!(ref1 == ref2));
}

#endif
