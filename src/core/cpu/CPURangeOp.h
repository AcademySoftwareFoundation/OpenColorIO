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


#ifndef INCLUDED_OCIO_CPU_RANGEOP
#define INCLUDED_OCIO_CPU_RANGEOP


#include <OpenColorIO/OpenColorIO.h>


#include "../opdata/OpDataRange.h"

#include "CPUOp.h"


OCIO_NAMESPACE_ENTER
{
    class CPURangeOp : public CPUOp
    {
    public:

        // Get the dedicated renderer
        static CPUOpRcPtr GetRenderer(const OpData::OpDataRangeRcPtr & cdl);

        // Constructor
        // - range is the Op to process
        CPURangeOp(const OpData::OpDataRangeRcPtr & range);

    protected:
        float m_scale;
        float m_offset;
        float m_lowerBound;
        float m_upperBound;
        float m_alphaScale;

    private:
        CPURangeOp();
    };

    class RangeScaleMinMaxRenderer : public CPURangeOp
    {
    public:
        RangeScaleMinMaxRenderer(const OpData::OpDataRangeRcPtr & range);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };

    class RangeScaleMinRenderer : public CPURangeOp
    {
    public:
        RangeScaleMinRenderer(const OpData::OpDataRangeRcPtr & range);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };

    class RangeScaleMaxRenderer : public CPURangeOp
    {
    public:
        RangeScaleMaxRenderer(const OpData::OpDataRangeRcPtr & range);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };

    class RangeScaleRenderer : public CPURangeOp
    {
    public:
        RangeScaleRenderer(const OpData::OpDataRangeRcPtr & range);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };

    class RangeMinMaxRenderer : public CPURangeOp
    {
    public:
        RangeMinMaxRenderer(const OpData::OpDataRangeRcPtr & range);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };

    class RangeMinRenderer : public CPURangeOp
    {
    public:
        RangeMinRenderer(const OpData::OpDataRangeRcPtr & range);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };

    class RangeMaxRenderer : public CPURangeOp
    {
    public:
        RangeMaxRenderer(const OpData::OpDataRangeRcPtr & range);

        virtual void apply(float * rgbaBuffer, unsigned numPixels) const;
    };
}
OCIO_NAMESPACE_EXIT


#endif