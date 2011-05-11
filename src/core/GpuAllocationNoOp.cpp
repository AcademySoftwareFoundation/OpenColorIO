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

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"
#include "Op.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        class AllocationNoOp : public Op
        {
        public:
            AllocationNoOp(const AllocationData & allocationData);
            virtual ~AllocationNoOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;
            
            virtual bool isNoOp() const;
            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual bool supportsGpuShader() const;
            virtual void writeGpuShader(std::ostream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const;
            
            virtual bool definesAllocation() const;
            virtual AllocationData getAllocation() const;
            
            float getMin() const;
            float getMax() const;
        
        private:
            AllocationData m_allocationData;
            
            std::string m_cacheID;
        };
        
        
        
        AllocationNoOp::AllocationNoOp(const AllocationData & allocationData) :
                        m_allocationData(allocationData)
        { }

        OpRcPtr AllocationNoOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new AllocationNoOp(m_allocationData));
            return op;
        }
        
        AllocationNoOp::~AllocationNoOp()
        {

        }

        std::string AllocationNoOp::getInfo() const
        {
            return "<AllocationNoOp>";
        }

        std::string AllocationNoOp::getCacheID() const
        {
            return m_cacheID;
        }

        bool AllocationNoOp::isNoOp() const
        {
            return true;
        }
        
        bool AllocationNoOp::hasChannelCrosstalk() const
        {
            return false;
        }
        
        void AllocationNoOp::finalize()
        {
            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<AllocationOp ";
            cacheIDStream << m_allocationData.getCacheID();
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();
        }

        void AllocationNoOp::apply(float* /*rgbaBuffer*/, long /*numPixels*/) const
        { }

        bool AllocationNoOp::supportsGpuShader() const
        {
            return true;
        }

        void AllocationNoOp::writeGpuShader(std::ostream & /*shader*/,
                                             const std::string & /*pixelName*/,
                                             const GpuShaderDesc & /*shaderDesc*/) const
        { }
        
        bool AllocationNoOp::definesAllocation() const
        {
            return true;
        }
        
        AllocationData AllocationNoOp::getAllocation() const
        {
            return m_allocationData;
        }

    }
    
    
    void CreateAllocationNoOp(OpRcPtrVec & ops,
                              const AllocationData & allocationData)
    {
        ops.push_back( OpRcPtr(new AllocationNoOp(allocationData)) );
    }

}
OCIO_NAMESPACE_EXIT
