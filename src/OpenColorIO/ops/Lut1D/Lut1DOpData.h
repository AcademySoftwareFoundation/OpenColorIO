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

#ifndef INCLUDED_OCIO_LUT1DOPDATA_H
#define INCLUDED_OCIO_LUT1DOPDATA_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/OpArray.h"

OCIO_NAMESPACE_ENTER
{
class Lut1DOpData;
typedef OCIO_SHARED_PTR<Lut1DOpData> Lut1DOpDataRcPtr;
typedef OCIO_SHARED_PTR<const Lut1DOpData> ConstLut1DOpDataRcPtr;

class Lut1DOpData : public OpData
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
        LUT_STANDARD = 0x00,         // Indices & values use standard encoding.
        LUT_INPUT_HALF_CODE = 0x01,  // LUT indices are half float codes.
        LUT_OUTPUT_HALF_CODE = 0x02, // LUT values are half float codes.

        LUT_INPUT_OUTPUT_HALF_CODE =
        LUT_INPUT_HALF_CODE
        | LUT_OUTPUT_HALF_CODE       // Indices and values are half float codes.
    };

    // Enum to control optional hue restoration algorithm.
    enum HueAdjust
    {
        HUE_NONE = 0, // No adjustment.
        HUE_DW3       // Algorithm used in ACES Output Transforms through v0.7.
    };

    // Enumeration of the inverse 1D LUT styles.
    enum InvStyle
    {
        INV_EXACT = 0,  // Exact, but slow, inverse processing.
        INV_FAST        // Fast, but approximate, inverse processing.
    };

    // Contains properties needed for inversion of a single channel of a LUT.
    struct ComponentProperties
    {
        ComponentProperties()
            : isIncreasing(false)
            , startDomain(0)
            , endDomain(0)
            , negStartDomain(0)
            , negEndDomain(0) {}

        bool isIncreasing;            // Represents the overall increasing state.
        unsigned long startDomain;    // Is the lowest index such that LUT[start] != LUT[start+1].
        unsigned long endDomain;      // Is the highest index such that LUT[end-1] != LUT[end].
        unsigned long negStartDomain; // StartDomain for half-domain negative values.
        unsigned long negEndDomain;   // EndDomain for half-domain negative values.
    };

    // Make an identity LUT with a domain suitable for pre-composing
    // with this LUT so that a lookup may be done rather than interpolation.
    static Lut1DOpDataRcPtr MakeLookupDomain(BitDepth incomingDepth);

    // Control behavior of 1D LUT composition.
    enum ComposeMethod
    {
        COMPOSE_RESAMPLE_NO = 0,      // Preserve original domain.
        COMPOSE_RESAMPLE_INDEPTH = 1, // InDepth controls min size.
        COMPOSE_RESAMPLE_BIG = 2      // Min size is 65536.
    };

    // Use functional composition to generate a single op that 
    // approximates the effect of the pair of ops.
    // A is used as in/out parameter. As input is it the first LUT in the composition,
    // as output it is the result of the composition.
    // B is the second LUT to compose and will not be modified.
    static void Compose(Lut1DOpDataRcPtr & A,
                        ConstLut1DOpDataRcPtr & B,
                        ComposeMethod compFlag);

    // Return the size to use for an identity LUT of the specified bit-depth.
    static unsigned long GetLutIdealSize(BitDepth incomingBitDepth);

    explicit Lut1DOpData(unsigned long dimension);
    Lut1DOpData(BitDepth inBitDepth, BitDepth outBitDepth, HalfFlags halfFlags);

    Lut1DOpData(BitDepth inBitDepth,
                BitDepth outBitDepth,
                const std::string & id,
                const Descriptions & descriptions,
                Interpolation interpolation,
                HalfFlags halfFlags);

    Lut1DOpData(BitDepth inBitDepth,
                BitDepth outBitDepth,
                const std::string & id,
                const Descriptions & descriptions,
                Interpolation interpolation,
                HalfFlags halfFlags,
                unsigned long dimension);

    virtual ~Lut1DOpData();

    inline Interpolation getInterpolation() const { return m_interpolation; }

    // Get the interpolation algorithm that has to be used.
    // INTERP_BEST and INTERP_DEFAULT are translated to what should be used.
    Interpolation getConcreteInterpolation() const;

    void setInterpolation(Interpolation algo);

    TransformDirection getDirection() const { return m_direction; }

    inline InvStyle getInvStyle() const { return m_invStyle; }

    void setInvStyle(InvStyle style);

    Type getType() const override { return Lut1DType; }

    bool isNoOp() const override;

    bool isIdentity() const override;

    bool hasChannelCrosstalk() const override;

    void setOutputBitDepth(BitDepth out) override;
    void setInputBitDepth(BitDepth in) override;

    // The file readers should call this to record the original scaling of the LUT values.
    void setFileBitDepth(BitDepth depth)
    {
        m_fileBitDepth = depth;
    }

    void finalize() override;

    // Check if the LUT is using half code indices as its domain.
    // Return returns true if this LUT requires half code indices as input.
    static inline bool IsInputHalfDomain(HalfFlags halfFlags)
    {
        return ((halfFlags & LUT_INPUT_HALF_CODE) ==
                LUT_INPUT_HALF_CODE);
    }
    inline bool isInputHalfDomain() const
    {
        return IsInputHalfDomain(m_halfFlags);
    }

    // Note: this function is used by the xml reader to build the op and is
    //       not intended for other use.
    void setInputHalfDomain(bool isHalfDomain);

    // Note: this function is used by the xml reader to build the op and is
    //       not intended for other use.
    void setOutputRawHalfs(bool isRawHalfs);

    inline bool isOutputRawHalfs() const {
        return ((m_halfFlags & LUT_OUTPUT_HALF_CODE) ==
                LUT_OUTPUT_HALF_CODE);
    }

    inline HalfFlags getHalfFlags() const { return m_halfFlags; }

    inline HueAdjust getHueAdjust() const { return m_hueAdjust; }

    void setHueAdjust(HueAdjust algo);

    // Get an array containing the LUT elements.
    // The elements are stored as a vector [r0,g0,b0, r1,g1,b1, r2,g2,b2, ...].
    inline const Array & getArray() const { return m_array; }

    // Get an array containing the LUT elements.
    // The elements are stored as a vector [r0,g0,b0, r1,g1,b1, r2,g2,b2, ...].
    inline Array & getArray() { return m_array; }

    void validate() const override;

    virtual Lut1DOpDataRcPtr clone() const;

    virtual Lut1DOpDataRcPtr inverse() const;

    bool isInverse(ConstLut1DOpDataRcPtr & lut) const;

    bool mayCompose(ConstLut1DOpDataRcPtr & B) const;

    // Return true if this Lut1DOp applies the same LUT 
    // to each of r, g, and b.
    inline bool hasSingleLut() const
    {
        return (m_array.getNumColorComponents() == 1);
    }

    // Determine if the LUT has an appropriate domain to allow
    // lookup rather than interpolation.
    bool mayLookup(BitDepth incomingDepth) const;

    bool operator==(const OpData & other) const override;

    OpDataRcPtr getIdentityReplacement() const;

    // Make a forward Lut1DOpData that approximates the exact inverse
    // Lut1DOpData to be used for the fast rendering style.
    // LUT has to be inverse or the function will throw.
    static Lut1DOpDataRcPtr MakeFastLut1DFromInverse(ConstLut1DOpDataRcPtr & lut, bool forGPU);

    inline const ComponentProperties & getRedProperties() const
    {
        return m_componentProperties[0];
    }

    inline const ComponentProperties & getGreenProperties() const
    {
        return m_componentProperties[1];
    }

    inline const ComponentProperties & getBlueProperties() const
    {
        return m_componentProperties[2];
    }

private:
    static bool IsInverse(const Lut1DOpData * lutfwd,
                          const Lut1DOpData * lutinv);

    Lut1DOpData() = delete;

    // Test core parts of LUTs for equality.
    bool haveEqualBasics(const Lut1DOpData & lut) const;

    // For inverse LUT.

    // Determine if the inverse LUT needs to handle values outside
    // the normal domain: e.g. [0,1023] for 10i or [0.,1.] for 16f.
    // (This is true if the forward LUT had an extended range.)
    bool hasExtendedDomain() const;
    void initializeFromForward();
    // Make the array monotonic and prepare params for the renderer.
    void prepareArray();

    class Lut3by1DArray : public Array
    {
    public:
        Lut3by1DArray(BitDepth  inBitDepth,
                      BitDepth  outBitDepth,
                      HalfFlags halfFlags);

        Lut3by1DArray(BitDepth  outBitDepth,
                      HalfFlags halfFlags,
                      unsigned long length);

        ~Lut3by1DArray();

        bool isIdentity(HalfFlags halfFlags, BitDepth outBitDepth) const;

        unsigned long getNumValues() const override;

        void scale(float scaleFactor);

    protected:
        // Fill the LUT 1D with appropriate default values 
        // representing an identity LUT.
        void fill(HalfFlags halfFlags, BitDepth outBitDepth);

    public:
        // Default copy constructor and assignation operator are fine.
        Lut3by1DArray() = default;
        Lut3by1DArray(const Lut3by1DArray &) = default;
        Lut3by1DArray & operator= (const Lut3by1DArray &) = default;
    };

    // Get the LUT length that would allow a look-up for inputBitDepth.
    // - halfFlags except if the LUT has a half domain, always return 65536
    static unsigned long GetLutIdealSize(BitDepth inputBitDepth,
                                         HalfFlags halfFlags);

    Interpolation       m_interpolation;
    Lut3by1DArray       m_array;
    HalfFlags           m_halfFlags;
    HueAdjust           m_hueAdjust;
                        
    TransformDirection  m_direction;

    // Members for inverse LUT.
    InvStyle            m_invStyle;

    ComponentProperties m_componentProperties[3];

    // The original LUT scaling from the file.
    // Must be set by the file reader.
    // Note: This is hopefully only needed temporarily.
    //       Used in MakeFastLut1DFromInverse.
    BitDepth            m_fileBitDepth;

};

}
OCIO_NAMESPACE_EXIT

#endif
