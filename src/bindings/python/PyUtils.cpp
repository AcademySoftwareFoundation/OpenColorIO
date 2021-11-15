// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cmath>
#include <sstream>

#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

namespace 
{

// Reference:
//   https://github.com/python/cpython/blob/main/Objects/memoryobject.c
//   https://docs.python.org/3.7/c-api/arg.html#numbers
//   https://numpy.org/devdocs/user/basics.types.html

const std::vector<std::string>  UINT_FORMATS = { "B", "H", "I", "L", "Q", "N" };
const std::vector<std::string>   INT_FORMATS = { "b", "h", "i", "l", "q", "n" };
const std::vector<std::string> FLOAT_FORMATS = { "e", "f", "d", "g", "Ze", "Zf", "Zd", "Zg" };

} // namespace

std::string formatCodeToDtypeName(const std::string & format, py::ssize_t numBits)
{
    std::ostringstream os;

    if (std::find(std::begin(FLOAT_FORMATS), 
                  std::end(FLOAT_FORMATS), 
                  format) != std::end(FLOAT_FORMATS))
    {
        os << "float" << numBits;
    }
    else if (std::find(std::begin(UINT_FORMATS), 
                       std::end(UINT_FORMATS), 
                       format) != std::end(UINT_FORMATS))
    {
        os << "uint" << numBits;
    }
    else if (std::find(std::begin(INT_FORMATS), 
                       std::end(INT_FORMATS), 
                       format) != std::end(INT_FORMATS))
    {
        os << "int" << numBits;
    }
    else 
    {
        os << "'" << format << "' (" << numBits << "-bit)";
    }

    return os.str();
}

py::dtype bitDepthToDtype(BitDepth bitDepth)
{
    std::string name, err;

    switch(bitDepth)
    {
        case BIT_DEPTH_UINT8:
            name = "uint8";
            break;
        case BIT_DEPTH_UINT10:
        case BIT_DEPTH_UINT12:
        case BIT_DEPTH_UINT16:
            name = "uint16";
            break;
        case BIT_DEPTH_F16:
            name = "float16";
            break;
        case BIT_DEPTH_F32:
            name = "float32";
            break;
        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT32:
        case BIT_DEPTH_UNKNOWN:
        default:
            err = "Error: Unsupported bit-depth: ";
            err += BitDepthToString(bitDepth);
            throw Exception(err.c_str());
    }

    return py::dtype(name);
}

py::ssize_t bitDepthToBytes(BitDepth bitDepth)
{
    std::string name, err;

    switch(bitDepth)
    {
        case BIT_DEPTH_UINT8:
            return 1;
        case BIT_DEPTH_UINT10:
        case BIT_DEPTH_UINT12:
        case BIT_DEPTH_UINT16:
        case BIT_DEPTH_F16:
            return 2;
        case BIT_DEPTH_F32:
            return 4;
        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT32:
        case BIT_DEPTH_UNKNOWN:
        default:
            err = "Error: Unsupported bit-depth: ";
            err += BitDepthToString(bitDepth);
            throw Exception(err.c_str());
    }
}

long chanOrderToNumChannels(ChannelOrdering chanOrder)
{
    switch (chanOrder)
    {
        case CHANNEL_ORDERING_RGBA:
        case CHANNEL_ORDERING_BGRA:
        case CHANNEL_ORDERING_ABGR:
            return 4;
        case CHANNEL_ORDERING_RGB:
        case CHANNEL_ORDERING_BGR:
            return 3;
        default:
            throw Exception("Error: Unsupported channel ordering");
    }
}

std::string getBufferShapeStr(const py::buffer_info & info)
{
    std::ostringstream os;
    os << "(";
    for (size_t i = 0; i < info.shape.size(); i++) 
    {
        os << info.shape[i] << (i < info.shape.size()-1 ? ", " : "" );
    }
    os << ")";
    return os.str();
}

BitDepth getBufferBitDepth(const py::buffer_info & info)
{
    BitDepth bitDepth;
    
    std::string dtName = formatCodeToDtypeName(info.format, info.itemsize*8);

    if (dtName == "float32")
        bitDepth = BIT_DEPTH_F32;
    else if (dtName == "float16")
        bitDepth = BIT_DEPTH_F16;
    else if (dtName == "uint16" || dtName == "uint12" || dtName == "uint10")
        bitDepth = BIT_DEPTH_UINT16;
    else if (dtName == "uint8")
        bitDepth = BIT_DEPTH_UINT8;
    else 
    {
        std::ostringstream os;
        os << "Unsupported data type: " << dtName;
        throw std::runtime_error(os.str().c_str());
    }

    return bitDepth;
}

void checkBufferType(const py::buffer_info & info, const py::dtype & dt)
{
    if (!py::dtype(info).is(dt))
    {
        std::ostringstream os;
        os << "Incompatible buffer format: expected ";
        os << formatCodeToDtypeName(std::string(1, dt.kind()), dt.itemsize()*8);
        os << ", but received " << formatCodeToDtypeName(info.format, info.itemsize*8);
        throw std::runtime_error(os.str().c_str());
    }
}

void checkBufferType(const py::buffer_info & info, BitDepth bitDepth)
{
    checkBufferType(info, bitDepthToDtype(bitDepth));
}

void checkBufferDivisible(const py::buffer_info & info, py::ssize_t numChannels)
{
    if (info.size % numChannels != 0)
    {
        std::ostringstream os;
        os << "Incompatible buffer dimensions: expected size to be divisible by " << numChannels;
        os << ", but received " << info.size << " entries";
        throw std::runtime_error(os.str().c_str());
    }
}

void checkBufferSize(const py::buffer_info & info, py::ssize_t numEntries)
{
    if (info.size != numEntries)
    {
        std::ostringstream os;
        os << "Incompatible buffer dimensions: expected " << numEntries;
        os << " entries, but received " << info.size << " entries";
        throw std::runtime_error(os.str().c_str());
    }
}

unsigned long getBufferLut3DGridSize(const py::buffer_info & info)
{
    checkBufferDivisible(info, 3);

    unsigned long gs = 2;
    unsigned long size = (info.size >= 0 ? static_cast<unsigned long>(info.size) : 0);

    if (info.ndim == 1)
    {
        gs = static_cast<unsigned long>(std::round(std::cbrt(size / 3)));
    }
    else if (info.ndim > 1)
    {
        gs = (info.size >= 0 ? static_cast<unsigned long>(info.shape[0]) : 0);
    }

    if (gs*gs*gs * 3 != size)
    {
        std::ostringstream os;
        os << "Incompatible buffer dimensions: failed to calculate grid size from shape ";
        os << getBufferShapeStr(info);
        throw std::runtime_error(os.str().c_str());
    }

    return gs;
}

void checkVectorDivisible(const std::vector<float> & pixel, size_t numChannels)
{
    if (pixel.size() % numChannels != 0)
    {
        std::ostringstream os;
        os << "Incompatible vector dimensions: expected (N*" << numChannels;
        os << ", 1), but received (" << pixel.size() << ", 1)";
        throw std::runtime_error(os.str().c_str());
    }
}

} // namespace OCIO_NAMESPACE
