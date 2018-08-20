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



#include <OpenColorIO/OpenColorIO.h>

#include "../Platform.h"
#include "../BitDepthUtils.h"
#include "../MathUtils.h"

#include "OpDataMatrix.h"

#include <sstream>

OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace OpData
{

Matrix::Offsets::Offsets()
{
    memset(m_values, 0, 4 * sizeof(double));
}

Matrix::Offsets::Offsets(const Offsets& o)
{
    memcpy(m_values, o.m_values, 4 * sizeof(double));
}

Matrix::Offsets::~Offsets()
{
}

Matrix::Offsets& Matrix::Offsets::operator=(const Offsets& o)
{
    if (this != &o)
    {
        memcpy(m_values, o.m_values, 4 * sizeof(double));
    }
    return *this;
}

void Matrix::Offsets::setRGBValues(const float* v3)
{
    if (!v3)
    {
        throw Exception("Matrix: setRGBValues NULL pointer.");
    }

    m_values[0] = v3[0];
    m_values[1] = v3[1];
    m_values[2] = v3[2];
    m_values[3] = 0.;
}

void Matrix::Offsets::setRGBAValues(const float* v4)
{
    if (!v4)
    {
        throw Exception("Matrix: setRGBAValues NULL pointer.");
    }

    m_values[0] = v4[0];
    m_values[1] = v4[1];
    m_values[2] = v4[2];
    m_values[3] = v4[3];
}

bool Matrix::Offsets::operator==(const Offsets& o) const
{
    return (memcmp(m_values, o.m_values, 4 * sizeof(double)) == 0);
}

bool Matrix::Offsets::isNotNull() const
{
    static const double zero4[] = { 0., 0., 0., 0. };
    return (memcmp(m_values, zero4, 4 * sizeof(double)) != 0);
}

void Matrix::Offsets::scale(double s)
{
    for (unsigned int i = 0; i < 4; ++i)
    {
        m_values[i] *= s;
    }
}

Matrix::MatrixArray::MatrixArray(BitDepth inBitDepth,
                                    BitDepth outBitDepth,
                                    unsigned dimension,
                                    unsigned numColorComponents)
    : m_inBitDepth(inBitDepth)
    , m_outBitDepth(outBitDepth)
{
    resize(dimension, numColorComponents);
    fill();
}

Matrix::MatrixArray::~MatrixArray()
{
}

Matrix::MatrixArray& Matrix::MatrixArray::operator=(const ArrayDouble& a)
{
    if (this == &a) return *this;

    *(ArrayDouble*)this = a;

    validate();

    return *this;
}

Matrix::MatrixArrayPtr
    Matrix::MatrixArray::inner(const MatrixArray& B) const
{
    // Use operator= to make sure we have a 4x4 copy
    // of the original matrices.
    MatrixArray A_4x4 = *this;
    MatrixArray B_4x4 = B;
    const ArrayDouble::Values& Avals = A_4x4.getValues();
    const ArrayDouble::Values& Bvals = B_4x4.getValues();
    const unsigned dim = 4;

    MatrixArrayPtr OutPtr(new MatrixArray(
        BIT_DEPTH_F32,
        BIT_DEPTH_F32,
        dim,
        4));
    ArrayDouble::Values& Ovals = OutPtr->getValues();

    // Note: The matrix elements are stored in the vector
    // in row-major order,
    // [ a00, a01, a02, a03, a10, a11, a12, a13, a20, ... a44 ];
    for (unsigned row = 0; row<dim; ++row)
    {
        for (unsigned col = 0; col<dim; ++col)
        {
            double accum = 0.;
            for (unsigned i = 0; i<dim; ++i)
            {
                accum += Avals[row * dim + i] * Bvals[i * dim + col];
            }
            Ovals[row * dim + col] = accum;
        }
    }

    return OutPtr;
}

void Matrix::MatrixArray::inner(const Matrix::Offsets& b,
                                Offsets& out) const
{
    const unsigned dim = getLength();
    const ArrayDouble::Values& Avals = getValues();

    for (unsigned i = 0; i<dim; ++i)
    {
        double accum = 0.;
        for (unsigned j = 0; j<dim; ++j)
        {
            accum += Avals[i * dim + j] * b[j];
        }
        out[i] = accum;
    }
}

Matrix::MatrixArrayPtr
    Matrix::MatrixArray::inverse() const
{
    // Call validate to ensure that the matrix is 4x4,
    // will be expanded if only 3x3
    validate();

    MatrixArray t(*this);

    const unsigned dim = 4;
    // Create a new matrix array, with swapped input/output bitdepths
    // The new matrix is initialized as identity
    MatrixArrayPtr invPtr(new MatrixArray(
        m_outBitDepth,
        m_inBitDepth,
        dim,
        4));
    MatrixArray & s = *invPtr;

    // Inversion starts with identity (without bit depth scaling)
    s[0] = 1.;
    s[5] = 1.;
    s[10] = 1.;
    s[15] = 1.;

    // From Imath
    // Code copied from Matrix44<T>::gjInverse (bool singExc) const in ImathMatrix.h

    // Forward elimination

    for (int i = 0; i < 3; i++)
    {
        int pivot = i;

        double pivotsize = t[i*dim + i];

        if (pivotsize < 0)
            pivotsize = -pivotsize;

        for (int j = i + 1; j < 4; j++)
        {
            double tmp = t[j*dim + i];

            if (tmp < 0.0)
                tmp = -tmp;

            if (tmp > pivotsize)
            {
                pivot = j;
                pivotsize = tmp;
            }
        }

        if (pivotsize == 0.0)
        {
            throw Exception("Singular Matrix can't be inverted.");
        }

        if (pivot != i)
        {
            for (int j = 0; j < 4; j++)
            {
                double tmp;

                tmp = t[i*dim + j];
                t[i*dim + j] = t[pivot*dim + j];
                t[pivot*dim + j] = tmp;

                tmp = s[i*dim + j];
                s[i*dim + j] = s[pivot*dim + j];
                s[pivot*dim + j] = tmp;
            }
        }

        for (int j = i + 1; j < 4; j++)
        {
            double f = t[j*dim + i] / t[i*dim + i];

            for (int k = 0; k < 4; k++)
            {
                t[j*dim + k] -= f * t[i*dim + k];
                s[j*dim + k] -= f * s[i*dim + k];
            }
        }
    }

    // Backward substitution

    for (int i = 3; i >= 0; --i)
    {
        double f;
                
        // TODO: Perhaps change to throw even if f is near
        //       zero (nearly singular).
        if ((f = t[i*dim + i]) == 0.0)
        {
            throw Exception("Singular Matrix can't be inverted.");
        }

        for (int j = 0; j < 4; j++)
        {
            t[i*dim + j] /= f;
            s[i*dim + j] /= f;
        }

        for (int j = 0; j < i; j++)
        {
            f = t[j*dim + i];

            for (int k = 0; k < 4; k++)
            {
                t[j*dim + k] -= f * t[i*dim + k];
                s[j*dim + k] -= f * s[i*dim + k];
            }
        }
    }

    return invPtr;
}

unsigned Matrix::MatrixArray::getNumValues() const
{
    return getLength() * getLength();
}

bool Matrix::MatrixArray::isIdentity() const
{
    const unsigned dim = getLength();
    const ArrayDouble::Values& values = getValues();

    for (unsigned i = 0; i<dim; ++i)
    {
        for (unsigned j = 0; j<dim; ++j)
        {
            if (i == j)
            {
                if (values[i*dim + j] != 1.0)  // Strict comparison intended
                {
                    return false;
                }
            }
            else
            {
                if (values[i*dim + j] != 0.0)  // Strict comparison intended
                {
                    return false;
                }
            }
        }
    }

    return true;
}

void Matrix::MatrixArray::fill()
{
    const unsigned dim = getLength();
    ArrayDouble::Values& values = getValues();

    const double scaleFactor
        = (double)GetBitDepthMaxValue(m_outBitDepth)
        / (double)GetBitDepthMaxValue(m_inBitDepth);

    memset(&values[0], 0, values.size() * sizeof(double));

    for (unsigned i = 0; i<dim; ++i)
    {
        for (unsigned j = 0; j<dim; ++j)
        {
            if (i == j)
            {
                values[i*dim + j] = scaleFactor;
            }
        }
    }
}

void Matrix::MatrixArray::expandFrom3x3To4x4()
{
    const Values oldValues = getValues();

    resize(4, 4);
    Values& v = getValues();

    v[0] = oldValues[0];
    v[1] = oldValues[1];
    v[2] = oldValues[2];
    v[3] = 0.0;

    v[4] = oldValues[3];
    v[5] = oldValues[4];
    v[6] = oldValues[5];
    v[7] = 0.0;

    v[8] = oldValues[6];
    v[9] = oldValues[7];
    v[10] = oldValues[8];
    v[11] = 0.0;

    const double scaleFactor
        = (double)GetBitDepthMaxValue(m_outBitDepth)
        / (double)GetBitDepthMaxValue(m_inBitDepth);

    v[12] = 0.0;
    v[13] = 0.0;
    v[14] = 0.0;
    v[15] = scaleFactor;
}

void Matrix::MatrixArray::setRGBValues(const float* values)
{
    Values& v = getValues();

    v[0] = values[0];
    v[1] = values[1];
    v[2] = values[2];
    v[3] = 0.0;

    v[4] = values[3];
    v[5] = values[4];
    v[6] = values[5];
    v[7] = 0.0;

    v[8] = values[6];
    v[9] = values[7];
    v[10] = values[8];
    v[11] = 0.0;

    const double scaleFactor
        = (double)GetBitDepthMaxValue(m_outBitDepth)
        / (double)GetBitDepthMaxValue(m_inBitDepth);

    v[12] = 0.0;
    v[13] = 0.0;
    v[14] = 0.0;
    v[15] = scaleFactor;
}

void Matrix::MatrixArray::setRGBAValues(const float* values)
{
    Values& v = getValues();

    v[0] = values[0];
    v[1] = values[1];
    v[2] = values[2];
    v[3] = values[3];

    v[4] = values[4];
    v[5] = values[5];
    v[6] = values[6];
    v[7] = values[7];

    v[8] = values[8];
    v[9] = values[9];
    v[10] = values[10];
    v[11] = values[11];

    v[12] = values[12];
    v[13] = values[13];
    v[14] = values[14];
    v[15] = values[15];
}

void Matrix::MatrixArray::setRGBAValues(const double* values)
{
    Values& v = getValues();
    memcpy(&v[0], values, 16 * sizeof(double));
}

void Matrix::MatrixArray::validate() const
{
    ArrayDouble::validate();

    // A 4x4 matrix is the canonical form, convert if it is only a 3x3.
    if (getLength() == 3)
    {
        const_cast<MatrixArray*>(this)->expandFrom3x3To4x4();
    }
    else if (getLength() != 4)
    {
        throw Exception("Matrix: array content issue.");
    }

    if (getNumColorComponents() != 4)
    {
        throw Exception("Matrix: Dimensions must be 4x4.");
    }
}


void Matrix::MatrixArray::setOutputBitDepth(BitDepth out)
{
    // Scale factor is max_new_depth/max_old_depth
    const double scaleFactor
        = (double)GetBitDepthMaxValue(out)
        / (double)GetBitDepthMaxValue(m_outBitDepth);

    // Save the new outputbitdepth
    m_outBitDepth = out;

    // Scale the values
    Values& v = getValues();
    const size_t size = v.size();

    for (unsigned i = 0; i < size; ++i) {
        v[i] *= scaleFactor;
    }
}

void Matrix::MatrixArray::setInputBitDepth(BitDepth in)
{
    // Scale factor is max_old_depth/max_new_depth
    const double scaleFactor
        = (double)GetBitDepthMaxValue(m_inBitDepth)
        / (double)GetBitDepthMaxValue(in);

    // Save the new inputbitdepth
    m_inBitDepth = in;

    // Scale the values
    Values& v = getValues();
    const size_t size = v.size();

    for (unsigned i = 0; i < size; ++i) {
        v[i] *= scaleFactor;
    }
}

////////////////////////////////////////////////

Matrix::Matrix()
    : OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    , m_array(getInputBitDepth(), getOutputBitDepth(), 4, 4)
{
}

Matrix::Matrix(BitDepth inBitDepth,
                BitDepth outBitDepth)
    : OpData(inBitDepth, outBitDepth)
    , m_array(getInputBitDepth(), getOutputBitDepth(), 4, 4)
{
}

Matrix::Matrix(BitDepth inBitDepth,
                BitDepth outBitDepth,
                const std::string& id,
                const std::string& name,
                const Descriptions& descriptions)
    : OpData(inBitDepth, outBitDepth, id, name, descriptions)
    , m_array(getInputBitDepth(), getOutputBitDepth(), 4, 4)
{
}

Matrix::~Matrix()
{
}

OpData* Matrix::clone(CloneType) const
{
    return new Matrix(*this);
}

const std::string& Matrix::getOpTypeName() const
{
    static const std::string type("Matrix");
    return type;
}

OpData::OpType Matrix::getOpType() const
{
    return OpData::MatrixType;
}

void Matrix::setArrayValue(unsigned index, double value)
{
    m_array.getValues()[index] = value;
}

void  Matrix::setRGBValues(const float* values)
{
    m_array.setRGBValues(values);
}

void  Matrix::setRGBAValues(const float* values)
{
    m_array.setRGBAValues(values);
}

void Matrix::setRGBAValues(const double* values)
{
    m_array.setRGBAValues(values);
}

void Matrix::validate() const
{
    OpData::validate();

    try
    {
        m_array.validate();
    }
    catch (Exception& e)
    {
        std::ostringstream oss;
        oss << "Matrix array content issue: ";
        oss << e.what();

        throw Exception(oss.str().c_str());
    }

    if (m_array.getNumColorComponents() != 4)
    {
        throw Exception("Matrix: missing color component.");
    }

    if (m_array.getLength() != 4)
    {
        throw Exception("Matrix: array content issue.");
    }
}

// We do a number of exact floating-point comparisons in the following
// methods. Note that this op may be used to do very fine adjustments
// to pixels. Therefore it is problematic to attempt to judge values
// passed in from a user's transform as to whether they are "close enough"
// to e.g. 1 or 0. However, we still want to allow a matrix and its
// inverse to be composed and be able to call the result an identity
// (recognizing it won't quite be). Therefore, the strategy here is to do
// exact compares on users files but to "clean up" matrices as part of
// composition to make this work in practice. The concept is that the
// tolerances are moved to where errors are introduced rather than
// indiscriminately applying them to all user ops.

// Since "Identity" has a generic meaning for all ops, we use the Matlab
// term "Eye" to refer to the case where the array coefficients are 1 on
// the diagonal an 0 elsewhere.
bool Matrix::isEye() const
{
    return m_array.isIdentity();
}

// For all ops, an "Identity" is an op that only does bit-depth conversion
// and is therefore a candidate for the optimizer to remove.
bool Matrix::isIdentity() const
{
    if (hasOffsets() || hasAlpha() || !isDiagonal())
    {
        return false;
    }

    return isMatrixIdentity();
}
         
bool Matrix::isMatrixIdentity() const
{
    // Now check the diagonal elements
    const double scaleFactor
        = (double)GetBitDepthMaxValue(getOutputBitDepth())
        / (double)GetBitDepthMaxValue(getInputBitDepth());

    const double maxDiff = scaleFactor * 1e-6;

    const ArrayDouble& a = getArray();
    const ArrayDouble::Values& m = a.getValues();
    const unsigned dim = a.getLength();

    for (unsigned i = 0; i<dim; ++i)
    {
        for (unsigned j = 0; j<dim; ++j)
        {
            if (i == j)
            {
                if (!equalWithAbsError(m[i*dim + j],
                                        scaleFactor,
                                        maxDiff))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool Matrix::isDiagonal() const
{
    const ArrayDouble& a = getArray();
    const ArrayDouble::Values& m = a.getValues();
    const unsigned max = a.getNumValues();
    const unsigned dim = a.getLength();

    for (unsigned idx = 0; idx<max; ++idx)
    {
        if ((idx % (dim + 1)) != 0) // Not on the diagonal
        {
            if (m[idx] != 0.0) // Strict comparison intended
            {
                return false;
            }
        }
    }

    return true;
}

bool Matrix::hasAlpha() const
{
    const ArrayDouble& a = getArray();
    const ArrayDouble::Values& m = a.getValues();

    // Now check the diagonal elements
    const double scaleFactor
        = (double)GetBitDepthMaxValue(getOutputBitDepth())
        / (double)GetBitDepthMaxValue(getInputBitDepth());
    const double maxDiff = scaleFactor * 1e-6;

    return

        // Last column
        (m[3] != 0.0) || // Strict comparison intended
        (m[7] != 0.0) ||
        (m[11] != 0.0) ||

        // Diagonal
        !equalWithAbsError(m[15], scaleFactor, maxDiff) ||

        // Bottom row
        (m[12] != 0.0) || // Strict comparison intended
        (m[13] != 0.0) ||
        (m[14] != 0.0);
}

Matrix* Matrix::CreateDiagonalMatrix(
    BitDepth inBitDepth,
    BitDepth outBitDepth,
    double diagValue)
{
    // Create a matrix with no offset
    Matrix* pM = new Matrix(inBitDepth, outBitDepth);
    pM->setName("Diagonal matrix");

    pM->validate();

    pM->setArrayValue(0, diagValue);
    pM->setArrayValue(5, diagValue);
    pM->setArrayValue(10, diagValue);
    pM->setArrayValue(15, diagValue);

    return pM;
}

double Matrix::getOffsetValue(unsigned index) const
{
    const unsigned dim = getArray().getLength();
    if (index >= dim)
    {
        // TODO: should never happen. Consider assert.
        std::ostringstream oss;
        oss << "Matrix array content issue: '";
        oss << getMeaningfullIdentifier().c_str();
        oss << "' offset index out of range '";
        oss << index;
        oss << "'. ";

        throw Exception(oss.str().c_str());
    }

    return m_offsets[index];
}

void Matrix::setOffsetValue(unsigned index, double value)
{
    const unsigned dim = getArray().getLength();
    if (index >= dim)
    {
        // TODO: should never happen. Consider assert.
        std::ostringstream oss;
        oss << "Matrix array content issue: '";
        oss << getMeaningfullIdentifier().c_str();
        oss << "' offset index out of range '";
        oss << index;
        oss << "'. ";

        throw Exception(oss.str().c_str());
    }

    m_offsets[index] = value;
}

void Matrix::setOutputBitDepth(BitDepth out)
{
    // Scale factor is max_new_depth/max_old_depth
    const double scaleFactor
        = (double)GetBitDepthMaxValue(out)
        / (double)GetBitDepthMaxValue(getOutputBitDepth());

    // Call parent to set the outputbitdepth
    OpData::setOutputBitDepth(out);

    // set new outputbitdepth to the array
    m_array.setOutputBitDepth(out);

    // Scale the offsets
    m_offsets.scale(scaleFactor);
}

void Matrix::setInputBitDepth(BitDepth in)
{
    // Call parent to set the inputbitdepth
    OpData::setInputBitDepth(in);

    // set new outputbitdepth to the array
    m_array.setInputBitDepth(in);
}

OpDataMatrixRcPtr Matrix::compose(const Matrix& B) const
{
    if (getOutputBitDepth() != B.getInputBitDepth())
    {
        std::ostringstream oss;
        oss << "Matrix bitdepth missmatch between '";
        oss << getMeaningfullIdentifier();
        oss << "' and '";
        oss << B.getMeaningfullIdentifier();
        oss << "'. ";

        throw Exception(oss.str().c_str());
    }

    // Ensure that both matrices will have the right dimension (ie. 4x4)
    if (m_array.getLength() != 4 || B.m_array.getLength() != 4)
    {
        // TODO: should never happen. Consider assert.
        throw Exception("Matrix: array content issue.");
    }

    Descriptions newDesc = getDescriptions();
    newDesc += B.getDescriptions();
    OpDataMatrixRcPtr Out(
        new Matrix(getInputBitDepth(), B.getOutputBitDepth()));
    Out->setId(getId() + B.getId());
    Out->setName(getName() + B.getName());
    Out->getDescriptions() = newDesc;
    // TODO: May want to revisit how the metadata is set.

    // By definition, A.compose(B) implies that op A precedes op B
    // in the opList. The LUT format coefficients follow matrix math:
    // vec2 = A x vec1 where A is 3x3 and vec is 3x1.
    // So the composite operation in matrix form is vec2 = B x A x vec1.
    // Hence we compute B x A rather than A x B.

    MatrixArrayPtr OutPtr = B.m_array.inner(this->m_array);

    Out->getArray() = *OutPtr.get();

    // Compute matrix B times offsets from A.

    Matrix::Offsets offs;

    B.m_array.inner(getOffsets(), offs);

    const unsigned dim = this->m_array.getLength();

    // Determine overall scaling of the offsets prior to any catastrophic
    // cancellation that may occur during the add.
    double val, max_val = 0.;
    for (unsigned i = 0; i<dim; ++i)
    {
        val = fabs(offs[i]);
        max_val = max_val > val ? max_val : val;
        val = fabs(B.getOffsets()[i]);
        max_val = max_val > val ? max_val : val;
    }

    // Add offsets from B.
    for (unsigned i = 0; i<dim; ++i)
    {
        offs[i] += B.getOffsets()[i];
    }

    Out->setOffsets(offs);

    // To enable use of strict float comparisons above, we adjust the
    // result so that values very near integers become exactly integers.
    Out->cleanUp(max_val);

    return Out;
}

void Matrix::cleanUp(double offsetScale)
{
    const ArrayDouble& a = getArray();
    const ArrayDouble::Values& m = a.getValues();
    const unsigned dim = a.getLength();

    // Estimate the magnitude of the matrix
    double max_val = 0.;
    for (unsigned i = 0; i<dim; ++i)
    {
        for (unsigned j = 0; j<dim; ++j)
        {
            const double val = fabs(m[i * dim + j]);
            max_val = max_val > val ? max_val : val;
        }
    }

    // Determine an absolute tolerance
    const double scale = max_val > 1e-4 ? max_val : 1e-4;
    const double abs_tol = scale * 1e-6;

    // Replace values that are close to integers by exact values
    for (unsigned i = 0; i<dim; ++i)
    {
        for (unsigned j = 0; j<dim; ++j)
        {
            const double val = m[i * dim + j];
            const double round_val = round(val);
            const double diff = fabs(val - round_val);
            if (diff < abs_tol)
            {
                setArrayValue(i * dim + j, round_val);
            }
        }
    }

    // Do likewise for the offsets
    const double scale2 = offsetScale > 1e-4 ? offsetScale : 1e-4;
    const double abs_tol2 = scale2 * 1e-6;

    for (unsigned i = 0; i<dim; ++i)
    {
        const double val = getOffsets()[i];
        const double round_val = round(val);
        const double diff = fabs(val - round_val);
        if (diff < abs_tol2)
        {
            setOffsetValue(i, round_val);
        }
    }
}

bool Matrix::operator==(const OpData& other) const
{
    if (this == &other) return true;
    if (getOpType() != other.getOpType()) return false;

    const Matrix* mop = static_cast<const Matrix*>(&other);

    return (OpData::operator==(other) &&
        m_array == mop->m_array &&
        m_offsets == mop->m_offsets);
}

void Matrix::inverse(OpDataVec & v) const
{
    // Get the inverse matrix
    MatrixArrayPtr invMatrixArray = m_array.inverse();
    // MatrixArray::inverse() will throw for singular matrices

    // Calculate the inverse offset
    const Offsets& offsets = getOffsets();
    Matrix::Offsets invOffsets;
    if (offsets.isNotNull())
    {
        invMatrixArray->inner(offsets, invOffsets);
        invOffsets.scale(-1);
    }

    Matrix* invOp = 0x0;
    try
    {
        invOp = new Matrix(getOutputBitDepth(), getInputBitDepth());
        invOp->setRGBAValues(&(invMatrixArray->getValues()[0]));
        invOp->setOffsets(invOffsets);
    }
    catch (const Exception& e)
    {
        delete invOp;
        throw e;
    }

    // No need to call validate(), the invOp will have proper dimension,
    // bitdepths, matrix and offets values.

    v.append(invOp);
}

std::auto_ptr<OpData> Matrix::getIdentityReplacement() const
{
    return std::auto_ptr<OpData>(
        new Matrix(getInputBitDepth(), getOutputBitDepth()));
}
} // exit OpData namespace
}
OCIO_NAMESPACE_EXIT


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "../UnitTest.h"

OIIO_ADD_TEST(OpDataMatrix, matrixOp_empty_test)
{
    OCIO::OpData::Matrix m;
    OIIO_CHECK_ASSERT(m.isNoOp());
    OIIO_CHECK_ASSERT(m.isEye());
    OIIO_CHECK_ASSERT(m.isDiagonal());
    OIIO_CHECK_NO_THROW(m.validate());
    OIIO_CHECK_EQUAL(m.getOpTypeName(), "Matrix");

    OIIO_CHECK_EQUAL(m.getArray().getLength(), (unsigned)4);
    OIIO_CHECK_EQUAL(m.getArray().getNumValues(), (unsigned)16);
    OIIO_CHECK_EQUAL(m.getArray().getNumColorComponents(), (unsigned)4);

    m.getArray().resize(3, 3);

    OIIO_CHECK_EQUAL(m.getArray().getNumValues(), (unsigned)9);
    OIIO_CHECK_EQUAL(m.getArray().getLength(), (unsigned)3);
    OIIO_CHECK_EQUAL(m.getArray().getNumColorComponents(), (unsigned)3);
    OIIO_CHECK_NO_THROW(m.validate());
}

OIIO_ADD_TEST(OpDataMatrix, matrixOp_accessors_test)
{
    OCIO::OpData::Matrix m(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_ASSERT(m.isNoOp());
    OIIO_CHECK_ASSERT(m.isEye());
    OIIO_CHECK_ASSERT(m.isDiagonal());
    OIIO_CHECK_ASSERT(m.isIdentity());
    OIIO_CHECK_NO_THROW(m.validate());

    m.setArrayValue(15, 1 + 1e-5f);

    OIIO_CHECK_ASSERT(!m.isNoOp());
    OIIO_CHECK_ASSERT(!m.isEye());
    OIIO_CHECK_ASSERT(m.isDiagonal());
    OIIO_CHECK_ASSERT(!m.isIdentity());
    OIIO_CHECK_NO_THROW(m.validate());

    m.setArrayValue(1, 1e-5f);
    m.setArrayValue(15, 1.0f);

    OIIO_CHECK_ASSERT(!m.isNoOp());
    OIIO_CHECK_ASSERT(!m.isEye());
    OIIO_CHECK_ASSERT(!m.isDiagonal());
    OIIO_CHECK_ASSERT(!m.isIdentity());
    OIIO_CHECK_NO_THROW(m.validate());
}

OIIO_ADD_TEST(OpDataMatrix, matrixOp_offsets_test)
{
    OCIO::OpData::Matrix m(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_ASSERT(m.isNoOp());
    OIIO_CHECK_ASSERT(m.isEye());
    OIIO_CHECK_ASSERT(m.isDiagonal());
    OIIO_CHECK_ASSERT(!m.hasOffsets());
    OIIO_CHECK_NO_THROW(m.validate());

    m.setOffsetValue(2, 1.0f);
    OIIO_CHECK_ASSERT(!m.isNoOp());
    OIIO_CHECK_ASSERT(m.isEye());
    OIIO_CHECK_ASSERT(m.isDiagonal());
    OIIO_CHECK_ASSERT(m.hasOffsets());
    OIIO_CHECK_NO_THROW(m.validate());
    OIIO_CHECK_EQUAL(m.getOffsets()[2], 1.0f);
}

OIIO_ADD_TEST(OpDataMatrix, matrixOp_offsets4_test)
{
    OCIO::OpData::Matrix m(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_ASSERT(m.isNoOp());
    OIIO_CHECK_ASSERT(m.isEye());
    OIIO_CHECK_ASSERT(m.isDiagonal());
    OIIO_CHECK_ASSERT(!m.hasOffsets());
    OIIO_CHECK_NO_THROW(m.validate());

    m.setOffsetValue(3, -1e-6f);
    OIIO_CHECK_ASSERT(!m.isNoOp());
    OIIO_CHECK_ASSERT(m.isEye());
    OIIO_CHECK_ASSERT(m.isDiagonal());
    OIIO_CHECK_ASSERT(m.hasOffsets());
    OIIO_CHECK_NO_THROW(m.validate());
    OIIO_CHECK_EQUAL(m.getOffsets()[3], -1e-6f);
}

OIIO_ADD_TEST(OpDataMatrix, matrixOp_diagonal_test)
{
    std::auto_ptr<OCIO::OpData::Matrix> pM;
    pM.reset(
        OCIO::OpData::Matrix::CreateDiagonalMatrix(OCIO::BIT_DEPTH_UINT8, 
                                                   OCIO::BIT_DEPTH_UINT8, 
                                                   0.5));

    OIIO_CHECK_ASSERT(pM->isDiagonal());
    OIIO_CHECK_ASSERT(!pM->hasOffsets());
    OIIO_CHECK_NO_THROW(pM->validate());
    OIIO_CHECK_EQUAL(pM->getArray().getValues()[0], 0.5);
    OIIO_CHECK_EQUAL(pM->getArray().getValues()[5], 0.5);
    OIIO_CHECK_EQUAL(pM->getArray().getValues()[10], 0.5);
    OIIO_CHECK_EQUAL(pM->getArray().getValues()[15], 0.5);
}

OIIO_ADD_TEST(OpDataMatrix, matrixOp_clone_test)
{
    OCIO::OpData::Matrix ref;
    ref.setOffsetValue(2, 1.0f);
    ref.setArrayValue(0, 2.0f);

    std::auto_ptr<OCIO::OpData::Matrix> pClone;
    pClone.reset(dynamic_cast<OCIO::OpData::Matrix*>(
        ref.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY)));

    OIIO_CHECK_ASSERT(pClone.get());
    OIIO_CHECK_ASSERT(!pClone->isNoOp());
    OIIO_CHECK_ASSERT(!pClone->isEye());
    OIIO_CHECK_ASSERT(pClone->isDiagonal());
    OIIO_CHECK_NO_THROW(pClone->validate());
    OIIO_CHECK_EQUAL(pClone->getOpTypeName(), "Matrix");
    OIIO_CHECK_EQUAL(pClone->getOffsets()[0], 0.0f);
    OIIO_CHECK_EQUAL(pClone->getOffsets()[1], 0.0f);
    OIIO_CHECK_EQUAL(pClone->getOffsets()[2], 1.0f);
    OIIO_CHECK_EQUAL(pClone->getOffsets()[3], 0.0f);
    OIIO_CHECK_ASSERT(pClone->getArray() == ref.getArray());
}

OIIO_ADD_TEST(OpDataMatrix, matrixOp_clone_offsets4_test)
{
    OCIO::OpData::Matrix ref;
    ref.setOffsetValue(0, 1.0f);
    ref.setOffsetValue(1, 2.0f);
    ref.setOffsetValue(2, 3.0f);
    ref.setOffsetValue(3, 4.0f);
    ref.setArrayValue(0, 2.0f);

    std::auto_ptr<OCIO::OpData::Matrix> pClone;
    pClone.reset(dynamic_cast<OCIO::OpData::Matrix*>(
        ref.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY)));

    OIIO_CHECK_ASSERT(pClone.get());
    OIIO_CHECK_ASSERT(!pClone->isNoOp());
    OIIO_CHECK_ASSERT(!pClone->isEye());
    OIIO_CHECK_ASSERT(pClone->isDiagonal());
    OIIO_CHECK_NO_THROW(pClone->validate());
    OIIO_CHECK_ASSERT(pClone->getOpTypeName() == "Matrix");
    OIIO_CHECK_EQUAL(pClone->getOffsets()[0], 1.0f);
    OIIO_CHECK_EQUAL(pClone->getOffsets()[1], 2.0f);
    OIIO_CHECK_EQUAL(pClone->getOffsets()[2], 3.0f);
    OIIO_CHECK_EQUAL(pClone->getOffsets()[3], 4.0f);
    OIIO_CHECK_ASSERT(pClone->getArray() == ref.getArray());
}

OIIO_ADD_TEST(OpDataMatrix, matrixOp_diff_bitdepth_test)
{
    OCIO::OpData::Matrix m1(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_ASSERT(m1.isNoOp());
    OIIO_CHECK_ASSERT(m1.isEye());
    OIIO_CHECK_ASSERT(m1.isDiagonal());
    OIIO_CHECK_ASSERT(!m1.hasOffsets());
    OIIO_CHECK_NO_THROW(m1.validate());

    OCIO::OpData::Matrix m2(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_ASSERT(!m2.isNoOp());
    OIIO_CHECK_ASSERT(!m2.isEye());
    OIIO_CHECK_ASSERT(m2.isDiagonal());
    OIIO_CHECK_ASSERT(!m2.hasOffsets());
    OIIO_CHECK_NO_THROW(m2.validate());

    const double coeff
        = (double)OCIO::GetBitDepthMaxValue(m2.getOutputBitDepth())
        / (double)OCIO::GetBitDepthMaxValue(m2.getInputBitDepth());

    const double error = 1e-6f;
    // To validate the result, compute the expected results
    //  not using the MatrixOp algorithm
    //
    for (unsigned idx = 0; idx<3; ++idx)
    {
        OIIO_CHECK_CLOSE(m1.getArray().getValues()[idx * 5] * coeff,
            m2.getArray().getValues()[idx * 5], error);
    }
}

OIIO_ADD_TEST(OpDataMatrix, TestConstruct)
{
    OCIO::OpData::Matrix matOp;

    OIIO_CHECK_EQUAL(matOp.getName(), "");
    OIIO_CHECK_EQUAL(matOp.getId(), "");
    OIIO_CHECK_EQUAL(matOp.getOpType(), OCIO::OpData::OpData::MatrixType);
    OIIO_CHECK_EQUAL(matOp.getOpTypeName(), "Matrix");
    OIIO_CHECK_EQUAL(matOp.getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(matOp.getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_ASSERT(matOp.getDescriptions().getList().empty());
    OIIO_CHECK_EQUAL(matOp.getOffsets()[0], 0.0f);
    OIIO_CHECK_EQUAL(matOp.getOffsets()[1], 0.0f);
    OIIO_CHECK_EQUAL(matOp.getOffsets()[2], 0.0f);
    OIIO_CHECK_EQUAL(matOp.getOffsets()[3], 0.0f);
    OIIO_CHECK_EQUAL(matOp.getArray().getLength(), (unsigned)4);
    OIIO_CHECK_EQUAL(matOp.getArray().getNumColorComponents(), (unsigned)4);
    OIIO_CHECK_EQUAL(matOp.getArray().getNumValues(), (unsigned)16);
    const OCIO::OpData::ArrayDouble::Values & val = matOp.getArray().getValues();
    OIIO_CHECK_EQUAL(val.size(), 16);
    OIIO_CHECK_EQUAL(val[0], 1.0f);
    OIIO_CHECK_EQUAL(val[1], 0.0f);
    OIIO_CHECK_EQUAL(val[2], 0.0f);
    OIIO_CHECK_EQUAL(val[3], 0.0f);

    OIIO_CHECK_EQUAL(val[4], 0.0f);
    OIIO_CHECK_EQUAL(val[5], 1.0f);
    OIIO_CHECK_EQUAL(val[6], 0.0f);
    OIIO_CHECK_EQUAL(val[7], 0.0f);

    OIIO_CHECK_EQUAL(val[8], 0.0f);
    OIIO_CHECK_EQUAL(val[9], 0.0f);
    OIIO_CHECK_EQUAL(val[10], 1.0f);
    OIIO_CHECK_EQUAL(val[11], 0.0f);

    OIIO_CHECK_EQUAL(val[12], 0.0f);
    OIIO_CHECK_EQUAL(val[13], 0.0f);
    OIIO_CHECK_EQUAL(val[14], 0.0f);
    OIIO_CHECK_EQUAL(val[15], 1.0f);

    OIIO_CHECK_NO_THROW(matOp.validate());

    matOp.getArray().resize(3, 3); // validate() will resize to 4x4

    OIIO_CHECK_EQUAL(matOp.getArray().getNumValues(), (unsigned)9);
    OIIO_CHECK_EQUAL(matOp.getArray().getLength(), (unsigned)3);
    OIIO_CHECK_EQUAL(matOp.getArray().getNumColorComponents(), (unsigned)3);

    OIIO_CHECK_NO_THROW(matOp.validate());

    OIIO_CHECK_EQUAL(matOp.getArray().getNumValues(), (unsigned)16);
    OIIO_CHECK_EQUAL(matOp.getArray().getLength(), (unsigned)4);
    OIIO_CHECK_EQUAL(matOp.getArray().getNumColorComponents(), (unsigned)4);


    const OCIO::BitDepth bitDepth = OCIO::BIT_DEPTH_UINT8;

    OCIO::OpData::Matrix m(bitDepth, bitDepth);
    OIIO_CHECK_NO_THROW(m.validate());

    OIIO_CHECK_EQUAL(m.getInputBitDepth(), bitDepth);
    OIIO_CHECK_EQUAL(m.getOutputBitDepth(), bitDepth);
}

OIIO_ADD_TEST(OpDataMatrix, OutputDepthScaling)
{
    OCIO::OpData::Matrix ref(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8);

    // Set arbitrary matrix and offset values
    ref.setArrayValue(0, 3.24f);
    ref.setArrayValue(1, -1.537f);
    ref.setArrayValue(2, -0.49850f);
    ref.setArrayValue(3, 0);
    ref.setArrayValue(4, -0.9693f);
    ref.setArrayValue(5, 1.876f);
    ref.setArrayValue(6, 0.04156f);
    ref.setArrayValue(7, 0);
    ref.setArrayValue(8, 0.0556f);
    ref.setArrayValue(9, -0.2040f);
    ref.setArrayValue(10, 1.0573f);
    ref.setArrayValue(11, 0);
    ref.setArrayValue(12, 0);
    ref.setArrayValue(13, 0);
    ref.setArrayValue(14, 0);
    ref.setArrayValue(15, 1);

    ref.setOffsetValue(0, -0.111f);
    ref.setOffsetValue(1, 0.297f);
    ref.setOffsetValue(2, -0.118f);
    ref.setOffsetValue(3, 0.425f);

    //get the array values and keep them to compare later
    const OCIO::OpData::ArrayDouble::Values initialCoeff
        = ref.getArray().getValues();
    double initialOffset[4];

    initialOffset[0] = ref.getOffsets()[0];
    initialOffset[1] = ref.getOffsets()[1];
    initialOffset[2] = ref.getOffsets()[2];
    initialOffset[3] = ref.getOffsets()[3];

    const OCIO::BitDepth initialBitdepth = ref.getOutputBitDepth();

    const OCIO::BitDepth newBitdepth = OCIO::BIT_DEPTH_UINT16;

    const float factor = OCIO::GetBitDepthMaxValue(newBitdepth)
        / OCIO::GetBitDepthMaxValue(initialBitdepth);

    ref.setOutputBitDepth(newBitdepth);
    // now we need to make sure that the bitdepth was changed from
    // the overriden method.
    OIIO_CHECK_EQUAL(ref.getOutputBitDepth(), newBitdepth);

    // now we need to check that the scaling between the new coefficients and
    // initial coefficients matches the factor computed above.
    const OCIO::OpData::ArrayDouble::Values& newCoeff = ref.getArray().getValues();

    // sanity check first
    OIIO_CHECK_EQUAL(initialCoeff.size(), newCoeff.size());

    double expectedValue;

    // Coefficient check
    for (unsigned i = 0; i < newCoeff.size(); i++)
    {
        expectedValue = initialCoeff[i] * factor;
        OIIO_CHECK_EQUAL(expectedValue, newCoeff[i]);
    }

    // Offset check
    const unsigned dim = ref.getArray().getLength();
    for (unsigned i = 0; i<dim; i++) {
        expectedValue = initialOffset[i] * factor;
        OIIO_CHECK_EQUAL(expectedValue, ref.getOffsets()[i]);
    }
}

OIIO_ADD_TEST(OpDataMatrix, InputDepthScaling)
{
    OCIO::OpData::Matrix ref(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8);

    // Set arbitrary matrix and offset values
    ref.setArrayValue(0, 3.24f);
    ref.setArrayValue(1, -1.537f);
    ref.setArrayValue(2, -0.49850f);
    ref.setArrayValue(3, 0);
    ref.setArrayValue(4, -0.9693f);
    ref.setArrayValue(5, 1.876f);
    ref.setArrayValue(6, 0.04156f);
    ref.setArrayValue(7, 0);
    ref.setArrayValue(8, 0.0556f);
    ref.setArrayValue(9, -0.2040f);
    ref.setArrayValue(10, 1.0573f);
    ref.setArrayValue(11, 0);
    ref.setArrayValue(12, 0);
    ref.setArrayValue(13, 0);
    ref.setArrayValue(14, 0);
    ref.setArrayValue(15, 1);

    ref.setOffsetValue(0, -0.111f);
    ref.setOffsetValue(1, 0.297f);
    ref.setOffsetValue(2,-0.118f);
    ref.setOffsetValue(3, 0.425f);

    //get the array values and keep them to compare later
    const OCIO::OpData::ArrayDouble::Values initialCoeff
        = ref.getArray().getValues();
    double initialOffset[4];

    initialOffset[0] = ref.getOffsets()[0];
    initialOffset[1] = ref.getOffsets()[1];
    initialOffset[2] = ref.getOffsets()[2];
    initialOffset[3] = ref.getOffsets()[3];

    const OCIO::BitDepth initialBitdepth = ref.getInputBitDepth();

    const OCIO::BitDepth newBitdepth = OCIO::BIT_DEPTH_UINT16;

    const double factor = OCIO::GetBitDepthMaxValue(initialBitdepth)
        / OCIO::GetBitDepthMaxValue(newBitdepth);

    ref.setInputBitDepth(newBitdepth);
    // now we need to make sure that the bitdepth was changed from the
    // overriden method.
    OIIO_CHECK_EQUAL(ref.getInputBitDepth(), newBitdepth);

    // now we need to check that the scaling between the new coefficients and
    // initial coefficients matches the factor computed above.
    const OCIO::OpData::ArrayDouble::Values& newCoeff = ref.getArray().getValues();

    // sanity check first
    OIIO_CHECK_EQUAL(initialCoeff.size(), newCoeff.size());

    const double error = 1e-10;
    // Coefficient check
    for (unsigned i = 0; i < newCoeff.size(); i++)
    {
        OIIO_CHECK_CLOSE((initialCoeff[i] * factor), newCoeff[i], error);
    }

    // Offset need to be unchanged!!
    const unsigned dim = ref.getArray().getLength();
    for (unsigned i = 0; i<dim; i++) 
    {
        OIIO_CHECK_EQUAL(initialOffset[i], ref.getOffsets()[i]);
    }
}

// Test that setting bit-depth does not affect Identity status
OIIO_ADD_TEST(OpDataMatrix, identityTest)
{
    OCIO::OpData::Matrix m1(OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_ASSERT( m1.isIdentity() );  // 16i --> 16f

    m1.setInputBitDepth(OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_ASSERT( m1.isIdentity() );  // 32f --> 16f

    m1.setOutputBitDepth(OCIO::BIT_DEPTH_UINT16);
    OIIO_CHECK_ASSERT( m1.isIdentity() );  // 32f --> 16i
}

// Validate matrix composition
OIIO_ADD_TEST(OpDataMatrix, composition)
{
    // create two test ops
    const float mtxA[] = {  1, 2, 3, 4,
                            4, 5, 6, 7,
                            7, 8, 9, 10,
                            11, 12, 13, 14 };
    const float offsA[] = { 10, 11, 12, 13 };

    OCIO::OpData::Matrix mA(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_F16);
    mA.setRGBAValues(mtxA);
    mA.setRGBAOffsets(offsA);

    const float mtxB[] = { 21, 22, 23, 24,
                           24, 25, 26, 27,
                           27, 28, 29, 30,
                           31, 32, 33, 34 };
    const float offsB[] = { 30, 31, 32, 33 };

    OCIO::OpData::Matrix mB(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT10);
    mB.setRGBAValues(mtxB);
    mB.setRGBAOffsets(offsB);

    // correct results
    const float aim[] = { 534, 624, 714, 804,
                          603, 705, 807, 909,
                          672, 786, 900, 1014,
                          764, 894, 1024, 1154 };
    const float aim_offs[] = { 1040 + 30, 1178 + 31, 1316 + 32, 1500 + 33 };

    // compose
    OCIO::OpData::OpDataMatrixRcPtr result(mA.compose(mB));

    // check bit-depths copied correctly
    OIIO_CHECK_EQUAL(result->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(result->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    const OCIO::OpData::ArrayDouble::Values& newCoeff = result->getArray().getValues();

    // sanity check on size
    OIIO_CHECK_ASSERT(newCoeff.size() == 16);

    // coefficient check
    for (unsigned i = 0; i < newCoeff.size(); i++)
    {
        OIIO_CHECK_EQUAL(aim[i], newCoeff[i]);
    }

    // offset check
    const unsigned dim = result->getArray().getLength();
    for (unsigned i = 0; i < dim; i++)
    {
        OIIO_CHECK_EQUAL(aim_offs[i], result->getOffsets()[i]);
    }
}

OIIO_ADD_TEST(OpDataMatrix, equality_test)
{
    OCIO::OpData::Matrix m1(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8);
    m1.setArrayValue(0, 2);

    OCIO::OpData::Matrix m2(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8);
    m2.setId("invalid_u_id_test");
    m2.setArrayValue(0, 2);

    OIIO_CHECK_ASSERT(!(m1 == m2));

    OCIO::OpData::Matrix m3(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8);
    m3.setArrayValue(0, 6);

    OIIO_CHECK_ASSERT(!(m1 == m3));

    OCIO::OpData::Matrix m4(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8);
    m4.setArrayValue(0, 2);

    OIIO_CHECK_ASSERT(m1 == m4);

    m4.setOffsetValue(3, 1e-5f);

    OIIO_CHECK_ASSERT(!(m1 == m4));
}

OIIO_ADD_TEST(OpDataMatrix, rgbTest)
{
    OCIO::OpData::Matrix m(OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16);

    const float rgb[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    m.setRGBValues(rgb);

    const OCIO::OpData::ArrayDouble::Values& v = m.getArray().getValues();
    OIIO_CHECK_EQUAL(v[0], rgb[0]);
    OIIO_CHECK_EQUAL(v[1], rgb[1]);
    OIIO_CHECK_EQUAL(v[2], rgb[2]);
    OIIO_CHECK_EQUAL(v[3], 0.0f);

    OIIO_CHECK_EQUAL(v[4], rgb[3]);
    OIIO_CHECK_EQUAL(v[5], rgb[4]);
    OIIO_CHECK_EQUAL(v[6], rgb[5]);
    OIIO_CHECK_EQUAL(v[7], 0.0f);

    OIIO_CHECK_EQUAL(v[8], rgb[6]);
    OIIO_CHECK_EQUAL(v[9], rgb[7]);
    OIIO_CHECK_EQUAL(v[10], rgb[8]);
    OIIO_CHECK_EQUAL(v[11], 0.0f);

    OIIO_CHECK_EQUAL(v[12], 0.0f);
    OIIO_CHECK_EQUAL(v[13], 0.0f);
    OIIO_CHECK_EQUAL(v[14], 0.0f);
    OIIO_CHECK_EQUAL(v[15], 1.0f);
}

OIIO_ADD_TEST(OpDataMatrix, rgba)
{
    OCIO::OpData::Matrix m(OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16);

    const float rgba[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15 };
    m.setRGBAValues(rgba);

    const OCIO::OpData::ArrayDouble::Values& v = m.getArray().getValues();
    for (unsigned i = 0; i<16; ++i)
    {
        OIIO_CHECK_EQUAL(v[i], rgba[i]);
    }
}

OIIO_ADD_TEST(OpDataMatrix, bitdepth_successive_changes)
{
    OCIO::OpData::Matrix m1(OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_ASSERT(m1.isDiagonal());

    const float scaleFactor = 
        OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F32)
        / OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT10);

    const OCIO::OpData::ArrayDouble::Values& m1Coeff = m1.getArray().getValues();

    const double error = 1e-10;

    const unsigned dim = m1.getArray().getLength();
    for (unsigned i = 0; i<dim; ++i)
    {
        for (unsigned j = 0; j<dim; ++j)
        {
            if (i == j)
            {
                OIIO_CHECK_CLOSE(m1Coeff[i*dim + j], scaleFactor, error);
            }
        }
    }

    OCIO::OpData::Matrix m2(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT8);

    m2.setInputBitDepth(OCIO::BIT_DEPTH_UINT8);
    m2.setOutputBitDepth(OCIO::BIT_DEPTH_UINT12);

    m2.setInputBitDepth(OCIO::BIT_DEPTH_F32);
    m2.setOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    m2.setInputBitDepth(OCIO::BIT_DEPTH_UINT10);
    m2.setOutputBitDepth(OCIO::BIT_DEPTH_UINT16);

    m2.setInputBitDepth(OCIO::BIT_DEPTH_UINT16);
    m2.setOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    m2.setInputBitDepth(OCIO::BIT_DEPTH_UINT10);
    m2.setOutputBitDepth(OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_ASSERT(m2.isDiagonal());

    const OCIO::OpData::ArrayDouble::Values& m2Coeff = m2.getArray().getValues();

    const unsigned m1Size = (unsigned)m1Coeff.size();
    const unsigned m2Size = (unsigned)m2Coeff.size();
    OIIO_CHECK_EQUAL(m1Size, m2Size);

    for (unsigned i = 0; i < m1Size; ++i)
    {
        OIIO_CHECK_CLOSE(m1Coeff[i], m2Coeff[i], error);
    }
}

OIIO_ADD_TEST(OpDataMatrix, matrixInverse_identity)
{
    OCIO::OpData::Matrix 
        refMatrixOp(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT12);

    OIIO_CHECK_ASSERT(refMatrixOp.isDiagonal());
    OIIO_CHECK_ASSERT(refMatrixOp.isIdentity());
    OIIO_CHECK_ASSERT(!refMatrixOp.hasOffsets());

    // Get inverse of reference matrix operation 
    OCIO::OpData::OpDataVec invOps;
    OIIO_CHECK_NO_THROW(refMatrixOp.inverse(invOps));
    OIIO_CHECK_EQUAL(invOps.size(), 1u);

    const OCIO::OpData::Matrix* invMatrixOp
        = dynamic_cast<const OCIO::OpData::Matrix*>(invOps.get(0));
    OIIO_CHECK_ASSERT(invMatrixOp);

    // Inverse op should have its input/output bitdepth inverted ...
    OIIO_CHECK_EQUAL(invMatrixOp->getInputBitDepth(),
        refMatrixOp.getOutputBitDepth());
    OIIO_CHECK_EQUAL(invMatrixOp->getOutputBitDepth(),
        refMatrixOp.getInputBitDepth());

    // But still be an identity matrix
    OIIO_CHECK_ASSERT(invMatrixOp->isDiagonal());
    OIIO_CHECK_ASSERT(invMatrixOp->isIdentity());
    OIIO_CHECK_ASSERT(!invMatrixOp->hasOffsets());
}

OIIO_ADD_TEST(OpDataMatrix, matrixInverse_singular)
{
    OCIO::OpData::Matrix singularMatrixOp(OCIO::BIT_DEPTH_F32,
        OCIO::BIT_DEPTH_UINT12);

    // Set singular matrix values
    const float mat[16] 
        = { 1.f,  0.f, 0.f, .2f,
            0.f,  0.f, 0.f,  0.f,
            0.f,  0.f, 0.f,  0.f,
             .2f, 0.f, 0.f,  1.f };

    singularMatrixOp.setRGBAValues(mat);

    OIIO_CHECK_ASSERT(!singularMatrixOp.isNoOp());
    OIIO_CHECK_ASSERT(!singularMatrixOp.isEye());
    OIIO_CHECK_ASSERT(!singularMatrixOp.isDiagonal());
    OIIO_CHECK_ASSERT(!singularMatrixOp.isIdentity());
    OIIO_CHECK_ASSERT(!singularMatrixOp.hasOffsets());

    // Get inverse of singular matrix operation 
    OCIO::OpData::OpDataVec invOps;
    OIIO_CHECK_THROW_WHAT(singularMatrixOp.inverse(invOps),
                          OCIO::Exception,
                          "Singular Matrix can't be inverted");
}

OIIO_ADD_TEST(OpDataMatrix, matrixInverse)
{
    OCIO::OpData::Matrix 
        refMatrixOp(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32);

    // Set arbitrary matrix and offset values
    const float matrix[16] = { 0.9f,  0.8f, -0.7f, 0.6f,
                              -0.4f,  0.5f,  0.3f, 0.2f,
                               0.1f, -0.2f,  0.4f, 0.3f,
                              -0.5f,  0.6f,  0.7f, 0.8f };
    const float offsets[4] = { -0.1f, 0.2f, -0.3f, 0.4f };

    refMatrixOp.setRGBAValues(matrix);
    refMatrixOp.setRGBAOffsets(offsets);

    // Get inverse of reference matrix operation 
    OCIO::OpData::OpDataVec invOps;
    OIIO_CHECK_NO_THROW(refMatrixOp.inverse(invOps));
    OIIO_CHECK_EQUAL(invOps.size(), 1u);

    const OCIO::OpData::Matrix* invMatrixOp
        = dynamic_cast<const OCIO::OpData::Matrix*>(invOps.get(0));
    OIIO_CHECK_ASSERT(invMatrixOp);

    // input/output bitdepths inverted
    OIIO_CHECK_EQUAL(invMatrixOp->getInputBitDepth(),
        refMatrixOp.getOutputBitDepth());
    OIIO_CHECK_EQUAL(invMatrixOp->getOutputBitDepth(),
        refMatrixOp.getInputBitDepth());

    const float expectedMatrix[16] = {
        0.75f,                3.5f,               3.5f,              -2.75f,
        0.546296296296297f,   3.90740740740741f,  1.31481481481482f, -1.87962962962963f,
        0.12037037037037f,    4.75925925925926f,  4.01851851851852f, -2.78703703703704f,
       -0.0462962962962963f, -4.90740740740741f, -2.31481481481482f,  3.37962962962963f };

    const float expectedOffsets[4] = {
        1.525f, 0.419444444444445f, 1.38055555555556f, -1.06944444444444f };

    const OCIO::OpData::ArrayDouble::Values& invValues =
        invMatrixOp->getArray().getValues();
    const double* invOffsets = invMatrixOp->getOffsets().getValues();

    // Check matrix coeffs
    for (unsigned i = 0; i < 16; ++i) {
        OIIO_CHECK_CLOSE(invValues[i], expectedMatrix[i], 1e-6f);
    }

    // Check matrix offsets
    for (unsigned i = 0; i < 4; ++i) {
        OIIO_CHECK_CLOSE(invOffsets[i],expectedOffsets[i], 1e-6f);
    }
}

OIIO_ADD_TEST(OpDataMatrix, matrix_channel_test)
{
    OCIO::OpData::Matrix refMatrixOp(OCIO::BIT_DEPTH_F32,
        OCIO::BIT_DEPTH_UINT12);

    OIIO_CHECK_ASSERT(!refMatrixOp.hasChannelCrosstalk());

    const float offsets[4] = { -0.1f, 0.2f, -0.3f, 0.4f };
    refMatrixOp.setRGBAOffsets(offsets);
    // false: with offsets
    OIIO_CHECK_ASSERT(!refMatrixOp.hasChannelCrosstalk());

    const float matrix[16] = { 0.9f,  0.0f,  0.0f, 0.0f,
                               0.0f,  0.5f,  0.0f, 0.0f,
                               0.0f,  0.0f, -0.4f, 0.0f,
                               0.0f,  0.0f,  0.0f, 0.8f };
    refMatrixOp.setRGBAValues(matrix);
    // false: with diagonal
    OIIO_CHECK_ASSERT(!refMatrixOp.hasChannelCrosstalk());

    const float matrix2[16] = { 1.0f,  0.0f,  0.0f, 0.0f,
                                0.0f,  1.0f,  0.0f, 0.0f,
                                0.0f,  0.0f,  1.0f, 0.000000001f,
                                0.0f,  0.0f,  0.0f, 1.0f };
    refMatrixOp.setRGBAValues(matrix2);
    // true: with off-diagonal
    OIIO_CHECK_ASSERT(refMatrixOp.hasChannelCrosstalk());
}

#endif
