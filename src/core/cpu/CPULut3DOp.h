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


#ifndef INCLUDED_OCIO_CPU_LUT3DOP
#define INCLUDED_OCIO_CPU_LUT3DOP


#include <OpenColorIO/OpenColorIO.h>


#include "../opdata/OpDataLut3D.h"

#include "CPUOp.h"


OCIO_NAMESPACE_ENTER
{

    class BaseLut3DRenderer : public CPUOp
    {
    public:
        explicit BaseLut3DRenderer(const OpData::OpDataLut3DRcPtr & lut);
        virtual ~BaseLut3DRenderer();

    protected:
        void updateData(const OpData::OpDataLut3DRcPtr & lut);

        // Creates a LUT aligned to a 16 byte boundary with RGB and 0 for alpha
        // in order to be able to load the LUT using _mm_load_ps.
        float* createOptLut(const OpData::Array::Values& lut) const;

    protected:
        // Keep all these values because they are invariant during the
        // processing. So to slim the processing code, these variables
        // are computed in the constructor.
        float* m_optLut;
        unsigned m_dim;
        float m_step;
        float m_maxIdx;
        float m_alphaScale;

    private:
        BaseLut3DRenderer();
        BaseLut3DRenderer(const BaseLut3DRenderer&);
        BaseLut3DRenderer& operator=(const BaseLut3DRenderer&);
    };

    class Lut3DTetrahedralRenderer : public BaseLut3DRenderer
    {
    public:
        Lut3DTetrahedralRenderer(const OpData::OpDataLut3DRcPtr & lut);
        virtual ~Lut3DTetrahedralRenderer();

        void apply(float *rgbaBuffer, unsigned nPixels) const;
    };

    class Lut3DRenderer : public BaseLut3DRenderer
    {
    public:
        // Get the dedicated renderer
        static CPUOpRcPtr GetRenderer(const OpData::OpDataLut3DRcPtr & lut);

    public:
        Lut3DRenderer(const OpData::OpDataLut3DRcPtr & lut);
        virtual ~Lut3DRenderer();

        void apply(float *rgbaBuffer, unsigned nPixels) const;
    };

}
OCIO_NAMESPACE_EXIT


#endif