// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/allocation/AllocationOp.h"
#include "NoOps.h"
#include "OpBuilders.h"
#include "Op.h"

namespace OCIO_NAMESPACE
{
namespace
{
class AllocationNoOp : public Op
{
public:
    AllocationNoOp() = delete;
    AllocationNoOp(const AllocationNoOp &) = delete;
    AllocationNoOp& operator=(const AllocationNoOp &) = delete;

    explicit AllocationNoOp(const AllocationData & allocationData)
        :   Op()
        ,   m_allocationData(allocationData)
    {
        data().reset(new NoOpData());
    }

    virtual ~AllocationNoOp() {}

    OpRcPtr clone() const override;

    std::string getInfo() const override { return "<AllocationNoOp>"; }

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;

    std::string getCacheID() const override;

    ConstOpCPURcPtr getCPUOp(bool /*fastLogExpPow*/) const override { return nullptr; }

    void apply(void * img, long numPixels) const override
    { apply(img, img, numPixels); }

    void apply(const void * inImg, void * outImg, long numPixels) const override
    { memcpy(outImg, inImg, numPixels * 4 * sizeof(float)); }

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & /*shaderCreator*/) const override {}

    void getGpuAllocation(AllocationData & allocation) const;

private:
    AllocationData m_allocationData;
};

typedef OCIO_SHARED_PTR<AllocationNoOp> AllocationNoOpRcPtr;
typedef OCIO_SHARED_PTR<const AllocationNoOp> ConstAllocationNoOpRcPtr;

OpRcPtr AllocationNoOp::clone() const
{
    return std::make_shared<AllocationNoOp>(m_allocationData);
}

bool AllocationNoOp::isSameType(ConstOpRcPtr & op) const
{
    ConstAllocationNoOpRcPtr typedRcPtr = DynamicPtrCast<const AllocationNoOp>(op);
    if(!typedRcPtr) return false;
    return true;
}

bool AllocationNoOp::isInverse(ConstOpRcPtr & op) const
{
    if(!isSameType(op)) return false;
    return true;
}

std::string AllocationNoOp::getCacheID() const
{
    throw Exception("AllocationNoOp::getCacheID should never be called. "
                    "NoOp types should have been removed.");
}

void AllocationNoOp::getGpuAllocation(AllocationData & allocation) const
{
    allocation = m_allocationData;
}

// Return whether the op defines an Allocation
bool DefinesGpuAllocation(const OpRcPtr & op)
{
    ConstAllocationNoOpRcPtr allocationNoOpRcPtr =
        DynamicPtrCast<const AllocationNoOp>(op);

    if(allocationNoOpRcPtr) return true;
    return false;
}
}

void CreateGpuAllocationNoOp(OpRcPtrVec & ops,
                                const AllocationData & allocationData)
{
    ops.push_back( std::make_shared<AllocationNoOp>(allocationData) );
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

        if(!opVec[i]->supportedByLegacyShader())
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

    // Write the entire shader using only shader text (3D LUT is unused)
    if(gpuLut3DOpStartIndex == -1 && gpuLut3DOpEndIndex == -1)
    {
        for(unsigned int i=0; i<ops.size(); ++i)
        {
            gpuPreOps.push_back( ops[i]->clone() );
        }
    }
    // Analytical -> 3D LUT -> analytical
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
            CreateAllocationOps(gpuPreOps, allocation, TRANSFORM_DIR_FORWARD);
            CreateAllocationOps(gpuLatticeOps, allocation, TRANSFORM_DIR_INVERSE);
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
        if(!gpuPreOps[i]->supportedByLegacyShader())
        {
            throw Exception("Partition failed check. One gpuPreOps op does not support GPU.");
        }
    }

    // If there are any lattice ops, at lease one must NOT support GPU
    // shaders (otherwise this block isnt necessary!)
    if(gpuLatticeOps.size()>0)
    {
        bool requiresLattice = false;
        for(unsigned int i=0; i<gpuLatticeOps.size() && !requiresLattice; ++i)
        {
            if (!gpuLatticeOps[i]->supportedByLegacyShader())
            {
                requiresLattice = true;
            }
        }

        if(!requiresLattice)
        {
            throw Exception("Partition failed check. All gpuLatticeOps ops do support GPU.");
        }
    }

    // All gpu post ops must support analytical gpu shader generation
    for(unsigned int i=0; i<gpuPostOps.size(); ++i)
    {
        if(!gpuPostOps[i]->supportedByLegacyShader())
        {
            throw Exception("Partition failed check. One gpuPostOps op does not support GPU.");
        }
    }
}

////////////////////////////////////////////////////////////////////////////

namespace
{
class FileNoOp : public Op
{
public:
    FileNoOp() = delete;
    FileNoOp(const FileNoOp &) = delete;
    FileNoOp& operator=(const FileNoOp &) = delete;

    explicit FileNoOp(const std::string & fileReference)
        : Op()
    {
        data().reset(new FileNoOpData(fileReference));
    }

    virtual ~FileNoOp() {}

    OpRcPtr clone() const override;

    std::string getInfo() const override { return "<FileNoOp>"; }

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    void dumpMetadata(ProcessorMetadataRcPtr & metadata) const override;

    std::string getCacheID() const override;

    ConstOpCPURcPtr getCPUOp(bool /*fastLogExpPow*/) const override { return nullptr; }

    void apply(void * img, long numPixels) const override
    { apply(img, img, numPixels); }

    void apply(const void * inImg, void * outImg, long numPixels) const override
    { memcpy(outImg, inImg, numPixels * 4 * sizeof(float)); }

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & /*shaderCreator*/) const override {}

private:
    std::string m_fileReference;
};

typedef OCIO_SHARED_PTR<FileNoOp> FileNoOpRcPtr;
typedef OCIO_SHARED_PTR<const FileNoOp> ConstFileNoOpRcPtr;

OpRcPtr FileNoOp::clone() const
{
    auto fileData = DynamicPtrCast<const FileNoOpData>(data());
    return std::make_shared<FileNoOp>(fileData->getPath());
}

bool FileNoOp::isSameType(ConstOpRcPtr & op) const
{
    ConstFileNoOpRcPtr typedRcPtr = DynamicPtrCast<const FileNoOp>(op);
    if(!typedRcPtr) return false;
    return true;
}

bool FileNoOp::isInverse(ConstOpRcPtr & op) const
{
    return isSameType(op);
}

void FileNoOp::dumpMetadata(ProcessorMetadataRcPtr & metadata) const
{
    auto fileData = DynamicPtrCast<const FileNoOpData>(data());
    metadata->addFile(fileData->getPath().c_str());
}

std::string FileNoOp::getCacheID() const
{
    throw Exception("FileNoOp::getCacheID should never be called. "
                    "NoOp types should have been removed.");
}

}

void CreateFileNoOp(OpRcPtrVec & ops,
                    const std::string & fileReference)
{
    ops.push_back( std::make_shared<FileNoOp>(fileReference) );
}




////////////////////////////////////////////////////////////////////////////

namespace
{
class LookNoOp : public Op
{
public:
    LookNoOp() = delete;
    LookNoOp(const LookNoOp &) = delete;
    LookNoOp& operator=(const LookNoOp &) = delete;

    explicit LookNoOp(const std::string & look)
        :   Op()
        ,   m_look(look)
    {
        data().reset(new NoOpData());
    }

    virtual ~LookNoOp() {}

    OpRcPtr clone() const override;

    std::string getInfo() const override { return "<LookNoOp>"; }

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    void dumpMetadata(ProcessorMetadataRcPtr & metadata) const override;

    std::string getCacheID() const override;

    ConstOpCPURcPtr getCPUOp(bool /*fastLogExpPow*/) const override { return nullptr; }

    void apply(void * img, long numPixels) const override
    { apply(img, img, numPixels); }

    void apply(const void * inImg, void * outImg, long numPixels) const override
    { memcpy(outImg, inImg, numPixels * 4 * sizeof(float)); }

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & /*shaderCreator*/) const override {}

private:
    std::string m_look;
};

typedef OCIO_SHARED_PTR<LookNoOp> LookNoOpRcPtr;
typedef OCIO_SHARED_PTR<const LookNoOp> ConstLookNoOpRcPtr;

OpRcPtr LookNoOp::clone() const
{
    return std::make_shared<LookNoOp>(m_look);
}

bool LookNoOp::isSameType(ConstOpRcPtr & op) const
{
    ConstLookNoOpRcPtr typedRcPtr = DynamicPtrCast<const LookNoOp>(op);
    if(!typedRcPtr) return false;
    return true;
}

bool LookNoOp::isInverse(ConstOpRcPtr & op) const
{
    return isSameType(op);
}

void LookNoOp::dumpMetadata(ProcessorMetadataRcPtr & metadata) const
{
    metadata->addLook(m_look.c_str());
}

std::string LookNoOp::getCacheID() const
{
    throw Exception("LookNoOp::getCacheID should never be called. "
                    "NoOp types should have been removed.");
}

}

void CreateLookNoOp(OpRcPtrVec & ops,
                    const std::string & look)
{
    ops.push_back( std::make_shared<LookNoOp>(look) );
}

} // namespace OCIO_NAMESPACE

