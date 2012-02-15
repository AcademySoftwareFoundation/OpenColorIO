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

#include "AllocationOp.h"
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
            virtual bool isSameType(const OpRcPtr & op) const;
            virtual bool isInverse(const OpRcPtr & op) const;
            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual bool supportsGpuShader() const;
            virtual void writeGpuShader(std::ostream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const;
        
            void getGpuAllocation(AllocationData & allocation) const;
            
        private:
            AllocationData m_allocationData;
            
            std::string m_cacheID;
        };
        
        typedef OCIO_SHARED_PTR<AllocationNoOp> AllocationNoOpRcPtr;
        
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
        
        bool AllocationNoOp::isSameType(const OpRcPtr & op) const
        {
            AllocationNoOpRcPtr typedRcPtr = DynamicPtrCast<AllocationNoOp>(op);
            if(!typedRcPtr) return false;
            return true;
        }
        
        bool AllocationNoOp::isInverse(const OpRcPtr & op) const
        {
            if(!isSameType(op)) return false;
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
        
        void AllocationNoOp::getGpuAllocation(AllocationData & allocation) const
        {
            allocation = m_allocationData;
        }
        
        // Return whether the op defines an Allocation
        bool DefinesGpuAllocation(const OpRcPtr & op)
        {
            AllocationNoOpRcPtr allocationNoOpRcPtr = 
                DynamicPtrCast<AllocationNoOp>(op);
            
            if(allocationNoOpRcPtr) return true;
            return false;
        }
    }
    
    void CreateGpuAllocationNoOp(OpRcPtrVec & ops,
                                 const AllocationData & allocationData)
    {
        ops.push_back( AllocationNoOpRcPtr(new AllocationNoOp(allocationData)) );
    }
    
    
    namespace
    {
        // Find the minimal index range in the opVec that does not support
        // shader text generation.  The endIndex *is* inclusive.
        // 
        // I.e., if the entire opVec does not support GPUShaders, the
        // result will be startIndex = 0, endIndex = opVec.size() - 1
        // 
        // If the entire opVec supports GPU generation, both the
        // startIndex and endIndex will equal -1
        
        void GetGpuUnsupportedIndexRange(int * startIndex, int * endIndex,
                                         const OpRcPtrVec & opVec)
        {
            int start = -1;
            int end = -1;
            
            for(unsigned int i=0; i<opVec.size(); ++i)
            {
                // We've found a gpu unsupported op.
                // If it's the first, save it as our start.
                // Otherwise, update the end.
                
                if(!opVec[i]->supportsGpuShader())
                {
                    if(start<0)
                    {
                        start = i;
                        end = i;
                    }
                    else end = i;
                }
            }
            
            // Now that we've found a startIndex, walk back until we find
            // one that defines a GpuAllocation. (we can only upload to
            // the gpu at a location are tagged with an allocation)
            
            while(start>0)
            {
                if(DefinesGpuAllocation(opVec[start])) break;
                 --start;
            }
            
            if(startIndex) *startIndex = start;
            if(endIndex) *endIndex = end;
        }
        
        
        bool GetGpuAllocation(AllocationData & allocation,
                              const OpRcPtr & op)
        {
            AllocationNoOpRcPtr allocationNoOpRcPtr = 
                DynamicPtrCast<AllocationNoOp>(op);
            
            if(!allocationNoOpRcPtr)
            {
                return false;
            }
            
            allocationNoOpRcPtr->getGpuAllocation(allocation);
            return true;
        }
    }
    
    
    void PartitionGPUOps(OpRcPtrVec & gpuPreOps,
                         OpRcPtrVec & gpuLatticeOps,
                         OpRcPtrVec & gpuPostOps,
                         const OpRcPtrVec & ops)
    {
        //
        // Partition the original, raw opvec into 3 segments for GPU Processing
        //
        // gpuLatticeOps need not support analytical gpu shader generation
        // the pre and post ops must support analytical generation.
        // Additional ops will be inserted to take into account allocations
        // transformations.
        
        
        // This is used to bound our analytical shader text generation
        // start index and end index are inclusive.
        
        int gpuLut3DOpStartIndex = 0;
        int gpuLut3DOpEndIndex = 0;
        GetGpuUnsupportedIndexRange(&gpuLut3DOpStartIndex,
                                    &gpuLut3DOpEndIndex,
                                    ops);
        
        // Write the entire shader using only shader text (3d lut is unused)
        if(gpuLut3DOpStartIndex == -1 && gpuLut3DOpEndIndex == -1)
        {
            for(unsigned int i=0; i<ops.size(); ++i)
            {
                gpuPreOps.push_back( ops[i]->clone() );
            }
        }
        // Analytical -> 3dlut -> analytical
        else
        {
            // Handle analytical shader block before start index.
            for(int i=0; i<gpuLut3DOpStartIndex; ++i)
            {
                gpuPreOps.push_back( ops[i]->clone() );
            }
            
            // Get the GPU Allocation at the cross-over point
            // Create 2 symmetrically canceling allocation ops,
            // where the shader text moves to a nicely allocated LDR
            // (low dynamic range color space), and the lattice processing
            // does the inverse (making the overall operation a no-op
            // color-wise
            
            AllocationData allocation;
            if(gpuLut3DOpStartIndex<0 || gpuLut3DOpStartIndex>=(int)ops.size())
            {
                std::ostringstream error;
                error << "Invalid GpuUnsupportedIndexRange: ";
                error << "gpuLut3DOpStartIndex: " << gpuLut3DOpStartIndex << " ";
                error << "gpuLut3DOpEndIndex: " << gpuLut3DOpEndIndex << " ";
                error << "cpuOps.size: " << ops.size();
                throw Exception(error.str().c_str());
            }
            
            if(!GetGpuAllocation(allocation, ops[gpuLut3DOpStartIndex]))
            {
                std::ostringstream error;
                error << "Specified GpuAllocation could not be queried at ";
                error << "index " << gpuLut3DOpStartIndex << " in cpuOps.";
                throw Exception(error.str().c_str());
            }
            
            CreateAllocationOps(gpuPreOps, allocation, TRANSFORM_DIR_FORWARD);
            CreateAllocationOps(gpuLatticeOps, allocation, TRANSFORM_DIR_INVERSE);
            
            // Handle cpu lattice processing
            for(int i=gpuLut3DOpStartIndex; i<=gpuLut3DOpEndIndex; ++i)
            {
                gpuLatticeOps.push_back( ops[i]->clone() );
            }
            
            // And then handle the gpu post processing
            for(int i=gpuLut3DOpEndIndex+1; i<(int)ops.size(); ++i)
            {
                gpuPostOps.push_back( ops[i]->clone() );
            }
        }
    }
}
OCIO_NAMESPACE_EXIT
