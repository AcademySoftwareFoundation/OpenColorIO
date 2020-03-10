// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_OPBUILDERS_H
#define INCLUDED_OCIO_OPBUILDERS_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "LookParse.h"
#include "PrivateTypes.h"

namespace OCIO_NAMESPACE
{
void BuildOps(OpRcPtrVec & ops,
              const Config & config,
              const ConstContextRcPtr & context,
              const ConstTransformRcPtr & transform,
              TransformDirection dir);

////////////////////////////////////////////////////////////////////////

void BuildAllocationOp(OpRcPtrVec & ops,
                       const Config & config,
                       const AllocationTransform & transform,
                       TransformDirection dir);

void BuildCDLOp(OpRcPtrVec & ops,
                const Config & config,
                const CDLTransform & transform,
                TransformDirection dir);

void BuildColorSpaceOps(OpRcPtrVec & ops,
                        const Config & config,
                        const ConstContextRcPtr & context,
                        const ColorSpaceTransform & transform,
                        TransformDirection dir);

void BuildColorSpaceOps(OpRcPtrVec & ops,
                        const Config & config,
                        const ConstContextRcPtr & context,
                        const ConstColorSpaceRcPtr & srcColorSpace,
                        const ConstColorSpaceRcPtr & dstColorSpace);

void BuildColorSpaceToReferenceOps(OpRcPtrVec & ops,
                                   const Config & config,
                                   const ConstContextRcPtr & context,
                                   const ConstColorSpaceRcPtr & srcColorSpace);

void BuildColorSpaceFromReferenceOps(OpRcPtrVec & ops,
                                     const Config & config,
                                     const ConstContextRcPtr & context,
                                     const ConstColorSpaceRcPtr & dstColorSpace);

void BuildReferenceConversionOps(OpRcPtrVec & ops,
                                 const Config & config,
                                 const ConstContextRcPtr & context,
                                 ReferenceSpaceType srcReferenceSpace,
                                 ReferenceSpaceType dstReferenceSpace);

void BuildDisplayOps(OpRcPtrVec & ops,
                     const Config & config,
                     const ConstContextRcPtr & context,
                     const DisplayTransform & transform,
                     TransformDirection dir);

void BuildExponentOp(OpRcPtrVec & ops,
                     const Config & config,
                     const ExponentTransform & transform,
                     TransformDirection dir);

void BuildExponentWithLinearOp(OpRcPtrVec & ops,
                               const Config & config,
                               const ExponentWithLinearTransform & transform,
                               TransformDirection dir);

void BuildExposureContrastOp(OpRcPtrVec & ops,
                             const Config & config,
                             const ExposureContrastTransform & transform,
                             TransformDirection dir);

void BuildFileTransformOps(OpRcPtrVec & ops,
                           const Config & config,
                           const ConstContextRcPtr & context,
                           const FileTransform & transform,
                           TransformDirection dir);

void BuildFixedFunctionOp(OpRcPtrVec & ops,
                          const Config & config,
                          const ConstContextRcPtr & context,
                          const FixedFunctionTransform & transform,
                          TransformDirection dir);

void BuildGroupOps(OpRcPtrVec & ops,
                   const Config & config,
                   const ConstContextRcPtr & context,
                   const GroupTransform & transform,
                   TransformDirection dir);

void BuildLogOp(OpRcPtrVec & ops,
                const Config & config,
                const LogAffineTransform& transform,
                TransformDirection dir);

void BuildLogOp(OpRcPtrVec & ops,
                const Config & config,
                const LogCameraTransform& transform,
                TransformDirection dir);

void BuildLogOp(OpRcPtrVec & ops,
                const Config & config,
                const LogTransform& transform,
                TransformDirection dir);

void BuildLookOps(OpRcPtrVec & ops,
                  const Config & config,
                  const ConstContextRcPtr & context,
                  const LookTransform & lookTransform,
                  TransformDirection dir);

void BuildLookOps(OpRcPtrVec & ops,
                  ConstColorSpaceRcPtr & currentColorSpace,
                  bool skipColorSpaceConversions,
                  const Config & config,
                  const ConstContextRcPtr & context,
                  const LookParseResult & looks);

void BuildLut1DOp(OpRcPtrVec & ops,
                  const Config & config,
                  const Lut1DTransform & transform,
                  TransformDirection dir);

void BuildLut3DOp(OpRcPtrVec & ops,
                  const Config & config,
                  const Lut3DTransform & transform,
                  TransformDirection dir);

void BuildMatrixOp(OpRcPtrVec & ops,
                   const Config & config,
                   const MatrixTransform & transform,
                   TransformDirection dir);

void BuildRangeOp(OpRcPtrVec & ops,
                  const Config & config,
                  const RangeTransform & transform,
                  TransformDirection dir);

} // namespace OCIO_NAMESPACE

#endif
