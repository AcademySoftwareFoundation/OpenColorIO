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

#include <OpenColorIO/OpenColorIO.h>

#include "GpuAllocationOp.h"

#include <sstream>

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        const int FLOAT_DECIMALS = 7;
    }
    
    GpuAllocationOp::GpuAllocationOp(GpuAllocation allocation,
                    float min,
                    float max) :
                    m_allocation(allocation),
                    m_min(min),
                    m_max(max)
    { };

    GpuAllocationOp::~GpuAllocationOp()
    {
    
    }
    
    std::string GpuAllocationOp::getInfo() const
    {
        return "<GPUAllocationOp>";
    }
    
    std::string GpuAllocationOp::getCacheID() const
    {
        return m_cacheID;
    }
    
    void GpuAllocationOp::setup()
    {
        // Create the cacheID
        std::ostringstream cacheIDStream;
        cacheIDStream.precision(FLOAT_DECIMALS);
        cacheIDStream << "<GpuAllocationOp ";;
        cacheIDStream << GpuAllocationToString(m_allocation) << " ";
        cacheIDStream << "min " << " " << m_min << " ";
        cacheIDStream << "max " << " " << m_max << " ";
        cacheIDStream << ">";
        
        m_cacheID = cacheIDStream.str();
    }
    
    void GpuAllocationOp::apply(float* rgbaBuffer, long numPixels) const
    { }
    
    bool GpuAllocationOp::supportsGpuShader() const
    {
        // TODO: throw exception instead?
        return true;
    }

    GpuAllocation GpuAllocationOp::getAllocation() const
    {
        return m_allocation;
    }
    
    float GpuAllocationOp::getMin() const
    {
        return m_min;
    }
    
    float GpuAllocationOp::getMax() const
    {
        return m_max;
    }
    
    void CreateGpuAllocationOp(LocalProcessor & processor,
                              GpuAllocation allocation,
                              float min, float max)
    {
        processor.registerOp( GpuAllocationOpRcPtr(new GpuAllocationOp(allocation, min, max)) );
    }

}
OCIO_NAMESPACE_EXIT
