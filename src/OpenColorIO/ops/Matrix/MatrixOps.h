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

OCIO_NAMESPACE_ENTER
{
    class MatrixOpData;
    typedef OCIO_SHARED_PTR<MatrixOpData> MatrixOpDataRcPtr;

    class MatrixOpData : public OpData
    {
    public:
        MatrixOpData();
        // Create a scale matrix from the input to output value range
        MatrixOpData(BitDepth inBitDepth, BitDepth outBitDepth);
        MatrixOpData(const float * m44, const float * offset4);
        virtual ~MatrixOpData() {}

        MatrixOpData & operator = (const MatrixOpData & rhs);

        virtual Type getType() const override { return MatrixType; }

        // Determine whether the output of the op mixes R, G, B channels.
        // For example, Rout = 5*Rin is channel independent, but Rout = Rin + Gin
        // is not.  Note that the property may depend on the op parameters,
        // so, e.g. MatrixOps may sometimes return true and other times false.
        // returns true if the op's output does not combine input channels
        virtual bool hasChannelCrosstalk() const override;

        virtual bool isNoOp() const override;
        virtual bool isIdentity() const override;
        virtual bool hasOffsets() const;
        virtual bool isMatrixIdentity() const;
        virtual bool isMatrixDiagonal() const;

        float m_m44[16];
        float m_offset4[4];

        virtual void finalize() override;
    };

    // Use whichever is most convenient; they are equally efficient
    
    void CreateScaleOp(OpRcPtrVec & ops,
                       const float * scale4,
                       TransformDirection direction);
    
    void CreateMatrixOp(OpRcPtrVec & ops,
                        const float * m44,
                        TransformDirection direction);
    
    void CreateOffsetOp(OpRcPtrVec & ops,
                        const float * offset4,
                        TransformDirection direction);
    
    void CreateMatrixOffsetOp(OpRcPtrVec & ops,
                              const float * m44, const float * offset4,
                              TransformDirection direction);
    
    void CreateScaleOffsetOp(OpRcPtrVec & ops,
                             const float * scale4, const float * offset4,
                             TransformDirection direction);
    
    void CreateFitOp(OpRcPtrVec & ops,
                     const float * oldmin4, const float * oldmax4,
                     const float * newmin4, const float * newmax4,
                     TransformDirection direction);
    
    void CreateSaturationOp(OpRcPtrVec & ops,
                            float sat,
                            const float * lumaCoef3,
                            TransformDirection direction);

    void CreateMinMaxOp(OpRcPtrVec & ops,
                        const float * from_min3,
                        const float * from_max3,
                        TransformDirection direction);
}
OCIO_NAMESPACE_EXIT

#endif
