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
#include <iterator>

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
    
    class DisplayTransform::Impl
    {
    public:
        TransformDirection dir_;
        std::string inputColorSpaceName_;
        TransformRcPtr linearCC_;
        TransformRcPtr colorTimingCC_;
        TransformRcPtr channelView_;
        std::string displayColorSpaceName_;
        TransformRcPtr displayCC_;
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD)
        { }
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            dir_ = rhs.dir_;
            inputColorSpaceName_ = rhs.inputColorSpaceName_;
            
            linearCC_ = rhs.linearCC_;
            if(linearCC_) linearCC_ = linearCC_->createEditableCopy();
            
            colorTimingCC_ = rhs.colorTimingCC_;
            if(colorTimingCC_) colorTimingCC_ = colorTimingCC_->createEditableCopy();
            
            channelView_ = rhs.channelView_;
            if(channelView_) channelView_ = channelView_->createEditableCopy();
            
            displayCC_ = rhs.displayCC_;
            if(displayCC_) displayCC_ = displayCC_->createEditableCopy();
            
            displayColorSpaceName_ = rhs.displayColorSpaceName_;
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
    
    void DisplayTransform::setInputColorSpaceName(const char * name)
    {
        m_impl->inputColorSpaceName_ = name;
    }
    
    const char * DisplayTransform::getInputColorSpaceName() const
    {
        return m_impl->inputColorSpaceName_.c_str();
    }
    
    void DisplayTransform::setLinearCC(const ConstTransformRcPtr & cc)
    {
        m_impl->linearCC_ = cc->createEditableCopy();
    }
    
    ConstTransformRcPtr DisplayTransform::getLinearCC() const
    {
        return m_impl->linearCC_;
    }
    
    void DisplayTransform::setColorTimingCC(const ConstTransformRcPtr & cc)
    {
        m_impl->colorTimingCC_ = cc->createEditableCopy();
    }
    
    ConstTransformRcPtr DisplayTransform::getColorTimingCC() const
    {
        return m_impl->colorTimingCC_;
    }
    
    void DisplayTransform::setChannelView(const ConstTransformRcPtr & transform)
    {
        m_impl->channelView_ = transform->createEditableCopy();
    }
    
    ConstTransformRcPtr DisplayTransform::getChannelView() const
    {
        return m_impl->channelView_;
    }
    
    void DisplayTransform::setDisplayColorSpaceName(const char * name)
    {
        m_impl->displayColorSpaceName_ = name;
    }
    
    const char * DisplayTransform::getDisplayColorSpaceName() const
    {
        return m_impl->displayColorSpaceName_.c_str();
    }
    
    
    void DisplayTransform::setDisplayCC(const ConstTransformRcPtr & cc)
    {
        m_impl->displayCC_ = cc->createEditableCopy();
    }
    
    ConstTransformRcPtr DisplayTransform::getDisplayCC() const
    {
        return m_impl->displayCC_;
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
        
        std::string inputColorSpaceName = displayTransform.getInputColorSpaceName();
        ConstColorSpaceRcPtr inputColorSpace = config.getColorSpace(inputColorSpaceName.c_str());
        if(!inputColorSpace)
        {
            std::ostringstream os;
            os << "DisplayTransform error.";
            if(inputColorSpaceName.empty()) os << " InputColorSpaceName is unspecified.";
            else os <<  " Cannot find inputColorSpace, named '" << inputColorSpaceName << "'.";
            throw Exception(os.str().c_str());
        }
        
        std::string displayColorSpaceName = displayTransform.getDisplayColorSpaceName();
        ConstColorSpaceRcPtr displayColorspace = config.getColorSpace(displayColorSpaceName.c_str());
        if(!displayColorspace)
        {
            std::ostringstream os;
            os << "DisplayTransform error.";
            if(displayColorSpaceName.empty()) os << " displayColorspace is unspecified.";
            else os <<  " Cannot find displayColorspace, named '" << displayColorSpaceName << "'.";
            throw Exception(os.str().c_str());
        }
        
        bool skipColorSpaceConversions = (inputColorSpace->isData() || displayColorspace->isData());
        
        // If we're viewing alpha, also skip all color space conversions.
        // TODO: Should we enforce the use of a MatrixTransform at the API level?
        ConstMatrixTransformRcPtr typedChannelView = DynamicPtrCast<const MatrixTransform>(
            displayTransform.getChannelView());
        if(typedChannelView)
        {
            float matrix44[16];
            typedChannelView->getValue(matrix44, 0x0);
            
            if((matrix44[3]>0.0f) || (matrix44[7]>0.0f) || (matrix44[11]>0.0f))
            {
                skipColorSpaceConversions = true;
            }
        }
        
        
        
        ConstColorSpaceRcPtr currentColorspace = inputColorSpace;
        
        
        
        // Apply a transform in ROLE_SCENE_LINEAR
        ConstTransformRcPtr linearCC = displayTransform.getLinearCC();
        if(linearCC)
        {
            // Put the new ops into a temp array, to see if it's a no-op
            // If it is a no-op, dont bother doing the colorspace conversion.
            OpRcPtrVec tmpOps;
            BuildOps(tmpOps, config, linearCC, TRANSFORM_DIR_FORWARD);
            
            if(!IsOpVecNoOp(tmpOps))
            {
                ConstColorSpaceRcPtr targetColorSpace = config.getColorSpace(ROLE_SCENE_LINEAR);
                
                if(!skipColorSpaceConversions)
                {
                    BuildColorSpaceOps(ops, config,
                                       currentColorspace,
                                       targetColorSpace);
                    currentColorspace = targetColorSpace;
                }
                
                std::copy(tmpOps.begin(), tmpOps.end(), std::back_inserter(ops));
            }
        }
        
        
        // Apply a color correction, in ROLE_COLOR_TIMING
        ConstTransformRcPtr colorTimingCC = displayTransform.getColorTimingCC();
        if(colorTimingCC)
        {
            // Put the new ops into a temp array, to see if it's a no-op
            // If it is a no-op, dont bother doing the colorspace conversion.
            OpRcPtrVec tmpOps;
            BuildOps(tmpOps, config, colorTimingCC, TRANSFORM_DIR_FORWARD);
            
            if(!IsOpVecNoOp(tmpOps))
            {
                ConstColorSpaceRcPtr targetColorSpace = config.getColorSpace(ROLE_COLOR_TIMING);
                
                if(!skipColorSpaceConversions)
                {
                    BuildColorSpaceOps(ops, config,
                                       currentColorspace,
                                       targetColorSpace);
                    currentColorspace = targetColorSpace;
                }
                
                std::copy(tmpOps.begin(), tmpOps.end(), std::back_inserter(ops));
            }
        }
        
        // Apply a channel view
        ConstTransformRcPtr channelView = displayTransform.getChannelView();
        if(channelView)
        {
            BuildOps(ops, config, channelView, TRANSFORM_DIR_FORWARD);
        }
        
        
        // Apply the conversion to the display color space
        if(!skipColorSpaceConversions)
        {
            BuildColorSpaceOps(ops, config,
                               currentColorspace,
                               displayColorspace);
            currentColorspace = displayColorspace;
        }
        
        
        // Apply a display cc
        ConstTransformRcPtr displayCC = displayTransform.getDisplayCC();
        if(displayCC)
        {
            BuildOps(ops, config, displayCC, TRANSFORM_DIR_FORWARD);
        }
        
    }
}
OCIO_NAMESPACE_EXIT
