// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_MATRIXOFFSETOP_H
#define INCLUDED_OCIO_MATRIXOFFSETOP_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/matrix/MatrixOpData.h"

namespace OCIO_NAMESPACE
{

// Use whichever is most convenient; they are equally efficient.
// Note that op is always created even if resulting op is a no-op.

void CreateScaleOp(OpRcPtrVec & ops,
                    const double * scale4,
                    TransformDirection direction);

void CreateMatrixOp(OpRcPtrVec & ops,
                    const double * m44,
                    TransformDirection direction);

void CreateOffsetOp(OpRcPtrVec & ops,
                    const double * offset4,
                    TransformDirection direction);

void CreateMatrixOffsetOp(OpRcPtrVec & ops,
                            const double * m44, const double * offset4,
                            TransformDirection direction);

void CreateScaleOffsetOp(OpRcPtrVec & ops,
                            const double * scale4, const double * offset4,
                            TransformDirection direction);

void CreateFitOp(OpRcPtrVec & ops,
                    const double * oldmin4, const double * oldmax4,
                    const double * newmin4, const double * newmax4,
                    TransformDirection direction);

void CreateSaturationOp(OpRcPtrVec & ops,
                        double sat,
                        const double * lumaCoef3,
                        TransformDirection direction);

void CreateMatrixOffsetOp(OpRcPtrVec & ops,
                            const float * m44, const float * offset4,
                            TransformDirection direction);

void CreateFitOp(OpRcPtrVec & ops,
                    const float * oldmin4, const float * oldmax4,
                    const float * newmin4, const float * newmax4,
                    TransformDirection direction);

void CreateMinMaxOp(OpRcPtrVec & ops,
                    const double * from_min3,
                    const double * from_max3,
                    TransformDirection direction);

void CreateMinMaxOp(OpRcPtrVec & ops,
                    float from_min,
                    float from_max,
                    TransformDirection direction);

void CreateMatrixOp(OpRcPtrVec & ops,
                    MatrixOpData::MatrixArrayPtr & matrix,
                    TransformDirection direction);

void CreateMatrixOp(OpRcPtrVec & ops,
                    MatrixOpDataRcPtr & matrix,
                    TransformDirection direction);

void CreateIdentityMatrixOp(OpRcPtrVec & ops);

// Create a copy of the matrix transform in the op and append it to the GroupTransform.
void CreateMatrixTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

} // namespace OCIO_NAMESPACE

#endif
