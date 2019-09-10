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


#ifndef INCLUDED_OCIO_MATRIXOFFSETOP_H
#define INCLUDED_OCIO_MATRIXOFFSETOP_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/Matrix/MatrixOpData.h"

OCIO_NAMESPACE_ENTER
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
                        MatrixOpDataRcPtr & matrix,
                        TransformDirection direction);

    void CreateIdentityMatrixOp(OpRcPtrVec & ops);

    // Create a copy of the matrix transform in the op and append it to the GroupTransform.
    void CreateMatrixTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

}
OCIO_NAMESPACE_EXIT

#endif
