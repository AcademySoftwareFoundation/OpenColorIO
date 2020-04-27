// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_OIIO_HELPERS_H
#define INCLUDED_OCIO_OIIO_HELPERS_H

#include <OpenColorIO/OpenColorIO.h>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
#if (OIIO_VERSION < 10100)
namespace OIIO = OIIO_NAMESPACE;
#endif

namespace OCIO_NAMESPACE
{

BitDepth GetBitDepth(const OIIO::ImageSpec & spec);

// Print information of the image.
void PrintImageSpec(const OIIO::ImageSpec & spec, bool verbose);

// Manage the image buffer.
class ImgBuffer
{
public:
    ImgBuffer() = default;
    ImgBuffer(const OIIO::ImageSpec & spec);
    ImgBuffer(const ImgBuffer & img);

    ImgBuffer(ImgBuffer &&) = delete;

    ImgBuffer & operator = (const ImgBuffer &) = delete;
    ImgBuffer & operator = (ImgBuffer &&) noexcept;

    ~ImgBuffer();

    void allocate(const OIIO::ImageSpec & spec);

    inline void * getBuffer() const noexcept { return m_buffer; }

private:
    OIIO::ImageSpec m_spec;
    void * m_buffer = nullptr;
};


ImageDescRcPtr CreateImageDesc(const OIIO::ImageSpec & spec, const ImgBuffer & img);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_OIIO_HELPERS_H

