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

#ifndef INCLUDED_OCIO_OPS_INDEXMAPPING_H
#define INCLUDED_OCIO_OPS_INDEXMAPPING_H

#include <utility>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

OCIO_NAMESPACE_ENTER
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

}
OCIO_NAMESPACE_EXIT


#endif
