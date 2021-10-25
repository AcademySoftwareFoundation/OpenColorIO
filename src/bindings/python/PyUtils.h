// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYUTILS_H
#define INCLUDED_OCIO_PYUTILS_H

#include <string>
#include <tuple>
#include <vector>

#include "PyOpenColorIO.h"

namespace OCIO_NAMESPACE
{

// Define __repr__ implementation compatible with *most* OCIO classes
template<typename T, typename ... EXTRA>
void defRepr(py::class_<T, OCIO_SHARED_PTR<T>, EXTRA ...> & cls)
{
    cls.def("__repr__", [](OCIO_SHARED_PTR<T> & self)
        { 
            std::ostringstream os;
            os << (*self);
            return os.str();
        });
}

template<typename T>
void defRepr(py::class_<T> & cls)
{
    cls.def("__repr__", [](T & self)
        { 
            std::ostringstream os;
            os << self;
            return os.str();
        });
}

// Standard interface for Python iterator mechanics
template<typename T, int UNIQUE, typename ... ARGS>
struct PyIterator
{
    PyIterator(T obj, ARGS ... args) : m_obj(obj), m_args(args ...) {}

    int nextIndex(int num)
    {
        if (m_i >= num) { throw py::stop_iteration(); }
        return m_i++;
    }

    void checkIndex(int i, int num)
    {
        if (i >= num) { throw py::index_error("Iterator index out of range"); }
    }

    T m_obj;
    std::tuple<ARGS ...> m_args;

private:
    int m_i = 0;
};

// Convert Python buffer protocol format code to NumPy dtype name
std::string formatCodeToDtypeName(const std::string & format, py::ssize_t numBits);
// Convert OCIO BitDepth to NumPy dtype
py::dtype bitDepthToDtype(BitDepth bitDepth);
// Convert OCIO BitDepth to data type byte count
py::ssize_t bitDepthToBytes(BitDepth bitDepth);
// Convert OCIO ChannelOrdering to channel count
long chanOrderToNumChannels(ChannelOrdering chanOrder);

// Return string that describes Python buffer's N-dimensional array shape
std::string getBufferShapeStr(const py::buffer_info & info);
// Return BitDepth for a supported Python buffer data type
BitDepth getBufferBitDepth(const py::buffer_info & info);

// Throw if Python buffer format is incompatible with a NumPy dtype
void checkBufferType(const py::buffer_info & info, const py::dtype & dt);
// Throw if Python buffer format is incompatible with an OCIO BitDepth
void checkBufferType(const py::buffer_info & info, BitDepth bitDepth);
// Throw if Python buffer size is not divisible by channel count
void checkBufferDivisible(const py::buffer_info & info, py::ssize_t numChannels);
// Throw if Python buffer does not have an exact count of entries
void checkBufferSize(const py::buffer_info & info, py::ssize_t numEntries);

// Calculate 3D grid size from a packed 3D LUT buffer
unsigned long getBufferLut3DGridSize(const py::buffer_info & info);

// Throw if vector size is not divisible by channel count
void checkVectorDivisible(const std::vector<float> & pixel, size_t numChannels);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PYUTILS_H
