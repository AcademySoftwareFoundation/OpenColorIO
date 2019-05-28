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

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"
#include "Processor.h"

#include <sstream>
#include <typeinfo>

OCIO_NAMESPACE_ENTER
{
    Transform::~Transform()
    { }

    void Transform::validate() const
    { 
        if (getDirection() != TRANSFORM_DIR_FORWARD
            && getDirection() != TRANSFORM_DIR_INVERSE)
        {
            std::string err(typeid(*this).name());
            err += ": invalid direction";

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
            BuildAllocationOps(ops, config, *allocationTransform, dir);
        }
        else if(ConstCDLTransformRcPtr cdlTransform = \
            DynamicPtrCast<const CDLTransform>(transform))
        {
            BuildCDLOps(ops, config, *cdlTransform, dir);
        }
        else if(ConstColorSpaceTransformRcPtr colorSpaceTransform = \
            DynamicPtrCast<const ColorSpaceTransform>(transform))
        {
            BuildColorSpaceOps(ops, config, context, *colorSpaceTransform, dir);
        }
        else if(ConstDisplayTransformRcPtr displayTransform = \
            DynamicPtrCast<const DisplayTransform>(transform))
        {
            BuildDisplayOps(ops, config, context, *displayTransform, dir);
        }
        else if(ConstExponentTransformRcPtr exponentTransform = \
            DynamicPtrCast<const ExponentTransform>(transform))
        {
            BuildExponentOps(ops, config, *exponentTransform, dir);
        }
        else if(ConstExponentWithLinearTransformRcPtr expWithLinearTransform = \
            DynamicPtrCast<const ExponentWithLinearTransform>(transform))
        {
            BuildExponentWithLinearOps(ops, config, *expWithLinearTransform, dir);
        }
        else if (ConstExposureContrastTransformRcPtr ecTransform = \
            DynamicPtrCast<const ExposureContrastTransform>(transform))
        {
            BuildExposureContrastOps(ops, config, *ecTransform, dir);
        }
        else if(ConstFixedFunctionTransformRcPtr fixedFunctionTransform = \
            DynamicPtrCast<const FixedFunctionTransform>(transform))
        {
            BuildFixedFunctionOps(ops, config, context, *fixedFunctionTransform, dir);
        }
        else if(ConstFileTransformRcPtr fileTransform = \
            DynamicPtrCast<const FileTransform>(transform))
        {
            BuildFileTransformOps(ops, config, context, *fileTransform, dir);
        }
        else if(ConstGroupTransformRcPtr groupTransform = \
            DynamicPtrCast<const GroupTransform>(transform))
        {
            BuildGroupOps(ops, config, context, *groupTransform, dir);
        }
        else if(ConstLogAffineTransformRcPtr logAffineTransform = \
            DynamicPtrCast<const LogAffineTransform>(transform))
        {
            BuildLogOps(ops, config, *logAffineTransform, dir);
        }
        else if(ConstLogTransformRcPtr logTransform = \
            DynamicPtrCast<const LogTransform>(transform))
        {
            BuildLogOps(ops, config, *logTransform, dir);
        }
        else if(ConstLookTransformRcPtr lookTransform = \
            DynamicPtrCast<const LookTransform>(transform))
        {
            BuildLookOps(ops, config, context, *lookTransform, dir);
        }
        else if(ConstMatrixTransformRcPtr matrixTransform = \
            DynamicPtrCast<const MatrixTransform>(transform))
        {
            BuildMatrixOps(ops, config, *matrixTransform, dir);
        }
        else if(ConstRangeTransformRcPtr rangeTransform = \
            DynamicPtrCast<const RangeTransform>(transform))
        {
            BuildRangeOps(ops, config, *rangeTransform, dir);
        }
        else if(ConstTruelightTransformRcPtr truelightTransform = \
            DynamicPtrCast<const TruelightTransform>(transform))
        {
            BuildTruelightOps(ops, config, *truelightTransform, dir);
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
        else if(const DisplayTransform * displayTransform = \
            dynamic_cast<const DisplayTransform*>(t))
        {
            os << *displayTransform;
        }
        else if(const ExponentTransform * exponentTransform = \
            dynamic_cast<const ExponentTransform*>(t))
        {
            os << *exponentTransform;
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
        else if(const GroupTransform * groupTransform = \
            dynamic_cast<const GroupTransform*>(t))
        {
            os << *groupTransform;
        }
        else if(const LogTransform * logTransform = \
            dynamic_cast<const LogTransform*>(t))
        {
            os << *logTransform;
        }
        else if(const LookTransform * lookTransform = \
            dynamic_cast<const LookTransform*>(t))
        {
            os << *lookTransform;
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
        else if(const TruelightTransform * truelightTransform = \
            dynamic_cast<const TruelightTransform*>(t))
        {
            os << *truelightTransform;
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
}
OCIO_NAMESPACE_EXIT
