// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_MATRIXOPDATA_H
#define INCLUDED_OCIO_MATRIXOPDATA_H

#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/OpArray.h"

namespace OCIO_NAMESPACE
{

class MatrixOpData;
typedef OCIO_SHARED_PTR<MatrixOpData> MatrixOpDataRcPtr;
typedef OCIO_SHARED_PTR<const MatrixOpData> ConstMatrixOpDataRcPtr;

// The class represents the Matrix op.
// 
// The class specifies a matrix transformation to be applied to
// the input values. The input and output of a matrix are always
// 4-component values.
// An offset vector is also applied to the result.
// The output values are calculated using the row-order convention:
// 
// Rout = a[0][0]*Rin + a[0][1]*Gin + a[0][2]*Bin + a[0][3]*Ain + o[0];
// Gout = a[1][0]*Rin + a[1][1]*Gin + a[1][2]*Bin + a[1][3]*Ain + o[1];
// Bout = a[2][0]*Rin + a[2][1]*Gin + a[2][2]*Bin + a[2][3]*Ain + o[2];
// Aout = a[3][0]*Rin + a[3][1]*Gin + a[3][2]*Bin + a[3][3]*Ain + o[3];

class MatrixOpData : public OpData
{
public:

    static MatrixOpDataRcPtr CreateDiagonalMatrix(double diagValue);

    class Offsets
    {
    public:
        Offsets() = default;
        Offsets(double redOff, double grnOff, double bluOff, double whtOff);
        Offsets(const Offsets & o);
        ~Offsets() = default;

        Offsets& operator=(const Offsets & o);

        bool operator==(const Offsets & o) const;

        template<typename T>
        void setRGB(const T * v3);

        template<typename T>
        void setRGBA(const T * v4);

        inline const double & operator[](unsigned long index) const
        {
            return m_values[index];
        }

        inline double & operator[](unsigned long index)
        {
            return m_values[index];
        }

        inline const double * getValues() const
        {
            return m_values;
        }

        inline double * getValues()
        {
            return m_values;
        }

        bool isNotNull() const;

        void scale(double s);

    private:
        double m_values[4] {0., 0., 0., 0.};
    };

    class MatrixArray;
    typedef OCIO_SHARED_PTR<MatrixArray> MatrixArrayPtr;

    class MatrixArray : public ArrayDouble
    {
    public:
        MatrixArray();
        MatrixArray(const MatrixArray &) = default;
        MatrixArray & operator=(const MatrixArray & m) = default;
        virtual ~MatrixArray() = default;

        MatrixArray & operator=(const ArrayDouble & a);

        bool isUnityDiagonal() const;

        void validate() const override;

        unsigned long getNumValues() const override;

        // Inner product of this matrix times matrix B.
        MatrixArrayPtr inner(const MatrixArray & B) const;
        MatrixArrayPtr inner(const MatrixArrayPtr & B) const;

        // Inner product (multiplication) of the matrix with the offsets b. 
        Offsets inner(const Offsets & b) const;

        MatrixArrayPtr inverse() const;

        template<typename T>
        void setRGB(const T * values);

        void setRGBA(const float * values);
        void setRGBA(const double * values);

    protected:
        void fill();

        void expandFrom3x3To4x4();
    };

public:
    MatrixOpData();
    explicit MatrixOpData(TransformDirection direction);
    MatrixOpData(const MatrixArray & matrix);
    MatrixOpData(const MatrixOpData &) = default;

    virtual ~MatrixOpData();

    MatrixOpDataRcPtr clone() const;

    inline const ArrayDouble & getArray() const { return m_array; }

    inline ArrayDouble & getArray() { return m_array; }

    void setArrayValue(unsigned long index, double value);
    double getArrayValue(unsigned long index) const;

    // Set the RGB values (alpha reset to 0).
    void setRGB(const float * values);

    template<typename T>
    void setRGBA(const T * values);

    inline const Offsets & getOffsets() const
    {
        return m_offsets;
    }

    inline Offsets & getOffsets()
    {
        return m_offsets;
    }

    double getOffsetValue(unsigned long index) const;

    inline void setRGBOffsets(const float * offsets)
    {
        m_offsets.setRGB(offsets);
    }

    inline void setRGBAOffsets(const float * offsets)
    {
        m_offsets.setRGBA(offsets);
    }

    inline void setRGBAOffsets(const double * offsets)
    {
        m_offsets.setRGBA(offsets);
    }

    inline void setOffsets(const Offsets & offsets)
    {
        m_offsets = offsets;
    }

    void setOffsetValue(unsigned long index, double value);

    void validate() const override;

    Type getType() const override { return MatrixType; }

    bool isNoOp() const override;

    bool isIdentity() const override;

    // Determine whether the output of the op mixes R, G, B channels.
    // For example, Rout = 5*Rin is channel independent,
    // but Rout = Rin + Gin is not.
    // Note that the property may depend on the op parameters,
    // so, e.g. MatrixOps may sometimes return true and other times false.
    // Returns true if the op's output combines input channels.
    bool hasChannelCrosstalk() const override { return !isDiagonal(); }

    std::string getCacheID() const override;

    // Check if the matrix array is a no-op (ignoring the offsets).
    bool isUnityDiagonal() const;

    // Is it a diagonal matrix (off-diagonal coefs are 0)?
    bool isDiagonal() const;

    inline bool hasOffsets() const { return m_offsets.isNotNull(); }

    bool hasAlpha() const;

    MatrixOpDataRcPtr compose(ConstMatrixOpDataRcPtr & B) const;

    // Used by composition to remove small errors.
    void cleanUp(double offsetScale);

    bool operator==(const OpData & other) const override;

    TransformDirection getDirection() const noexcept { return m_direction; }
    void setDirection(TransformDirection dir) noexcept;

    MatrixOpDataRcPtr getAsForward() const;

    BitDepth getFileInputBitDepth() const { return m_fileInBitDepth; }
    void setFileInputBitDepth(BitDepth in) { m_fileInBitDepth = in; }

    BitDepth getFileOutputBitDepth() const { return m_fileOutBitDepth; }
    void setFileOutputBitDepth(BitDepth out) { m_fileOutBitDepth = out; }

    void scale(double inScale, double outScale);

private:

    MatrixArray m_array;
    Offsets     m_offsets;

    // In bit-depth to be used for file I/O.
    BitDepth m_fileInBitDepth = BIT_DEPTH_UNKNOWN;
    // Out bit-depth to be used for file I/O.
    BitDepth m_fileOutBitDepth = BIT_DEPTH_UNKNOWN;

    TransformDirection m_direction{ TRANSFORM_DIR_FORWARD };
};
} // namespace OCIO_NAMESPACE

#endif
