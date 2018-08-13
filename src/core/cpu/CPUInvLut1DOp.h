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


#ifndef INCLUDED_OCIO_CPU_INVLUT1DOP
#define INCLUDED_OCIO_CPU_INVLUT1DOP


#include <OpenColorIO/OpenColorIO.h>

#include "../opdata/OpDataInvLut1D.h"

#include "CPUOp.h"


OCIO_NAMESPACE_ENTER
{
    class InvLut1DRenderer : public CPUOp
    {
    public:

        static CPUOpRcPtr GetRenderer(const OpData::OpDataLut1DRcPtr & lut);

        InvLut1DRenderer(const OpData::OpDataLut1DRcPtr & lut);
        virtual ~InvLut1DRenderer();

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;

        void resetData();

        virtual void updateData(const OpData::OpDataLut1DRcPtr & lut);

    public:
        // Holds the parameters of a color component
        // Note: The structure does not own any of the pointers.
        struct ComponentParams
        {
            ComponentParams() 
                :   lutStart(0x0), startOffset(0.f), lutEnd(0x0), 
                    negLutStart(0x0), negStartOffset(0.f), negLutEnd(0x0), 
                    flipSign(1.f), bisectPoint(0.f) 
            {}

            const float * lutStart;   // copy of the pointer to start of effective lutData
            float startOffset;        // difference between real and effective start of lut
            const float * lutEnd;     // copy of the pointer to end of effective lutData
            const float * negLutStart;// lutStart for negative part of half domain LUT
            float negStartOffset;     // startOffset for negative part of half domain LUT
            const float * negLutEnd;  // lutEnd for negative part of half domain LUT
            float flipSign;           // flip the sign of value to handle decreasing luts
            float bisectPoint;        // point of switching from pos to neg of half domain
        };

        void setComponentParams(ComponentParams & params,
                                const OpData::InvLut1D::ComponentProperties & properties,
                                const float * lutPtr,
                                const float lutZeroEntry);

    protected:
        float m_scale; // output scaling for the r, g and b components

        ComponentParams m_paramsR; // parameters of the red component
        ComponentParams m_paramsG; // parameters of the green component
        ComponentParams m_paramsB; // parameters of the blue component

        unsigned m_dim;           // length of the temp arrays
        float *  m_tmpLutR;       // temp array of red LUT entries
        float *  m_tmpLutG;       // temp array of green LUT entries
        float *  m_tmpLutB;       // temp array of blue LUT entries
        float    m_alphaScaling;  // bit-depth scale factor for alpha channel

    private:
        InvLut1DRenderer();
        InvLut1DRenderer(const InvLut1DRenderer&);
        InvLut1DRenderer& operator=(const InvLut1DRenderer&);
    };

    class InvLut1DRendererHalfCode : public InvLut1DRenderer
    {
    public:
        InvLut1DRendererHalfCode(const OpData::OpDataLut1DRcPtr & lut);
        virtual ~InvLut1DRendererHalfCode();

        virtual void updateData(const OpData::OpDataLut1DRcPtr & lut);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };

    class InvLut1DRendererHueAdjust : public InvLut1DRenderer
    {
    public:
        InvLut1DRendererHueAdjust(const OpData::OpDataLut1DRcPtr & lut);
        virtual ~InvLut1DRendererHueAdjust();

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };

    class InvLut1DRendererHalfCodeHueAdjust : public InvLut1DRendererHalfCode
    {
    public:
        InvLut1DRendererHalfCodeHueAdjust(const OpData::OpDataLut1DRcPtr & lut);
        virtual ~InvLut1DRendererHalfCodeHueAdjust();

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };
}
OCIO_NAMESPACE_EXIT


#endif