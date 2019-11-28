// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CDLOP_CPU_H
#define INCLUDED_OCIO_CDLOP_CPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/cdl/CDLOpData.h"


namespace OCIO_NAMESPACE
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

} // namespace OCIO_NAMESPACE


#endif
