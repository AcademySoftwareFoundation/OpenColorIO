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


#ifndef INCLUDED_OCIO_CTFLUT1DELT_H
#define INCLUDED_OCIO_CTFLUT1DELT_H

#include "CTFOpElt.h"
#include "CTFArrayMgt.h"
#include "CTFIndexMapMgt.h"
#include "../opdata/OpDataLut1D.h"
#include "../opdata/OpDataIndexMapping.h"

OCIO_NAMESPACE_ENTER
{
    // Private namespace to the CTF sub-directory
    namespace CTF
    {
        // Private namespace for the xml reader utils
        namespace Reader
        {

            // Class for Lut1DData
            class Lut1DElt : public OpElt, public ArrayMgt, public IndexMapMgt
            {
            public:
                // Constructor
                Lut1DElt();
                // Destructor
                ~Lut1DElt();

                // Start the parsing of the element
                virtual void start(const char **atts);

                // End the parsing of the element
                virtual void end();

                // Get the associated OpData
                OpData::OpData* getOp() const;

                // Get the associated Lut1D
                OpData::Lut1D* getLut() const
                {
                    return m_lut;
                }

                // Update the array dimensions
                // - dims are the array dimensions
                // returns the resized array
                virtual OpData::ArrayBase* updateDimension(const Dimensions& dims);

                // Finalize the array data origanization
                // - position is the position of the last value found
                virtual void finalize(unsigned position);

                // Update the IndexMap dimensions
                // - dims holds the desired IndexMap dimensions
                // Returns the resized IndexMap
                virtual OpData::IndexMapping* updateDimensionIM(const DimensionsIM& dims);

                // Finalize the IndexMap data origanization
                // - 'position' is the position of the last value found
                virtual void finalizeIM(unsigned position);

            protected:
                OpData::Lut1D* m_lut; // The associated Lut1D
                OpData::IndexMapping m_indexMapping;
            };

            // Class for Lut1D version Xml transform version 1.4
            class Lut1DElt_1_4 : public Lut1DElt
            {
            public:
                // Constructor
                Lut1DElt_1_4() : Lut1DElt() {}

                // Destructor
                ~Lut1DElt_1_4() {}

                // Start the parsing of the element
                virtual void start(const char **atts);
            };

            // Class for Lut1D version Xml transform version 1.7
            class Lut1DElt_1_7 : public Lut1DElt_1_4
            {
            public:
                // Constructor
                Lut1DElt_1_7() : Lut1DElt_1_4() {}
        
                // Destructor
                ~Lut1DElt_1_7() {}

                // End the parsing of the element
                void end();
            };

        } // exit Reader namespace
    } // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

#endif
