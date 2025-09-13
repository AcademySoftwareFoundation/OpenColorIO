// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "imageio.h"

namespace OCIO_NAMESPACE
{

namespace
{

const std::vector<std::string> RgbaChans = { "R", "G", "B", "A" };
const std::vector<std::string> RgbChans  = { "R", "G", "B" };

std::vector<std::string> GetChannelNames(const ChannelOrdering & chanOrder)
{
    switch (chanOrder)
    {
        case CHANNEL_ORDERING_RGBA:
            return RgbaChans;
        case CHANNEL_ORDERING_RGB:
            return RgbChans;
        case CHANNEL_ORDERING_BGRA:
        case CHANNEL_ORDERING_ABGR:
        case CHANNEL_ORDERING_BGR:
        default:
        {
            std::stringstream ss;
            ss << "Error: Unsupported channel ordering: " << chanOrder;
            throw Exception(ss.str().c_str());
        }
    }
}

size_t GetNumChannels(const ChannelOrdering & chanOrder)
{
    switch (chanOrder)
    {
        case CHANNEL_ORDERING_RGBA:
            return 4;
        case CHANNEL_ORDERING_RGB:
            return 3;
        case CHANNEL_ORDERING_BGRA:
        case CHANNEL_ORDERING_ABGR:
        case CHANNEL_ORDERING_BGR:
        default:
        {
            std::stringstream ss;
            ss << "Error: Unsupported channel ordering: " << chanOrder;
            throw Exception(ss.str().c_str());
        }
    }
}

unsigned GetChannelSizeInBytes(BitDepth bitdepth)
{
    switch(bitdepth)
    {
        case BIT_DEPTH_UINT8:
            return 1;
        case BIT_DEPTH_UINT16:
            return 2;
        case BIT_DEPTH_F16:
            return 2;
        case BIT_DEPTH_F32:
            return 4;
        case BIT_DEPTH_UINT10:
        case BIT_DEPTH_UINT12:
        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT32:
        case BIT_DEPTH_UNKNOWN:
        default:
        {
            std::stringstream ss;
            ss << "Error: Unsupported bitdepth: " << BitDepthToString(bitdepth);
            throw Exception(ss.str().c_str());
        }
    }
}

} // anonymous namespace

} // OCIO_NAMESPACE

#ifdef USE_OPENIMAGEIO
#   include "imageio_oiio.cpp"
#elif USE_OPENEXR
#   include "imageio_exr.cpp"
#else
#   error "No image backend found to compile ImageIO."
#endif

namespace OCIO_NAMESPACE
{

ImageIO::ImageIO()
: m_impl(new ImageIO::Impl())
{

}

ImageIO::ImageIO(const std::string & filename)
: m_impl(new ImageIO::Impl())
{
    m_impl->read(filename, BIT_DEPTH_UNKNOWN);
}

ImageIO::ImageIO(long width, long height, ChannelOrdering chanOrder, BitDepth bitDepth)
: m_impl(new ImageIO::Impl())
{
    m_impl->init(width, height, chanOrder, bitDepth);
}

ImageIO::~ImageIO()
{
    delete m_impl;
    m_impl = nullptr;
}

std::string ImageIO::getImageDescStr() const
{
    return m_impl->getImageDescStr();
}

ImageDescRcPtr ImageIO::getImageDesc() const
{
    return m_impl->getImageDesc();
}

uint8_t * ImageIO::getData()
{
    return m_impl->getData();
}

const uint8_t * ImageIO::getData() const
{
    return m_impl->getData();
}

long ImageIO::getWidth() const
{
    return m_impl->getWidth();
}

long ImageIO::getHeight() const
{
    return m_impl->getHeight();
}

BitDepth ImageIO::getBitDepth() const
{
    return m_impl->getBitDepth();
}

long ImageIO::getNumChannels() const
{
    return m_impl->getNumChannels();
}

ChannelOrdering ImageIO::getChannelOrder() const
{
    return m_impl->getChannelOrder();
}

const std::vector<std::string> ImageIO::getChannelNames() const
{
    return m_impl->getChannelNames();
}

ptrdiff_t ImageIO::getChanStrideBytes() const
{
    return m_impl->getChanStrideBytes();
}

ptrdiff_t ImageIO::getXStrideBytes() const
{
    return m_impl->getXStrideBytes();
}

ptrdiff_t ImageIO::getYStrideBytes() const
{
    return m_impl->getYStrideBytes();
}

ptrdiff_t ImageIO::getImageBytes() const
{
    return m_impl->getImageBytes();
}

void ImageIO::init(const ImageIO & img, BitDepth bitDepth)
{
    m_impl->init(*img.m_impl, bitDepth);

    // Do not propagate colorInteropID.
    attribute("colorInteropID", "unknown");
}

void ImageIO::init(long width, long height, ChannelOrdering chanOrder, BitDepth bitDepth)
{
    m_impl->init(width, height, chanOrder, bitDepth);
}

void ImageIO::attribute(const std::string & name, const std::string & value)
{
    m_impl->attribute(name, value);
}

void ImageIO::attribute(const std::string & name, float value)
{
    m_impl->attribute(name, value);
}

void ImageIO::attribute(const std::string & name, int value)
{
    m_impl->attribute(name, value);
}

void ImageIO::read(const std::string & filename, BitDepth bitdepth)
{
    m_impl->read(filename, bitdepth);
}

void ImageIO::write(const std::string & filename, BitDepth bitdepth) const
{
    m_impl->write(filename, bitdepth);
}


} // namespace OCIO_NAMESPACE
