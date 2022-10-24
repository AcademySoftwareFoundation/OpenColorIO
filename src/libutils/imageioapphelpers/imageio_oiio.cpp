// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>


namespace OCIO_NAMESPACE
{

namespace
{

BitDepth BitDepthFromTypeDesc(OIIO::TypeDesc type)
{
    switch (type.basetype)
    {
        case OIIO::TypeDesc::UINT8:     return BIT_DEPTH_UINT8;
        case OIIO::TypeDesc::UINT16:    return BIT_DEPTH_UINT16;
        case OIIO::TypeDesc::HALF:      return BIT_DEPTH_F16;
        case OIIO::TypeDesc::FLOAT:     return BIT_DEPTH_F32;
        case OIIO::TypeDesc::UNKNOWN:
        case OIIO::TypeDesc::NONE:
        case OIIO::TypeDesc::INT8:
        case OIIO::TypeDesc::INT16:
        case OIIO::TypeDesc::UINT32:
        case OIIO::TypeDesc::INT32:
        case OIIO::TypeDesc::UINT64:
        case OIIO::TypeDesc::INT64:
        case OIIO::TypeDesc::DOUBLE:
        case OIIO::TypeDesc::STRING:
        case OIIO::TypeDesc::PTR:
        case OIIO::TypeDesc::LASTBASE:
        default:
        {
            std::stringstream ss;
            ss << "Error: Unsupported type desc: " << type;
            throw Exception(ss.str().c_str());
        }
    }
}

OIIO::TypeDesc BitDepthToTypeDesc(BitDepth bitdepth)
{
    switch (bitdepth)
    {
        case BIT_DEPTH_UINT8:   return OIIO::TypeDesc(OIIO::TypeDesc::UINT8);
        case BIT_DEPTH_UINT16:  return OIIO::TypeDesc(OIIO::TypeDesc::UINT16);
        case BIT_DEPTH_F16:     return OIIO::TypeDesc(OIIO::TypeDesc::HALF);
        case BIT_DEPTH_F32:     return OIIO::TypeDesc(OIIO::TypeDesc::FLOAT);
        case BIT_DEPTH_UNKNOWN: return OIIO::TypeDesc(OIIO::TypeDesc::UNKNOWN);
        case BIT_DEPTH_UINT10:
        case BIT_DEPTH_UINT12:
        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT32:
        default:
        {
            std::stringstream ss;
            ss << "Error: Unsupported bitdepth: " << BitDepthToString(bitdepth);
            throw Exception(ss.str().c_str());
        }
    }
}

} // anonymous namespace

std::string ImageIO::GetVersion()
{
    std::ostringstream ss;
    ss << "OpenImageIO Version: " << OIIO_VERSION_STRING;
    return ss.str();
}

class ImageIO::Impl
{
public:
    OIIO::ImageBuf m_buffer;

    Impl() = default;

    Impl(const Impl &) = delete;
    Impl(Impl &&) = delete;

    Impl& operator= (const Impl & rhs) = delete;
    Impl& operator= (Impl && rhs) = delete;

    ~Impl() = default;

    std::string getImageDescStr() const
    {
        std::ostringstream ss;

        ss << std::endl;
        ss << "Image: [" << getWidth() << "x" << getHeight() << "] "
           << BitDepthToString(getBitDepth()) << " ";

        const std::vector<std::string> chanNames = getChannelNames();
        for (int i = 0; i < getNumChannels(); ++i)
        {
            if (i < (int)chanNames.size())
            {
                ss << chanNames[i];
            }
            else
            {
                ss << "Unknown";
            }

            if (i < (getNumChannels() - 1))
            {
                ss << ", ";
            }
        }
        ss << std::endl;

        return ss.str();
    }

    ImageDescRcPtr getImageDesc() const
    {
        return std::make_shared<PackedImageDesc>(
            (void*) getData(),
            getWidth(),
            getHeight(),
            getChannelOrder(),
            getBitDepth(),
            getChanStrideBytes(),
            getXStrideBytes(),
            getYStrideBytes()
        );
    }

    uint8_t * getData()
    {
        return (uint8_t*) m_buffer.localpixels();
    }

    const uint8_t * getData() const
    {
        return (const uint8_t*) m_buffer.localpixels();
    }

    long getWidth() const
    {
        return m_buffer.spec().width;
    }

    long getHeight() const
    {
        return m_buffer.spec().height;
    }

    BitDepth getBitDepth() const
    {
        return BitDepthFromTypeDesc(m_buffer.spec().format);
    }

    long getNumChannels() const
    {
        return m_buffer.spec().nchannels;
    }

    ChannelOrdering getChannelOrder() const
    {
        if (getNumChannels() == 4)
        {
            return CHANNEL_ORDERING_RGBA;
        }
        else
        {
            return CHANNEL_ORDERING_RGB;
        }
    }

    const std::vector<std::string> getChannelNames() const
    {
        return GetChannelNames(getChannelOrder());
    }

    ptrdiff_t getChanStrideBytes() const
    {
        return GetChannelSizeInBytes(getBitDepth());
    }

    ptrdiff_t getXStrideBytes() const
    {
        return getNumChannels() * getChanStrideBytes();
    }

    ptrdiff_t getYStrideBytes() const
    {
        return getWidth() * getXStrideBytes();
    }

    ptrdiff_t getImageBytes() const
    {
        return getYStrideBytes() * getHeight();
    }

    void attribute(const std::string & name, const std::string & value)
    {
        m_buffer.specmod().attribute(name, value);
    }

    void attribute(const std::string & name, float value)
    {
        m_buffer.specmod().attribute(name, value);
    }

    void attribute(const std::string & name, int value)
    {
        m_buffer.specmod().attribute(name, value);
    }

    void init(const ImageIO::Impl & img, BitDepth bitDepth)
    {
        bitDepth = bitDepth == BIT_DEPTH_UNKNOWN ? img.getBitDepth() : bitDepth;

        OIIO::ImageSpec spec = img.m_buffer.spec();
        spec.format = BitDepthToTypeDesc(bitDepth);

        m_buffer = OIIO::ImageBuf(spec);
    }

    void init(long width, long height, ChannelOrdering chanOrder, BitDepth bitDepth)
    {
        OIIO::ImageSpec spec(
            width, height,
            GetNumChannels(chanOrder),
            BitDepthToTypeDesc(bitDepth));

        m_buffer = OIIO::ImageBuf(spec);
    }

    void read(const std::string & filename, BitDepth bitdepth)
    {
        const OIIO::TypeDesc typeDesc = BitDepthToTypeDesc(bitdepth);

        m_buffer = OIIO::ImageBuf(filename);

        if (!m_buffer.read(
                0,              // subimage
                0,              // miplevel
                true,           // force immediate read
                typeDesc        // convert to type
        ))
        {
            std::stringstream ss;
            ss << "Error: Could not read image: " << m_buffer.geterror();
            throw Exception(ss.str().c_str());
        }
    }

    void write(const std::string & filename, BitDepth bitdepth)
    {
        const OIIO::TypeDesc typeDesc = BitDepthToTypeDesc(bitdepth);

        if (!m_buffer.write(filename, typeDesc))
        {
            std::stringstream ss;
            ss << "Error: Could not write image: " << m_buffer.geterror();
            throw Exception(ss.str().c_str());
        }
    }

};

} // namespace OCIO_NAMESPACE
