// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYUTILS_H
#define INCLUDED_OCIO_PYUTILS_H

#include <tuple>
#include <sstream>

#include "OpenColorABI.h"

#include <pybind11/pybind11.h>


namespace OCIO_NAMESPACE
{

// Define __repr__ implementation compatible with *most* OCIO classes
template<typename T, typename ... EXTRA>
void defRepr(pybind11::class_<T, OCIO_SHARED_PTR<T>, EXTRA ...> & cls)
{
    cls.def("__repr__", [](OCIO_SHARED_PTR<T> & self)
        { 
            std::ostringstream os;
            os << (*self);
            return os.str();
        });
}

template<typename T>
void defRepr(pybind11::class_<T> & cls)
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
        if (m_i >= num) { throw pybind11::stop_iteration(); }
        return m_i++;
    }

    void checkIndex(int i, int num)
    {
        if (i >= num) { throw pybind11::index_error("Iterator index out of range"); }
    }

    T m_obj;
    std::tuple<ARGS ...> m_args;

private:
    int m_i = 0;
};


} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PYUTILS_H
