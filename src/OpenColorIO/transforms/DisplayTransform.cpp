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
        std::string display_;
        std::string view_;
        TransformRcPtr displayCC_;
        
        std::string looksOverride_;
        bool looksOverrideEnabled_;
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD),
            looksOverrideEnabled_(false)
        { }

        Impl(const Impl &) = delete;

        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                dir_ = rhs.dir_;
                inputColorSpaceName_ = rhs.inputColorSpaceName_;

                linearCC_ = rhs.linearCC_?
                    rhs.linearCC_->createEditableCopy() : rhs.linearCC_;

                colorTimingCC_ = rhs.colorTimingCC_?
                    rhs.colorTimingCC_->createEditableCopy()
                    : rhs.colorTimingCC_;

                channelView_ = rhs.channelView_?
                    rhs.channelView_->createEditableCopy() : rhs.channelView_;

                display_ = rhs.display_;
                view_ = rhs.view_;

                displayCC_ = rhs.displayCC_?
                    rhs.displayCC_->createEditableCopy() : rhs.displayCC_;

                looksOverride_ = rhs.looksOverride_;
                looksOverrideEnabled_ = rhs.looksOverrideEnabled_;
            }
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
        delete m_impl;
        m_impl = NULL;
    }
    
    DisplayTransform& DisplayTransform::operator= (const DisplayTransform & rhs)
    {
        if (this != &rhs)
        {
            *m_impl = *rhs.m_impl;
        }
        return *this;
    }
    
    TransformDirection DisplayTransform::getDirection() const
    {
        return getImpl()->dir_;
    }
    
    void DisplayTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }
    
    void DisplayTransform::validate() const
    {
        Transform::validate();

        // TODO: Improve DisplayTransform::validate()
    }

    void DisplayTransform::setInputColorSpaceName(const char * name)
    {
        getImpl()->inputColorSpaceName_ = name;
    }
    
    const char * DisplayTransform::getInputColorSpaceName() const
    {
        return getImpl()->inputColorSpaceName_.c_str();
    }
    
    void DisplayTransform::setLinearCC(const ConstTransformRcPtr & cc)
    {
        getImpl()->linearCC_ = cc->createEditableCopy();
    }
    
    ConstTransformRcPtr DisplayTransform::getLinearCC() const
    {
        return getImpl()->linearCC_;
    }
    
    void DisplayTransform::setColorTimingCC(const ConstTransformRcPtr & cc)
    {
        getImpl()->colorTimingCC_ = cc->createEditableCopy();
    }
    
    ConstTransformRcPtr DisplayTransform::getColorTimingCC() const
    {
        return getImpl()->colorTimingCC_;
    }
    
    void DisplayTransform::setChannelView(const ConstTransformRcPtr & transform)
    {
        getImpl()->channelView_ = transform->createEditableCopy();
    }
    
    ConstTransformRcPtr DisplayTransform::getChannelView() const
    {
        return getImpl()->channelView_;
    }
    
    void DisplayTransform::setDisplay(const char * display)
    {
        getImpl()->display_ = display;
    }
    
    const char * DisplayTransform::getDisplay() const
    {
        return getImpl()->display_.c_str();
    }
    
    void DisplayTransform::setView(const char * view)
    {
        getImpl()->view_ = view;
    }
    
    const char * DisplayTransform::getView() const
    {
        return getImpl()->view_.c_str();
    }
    
    void DisplayTransform::setDisplayCC(const ConstTransformRcPtr & cc)
    {
        getImpl()->displayCC_ = cc->createEditableCopy();
    }
    
    ConstTransformRcPtr DisplayTransform::getDisplayCC() const
    {
        return getImpl()->displayCC_;
    }
    
    void DisplayTransform::setLooksOverride(const char * looks)
    {
        getImpl()->looksOverride_ = looks;
    }
    
    const char * DisplayTransform::getLooksOverride() const
    {
        return getImpl()->looksOverride_.c_str();
    }
    
    void DisplayTransform::setLooksOverrideEnabled(bool enabled)
    {
        getImpl()->looksOverrideEnabled_ = enabled;
    }
    
    bool DisplayTransform::getLooksOverrideEnabled() const
    {
        return getImpl()->looksOverrideEnabled_;
    }
    
    std::ostream& operator<< (std::ostream& os, const DisplayTransform& t)
    {
        os << "<DisplayTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << "inputColorSpace=" << t.getInputColorSpaceName() << ", ";
        os << "display=" << t.getDisplay() << ", ";
        os << "view=" << t.getView();
        if (t.getLooksOverrideEnabled())
        {
            os << ", looksOverride=" << t.getLooksOverride();
        }
        ConstTransformRcPtr transform(t.getLinearCC());
        if (transform)
        {
            os << ", linearCC: " << *transform;
        }
        transform = t.getColorTimingCC();
        if (transform)
        {
            os << ", colorTimingCC: " << *transform;
        }
        transform = t.getChannelView();
        if (transform)
        {
            os << ", channelView: " << *transform;
        }
        transform = t.getDisplayCC();
        if (transform)
        {
            os << ", displayCC: " << *transform;
        }
        os << ">";
        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    void BuildDisplayOps(OpRcPtrVec & ops,
                         const Config & config,
                         const ConstContextRcPtr & context,
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
        
        std::string display = displayTransform.getDisplay();
        std::string view = displayTransform.getView();
        
        std::string displayColorSpaceName = config.getDisplayColorSpaceName(display.c_str(), view.c_str());
        ConstColorSpaceRcPtr displayColorspace = config.getColorSpace(displayColorSpaceName.c_str());
        if(!displayColorspace)
        {
            std::ostringstream os;
            os << "DisplayTransform error.";
            os <<  " Cannot find display colorspace,  '" << displayColorSpaceName << "'.";
            throw Exception(os.str().c_str());
        }
        
        bool skipColorSpaceConversions = (inputColorSpace->isData() || displayColorspace->isData());
        
        // If we're viewing alpha, also skip all color space conversions.
        // If the user does uses a different transform for the channel view,
        // in place of a simple matrix, they run the risk that when viewing alpha
        // the colorspace transforms will not be skipped. (I.e., filmlook will be applied
        // to alpha.)  If this ever becomes an issue, additional engineering will be
        // added at that time.
        
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
        
        
        
        ConstColorSpaceRcPtr currentColorSpace = inputColorSpace;
        
        
        
        // Apply a transform in ROLE_SCENE_LINEAR
        ConstTransformRcPtr linearCC = displayTransform.getLinearCC();
        if(linearCC)
        {
            // Put the new ops into a temp array, to see if it's a no-op
            // If it is a no-op, dont bother doing the colorspace conversion.
            OpRcPtrVec tmpOps;
            BuildOps(tmpOps, config, context, linearCC, TRANSFORM_DIR_FORWARD);
            
            if(!IsOpVecNoOp(tmpOps))
            {
                ConstColorSpaceRcPtr targetColorSpace = config.getColorSpace(ROLE_SCENE_LINEAR);
                
                if(!skipColorSpaceConversions)
                {
                    BuildColorSpaceOps(ops, config, context,
                                       currentColorSpace,
                                       targetColorSpace);
                    currentColorSpace = targetColorSpace;
                }
                
                ops += tmpOps;
            }
        }
        
        
        // Apply a color correction, in ROLE_COLOR_TIMING
        ConstTransformRcPtr colorTimingCC = displayTransform.getColorTimingCC();
        if(colorTimingCC)
        {
            // Put the new ops into a temp array, to see if it's a no-op
            // If it is a no-op, dont bother doing the colorspace conversion.
            OpRcPtrVec tmpOps;
            BuildOps(tmpOps, config, context, colorTimingCC, TRANSFORM_DIR_FORWARD);
            
            if(!IsOpVecNoOp(tmpOps))
            {
                ConstColorSpaceRcPtr targetColorSpace = config.getColorSpace(ROLE_COLOR_TIMING);
                
                if(!skipColorSpaceConversions)
                {
                    BuildColorSpaceOps(ops, config, context,
                                       currentColorSpace,
                                       targetColorSpace);
                    currentColorSpace = targetColorSpace;
                }
                
                ops += tmpOps;
            }
        }
        
        // Apply a look, if specified
        LookParseResult looks;
        if(displayTransform.getLooksOverrideEnabled())
        {
            looks.parse(displayTransform.getLooksOverride());
        }
        else if(!skipColorSpaceConversions)
        {
            looks.parse(config.getDisplayLooks(display.c_str(), view.c_str()));
        }
        
        if(!looks.empty())
        {
            BuildLookOps(ops,
                         currentColorSpace,
                         skipColorSpaceConversions,
                         config,
                         context,
                         looks);
        }
        
        // Apply a channel view
        ConstTransformRcPtr channelView = displayTransform.getChannelView();
        if(channelView)
        {
            BuildOps(ops, config, context, channelView, TRANSFORM_DIR_FORWARD);
        }
        
        
        // Apply the conversion to the display color space
        if(!skipColorSpaceConversions)
        {
            BuildColorSpaceOps(ops, config, context,
                               currentColorSpace,
                               displayColorspace);
            currentColorSpace = displayColorspace;
        }
        
        
        // Apply a display cc
        ConstTransformRcPtr displayCC = displayTransform.getDisplayCC();
        if(displayCC)
        {
            BuildOps(ops, config, context, displayCC, TRANSFORM_DIR_FORWARD);
        }
        
    }
}
OCIO_NAMESPACE_EXIT
