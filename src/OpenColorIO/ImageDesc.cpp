// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdlib>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "ImagePacking.h"

namespace OCIO_NAMESPACE
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
        os << "width=" << planarImg->getWidth() << ", ";
        os << "height=" << planarImg->getHeight() << ", ";
        os << "xStrideBytes=" << planarImg->getXStrideBytes() << ", ";
        os << "yStrideBytes=" << planarImg->getYStrideBytes() << "";
        os << ">";
    }
    else
    {
        os << "<ImageDesc ";
        os << "rData=" << img.getRData() << ", ";
        os << "gData=" << img.getGData() << ", ";
        os << "bData=" << img.getBData() << ", ";
        os << "aData=" << img.getAData() << ", ";
        os << "width=" << img.getWidth() << ", ";
        os << "height=" << img.getHeight() << ", ";
        os << "xStrideBytes=" << img.getXStrideBytes() << ", ";
        os << "yStrideBytes=" << img.getYStrideBytes() << "";
        os << ">";
    }

    return os;
}

ImageDesc::ImageDesc()
{

}

ImageDesc::~ImageDesc()
{

}


///////////////////////////////////////////////////////////////////////////


void GenericImageDesc::init(const ImageDesc & img, BitDepth bitDepth, const ConstOpCPURcPtr & bitDepthOp)
{
    m_bitDepthOp = bitDepthOp;

    m_width  = img.getWidth();
    m_height = img.getHeight();

    m_xStrideBytes = img.getXStrideBytes();
    m_yStrideBytes = img.getYStrideBytes();

    m_rData = reinterpret_cast<char *>(img.getRData());
    m_gData = reinterpret_cast<char *>(img.getGData());
    m_bData = reinterpret_cast<char *>(img.getBData());
    m_aData = reinterpret_cast<char *>(img.getAData());

    m_isRGBAPacked = img.isRGBAPacked();
    m_isFloat      = img.isFloat();

    if(img.getBitDepth()!=bitDepth)
    {
        throw Exception("Bit-depth mismatch between the image buffer and the finalization setting.");
    }
}

bool GenericImageDesc::isPackedFloatRGBA() const
{
    return m_isFloat && m_isRGBAPacked;
}

bool GenericImageDesc::isRGBAPacked() const
{
    return m_isRGBAPacked;
}

bool GenericImageDesc::isFloat() const
{
    return m_isFloat;
}


///////////////////////////////////////////////////////////////////////////


struct PackedImageDesc::Impl
{
    void * m_data = nullptr;

    void * m_rData = nullptr;
    void * m_gData = nullptr;
    void * m_bData = nullptr;
    void * m_aData = nullptr;

    ChannelOrdering m_chanOrder = CHANNEL_ORDERING_RGBA;

    BitDepth m_bitDepth = BIT_DEPTH_UNKNOWN;

    long m_width = 0;
    long m_height = 0;
    long m_numChannels = 0;

    // Byte numbers computed from the bit depth.
    ptrdiff_t m_chanStrideBytes = 0;
    ptrdiff_t m_xStrideBytes = 0;
    ptrdiff_t m_yStrideBytes = 0;

    bool m_isRGBAPacked = false;
    bool m_isFloat = false;

    void initValues()
    {
        if(m_chanOrder==CHANNEL_ORDERING_RGBA
            || m_chanOrder==CHANNEL_ORDERING_RGB)
        {
            m_rData = m_data;
            m_gData = (char*)m_rData + m_chanStrideBytes;
            m_bData = (char*)m_rData + 2 * m_chanStrideBytes;
            if(m_numChannels==4)
            {
                m_aData = (char*)m_rData + 3 * m_chanStrideBytes;
            }
            else
            {
                m_aData = nullptr;
            }
        }
        else if(m_chanOrder==CHANNEL_ORDERING_BGRA
            || m_chanOrder==CHANNEL_ORDERING_BGR)
        {
            m_bData = m_data;
            m_gData = (char*)m_bData + m_chanStrideBytes;
            m_rData = (char*)m_bData + 2 * m_chanStrideBytes;
            if(m_numChannels==4)
            {
                m_aData = (char*)m_bData + 3 * m_chanStrideBytes;
            }
            else
            {
                m_aData = nullptr;
            }
        }
        else if(m_chanOrder==CHANNEL_ORDERING_ABGR)
        {
            m_aData = m_data;
            m_bData = (char*)m_aData + m_chanStrideBytes;
            m_gData = (char*)m_aData + 2 * m_chanStrideBytes;
            m_rData = (char*)m_aData + 3 * m_chanStrideBytes;
        }
        else
        {
            throw Exception("PackedImageDesc Error: Unknown channel ordering.");
        }
    }

    bool isRGBAPacked() const
    {
        if(m_aData==nullptr) return false;

        switch(m_bitDepth)
        {
            case BIT_DEPTH_UINT8:
            {
                if(m_chanStrideBytes!=sizeof(BitDepthInfo<BIT_DEPTH_UINT8>::Type))
                {
                    return false;
                }
                break;
            }
            case BIT_DEPTH_UINT10:
            {
                // Note that a 10-bit integer bit-depth value is stored in a uint16_t type.
                if(m_chanStrideBytes!=sizeof(BitDepthInfo<BIT_DEPTH_UINT10>::Type))
                {
                    return false;
                }
                break;
            }
            case BIT_DEPTH_UINT12:
            {
                // Note that a 12-bit integer bit-depth value is stored in a uint16_t type.
                if(m_chanStrideBytes!=sizeof(BitDepthInfo<BIT_DEPTH_UINT12>::Type))
                {
                    return false;
                }
                break;
            }
            case BIT_DEPTH_UINT16:
            {
                if(m_chanStrideBytes!=sizeof(BitDepthInfo<BIT_DEPTH_UINT16>::Type))
                {
                    return false;
                }
                break;
            }
            case BIT_DEPTH_F16:
            {
                // Note that a 16-bit float bit-depth value is stored in a half type.
                if(m_chanStrideBytes!=sizeof(BitDepthInfo<BIT_DEPTH_F16>::Type))
                {
                    return false;
                }
                break;
            }
            case BIT_DEPTH_F32:
            {
                if(m_chanStrideBytes!=sizeof(BitDepthInfo<BIT_DEPTH_F32>::Type))
                {
                    return false;
                }
                break;
            }
            case BIT_DEPTH_UINT14:
            case BIT_DEPTH_UINT32:
            case BIT_DEPTH_UNKNOWN:
            {
                std::string err("PackedImageDesc Error: Unsupported bit-depth: ");
                err += BitDepthToString(m_bitDepth);
                err += ".";
                throw Exception(err.c_str());
            }
        }

        if((char*)m_gData-(char*)m_rData != m_chanStrideBytes) return false;
        if((char*)m_bData-(char*)m_gData != m_chanStrideBytes) return false;
        if((char*)m_aData-(char*)m_bData != m_chanStrideBytes) return false;

        {
            // Confirm xStrideBytes is a pure packing
            // (I.e., it will divide evenly)
            const div_t result = div((int)m_xStrideBytes, (int)m_chanStrideBytes);
            if(result.rem != 0) return false;

            const int implicitChannels = result.quot;
            if(implicitChannels != 4) return false;
        }

        // Note: The optimization paths only process line by line
        //       so 'm_yStrideBytes' is not checked.

        return true;
    }

    bool isFloat() const
    {
        return m_chanStrideBytes==sizeof(float) && m_bitDepth==BIT_DEPTH_F32;
    }

    void validate() const
    {
        if(m_data==nullptr)
        {
            throw Exception("PackedImageDesc Error: Invalid image buffer.");
        }

        if(m_width<=0 || m_height<=0)
        {
            throw Exception("PackedImageDesc Error: Invalid image dimensions.");
        }

        if(m_chanStrideBytes<0 || m_chanStrideBytes==AutoStride)
        {
            throw Exception("PackedImageDesc Error: Invalid channel stride.");
        }

        if(m_numChannels<3 || m_numChannels>4)
        {
            throw Exception("PackedImageDesc Error: Invalid channel number.");
        }

        if((m_chanStrideBytes*m_numChannels)>m_xStrideBytes)
        {
            throw Exception("PackedImageDesc Error: The channel and x strides are inconsistent.");
        }

        if(m_xStrideBytes<0 || m_xStrideBytes==AutoStride)
        {
            throw Exception("PackedImageDesc Error: Invalid x stride.");
        }

        if(m_yStrideBytes<0 || m_yStrideBytes==AutoStride)
        {
            throw Exception("PackedImageDesc Error: Invalid y stride.");
        }

        if((m_xStrideBytes*m_width)>m_yStrideBytes)
        {
            throw Exception("PackedImageDesc Error: The x and y strides are inconsistent.");
        }

        if(m_bitDepth==BIT_DEPTH_UNKNOWN)
        {
            throw Exception("PackedImageDesc Error: Unknown bit-depth of the image buffer.");
        }
    }
};

PackedImageDesc::PackedImageDesc(void * data,
                                 long width, long height,
                                 long numChannels)
    :   ImageDesc()
    ,   m_impl(new PackedImageDesc::Impl)
{
    getImpl()->m_data        = data;
    getImpl()->m_width       = width;
    getImpl()->m_height      = height;
    getImpl()->m_numChannels = numChannels;
    getImpl()->m_bitDepth    = BIT_DEPTH_F32;

    if(numChannels==4)
    {
        getImpl()->m_chanOrder = CHANNEL_ORDERING_RGBA;
    }
    else if(numChannels==3)
    {
        getImpl()->m_chanOrder = CHANNEL_ORDERING_RGB;
    }
    else
    {
        throw Exception("PackedImageDesc Error: Invalid number of channels.");
    }

    constexpr unsigned oneChannelInBytes = sizeof(BitDepthInfo<BIT_DEPTH_F32>::Type);

    getImpl()->m_chanStrideBytes = oneChannelInBytes;
    getImpl()->m_xStrideBytes    = getImpl()->m_chanStrideBytes * getImpl()->m_numChannels;
    getImpl()->m_yStrideBytes    = getImpl()->m_xStrideBytes* width;

    getImpl()->initValues();

    getImpl()->m_isRGBAPacked = getImpl()->isRGBAPacked();
    getImpl()->m_isFloat      = getImpl()->isFloat();

    getImpl()->validate();
}

PackedImageDesc::PackedImageDesc(void * data,
                                 long width, long height,
                                 ChannelOrdering chanOrder)
    :   ImageDesc()
    ,   m_impl(new PackedImageDesc::Impl)
{
    getImpl()->m_data      = data;
    getImpl()->m_width     = width;
    getImpl()->m_height    = height;
    getImpl()->m_chanOrder = chanOrder;
    getImpl()->m_bitDepth  = BIT_DEPTH_F32;

    if(chanOrder==CHANNEL_ORDERING_RGBA 
        || chanOrder==CHANNEL_ORDERING_BGRA
        || chanOrder==CHANNEL_ORDERING_ABGR)
    {
        getImpl()->m_numChannels = 4;
    }
    else if(chanOrder==CHANNEL_ORDERING_RGB 
        || chanOrder==CHANNEL_ORDERING_BGR)
    {
        getImpl()->m_numChannels = 3;
    }
    else
    {
        throw Exception("PackedImageDesc Error: Unknown channel ordering.");
    }

    constexpr unsigned oneChannelInBytes = sizeof(BitDepthInfo<BIT_DEPTH_F32>::Type);

    getImpl()->m_chanStrideBytes = oneChannelInBytes;
    getImpl()->m_xStrideBytes    = getImpl()->m_chanStrideBytes * getImpl()->m_numChannels;
    getImpl()->m_yStrideBytes    = getImpl()->m_xStrideBytes * width;

    getImpl()->initValues();

    getImpl()->m_isRGBAPacked = getImpl()->isRGBAPacked();
    getImpl()->m_isFloat      = getImpl()->isFloat();

    getImpl()->validate();
}

PackedImageDesc::PackedImageDesc(void * data,
                                 long width, long height,
                                 ChannelOrdering chanOrder,
                                 BitDepth bitDepth,
                                 ptrdiff_t chanStrideBytes,
                                 ptrdiff_t xStrideBytes,
                                 ptrdiff_t yStrideBytes)
    :   ImageDesc()
    ,   m_impl(new PackedImageDesc::Impl)
{
    getImpl()->m_data      = data;
    getImpl()->m_width     = width;
    getImpl()->m_height    = height;
    getImpl()->m_chanOrder = chanOrder;
    getImpl()->m_bitDepth  = bitDepth;

    if(chanOrder==CHANNEL_ORDERING_RGBA 
        || chanOrder==CHANNEL_ORDERING_BGRA
        || chanOrder==CHANNEL_ORDERING_ABGR)
    {
        getImpl()->m_numChannels = 4;
    }
    else if(chanOrder==CHANNEL_ORDERING_RGB 
        || chanOrder==CHANNEL_ORDERING_BGR)
    {
        getImpl()->m_numChannels = 3;
    }
    else
    {
        throw Exception("PackedImageDesc Error: Unknown channel ordering.");
    }

    const unsigned oneChannelInBytes = GetChannelSizeInBytes(bitDepth);

    getImpl()->m_chanStrideBytes = (chanStrideBytes == AutoStride)
        ? oneChannelInBytes : chanStrideBytes;
    getImpl()->m_xStrideBytes = (xStrideBytes == AutoStride)
        ? getImpl()->m_chanStrideBytes * getImpl()->m_numChannels : xStrideBytes;
    getImpl()->m_yStrideBytes = (yStrideBytes == AutoStride)
        ? getImpl()->m_xStrideBytes * width : yStrideBytes;

    getImpl()->initValues();

    getImpl()->m_isRGBAPacked = getImpl()->isRGBAPacked();
    getImpl()->m_isFloat      = getImpl()->isFloat();

    getImpl()->validate();
}

PackedImageDesc::PackedImageDesc(void * data,
                                 long width, long height,
                                 long numChannels,
                                 BitDepth bitDepth,
                                 ptrdiff_t chanStrideBytes,
                                 ptrdiff_t xStrideBytes,
                                 ptrdiff_t yStrideBytes)
    :   ImageDesc()
    ,   m_impl(new PackedImageDesc::Impl)
{
    getImpl()->m_data        = data;
    getImpl()->m_width       = width;
    getImpl()->m_height      = height;
    getImpl()->m_numChannels = numChannels;
    getImpl()->m_bitDepth    = bitDepth;


    if (numChannels==4)
    {
        getImpl()->m_chanOrder = CHANNEL_ORDERING_RGBA;
    }
    else if (numChannels==3)
    {
        getImpl()->m_chanOrder = CHANNEL_ORDERING_RGB;
    }
    else
    {
        throw Exception("PackedImageDesc Error: Invalid number of channels.");
    }

    const unsigned oneChannelInBytes = GetChannelSizeInBytes(bitDepth);

    getImpl()->m_chanStrideBytes = (chanStrideBytes == AutoStride)
        ? oneChannelInBytes : chanStrideBytes;
    getImpl()->m_xStrideBytes = (xStrideBytes == AutoStride)
        ? getImpl()->m_chanStrideBytes * getImpl()->m_numChannels : xStrideBytes;
    getImpl()->m_yStrideBytes = (yStrideBytes == AutoStride)
        ? getImpl()->m_xStrideBytes * width : yStrideBytes;

    getImpl()->initValues();

    getImpl()->m_isRGBAPacked = getImpl()->isRGBAPacked();
    getImpl()->m_isFloat      = getImpl()->isFloat();

    getImpl()->validate();
}

PackedImageDesc::~PackedImageDesc()
{
    delete m_impl;
    m_impl = nullptr;
}

ChannelOrdering PackedImageDesc::getChannelOrder() const
{
    return getImpl()->m_chanOrder;
}

BitDepth PackedImageDesc::getBitDepth() const
{
    return getImpl()->m_bitDepth;
}

void * PackedImageDesc::getData() const
{
    return getImpl()->m_data;
}

void * PackedImageDesc::getRData() const
{
    return getImpl()->m_rData;
}

void * PackedImageDesc::getGData() const
{
    return getImpl()->m_gData;
}

void * PackedImageDesc::getBData() const
{
    return getImpl()->m_bData;
}

void * PackedImageDesc::getAData() const
{
    return getImpl()->m_aData;
}   

long PackedImageDesc::getWidth() const
{
    return getImpl()->m_width;
}

long PackedImageDesc::getHeight() const
{
    return getImpl()->m_height;
}

long PackedImageDesc::getNumChannels() const
{
    return getImpl()->m_numChannels;
}

ptrdiff_t PackedImageDesc::getChanStrideBytes() const
{
    return getImpl()->m_chanStrideBytes;
}

ptrdiff_t PackedImageDesc::getXStrideBytes() const
{
    return getImpl()->m_xStrideBytes;
}

ptrdiff_t PackedImageDesc::getYStrideBytes() const
{
    return getImpl()->m_yStrideBytes;
}

bool PackedImageDesc::isRGBAPacked() const
{
    return getImpl()->m_isRGBAPacked;
}

bool PackedImageDesc::isFloat() const
{
    return getImpl()->m_isFloat;
}

///////////////////////////////////////////////////////////////////////////

struct PlanarImageDesc::Impl
{
    void * m_rData = nullptr;
    void * m_gData = nullptr;
    void * m_bData = nullptr;
    void * m_aData = nullptr;

    BitDepth m_bitDepth = BIT_DEPTH_UNKNOWN;

    long m_width = 0;
    long m_height = 0;

    ptrdiff_t m_xStrideBytes = 0;
    ptrdiff_t m_yStrideBytes = 0;

    bool m_isFloat = false;

    bool isFloat() const
    {
        return m_xStrideBytes==sizeof(float) && m_bitDepth==BIT_DEPTH_F32;
    }

    void validate() const
    {
        if(m_xStrideBytes<0 || m_xStrideBytes==AutoStride)
        {
            throw Exception("PlanarImageDesc Error: Invalid x stride.");
        }

        if(m_yStrideBytes<0 || m_yStrideBytes==AutoStride)
        {
            throw Exception("PlanarImageDesc Error: Invalid y stride.");
        }

        if((m_xStrideBytes*m_width)>m_yStrideBytes)
        {
            throw Exception("PlanarImageDesc Error: The x and y strides are inconsistent.");
        }

        if(m_bitDepth==BIT_DEPTH_UNKNOWN)
        {
            throw Exception("PlanarImageDesc Error: Unknown bit-depth of the image buffer.");
        }
    }
};

PlanarImageDesc::PlanarImageDesc(void * rData, void * gData, void * bData, void * aData,
                                    long width, long height)
    :   ImageDesc()
    ,   m_impl(new PlanarImageDesc::Impl())
{
    if(rData==nullptr || gData==nullptr || bData==nullptr)
    {
        throw Exception("PlanarImageDesc Error: Invalid image buffer.");
    }

    if(width<=0 || height<=0)
    {
        throw Exception("PlanarImageDesc Error: Invalid image dimensions.");
    }

    getImpl()->m_bitDepth = BIT_DEPTH_F32;

    getImpl()->m_rData = rData;
    getImpl()->m_gData = gData;
    getImpl()->m_bData = bData;
    getImpl()->m_aData = aData;

    getImpl()->m_width  = width;
    getImpl()->m_height = height;

    getImpl()->m_xStrideBytes = sizeof(BitDepthInfo<BIT_DEPTH_F32>::Type);
    getImpl()->m_yStrideBytes = getImpl()->m_xStrideBytes * width;

    getImpl()->m_isFloat = getImpl()->isFloat();

    getImpl()->validate();
}

PlanarImageDesc::PlanarImageDesc(void * rData, void * gData, void * bData, void * aData,
                                    long width, long height,
                                    BitDepth bitDepth,
                                    ptrdiff_t xStrideBytes,
                                    ptrdiff_t yStrideBytes)
    :   ImageDesc()
    ,   m_impl(new PlanarImageDesc::Impl())
{
    if(rData==nullptr || gData==nullptr || bData==nullptr)
    {
        throw Exception("PlanarImageDesc Error: Invalid image buffer.");
    }

    if(width<=0 || height<=0)
    {
        throw Exception("PlanarImageDesc Error: Invalid image dimensions.");
    }

    getImpl()->m_bitDepth = bitDepth;

    getImpl()->m_rData = rData;
    getImpl()->m_gData = gData;
    getImpl()->m_bData = bData;
    getImpl()->m_aData = aData;

    getImpl()->m_width  = width;
    getImpl()->m_height = height;

    getImpl()->m_xStrideBytes = (xStrideBytes == AutoStride)
        ? sizeof(BitDepthInfo<BIT_DEPTH_F32>::Type) : xStrideBytes;

    if(xStrideBytes==AutoStride && bitDepth!=BIT_DEPTH_F32)
    {
        throw Exception("PlanarImageDesc Error: Mimsmatch between the bit-depth and channel stride.");
    }

    getImpl()->m_yStrideBytes = (yStrideBytes == AutoStride)
        ? getImpl()->m_xStrideBytes * width : yStrideBytes;

    getImpl()->m_isFloat = getImpl()->isFloat();

    getImpl()->validate();
}

PlanarImageDesc::~PlanarImageDesc()
{
    delete m_impl;
    m_impl = nullptr;
}

void * PlanarImageDesc::getRData() const
{
    return getImpl()->m_rData;
}

void * PlanarImageDesc::getGData() const
{
    return getImpl()->m_gData;
}

void * PlanarImageDesc::getBData() const
{
    return getImpl()->m_bData;
}

void * PlanarImageDesc::getAData() const
{
    return getImpl()->m_aData;
}

long PlanarImageDesc::getWidth() const
{
    return getImpl()->m_width;
}

long PlanarImageDesc::getHeight() const
{
    return getImpl()->m_height;
}

BitDepth PlanarImageDesc::getBitDepth() const
{
    return getImpl()->m_bitDepth;
}

ptrdiff_t PlanarImageDesc::getXStrideBytes() const
{
    return getImpl()->m_xStrideBytes;
}

ptrdiff_t PlanarImageDesc::getYStrideBytes() const
{
    return getImpl()->m_yStrideBytes;
}

bool PlanarImageDesc::isRGBAPacked() const
{
    return false;
}

bool PlanarImageDesc::isFloat() const
{
    return getImpl()->m_isFloat;
}

} // namespace OCIO_NAMESPACE
