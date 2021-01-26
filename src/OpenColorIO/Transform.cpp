// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include <typeinfo>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/FormatMetadata.h"
#include "OpBuilders.h"
#include "ops/cdl/CDLOp.h"
#include "ops/exponent/ExponentOp.h"
#include "ops/exposurecontrast/ExposureContrastOp.h"
#include "ops/fixedfunction/FixedFunctionOp.h"
#include "ops/gamma/GammaOp.h"
#include "ops/gradingprimary/GradingPrimaryOp.h"
#include "ops/gradingrgbcurve/GradingRGBCurveOp.h"
#include "ops/gradingtone/GradingToneOp.h"
#include "ops/log/LogOp.h"
#include "ops/lut1d/Lut1DOp.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ops/matrix/MatrixOp.h"
#include "ops/range/RangeOp.h"
#include "Processor.h"
#include "TransformBuilder.h"


namespace OCIO_NAMESPACE
{
void Transform::validate() const
{
    if (getDirection() != TRANSFORM_DIR_FORWARD
        && getDirection() != TRANSFORM_DIR_INVERSE)
    {
        std::string err(typeid(*this).name());
        err += ": invalid direction.";

        throw Exception(err.c_str());
    }
}

void BuildOps(OpRcPtrVec & ops,
              const Config & config,
              const ConstContextRcPtr & context,
              const ConstTransformRcPtr & transform,
              TransformDirection dir)
{
    // A null transform is valid, and corresponds to a no-op.
    if(!transform)
        return;

    if(ConstAllocationTransformRcPtr allocationTransform = \
        DynamicPtrCast<const AllocationTransform>(transform))
    {
        BuildAllocationOp(ops, *allocationTransform, dir);
    }
    else if(ConstBuiltinTransformRcPtr builtInTransform = \
        DynamicPtrCast<const BuiltinTransform>(transform))
    {
        BuildBuiltinOps(ops, *builtInTransform, dir);
    }
    else if(ConstCDLTransformRcPtr cdlTransform = \
        DynamicPtrCast<const CDLTransform>(transform))
    {
        BuildCDLOp(ops, config, *cdlTransform, dir);
    }
    else if(ConstColorSpaceTransformRcPtr colorSpaceTransform = \
        DynamicPtrCast<const ColorSpaceTransform>(transform))
    {
        BuildColorSpaceOps(ops, config, context, *colorSpaceTransform, dir);
    }
    else if(ConstDisplayViewTransformRcPtr displayViewTransform = \
        DynamicPtrCast<const DisplayViewTransform>(transform))
    {
        BuildDisplayOps(ops, config, context, *displayViewTransform, dir);
    }
    else if(ConstExponentTransformRcPtr exponentTransform = \
        DynamicPtrCast<const ExponentTransform>(transform))
    {
        BuildExponentOp(ops, config, *exponentTransform, dir);
    }
    else if(ConstExponentWithLinearTransformRcPtr expWithLinearTransform = \
        DynamicPtrCast<const ExponentWithLinearTransform>(transform))
    {
        BuildExponentWithLinearOp(ops, *expWithLinearTransform, dir);
    }
    else if (ConstExposureContrastTransformRcPtr ecTransform = \
        DynamicPtrCast<const ExposureContrastTransform>(transform))
    {
        BuildExposureContrastOp(ops, *ecTransform, dir);
    }
    else if(ConstFileTransformRcPtr fileTransform = \
        DynamicPtrCast<const FileTransform>(transform))
    {
        BuildFileTransformOps(ops, config, context, *fileTransform, dir);
    }
    else if (ConstFixedFunctionTransformRcPtr fixedFunctionTransform = \
        DynamicPtrCast<const FixedFunctionTransform>(transform))
    {
        BuildFixedFunctionOp(ops, *fixedFunctionTransform, dir);
    }
    else if (ConstGradingPrimaryTransformRcPtr gradingPrimaryTransform = \
        DynamicPtrCast<const GradingPrimaryTransform>(transform))
    {
        BuildGradingPrimaryOp(ops, config, context, *gradingPrimaryTransform, dir);
    }
    else if (ConstGradingRGBCurveTransformRcPtr gradingCurveTransform = \
        DynamicPtrCast<const GradingRGBCurveTransform>(transform))
    {
        BuildGradingRGBCurveOp(ops, config, context, *gradingCurveTransform, dir);
    }
    else if (ConstGradingToneTransformRcPtr gradingToneTransform = \
        DynamicPtrCast<const GradingToneTransform>(transform))
    {
        BuildGradingToneOp(ops, config, context, *gradingToneTransform, dir);
    }
    else if(ConstGroupTransformRcPtr groupTransform = \
        DynamicPtrCast<const GroupTransform>(transform))
    {
        BuildGroupOps(ops, config, context, *groupTransform, dir);
    }
    else if(ConstLogAffineTransformRcPtr logAffineTransform = \
        DynamicPtrCast<const LogAffineTransform>(transform))
    {
        BuildLogOp(ops, *logAffineTransform, dir);
    }
    else if(ConstLogCameraTransformRcPtr logCameraTransform = \
        DynamicPtrCast<const LogCameraTransform>(transform))
    {
        BuildLogOp(ops, *logCameraTransform, dir);
    }
    else if(ConstLogTransformRcPtr logTransform = \
        DynamicPtrCast<const LogTransform>(transform))
    {
        BuildLogOp(ops, *logTransform, dir);
    }
    else if(ConstLookTransformRcPtr lookTransform = \
        DynamicPtrCast<const LookTransform>(transform))
    {
        BuildLookOps(ops, config, context, *lookTransform, dir);
    }
    else if (ConstLut1DTransformRcPtr lut1dTransform = \
        DynamicPtrCast<const Lut1DTransform>(transform))
    {
        BuildLut1DOp(ops, *lut1dTransform, dir);
    }
    else if (ConstLut3DTransformRcPtr lut3dTransform = \
        DynamicPtrCast<const Lut3DTransform>(transform))
    {
        BuildLut3DOp(ops, *lut3dTransform, dir);
    }
    else if(ConstMatrixTransformRcPtr matrixTransform = \
        DynamicPtrCast<const MatrixTransform>(transform))
    {
        BuildMatrixOp(ops, *matrixTransform, dir);
    }
    else if(ConstRangeTransformRcPtr rangeTransform = \
        DynamicPtrCast<const RangeTransform>(transform))
    {
        BuildRangeOp(ops, *rangeTransform, dir);
    }
    else
    {
        std::ostringstream error;
        error << "Unknown transform type for creation: "
              << typeid(transform).name();

        throw Exception(error.str().c_str());
    }
}

std::ostream& operator<< (std::ostream & os, const Transform & transform)
{
    const Transform* t = &transform;

    if(const AllocationTransform * allocationTransform = \
        dynamic_cast<const AllocationTransform*>(t))
    {
        os << *allocationTransform;
    }
    else if(const BuiltinTransform * builtInTransform = \
        dynamic_cast<const BuiltinTransform*>(t))
    {
        os << *builtInTransform;
    }
    else if(const CDLTransform * cdlTransform = \
        dynamic_cast<const CDLTransform*>(t))
    {
        os << *cdlTransform;
    }
    else if(const ColorSpaceTransform * colorSpaceTransform = \
        dynamic_cast<const ColorSpaceTransform*>(t))
    {
        os << *colorSpaceTransform;
    }
    else if(const DisplayViewTransform * displayViewTransform = \
        dynamic_cast<const DisplayViewTransform*>(t))
    {
        os << *displayViewTransform;
    }
    else if(const ExponentTransform * exponentTransform = \
        dynamic_cast<const ExponentTransform*>(t))
    {
        os << *exponentTransform;
    }
    else if (const ExponentWithLinearTransform * exponentLinearTransform = \
        dynamic_cast<const ExponentWithLinearTransform*>(t))
    {
        os << *exponentLinearTransform;
    }
    else if (const ExposureContrastTransform * ecTransform = \
        dynamic_cast<const ExposureContrastTransform*>(t))
    {
        os << *ecTransform;
    }
    else if(const FileTransform * fileTransform = \
        dynamic_cast<const FileTransform*>(t))
    {
        os << *fileTransform;
    }
    else if(const FixedFunctionTransform * fixedFunctionTransform = \
        dynamic_cast<const FixedFunctionTransform*>(t))
    {
        os << *fixedFunctionTransform;
    }
    else if (const GradingPrimaryTransform * gradingPrimaryTransform = \
        dynamic_cast<const GradingPrimaryTransform*>(t))
    {
        os << *gradingPrimaryTransform;
    }
    else if (const GradingRGBCurveTransform * gradingRGBCurveTransform = \
        dynamic_cast<const GradingRGBCurveTransform*>(t))
    {
        os << *gradingRGBCurveTransform;
    }
    else if (const GradingToneTransform * gradingToneTransform = \
        dynamic_cast<const GradingToneTransform*>(t))
    {
        os << *gradingToneTransform;
    }
    else if(const GroupTransform * groupTransform = \
        dynamic_cast<const GroupTransform*>(t))
    {
        os << *groupTransform;
    }
    else if (const LogAffineTransform * logAffineTransform = \
        dynamic_cast<const LogAffineTransform*>(t))
    {
        os << *logAffineTransform;
    }
    else if (const LogCameraTransform * logCamTransform = \
        dynamic_cast<const LogCameraTransform*>(t))
    {
        os << *logCamTransform;
    }
    else if (const LogTransform * logTransform = \
        dynamic_cast<const LogTransform*>(t))
    {
        os << *logTransform;
    }
    else if(const LookTransform * lookTransform = \
        dynamic_cast<const LookTransform*>(t))
    {
        os << *lookTransform;
    }
    else if (const Lut1DTransform * lut1dTransform = \
        dynamic_cast<const Lut1DTransform*>(t))
    {
        os << *lut1dTransform;
    }
    else if (const Lut3DTransform * lut3dTransform = \
        dynamic_cast<const Lut3DTransform*>(t))
    {
        os << *lut3dTransform;
    }
    else if(const MatrixTransform * matrixTransform = \
        dynamic_cast<const MatrixTransform*>(t))
    {
        os << *matrixTransform;
    }
    else if(const RangeTransform * rangeTransform = \
        dynamic_cast<const RangeTransform*>(t))
    {
        os << *rangeTransform;
    }
    else
    {
        std::ostringstream error;
        error << "Unknown transform type for serialization: "
                << typeid(transform).name();

        throw Exception(error.str().c_str());

    }

    return os;
}


void CreateTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    // AllocationNoOp, FileNoOp, LookNoOp won't create a Transform.
    if (!op || op->isNoOpType())
        return;

    auto data = op->data();

    if (DynamicPtrCast<const CDLOpData>(data))
    {
        CreateCDLTransform(group, op);
    }
    else if (DynamicPtrCast<const ExponentOpData>(data))
    {
        CreateExponentTransform(group, op);
    }
    else if (DynamicPtrCast<const ExposureContrastOpData>(data))
    {
        CreateExposureContrastTransform(group, op);
    }
    else if (DynamicPtrCast<const FixedFunctionOpData>(data))
    {
        CreateFixedFunctionTransform(group, op);
    }
    else if (DynamicPtrCast<const GammaOpData>(data))
    {
        CreateGammaTransform(group, op);
    }
    else if (DynamicPtrCast<const GradingPrimaryOpData>(data))
    {
        CreateGradingPrimaryTransform(group, op);
    }
    else if (DynamicPtrCast<const GradingRGBCurveOpData>(data))
    {
        CreateGradingRGBCurveTransform(group, op);
    }
    else if (DynamicPtrCast<const GradingToneOpData>(data))
    {
        CreateGradingToneTransform(group, op);
    }
    else if (DynamicPtrCast<const LogOpData>(data))
    {
        CreateLogTransform(group, op);
    }
    else if (DynamicPtrCast<const Lut1DOpData>(data))
    {
        CreateLut1DTransform(group, op);
    }
    else if (DynamicPtrCast<const Lut3DOpData>(data))
    {
        CreateLut3DTransform(group, op);
    }
    else if (DynamicPtrCast<const MatrixOpData>(data))
    {
        CreateMatrixTransform(group, op);
    }
    else if (DynamicPtrCast<const RangeOpData>(data))
    {
        CreateRangeTransform(group, op);
    }
    else
    {
        std::ostringstream error;
        error << "CreateTransform from op. Missing implementation for: "
                <<  typeid(op).name();

        throw Exception(error.str().c_str());
    }
}

} // namespace OCIO_NAMESPACE
