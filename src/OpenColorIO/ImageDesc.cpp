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

#include <cstdlib>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "ImagePacking.h"


OCIO_NAMESPACE_ENTER
{
    
    std::ostream& operator<< (std::ostream& os, const ImageDesc& img)
    {
        if(const PackedImageDesc * packedImg = dynamic_cast<const PackedImageDesc*>(&img))
        {
            os << "<PackedImageDesc ";
            os << "data=" << packedImg->getData() << ", ";
            os << "chanOrder=" << packedImg->getChannelOrder() << ", ";
            os << "width=" << packedImg->getWidth() << ", ";
            os << "height=" << packedImg->getHeight() << ", ";
            os << "numChannels=" << packedImg->getNumChannels() << ", ";
            os << "chanStrideBytes=" << packedImg->getChanStrideBytes() << ", ";
            os << "xStrideBytes=" << packedImg->getXStrideBytes() << ", ";
            os << "yStrideBytes=" << packedImg->getYStrideBytes() << "";
            os << ">";
        }
        else if(const PlanarImageDesc * planarImg = dynamic_cast<const PlanarImageDesc*>(&img))
        {
            os << "<PlanarImageDesc ";
            os << "rData=" << planarImg->getRData() << ", ";
            os << "gData=" << planarImg->getGData() << ", ";
            os << "bData=" << planarImg->getBData() << ", ";
            os << "aData=" << planarImg->getAData() << ", ";
            os << "width=" << packedImg->getWidth() << ", ";
            os << "height=" << packedImg->getHeight() << ", ";
            os << "xStrideBytes=" << planarImg->getXStrideBytes() << ", ";
            os << "yStrideBytes=" << planarImg->getYStrideBytes() << "";
            os << ">";
        }
        else
        {
            os << "<UnknownImageDesc>";
        }
        
        return os;
    }
    
    ImageDesc::~ImageDesc()
    {
    
    }

    void GenericImageDesc::init(const ImageDesc & img, BitDepth bitDepth, const ConstOpCPURcPtr & bitDepthOp)
    {
        m_packedFloatRGBA = false;
        m_bitDepth = bitDepth;
        m_bitDepthOp = bitDepthOp;

        if(const PackedImageDesc * packedImg = dynamic_cast<const PackedImageDesc*>(&img))
        {
            m_width = packedImg->getWidth();
            m_height = packedImg->getHeight();
            const long numChannels = packedImg->getNumChannels();

            m_packedFloatRGBA = packedImg->getChannelOrder()==CHANNEL_ORDERING_RGBA
                && bitDepth==BIT_DEPTH_F32;
            
            m_chanStrideBytes = packedImg->getChanStrideBytes();
            m_xStrideBytes    = packedImg->getXStrideBytes();
            m_yStrideBytes    = packedImg->getYStrideBytes();
            
            // AutoStrides will already be resolved by here, in the constructor of the ImageDesc
            if(m_chanStrideBytes == AutoStride ||
                m_xStrideBytes == AutoStride ||
                m_yStrideBytes == AutoStride)
            {
                throw Exception("Malformed PackedImageDesc: Unresolved AutoStride.");
            }

            if(packedImg->getChannelOrder()==CHANNEL_ORDERING_RGBA
                || packedImg->getChannelOrder()==CHANNEL_ORDERING_RGB)
            {
                if(packedImg->getChannelOrder()==CHANNEL_ORDERING_RGBA && numChannels!=4)
                {
                    throw Exception("Malformed PackedImageDesc: Wrong number of color channels.");
                }
                else if(packedImg->getChannelOrder()==CHANNEL_ORDERING_RGB && numChannels!=3)
                {
                    throw Exception("Malformed PackedImageDesc: Wrong number of color channels.");
                }

                m_rData = reinterpret_cast<void *>( packedImg->getData() );
                m_gData = reinterpret_cast<void *>( reinterpret_cast<char *>(m_rData)
                    + m_chanStrideBytes );
                m_bData = reinterpret_cast<void *>( reinterpret_cast<char *>(m_rData)
                    + 2 * m_chanStrideBytes );
                if(packedImg->getChannelOrder()==CHANNEL_ORDERING_RGBA)
                {
                    m_aData = reinterpret_cast<void *>( reinterpret_cast<char *>(m_rData)
                        + 3 * m_chanStrideBytes );
                }
                else
                {
                    m_aData = nullptr;
                }
            }
            else if(packedImg->getChannelOrder()==CHANNEL_ORDERING_BGRA
                || packedImg->getChannelOrder()==CHANNEL_ORDERING_BGR)
            {
                if(packedImg->getChannelOrder()==CHANNEL_ORDERING_BGRA && numChannels!=4)
                {
                    throw Exception("Malformed PackedImageDesc: Wrong number of color channels.");
                }
                else if(packedImg->getChannelOrder()==CHANNEL_ORDERING_BGR && numChannels!=3)
                {
                    throw Exception("Malformed PackedImageDesc: Wrong number of color channels.");
                }

                m_bData = reinterpret_cast<void *>( packedImg->getData() );
                m_gData = reinterpret_cast<void *>( reinterpret_cast<char *>(m_bData)
                    + m_chanStrideBytes );
                m_rData = reinterpret_cast<void *>( reinterpret_cast<char *>(m_bData)
                    + 2 * m_chanStrideBytes );
                if(packedImg->getChannelOrder()==CHANNEL_ORDERING_BGRA)
                {
                    m_aData = reinterpret_cast<void *>( reinterpret_cast<char *>(m_bData)
                        + 3 * m_chanStrideBytes );
                }
                else
                {
                    m_aData = nullptr;
                }
            }
            else if(packedImg->getChannelOrder()==CHANNEL_ORDERING_ABGR)
            {
                if(numChannels!=4)
                {
                    throw Exception("Malformed PackedImageDesc: Missing color channels.");
                }

                m_aData = reinterpret_cast<void *>( packedImg->getData() );
                m_bData = reinterpret_cast<void *>( reinterpret_cast<char *>(m_aData)
                    + m_chanStrideBytes );
                m_gData = reinterpret_cast<void *>( reinterpret_cast<char *>(m_aData)
                    + 2 * m_chanStrideBytes );
                m_rData = reinterpret_cast<void *>( reinterpret_cast<char *>(m_aData)
                    + 3 * m_chanStrideBytes );
            }
            else
            {
                throw Exception("PackedImageDesc Error: Unknown channel ordering.");
            }

            if(m_rData == nullptr || m_gData == nullptr || m_bData == nullptr)
            {
                throw Exception("PackedImageDesc Error: A null image ptr was specified.");
            }
            
            if(m_width <= 0 || m_height <= 0)
            {
                std::ostringstream os;
                os << "PackedImageDesc Error: Image dimensions must be positive for both x,y. '";
                os << m_width << "x" << m_height << "' is not allowed.";
                throw Exception(os.str().c_str());
            }
            
            if(numChannels < 3)
            {
                std::ostringstream os;
                os << "PackedImageDesc Error: Image numChannels must be three (or more) (rgb+). '";
                os << numChannels << "' is not allowed.";
                throw Exception(os.str().c_str());
            }
        }
        else if(const PlanarImageDesc * planarImg = dynamic_cast<const PlanarImageDesc*>(&img))
        {
            m_width = planarImg->getWidth();
            m_height = planarImg->getHeight();

            m_chanStrideBytes = planarImg->getXStrideBytes();
            m_xStrideBytes    = planarImg->getXStrideBytes();
            m_yStrideBytes    = planarImg->getYStrideBytes();
            
            // AutoStrides will already be resolved by here, in the constructor of the ImageDesc
            if(m_yStrideBytes == AutoStride)
            {
                throw Exception("Malformed PlanarImageDesc: Unresolved AutoStride.");
            }
            
            m_rData = reinterpret_cast<void *>(planarImg->getRData());
            m_gData = reinterpret_cast<void *>(planarImg->getGData());
            m_bData = reinterpret_cast<void *>(planarImg->getBData());
            m_aData = reinterpret_cast<void *>(planarImg->getAData());
            
            if(m_width <= 0 || m_height <= 0)
            {
                std::ostringstream os;
                os << "PlanarImageDesc Error: Image dimensions must be positive for both x,y. '";
                os << m_width << "x" << m_height << "' is not allowed.";
                throw Exception(os.str().c_str());
            }
            
            if(m_rData == nullptr || m_gData == nullptr || m_bData == nullptr)
            {
                std::ostringstream os;
                os << "PlanarImageDesc Error: Valid ptrs must be passed for all 3 image rgb color channels.";
                throw Exception(os.str().c_str());
            }
        }
        else
        {
            throw Exception("Unknown ImageDesc type.");
        }
    }
    
    bool GenericImageDesc::isPackedFloatRGBA() const
    {
        if(m_chanStrideBytes!=sizeof(float)) return false;

        char* rPtr = reinterpret_cast<char*>(m_rData);
        char* gPtr = reinterpret_cast<char*>(m_gData);
        char* bPtr = reinterpret_cast<char*>(m_bData);
        char* aPtr = reinterpret_cast<char*>(m_aData);

        if(gPtr-rPtr != sizeof(float)) return false;
        if(bPtr-gPtr != sizeof(float)) return false;
        if(aPtr && (aPtr-bPtr != sizeof(float))) return false;

        // Confirm xStrideBytes is a pure float-sized packing
        // (I.e., it will divide evenly)
        if(m_xStrideBytes <= 0) return false;
        div_t result = div((int) m_xStrideBytes, (int)sizeof(float));
        if(result.rem != 0) return false;

        int implicitChannels = result.quot;
        if(implicitChannels != 4) return false;

        return m_packedFloatRGBA;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    struct PackedImageDesc::Impl
    {
        void * data_ = nullptr;

        ChannelOrdering chanOrder_ = CHANNEL_ORDERING_RGBA;

        long width_ = 0;
        long height_ = 0;
        long numChannels_ = 0;

        // Byte numbers computed from the bit depth.
        ptrdiff_t chanStrideBytes_ = 0;
        ptrdiff_t xStrideBytes_ = 0;
        ptrdiff_t yStrideBytes_ = 0;
    };
    
    PackedImageDesc::PackedImageDesc(void * data,
                                     long width, long height,
                                     long numChannels)
        :   ImageDesc()
        ,   m_impl(new PackedImageDesc::Impl)
    {
		getImpl()->data_        = data;
		getImpl()->width_       = width;
        getImpl()->height_      = height;
        getImpl()->numChannels_ = numChannels;

        if(numChannels==4)
        {
            getImpl()->chanOrder_ = CHANNEL_ORDERING_RGBA;
        }
        else if(numChannels==3)
        {
            getImpl()->chanOrder_ = CHANNEL_ORDERING_RGB;
        }
        else
        {
            throw Exception("Invalid number of channels.");
        }

        const unsigned oneChannelInBytes = sizeof(BitDepthInfo<BIT_DEPTH_F32>::Type);

        getImpl()->chanStrideBytes_ = oneChannelInBytes;
        getImpl()->xStrideBytes_    = getImpl()->chanStrideBytes_ * getImpl()->numChannels_;
        getImpl()->yStrideBytes_    = getImpl()->xStrideBytes_* width;
    }

    PackedImageDesc::PackedImageDesc(void * data,
                                     long width, long height,
                                     ChannelOrdering chanOrder)
        :   ImageDesc()
        ,   m_impl(new PackedImageDesc::Impl)
    {
        getImpl()->data_      = data;
        getImpl()->chanOrder_ = chanOrder;
        getImpl()->width_     = width;
        getImpl()->height_    = height;

        if(chanOrder==CHANNEL_ORDERING_RGBA 
            || chanOrder==CHANNEL_ORDERING_BGRA
            || chanOrder==CHANNEL_ORDERING_ABGR)
        {
            getImpl()->numChannels_ = 4;
        }
        else if(chanOrder==CHANNEL_ORDERING_RGB 
            || chanOrder==CHANNEL_ORDERING_BGR)
        {
            getImpl()->numChannels_ = 3;
        }
        else
        {
            throw Exception("Unknown channel ordering.");
        }

        const unsigned oneChannelInBytes = sizeof(BitDepthInfo<BIT_DEPTH_F32>::Type);

        getImpl()->chanStrideBytes_ = oneChannelInBytes;
        getImpl()->xStrideBytes_    = getImpl()->chanStrideBytes_ * getImpl()->numChannels_;
        getImpl()->yStrideBytes_    = getImpl()->xStrideBytes_ * width;
    }

    PackedImageDesc::PackedImageDesc(void * data,
                                     long width, long height,
                                     ChannelOrdering chanOrder,
                                     ptrdiff_t chanStrideBytes,
                                     ptrdiff_t xStrideBytes,
                                     ptrdiff_t yStrideBytes)
        :   ImageDesc()
        ,   m_impl(new PackedImageDesc::Impl)
    {
        getImpl()->data_      = data;
        getImpl()->chanOrder_ = chanOrder;
        getImpl()->width_     = width;
        getImpl()->height_    = height;

        if(chanOrder==CHANNEL_ORDERING_RGBA 
            || chanOrder==CHANNEL_ORDERING_BGRA
            || chanOrder==CHANNEL_ORDERING_ABGR)
        {
            getImpl()->numChannels_ = 4;
        }
        else if(chanOrder==CHANNEL_ORDERING_RGB 
            || chanOrder==CHANNEL_ORDERING_BGR)
        {
            getImpl()->numChannels_ = 3;
        }
        else
        {
            throw Exception("Unknown channel ordering.");
        }

        const unsigned oneChannelInBytes = sizeof(BitDepthInfo<BIT_DEPTH_F32>::Type);

        getImpl()->chanStrideBytes_ = (chanStrideBytes == AutoStride)
            ? oneChannelInBytes : chanStrideBytes;
        getImpl()->xStrideBytes_ = (xStrideBytes == AutoStride)
            ? getImpl()->chanStrideBytes_ * getImpl()->numChannels_ : xStrideBytes;
        getImpl()->yStrideBytes_ = (yStrideBytes == AutoStride)
            ? getImpl()->xStrideBytes_ * width : yStrideBytes;

        if((getImpl()->chanStrideBytes_ * getImpl()->numChannels_) > getImpl()->xStrideBytes_)
        {
            throw Exception("The channel and x strides are inconsistent.");
        }
        if((getImpl()->xStrideBytes_ * width) > getImpl()->yStrideBytes_)
        {
            throw Exception("The x and y strides are inconsistent.");
        }
    }

    PackedImageDesc::PackedImageDesc(void * data,
                                     long width, long height,
                                     long numChannels,
                                     ptrdiff_t chanStrideBytes,
                                     ptrdiff_t xStrideBytes,
                                     ptrdiff_t yStrideBytes)
        :   ImageDesc()
        ,   m_impl(new PackedImageDesc::Impl)
    {
        getImpl()->data_        = data;
        getImpl()->width_       = width;
        getImpl()->height_      = height;
        getImpl()->numChannels_ = numChannels;


        if(numChannels==4)
        {
            getImpl()->chanOrder_ = CHANNEL_ORDERING_RGBA;
        }
        else if(numChannels==3)
        {
            getImpl()->chanOrder_ = CHANNEL_ORDERING_RGB;
        }
        else
        {
            throw Exception("Invalid number of channels.");
        }

        const unsigned oneChannelInBytes = sizeof(BitDepthInfo<BIT_DEPTH_F32>::Type);

        getImpl()->chanStrideBytes_ = (chanStrideBytes == AutoStride)
            ? oneChannelInBytes : chanStrideBytes;
        getImpl()->xStrideBytes_ = (xStrideBytes == AutoStride)
            ? getImpl()->chanStrideBytes_ * getImpl()->numChannels_ : xStrideBytes;
        getImpl()->yStrideBytes_ = (yStrideBytes == AutoStride)
            ? getImpl()->xStrideBytes_ * width : yStrideBytes;

        if((getImpl()->chanStrideBytes_ * getImpl()->numChannels_) > getImpl()->xStrideBytes_)
        {
            throw Exception("The channel and x strides are inconsistent.");
        }
        if((getImpl()->xStrideBytes_ * width) > getImpl()->yStrideBytes_)
        {
            throw Exception("The x and y strides are inconsistent.");
        }
    }

    PackedImageDesc::~PackedImageDesc()
    {
        delete m_impl;
        m_impl = nullptr;
    }

    ChannelOrdering PackedImageDesc::getChannelOrder() const
    {
        return getImpl()->chanOrder_;
    }

    void * PackedImageDesc::getData() const
    {
        return getImpl()->data_;
    }
    
    long PackedImageDesc::getWidth() const
    {
        return getImpl()->width_;
    }
    
    long PackedImageDesc::getHeight() const
    {
        return getImpl()->height_;
    }
    
    long PackedImageDesc::getNumChannels() const
    {
        return getImpl()->numChannels_;
    }
    
    ptrdiff_t PackedImageDesc::getChanStrideBytes() const
    {
        return getImpl()->chanStrideBytes_;
    }
    
    ptrdiff_t PackedImageDesc::getXStrideBytes() const
    {
        return getImpl()->xStrideBytes_;
    }
    
    ptrdiff_t PackedImageDesc::getYStrideBytes() const
    {
        return getImpl()->yStrideBytes_;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    struct PlanarImageDesc::Impl
    {
        void * rData_ = nullptr;
        void * gData_ = nullptr;
        void * bData_ = nullptr;
        void * aData_ = nullptr;

        long width_ = 0;
        long height_ = 0;

        ptrdiff_t xStrideBytes_ = 0;
        ptrdiff_t yStrideBytes_ = 0;
    };
    
    
    PlanarImageDesc::PlanarImageDesc(void * rData, void * gData, void * bData, void * aData,
                                     long width, long height)
        :   ImageDesc()
        ,   m_impl(new PlanarImageDesc::Impl())
    {
        getImpl()->rData_ = rData;
        getImpl()->gData_ = gData;
        getImpl()->bData_ = bData;
        getImpl()->aData_ = aData;
        getImpl()->width_ = width;
        getImpl()->height_ = height;

        getImpl()->xStrideBytes_ = sizeof(BitDepthInfo<BIT_DEPTH_F32>::Type);
        getImpl()->yStrideBytes_ = getImpl()->xStrideBytes_ * width;
    }
    
    PlanarImageDesc::PlanarImageDesc(void * rData, void * gData, void * bData, void * aData,
                                     long width, long height,
                                     ptrdiff_t xStrideBytes,
                                     ptrdiff_t yStrideBytes)
        :   ImageDesc()
        ,   m_impl(new PlanarImageDesc::Impl())
    {
        getImpl()->rData_ = rData;
        getImpl()->gData_ = gData;
        getImpl()->bData_ = bData;
        getImpl()->aData_ = aData;
        getImpl()->width_ = width;
        getImpl()->height_ = height;

        getImpl()->xStrideBytes_ = (xStrideBytes == AutoStride)
            ? sizeof(BitDepthInfo<BIT_DEPTH_F32>::Type) : xStrideBytes;

        getImpl()->yStrideBytes_ = (yStrideBytes == AutoStride)
            ? getImpl()->xStrideBytes_ * width : yStrideBytes;

        if((getImpl()->xStrideBytes_ * width) > getImpl()->yStrideBytes_)
        {
            throw Exception("The x and y strides are inconsistent.");
        }
    }
    
    PlanarImageDesc::~PlanarImageDesc()
    {
        delete m_impl;
        m_impl = nullptr;
    }

    void * PlanarImageDesc::getRData() const
    {
        return getImpl()->rData_;
    }
    
    void * PlanarImageDesc::getGData() const
    {
        return getImpl()->gData_;
    }
    
    void * PlanarImageDesc::getBData() const
    {
        return getImpl()->bData_;
    }
    
    void * PlanarImageDesc::getAData() const
    {
        return getImpl()->aData_;
    }
    
    long PlanarImageDesc::getWidth() const
    {
        return getImpl()->width_;
    }
    
    long PlanarImageDesc::getHeight() const
    {
        return getImpl()->height_;
    }
    
    ptrdiff_t PlanarImageDesc::getXStrideBytes() const
    {
        return getImpl()->xStrideBytes_;
    }
    
    ptrdiff_t PlanarImageDesc::getYStrideBytes() const
    {
        return getImpl()->yStrideBytes_;
    }
    
}
OCIO_NAMESPACE_EXIT
