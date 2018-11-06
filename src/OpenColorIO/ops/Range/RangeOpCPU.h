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


#ifndef INCLUDED_OCIO_RANGEOP_CPU
#define INCLUDED_OCIO_RANGEOP_CPU


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/Range/RangeOpData.h"


OCIO_NAMESPACE_ENTER
{

class RangeOpCPU;
typedef OCIO_SHARED_PTR<RangeOpCPU> RangeOpCPURcPtr;

class RangeOpCPU : public OpCPU
{
public:

    // Get the dedicated renderer
    static OpCPURcPtr GetRenderer(const RangeOpDataRcPtr & range);

    RangeOpCPU(const RangeOpDataRcPtr & range);

protected:
    float m_scale;
    float m_offset;
    float m_lowerBound;
    float m_upperBound;
    float m_alphaScale;

private:
    RangeOpCPU();
};

class RangeScaleMinMaxRenderer : public RangeOpCPU
{
public:
    RangeScaleMinMaxRenderer(const RangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const;
};

class RangeScaleMinRenderer : public RangeOpCPU
{
public:
    RangeScaleMinRenderer(const RangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const;
};

class RangeScaleMaxRenderer : public RangeOpCPU
{
public:
    RangeScaleMaxRenderer(const RangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const;
};

class RangeScaleRenderer : public RangeOpCPU
{
public:
    RangeScaleRenderer(const RangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const;
};

class RangeMinMaxRenderer : public RangeOpCPU
{
public:
    RangeMinMaxRenderer(const RangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const;
};

class RangeMinRenderer : public RangeOpCPU
{
public:
    RangeMinRenderer(const RangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const;
};

class RangeMaxRenderer : public RangeOpCPU
{
public:
    RangeMaxRenderer(const RangeOpDataRcPtr & range);

    virtual void apply(float * rgbaBuffer, long numPixels) const;
};

}
OCIO_NAMESPACE_EXIT


#endif