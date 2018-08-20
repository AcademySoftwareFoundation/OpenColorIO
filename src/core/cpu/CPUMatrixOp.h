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


#ifndef INCLUDED_OCIO_CPU_MATRIXOP
#define INCLUDED_OCIO_CPU_MATRIXOP


#include <OpenColorIO/OpenColorIO.h>


#include "../opdata/OpDataMatrix.h"

#include "CPUOp.h"

OCIO_NAMESPACE_ENTER
{

    namespace CPUMatrixOp
    {
        // Get the dedicated renderer
        CPUOpRcPtr GetRenderer(const OpData::OpDataMatrixRcPtr & mat);
    }

    class ScaleRenderer : public CPUOp
    {
    public:
        ScaleRenderer(const OpData::OpDataMatrixRcPtr & mat);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;

    private:
        ScaleRenderer(); // no default constructor
        float m_scale[4];
    };

    class ScaleWithOffsetRenderer : public CPUOp
    {
    public:
        ScaleWithOffsetRenderer(const OpData::OpDataMatrixRcPtr & mat);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;

    private:
        ScaleWithOffsetRenderer(); // no default constructor
        float m_scale[4];
        float m_offset[4];
    };

    class MatrixWithOffsetRenderer : public CPUOp
    {
    public:
        MatrixWithOffsetRenderer(const OpData::OpDataMatrixRcPtr & mat);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;

    private:
        MatrixWithOffsetRenderer(); // no default constructor
        // Keep all these values because they are invariant during the
        // processing. So to slim the processing code, these variables
        // are computed in the constructor.
        float m_column1[4];
        float m_column2[4];
        float m_column3[4];
        float m_column4[4];

        float m_offset[4];
    };

    class MatrixRenderer : public CPUOp
    {
    public:
        MatrixRenderer(const OpData::OpDataMatrixRcPtr & mat);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;

    private:
        MatrixRenderer(); // no default constructor

        // Keep all these values because they are invariant during the
        // processing. So to slim the processing code, these variables
        // are computed in the constructor.
        float m_column1[4];
        float m_column2[4];
        float m_column3[4];
        float m_column4[4];
    };

}
OCIO_NAMESPACE_EXIT

#endif