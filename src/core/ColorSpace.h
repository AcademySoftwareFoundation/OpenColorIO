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


#ifndef INCLUDED_OCIO_COLORSPACE_H
#define INCLUDED_OCIO_COLORSPACE_H

#include <OpenColorIO/OpenColorIO.h>

OCIO_NAMESPACE_ENTER
{
    class ColorSpace::Impl
    {
        public:
        Impl();
        ~Impl();
        
        Impl& operator= (const Impl &);
        
        /*
        Impl(const Impl &);
        */
        
        bool equals(const Impl &) const;
        
        const char * getName() const;
        void setName(const char * name);
        
        const char * getFamily() const;
        void setFamily(const char * family);
        
        BitDepth getBitDepth() const;
        void setBitDepth(BitDepth bitDepth);
        
        bool isData() const;
        void setIsData(bool isData);
        
        GpuAllocation getGPUAllocation() const;
        void setGPUAllocation(GpuAllocation allocation);
        
        float getGPUMin() const;
        void setGPUMin(float min);
        
        float getGPUMax() const;
        void setGPUMax(float max);
        
        ConstGroupTransformRcPtr getTransform(ColorSpaceDirection dir) const;
        GroupTransformRcPtr getEditableTransform(ColorSpaceDirection dir);
        
        void setTransform(const ConstGroupTransformRcPtr & groupTransform,
                          ColorSpaceDirection dir);
        
        bool isTransformSpecified(ColorSpaceDirection dir) const;
        
        private:
        
        std::string m_name;
        std::string m_family;
        
        BitDepth m_bitDepth;
        bool m_isData;
        
        GpuAllocation m_gpuAllocation;
        float m_gpuMin;
        float m_gpuMax;
        
        GroupTransformRcPtr m_toRefTransform;
        GroupTransformRcPtr m_fromRefTransform;
        
        bool m_toRefSpecified;
        bool m_fromRefSpecified;
    };
}
OCIO_NAMESPACE_EXIT

#endif
