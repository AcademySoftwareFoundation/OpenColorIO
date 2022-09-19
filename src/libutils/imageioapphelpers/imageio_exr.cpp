// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <ImfArray.h>
#include <ImfFrameBuffer.h>
#include <ImfChannelList.h>
#include <ImfPixelType.h>
#include <ImfHeader.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfFloatAttribute.h>
#include <ImfIntAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfStringAttribute.h>


namespace OCIO_NAMESPACE
{

namespace
{

BitDepth BitDepthFromPixelType(Imf::PixelType type)
{
    switch (type)
    {
        case Imf::HALF:  return BIT_DEPTH_F16;
        case Imf::FLOAT: return BIT_DEPTH_F32;
        case Imf::UINT:
        case Imf::NUM_PIXELTYPES:
        default:
        {
            std::stringstream ss;
            ss << "Error: Unsupported pixel type: " << type;
            throw Exception(ss.str().c_str());
        }
    }
}

Imf::PixelType BitDepthToPixelType(BitDepth bitdepth)
{
    switch (bitdepth)
    {
        case BIT_DEPTH_F16:     return Imf::HALF;
        case BIT_DEPTH_F32:     return Imf::FLOAT;
        case BIT_DEPTH_UNKNOWN:
        case BIT_DEPTH_UINT8:
        case BIT_DEPTH_UINT10:
        case BIT_DEPTH_UINT12:
        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT16:
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
    ss << "OpenEXR Version: " << OPENEXR_LIB_VERSION_STRING;
    return ss.str();
}

class ImageIO::Impl
{
public:
    Imf::Header m_header;
    std::vector<uint8_t> m_data;

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
        return m_data.data();
    }

    const uint8_t * getData() const
    {
        return m_data.data();
    }

    long getWidth() const
    {
        const Imath::Box2i & dw = m_header.dataWindow();
        return (long)(dw.max.x - dw.min.x + 1);
    }

    long getHeight() const
    {
        const Imath::Box2i & dw = m_header.dataWindow();
        return (long)(dw.max.y - dw.min.y + 1);
    }

    BitDepth getBitDepth() const
    {
        const Imf::ChannelList & channels = m_header.channels();
        if (channels.begin() != channels.end())
        {
            return BitDepthFromPixelType(channels.begin().channel().type);
        }
        else
        {
            return BIT_DEPTH_UNKNOWN;
        }
    }

    long getNumChannels() const
    {
        long channel_count = 0;
        const Imf::ChannelList & channels = m_header.channels();
        for (auto it = channels.begin(); it != channels.end(); ++it)
        {
            ++channel_count;
        }
        return channel_count;
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
        m_header.insert(name, Imf::StringAttribute(value));
    }

    void attribute(const std::string & name, float value)
    {
        m_header.insert(name, Imf::FloatAttribute(value));
    }

    void attribute(const std::string & name, int value)
    {
        m_header.insert(name, Imf::IntAttribute(value));
    }

    void init(const ImageIO::Impl & img, BitDepth bitDepth)
    {
        bitDepth = bitDepth == BIT_DEPTH_UNKNOWN ? img.getBitDepth() : bitDepth;

        const size_t numChans = img.getNumChannels();
        const size_t bitDepthBytes = GetChannelSizeInBytes(bitDepth);
        const size_t imgSizeInBytes =
            bitDepthBytes * numChans * (size_t)(img.getWidth() * img.getHeight());

        m_data.resize(imgSizeInBytes, 0);

        m_header = img.m_header;

        m_header.channels() = Imf::ChannelList();

        Imf::PixelType pixelType = BitDepthToPixelType(bitDepth);
        for (auto name : GetChannelNames(img.getChannelOrder()))
        {
            m_header.channels().insert(name, Imf::Channel(pixelType));
        }
    }

    void init(long width, long height, ChannelOrdering chanOrder, BitDepth bitDepth)
    {
        const size_t numChans = GetNumChannels(chanOrder);
        const size_t bitDepthBytes = GetChannelSizeInBytes(bitDepth);
        const size_t imgSizeInBytes = bitDepthBytes * numChans * (size_t)(width * height);

        m_data.resize(imgSizeInBytes, 0);

        m_header = Imf::Header();

        m_header.dataWindow().min.x = 0;
        m_header.dataWindow().min.y = 0;
        m_header.dataWindow().max.x = (int)width - 1;
        m_header.dataWindow().max.y = (int)height - 1;

        m_header.displayWindow().min.x = 0;
        m_header.displayWindow().min.y = 0;
        m_header.displayWindow().max.x = (int)width - 1;
        m_header.displayWindow().max.y = (int)height - 1;

        Imf::PixelType pixelType = BitDepthToPixelType(bitDepth);
        for (auto name : GetChannelNames(chanOrder))
        {
            m_header.channels().insert(name, Imf::Channel(pixelType));
        }
    }

    void read(const std::string & filename, BitDepth bitdepth)
    {
        Imf::InputFile file(filename.c_str());

        // Detect channels, RGB channels are required at a minimum. If channels
        // R, G, and B don't exist, they will be created and zero filled.
        // Except for Alpha, no other channel are preserved.
        const Imf::ChannelList & chanList = file.header().channels();

        ChannelOrdering chanOrder = CHANNEL_ORDERING_RGB;
        if (chanList.findChannel(RgbaChans[3]))
        {
            chanOrder = CHANNEL_ORDERING_RGBA;
        }

        // Detect pixel type, support only 16 or 32 bits floating point. All
        // channels will be converted to the same type if required.
        Imf::PixelType pixelType = Imf::HALF;

        // Use the specified bitdepth as requested.
        if (bitdepth != BIT_DEPTH_UNKNOWN)
        {
            pixelType = BitDepthToPixelType(bitdepth);
        }
        // Start with the minimum supported bit-depth and increase to match the
        // channel with the largest pixel type.
        else
        {
            for (const auto & name : RgbaChans)
            {
                auto chan = chanList.findChannel(name);
                if (chan && chan->type == Imf::FLOAT)
                {
                    pixelType = Imf::FLOAT;
                    break;
                }
            }
        }

        // Allocate buffer for image data
        const Imath::Box2i & dw = file.header().dataWindow();
        const long width  = (long)(dw.max.x - dw.min.x + 1);
        const long height = (long)(dw.max.y - dw.min.y + 1);
        init(width, height, chanOrder, BitDepthFromPixelType(pixelType));

        // Copy existing attributes, except for channels which we force to
        // RGB or RGBA of the derived pixel type.
        Imf::Header::ConstIterator attrIt = file.header().begin();
        for (; attrIt != file.header().end(); attrIt++)
        {
            if (std::string(attrIt.name()) == "channels")
            {
                continue;
            }

            m_header.insert(attrIt.name(), attrIt.attribute());
        }

        // Read pixels into buffer
        const size_t x          = (size_t)dw.min.x;
        const size_t y          = (size_t)dw.min.y;
        const size_t chanStride = (size_t)getChanStrideBytes();
        const size_t xStride    = (size_t)getXStrideBytes();
        const size_t yStride    = (size_t)getYStrideBytes();

        Imf::FrameBuffer frameBuffer;

        const std::vector<std::string> chanNames = getChannelNames();
        for (size_t i = 0; i < chanNames.size(); i++)
        {
            frameBuffer.insert(
                chanNames[i],
                Imf::Slice(
                    pixelType,
                    (char *)(getData() - x*xStride - y*yStride + i*chanStride),
                    xStride, yStride,
                    1, 1,
                    // RGB default to 0.0, A default to 1.0
                    (i == 3 ? 1.0 : 0.0)
                )
            );
        }

        file.setFrameBuffer(frameBuffer);
        file.readPixels(dw.min.y, dw.max.y);
    }

    void write(const std::string & filename, BitDepth bitdepth)
    {
        Imf::Header header(m_header);

        Imf::OutputFile file(filename.c_str(), header);

        const Imath::Box2i & dw = header.dataWindow();
        const size_t x          = (size_t)dw.min.x;
        const size_t y          = (size_t)dw.min.y;
        const size_t chanStride = (size_t)getChanStrideBytes();
        const size_t xStride    = (size_t)getXStrideBytes();
        const size_t yStride    = (size_t)getYStrideBytes();

        const std::vector<std::string> chanNames = getChannelNames();

        Imf::PixelType pixelType;
        // Use the specified bitdepth as requested.
        if (bitdepth != BIT_DEPTH_UNKNOWN)
        {
            pixelType = BitDepthToPixelType(bitdepth);
        }
        else
        {
            pixelType = BitDepthToPixelType(getBitDepth());
        }

        Imf::FrameBuffer frameBuffer;

        for (size_t i = 0; i < chanNames.size(); i++)
        {
            frameBuffer.insert(
                chanNames[i],
                Imf::Slice(
                    pixelType,
                    (char *)(getData() - x*xStride - y*yStride + i*chanStride),
                    xStride, yStride,
                    1, 1,
                    // RGB default to 0.0, A default to 1.0
                    (i == 3 ? 1.0 : 0.0)
                )
            );
        }

        file.setFrameBuffer(frameBuffer);
        file.writePixels(getHeight());
    }

};

} // namespace OCIO_NAMESPACE
