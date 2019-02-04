/*
Copyright (c) 2019 Autodesk Inc., et al.
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


#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "ops/Range/RangeOpGPU.h"


OCIO_NAMESPACE_ENTER
{

void GetRangeGPUShaderProgram(GpuShaderDescRcPtr & shaderDesc,
                              ConstRangeOpDataRcPtr & range)
{
    GpuShaderText ss(shaderDesc->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add a Range processing";
    ss.newLine() << "";

    if(range->scales(true))
    {
        const double scale[4] 
            = { range->getScale(), 
                range->getScale(), 
                range->getScale(), 
                range->getAlphaScale() };

        const double offset[4] 
            = { range->getOffset(), 
                range->getOffset(), 
                range->getOffset(), 
                0. };

        ss.newLine() << shaderDesc->getPixelName() << " = "
                     << shaderDesc->getPixelName()
                     << " * "
                     << ss.vec4fConst(scale[0], scale[1], scale[2], scale[3])
                     << " + "
                     << ss.vec4fConst(offset[0], offset[1], offset[2], offset[3])
                     << ";";
    }

    if(range->minClips())
    {
        const double lowerBound[4] 
            = { range->getLowBound(), 
                range->getLowBound(), 
                range->getLowBound(), 
                -1. * HALF_MAX };

        ss.newLine() << shaderDesc->getPixelName() << " = "
                     << "max(" << shaderDesc->getPixelName() << ", "
                     << ss.vec4fConst(lowerBound[0], lowerBound[1], lowerBound[2], lowerBound[3])
                     << ");";
    }

    if(range->maxClips())
    {
        const double upperBound[4] 
            = { range->getHighBound(), 
                range->getHighBound(), 
                range->getHighBound(), 
                HALF_MAX };

        ss.newLine() << shaderDesc->getPixelName() << " = "
                     << "min(" << shaderDesc->getPixelName() << ", "
                     << ss.vec4fConst(upperBound[0], upperBound[1], upperBound[2], upperBound[3])
                     << ");";
    }

    shaderDesc->addToFunctionShaderCode(ss.string().c_str());
}


}
OCIO_NAMESPACE_EXIT
