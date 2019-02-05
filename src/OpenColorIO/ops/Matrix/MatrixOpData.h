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


#ifndef INCLUDED_OCIO_MATRIXOPDATA_H
#define INCLUDED_OCIO_MATRIXOPDATA_H

#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/OpArray.h"

OCIO_NAMESPACE_ENTER
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

    static MatrixOpDataRcPtr CreateDiagonalMatrix(BitDepth inBitDepth,
                                                  BitDepth outBitDepth,
                                                  double diagValue);

public:
    class Offsets
    {
    public:
        Offsets();

        Offsets(const Offsets & o);

        ~Offsets();

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

        inline double& operator[](unsigned long index)
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
        double m_values[4];
    };

public:

    MatrixOpData();
        
    MatrixOpData(BitDepth inBitDepth, BitDepth outBitDepth);

    MatrixOpData(BitDepth inBitDepth,
                 BitDepth outBitDepth,
                 const std::string & id,
                 const Descriptions & descriptions);

    virtual ~MatrixOpData();

    MatrixOpDataRcPtr clone() const;

    inline const ArrayDouble & getArray() const { return m_array; }

    inline ArrayDouble & getArray() { return m_array; }

    void setArrayValue(unsigned long index, double value);

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

    void finalize() override;

    OpDataRcPtr getIdentityReplacement() const;

    // Check if the matrix array is a no-op (ignoring the offsets).
    bool isUnityDiagonal() const;

    // Is it a diagonal matrix (off-diagonal coefs are 0)?
    bool isDiagonal() const;

    inline bool hasOffsets() const { return m_offsets.isNotNull(); }

    bool hasAlpha() const;

    virtual void setOutputBitDepth(BitDepth out) override;

    virtual void setInputBitDepth(BitDepth in) override;

    MatrixOpDataRcPtr compose(ConstMatrixOpDataRcPtr & B) const;

    // Used by composition to remove small errors.
    void cleanUp(double offsetScale);

    bool operator==(const OpData & other) const override;

    MatrixOpDataRcPtr inverse() const;

private:

    bool isMatrixIdentity() const;

    class MatrixArray;
    typedef OCIO_SHARED_PTR<MatrixArray> MatrixArrayPtr;

    class MatrixArray : public ArrayDouble
    {
    public:
        MatrixArray(BitDepth inBitDepth,
                    BitDepth outBitDepth,
                    unsigned long dimension,
                    unsigned long numColorComponents);

        ~MatrixArray();

        MatrixArray & operator=(const ArrayDouble & a);

        bool isUnityDiagonal() const;

        void validate() const override;

        unsigned long getNumValues() const override;

        // Inner product of this matrix times matrix B.
        MatrixArrayPtr inner(const MatrixArray & B) const;

        // Inner product (multiplication) of the matrix with the offsets b. 
        void inner(const Offsets & b, Offsets & out) const;

        MatrixArrayPtr inverse() const;

        template<typename T>
        void setRGB(const T * values);

        void setRGBA(const float * values);
        void setRGBA(const double * values);

        void setOutputBitDepth(BitDepth out);
        void setInputBitDepth(BitDepth in);

    protected:
        void fill();

        void expandFrom3x3To4x4();

    private:
        BitDepth m_inBitDepth;
        BitDepth m_outBitDepth;

    };

    MatrixArray m_array;
    Offsets     m_offsets;
};
}
OCIO_NAMESPACE_EXIT

#endif
