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

#include <sstream>

OCIO_NAMESPACE_ENTER
{
    ColorSpaceRcPtr ColorSpace::Create()
    {
        return ColorSpaceRcPtr(new ColorSpace(), &deleter);
    }
    
    void ColorSpace::deleter(ColorSpace* c)
    {
        delete c;
    }
    
    
    class ColorSpace::Impl
    {
    public:
        std::string name_;
        std::string family_;
        std::string description_;
        
        BitDepth bitDepth_;
        bool isData_;
        
        GpuAllocation gpuAllocation_;
        float gpuMin_;
        float gpuMax_;
        
        TransformRcPtr toRefTransform_;
        TransformRcPtr fromRefTransform_;
        
        bool toRefSpecified_;
        bool fromRefSpecified_;
        
        Impl() :
            bitDepth_(BIT_DEPTH_UNKNOWN),
            isData_(false),
            gpuAllocation_(GPU_ALLOCATION_UNIFORM),
            gpuMin_(0.0),
            gpuMax_(1.0),
            toRefSpecified_(false),
            fromRefSpecified_(false)
        { }
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            name_ = rhs.name_;
            family_ = rhs.family_;
            description_ = rhs.description_;
            bitDepth_ = rhs.bitDepth_;
            isData_ = rhs.isData_;
            gpuAllocation_ = rhs.gpuAllocation_;
            gpuMin_ = rhs.gpuMin_;
            gpuMax_ = rhs.gpuMax_;
            
            toRefTransform_ = rhs.toRefTransform_;
            if(toRefTransform_) toRefTransform_ = toRefTransform_->createEditableCopy();
            
            fromRefTransform_ = rhs.fromRefTransform_;
            if(fromRefTransform_) fromRefTransform_ = fromRefTransform_->createEditableCopy();
            
            toRefSpecified_ = rhs.toRefSpecified_;
            fromRefSpecified_ = rhs.fromRefSpecified_;
            return *this;
        }
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    ColorSpace::ColorSpace()
    : m_impl(new ColorSpace::Impl)
    {
    }
    
    ColorSpace::~ColorSpace()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    ColorSpaceRcPtr ColorSpace::createEditableCopy() const
    {
        ColorSpaceRcPtr cs = ColorSpace::Create();
        *cs->m_impl = *m_impl;
        return cs;
    }
    
    const char * ColorSpace::getName() const
    {
        return m_impl->name_.c_str();
    }
    
    void ColorSpace::setName(const char * name)
    {
        m_impl->name_ = name;
    }
    const char * ColorSpace::getFamily() const
    {
        return m_impl->family_.c_str();
    }
    
    void ColorSpace::setFamily(const char * family)
    {
        m_impl->family_ = family;
    }
    
    const char * ColorSpace::getDescription() const
    {
        return m_impl->description_.c_str();
    }
    
    void ColorSpace::setDescription(const char * description)
    {
        m_impl->description_ = description;
    }
    
    BitDepth ColorSpace::getBitDepth() const
    {
        return m_impl->bitDepth_;
    }
    
    void ColorSpace::setBitDepth(BitDepth bitDepth)
    {
        m_impl->bitDepth_ = bitDepth;
    }
    
    bool ColorSpace::isData() const
    {
        return m_impl->isData_;
    }
    
    void ColorSpace::setIsData(bool val)
    {
        m_impl->isData_ = val;
    }
    
    GpuAllocation ColorSpace::getGpuAllocation() const
    {
        return m_impl->gpuAllocation_;
    }
    
    void ColorSpace::setGpuAllocation(GpuAllocation allocation)
    {
        m_impl->gpuAllocation_ = allocation;
    }
    
    float ColorSpace::getGpuMin() const
    {
        return m_impl->gpuMin_;
    }
    
    void ColorSpace::setGpuMin(float min)
    {
        m_impl->gpuMin_ = min;
    }
    
    float ColorSpace::getGpuMax() const
    {
        return m_impl->gpuMax_;
    }
    
    void ColorSpace::setGpuMax(float max)
    {
        m_impl->gpuMax_ = max;
    }
    
    ConstTransformRcPtr ColorSpace::getTransform(ColorSpaceDirection dir) const
    {
        if(dir == COLORSPACE_DIR_TO_REFERENCE)
            return m_impl->toRefTransform_;
        else if(dir == COLORSPACE_DIR_FROM_REFERENCE)
            return m_impl->fromRefTransform_;
        
        throw Exception("Unspecified ColorSpaceDirection");
    }
    
    TransformRcPtr ColorSpace::getEditableTransform(ColorSpaceDirection dir)
    {
        if(dir == COLORSPACE_DIR_TO_REFERENCE)
            return m_impl->toRefTransform_;
        else if(dir == COLORSPACE_DIR_FROM_REFERENCE)
            return m_impl->fromRefTransform_;
        
        throw Exception("Unspecified ColorSpaceDirection");
    }
    
    void ColorSpace::setTransform(const ConstTransformRcPtr & transform,
                                  ColorSpaceDirection dir)
    {
        TransformRcPtr * majorTransform;
        TransformRcPtr * minorTransform;
        bool * majorIsSpecified = 0;
        bool * minorIsSpecified = 0;
        
        if(dir == COLORSPACE_DIR_TO_REFERENCE)
        {
            majorTransform = &(m_impl->toRefTransform_);
            majorIsSpecified = &(m_impl->toRefSpecified_);
            
            minorTransform = &(m_impl->fromRefTransform_);
            minorIsSpecified = &(m_impl->fromRefSpecified_);
        }
        else if(dir == COLORSPACE_DIR_FROM_REFERENCE)
        {
            majorTransform = &(m_impl->fromRefTransform_);
            majorIsSpecified = &(m_impl->fromRefSpecified_);
            
            minorTransform = &(m_impl->toRefTransform_);
            minorIsSpecified = &(m_impl->toRefSpecified_);
        }
        else
        {
            throw Exception("Unspecified ColorSpaceDirection");
        }
        
        if(!transform)
        {
            *majorTransform = TransformRcPtr();
            *majorIsSpecified = false;
            if(!*minorIsSpecified) *minorTransform = TransformRcPtr();
        }
        else
        {
            *majorTransform = transform->createEditableCopy();
            *majorIsSpecified = true;
            
            if(!*minorIsSpecified)
            {
                *minorTransform = transform->createEditableCopy();
                (*minorTransform)->setDirection( GetInverseTransformDirection((*majorTransform)->getDirection()) );
            }
        }
    }
    
    bool ColorSpace::isTransformSpecified(ColorSpaceDirection dir) const
    {
        if(dir == COLORSPACE_DIR_TO_REFERENCE)
            return m_impl->toRefSpecified_;
        else if(COLORSPACE_DIR_FROM_REFERENCE)
            return m_impl->fromRefSpecified_;
        
        throw Exception("Unspecified ColorSpaceDirection");
    }
    
    std::ostream& operator<< (std::ostream& os, const ColorSpace& cs)
    {
        os << "<ColorSpace ";
        os << "name=" << cs.getName() << ", ";
        os << "family=" << cs.getFamily() << ", ";
        os << "bitDepth=" << BitDepthToString(cs.getBitDepth()) << ", ";
        os << "isData=" << BoolToString(cs.isData()) << ", ";
        os << "GpuAllocation=" << GpuAllocationToString(cs.getGpuAllocation()) << ", ";
        // TODO: make this not warn
        //os << "GpuMin=" << cs.getGpuMin() << ", ";
        //os << "GpuMax=" << cs.getGpuMax() << ", ";
        
        os << ">\n";
        if(cs.isTransformSpecified(COLORSPACE_DIR_TO_REFERENCE))
        {
            os << "\t" << cs.getName() << " --> Reference\n";
            os << cs.getTransform(COLORSPACE_DIR_TO_REFERENCE);
        }
        
        if(cs.isTransformSpecified(COLORSPACE_DIR_FROM_REFERENCE))
        {
            os << "\tReference --> " << cs.getName() << "\n";
            os << cs.getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        }
        return os;
    }
}
OCIO_NAMESPACE_EXIT
