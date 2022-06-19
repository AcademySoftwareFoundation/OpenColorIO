// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_IMAGEIO_H
#define INCLUDED_OCIO_IMAGEIO_H

#include <cstddef>
#include <string>
#include <vector>

#include "OpenColorIO.h"

namespace OCIO_NAMESPACE
{

/**
 * ImageIO provide basic support for image input and output.
 */
class ImageIO
{
public:
    static std::string GetVersion();

    ImageIO();

    // Construct and load the image into memory.
    explicit ImageIO(const std::string & filename);

    // Construct and allocate an empty image buffer.
    ImageIO(long width, long height, ChannelOrdering chanOrder, BitDepth bitDepth);

    ImageIO(const ImageIO & img) = delete;
    ImageIO(ImageIO &&) = delete;

    ImageIO & operator = (const ImageIO &) = delete;
    ImageIO & operator = (ImageIO &&) = delete;

    ~ImageIO();

    // Returns printable information about the image.
    std::string getImageDescStr() const;

    ImageDescRcPtr getImageDesc() const;

    uint8_t * getData();
    const uint8_t * getData() const;

    long getWidth() const;
    long getHeight() const;

    BitDepth getBitDepth() const;

    long getNumChannels() const;
    ChannelOrdering getChannelOrder() const;
    const std::vector<std::string> getChannelNames() const;

    ptrdiff_t getChanStrideBytes() const;
    ptrdiff_t getXStrideBytes() const;
    ptrdiff_t getYStrideBytes() const;
    ptrdiff_t getImageBytes() const;

    // Set metadata attributes on the image, depends on format.
    void attribute(const std::string & name, const std::string & value);
    void attribute(const std::string & name, float value);
    void attribute(const std::string & name, int value);

    // Initialize to an empty image buffer.
    void init(const ImageIO & img, BitDepth bitDepth = BIT_DEPTH_UNKNOWN);
    void init(long width, long height, ChannelOrdering chanOrder, BitDepth bitDepth);

    // Read or write using the specified bitdepth or the input / current bitdepth.
    void read(const std::string & filename, BitDepth bitdepth = BIT_DEPTH_UNKNOWN);
    void write(const std::string & filename, BitDepth bitdepth = BIT_DEPTH_UNKNOWN) const;

private:
    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_IMAGEIO_H
