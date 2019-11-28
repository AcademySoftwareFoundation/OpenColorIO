// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "fileformats/ctf/IndexMapping.h"

namespace OCIO_NAMESPACE
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

} // namespace OCIO_NAMESPACE

