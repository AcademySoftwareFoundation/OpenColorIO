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

#include <cstdlib>
#include <sstream>

#include "ImagePacking.h"

OCIO_NAMESPACE_ENTER
{
    
    std::ostream& operator<< (std::ostream& os, const ImageDesc& img)
    {
        if(const PackedImageDesc * packedImg = dynamic_cast<const PackedImageDesc*>(&img))
        {
            os << "<PackedImageDesc ";
            os << "data=" << packedImg->getData() << ", ";
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
    
    
    
    GenericImageDesc::GenericImageDesc():
        width(0),
        height(0),
        xStrideBytes(0),
        yStrideBytes(0),
        rData(NULL),
        gData(NULL),
        bData(NULL),
        aData(NULL)
    { };
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    GenericImageDesc::~GenericImageDesc()
    { };
    
    void GenericImageDesc::init(const ImageDesc& img)
    {
        if(const PackedImageDesc * packedImg = dynamic_cast<const PackedImageDesc*>(&img))
        {
            width = packedImg->getWidth();
            height = packedImg->getHeight();
            long numChannels = packedImg->getNumChannels();
            
            ptrdiff_t chanStrideBytes = packedImg->getChanStrideBytes();
            xStrideBytes = packedImg->getXStrideBytes();
            yStrideBytes = packedImg->getYStrideBytes();
            
            // AutoStrides will already be resolved by here, in the constructor of the ImageDesc
            if(chanStrideBytes == AutoStride ||
                xStrideBytes == AutoStride ||
                yStrideBytes == AutoStride)
            {
                throw Exception("Malformed PackedImageDesc: Unresolved AutoStride.");
            }
            
            rData = packedImg->getData();
            gData = reinterpret_cast<float*>( reinterpret_cast<char*>(rData)
                + chanStrideBytes );
            bData = reinterpret_cast<float*>( reinterpret_cast<char*>(rData)
                + 2*chanStrideBytes );
            if(numChannels >= 4)
            {
                aData = reinterpret_cast<float*>( reinterpret_cast<char*>(rData)
                  + 3*chanStrideBytes );
            }
            
            if(rData == NULL)
            {
                std::ostringstream os;
                os << "PackedImageDesc Error: A null image ptr was specified.";
                throw Exception(os.str().c_str());
            }
            
            if(width <= 0 || height <= 0)
            {
                std::ostringstream os;
                os << "PackedImageDesc Error: Image dimensions must be positive for both x,y. '";
                os << width << "x" << height << "' is not allowed.";
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
            width = planarImg->getWidth();
            height = planarImg->getHeight();
            xStrideBytes = sizeof(float);
            yStrideBytes = planarImg->getYStrideBytes();
            
            // AutoStrides will already be resolved by here, in the constructor of the ImageDesc
            if(yStrideBytes == AutoStride)
            {
                throw Exception("Malformed PlanarImageDesc: Unresolved AutoStride.");
            }
            
            rData = planarImg->getRData();
            gData = planarImg->getGData();
            bData = planarImg->getBData();
            aData = planarImg->getAData();
            
            if(width <= 0 || height <= 0)
            {
                std::ostringstream os;
                os << "PlanarImageDesc Error: Image dimensions must be positive for both x,y. '";
                os << width << "x" << height << "' is not allowed.";
                throw Exception(os.str().c_str());
            }
            
            if(rData == NULL || gData == NULL || bData == NULL)
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
    
    bool GenericImageDesc::isPackedRGBA() const
    {
        char* rPtr = reinterpret_cast<char*>(rData);
        char* gPtr = reinterpret_cast<char*>(gData);
        char* bPtr = reinterpret_cast<char*>(bData);
        char* aPtr = reinterpret_cast<char*>(aData);
        
        if(gPtr-rPtr != sizeof(float)) return false;
        if(bPtr-gPtr != sizeof(float)) return false;
        if(aPtr && (aPtr-bPtr != sizeof(float))) return false;
        
        // Confirm xStrideBytes is a pure float-sized packing
        // (I.e., it will divide evenly)
        if(xStrideBytes <= 0) return false;
        div_t result = div((int) xStrideBytes, (int)sizeof(float));
        if(result.rem != 0) return false;
        
        int implicitChannels = result.quot;
        if(implicitChannels != 4) return false;
        
        return true;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    class PackedImageDesc::Impl
    {
    public:
        float * data_;
        long width_;
        long height_;
        long numChannels_;
        ptrdiff_t chanStrideBytes_;
        ptrdiff_t xStrideBytes_;
        ptrdiff_t yStrideBytes_;
        
        Impl() :
            data_(NULL),
            width_(0),
            height_(0),
            numChannels_(0),
            chanStrideBytes_(0),
            xStrideBytes_(0),
            yStrideBytes_(0)
        {
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
        : m_impl(new PackedImageDesc::Impl)
    {
        getImpl()->data_ = data;
        getImpl()->width_ = width;
        getImpl()->height_ = height;
        getImpl()->numChannels_ = numChannels;
        getImpl()->chanStrideBytes_ = (chanStrideBytes == AutoStride)
            ? sizeof(float) : chanStrideBytes;
        getImpl()->xStrideBytes_ = (xStrideBytes == AutoStride)
            ? sizeof(float)*numChannels : xStrideBytes;
        getImpl()->yStrideBytes_ = (yStrideBytes == AutoStride)
            ? sizeof(float)*width*numChannels : yStrideBytes;
    }
    
    PackedImageDesc::~PackedImageDesc()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    float * PackedImageDesc::getData() const
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
    
    
    
    class PlanarImageDesc::Impl
    {
    public:
        float * rData_;
        float * gData_;
        float * bData_;
        float * aData_;
        long width_;
        long height_;
        ptrdiff_t yStrideBytes_;
        
        Impl() :
            rData_(NULL),
            gData_(NULL),
            bData_(NULL),
            aData_(NULL),
            width_(0),
            height_(0),
            yStrideBytes_(0)
        { }
        
        ~Impl()
        { }
    };
    
    
    PlanarImageDesc::PlanarImageDesc(float * rData, float * gData, float * bData, float * aData,
                                     long width, long height,
                                     ptrdiff_t yStrideBytes)
        : m_impl(new PlanarImageDesc::Impl())
    {
        getImpl()->rData_ = rData;
        getImpl()->gData_ = gData;
        getImpl()->bData_ = bData;
        getImpl()->aData_ = aData;
        getImpl()->width_ = width;
        getImpl()->height_ = height;
        getImpl()->yStrideBytes_ = (yStrideBytes == AutoStride)
            ? sizeof(float)*width : yStrideBytes;
    }
    
    PlanarImageDesc::~PlanarImageDesc()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    float* PlanarImageDesc::getRData() const
    {
        return getImpl()->rData_;
    }
    
    float* PlanarImageDesc::getGData() const
    {
        return getImpl()->gData_;
    }
    
    float* PlanarImageDesc::getBData() const
    {
        return getImpl()->bData_;
    }
    
    float* PlanarImageDesc::getAData() const
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
    
    ptrdiff_t PlanarImageDesc::getYStrideBytes() const
    {
        return getImpl()->yStrideBytes_;
    }
    
}
OCIO_NAMESPACE_EXIT
