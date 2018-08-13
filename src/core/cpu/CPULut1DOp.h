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


#ifndef INCLUDED_OCIO_CPU_LUT1DOP
#define INCLUDED_OCIO_CPU_LUT1DOP


#include <OpenColorIO/OpenColorIO.h>


#include "../opdata/OpDataLut1D.h"

#include "CPUOp.h"


OCIO_NAMESPACE_ENTER
{

    class BaseLut1DRenderer : public CPUOp
    {
    public:
        BaseLut1DRenderer(const OpData::OpDataLut1DRcPtr & lut);
        virtual ~BaseLut1DRenderer();

        virtual void updateData(const OpData::OpDataLut1DRcPtr & lut) = 0;

        void resetData();

    protected:
        unsigned m_dim;

        // TODO: Anticipates tmpLut eventually allowing integer data types.
        float * m_tmpLutR;
        float * m_tmpLutG;
        float * m_tmpLutB;
        
        float m_alphaScaling;

        BitDepth m_outBitDepth;

    private:
        BaseLut1DRenderer();
        BaseLut1DRenderer(const BaseLut1DRenderer&);
        BaseLut1DRenderer& operator=(const BaseLut1DRenderer&);
    };

    class Lut1DRendererHalfCode : public BaseLut1DRenderer
    {
    public:
        explicit Lut1DRendererHalfCode(const OpData::OpDataLut1DRcPtr & lut);
        virtual ~Lut1DRendererHalfCode();

        //
        // Structure used to keep track of 
        // the interpolation data for
        // special case 16f/64k 1D LUT
        //
        struct IndexPair
        {
            unsigned short valA;
            unsigned short valB;
            float fraction;

         public:
            IndexPair()
            {
                valA = 0;
                valB = 0;
                fraction = 0.0f;
            }
        };

        virtual void updateData(const OpData::OpDataLut1DRcPtr & lut);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;

    protected:
        // Interpolation helper to gather data 
        IndexPair getEdgeFloatValues(float fIn) const;
    };

    class Lut1DRenderer : public BaseLut1DRenderer
    {
    public:

        // Get the dedicated renderer
        static CPUOpRcPtr GetRenderer(const OpData::OpDataLut1DRcPtr & lut);

        Lut1DRenderer(const OpData::OpDataLut1DRcPtr & lut);
        virtual ~Lut1DRenderer();

        virtual void updateData(const OpData::OpDataLut1DRcPtr & lut);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;

    protected:
        float m_step;
        float m_dimMinusOne;
    };

    class Lut1DRendererHueAdjust : public Lut1DRenderer
    {
    public:
        Lut1DRendererHueAdjust(const OpData::OpDataLut1DRcPtr & lut);
        virtual ~Lut1DRendererHueAdjust();

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };

    class Lut1DRendererHalfCodeHueAdjust : public Lut1DRendererHalfCode
    {
    public:
        Lut1DRendererHalfCodeHueAdjust(const OpData::OpDataLut1DRcPtr & lut);
        virtual ~Lut1DRendererHalfCodeHueAdjust();

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };
}
OCIO_NAMESPACE_EXIT


#endif