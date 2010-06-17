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

#include <OpenColorSpace/OpenColorSpace.h>

#include "ImageDesc.h"

#include <sstream>

OCS_NAMESPACE_ENTER
{
    
    std::ostream& operator<< (std::ostream& os, const ImageDesc& img)
    {
        os << "<ImageDesc ";
        os << "width=" << img.getWidth() << ", ";
        os << "height=" << img.getHeight() << ", ";
        os << "xStrideBytes=" << img.getXStrideBytes() << ", ";
        os << "yStrideBytes=" << img.getYStrideBytes() << ", ";
        os << "rDataPtr=" << img.getRData() << ", ";
        os << "gDataPtr=" << img.getGData() << ", ";
        os << "bDataPtr=" << img.getBData() << "";
        os << ">";
        return os;
    }
    
    ImageDesc::~ImageDesc()
    {
    
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    PackedImageDesc::PackedImageDesc(float * data,
                                     long width, long height,
                                     long numChannels,
                                     ptrdiff_t chanStrideBytes,
                                     ptrdiff_t xStrideBytes,
                                     ptrdiff_t yStrideBytes)
        : m_impl(new PackedImageDesc::Impl(data,
                                           width, height, numChannels,
                                           chanStrideBytes,
                                           xStrideBytes, yStrideBytes))
    { }
    
    PackedImageDesc::~PackedImageDesc()
    { }
    
    long PackedImageDesc::getWidth() const
    {
        return m_impl->getWidth();
    }
    
    long PackedImageDesc::getHeight() const
    {
        return m_impl->getHeight();
    }

    ptrdiff_t PackedImageDesc::getXStrideBytes() const
    {
        return m_impl->getXStrideBytes();
    }
    
    ptrdiff_t PackedImageDesc::getYStrideBytes() const
    {
        return m_impl->getYStrideBytes();
    }
    
    float* PackedImageDesc::getRData() const
    {
        return m_impl->getRData();
    }
    
    float* PackedImageDesc::getGData() const
    {
        return m_impl->getGData();
    }
    
    float* PackedImageDesc::getBData() const
    {
        return m_impl->getBData();
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    
    PackedImageDesc::Impl::Impl(float * data,
                                long width, long height,
                                long numChannels,
                                ptrdiff_t chanStrideBytes,
                                ptrdiff_t xStrideBytes,
                                ptrdiff_t yStrideBytes ) :
        m_data(data),
        m_width(width),
        m_height(height),
        m_chanStrideBytes(chanStrideBytes),
        m_xStrideBytes(xStrideBytes),
        m_yStrideBytes(yStrideBytes)
    {
        if(width <= 0 || height <= 0)
        {
            std::ostringstream os;
            os << "Error: Image dimensions must be positive for both x,y. '";
            os << width << "x" << height << "' is not allowed.";
            throw OCSException(os.str().c_str());
        }
        
        if(numChannels < 3)
        {
            std::ostringstream os;
            os << "Error: Image numChannels must be three (or more) (rgb+). '";
            os << numChannels << "' is not allowed.";
            throw OCSException(os.str().c_str());
        }
        
        if(m_chanStrideBytes == AutoStride)
            m_chanStrideBytes = sizeof(float);
        if(m_xStrideBytes == AutoStride)
            m_xStrideBytes = sizeof(float)*numChannels;
        if(m_yStrideBytes == AutoStride)
            m_yStrideBytes = sizeof(float)*width*numChannels;
    }
    
    PackedImageDesc::Impl::~Impl()
    {
    
    }
    
    long PackedImageDesc::Impl::getWidth() const
    {
        return m_width;
    }
    
    long PackedImageDesc::Impl::getHeight() const
    {
        return m_height;
    }

    ptrdiff_t PackedImageDesc::Impl::getXStrideBytes() const
    {
        return m_xStrideBytes;
    }
    
    ptrdiff_t PackedImageDesc::Impl::getYStrideBytes() const
    {
        return m_yStrideBytes;
    }
    
    float* PackedImageDesc::Impl::getRData() const
    {
        return m_data;
    }
    
    float* PackedImageDesc::Impl::getGData() const
    {
        return reinterpret_cast<float*>( reinterpret_cast<char*>(m_data) \
            + m_chanStrideBytes );
    }
    
    float* PackedImageDesc::Impl::getBData() const
    {
        return reinterpret_cast<float*>( reinterpret_cast<char*>(m_data) \
            + 2*m_chanStrideBytes );
    }
    
    
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    PlanarImageDesc::PlanarImageDesc(float * rData, float * gData, float * bData,
                                     long width, long height,
                                     ptrdiff_t yStrideBytes)
        : m_impl(new PlanarImageDesc::Impl(rData, gData, bData,
                                           width, height,
                                           yStrideBytes))
    { }
    
    PlanarImageDesc::~PlanarImageDesc()
    { }
    
    long PlanarImageDesc::getWidth() const
    {
        return m_impl->getWidth();
    }
    
    long PlanarImageDesc::getHeight() const
    {
        return m_impl->getHeight();
    }

    ptrdiff_t PlanarImageDesc::getXStrideBytes() const
    {
        return m_impl->getXStrideBytes();
    }
    
    ptrdiff_t PlanarImageDesc::getYStrideBytes() const
    {
        return m_impl->getYStrideBytes();
    }
    
    float* PlanarImageDesc::getRData() const
    {
        return m_impl->getRData();
    }
    
    float* PlanarImageDesc::getGData() const
    {
        return m_impl->getGData();
    }
    
    float* PlanarImageDesc::getBData() const
    {
        return m_impl->getBData();
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    PlanarImageDesc::Impl::Impl(float * rData, float * gData, float * bData,
                                long width, long height,
                                ptrdiff_t yStrideBytes ) :
        m_rData(rData),
        m_gData(gData),
        m_bData(bData),
        m_width(width),
        m_height(height),
        m_yStrideBytes(yStrideBytes)
    {
        if(width <= 0 || height <= 0)
        {
            std::ostringstream os;
            os << "Error: Image dimensions must be positive for both x,y. '";
            os << width << "x" << height << "' is not allowed.";
            throw OCSException(os.str().c_str());
        }
        
        if(rData == 0x0 || gData == 0x0 || bData == 0x0)
        {
            std::ostringstream os;
            os << "Error: Valid ptrs must be passed in for all 3 image channels.";
            throw OCSException(os.str().c_str());
        }
        
        if(m_yStrideBytes == AutoStride)
            m_yStrideBytes = sizeof(float)*width;
    }
    
    PlanarImageDesc::Impl::~Impl()
    {
    
    }
    
    long PlanarImageDesc::Impl::getWidth() const
    {
        return m_width;
    }
    
    long PlanarImageDesc::Impl::getHeight() const
    {
        return m_height;
    }
    
    ptrdiff_t PlanarImageDesc::Impl::getXStrideBytes() const
    {
        return sizeof(float);
    }
    
    ptrdiff_t PlanarImageDesc::Impl::getYStrideBytes() const
    {
        return m_yStrideBytes;
    }
    
    float* PlanarImageDesc::Impl::getRData() const
    {
        return m_rData;
    }
    
    float* PlanarImageDesc::Impl::getGData() const
    {
        return m_gData;
    }
    
    float* PlanarImageDesc::Impl::getBData() const
    {
        return m_bData;
    }
}
OCS_NAMESPACE_EXIT
