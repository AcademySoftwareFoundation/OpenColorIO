// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


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
    
    void BuildExponentWithLinearOps(OpRcPtrVec & ops,
                                    const Config& config,
                                    const ExponentWithLinearTransform & transform,
                                    TransformDirection dir);
    
    void BuildExposureContrastOps(OpRcPtrVec & ops,
                                  const Config& config,
                                  const ExposureContrastTransform & transform,
                                  TransformDirection dir);
    
    void BuildFileTransformOps(OpRcPtrVec & ops,
                               const Config& config,
                               const ConstContextRcPtr & context,
                               const FileTransform & transform,
                               TransformDirection dir);
    
    void BuildFixedFunctionOps(OpRcPtrVec & ops,
                               const Config & config,
                               const ConstContextRcPtr & context,
                               const FixedFunctionTransform & transform,
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
    
    void BuildLogOps(OpRcPtrVec & ops,
                     const Config& config,
                     const LogAffineTransform& transform,
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
    
    void BuildLut1DOps(OpRcPtrVec & ops,
                       const Config& config,
                       const LUT1DTransform & transform,
                       TransformDirection dir);
    
    void BuildLut3DOps(OpRcPtrVec & ops,
                       const Config& config,
                       const LUT3DTransform & transform,
                       TransformDirection dir);

    void BuildMatrixOps(OpRcPtrVec & ops,
                        const Config& config,
                        const MatrixTransform & transform,
                        TransformDirection dir);
    
    void BuildRangeOps(OpRcPtrVec & ops,
                       const Config& config,
                       const RangeTransform & transform,
                       TransformDirection dir);
}
OCIO_NAMESPACE_EXIT

#endif
