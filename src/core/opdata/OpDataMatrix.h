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


#ifndef INCLUDED_OCIO_OPDATAMATRIX_H
#define INCLUDED_OCIO_OPDATAMATRIX_H

#include <OpenColorIO/OpenColorIO.h>

#include <string>
#include "OpDataVec.h"
#include "OpDataArray.h"

OCIO_NAMESPACE_ENTER
{
    // Private namespace to the OpData sub-directory
    namespace OpData
    {
        class Matrix;
        typedef OCIO_SHARED_PTR<Matrix> OpDataMatrixRcPtr;

        /* The class represents the Matrix op

        The class specifies a matrix transformation to be applied to
        the input values. The input and output of a matrix are always
        4-component values.
        An offset vector is also applied to the result.
        The output values are calculated using the row-order convention:

        Rout = a[0][0]*Rin + a[0][1]*Gin + a[0][2]*Bin + a[0][3]*Ain + o[0];
        Gout = a[1][0]*Rin + a[1][1]*Gin + a[1][2]*Bin + a[1][3]*Ain + o[1];
        Bout = a[2][0]*Rin + a[2][1]*Gin + a[2][2]*Bin + a[2][3]*Ain + o[2];
        Aout = a[3][0]*Rin + a[3][1]*Gin + a[3][2]*Bin + a[3][3]*Ain + o[3];
        */
        class Matrix : public OpData
        {
        public:
            // Create a diagonal matrix
            // - inBitDepth is the input bit depth
            // - outBitDepth is the output bit depth
            // - diagValue is the value to put on the diagonal
            // returns a new instance (to be managed by the caller)
            static Matrix* CreateDiagonalMatrix(BitDepth inBitDepth,
                                                BitDepth outBitDepth,
                                                double diagValue);

        public:
            // Matrix offset coefficient class.
            class Offsets
            {
            public:
                // Constructor
                Offsets();

                // Copy constructor
                Offsets(const Offsets& o);

                // Destructor
                ~Offsets();

                // Assign an Offset
                Offsets& operator=(const Offsets& o);

                // TODO: long term, Matrix should only accept double
                // Set the RGB value of the offsets
                // - v3 the array of float to copy value from.
                void setRGBValues(const float* v3);

                // Set the RGBA value of the offsets
                // - v4 the array of float to copy value from.
                void setRGBAValues(const float* v4);

                // Check if the offsets have the same values
                // returns true upon success
                bool operator==(const Offsets& o) const;

                // Get a const reference to the offset value at index
                // - index of the offset value to return
                inline const double& operator[](unsigned index) const
                {
                    return m_values[index];
                }

                // Get a reference to the offset value at index
                // - index of the offset value to return
                inline double& operator[](unsigned index)
                {
                    return m_values[index];
                }

                // Return a pointer to the array of floats containing 
                // the offset values
                inline const double* getValues() const
                {
                    return m_values;
                }

                // Return a pointer to the array of floats containing 
                // the offset values
                inline double* getValues()
                {
                    return m_values;
                }

                // Check if any value is not zero
                bool isNotNull() const;

                // Apply a scale factor to all elements of the offset
                // - s the scale factor to apply
                void scale(double s);

            private:
                double m_values[4];
            };

        public:

            // Constructor
            Matrix();
        
            // Constructor
            // - inputBitDepth is the requested bit depth for the input
            // - outputBitDepth is the requested bit depth for the output
            Matrix(BitDepth inBitDepth, BitDepth outBitDepth);

            // Constructor
            // - inputBitDepth is the requested bit depth for the input
            // - outputBitDepth is the requested bit depth for the output
            // - id is the identifier
            // - name is the human readable name
            // - descriptions is the human readable descriptions
            Matrix(BitDepth inBitDepth,
                   BitDepth outBitDepth,
                   const std::string& id,
                   const std::string& name,
                   const Descriptions& descriptions);

            // Destructor
            virtual ~Matrix();

            // Clone the instance
            // - type is the type of clone to be used
            // returns the new instance
            OpData* clone(CloneType type) const;

            // Get the array
            inline const ArrayDouble& getArray() const { return m_array; }

            // Get the array
            inline ArrayDouble& getArray() { return m_array; }

            // Set an array value
            void setArrayValue(unsigned index, double value);

            // Set the RGB values (not the alpha)
            void setRGBValues(const float* values);

            // Set the RGBA values
            void setRGBAValues(const float* values);
            void setRGBAValues(const double* values);

            // Get the offsets
            inline const Offsets& getOffsets() const
            {
                return m_offsets;
            }

            // Get the offsets
            inline Offsets& getOffsets()
            {
                return m_offsets;
            }

            // Get the offset value at index
            double getOffsetValue(unsigned index) const;

            // Set the rgb offset values
            inline void setRGBOffsets(const float *offsets)
            {
                m_offsets.setRGBValues(offsets);
            }

            // Set the rgba offset values
            inline void setRGBAOffsets(const float *offsets)
            {
                m_offsets.setRGBAValues(offsets);
            }

            // Set all of the offset values
            inline void setOffsets(const Offsets& offsets)
            {
                m_offsets = offsets;
            }

            // Set an offset value
            void setOffsetValue(unsigned index, double value);

            // Validate the state of the instance
            void validate() const;

            // Get the operation type
            virtual OpData::OpType getOpType() const;

            // Get the operation type name
            const std::string& getOpTypeName() const;

            // True if the op only does bit-depth conversion
            bool isIdentity() const;

            virtual bool isClamping() const
            {
                return false;
            }

            // True if matrix is identity (not considering offset)
            bool isMatrixIdentity() const;

            // Make an op to replace an identity (or pair identity) of
            // this op type. (Note: For a pair identity, call 
            // this on the first half and then set the result's output
            // bit-depth to match the second half.)
            // returns the opData (to be managed by caller)
            virtual std::auto_ptr<OpData> getIdentityReplacement() const;

            // Test if the array coefs are 1 on diagonal and 0 elsewhere
            // (does not consider the offsets)
            bool isEye() const;

            // Is it a diagonal matrix (off-diagonal coefs are 0) ?
            bool isDiagonal() const;

            // Has offsets ?
            inline bool hasOffsets() const { return m_offsets.isNotNull(); }

            // Has alpha color component ?
            bool hasAlpha() const;

            // Determine whether the output of the op mixes R, G, B channels.
            // For example, Rout = 5*Rin is channel independent,
            // but Rout = Rin + Gin is not.
            // Note that the property may depend on the op parameters,
            // so, e.g. MatrixOps may sometimes return true and other times false.
            // returns true if the op's output does not combine input channels
            bool hasChannelCrosstalk() const { return !isDiagonal(); }

            // Set the output bit depth
            virtual void setOutputBitDepth(BitDepth out);

            // Set the input bit depth
            virtual void setInputBitDepth(BitDepth in);

            // Functional composition of two MatrixOps.
            // - B is the MatrixOp in the opList following this.
            // returns a new instance (to be managed by the caller)
            OpDataMatrixRcPtr compose(const Matrix& B) const;

            // Used by composition to remove small errors.
            // - offsetScale used to judge errors in the offsets.
            void cleanUp(double offsetScale);

            // Check if a Matrix is equal to this instance.
            bool operator==(const OpData& other) const;

            // Get the corresponding inverse operations
            // - ops an opList to which are appended the ops (may be >1) 
            //   needed to invert the op instance
            void inverse(OpDataVec& ops) const;


        private:

            class MatrixArray;

            // Matrix array shared pointer
            typedef OCIO_SHARED_PTR<MatrixArray> MatrixArrayPtr;

            class MatrixArray : public ArrayDouble
            {
            public:
                // Constructor
                MatrixArray(BitDepth inBitDepth,
                            BitDepth outBitDepth,
                            unsigned dimension,
                            unsigned numColorComponents);

                // Destructor
                ~MatrixArray();

                // Assignation operator
                MatrixArray& operator=(const ArrayDouble& a);

                // Is identity matrix?
                bool isIdentity() const;

                // Validate the state of the instance
                void validate() const;

                // Get the number of values in the array
                unsigned getNumValues() const;

                // Inner product (multiplication) of Matrix A (this) 
                // times Matrix B
                // - B is the matrix to multiply
                // returns a new instance (managed by a smart pointer)
                MatrixArrayPtr inner(const MatrixArray& B) const;

                // Inner product (multiplication) of Matrix A (this) times Offsets
                // - b is the vector of offsets to multiply
                // - out is used to return the results
                void inner(const Offsets& b, Offsets& out) const;

                // Get the matrix inverse
                // The inversion algorithm uses the Gauss-Jordan method.
                // returns a new instance (managed by a smart pointer)
                MatrixArrayPtr inverse() const;

                // Set the RGB values (not the alpha)
                void setRGBValues(const float* values);

                // Set the RGBA values
                void setRGBAValues(const float* values);
                void setRGBAValues(const double* values);

                // Set the output bit depth
                void setOutputBitDepth(BitDepth out);

                // Set the input bit depth
                void setInputBitDepth(BitDepth in);

            protected:
                // Fill the Matrix with appropriate values
                void fill();

                // Expand array from 3x3 to a 4x4 matrix
                // which is the default in Syncolor
                void expandFrom3x3To4x4();

            private:
                BitDepth m_inBitDepth;
                BitDepth m_outBitDepth;

            };

            MatrixArray m_array;  // All Matrix values
            Offsets m_offsets;    // All offset values
        };


    } // exit OpData namespace
}
OCIO_NAMESPACE_EXIT

#endif
