// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GPUALLOCATIONNOOP_H
#define INCLUDED_OCIO_GPUALLOCATIONNOOP_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"

#include <vector>

namespace OCIO_NAMESPACE
{
void CreateGpuAllocationNoOp(OpRcPtrVec & ops,
                                const AllocationData & allocationData);


// Partition an opvec into 3 segments for GPU Processing
//
// gpuLatticeOps need not support analytical gpu shader generation
// the pre and post ops must support analytical generation.
//
// Additional ops will optionally be inserted to take into account
// allocation transformations

void PartitionGPUOps(OpRcPtrVec & gpuPreOps,
                        OpRcPtrVec & gpuLatticeOps,
                        OpRcPtrVec & gpuPostOps,
                        const OpRcPtrVec & ops);

void CreateFileNoOp(OpRcPtrVec & ops,
                    const std::string & fname);

void CreateLookNoOp(OpRcPtrVec & ops,
                    const std::string & lookName);

class NoOpData : public OpData
{
public:
    NoOpData() : OpData() { }
    NoOpData(const NoOpData &) = delete;

    Type getType() const override { return NoOpType; }
    bool isNoOp() const override { return true; }
    bool isIdentity() const override { return true; }
    bool hasChannelCrosstalk() const override { return false; }
    std::string getCacheID() const override
    {
        return "";
    }
    void validate() const override {}
};

// Need to declare this here in order to allow FileTransform to be able to
// detect if a ReferenceOpData references itself in a cycle (either
// directly or indirectly).
class FileNoOpData : public NoOpData
{
public:
    FileNoOpData() = delete;
    FileNoOpData(const FileNoOpData &) = delete;
    FileNoOpData(const std::string & path) : NoOpData(), m_path(path) { }

    const std::string & getPath() const { return m_path; }
    void setComplete() const { m_complete = true; }
    bool getComplete() const { return m_complete;  }

private:
    std::string m_path;
    // false while the file is still being loaded.
    mutable bool m_complete = false;
};

typedef OCIO_SHARED_PTR<FileNoOpData> FileNoOpDataRcPtr;
typedef OCIO_SHARED_PTR<const FileNoOpData> ConstFileNoOpDataRcPtr;


} // namespace OCIO_NAMESPACE

#endif
