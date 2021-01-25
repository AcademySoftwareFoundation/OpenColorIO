// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "HashUtils.h"
#include "MathUtils.h"
#include "ops/matrix/MatrixOpData.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

MatrixOpData::Offsets::Offsets(double redOff, double grnOff, double bluOff, double whtOff)
{
    m_values[0] = redOff;
    m_values[1] = grnOff;
    m_values[2] = bluOff;
    m_values[3] = whtOff;
}

MatrixOpData::Offsets::Offsets(const Offsets & o)
{
    memcpy(m_values, o.m_values, 4 * sizeof(double));
}

MatrixOpData::Offsets& MatrixOpData::Offsets::operator=(const Offsets & o)
{
    if (this != &o)
    {
        std::memcpy(m_values, o.m_values, 4 * sizeof(double));
    }
    return *this;
}

bool MatrixOpData::Offsets::operator==(const Offsets & o) const
{
    return (std::memcmp(m_values, o.m_values, 4 * sizeof(double)) == 0);
}

template<typename T>
void MatrixOpData::Offsets::setRGB(const T * v3)
{
    if (!v3)
    {
        throw Exception("Matrix: setRGB NULL pointer.");
    }

    m_values[0] = double(v3[0]);
    m_values[1] = double(v3[1]);
    m_values[2] = double(v3[2]);
    m_values[3] = 0.;
}

template void MatrixOpData::Offsets::setRGB(const float * v3);
template void MatrixOpData::Offsets::setRGB(const double * v3);

template<typename T>
void MatrixOpData::Offsets::setRGBA(const T * v4)
{
    if (!v4)
    {
        throw Exception("Matrix: setRGBA NULL pointer.");
    }

    m_values[0] = double(v4[0]);
    m_values[1] = double(v4[1]);
    m_values[2] = double(v4[2]);
    m_values[3] = double(v4[3]);
}

template void MatrixOpData::Offsets::setRGBA(const float * v4);
template void MatrixOpData::Offsets::setRGBA(const double * v4);

bool MatrixOpData::Offsets::isNotNull() const
{
    return m_values[0] != 0. || m_values[1] != 0. ||
           m_values[2] != 0. || m_values[3] != 0.;
}

void MatrixOpData::Offsets::scale(double s)
{
    for (unsigned int i = 0; i < 4; ++i)
    {
        m_values[i] *= s;
    }
}

MatrixOpData::MatrixArray::MatrixArray()
{
    resize(4, 4);
    fill();
}

MatrixOpData::MatrixArray & MatrixOpData::MatrixArray::operator=(const ArrayDouble & a)
{
    if (this == &a) return *this;

    *dynamic_cast<ArrayDouble*>(this) = a;

    validate();

    return *this;
}

MatrixOpData::MatrixArrayPtr MatrixOpData::MatrixArray::inner(const MatrixArray & B) const
{
    // Use operator= to make sure we have a 4x4 copy
    // of the original matrices.
    MatrixArray A_4x4 = *this;
    MatrixArray B_4x4 = B;
    const ArrayDouble::Values & Avals = A_4x4.getValues();
    const ArrayDouble::Values & Bvals = B_4x4.getValues();

    MatrixArrayPtr OutPtr = std::make_shared<MatrixArray>();
    ArrayDouble::Values& Ovals = OutPtr->getValues();

    const unsigned long dim = OutPtr->getLength();

    // Note: The matrix elements are stored in the vector
    // in row-major order.
    // [ a00, a01, a02, a03, a10, a11, a12, a13, a20, ... a44 ]
    for (unsigned long row = 0; row<dim; ++row)
    {
        for (unsigned long col = 0; col<dim; ++col)
        {
            double accum = 0.;
            for (unsigned long i = 0; i<dim; ++i)
            {
                accum += Avals[row * dim + i] * Bvals[i * dim + col];
            }
            Ovals[row * dim + col] = accum;
        }
    }

    return OutPtr;
}

MatrixOpData::MatrixArrayPtr MatrixOpData::MatrixArray::inner(const MatrixArrayPtr & B) const
{
    return inner(*B.get());
}

MatrixOpData::Offsets MatrixOpData::MatrixArray::inner(const MatrixOpData::Offsets & b) const
{
    MatrixOpData::Offsets out;

    const unsigned long dim = getLength();
    const ArrayDouble::Values & Avals = getValues();

    for (unsigned long i = 0; i < dim; ++i)
    {
        double accum = 0.;
        for (unsigned long j = 0; j < dim; ++j)
        {
            accum += Avals[i * dim + j] * b[j];
        }
        out[i] = accum;
    }

    return out;
}

MatrixOpData::MatrixArrayPtr MatrixOpData::MatrixArray::inverse() const
{
    // Call validate to ensure that the matrix is 4x4,
    // will be expanded if only 3x3.
    validate();

    MatrixArray t(*this);

    // Create a new matrix array.
    // The new matrix is initialized as identity.
    MatrixArrayPtr invPtr = std::make_shared<MatrixArray>();
    MatrixArray & s = *invPtr;

    const unsigned long dim = invPtr->getLength();

    // Inversion starts with identity (without bit-depth scaling).
    s[0] = 1.;
    s[5] = 1.;
    s[10] = 1.;
    s[15] = 1.;

    // From Imath
    // Code copied from Matrix44<T>::gjInverse (bool singExc) const in ImathMatrix.h

    // Forward elimination.

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

    // Backward substitution.

    for (int i = 3; i >= 0; --i)
    {
        double f = 0.;

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

template<typename T>
void MatrixOpData::MatrixArray::setRGB(const T * values)
{
    Values & v = getValues();

    v[ 0] = double(values[0]);
    v[ 1] = double(values[1]);
    v[ 2] = double(values[2]);
    v[ 3] = 0.;

    v[ 4] = double(values[3]);
    v[ 5] = double(values[4]);
    v[ 6] = double(values[5]);
    v[ 7] = 0.;

    v[ 8] = double(values[6]);
    v[ 9] = double(values[7]);
    v[10] = double(values[8]);
    v[11] = 0.;

    v[12] = 0.;
    v[13] = 0.;
    v[14] = 0.;
    v[15] = 1.;
}

template void MatrixOpData::MatrixArray::setRGB(const float * values);
template void MatrixOpData::MatrixArray::setRGB(const double * values);

unsigned long MatrixOpData::MatrixArray::getNumValues() const
{
    return getLength() * getLength();
}

bool MatrixOpData::MatrixArray::isUnityDiagonal() const
{
    const unsigned long dim = getLength();
    const ArrayDouble::Values & values = getValues();

    for (unsigned long i = 0; i<dim; ++i)
    {
        for (unsigned long j = 0; j<dim; ++j)
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

void MatrixOpData::MatrixArray::fill()
{
    const unsigned long dim = getLength();
    ArrayDouble::Values & values = getValues();

    std::memset(&values[0], 0, values.size() * sizeof(double));

    for (unsigned long i = 0; i<dim; ++i)
    {
        for (unsigned long j = 0; j<dim; ++j)
        {
            if (i == j)
            {
                values[i*dim + j] = 1.0;
            }
        }
    }
}

void MatrixOpData::MatrixArray::expandFrom3x3To4x4()
{
    const Values oldValues = getValues();

    resize(4, 4);

    setRGB(oldValues.data());
}

void MatrixOpData::MatrixArray::setRGBA(const float * values)
{
    Values & v = getValues();

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

void MatrixOpData::MatrixArray::setRGBA(const double * values)
{
    Values & v = getValues();
    std::memcpy(&v[0], values, 16 * sizeof(double));
}

void MatrixOpData::MatrixArray::validate() const
{
    // Note: By design, only 4x4 matrices are instantiated.
    // The CLF 3x3 (and 3x4) matrices are automatically converted
    // to 4x4 matrices, and a Matrix Transform only expects 4x4 matrices.

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
        throw Exception("Matrix: dimensions must be 4x4.");
    }
}

////////////////////////////////////////////////

MatrixOpData::MatrixOpData()
    : OpData()
    , m_array()
{
}

MatrixOpData::MatrixOpData(const MatrixArray & matrix)
    : OpData()
    , m_array(matrix)
{
}

MatrixOpData::MatrixOpData(TransformDirection direction)
    : OpData()
    , m_array()
{
    setDirection(direction);
}

MatrixOpData::~MatrixOpData()
{
}

MatrixOpDataRcPtr MatrixOpData::clone() const
{
    return std::make_shared<MatrixOpData>(*this);
}

void MatrixOpData::setArrayValue(unsigned long index, double value)
{
    m_array.getValues()[index] = value;
}

double MatrixOpData::getArrayValue(unsigned long index) const
{
    return m_array.getValues()[index];
}

void  MatrixOpData::setRGB(const float* values)
{
    m_array.setRGB(values);
}

template<typename T>
void MatrixOpData::setRGBA(const T * values)
{
    m_array.setRGBA(values);
}

template void MatrixOpData::setRGBA(const float * values);
template void MatrixOpData::setRGBA(const double * values);

void MatrixOpData::validate() const
{
    try
    {
        m_array.validate();
    }
    catch (Exception & e)
    {
        std::ostringstream oss;
        oss << "Matrix array content issue: ";
        oss << e.what();

        throw Exception(oss.str().c_str());
    }
    if (m_direction == TRANSFORM_DIR_INVERSE)
    {
        // Make sure matrix can be inverted.
        getAsForward();
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

bool MatrixOpData::isUnityDiagonal() const
{
    return m_array.isUnityDiagonal();
}

bool MatrixOpData::isNoOp() const
{
    return isIdentity();
}

bool MatrixOpData::isIdentity() const
{
    if (hasOffsets() || hasAlpha() || !isDiagonal())
    {
        return false;
    }

    // Now check the diagonal elements.

    const double maxDiff = 1e-6;

    const ArrayDouble & a = getArray();
    const ArrayDouble::Values & m = a.getValues();
    const unsigned long dim = a.getLength();

    for (unsigned long i = 0; i<dim; ++i)
    {
        for (unsigned long j = 0; j<dim; ++j)
        {
            if (i == j)
            {
                if (!EqualWithAbsError(m[i*dim + j], 1.0, maxDiff))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool MatrixOpData::isDiagonal() const
{
    const ArrayDouble & a = getArray();
    const ArrayDouble::Values & m = a.getValues();
    const unsigned long max = a.getNumValues();
    const unsigned long dim = a.getLength();

    for (unsigned long idx = 0; idx<max; ++idx)
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

bool MatrixOpData::hasAlpha() const
{
    const ArrayDouble & a = getArray();
    const ArrayDouble::Values & m = a.getValues();

    // Now check the diagonal elements.

    static constexpr double maxDiff = 1e-6;

    return

        // Last column.
        (m[3] != 0.0) || // Strict comparison intended
        (m[7] != 0.0) ||
        (m[11] != 0.0) ||

        // Diagonal.
        !EqualWithAbsError(m[15], 1.0, maxDiff) ||

        // Bottom row.
        (m[12] != 0.0) || // Strict comparison intended
        (m[13] != 0.0) ||
        (m[14] != 0.0) ||

        // Alpha offset
        (m_offsets[3] != 0.0);

}

MatrixOpDataRcPtr MatrixOpData::CreateDiagonalMatrix(double diagValue)
{
    // Create a matrix with no offset.
    MatrixOpDataRcPtr pM = std::make_shared<MatrixOpData>();

    pM->validate();

    pM->setArrayValue(0, diagValue);
    pM->setArrayValue(5, diagValue);
    pM->setArrayValue(10, diagValue);
    pM->setArrayValue(15, diagValue);

    return pM;
}

double MatrixOpData::getOffsetValue(unsigned long index) const
{
    const unsigned long dim = getArray().getLength();
    if (index >= dim)
    {
        // TODO: should never happen. Consider assert.
        std::ostringstream oss;
        oss << "Matrix array content issue: '";
        oss << getID().c_str();
        oss << "' offset index out of range '";
        oss << index;
        oss << "'. ";

        throw Exception(oss.str().c_str());
    }

    return m_offsets[index];
}

void MatrixOpData::setOffsetValue(unsigned long index, double value)
{
    const unsigned long dim = getArray().getLength();
    if (index >= dim)
    {
        // TODO: should never happen. Consider assert.
        std::ostringstream oss;
        oss << "Matrix array content issue: '";
        oss << getID().c_str();
        oss << "' offset index out of range '";
        oss << index;
        oss << "'. ";

        throw Exception(oss.str().c_str());
    }

    m_offsets[index] = value;
}

MatrixOpDataRcPtr MatrixOpData::compose(ConstMatrixOpDataRcPtr & B) const
{
    // Ensure that both matrices will have the right dimension (ie. 4x4).
    if (m_array.getLength() != 4 || B->m_array.getLength() != 4)
    {
        // Note: By design, only 4x4 matrices are instantiated.
        // The CLF 3x3 (and 3x4) matrices are automatically converted
        // to 4x4 matrices, and a Matrix Transform only expects 4x4 matrices.
        throw Exception("MatrixOpData: array content issue.");
    }
    if (getDirection() == TRANSFORM_DIR_INVERSE || B->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        throw Exception("Op::finalize has to be called.");
    }

    // TODO: May want to revisit how the metadata is set.
    FormatMetadataImpl newDesc = getFormatMetadata();
    newDesc.combine(B->getFormatMetadata());

    MatrixOpDataRcPtr out = std::make_shared<MatrixOpData>();

    out->setFileInputBitDepth(getFileInputBitDepth());
    out->setFileOutputBitDepth(B->getFileOutputBitDepth());

    out->getFormatMetadata() = newDesc;

    // By definition, A.compose(B) implies that op A precedes op B
    // in the opList. The LUT format coefficients follow matrix math:
    // vec2 = A x vec1 where A is 3x3 and vec is 3x1.
    // So the composite operation in matrix form is vec2 = B x A x vec1.
    // Hence we compute B x A rather than A x B.

    MatrixArrayPtr outPtr = B->m_array.inner(m_array);

    out->getArray() = *outPtr.get();

    // Compute matrix B times offsets from A.

    Offsets offs(B->m_array.inner(getOffsets()));

    const unsigned long dim = B->m_array.getLength();

    // Determine overall scaling of the offsets prior to any catastrophic
    // cancellation that may occur during the add.
    double val, max_val = 0.;
    for (unsigned long i = 0; i < dim; ++i)
    {
        val = fabs(offs[i]);
        max_val = max_val > val ? max_val : val;
        val = fabs(B->getOffsets()[i]);
        max_val = max_val > val ? max_val : val;
    }

    // Add offsets from B.
    for (unsigned long i = 0; i < dim; ++i)
    {
        offs[i] += B->getOffsets()[i];
    }

    out->setOffsets(offs);

    // To enable use of strict float comparisons above, we adjust the
    // result so that values very near integers become exactly integers.
    out->cleanUp(max_val);

    return out;
}

void MatrixOpData::cleanUp(double offsetScale)
{
    const ArrayDouble & a = getArray();
    const ArrayDouble::Values & m = a.getValues();
    const unsigned long dim = a.getLength();

    // Estimate the magnitude of the matrix.
    double max_val = 0.;
    for (unsigned long i = 0; i<dim; ++i)
    {
        for (unsigned long j = 0; j<dim; ++j)
        {
            const double val = fabs(m[i * dim + j]);
            max_val = max_val > val ? max_val : val;
        }
    }

    // Determine an absolute tolerance.
    // TODO: For double matrices a smaller tolerance could be used.  However we
    // have matrices that may have been quantized to less than double precision
    // either from being written to files or via the factories that take float
    // args.  In any case, the tolerance is small enough to pick up anything
    // that would be significant in the context of color management.
    const double scale = max_val > 1e-4 ? max_val : 1e-4;
    const double abs_tol = scale * 1e-7;

    // Replace values that are close to integers by exact values.
    for (unsigned long i = 0; i<dim; ++i)
    {
        for (unsigned long j = 0; j<dim; ++j)
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

    // Do likewise for the offsets.
    const double scale2 = offsetScale > 1e-4 ? offsetScale : 1e-4;
    const double abs_tol2 = scale2 * 1e-7;

    for (unsigned long i = 0; i<dim; ++i)
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

bool MatrixOpData::operator==(const OpData & other) const
{
    if (!OpData::operator==(other)) return false;

    const MatrixOpData* mop = static_cast<const MatrixOpData*>(&other);

    return (m_direction == mop->m_direction &&
            m_offsets   == mop->m_offsets   &&
            m_array     == mop->m_array);
}

void MatrixOpData::setDirection(TransformDirection dir) noexcept
{
    m_direction = dir;
}

MatrixOpDataRcPtr MatrixOpData::getAsForward() const
{
    if (getDirection() == TRANSFORM_DIR_FORWARD)
    {
        return clone();
    }
    // Get the inverse matrix.
    MatrixArrayPtr invMatrixArray = m_array.inverse();
    // MatrixArray::inverse() will throw for singular matrices.
    // TODO: Perhaps calculate pseudo-inverse rather than throw.

    // Calculate the inverse offset.
    const Offsets& offsets = getOffsets();
    Offsets invOffsets;
    if (offsets.isNotNull())
    {
        invOffsets = invMatrixArray->inner(offsets);
        invOffsets.scale(-1);
    }

    MatrixOpDataRcPtr invOp = std::make_shared<MatrixOpData>();
    invOp->m_fileInBitDepth = m_fileOutBitDepth;
    invOp->m_fileOutBitDepth = m_fileInBitDepth;

    invOp->setRGBA(&(invMatrixArray->getValues()[0]));
    invOp->setOffsets(invOffsets);
    invOp->getFormatMetadata() = getFormatMetadata();

    // No need to call validate(), the invOp will have proper dimension,
    // bit-depths, matrix and offsets values.

    // Note that the metadata may become stale at this point but trying to update it is
    // challenging.
    return invOp;
}

std::string MatrixOpData::getCacheID() const
{
    AutoMutex lock(m_mutex);

    std::ostringstream cacheIDStream;
    if (!getID().empty())
    {
        cacheIDStream << getID() << " ";
    }

    cacheIDStream << TransformDirectionToString(m_direction) << " ";

    md5_state_t state;
    md5_byte_t digest[16];

    // TODO: array and offset do not require double precision in cache.
    md5_init(&state);
    md5_append(&state,
        (const md5_byte_t *)&(getArray().getValues()[0]),
        (int)(16 * sizeof(double)));
    md5_append(&state,
        (const md5_byte_t *)getOffsets().getValues(),
        (int)(4 * sizeof(double)));
    md5_finish(&state, digest);

    cacheIDStream << GetPrintableHash(digest);

    return cacheIDStream.str();
}

void MatrixOpData::scale(double inScale, double outScale)
{
    const double combinedScale = inScale * outScale;
    getArray().scale(combinedScale);

    m_offsets.scale(outScale);
}

} // namespace OCIO_NAMESPACE

