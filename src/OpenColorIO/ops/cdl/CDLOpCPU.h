// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CDLOP_CPU_H
#define INCLUDED_OCIO_CDLOP_CPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/cdl/CDLOpData.h"


namespace OCIO_NAMESPACE
{

ConstOpCPURcPtr GetCDLCPURenderer(ConstCDLOpDataRcPtr & func, bool fastPower);

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

    void setSlope(float r, float g, float b);

    void setOffset(float r, float g, float b);

    void setPower(float r, float g, float b);

    void setSaturation(float sat);

    // Update the render parameters from the operation data
    void update(ConstCDLOpDataRcPtr & cdl);

private:
    float m_slope[4];
    float m_offset[4];
    float m_power[4];
    float m_saturation;
    bool m_isReverse{ false };
    bool m_isNoClamp{ false };
};

} // namespace OCIO_NAMESPACE


#endif
