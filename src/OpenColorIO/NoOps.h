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


#ifndef INCLUDED_OCIO_GPUALLOCATIONNOOP_H
#define INCLUDED_OCIO_GPUALLOCATIONNOOP_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"

#include <vector>

OCIO_NAMESPACE_ENTER
{
    void CreateGpuAllocationNoOp(OpRcPtrVec & ops,
                                 const AllocationData & allocationData);
    
    
    // Partition an opvec into 3 segments for GPU Processing
    //
    // gpuLatticeOps need not support analytical gpu shader generation
    // the pre and post ops must support analytical generation.
    //
    // Additional ops will optinally be inserted to take into account
    // allocation transformations
    
    void PartitionGPUOps(OpRcPtrVec & gpuPreOps,
                         OpRcPtrVec & gpuLatticeOps,
                         OpRcPtrVec & gpuPostOps,
                         const OpRcPtrVec & ops);
    
    void CreateFileNoOp(OpRcPtrVec & ops,
                        const std::string & fname);
    
    void CreateLookNoOp(OpRcPtrVec & ops,
                        const std::string & lookName);
    
}
OCIO_NAMESPACE_EXIT

#endif
