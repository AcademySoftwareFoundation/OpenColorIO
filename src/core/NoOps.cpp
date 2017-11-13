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
#include "NoOps.h"
#include "OpBuilders.h"
#include "Op.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        class AllocationNoOp : public Op
        {
        public:
            AllocationNoOp(const AllocationData & allocationData):
                m_allocationData(allocationData) {}
            virtual ~AllocationNoOp() {}
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const { return "<AllocationNoOp>"; }
            virtual std::string getCacheID() const { return ""; }
            
            virtual bool isNoOp() const { return true; }
            virtual bool isSameType(const OpRcPtr & op) const;
            virtual bool isInverse(const OpRcPtr & op) const;
            virtual bool hasChannelCrosstalk() const { return false; }
            virtual void finalize() { }
            virtual void apply(float* /*rgbaBuffer*/, long /*numPixels*/) const { }
            
            virtual bool supportsGpuShader() const { return true; }
            virtual void writeGpuShader(std::ostream & /*shader*/,
                                        const std::string & /*pixelName*/,
                                        const GpuShaderDesc & /*shaderDesc*/) const
            { }
        
            void getGpuAllocation(AllocationData & allocation) const;
            
        private:
            AllocationData m_allocationData;
        };
        
        typedef OCIO_SHARED_PTR<AllocationNoOp> AllocationNoOpRcPtr;
        
        OpRcPtr AllocationNoOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new AllocationNoOp(m_allocationData));
            return op;
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
            
            // If the specified location defines an allocation, use it.
            // It's possible that this index wont define an allocation.
            // (For example in the case of getProcessor(FileTransform)
            if(GetGpuAllocation(allocation, ops[gpuLut3DOpStartIndex]))
            {
                CreateAllocationOps(gpuPreOps, allocation,
                    TRANSFORM_DIR_FORWARD);
                CreateAllocationOps(gpuLatticeOps, allocation,
                    TRANSFORM_DIR_INVERSE);
            }
            
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
    
    void AssertPartitionIntegrity(OpRcPtrVec & gpuPreOps,
                                  OpRcPtrVec & gpuLatticeOps,
                                  OpRcPtrVec & gpuPostOps)
    {
        // All gpu pre ops must support analytical gpu shader generation
        for(unsigned int i=0; i<gpuPreOps.size(); ++i)
        {
            if(!gpuPreOps[i]->supportsGpuShader())
            {
                throw Exception("Patition failed check. gpuPreOps");
            }
        }
        
        // If there are any lattice ops, at lease one must NOT support GPU
        // shaders (otherwise this block isnt necessary!)
        if(gpuLatticeOps.size()>0)
        {
            bool requireslattice = false;
            for(unsigned int i=0; i<gpuLatticeOps.size(); ++i)
            {
                if(!gpuLatticeOps[i]->supportsGpuShader()) requireslattice = true;
            }
            
            if(!requireslattice)
            {
                throw Exception("Patition failed check. gpuLatticeOps");
            }
        }
        
        // All gpu post ops must support analytical gpu shader generation
        for(unsigned int i=0; i<gpuPostOps.size(); ++i)
        {
            if(!gpuPostOps[i]->supportsGpuShader())
            {
                throw Exception("Patition failed check. gpuPostOps");
            }
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    
    namespace
    {
        class FileNoOp : public Op
        {
        public:
            FileNoOp(const std::string & fileReference):
                m_fileReference(fileReference) {}
            virtual ~FileNoOp() {}
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const { return "<FileNoOp>"; }
            virtual std::string getCacheID() const { return ""; }
            
            virtual bool isNoOp() const { return true; }
            virtual bool isSameType(const OpRcPtr & op) const;
            virtual bool isInverse(const OpRcPtr & op) const;
            virtual bool hasChannelCrosstalk() const { return false; }
            virtual void dumpMetadata(ProcessorMetadataRcPtr & metadata) const;
            
            virtual void finalize() {}
            virtual void apply(float* /*rgbaBuffer*/, long /*numPixels*/) const {}
            
            virtual bool supportsGpuShader() const { return true; }
            virtual void writeGpuShader(std::ostream & /*shader*/,
                                        const std::string & /*pixelName*/,
                                        const GpuShaderDesc & /*shaderDesc*/) const
            { }
            
        private:
            std::string m_fileReference;
        };
        
        typedef OCIO_SHARED_PTR<FileNoOp> FileNoOpRcPtr;
        
        OpRcPtr FileNoOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new FileNoOp(m_fileReference));
            return op;
        }
        
        bool FileNoOp::isSameType(const OpRcPtr & op) const
        {
            FileNoOpRcPtr typedRcPtr = DynamicPtrCast<FileNoOp>(op);
            if(!typedRcPtr) return false;
            return true;
        }
        
        bool FileNoOp::isInverse(const OpRcPtr & op) const
        {
            return isSameType(op);
        }
        
        void FileNoOp::dumpMetadata(ProcessorMetadataRcPtr & metadata) const
        {
            metadata->addFile(m_fileReference.c_str());
        }
    }
    
    void CreateFileNoOp(OpRcPtrVec & ops,
                                 const std::string & fileReference)
    {
        ops.push_back( FileNoOpRcPtr(new FileNoOp(fileReference)) );
    }
    
    
    
    
    ////////////////////////////////////////////////////////////////////////////
    
    namespace
    {
        class LookNoOp : public Op
        {
        public:
            LookNoOp(const std::string & look):
                m_look(look) {}
            virtual ~LookNoOp() {}
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const { return "<LookNoOp>"; }
            virtual std::string getCacheID() const { return ""; }
            
            virtual bool isNoOp() const { return true; }
            virtual bool isSameType(const OpRcPtr & op) const;
            virtual bool isInverse(const OpRcPtr & op) const;
            virtual bool hasChannelCrosstalk() const { return false; }
            virtual void dumpMetadata(ProcessorMetadataRcPtr & metadata) const;
            
            virtual void finalize() {}
            virtual void apply(float* /*rgbaBuffer*/, long /*numPixels*/) const {}
            
            virtual bool supportsGpuShader() const { return true; }
            virtual void writeGpuShader(std::ostream & /*shader*/,
                                        const std::string & /*pixelName*/,
                                        const GpuShaderDesc & /*shaderDesc*/) const
            { }
            
        private:
            std::string m_look;
        };
        
        typedef OCIO_SHARED_PTR<LookNoOp> LookNoOpRcPtr;
        
        OpRcPtr LookNoOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new LookNoOp(m_look));
            return op;
        }
        
        bool LookNoOp::isSameType(const OpRcPtr & op) const
        {
            LookNoOpRcPtr typedRcPtr = DynamicPtrCast<LookNoOp>(op);
            if(!typedRcPtr) return false;
            return true;
        }
        
        bool LookNoOp::isInverse(const OpRcPtr & op) const
        {
            return isSameType(op);
        }
        
        void LookNoOp::dumpMetadata(ProcessorMetadataRcPtr & metadata) const
        {
            metadata->addLook(m_look.c_str());
        }
    }
    
    void CreateLookNoOp(OpRcPtrVec & ops,
                        const std::string & look)
    {
        ops.push_back( LookNoOpRcPtr(new LookNoOp(look)) );
    }
    
}
OCIO_NAMESPACE_EXIT



///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

OCIO_NAMESPACE_USING

#include "UnitTest.h"
#include "Lut1DOp.h"
#include "MatrixOps.h"

void CreateGenericAllocationOp(OpRcPtrVec & ops)
{
    AllocationData srcAllocation;
    srcAllocation.allocation = ALLOCATION_LG2;
    srcAllocation.vars.push_back(-8.0f);
    srcAllocation.vars.push_back(8.0f);
    CreateGpuAllocationNoOp(ops, srcAllocation);
}

void CreateGenericScaleOp(OpRcPtrVec & ops)
{
    float scale4[4] = { 1.04f, 1.05f, 1.06f, 1.0f };
    CreateScaleOp(ops, scale4, TRANSFORM_DIR_FORWARD);
}

void CreateGenericLutOp(OpRcPtrVec & ops)
{
    // Make a lut that squares the input
    Lut1DRcPtr lut = Lut1D::Create();
    {
        lut->from_min[0] = 0.0f;
        lut->from_min[1] = 0.0f;
        lut->from_min[2] = 0.0f;
        lut->from_max[0] = 1.0f;
        lut->from_max[1] = 1.0f;
        lut->from_max[2] = 1.0f;
        int size = 256;
        for(int i=0; i<size; ++i)
        {
            float x = (float)i / (float)(size-1);
            float x2 = x*x;
            
            for(int c=0; c<3; ++c)
            {
                lut->luts[c].push_back(x2);
            }
        }
    }
    
    CreateLut1DOp(ops, lut, INTERP_LINEAR, TRANSFORM_DIR_FORWARD);
}

OIIO_ADD_TEST(NoOps, PartitionGPUOps)
{
    {
    OpRcPtrVec ops;
    
    OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops);
    
    OIIO_CHECK_EQUAL(gpuPreOps.size(), 0);
    OIIO_CHECK_EQUAL(gpuLatticeOps.size(), 0);
    OIIO_CHECK_EQUAL(gpuPostOps.size(), 0);
    
    OIIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    }
    
    {
    OpRcPtrVec ops;
    CreateGenericAllocationOp(ops);
    
    OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops);
    
    OIIO_CHECK_EQUAL(gpuPreOps.size(), 1);
    OIIO_CHECK_EQUAL(gpuLatticeOps.size(), 0);
    OIIO_CHECK_EQUAL(gpuPostOps.size(), 0);
    
    OIIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    }
    
    {
    OpRcPtrVec ops;
    
    CreateGenericAllocationOp(ops);
    CreateGenericScaleOp(ops);
    
    OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops);
    
    OIIO_CHECK_EQUAL(gpuPreOps.size(), 2);
    OIIO_CHECK_EQUAL(gpuLatticeOps.size(), 0);
    OIIO_CHECK_EQUAL(gpuPostOps.size(), 0);
    
    OIIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    }
    
    {
    OpRcPtrVec ops;
    
    CreateGenericAllocationOp(ops);
    CreateGenericLutOp(ops);
    CreateGenericScaleOp(ops);
    
    OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops);
    
    OIIO_CHECK_EQUAL(gpuPreOps.size(), 2);
    OIIO_CHECK_EQUAL(gpuLatticeOps.size(), 4);
    OIIO_CHECK_EQUAL(gpuPostOps.size(), 1);
    
    OIIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    }
    
    {
    OpRcPtrVec ops;
    
    CreateGenericLutOp(ops);
    
    OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops);
    
    OIIO_CHECK_EQUAL(gpuPreOps.size(), 0);
    OIIO_CHECK_EQUAL(gpuLatticeOps.size(), 1);
    OIIO_CHECK_EQUAL(gpuPostOps.size(), 0);
    
    OIIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    }
    
    {
    OpRcPtrVec ops;
    
    CreateGenericLutOp(ops);
    CreateGenericScaleOp(ops);
    CreateGenericAllocationOp(ops);
    CreateGenericLutOp(ops);
    CreateGenericScaleOp(ops);
    CreateGenericAllocationOp(ops);
    
    OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops);
    
    OIIO_CHECK_EQUAL(gpuPreOps.size(), 0);
    OIIO_CHECK_EQUAL(gpuLatticeOps.size(), 4);
    OIIO_CHECK_EQUAL(gpuPostOps.size(), 2);
    
    OIIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    }
    
    {
    OpRcPtrVec ops;
    
    CreateGenericAllocationOp(ops);
    CreateGenericScaleOp(ops);
    CreateGenericLutOp(ops);
    CreateGenericScaleOp(ops);
    CreateGenericAllocationOp(ops);
    CreateGenericLutOp(ops);
    CreateGenericScaleOp(ops);
    CreateGenericAllocationOp(ops);
    
    OpRcPtrVec gpuPreOps, gpuLatticeOps, gpuPostOps;
    PartitionGPUOps(gpuPreOps, gpuLatticeOps, gpuPostOps, ops);
    
    OIIO_CHECK_EQUAL(gpuPreOps.size(), 2);
    OIIO_CHECK_EQUAL(gpuLatticeOps.size(), 8);
    OIIO_CHECK_EQUAL(gpuPostOps.size(), 2);
    
    OIIO_CHECK_NO_THROW( AssertPartitionIntegrity(gpuPreOps,
                                                  gpuLatticeOps,
                                                  gpuPostOps) );
    /*
    std::cerr << "gpuPreOps" << std::endl;
    std::cerr << SerializeOpVec(gpuPreOps, 4) << std::endl;
    std::cerr << "gpuLatticeOps" << std::endl;
    std::cerr << SerializeOpVec(gpuLatticeOps, 4) << std::endl;
    std::cerr << "gpuPostOps" << std::endl;
    std::cerr << SerializeOpVec(gpuPostOps, 4) << std::endl;
    */
    }
} // PartitionGPUOps

#endif // OCIO_UNIT_TEST
