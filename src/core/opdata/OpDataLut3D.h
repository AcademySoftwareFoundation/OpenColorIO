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


#ifndef INCLUDED_OCIO_OPDATALUT3D_H
#define INCLUDED_OCIO_OPDATALUT3D_H

#include <OpenColorIO/OpenColorIO.h>

#include "OpDataVec.h"
#include "OpDataArray.h"

OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace OpData
{
class Lut3D;
typedef OCIO_SHARED_PTR<Lut3D> OpDataLut3DRcPtr;

// The class represents the 3D LUT process node
//
// This class represents a 3D LUT. In a 3D LUT, the 3 color components
//   of the input value are used to find the nearest indexed values along 
//   each axis of the 3D face-centered cube. The 3-component output value 
//   is calculated by interpolating within the volume defined be the 
//   nearest 8 positions in the LUT.
//
//    The lookup operation may be altered by redefining the mapping 
//    of the input values to index positions of the LUT 
//    using the IndexMapping.
//
class Lut3D : public OpData
{
public:

    // The maximum length supported for a 3D LUT
    static unsigned maxSupportedLength;

public:
    // Constructor
    //  -gridSize is the length of the cube axis
    explicit Lut3D(unsigned gridSize);

    // Constructor
    Lut3D(
        BitDepth inBitDepth,             // Requested bit depth for the input
        BitDepth outBitDepth,            // Requested bit depth for the output
        const std::string& id,           // Identifier
        const std::string& name,         // Human readable identifier
        const Descriptions& descriptions,// Human readable descriptions
        Interpolation interpolation,     // Interpolation algorithm
        unsigned gridSize                // Length of the cube axis
    );

    // Destructor
    virtual ~Lut3D();

    // Get the interpolation algorithm enumeration value from its name
    // -  str is the string representation of the interpolation algorithm
    // Return the enumeration value.
    //         Will throw if the name is unknown
    static Interpolation getInterpolation(const char* str);

    // Get the interpolation algorithm
    // Return the interpolation algorithm
    inline Interpolation getInterpolation() const { return m_interpolation; }

    // Get the interpolation algorithm that has to be used.
    // INTERP_BEST and INTERP_DEFAULT are translated to what should be used.
    Interpolation getConcreteInterpolation() const;

    // Set the interpolation algorithm
    // - algo is the interpolation algorithm
    void setInterpolation(Interpolation algo);

    // Get the array
    // Return the array of data
    inline const Array& getArray() const { return m_array; }

    // Get the array
    // Return the array of data
    inline Array& getArray() { return m_array; }

    // Get the grid dimensions of the array (array is N x N x N x 3)
    // Return the dimension N
    inline unsigned getGridSize() const { return m_array.getLength(); }

    // Validate the state of the instance
    void validate() const;

    // Get the operation type
    // Return the operation type
    virtual inline OpType getOpType() const { return OpData::Lut3DType; }

    // Get the operation type name
    // Return the name of the operation type
    virtual const std::string& getOpTypeName() const;

    // True if the op does nothing except bit-depth conversion
    virtual bool isIdentity() const;

    // LUTs are clamping
    virtual bool isClamping() const
    {
        return true;
    }

    // Is channel idenpendent ?
    bool hasChannelCrosstalk() const { return false; }

    // Get an identiry replacement
    virtual std::auto_ptr<OpData> getIdentityReplacement() const;

    // Set the output bit depth
    // Overwritten from parent class Op
    // - out the output bit depth.
    void setOutputBitDepth(BitDepth out);

    // Clone the OpData
    virtual OpData * clone(CloneType type) const;

    // Get the inverse of the OpData
    virtual void inverse(OpDataVec & v) const;

    // True if the pair of ops are inverses of each other (an identity pair).
    virtual bool isInverse(const Lut3D& lut) const;

    // Check if a Lut3DOp is equal to this instance.
    // - other is the other Lut3DOp to compare with.
    // Return true if they are equal, false otherwise
    bool operator==(const OpData& other) const;

protected:
    // Test core parts of LUTs for equality.
    bool haveEqualBasics(const Lut3D & B) const;

public:
    // Class which encapsulates an array dedicated to a 3D LUT
    class Lut3DArray : public Array
    {
    public:
        // Constructor
        // -  dimension is the dimension of the array
        // -  outBitDepth is the output bit depth
        Lut3DArray(unsigned dimension, BitDepth outBitDepth);

        // Destructor
        ~Lut3DArray();

        // Assignation operator
        // - a the raw array data
        // Return Itself
        Lut3DArray& operator=(const Array& a);

        // Is an identity LUT?
        // - outBitDepth is the output bit depth
        // Return true if the content is an identity LUT
        bool isIdentity(BitDepth outBitDepth) const;

        //Get the number of values
        unsigned getNumValues() const;

        // Extract values from the 3d LUT array.
        // - i index
        // - j index
        // - k index
        // - RGB pointer to 3 float values
        void getRGB(unsigned i, unsigned j, unsigned k, float* RGB) const;

        // Insert values into the 3d LUT array.
        // - i index
        // - j index
        // - k index
        // - RGB pointer to 3 float values
        void setRGB(unsigned i, unsigned j, unsigned k, float* RGB);

    protected:
        // Fill the LUT 1D with appropriate default values
        // - outBitDepth is the output bit depth
        void fill(BitDepth outBitDepth);

        // Default copy constructor and assignation operator are fine
    };

    Interpolation m_interpolation; // Interpolation algorithm
    Lut3DArray    m_array;         // All 3D LUT values
};
} // exit OpData namespace
}
OCIO_NAMESPACE_EXIT

#endif
