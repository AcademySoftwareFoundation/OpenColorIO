/*
Copyright (c) 2018 Autodesk Inc., et al.
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


#ifndef INCLUDED_OCIO_CDLOP_CPU
#define INCLUDED_OCIO_CDLOP_CPU

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/CDL/CDLOpData.h"


OCIO_NAMESPACE_ENTER
{

// Structure that holds parameters computed for CPU renderers
struct RenderParams
{
    RenderParams();

    const float * getSlope() const { return m_slope; }

    const float * getOffset() const { return m_offset; }

    const float * getPower() const { return m_power; }

    float getSaturation() const { return m_saturation; }

    bool isReverse() const { return m_isReverse; }

    bool isNoClamp() const { return m_isNoClamp; }

    void setSlope(float r, float g, float b, float a);

    void setOffset(float r, float g, float b, float a);

    void setPower(float r, float g, float b, float a);

    void setSaturation(float sat);

    // Update the render parameters from the operation data
    void update(ConstCDLOpDataRcPtr & cdl);

private:
    float m_slope[4];
    float m_offset[4];
    float m_power[4];
    float m_saturation;
    bool m_isReverse;
    bool m_isNoClamp;
};

class CDLOpCPU;
typedef OCIO_SHARED_PTR<CDLOpCPU> CDLOpCPURcPtr;

// Base class for the CDL operation renderers
class CDLOpCPU : public OpCPU
{
public:

    // Get the dedicated renderer
    static ConstOpCPURcPtr GetRenderer(ConstCDLOpDataRcPtr & cdl);

    CDLOpCPU(ConstCDLOpDataRcPtr & cdl);

protected:
    const RenderParams & getRenderParams() const { return m_renderParams; }

protected:
    float m_inScale;
    float m_outScale;
    float m_alphaScale;
    RenderParams m_renderParams;

private:
    CDLOpCPU();
};

class CDLRendererV1_2Fwd : public CDLOpCPU
{
public:
    CDLRendererV1_2Fwd(ConstCDLOpDataRcPtr & cdl);

    virtual void apply(const void * inImg, void * outImg, long numPixels) const;

protected:
    template<bool CLAMP>
    void _apply(const float * inImg, float * outImg, long numPixels) const;
};

class CDLRendererNoClampFwd : public CDLRendererV1_2Fwd
{
public:
    CDLRendererNoClampFwd(ConstCDLOpDataRcPtr & cdl);

    virtual void apply(const void * inImg, void * outImg, long numPixels) const;
};

class CDLRendererV1_2Rev : public CDLOpCPU
{
public:
    CDLRendererV1_2Rev(ConstCDLOpDataRcPtr & cdl);

    virtual void apply(const void * inImg, void * outImg, long numPixels) const;

protected:
    template<bool CLAMP>
    void _apply(const float * inImg, float * outImg, long numPixels) const;
};

class CDLRendererNoClampRev : public CDLRendererV1_2Rev
{
public:
    CDLRendererNoClampRev(ConstCDLOpDataRcPtr & cdl);

    virtual void apply(const void * inImg, void * outImg, long numPixels) const;
};

}
OCIO_NAMESPACE_EXIT


#endif
