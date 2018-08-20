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


#ifndef INCLUDED_OCIO_CTFLUT3DELT_H
#define INCLUDED_OCIO_CTFLUT3DELT_H

#include "CTFOpElt.h"
#include "CTFArrayElt.h"
#include "CTFIndexMapElt.h"
#include "../opdata/OpDataLut3D.h"
#include "../opdata/OpDataIndexMapping.h"

OCIO_NAMESPACE_ENTER
{

// Private namespace to the CTF sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{
// Class for Lut3DData
class Lut3DElt : public OpElt, public ArrayMgt, public IndexMapMgt
{
public:
    // Constructor
    Lut3DElt();

    // Destructor
    ~Lut3DElt();

    // Start the parsing of the element
    virtual void start(const char **atts);

    // End the parsing of the element
    virtual void end();

    // Get the associated OpData
    OpData::OpData* getOp() const;

    // Get the associated Lut3D
    OpData::Lut3D* getLut() const
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

    // Update the array dimensions
    // - dims holds the desired array dimensions
    // Returns the resized array
    virtual OpData::IndexMapping* updateDimensionIM(const DimensionsIM& dims);

    // Finalize the array data origanization
    // - position is the position of the last value found
    virtual void finalizeIM(unsigned position);

protected:
    // Convert the interpolation string to its enum's value
    // - str is the interpolation algorithm
    Interpolation getInterpolation(const std::string & str);

protected:
    OpData::Lut3D* m_lut; // The associated Lut3D
    OpData::IndexMapping m_indexMapping;
};

// Class for Lut3D version Xml transform version 1.7
class Lut3DElt_1_7 : public Lut3DElt
{
public:
    // Constructor
    Lut3DElt_1_7() : Lut3DElt() {}
        
    // Destructor
    ~Lut3DElt_1_7() {}

    // End the parsing of the element
    void end();
};

} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

#endif
