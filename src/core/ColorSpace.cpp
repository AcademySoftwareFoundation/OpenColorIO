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
#include "ColorSpace.h"

#include <sstream>


#include <iostream>

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
    
    
    
    ColorSpace::ColorSpace()
    : m_impl(new ColorSpace::Impl)
    {
    }
    
    ColorSpace::~ColorSpace()
    {
    }
    
    ColorSpaceRcPtr ColorSpace::createEditableCopy() const
    {
        ColorSpaceRcPtr cs = ColorSpace::Create();
        *cs->m_impl = *m_impl;
        return cs;
    }
    
    bool ColorSpace::equals(const ConstColorSpaceRcPtr & other) const
    {
        return m_impl->equals(*(other->m_impl));
    }
    
    const char * ColorSpace::getName() const
    {
        return m_impl->getName();
    }
    
    void ColorSpace::setName(const char * name)
    {
        m_impl->setName(name);
    }
    const char * ColorSpace::getFamily() const
    {
        return m_impl->getFamily();
    }
    
    void ColorSpace::setFamily(const char * family)
    {
        m_impl->setFamily(family);
    }
    
    BitDepth ColorSpace::getBitDepth() const
    {
        return m_impl->getBitDepth();
    }
    
    void ColorSpace::setBitDepth(BitDepth bitDepth)
    {
        m_impl->setBitDepth(bitDepth);
    }
    
    bool ColorSpace::isData() const
    {
        return m_impl->isData();
    }
    
    void ColorSpace::setIsData(bool val)
    {
        m_impl->setIsData(val);
    }
    
    GpuAllocation ColorSpace::getGPUAllocation() const
    {
        return m_impl->getGPUAllocation();
    }
    
    void ColorSpace::setGPUAllocation(GpuAllocation allocation)
    {
        m_impl->setGPUAllocation(allocation);
    }
    
    float ColorSpace::getGPUMin() const
    {
        return m_impl->getGPUMin();
    }
    
    void ColorSpace::setGPUMin(float min)
    {
        m_impl->setGPUMin(min);
    }
    
    float ColorSpace::getGPUMax() const
    {
        return m_impl->getGPUMax();
    }
    
    void ColorSpace::setGPUMax(float max)
    {
        m_impl->setGPUMax(max);
    }
    
    ConstGroupTransformRcPtr ColorSpace::getTransform(ColorSpaceDirection dir) const
    {
        return m_impl->getTransform(dir);
    }
    
    GroupTransformRcPtr ColorSpace::getEditableTransform(ColorSpaceDirection dir)
    {
        return m_impl->getEditableTransform(dir);
    }
    
    void ColorSpace::setTransform(const ConstGroupTransformRcPtr & groupTransform,
                                  ColorSpaceDirection dir)
    {
        m_impl->setTransform(groupTransform, dir);
    }
    
    bool ColorSpace::isTransformSpecified(ColorSpaceDirection dir) const
    {
        return m_impl->isTransformSpecified(dir);
    }
    
    std::ostream& operator<< (std::ostream& os, const ColorSpace& cs)
    {
        os << "<ColorSpace ";
        os << "name=" << cs.getName() << ", ";
        os << "family=" << cs.getFamily() << ", ";
        os << "bitDepth=" << BitDepthToString(cs.getBitDepth()) << ", ";
        os << "isData=" << BoolToString(cs.isData()) << ", ";
        os << "GPUAllocation=" << GpuAllocationToString(cs.getGPUAllocation()) << ", ";
        // TODO: make this not warn
        //os << "GPUMin=" << cs.getGPUMin() << ", ";
        //os << "GPUMax=" << cs.getGPUMax() << ", ";
        
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
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    
    ColorSpace::Impl::Impl() :
        m_bitDepth(BIT_DEPTH_UNKNOWN),
        m_isData(false),
        m_gpuAllocation(GPU_ALLOCATION_UNIFORM),
        m_gpuMin(0.0),
        m_gpuMax(1.0),
        m_toRefTransform(GroupTransform::Create()),
        m_fromRefTransform(GroupTransform::Create()),
        m_toRefSpecified(false),
        m_fromRefSpecified(false)
    {
    }
    
    ColorSpace::Impl::~Impl()
    {
    
    }
    
    ColorSpace::Impl& ColorSpace::Impl::operator= (const ColorSpace::Impl & rhs)
    {
        m_name = rhs.m_name;
        m_family = rhs.m_family;
        m_bitDepth = rhs.m_bitDepth;
        m_isData = rhs.m_isData;
        m_gpuAllocation = rhs.m_gpuAllocation;
        m_gpuMin = rhs.m_gpuMin;
        m_gpuMax = rhs.m_gpuMax;
        m_toRefTransform = DynamicPtrCast<GroupTransform>(rhs.m_toRefTransform->createEditableCopy());
        m_fromRefTransform = DynamicPtrCast<GroupTransform>(rhs.m_fromRefTransform->createEditableCopy());
        m_toRefSpecified = rhs.m_toRefSpecified;
        m_fromRefSpecified = rhs.m_fromRefSpecified;
        return *this;
    }

    bool ColorSpace::Impl::equals(const ColorSpace::Impl & other) const
    {
        return m_name == other.m_name;
    }
    
    const char * ColorSpace::Impl::getName() const
    {
        return m_name.c_str();
    }
    
    void ColorSpace::Impl::setName(const char * name)
    {
        m_name = name;
    }

    const char * ColorSpace::Impl::getFamily() const
    {
        return m_family.c_str();
    }
    
    void ColorSpace::Impl::setFamily(const char * family)
    {
        m_family = family;
    }
    
    BitDepth ColorSpace::Impl::getBitDepth() const
    {
        return m_bitDepth;
    }
    
    void ColorSpace::Impl::setBitDepth(BitDepth bitDepth)
    {
        m_bitDepth = bitDepth;
    }

    bool ColorSpace::Impl::isData() const
    {
        return m_isData;
    }
    
    void ColorSpace::Impl::setIsData(bool val)
    {
        m_isData = val;
    }

    GpuAllocation ColorSpace::Impl::getGPUAllocation() const
    {
        return m_gpuAllocation;
    }
    
    void ColorSpace::Impl::setGPUAllocation(GpuAllocation allocation)
    {
        m_gpuAllocation = allocation;
    }

    float ColorSpace::Impl::getGPUMin() const
    {
        return m_gpuMin;
    }
    
    void ColorSpace::Impl::setGPUMin(float min)
    {
        m_gpuMin = min;
    }

    float ColorSpace::Impl::getGPUMax() const
    {
        return m_gpuMax;
    }
    
    void ColorSpace::Impl::setGPUMax(float max)
    {
        m_gpuMax = max;
    }
    
    ConstGroupTransformRcPtr ColorSpace::Impl::getTransform(ColorSpaceDirection dir) const
    {
        if(dir == COLORSPACE_DIR_TO_REFERENCE)
            return m_toRefTransform;
        else if(dir == COLORSPACE_DIR_FROM_REFERENCE)
            return m_fromRefTransform;
        
        throw OCIOException("Unspecified ColorSpaceDirection");
    }
    
    GroupTransformRcPtr ColorSpace::Impl::getEditableTransform(ColorSpaceDirection dir)
    {
        if(dir == COLORSPACE_DIR_TO_REFERENCE)
            return m_toRefTransform;
        else if(dir == COLORSPACE_DIR_FROM_REFERENCE)
            return m_fromRefTransform;
        
        throw OCIOException("Unspecified ColorSpaceDirection");
    }
    
    void ColorSpace::Impl::setTransform(const ConstGroupTransformRcPtr & groupTransform,
                          ColorSpaceDirection dir)
    {
        GroupTransformRcPtr * majorTransform;
        GroupTransformRcPtr * minorTransform;
        bool * majorIsSpecified = 0;
        bool * minorIsSpecified = 0;
        
        if(dir == COLORSPACE_DIR_TO_REFERENCE)
        {
            majorTransform = &m_toRefTransform;
            majorIsSpecified = &m_toRefSpecified;
            
            minorTransform = &m_fromRefTransform;
            minorIsSpecified = &m_fromRefSpecified;
        }
        else if(dir == COLORSPACE_DIR_FROM_REFERENCE)
        {
            majorTransform = &m_fromRefTransform;
            majorIsSpecified = &m_fromRefSpecified;
            
            minorTransform = &m_toRefTransform;
            minorIsSpecified = &m_toRefSpecified;
        }
        else
        {
            throw OCIOException("Unspecified ColorSpaceDirection");
        }
        
        *majorTransform = DynamicPtrCast<GroupTransform>(groupTransform->createEditableCopy());
        *majorIsSpecified = (!groupTransform->empty());
        
        if(!*minorIsSpecified)
        {
            *minorTransform = DynamicPtrCast<GroupTransform>(groupTransform->createEditableCopy());
            (*minorTransform)->setDirection( GetInverseTransformDirection((*majorTransform)->getDirection()) );
        }
    }
    
    bool ColorSpace::Impl::isTransformSpecified(ColorSpaceDirection dir) const
    {
        if(dir == COLORSPACE_DIR_TO_REFERENCE)
            return m_toRefSpecified;
        else if(COLORSPACE_DIR_FROM_REFERENCE)
            return m_fromRefSpecified;
        
        throw OCIOException("Unspecified ColorSpaceDirection");
    }
}
OCIO_NAMESPACE_EXIT
