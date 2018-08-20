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


#ifndef INCLUDED_OCIO_OPDATALUT1D_H
#define INCLUDED_OCIO_OPDATALUT1D_H

#include <OpenColorIO/OpenColorIO.h>

#include "OpDataVec.h"
#include "OpDataArray.h"

OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace OpData
{
class Lut1D;
typedef OCIO_SHARED_PTR<Lut1D> OpDataLut1DRcPtr;

class Lut1D : public OpData
{
public:

    // List of flags that describe 1-D LUT index and value encoding.
    //
    // 1-D LUT indices and values can either be expressed in standard numeric
    // encodings or using half float codes.   Half float codes are 16 bit integer
    // representations of a 16 bit floating point value. See:
    // http://en.wikipedia.org/wiki/Half-precision_floating-point_format.
    //
    enum HalfFlags {
        LUT_STANDARD         = 0x00,// Indices and values use standard numeric encodings.
        LUT_INPUT_HALF_CODE  = 0x01,// LUT indices are half float codes.
        LUT_OUTPUT_HALF_CODE = 0x02,// LUT values are half float codes.

        LUT_INPUT_OUTPUT_HALF_CODE =
            LUT_INPUT_HALF_CODE
                | LUT_OUTPUT_HALF_CODE // Both indices and values are half float codes.
    };

    // Enum to control optional hue restoration algorithm.
    enum HueAdjust
    {
        HUE_NONE = 0, // No adjustment
        HUE_DW3       // DW3 hue restoration algorithm
    };

    // Make an identity LUT with a domain suitable for pre-composing
    // with this LUT so that a lookup may be done rather than interpolation.
    // - incomingDepth the type of data to be rendered
    // Return the lockup domain LUT.
    static OpDataLut1DRcPtr makeLookupDomain(BitDepth incomingDepth);


    explicit Lut1D(unsigned dimension);
    Lut1D(BitDepth inBitDepth, BitDepth outBitDepth, HalfFlags halfFlags);

    // Constructor
    Lut1D(BitDepth inBitDepth,             // Requested bit depth for the input
            BitDepth outBitDepth,            // Requested bit depth for the output
            const std::string& id,           // Identifier
            const std::string& name,         // Human readable identifier
            const Descriptions& descriptions,// Human readable descriptions
            Interpolation interpolation,     // Interpolation algorithm
            HalfFlags halfFlags);            // Half flags for LUT

    // Constructor
    Lut1D(BitDepth inBitDepth,             // Requested bit depth for the input
            BitDepth outBitDepth,            // Requested bit depth for the output
            const std::string& id,           // Identifier
            const std::string& name,         // Human readable identifier
            const Descriptions& descriptions,// Human readable descriptions
            Interpolation interpolation,     // Interpolation algorithm
            HalfFlags halfFlags,             // Half flags for LUT
            unsigned dimension);             // LUT color component dimension

                                            // Destructor
    virtual ~Lut1D();

    virtual const std::string& getOpTypeName() const;
    virtual OpType getOpType() const;

    // Get the interpolation algorithm enumeration value from its name
    // - str is the string representation of the interpolation algorithm
    // Return the enumeration value.
    //         Returns Color::Lut1DOp::ALGO_DEFAULT if the name is unknown
    static Interpolation getInterpolation(const char* str);

    inline Interpolation getInterpolation() const { return m_interpolation; }

    // Get the interpolation algorithm that has to be used.
    // INTERP_BEST and INTERP_DEFAULT are translated to what should be used.
    // TODO: some valid inetrpolation styles that are not implemented yet
    // are also translated.
    Interpolation getConcreteInterpolation() const;

    void setInterpolation(Interpolation algo);

    // True if the op does nothing except bit-depth conversion
    virtual bool isIdentity() const;

    // LUTs are clamping
    virtual bool isClamping() const
    {
        return true;
    }

    bool hasChannelCrosstalk() const;

    // True if the luts may be combined via composition.
    bool mayCompose(const Lut1D & lut) const;

    virtual std::auto_ptr<OpData> getIdentityReplacement() const;

    // Check if the LUT is using half code indices as its domain.
    // Return returns true if this LUT requires half code indices as input.
    inline bool isInputHalfDomain() const {
        return ((m_halfFlags & LUT_INPUT_HALF_CODE) ==
            LUT_INPUT_HALF_CODE);
    }

    virtual void setOutputBitDepth(BitDepth out);

    // Sets whether the LUT uses half codes as the input domain.
    // - half domain flag value.
    // Note: this function is used by the xml reader to build the op and is
    //       not intended for other use.
    void setInputHalfDomain(bool isHalfDomain);

    // Sets whether the LUT coefficients are raw half values.
    // - raw half flag value.
    // Note: this function is used by the xml reader to build the op and is
    //       not intended for other use.
    void setOutputRawHalfs(bool isRawHalfs);

    inline HueAdjust getHueAdjust() const { return m_hueAdjust; }

    void setHueAdjust(HueAdjust algo);

    // Check if the LUT coefficients are raw half values.
    // Return returns true if the LUT coefficients are raw half values.
    inline bool isOutputRawHalfs() const {
        return ((m_halfFlags & LUT_OUTPUT_HALF_CODE) ==
            LUT_OUTPUT_HALF_CODE);
    }

    // Get the halfFlags (to facilitate copying to a new LUT)
    // Return the halfFlags
    inline HalfFlags getHalfFlags() const { return m_halfFlags; }

    // Get an array containing the LUT elements.
    // The elements are stored as a vector [r0,g0,b0, r1,g1,b1, r2,g2,b2, ...].
    // Return the array of data
    inline const Array & getArray() const { return m_array; }

    // Get an array containing the LUT elements.
    // The elements are stored as a vector [r0,g0,b0, r1,g1,b1, r2,g2,b2, ...].
    // Return the array of data
    inline Array & getArray() { return m_array; }

    // Validate the LUT contents and throw if any issues are found
    void validate() const;

    // Clone the OpData
    virtual OpData * clone(CloneType type) const;

    // Get the inverse of the OpData
    virtual void inverse(OpDataVec & v) const;

    // True if the pair of ops are inverses of each other (an identity pair).
    virtual bool isInverse(const Lut1D & lut) const;

    // Apply functional composition such that the result is
    // equivalent to applying this LUT followed by LUT B.
    OpDataLut1DRcPtr compose(const Lut1D & B) const;

    // Determines whether all channels use the same LUT
    // Return true if this Lut1DOp applies the same LUT 
    // to each of r, g, and b.
    inline bool hasSingleLut() const
        { return ( m_array.getNumColorComponents() == 1 ); }

    // Determine if the LUT has an appropriate domain to allow
    // lookup rather than interpolation
    // - incomingDepth the type of data to be rendered
    bool mayLookup(BitDepth incomingDepth) const;

    // Check if a Lut1DOp is equal to this instance.
    // - other is the other Lut1DOp to compare with.
    // Return true if they are equal, false otherwise
    bool operator==(const OpData & other) const;

    // Get the minimum XML version required for this op.
    // Return the version number
    const CTF::Version & getMinimumVersion() const;

    // Iterate through the LUT and set the number of components to 1
    // if all channels are equal.
    void adjustColorComponentNumber();

private:
    // Test core parts of LUTs for equality.
    bool haveEqualBasics(const Lut1D & lut) const;

    // Class which encapsulates an array dedicated to a 3By1D LUT
    class Lut3by1DArray : public Array
    {
    public:
        // Constructor
        // - inBitDepth is the input bit depth
        // - outBitDepth is the output bit depth
        // - halfFlags Half flags for LUT
        Lut3by1DArray(BitDepth  inBitDepth,
                        BitDepth  outBitDepth,
                        HalfFlags halfFlags);

        // Constructor
        // - outBitDepth is the output bit depth
        // - halfFlags Half flags for LUT
        // - length is the number of LUT entries per channel
        Lut3by1DArray(BitDepth  outBitDepth,
                        HalfFlags halfFlags,
                        unsigned length);

        // Destructor
        ~Lut3by1DArray();

        // Is an identity LUT?
        // - halfFlags Half flags for LUT
        // - outBitDepth is the output bit depth
        // Return true if the array only does bit-depth conversion
        bool isIdentity(HalfFlags halfFlags, BitDepth outBitDepth) const;

        // Get the number of values inb the arrya
        unsigned getNumValues() const;

    protected:
        // Fill the LUT 1D with appropriate default values 
        // representing an identity LUT.
        // - halfFlags Half flags for LUT
        // - outBitDepth is the output bit depth
        void fill(HalfFlags halfFlags, BitDepth outBitDepth);

        // Default copy constructor and assignation operator are fine
    };

    // Get the LUT length that would allow a look-up for inputBitDepth
    // - inputBitDepth is the input bit depth
    // - halfFlags except if the LUT has a half domain, always return 65536
    // Return the ideal LUT size
    static unsigned GetLutIdealSize(BitDepth inputBitDepth,
                                    HalfFlags halfFlags);

    Interpolation m_interpolation; // Interpolation algorithm
    Lut3by1DArray m_array;         // All 1D LUT values
    HalfFlags     m_halfFlags;     // Half flags for the LUT.
    HueAdjust     m_hueAdjust;     // Hue adjustment algorithm

};
} // exit OpData namespace
}
OCIO_NAMESPACE_EXIT

#endif
