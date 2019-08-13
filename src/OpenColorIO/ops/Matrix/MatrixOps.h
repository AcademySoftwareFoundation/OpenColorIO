// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_MATRIXOFFSETOP_H
#define INCLUDED_OCIO_MATRIXOFFSETOP_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/Matrix/MatrixOpData.h"

OCIO_NAMESPACE_ENTER
{

    // Use whichever is most convenient; they are equally efficient.
    
    void CreateScaleOp(OpRcPtrVec & ops,
                       const float * scale4,
                       TransformDirection direction);
    
    void CreateMatrixOp(OpRcPtrVec & ops,
                        const float * m44,
                        TransformDirection direction);
    
    void CreateOffsetOp(OpRcPtrVec & ops,
                        const float * offset4,
                        TransformDirection direction);
    
    void CreateScaleOffsetOp(OpRcPtrVec & ops,
                             const float * scale4, const float * offset4,
                             TransformDirection direction);
    
    void CreateSaturationOp(OpRcPtrVec & ops,
                            float sat,
                            const float * lumaCoef3,
                            TransformDirection direction);

    void CreateMatrixOffsetOp(OpRcPtrVec & ops,
                              const float * m44, const float * offset4,
                              TransformDirection direction);
    
    void CreateFitOp(OpRcPtrVec & ops,
                     const float * oldmin4, const float * oldmax4,
                     const float * newmin4, const float * newmax4,
                     TransformDirection direction);
    
    void CreateMinMaxOp(OpRcPtrVec & ops,
                        const float * from_min3,
                        const float * from_max3,
                        TransformDirection direction);

    void CreateMatrixOp(OpRcPtrVec & ops,
                        MatrixOpDataRcPtr & matrix,
                        TransformDirection direction);

    void CreateIdentityMatrixOp(OpRcPtrVec & ops);
}
OCIO_NAMESPACE_EXIT

#endif
