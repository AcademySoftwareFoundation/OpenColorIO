// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>


#include "OpenEXR/half.h"
#include "oiiohelpers.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

BitDepth GetBitDepth(const OIIO::ImageSpec & spec)
{
    switch(spec.format.basetype)
    {
        case OIIO::TypeDesc::FLOAT:  return BIT_DEPTH_F32;
        case OIIO::TypeDesc::HALF:   return BIT_DEPTH_F16;
        case OIIO::TypeDesc::UINT16: return BIT_DEPTH_UINT16;
        case OIIO::TypeDesc::UINT8:  return BIT_DEPTH_UINT8;
    }

    std::stringstream ss;
    ss << "Error: Unsupported format: " << spec.format;
    throw Exception(ss.str().c_str());
}


namespace
{

ChannelOrdering GetChannelOrdering(const OIIO::ImageSpec & spec)
{
    std::string channels;
    for(auto chan : spec.channelnames)
    {
        channels += StringUtils::Upper(chan);
    }

    if(channels=="RGBA")
    {
        return CHANNEL_ORDERING_RGBA;
    }
    else if(channels=="RGB")
    {
        return CHANNEL_ORDERING_RGB;
    }
    else
    {
        const std::size_t found = channels.find_first_of("RGB");
        if(found!=std::string::npos && spec.nchannels==4)
        {
            // TODO: Consider as RGBA, but to be investigated...
            return CHANNEL_ORDERING_RGBA;
        }
    }

    std::stringstream ss;
    ss << "Error: Unsupported channel ordering: " << channels;
    throw Exception(ss.str().c_str());
}

ImageDescRcPtr CreateImageDesc(const OIIO::ImageSpec & spec, void * imgBuffer)
{
    return std::make_shared<PackedImageDesc>(imgBuffer,
                                             spec.width,
                                             spec.height,
                                             GetChannelOrdering(spec),
                                             GetBitDepth(spec),
                                             spec.channel_bytes(),
                                             spec.pixel_bytes(),
                                             spec.scanline_bytes());
}

void * AllocateImageBuffer(const OIIO::ImageSpec & spec)
{
    const size_t imgSizeInChars = spec.scanline_bytes() * spec.height;
    return (void *)new char[imgSizeInChars];
}

void DeallocateImageBuffer(const OIIO::ImageSpec & spec, void * & img)
{
    delete [](char*)img;
    img = nullptr;
}

void CopyImageBuffer(void * dstBuffer, void * srcBuffer, const OIIO::ImageSpec & spec)
{
    const size_t imgSizeInChars = spec.scanline_bytes() * spec.height;
    memcpy(dstBuffer, srcBuffer, imgSizeInChars);
}


} // anon


void PrintImageSpec(const OIIO::ImageSpec & spec, bool verbose)
{
    if(verbose)
    {
        std::cout << std::endl;
        std::cout << "Image specifications are:" << std::endl
                  << "\twith:     \t" << spec.width << std::endl
                  << "\theight:   \t" << spec.height << std::endl
                  << "\tchannels: \t" << spec.nchannels << std::endl
                  << "\ttype:     \t" << spec.format << std::endl
                  << "\tformat:   \t";
        for(int i = 0; i < spec.nchannels; ++i)
        {
            if(i < (int)spec.channelnames.size())
            {
                std::cout << spec.channelnames[i];
            }
            else
            {
                std::cout << "Unknown";
            }
            if(i < (int)spec.channelformats.size())
            {
                std::cout << " (%s)" << spec.channelformats[i];
            }
            if(i < (spec.nchannels - 1))
            {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;

        std::cout << "\tImage:        \t[" << spec.x << ", " << spec.y << "] to ["
                  << (spec.x+spec.width) << ", " << (spec.y+spec.height) << "]"
                  << std::endl;
        std::cout << "\tFull Image:   \t[" << spec.full_x << ", " << spec.full_y << "] to ["
                  << (spec.full_x + spec.full_width) << ", " << (spec.full_y + spec.full_height) << "]"
                  << std::endl;

        std::cout << "\tExtra Attributes:" << std::endl;
        for(auto attrib : spec.extra_attribs)
        {
            const std::string val = spec.metadata_val(attrib, true);
            std::cout << "\t\t" << attrib.name() << ": "
                      << val << std::endl;
        }
    }
    else
    {
        std::cout << std::endl;
        std::cout << "Image: [" << spec.width << "x" << spec.height << "] "
                  << spec.format << " ";
        for(int i = 0; i < spec.nchannels; ++i)
        {
            if(i < (int)spec.channelnames.size())
            {
                std::cout << spec.channelnames[i];
            }
            else
            {
                std::cout << "Unknown";
            }
        }
        std::cout << std::endl;
    }
}

ImgBuffer::ImgBuffer(const OIIO::ImageSpec & spec)
    :   m_spec(spec)
    ,   m_buffer(nullptr)
{
    m_buffer = AllocateImageBuffer(m_spec);
}

ImgBuffer::ImgBuffer(const ImgBuffer & img)
    :   m_spec(img.m_spec)
    ,   m_buffer(nullptr)
{
    m_buffer = AllocateImageBuffer(m_spec);
    CopyImageBuffer(m_buffer, img.m_buffer, img.m_spec);
}

ImgBuffer::~ImgBuffer()
{
    if(m_buffer!=nullptr)
    {
        DeallocateImageBuffer(m_spec, m_buffer);
        m_buffer = nullptr;
    }
}

void ImgBuffer::allocate(const OIIO::ImageSpec & spec)
{
    if(m_buffer!=nullptr)
    {
        DeallocateImageBuffer(m_spec, m_buffer);
    }

    m_spec = spec;
    m_buffer = AllocateImageBuffer(spec);
}

ImgBuffer & ImgBuffer::operator= (ImgBuffer && img) noexcept
{
    if(this!=&img)
    {
        DeallocateImageBuffer(m_spec, m_buffer);

        m_spec   = img.m_spec;
        m_buffer = img.m_buffer;

        img.m_buffer = nullptr;
    }

    return *this;
}

ImageDescRcPtr CreateImageDesc(const OIIO::ImageSpec & spec, const ImgBuffer & img)
{
    return CreateImageDesc(spec, img.getBuffer());
}


} // namespace OCIO_NAMESPACE
