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


#ifndef INCLUDED_OCIO_OPBUILDERS_H
#define INCLUDED_OCIO_OPBUILDERS_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "LookParse.h"
#include "PrivateTypes.h"

OCIO_NAMESPACE_ENTER
{
    void BuildOps(OpRcPtrVec & ops,
                  const Config & config,
                  const ConstContextRcPtr & context,
                  const ConstTransformRcPtr & transform,
                  TransformDirection dir);
    
    
    ////////////////////////////////////////////////////////////////////////
    
    void BuildAllocationOps(OpRcPtrVec & ops,
                            const Config & config,
                            const AllocationTransform & transform,
                            TransformDirection dir);
    
    void BuildCDLOps(OpRcPtrVec & ops,
                     const Config & config,
                     const CDLTransform & transform,
                     TransformDirection dir);
    
    void BuildClampOps(OpRcPtrVec & ops,
                       const Config & config,
                       const ClampTransform & transform,
                       TransformDirection dir);

    void BuildColorSpaceOps(OpRcPtrVec & ops,
                            const Config& config,
                            const ConstContextRcPtr & context,
                            const ColorSpaceTransform & transform,
                            TransformDirection dir);
    
    void BuildColorSpaceOps(OpRcPtrVec & ops,
                            const Config & config,
                            const ConstContextRcPtr & context,
                            const ConstColorSpaceRcPtr & srcColorSpace,
                            const ConstColorSpaceRcPtr & dstColorSpace);
    
    void BuildDisplayOps(OpRcPtrVec & ops,
                         const Config & config,
                         const ConstContextRcPtr & context,
                         const DisplayTransform & transform,
                         TransformDirection dir);
    
    void BuildExponentOps(OpRcPtrVec & ops,
                          const Config& config,
                          const ExponentTransform & transform,
                          TransformDirection dir);
    
    void BuildFileOps(OpRcPtrVec & ops,
                      const Config& config,
                      const ConstContextRcPtr & context,
                      const FileTransform & transform,
                      TransformDirection dir);
    
    void BuildGroupOps(OpRcPtrVec & ops,
                       const Config& config,
                       const ConstContextRcPtr & context,
                       const GroupTransform & transform,
                       TransformDirection dir);
    
    void BuildLogOps(OpRcPtrVec & ops,
                     const Config& config,
                     const LogTransform& transform,
                     TransformDirection dir);
    
    void BuildLookOps(OpRcPtrVec & ops,
                      const Config& config,
                      const ConstContextRcPtr & context,
                      const LookTransform & lookTransform,
                      TransformDirection dir);
    
    void BuildLookOps(OpRcPtrVec & ops,
                      ConstColorSpaceRcPtr & currentColorSpace,
                      bool skipColorSpaceConversions,
                      const Config& config,
                      const ConstContextRcPtr & context,
                      const LookParseResult & looks);
    
    void BuildMatrixOps(OpRcPtrVec & ops,
                        const Config& config,
                        const MatrixTransform & transform,
                        TransformDirection dir);
    
    void BuildTruelightOps(OpRcPtrVec & ops,
                           const Config & config,
                           const TruelightTransform & transform,
                           TransformDirection dir);
}
OCIO_NAMESPACE_EXIT

#endif
