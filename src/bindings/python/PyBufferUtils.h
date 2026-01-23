// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYBUFFERUTILS_H
#define INCLUDED_OCIO_PYBUFFERUTILS_H

#include <string>

#include <OpenColorABI.h>
#include <OpenColorIO/OpenColorTypes.h>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
    
namespace OCIO_NAMESPACE
{

// Convert Python buffer protocol format code to NumPy dtype name
std::string formatCodeToDtypeName(const std::string & format, pybind11::ssize_t numBits);
// Convert OCIO BitDepth to NumPy dtype
pybind11::dtype bitDepthToDtype(BitDepth bitDepth);
// Convert OCIO BitDepth to data type byte count
pybind11::ssize_t bitDepthToBytes(BitDepth bitDepth);
// Convert OCIO ChannelOrdering to channel count
long chanOrderToNumChannels(ChannelOrdering chanOrder);

// Return string that describes Python buffer's N-dimensional array shape
std::string getBufferShapeStr(const pybind11::buffer_info & info);
// Return BitDepth for a supported Python buffer data type
BitDepth getBufferBitDepth(const pybind11::buffer_info & info);

// Throw if Python buffer format is incompatible with a NumPy dtype
void checkBufferType(const pybind11::buffer_info & info, const pybind11::dtype & dt);
// Throw if Python buffer format is incompatible with an OCIO BitDepth
void checkBufferType(const pybind11::buffer_info & info, BitDepth bitDepth);
// Throw if Python buffer size is not divisible by channel count
void checkBufferDivisible(const pybind11::buffer_info & info, pybind11::ssize_t numChannels);
// Throw if Python buffer does not have an exact count of entries
void checkBufferSize(const pybind11::buffer_info & info, pybind11::ssize_t numEntries);

// Calculate 3D grid size from a packed 3D LUT buffer
unsigned long getBufferLut3DGridSize(const pybind11::buffer_info & info);

// Throw if vector size is not divisible by channel count
void checkVectorDivisible(const std::vector<float> & pixel, size_t numChannels);

// Throw if array is not C-contiguous
void checkCContiguousArray(const pybind11::buffer_info & info);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PYBUFFERUTILS_H
