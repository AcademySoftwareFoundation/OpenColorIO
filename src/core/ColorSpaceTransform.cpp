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

#include "CDLTransform.h"
#include "ColorSpaceTransform.h"
#include "GpuAllocationOp.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

#include <cmath>

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
    }
    
    ColorSpaceTransform& ColorSpaceTransform::operator= (const ColorSpaceTransform & rhs)
    {
        *m_impl = *rhs.m_impl;
        return *this;
    }
    
    TransformDirection ColorSpaceTransform::getDirection() const
    {
        return m_impl->getDirection();
    }
    
    void ColorSpaceTransform::setDirection(TransformDirection dir)
    {
        m_impl->setDirection(dir);
    }
    
    
    const char * ColorSpaceTransform::getSrc() const
    {
        return m_impl->getSrc();
    }
    
    void ColorSpaceTransform::setSrc(const char * src)
    {
        m_impl->setSrc(src);
    }
    
    const char * ColorSpaceTransform::getDst() const
    {
        return m_impl->getDst();
    }
    
    void ColorSpaceTransform::setDst(const char * dst)
    {
        m_impl->setDst(dst);
    }
    
    std::ostream& operator<< (std::ostream& os, const ColorSpaceTransform& t)
    {
        os << "<ColorSpaceTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << ">\n";
        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    // TODO: Deal with null ColorSpace in a better manner. Assert during Build?
    
    ColorSpaceTransform::Impl::Impl() :
        m_direction(TRANSFORM_DIR_FORWARD)
    {
    }
    
    ColorSpaceTransform::Impl::~Impl()
    {
    }
    
    ColorSpaceTransform::Impl& ColorSpaceTransform::Impl::operator= (const Impl & rhs)
    {
        m_direction = rhs.m_direction;
        m_src = rhs.m_src;
        m_dst = rhs.m_dst;
        return *this;
    }
    
    TransformDirection ColorSpaceTransform::Impl::getDirection() const
    {
        return m_direction;
    }
    
    void ColorSpaceTransform::Impl::setDirection(TransformDirection dir)
    {
        m_direction = dir;
    }
    
    
    const char * ColorSpaceTransform::Impl::getSrc() const
    {
        return m_src.c_str();
    }
    
    void ColorSpaceTransform::Impl::setSrc(const char * src)
    {
        m_src = src;
    }
    
    const char * ColorSpaceTransform::Impl::getDst() const
    {
        return m_dst.c_str();
    }
    
    void ColorSpaceTransform::Impl::setDst(const char * dst)
    {
        m_dst = dst;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    
    void BuildColorSpaceOps(LocalProcessor & processor,
                            const Config& config,
                            const ColorSpaceTransform & colorSpaceTransform,
                            TransformDirection dir)
    {
        TransformDirection combinedDir = CombineTransformDirections(dir,
                                                  colorSpaceTransform.getDirection());
        
        ConstColorSpaceRcPtr src, dst;
        
        if(combinedDir == TRANSFORM_DIR_FORWARD)
        {
            src = config.getColorSpaceByName( colorSpaceTransform.getSrc() );
            dst = config.getColorSpaceByName( colorSpaceTransform.getDst() );
        }
        else if(combinedDir == TRANSFORM_DIR_INVERSE)
        {
            dst = config.getColorSpaceByName( colorSpaceTransform.getSrc() );
            src = config.getColorSpaceByName( colorSpaceTransform.getDst() );
        }
        
        BuildColorSpaceOps(processor, config, src, dst);
    }
    
    void BuildColorSpaceOps(LocalProcessor & processor,
                            const Config & config,
                            const ConstColorSpaceRcPtr & srcColorSpace,
                            const ConstColorSpaceRcPtr & dstColorSpace)
    {
        if(srcColorSpace->getFamily() == dstColorSpace->getFamily())
        {
            return;
        }
        
        if(srcColorSpace->isData() || dstColorSpace->isData())
        {
            return;
        }
        
        // Consider dt8 -> vd8?
        // One would have to explode the srcColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE);
        // result, and walk through it step by step.  If the dstColorspace family were
        // ever encountered in transit, we'd want to short circuit the result.
        
        CreateGpuAllocationOp(processor,
                              srcColorSpace->getGpuAllocation(),
                              srcColorSpace->getGpuMin(),
                              srcColorSpace->getGpuMax());
        
        ConstGroupTransformRcPtr toref = srcColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE);
        BuildOps(processor, config, toref, TRANSFORM_DIR_FORWARD);
        
        OCIO::ConstColorSpaceRcPtr referenceColorSpace = config.getColorSpaceForRole(ROLE_REFERENCE);
        CreateGpuAllocationOp(processor,
                              referenceColorSpace->getGpuAllocation(),
                              referenceColorSpace->getGpuMin(),
                              referenceColorSpace->getGpuMax());
        
        ConstGroupTransformRcPtr fromref = dstColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        BuildOps(processor, config, fromref, TRANSFORM_DIR_FORWARD);
        
        CreateGpuAllocationOp(processor,
                              dstColorSpace->getGpuAllocation(),
                              dstColorSpace->getGpuMin(),
                              dstColorSpace->getGpuMax());
    }
}
OCIO_NAMESPACE_EXIT
