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
    
    
    class PackedImageDesc::Impl
    {
    public:
        float * data_;
        long width_;
        long height_;
        ptrdiff_t chanStrideBytes_;
        ptrdiff_t xStrideBytes_;
        ptrdiff_t yStrideBytes_;
        
        Impl() :
            data_(0x0),
            width_(0),
            height_(0),
            chanStrideBytes_(0),
            xStrideBytes_(0),
            yStrideBytes_(0)
        {
        }
        
        Impl(float * data,
             long width, long height,
             long numChannels,
             ptrdiff_t chanStrideBytes,
             ptrdiff_t xStrideBytes,
             ptrdiff_t yStrideBytes ) :
            data_(data),
            width_(width),
            height_(height),
            chanStrideBytes_(chanStrideBytes),
            xStrideBytes_(xStrideBytes),
            yStrideBytes_(yStrideBytes)
        {
            if(width <= 0 || height <= 0)
            {
                std::ostringstream os;
                os << "Error: Image dimensions must be positive for both x,y. '";
                os << width << "x" << height << "' is not allowed.";
                throw Exception(os.str().c_str());
            }
            
            if(numChannels < 3)
            {
                std::ostringstream os;
                os << "Error: Image numChannels must be three (or more) (rgb+). '";
                os << numChannels << "' is not allowed.";
                throw Exception(os.str().c_str());
            }
            
            if(chanStrideBytes_ == AutoStride)
                chanStrideBytes_ = sizeof(float);
            if(xStrideBytes_ == AutoStride)
                xStrideBytes_ = sizeof(float)*numChannels;
            if(yStrideBytes_ == AutoStride)
                yStrideBytes_ = sizeof(float)*width*numChannels;
        }
        
        ~Impl()
        { }
    };
    
    
    
    
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
        return m_impl->width_;
    }
    
    long PackedImageDesc::getHeight() const
    {
        return m_impl->height_;
    }
    
    ptrdiff_t PackedImageDesc::getXStrideBytes() const
    {
        return m_impl->xStrideBytes_;
    }
    
    ptrdiff_t PackedImageDesc::getYStrideBytes() const
    {
        return m_impl->yStrideBytes_;
    }
    
    float* PackedImageDesc::getRData() const
    {
        return m_impl->data_;
    }
    
    float* PackedImageDesc::getGData() const
    {
        return reinterpret_cast<float*>( reinterpret_cast<char*>(m_impl->data_) \
            + m_impl->chanStrideBytes_ );
    }
    
    float* PackedImageDesc::getBData() const
    {
        return reinterpret_cast<float*>( reinterpret_cast<char*>(m_impl->data_) \
            + 2*m_impl->chanStrideBytes_ );
    }
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    class PlanarImageDesc::Impl
    {
    public:
        float * rData_;
        float * gData_;
        float * bData_;
        
        long width_;
        long height_;
        ptrdiff_t yStrideBytes_;
        
        Impl() :
            rData_(0x0),
            gData_(0x0),
            bData_(0x0),
            width_(0),
            height_(0),
            yStrideBytes_(0)
        { }
        
        Impl(float * rData, float * gData, float * bData,
             long width, long height,
             ptrdiff_t yStrideBytes ) :
            rData_(rData),
            gData_(gData),
            bData_(bData),
            width_(width),
            height_(height),
            yStrideBytes_(yStrideBytes)
        {
            if(width <= 0 || height <= 0)
            {
                std::ostringstream os;
                os << "Error: Image dimensions must be positive for both x,y. '";
                os << width << "x" << height << "' is not allowed.";
                throw Exception(os.str().c_str());
            }
            
            if(rData == 0x0 || gData == 0x0 || bData == 0x0)
            {
                std::ostringstream os;
                os << "Error: Valid ptrs must be passed in for all 3 image channels.";
                throw Exception(os.str().c_str());
            }
            
            if(yStrideBytes_ == AutoStride)
                yStrideBytes_ = sizeof(float)*width;
        }
        
        ~Impl()
        { }
    };
    
    
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
        return m_impl->width_;
    }
    
    long PlanarImageDesc::getHeight() const
    {
        return m_impl->height_;
    }
    
    ptrdiff_t PlanarImageDesc::getXStrideBytes() const
    {
        return sizeof(float);
    }
    
    ptrdiff_t PlanarImageDesc::getYStrideBytes() const
    {
        return m_impl->yStrideBytes_;
    }
    
    float* PlanarImageDesc::getRData() const
    {
        return m_impl->rData_;
    }
    
    float* PlanarImageDesc::getGData() const
    {
        return m_impl->gData_;
    }
    
    float* PlanarImageDesc::getBData() const
    {
        return m_impl->bData_;
    }
}
OCIO_NAMESPACE_EXIT
