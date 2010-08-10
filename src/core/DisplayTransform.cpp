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

#include "DisplayTransform.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

#include <cmath>

OCIO_NAMESPACE_ENTER
{
    DisplayTransformRcPtr DisplayTransform::Create()
    {
        return DisplayTransformRcPtr(new DisplayTransform(), &deleter);
    }
    
    void DisplayTransform::deleter(DisplayTransform* t)
    {
        delete t;
    }
    
    
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
        return m_impl->getDirection();
    }
    
    void DisplayTransform::setDirection(TransformDirection dir)
    {
        m_impl->setDirection(dir);
    }
    
    
    
    void DisplayTransform::setInputColorspace(const ConstColorSpaceRcPtr & cs)
    {
        m_impl->setInputColorspace(cs);
    }
    
    void DisplayTransform::setLinearExposure(const float* v4)
    {
        m_impl->setLinearExposure(v4);
    }
    
    void DisplayTransform::setDisplayColorspace(const ConstColorSpaceRcPtr & cs)
    {
        m_impl->setDisplayColorspace(cs);
    }
    
    
    
    std::ostream& operator<< (std::ostream& os, const DisplayTransform& t)
    {
        os << "<DisplayTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << ">\n";
        return os;
    }
    
    
    
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    DisplayTransform::Impl::Impl() :
        m_direction(TRANSFORM_DIR_FORWARD)
    {
    }
    
    DisplayTransform::Impl::~Impl()
    {
    }
    
    DisplayTransform::Impl& DisplayTransform::Impl::operator= (const Impl & rhs)
    {
        m_direction = rhs.m_direction;
        m_inputColorSpace = rhs.m_inputColorSpace;
        m_linearCC = DynamicPtrCast<CDLTransform>(rhs.m_linearCC->createEditableCopy());
        m_displayColorSpace = rhs.m_displayColorSpace;
        return *this;
    }
    
    TransformDirection DisplayTransform::Impl::getDirection() const
    {
        return m_direction;
    }
    
    void DisplayTransform::Impl::setDirection(TransformDirection dir)
    {
        m_direction = dir;
    }
    
    void DisplayTransform::Impl::setInputColorspace(const ConstColorSpaceRcPtr & cs)
    {
        m_inputColorSpace = cs;
    }
    
    void DisplayTransform::Impl::setLinearExposure(const float* v4)
    {
        float cc[] = { powf(2.0, v4[0]),
                       powf(2.0, v4[1]),
                       powf(2.0, v4[2]) };
        m_linearCC->setSlope(cc);
    }
    
    void DisplayTransform::Impl::setDisplayColorspace(const ConstColorSpaceRcPtr & cs)
    {
        m_displayColorSpace = cs;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    void BuildDisplayOps(LocalProcessor & processor,
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
        
        /*
        ConstColorSpaceRcPtr currentColorspace = displayTransform.getInputColorspace();
        
        ConstCDLTransformRcPtr linearCC = displayTransform.getLinearCC();
        if(!linearCC.isNoOp())
        {
            ConstColorSpaceRcPtr linearColorSpace = ;
            
            BuildColorSpaceConversionOps(processor, config,
                                         currentColorspace,
                                         linearColorSpace);
            
            BuildCDLOps(processor, config,
                        *linearCC,
                        TRANSFORM_DIR_FORWARD);
            
            currentColorspace = linearCC;
        }
        
        
        BuildColorSpaceConversionOps(processor, config,
                                     currentColorspace,
                                     displayTransform.getDisplayColorSpace());
        */
    }
}
OCIO_NAMESPACE_EXIT
