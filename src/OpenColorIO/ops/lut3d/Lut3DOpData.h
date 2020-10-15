// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_LUT3DOPDATA_H
#define INCLUDED_OCIO_LUT3DOPDATA_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/OpArray.h"
#include "PrivateTypes.h"


namespace OCIO_NAMESPACE
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
    static Lut3DOpDataRcPtr Compose(ConstLut3DOpDataRcPtr & lut1, ConstLut3DOpDataRcPtr & lut2);

public:
    // The gridSize parameter is the length of the cube axis.
    explicit Lut3DOpData(unsigned long gridSize);

    Lut3DOpData(long gridSize, TransformDirection dir);

    Lut3DOpData(Interpolation interpolation, unsigned long gridSize);

    virtual ~Lut3DOpData();

    inline Interpolation getInterpolation() const { return m_interpolation; }

    // Get the interpolation algorithm that has to be used.
    // INTERP_BEST and INTERP_DEFAULT are translated to what should be used.
    Interpolation getConcreteInterpolation() const;
    static Interpolation GetConcreteInterpolation(Interpolation interp);

    void setInterpolation(Interpolation interpolation);
    static bool IsValidInterpolation(Interpolation interpolation);

    TransformDirection getDirection() const { return m_direction; }
    void setDirection(TransformDirection dir) { m_direction = dir; }

    // Note: The Lut3DOpData Array stores the values in blue-fastest order.
    inline const Array & getArray() const { return m_array; }
    inline Array & getArray() { return m_array; }

    void setArrayFromRedFastestOrder(const std::vector<float> & lut);

    // Get the grid dimensions of the array (array is N x N x N x 3).
    // Returns the dimension N.
    inline long getGridSize() const { return m_array.getLength(); }

    void validate() const override;

    Type getType() const override { return Lut3DType; }

    bool isNoOp() const override;

    bool isIdentity() const override;

    bool hasChannelCrosstalk() const override { return true; }

    OpDataRcPtr getIdentityReplacement() const override;

    Lut3DOpDataRcPtr clone() const;

    bool isInverse(ConstLut3DOpDataRcPtr & lut) const;

    Lut3DOpDataRcPtr inverse() const;

    bool operator==(const OpData& other) const override;

    std::string getCacheID() const override;

    inline BitDepth getFileOutputBitDepth() const { return m_fileOutBitDepth; }
    inline void setFileOutputBitDepth(BitDepth out) { m_fileOutBitDepth = out; }

    void scale(float scale);

protected:
    // Test core parts of LUTs for equality.
    bool haveEqualBasics(const Lut3DOpData & other) const;

public:
    // Class which encapsulates an array dedicated to a 3D LUT.
    class Lut3DArray : public Array
    {
    public:
        Lut3DArray(unsigned long dimension);

        Lut3DArray(const Lut3DArray &) = default;
        Lut3DArray & operator= (const Lut3DArray &) = default;

        ~Lut3DArray();

        Lut3DArray& operator=(const Array& a);

        void resize(unsigned long length, unsigned long numColorComponents) override;

        unsigned long getNumValues() const override;

        void getRGB(long i, long j, long k, float* RGB) const;

        void setRGB(long i, long j, long k, float* RGB);

        void scale(float scaleFactor);

    protected:
        // Fill the LUT 3D with appropriate default values.
        void fill();

    };

private:
    Lut3DOpData() = delete;

    Interpolation       m_interpolation;
    Lut3DArray          m_array;

    TransformDirection  m_direction;

    // Out bit-depth to be used for file I/O.
    BitDepth m_fileOutBitDepth = BIT_DEPTH_UNKNOWN;

};

// Make a forward Lut3DOpData that approximates the exact inverse Lut3DOpData
// to be used for the fast rendering style.
// LUT has to be inverse or the function will throw.
Lut3DOpDataRcPtr MakeFastLut3DFromInverse(ConstLut3DOpDataRcPtr & lut);

} // namespace OCIO_NAMESPACE

#endif
