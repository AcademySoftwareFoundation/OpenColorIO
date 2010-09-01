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

#include <cmath>
#include <cstring>

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        const float LOG2INV = 1.0f / logf(2.0f);
        const float FLTMIN = std::numeric_limits<float>::min();
        inline float log2(float f) { return logf(std::max(f, FLTMIN)) * LOG2INV; }
    }
    
    
    DisplayTransformRcPtr DisplayTransform::Create()
    {
        return DisplayTransformRcPtr(new DisplayTransform(), &deleter);
    }
    
    void DisplayTransform::deleter(DisplayTransform* t)
    {
        delete t;
    }
    
    class DisplayTransform::Impl
    {
    public:
        TransformDirection dir_;
        ConstColorSpaceRcPtr inputColorSpace_;
        CDLTransformRcPtr linearCC_;
        ConstColorSpaceRcPtr displayColorSpace_;
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD),
            inputColorSpace_(ColorSpace::Create()),
            linearCC_(CDLTransform::Create()),
            displayColorSpace_(ColorSpace::Create())
        { }
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            dir_ = rhs.dir_;
            inputColorSpace_ = rhs.inputColorSpace_;
            linearCC_ = DynamicPtrCast<CDLTransform>(rhs.linearCC_->createEditableCopy());
            displayColorSpace_ = rhs.displayColorSpace_;
            return *this;
        }
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    DisplayTransform::DisplayTransform()
        : m_impl(new DisplayTransform::Impl)
    {
    }
    
    TransformRcPtr DisplayTransform::createEditableCopy() const
    {
        DisplayTransformRcPtr transform = DisplayTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    DisplayTransform::~DisplayTransform()
    {
    }
    
    DisplayTransform& DisplayTransform::operator= (const DisplayTransform & rhs)
    {
        *m_impl = *rhs.m_impl;
        return *this;
    }
    
    TransformDirection DisplayTransform::getDirection() const
    {
        return m_impl->dir_;
    }
    
    void DisplayTransform::setDirection(TransformDirection dir)
    {
        m_impl->dir_ = dir;
    }
    
    void DisplayTransform::setInputColorSpace(const ConstColorSpaceRcPtr & cs)
    {
        if(!cs)
        {
            throw Exception("DisplayTransform::SetInputColorSpace failed. Colorspace is null.");
        }
        
        m_impl->inputColorSpace_ = cs;
    }
    
    ConstColorSpaceRcPtr DisplayTransform::getInputColorSpace() const
    {
        return m_impl->inputColorSpace_;
    }
    
    void DisplayTransform::setLinearCC(const ConstCDLTransformRcPtr & cc)
    {
        m_impl->linearCC_ = DynamicPtrCast<CDLTransform>(cc->createEditableCopy());
    }
    
    ConstCDLTransformRcPtr DisplayTransform::getLinearCC() const
    {
        return m_impl->linearCC_;
    }
    
    void DisplayTransform::setLinearExposure(const float* v4)
    {
        float cc[] = { powf(2.0, v4[0]),
                       powf(2.0, v4[1]),
                       powf(2.0, v4[2]) };
        m_impl->linearCC_->setSlope(cc);
    }
    
    void DisplayTransform::getLinearExposure(float* v4) const
    {
        float cc[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        m_impl->linearCC_->getSlope(cc);
        
        for(int i=0; i<2; i++)
        {
            cc[i] = log2(cc[i]);
        }
        
        memcpy(v4, cc, 4*sizeof(float));
    }
    
    void DisplayTransform::setDisplayColorSpace(const ConstColorSpaceRcPtr & cs)
    {
        if(!cs)
        {
            throw Exception("DisplayTransform::SetDisplayColorSpace failed. Colorspace is null.");
        }
        
        m_impl->displayColorSpace_ = cs;
    }
    
    ConstColorSpaceRcPtr DisplayTransform::getDisplayColorSpace() const
    {
        return m_impl->displayColorSpace_;
    }
    
    
    
    std::ostream& operator<< (std::ostream& os, const DisplayTransform& t)
    {
        os << "<DisplayTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << ">\n";
        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    void BuildDisplayOps(OpRcPtrVec & ops,
                         const Config & config,
                         const DisplayTransform & displayTransform,
                         TransformDirection dir)
    {
        TransformDirection combinedDir = CombineTransformDirections(dir,
                                                  displayTransform.getDirection());
        
        if(combinedDir != TRANSFORM_DIR_FORWARD)
        {
            std::ostringstream os;
            os << "DisplayTransform can only be applied in the forward direction.";
            throw Exception(os.str().c_str());
        }
        
        ConstColorSpaceRcPtr currentColorspace = displayTransform.getInputColorSpace();
        
        ConstCDLTransformRcPtr linearCC = displayTransform.getLinearCC();
        
        if(!linearCC->isNoOp())
        {
            ConstColorSpaceRcPtr linearColorSpace = config.getColorSpace(ROLE_SCENE_LINEAR);
            
            BuildColorSpaceOps(ops, config,
                               currentColorspace,
                               linearColorSpace);
            
            BuildCDLOps(ops, config,
                        *linearCC,
                        TRANSFORM_DIR_FORWARD);
            
            currentColorspace = linearColorSpace;
        }
        
        BuildColorSpaceOps(ops, config,
                           currentColorspace,
                           displayTransform.getDisplayColorSpace());
    }
}
OCIO_NAMESPACE_EXIT
