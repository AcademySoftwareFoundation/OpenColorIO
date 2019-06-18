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

#include "ops/NoOp/NoOps.h"
#include "OpBuilders.h"


OCIO_NAMESPACE_ENTER
{
    ColorSpaceTransformRcPtr ColorSpaceTransform::Create()
    {
        return ColorSpaceTransformRcPtr(new ColorSpaceTransform(), &deleter);
    }
    
    void ColorSpaceTransform::deleter(ColorSpaceTransform* t)
    {
        delete t;
    }
    
    
    class ColorSpaceTransform::Impl
    {
    public:
        TransformDirection dir_;
        std::string src_;
        std::string dst_;
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD)
        { }

        Impl(const Impl &) = delete;

        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                dir_ = rhs.dir_;
                src_ = rhs.src_;
                dst_ = rhs.dst_;
            }
            return *this;
        }
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    ColorSpaceTransform::ColorSpaceTransform()
        : m_impl(new ColorSpaceTransform::Impl)
    {
    }
    
    TransformRcPtr ColorSpaceTransform::createEditableCopy() const
    {
        ColorSpaceTransformRcPtr transform = ColorSpaceTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    ColorSpaceTransform::~ColorSpaceTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    ColorSpaceTransform& ColorSpaceTransform::operator= (const ColorSpaceTransform & rhs)
    {
        if (this != &rhs)
        {
            *m_impl = *rhs.m_impl;
        }
        return *this;
    }
    
    TransformDirection ColorSpaceTransform::getDirection() const
    {
        return getImpl()->dir_;
    }
    
    void ColorSpaceTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }
    
    void ColorSpaceTransform::validate() const
    {
        Transform::validate();

        if (getImpl()->src_.empty())
        {
            throw Exception("ColorSpaceTransform: empty source color space name");
        }

        if (getImpl()->dst_.empty())
        {
            throw Exception("ColorSpaceTransform: empty destination color space name");
        }
    }

    const char * ColorSpaceTransform::getSrc() const
    {
        return getImpl()->src_.c_str();
    }
    
    void ColorSpaceTransform::setSrc(const char * src)
    {
        getImpl()->src_ = src;
    }
    
    const char * ColorSpaceTransform::getDst() const
    {
        return getImpl()->dst_.c_str();
    }
    
    void ColorSpaceTransform::setDst(const char * dst)
    {
        getImpl()->dst_ = dst;
    }
    
    std::ostream& operator<< (std::ostream& os, const ColorSpaceTransform& t)
    {
        os << "<ColorSpaceTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << "src=" << t.getSrc() << ", ";
        os << "dst=" << t.getDst();
        os << ">";
        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    
    void BuildColorSpaceOps(OpRcPtrVec & ops,
                            const Config& config,
                            const ConstContextRcPtr & context,
                            const ColorSpaceTransform & colorSpaceTransform,
                            TransformDirection dir)
    {
        TransformDirection combinedDir = CombineTransformDirections(dir,
                                                  colorSpaceTransform.getDirection());
        
        ConstColorSpaceRcPtr src, dst;
        
        if(combinedDir == TRANSFORM_DIR_FORWARD)
        {
            src = config.getColorSpace( context->resolveStringVar( colorSpaceTransform.getSrc() ) );
            dst = config.getColorSpace( context->resolveStringVar( colorSpaceTransform.getDst() ) );
        }
        else if(combinedDir == TRANSFORM_DIR_INVERSE)
        {
            dst = config.getColorSpace( context->resolveStringVar( colorSpaceTransform.getSrc() ) );
            src = config.getColorSpace( context->resolveStringVar( colorSpaceTransform.getDst() ) );
        }
        
        BuildColorSpaceOps(ops, config, context, src, dst);
    }
    
    namespace
    {
        bool AreColorSpacesInSameEqualityGroup(const ConstColorSpaceRcPtr & csa,
                                               const ConstColorSpaceRcPtr & csb)
        {
            std::string a = csa->getEqualityGroup();
            std::string b = csb->getEqualityGroup();
            
            if(!a.empty()) return (a==b);
            return false;
        }
    }
    
    void BuildColorSpaceOps(OpRcPtrVec & ops,
                            const Config & config,
                            const ConstContextRcPtr & context,
                            const ConstColorSpaceRcPtr & srcColorSpace,
                            const ConstColorSpaceRcPtr & dstColorSpace)
    {
        if(!srcColorSpace)
            throw Exception("BuildColorSpaceOps failed, null srcColorSpace.");
        if(!dstColorSpace)
            throw Exception("BuildColorSpaceOps failed, null dstColorSpace.");
        
        if(AreColorSpacesInSameEqualityGroup(srcColorSpace, dstColorSpace))
            return;
        if(dstColorSpace->isData() || srcColorSpace->isData())
            return;
        
        // Consider dt8 -> vd8?
        // One would have to explode the srcColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE);
        // result, and walk through it step by step.  If the dstColorspace family were
        // ever encountered in transit, we'd want to short circuit the result.
        
        AllocationData srcAllocation;
        srcAllocation.allocation = srcColorSpace->getAllocation();
        srcAllocation.vars.resize( srcColorSpace->getAllocationNumVars());
        if(srcAllocation.vars.size() > 0)
        {
            srcColorSpace->getAllocationVars(&srcAllocation.vars[0]);
        }
        
        CreateGpuAllocationNoOp(ops, srcAllocation);
        
        // Go to the reference space, either by using
        // * cs->ref in the forward direction
        // * ref->cs in the inverse direction
        if(srcColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE))
        {
            BuildOps(ops, config, context, srcColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE), TRANSFORM_DIR_FORWARD);
        }
        else if(srcColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE))
        {
            BuildOps(ops, config, context, srcColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE), TRANSFORM_DIR_INVERSE);
        }
        // Otherwise, both are not defined so its a no-op. This is not an error condition.
        
        // Go from the reference space, either by using
        // * ref->cs in the forward direction
        // * cs->ref in the inverse direction
        if(dstColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE))
        {
            BuildOps(ops, config, context, dstColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE), TRANSFORM_DIR_FORWARD);
        }
        else if(dstColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE))
        {
            BuildOps(ops, config, context, dstColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE), TRANSFORM_DIR_INVERSE);
        }
        // Otherwise, both are not defined so its a no-op. This is not an error condition.
        
        AllocationData dstAllocation;
        dstAllocation.allocation = dstColorSpace->getAllocation();
        dstAllocation.vars.resize( dstColorSpace->getAllocationNumVars());
        if(dstAllocation.vars.size() > 0)
        {
            dstColorSpace->getAllocationVars(&dstAllocation.vars[0]);
        }
        
        CreateGpuAllocationNoOp(ops, dstAllocation);
    }
}
OCIO_NAMESPACE_EXIT
