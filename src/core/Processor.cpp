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

#include <OpenColorSpace/OpenColorSpace.h>

#include "Processor.h"
#include "ScanlineHelper.h"

#include <sstream>

OCS_NAMESPACE_ENTER
{
    
    Processor::~Processor()
    {
    
    }
    
    OCSProcessor::OCSProcessor(const OpRcPtrVec& opVec):
        m_opVec(opVec)
    { }
    
    OCSProcessor::~OCSProcessor()
    { }
    
    bool OCSProcessor::isNoOp() const
    {
        return m_opVec.empty();
    }
    
    void OCSProcessor::render(ImageDesc& img) const
    {
        if(isNoOp()) return;
        
        ScanlineHelper scanlineHelper(img);
        float * rgbaBuffer = 0;
        long numPixels = 0;
        
        while(true)
        {
            scanlineHelper.prepRGBAScanline(&rgbaBuffer, &numPixels);
            if(numPixels == 0) break;
            if(!rgbaBuffer)
                throw OCSException("Cannot render transform; null image.");
            
            for(unsigned int opIndex=0; opIndex<m_opVec.size(); ++opIndex)
            {
                m_opVec[opIndex]->render(rgbaBuffer, numPixels);
            }
            
            scanlineHelper.finishRGBAScanline();
        }
    }

    
    const char * OCSProcessor::getHWShaderText(const HwProfileDesc & hwDesc) const
    {
        return "";
    }
    
    int OCSProcessor::getHWLut3DEdgeSize() const
    {
        return 0;
    }
    
    const char * OCSProcessor::getHWLut3DCacheID(const HwProfileDesc & hwDesc) const
    {
        return "";
    }
    
    void OCSProcessor::getHWLut3D(float* lut3d, const HwProfileDesc & hwDesc) const
    {
        
    }
    
}
OCS_NAMESPACE_EXIT
