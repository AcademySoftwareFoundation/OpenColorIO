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

#ifndef INCLUDED_OCIO_OPDATAINDEXMAPPING_H
#define INCLUDED_OCIO_OPDATAINDEXMAPPING_H

#include <OpenColorIO/OpenColorIO.h>

#include <utility>
#include <vector>

OCIO_NAMESPACE_ENTER
{
    // Private namespace to the OpData sub-directory
    namespace OpData
    {

        // The class represents the index mapping for a LUT
        // This class defines a list that is the new mapping of input 
        // code values (inValues) to index positions (n) in a LUT.
        // 
        // TODO: This is an initial implementation to just do the minimal
        //       required for CLF support.  We may add more later.
        //
        // TODO: Although the Lut1D/Lut3D classes have an IndexMap instance,
        //       our current implementation converts a map into a separate
        //       Range op when the file is read.  If we ever want to use
        //       the IndexMap as part of the LUT, we need to update the LUT
        //       code so that resizing, setting bit-depth, cloning, etc.
        //       also make necessary adjustments to the IndexMap.
        //
        class IndexMapping
        {
        public:
            // Type definition of one entry of a component color
            typedef std::pair<float,float> Data;

            // Type definition of one component color
            typedef std::vector<Data> ComponentData;

            // Type definition of the three component colors
            typedef ComponentData Indices[3];

        public:
            // Constructor
            explicit IndexMapping(
                unsigned dimension // Dimension
                );

            // Destructor
            virtual ~IndexMapping();

            // Get the dimension
            unsigned getDimension() const;

            // Resize the array content
            // - newDimension the new number of _pairs_ of index data
            void resize(unsigned newDimension);

            // Get the indices
            const Indices& getIndices() const;

            // Get the selected component colors
            unsigned getComponents() const;

            // Get a pair of values
            // - index the index of the pair to get
            // - first the first value
            // - second the second value
            void getPair(unsigned index, float& first, float& second) const;

            // Set a pair of values
            // - index the index of the pair to set
            // - first the first value
            // - second the second value
            void setPair(unsigned index, float first, float second);

            // Validate the state of the instance
            void validate() const;

            // Check if an IndexMapping is equal to this instance.
            // - other is the other IndexMapping to compare with.
            // Return true if they are equal, false otherwise
            bool operator==(const IndexMapping& other) const;

        private:
            unsigned m_Dimension; // Dimension
            Indices m_Indices;    // All values
        };
    }
}
OCIO_NAMESPACE_EXIT


#endif
