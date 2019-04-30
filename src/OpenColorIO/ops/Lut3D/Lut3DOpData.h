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

#ifndef INCLUDED_OCIO_LUT3DOPDATA_H
#define INCLUDED_OCIO_LUT3DOPDATA_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/OpArray.h"

OCIO_NAMESPACE_ENTER
{
class Lut3DOpData;
typedef OCIO_SHARED_PTR<Lut3DOpData> Lut3DOpDataRcPtr;
typedef OCIO_SHARED_PTR<const Lut3DOpData> ConstLut3DOpDataRcPtr;

class Lut3DOpData : public OpData
{
public:

    // The maximum grid size supported for a 3D LUT.
    static const unsigned long maxSupportedLength;

    // Use functional composition to generate a single op that 
    // approximates the effect of the pair of ops.
    static void Compose(Lut3DOpDataRcPtr & A,
                        ConstLut3DOpDataRcPtr & B);

public:
    // The gridSize parameter is the length of the cube axis.
    explicit Lut3DOpData(unsigned long gridSize);

    Lut3DOpData(long gridSize, TransformDirection dir);

    Lut3DOpData(
        BitDepth             inBitDepth,
        BitDepth             outBitDepth,
        const std::string &  id,
        const Descriptions & descriptions,
        Interpolation        interpolation,
        unsigned long        gridSize
    );

    virtual ~Lut3DOpData();

    inline Interpolation getInterpolation() const { return m_interpolation; }

    // Get the interpolation algorithm that has to be used.
    // INTERP_BEST and INTERP_DEFAULT are translated to what should be used.
    Interpolation getConcreteInterpolation() const;

    void setInterpolation(Interpolation algo);

    TransformDirection getDirection() const { return m_direction; }

    // There are two inversion algorithms provided for 3D LUT, an exact
    // method (that assumes use of tetrahedral in the forward direction)
    // and a fast method that bakes the inverse out as another forward
    // 3D LUT. The exact method is currently unavailable on the GPU.
    // Both methods assume that the input and output to the 3D LUT are
    // roughly perceptually uniform. Values outside the range of the
    // forward 3D LUT are clamped to someplace on the exterior surface
    // of the 3D LUT.
    inline LutInversionQuality getInversionQuality() const { return m_invQuality; }

    // LUT_INVERSION_BEST and LUT_INVERSION_DEFAULT are translated to what should be used.
    LutInversionQuality getConcreteInversionQuality() const;

    void setInversionQuality(LutInversionQuality style);

    // Note: The Lut3DOpData Array stores the values in blue-fastest order.
    inline const Array & getArray() const { return m_array; }
    inline Array & getArray() { return m_array; }

    // Get the grid dimensions of the array (array is N x N x N x 3).
    // Returns the dimension N.
    inline long getGridSize() const { return m_array.getLength(); }

    void validate() const override;

    Type getType() const override { return Lut3DType; }

    bool isNoOp() const override;

    bool isIdentity() const override;

    bool hasChannelCrosstalk() const override { return false; }

    OpDataRcPtr getIdentityReplacement() const;

    void setInputBitDepth(BitDepth in) override;

    void setOutputBitDepth(BitDepth out) override;

    Lut3DOpDataRcPtr clone() const;

    bool isInverse(ConstLut3DOpDataRcPtr & lut) const;

    Lut3DOpDataRcPtr inverse() const;

    bool operator==(const OpData& other) const override;

    void finalize() override;

protected:
    // Test core parts of LUTs for equality.
    bool haveEqualBasics(const Lut3DOpData & B) const;

    static bool isInverse(const Lut3DOpData * lutfwd, const Lut3DOpData * lutinv);

public:
    // Class which encapsulates an array dedicated to a 3D LUT.
    class Lut3DArray : public Array
    {
    public:
        Lut3DArray(unsigned long dimension, BitDepth outBitDepth);

        Lut3DArray(const Lut3DArray &) = default;
        Lut3DArray & operator= (const Lut3DArray &) = default;

        ~Lut3DArray();

        Lut3DArray& operator=(const Array& a);

        bool isIdentity(BitDepth outBitDepth) const;

        unsigned long getNumValues() const override;

        void getRGB(long i, long j, long k, float* RGB) const;

        void setRGB(long i, long j, long k, float* RGB);

        void scale(float scaleFactor);

    protected:
        // Fill the LUT 3D with appropriate default values.
        void fill(BitDepth outBitDepth);

    };

private:
    Lut3DOpData() = delete;

    Interpolation       m_interpolation;
    Lut3DArray          m_array;

    TransformDirection  m_direction;
    LutInversionQuality m_invQuality;
};

// Make a forward Lut3DOpData that approximates the exact inverse Lut3DOpData
// to be used for the fast rendering style.
// LUT has to be inverse or the function will throw.
Lut3DOpDataRcPtr MakeFastLut3DFromInverse(ConstLut3DOpDataRcPtr & lut);

}
OCIO_NAMESPACE_EXIT

#endif
