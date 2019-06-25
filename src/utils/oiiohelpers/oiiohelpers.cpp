/*
Copyright (c) 2019 Autodesk Inc., et al.
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


#include "ilmbase/half.h"
#include "oiiohelpers.h"
#include "pystring.h"


OCIO_NAMESPACE_ENTER
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

    return BIT_DEPTH_F32;
}


namespace
{

ChannelOrdering GetChannelOrdering(const OIIO::ImageSpec & spec)
{
    std::string channels;
    for(auto chan : spec.channelnames)
    {
        channels += pystring::capitalize(chan);
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

    return CHANNEL_ORDERING_RGBA;
}

ImageDescRcPtr CreateImageDesc(const OIIO::ImageSpec & spec, 
                              int imgWidth, int imgHeight, 
                              void * imgBuffer)
{
    return std::make_shared<PackedImageDesc>(imgBuffer, 
                                             imgWidth,
                                             imgHeight,
                                             GetChannelOrdering(spec),
                                             spec.channel_bytes(),
                                             spec.pixel_bytes(),
                                             spec.scanline_bytes());
}

ImageDescRcPtr CreateImageDesc(const OIIO::ImageSpec & spec, void * imgBuffer)
{
    return CreateImageDesc(spec, spec.width, spec.height, imgBuffer);
}

void * AllocateImageBuffer(const OIIO::ImageSpec & spec, int imgWidth, int imgHeight)
{
    const size_t imgSize = imgWidth * imgHeight * spec.nchannels;

    if(spec.format==OIIO::TypeDesc::FLOAT)
    {
        return new float[imgSize];
    }
    else if(spec.format==OIIO::TypeDesc::HALF)
    {
        return new half[imgSize];
    }
    else if(spec.format==OIIO::TypeDesc::UINT16)
    {
        return new uint16_t[imgSize];
    }
    else if(spec.format==OIIO::TypeDesc::UINT8)
    {
        return new uint8_t[imgSize];
    }

    std::stringstream ss;
    ss << "Error: Unsupported image type: " << spec.format << std::endl;
    throw Exception(ss.str().c_str());

    return nullptr;  
}

void * AllocateImageBuffer(const OIIO::ImageSpec & spec)
{
    return AllocateImageBuffer(spec, spec.width, spec.height);
}

void DesallocateImageBuffer(const OIIO::ImageSpec & spec, void * & img)
{
    if(spec.format==OIIO::TypeDesc::FLOAT)
    {
        delete [](float*)img;
        img = nullptr;
    }
    else if(spec.format==OIIO::TypeDesc::HALF)
    {
        delete [] (half*)img;
    }
    else if(spec.format==OIIO::TypeDesc::UINT16)
    {
        delete [](uint16_t*)img;
        img = nullptr;
    }
    else if(spec.format==OIIO::TypeDesc::UINT8)
    {
        delete [](uint8_t*)img;
        img = nullptr;
    }
    else
    {
        std::stringstream ss;
        ss << "Error: Unsupported image type: " << spec.format << std::endl;
        throw Exception(ss.str().c_str());
    }   
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
}

ImgBuffer::ImgBuffer(const OIIO::ImageSpec & spec)
    :   m_spec(spec)
    ,   m_buffer(nullptr)
{ 
    allocate(m_spec);
}

ImgBuffer::~ImgBuffer() 
{ 
    if(m_buffer!=nullptr)
    {
        DesallocateImageBuffer(m_spec, m_buffer);
        m_buffer = nullptr;
    }
}

void ImgBuffer::allocate(const OIIO::ImageSpec & spec)
{
    if(m_buffer!=nullptr)
    {
        DesallocateImageBuffer(m_spec, m_buffer);
    }

    m_spec = spec;
    m_buffer = AllocateImageBuffer(spec);
}

ImgBuffer & ImgBuffer::operator= (ImgBuffer && img)
{
    if(this!=&img)
    {
        DesallocateImageBuffer(m_spec, m_buffer);

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


}
OCIO_NAMESPACE_EXIT
