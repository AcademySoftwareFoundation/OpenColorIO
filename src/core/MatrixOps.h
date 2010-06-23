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

#include <OpenColorIO/OpenColorIO.h>
#include <vector>

#include "Op.h"

OCIO_NAMESPACE_ENTER
{
    // Use whichever is most convenient; they are equally efficient
    //
    // NOTE: These currently do not affect alpha
    
    void CreateScaleOp(OpRcPtrVec * opVec,
                       const float * scale4,
                       TransformDirection direction);
    
    void CreateMatrixOp(OpRcPtrVec * opVec,
                        const float * m44,
                        TransformDirection direction);
    
    void CreateOffsetOp(OpRcPtrVec * opVec,
                        const float * offset4,
                        TransformDirection direction);
    
    void CreateMatrixOffsetOp(OpRcPtrVec * opVec,
                              const float * m44, const float * offset4,
                              TransformDirection direction);
    
    void CreateScaleOffsetOp(OpRcPtrVec * opVec,
                             const float * scale4, const float * offset4,
                             TransformDirection direction);
    
    void CreateSaturationOp(OpRcPtrVec * opVec,
                            float sat,
                            const float * lumaCoef3,
                            TransformDirection direction);
    
    // TODO: hue rotate Matrix Op
    // TODO: Fit matrix op
}
OCIO_NAMESPACE_EXIT

#endif
