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

#include "GpuAllocationNoOp.h"

#include <sstream>

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        class GpuAllocationNoOp : public Op
        {
        public:
            GpuAllocationNoOp(const GpuAllocationData & allocationData);
            virtual ~GpuAllocationNoOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;
            
            virtual bool isNoOp() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual bool supportsGpuShader() const;
            virtual void writeGpuShader(std::ostringstream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const;
            
            virtual bool definesGpuAllocation() const;
            virtual GpuAllocationData getGpuAllocation() const;
            
            GpuAllocation getAllocation() const;
            float getMin() const;
            float getMax() const;
        
        private:
            GpuAllocationData m_allocationData;
            
            std::string m_cacheID;
        };
        
        
        
        GpuAllocationNoOp::GpuAllocationNoOp(const GpuAllocationData & allocationData) :
                        m_allocationData(allocationData)
        { };

        OpRcPtr GpuAllocationNoOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new GpuAllocationNoOp(m_allocationData));
            return op;
        }
        
        GpuAllocationNoOp::~GpuAllocationNoOp()
        {

        }

        std::string GpuAllocationNoOp::getInfo() const
        {
            return "<GPUAllocationNoOp>";
        }

        std::string GpuAllocationNoOp::getCacheID() const
        {
            return m_cacheID;
        }

        bool GpuAllocationNoOp::isNoOp() const
        {
            return true;
        }
        
        void GpuAllocationNoOp::finalize()
        {
            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<GpuAllocationOp ";
            cacheIDStream << m_allocationData.getCacheID();
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();
        }

        void GpuAllocationNoOp::apply(float* /*rgbaBuffer*/, long /*numPixels*/) const
        { }

        bool GpuAllocationNoOp::supportsGpuShader() const
        {
            return true;
        }

        void GpuAllocationNoOp::writeGpuShader(std::ostringstream & /*shader*/,
                                             const std::string & /*pixelName*/,
                                             const GpuShaderDesc & /*shaderDesc*/) const
        { }
        
        bool GpuAllocationNoOp::definesGpuAllocation() const
        {
            return true;
        }
        
        GpuAllocationData GpuAllocationNoOp::getGpuAllocation() const
        {
            return m_allocationData;
        }

    }
    
    
    void CreateGpuAllocationNoOp(OpRcPtrVec & ops,
                                 const GpuAllocationData & allocationData)
    {
        ops.push_back( OpRcPtr(new GpuAllocationNoOp(allocationData)) );
    }

}
OCIO_NAMESPACE_EXIT
