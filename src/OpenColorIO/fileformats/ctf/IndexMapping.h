// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_OPS_INDEXMAPPING_H
#define INCLUDED_OCIO_OPS_INDEXMAPPING_H

#include <utility>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

// The class represents the index mapping for a LUT
// This class defines a list that is the new mapping of input 
// code values (inValues) to index positions (n) in a LUT.
// 
// TODO: This is an initial implementation to just do the minimal
//       required for CLF support.  We may add more later.
//
// Note: The 1D & 3D LUT classes do not have an IndexMaping instance,
//       our current implementation converts a map into a separate
//       Range op when the file is read.
//
class IndexMapping
{
public:
    typedef std::pair<float,float> Data;
    typedef std::vector<Data> ComponentData;

    // Type definition of the three component colors.
    typedef ComponentData Indices[3];

public:
    explicit IndexMapping(size_t dimension);
    IndexMapping() = delete;

    virtual ~IndexMapping();

    size_t getDimension() const;

    void resize(size_t newDimension);

    const Indices & getIndices() const;

    unsigned int getNumComponents() const;

    void getPair(size_t index, float & first, float & second) const;

    void setPair(size_t index, float first, float second);

    void validate() const;

    bool operator==(const IndexMapping & other) const;

private:

    void validateIndex(size_t index) const;

    size_t   m_dimension; // Dimension
    Indices  m_indices;   // All values
};

} // namespace OCIO_NAMESPACE


#endif
