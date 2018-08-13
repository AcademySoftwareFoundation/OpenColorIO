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


#ifndef INCLUDED_OCIO_CPU_CDLOP
#define INCLUDED_OCIO_CPU_CDLOP


#include <OpenColorIO/OpenColorIO.h>


#include "../opdata/OpDataCDL.h"

#include "CPUOp.h"
#include "SSE2.h"


OCIO_NAMESPACE_ENTER
{
    namespace CDLOpUtil
    {
        // Structure that holds parameters computed for CPU/GPU renderers
        class RenderParams
        {
        public:
            // Default contructor
            RenderParams();

            const float * getSlope() const { return m_slope; }

            const float * getOffset() const { return m_offset; }

            const float * getPower() const { return m_power; }

            float getSaturation() const { return m_saturation; }

            bool isReverse() const { return m_isReverse; }

            bool isNoClamp() const { return m_isNoClamp; }

            void setSlope(const float r,
                const float g,
                const float b,
                const float a);

            void setOffset(const float r,
                const float g,
                const float b,
                const float a);

            void setPower(const float r,
                const float g,
                const float b,
                const float a);

            void setSaturation(const float sat);

            // Update the render parameters from the operation data
            // - cdl holds the CDL information
            void update(const OpData::OpDataCDLRcPtr & cdl);

        private:
            float m_slope[4];
            float m_offset[4];
            float m_power[4];
            float m_saturation;
            bool m_isReverse;
            bool m_isNoClamp;
        };
    }

    // Base class for the CDL operation renderers
    class CPUCDLOp : public CPUOp
    {
    public:

        // Get the dedicated renderer
        static CPUOpRcPtr GetRenderer(const OpData::OpDataCDLRcPtr & cdl);

        // Constructor
        // - cdl is the Op to process
        CPUCDLOp(const OpData::OpDataCDLRcPtr & cdl);

    protected:

        // Get the rendering parameters
        const CDLOpUtil::RenderParams & getRenderParams() const { return m_renderParams; }

        // Initialize SSE registries
        void loadRenderParams(__m128 & inScale, 
                              __m128 & outScale,
                              __m128 & slope,
                              __m128 & offset,
                              __m128 & power,
                              __m128 & saturation) const;

    protected:
        float m_alphaScale;
        CDLOpUtil::RenderParams m_renderParams;

    private:
        CPUCDLOp();
    };

    class CDLRendererV1_2Fwd : public CPUCDLOp
    {
    public:
        CDLRendererV1_2Fwd(const OpData::OpDataCDLRcPtr & cdl);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;

    protected:
        template<bool CLAMP>
        void _apply(float * rgbaBuffer, unsigned numPixels) const;
    };

    class CDLRendererNoClampFwd : public CDLRendererV1_2Fwd
    {
    public:
        CDLRendererNoClampFwd(const OpData::OpDataCDLRcPtr & cdl);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };

    class CDLRendererV1_2Rev : public CPUCDLOp
    {
    public:
        CDLRendererV1_2Rev(const OpData::OpDataCDLRcPtr & cdl);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;

    protected:
        template<bool CLAMP>
        void _apply(float * rgbaBuffer, unsigned numPixels) const;
    };

    class CDLRendererNoClampRev : public CDLRendererV1_2Rev
    {
    public:
        CDLRendererNoClampRev(const OpData::OpDataCDLRcPtr & cdl);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };
}
OCIO_NAMESPACE_EXIT


#endif