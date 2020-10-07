// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "ops/matrix/MatrixOp.h"
#include "ops/matrix/MatrixOpCPU.h"
#include "ops/matrix/MatrixOpGPU.h"
#include "transforms/MatrixTransform.h"

namespace OCIO_NAMESPACE
{

namespace
{

// The class represents the Matrix Op
//
// The class specifies a matrix transformation to be applied to
// the input values. The input and output of a matrix are always
// 4-component values.
// An offset vector is also applied to the result.
// The output values are calculated using the row-major order convention:
//
// Rout = a[0][0]*Rin + a[0][1]*Gin + a[0][2]*Bin + a[0][3]*Ain + o[0];
// Gout = a[1][0]*Rin + a[1][1]*Gin + a[1][2]*Bin + a[1][3]*Ain + o[1];
// Bout = a[2][0]*Rin + a[2][1]*Gin + a[2][2]*Bin + a[2][3]*Ain + o[2];
// Aout = a[3][0]*Rin + a[3][1]*Gin + a[3][2]*Bin + a[3][3]*Ain + o[3];

class MatrixOffsetOp : public Op
{
public:
    MatrixOffsetOp() = delete;
    MatrixOffsetOp(const MatrixOffsetOp &) = delete;

    MatrixOffsetOp(const double * m44,
                    const double * offset4,
                    TransformDirection direction);

    MatrixOffsetOp(MatrixOpDataRcPtr & matrix);

    virtual ~MatrixOffsetOp();

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    void finalize() override;
    std::string getCacheID() const override;

    ConstOpCPURcPtr getCPUOp(bool fastLogExpPow) const override;

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const override;

protected:
    ConstMatrixOpDataRcPtr matrixData() const { return DynamicPtrCast<const MatrixOpData>(data()); }
    MatrixOpDataRcPtr matrixData() { return DynamicPtrCast<MatrixOpData>(data()); }
};


typedef OCIO_SHARED_PTR<MatrixOffsetOp> MatrixOffsetOpRcPtr;
typedef OCIO_SHARED_PTR<const MatrixOffsetOp> ConstMatrixOffsetOpRcPtr;


MatrixOffsetOp::MatrixOffsetOp(const double * m44,
                               const double * offset4,
                               TransformDirection direction)
    : Op()
{
    MatrixOpDataRcPtr mat = std::make_shared<MatrixOpData>(direction);
    mat->setRGBA(m44);
    mat->setRGBAOffsets(offset4);
    data() = mat;
}

MatrixOffsetOp::MatrixOffsetOp(MatrixOpDataRcPtr & matrix)
    : Op()
{
    data() = matrix;
}

OpRcPtr MatrixOffsetOp::clone() const
{
    MatrixOpDataRcPtr clonedData = matrixData()->clone();
    return std::make_shared<MatrixOffsetOp>(clonedData);
}

MatrixOffsetOp::~MatrixOffsetOp()
{ }

std::string MatrixOffsetOp::getInfo() const
{
    return "<MatrixOffsetOp>";
}

bool MatrixOffsetOp::isSameType(ConstOpRcPtr & op) const
{
    ConstMatrixOffsetOpRcPtr typedRcPtr = DynamicPtrCast<const MatrixOffsetOp>(op);
    if(!typedRcPtr) return false;
    return true;
}

bool MatrixOffsetOp::isInverse(ConstOpRcPtr & op) const
{
    // It is simpler to handle a pair of inverses by combining them and then removing
    // the identity.  So we just return false here.
    return false;
}

// Ops must have been validated and finalized.
bool MatrixOffsetOp::canCombineWith(ConstOpRcPtr & op) const
{
    // TODO: Could combine with certain ASC_CDL ops.
    if (isSameType(op))
    {
        if (matrixData()->getDirection() == TRANSFORM_DIR_INVERSE)
        {
            throw Exception("Op::finalize has to be called.");
        }
        ConstMatrixOffsetOpRcPtr typedRcPtr = DynamicPtrCast<const MatrixOffsetOp>(op);

        auto otherMat = typedRcPtr->matrixData();
        if (otherMat)
        {
            if (otherMat->getDirection() == TRANSFORM_DIR_INVERSE)
            {
                throw Exception("Op::finalize has to be called.");
            }
        }
        return true;
    }
    return false;
}

void MatrixOffsetOp::combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const
{
    if (!canCombineWith(secondOp))
    {
        throw Exception("MatrixOffsetOp: canCombineWith must be checked "
                        "before calling combineWith.");
    }
    ConstMatrixOffsetOpRcPtr typedRcPtr = DynamicPtrCast<const MatrixOffsetOp>(secondOp);

    auto thisData = typedRcPtr->matrixData();
    MatrixOpDataRcPtr composedMat = matrixData()->compose(thisData);
    if (!composedMat->isNoOp())
    {
        CreateMatrixOp(ops, composedMat, TRANSFORM_DIR_FORWARD);
    }
}

void MatrixOffsetOp::finalize()
{
    ConstMatrixOpDataRcPtr mat = matrixData();
    if (mat->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        data() = mat->getAsForward();
    }
}

std::string MatrixOffsetOp::getCacheID() const
{
    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<MatrixOffsetOp ";
    cacheIDStream << matrixData()->getCacheID() << " ";
    cacheIDStream << ">";

    return cacheIDStream.str();
}

ConstOpCPURcPtr MatrixOffsetOp::getCPUOp(bool /*fastLogExpPow*/) const
{
    ConstMatrixOpDataRcPtr data = matrixData();
    return GetMatrixRenderer(data);
}

void MatrixOffsetOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    ConstMatrixOpDataRcPtr data = matrixData();
    if (data->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        throw Exception("Op::finalize has to be called.");
    }
    GetMatrixGPUShaderProgram(shaderCreator, data);
}

}  // Anon namespace


///////////////////////////////////////////////////////////////////////////


void CreateScaleOp(OpRcPtrVec & ops,
                   const double * scale4,
                   TransformDirection direction)
{
    static constexpr double offset4[4] { 0, 0, 0, 0 };
    CreateScaleOffsetOp(ops, scale4, offset4, direction);
}

void CreateMatrixOp(OpRcPtrVec & ops,
                    const double * m44,
                    TransformDirection direction)
{
    static constexpr double offset4[4] { 0.0, 0.0, 0.0, 0.0 };
    CreateMatrixOffsetOp(ops, m44, offset4, direction);
}

void CreateOffsetOp(OpRcPtrVec & ops,
                    const double * offset4,
                    TransformDirection direction)
{
    static constexpr double scale4[4] { 1.0, 1.0, 1.0, 1.0 };
    CreateScaleOffsetOp(ops, scale4, offset4, direction);
}

void CreateScaleOffsetOp(OpRcPtrVec & ops,
                         const double * scale4, const double * offset4,
                         TransformDirection direction)
{
    double m44[16]{ 0.0 };

    m44[0] = scale4[0];
    m44[5] = scale4[1];
    m44[10] = scale4[2];
    m44[15] = scale4[3];

    CreateMatrixOffsetOp(ops, m44, offset4, direction);
}

void CreateSaturationOp(OpRcPtrVec & ops,
                        double sat,
                        const double * lumaCoef3,
                        TransformDirection direction)
{
    double matrix[16];
    double offset[4];
    MatrixTransform::Sat(matrix, offset, sat, lumaCoef3);

    CreateMatrixOffsetOp(ops, matrix, offset, direction);
}

void CreateMatrixOffsetOp(OpRcPtrVec & ops,
                          const double * m44, const double * offset4,
                          TransformDirection direction)
{
    auto mat = std::make_shared<MatrixOpData>();
    mat->setRGBA(m44);
    mat->setRGBAOffsets(offset4);
    mat->setDirection(direction);

    CreateMatrixOp(ops, mat, TRANSFORM_DIR_FORWARD);
}

void CreateFitOp(OpRcPtrVec & ops,
                 const double * oldmin4, const double * oldmax4,
                 const double * newmin4, const double * newmax4,
                 TransformDirection direction)
{
    double matrix[16];
    double offset[4];
    MatrixTransform::Fit(matrix, offset,
                         oldmin4, oldmax4,
                         newmin4, newmax4);

    CreateMatrixOffsetOp(ops, matrix, offset, direction);
}

void CreateIdentityMatrixOp(OpRcPtrVec & ops, TransformDirection direction)
{
    double matrix[16]{ 0.0 };
    matrix[0] = 1.0;
    matrix[5] = 1.0;
    matrix[10] = 1.0;
    matrix[15] = 1.0;
    const double offset[4] = { 0.0, 0.0, 0.0, 0.0 };

    ops.push_back(std::make_shared<MatrixOffsetOp>(matrix,
                                                   offset,
                                                   direction));
}

void CreateMinMaxOp(OpRcPtrVec & ops,
                    const double * from_min3,
                    const double * from_max3,
                    TransformDirection direction)
{
    double scale4[4] = { 1.0, 1.0, 1.0, 1.0 };
    double offset4[4] = { 0.0, 0.0, 0.0, 0.0 };

    bool somethingToDo = false;
    for (int i = 0; i < 3; ++i)
    {
        scale4[i] = 1.0 / (from_max3[i] - from_min3[i]);
        offset4[i] = -from_min3[i] * scale4[i];
        somethingToDo |= (scale4[i] != 1.0 || offset4[i] != 0.0);
    }

    if (somethingToDo)
    {
        CreateScaleOffsetOp(ops, scale4, offset4, direction);
    }
}

void CreateMinMaxOp(OpRcPtrVec & ops,
                    float from_min,
                    float from_max,
                    TransformDirection direction)
{
    const double min[3] = { from_min, from_min, from_min };
    const double max[3] = { from_max, from_max, from_max };
    CreateMinMaxOp(ops, min, max, direction);
}

void CreateMatrixOp(OpRcPtrVec & ops,
                    MatrixOpData::MatrixArrayPtr & matrix,
                    TransformDirection direction)
{
    MatrixOpDataRcPtr mat = std::make_shared<MatrixOpData>(*matrix.get());
    CreateMatrixOp(ops, mat, direction);
}

void CreateMatrixOp(OpRcPtrVec & ops, MatrixOpDataRcPtr & matrix, TransformDirection direction)
{
    auto mat = matrix;
    if (direction == TRANSFORM_DIR_INVERSE)
    {
        mat = matrix->clone();
        auto newDir = CombineTransformDirections(mat->getDirection(), direction);
        mat->setDirection(newDir);
    }

    ops.push_back(std::make_shared<MatrixOffsetOp>(mat));
}

void CreateIdentityMatrixOp(OpRcPtrVec & ops)
{
    MatrixOpDataRcPtr mat = MatrixOpData::CreateDiagonalMatrix(1.0);

    ops.push_back(std::make_shared<MatrixOffsetOp>(mat));
}

///////////////////////////////////////////////////////////////////////////

void CreateMatrixTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto mat = DynamicPtrCast<const MatrixOffsetOp>(op);
    if (!mat)
    {
        throw Exception("CreateMatrixTransform: op has to be a MatrixOffsetOp");
    }
    auto matTransform = MatrixTransform::Create();
    MatrixOpData & data = dynamic_cast<MatrixTransformImpl*>(matTransform.get())->data();

    auto matDataSrc = DynamicPtrCast<const MatrixOpData>(op->data());
    data = *matDataSrc;

    group->appendTransform(matTransform);
}

void BuildMatrixOp(OpRcPtrVec & ops,
                   const MatrixTransform & transform,
                   TransformDirection dir)
{
    const MatrixOpData & data = dynamic_cast<const MatrixTransformImpl &>(transform).data();
    data.validate();

    MatrixOpDataRcPtr mat = data.clone();
    CreateMatrixOp(ops, mat, dir);
}

} // namespace OCIO_NAMESPACE

