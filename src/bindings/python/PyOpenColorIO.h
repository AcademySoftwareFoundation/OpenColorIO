// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYOPENCOLORIO_H
#define INCLUDED_OCIO_PYOPENCOLORIO_H

#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <OpenColorIO/OpenColorIO.h>

namespace py = pybind11;
using namespace pybind11::literals;

OCIO_NAMESPACE_ENTER
{

// Many OCIO classes are strictly reference counted shared pointers, which lack
// a public destructor. This prevents their compatibility with pybind11's 
// standard holder types. To get around this, a thin wrapper class is used to 
// bridge OCIO's internal reference counting with Python. pybind11 maintains
// a unique_ptr to the OCIO wrapper instance, which owns a reference to the 
// underlying OCIO_SHARED_PTR (*RcPtr).
//
// To prevent these wrapper classes from breaking visibility into OCIO's class 
// hierarchy, a dummy base class is provided to give the wrappers a matching
// inheritance structure to the underlying classes. This permits support for 
// Python's `isinstance()` on the wrapper and base classes for example.
template<typename T>
class PyBase
{
public:
    PyBase() {}
    virtual ~PyBase() {}
};

template<typename T, typename B = T>
class PyPtr : public PyBase<B>
{
public:
    PyPtr() : PyBase<B>(), m_rcptr(T::Create()) {}
    OCIO_SHARED_PTR<T> getRcPtr() const { return m_rcptr; }

private:
    OCIO_SHARED_PTR<T> m_rcptr;
};

// Core
typedef PyPtr<Config> PyConfig;
typedef PyPtr<ColorSpace> PyColorSpace;
typedef PyPtr<ColorSpaceSet> PyColorSpaceSet;
typedef PyPtr<Look> PyLook;
typedef PyPtr<Context> PyContext;
typedef PyPtr<Processor> PyProcessor;
typedef PyPtr<CPUProcessor> PyCPUProcessor;
typedef PyPtr<GPUProcessor> PyGPUProcessor;
typedef PyPtr<ProcessorMetadata> PyProcessorMetadata;
typedef PyPtr<Baker> PyBaker;
typedef PyPtr<ImageDesc> PyImageDesc;
typedef PyPtr<GpuShaderDesc> PyGpuShaderDesc;

// Transforms
typedef PyBase<Transform> PyTransform;
typedef PyPtr<AllocationTransform, Transform> PyAllocationTransform;
typedef PyPtr<CDLTransform, Transform> PyCDLTransform;
typedef PyPtr<ColorSpaceTransform, Transform> PyColorSpaceTransform;
typedef PyPtr<DisplayTransform, Transform> PyDisplayTransform;
typedef PyPtr<ExponentTransform, Transform> PyExponentTransform;
typedef PyPtr<ExponentWithLinearTransform, Transform> PyExponentWithLinearTransform;
typedef PyPtr<ExposureContrastTransform, Transform> PyExposureContrastTransform;
typedef PyPtr<FileTransform, Transform> PyFileTransform;
typedef PyPtr<FixedFunctionTransform, Transform> PyFixedFunctionTransform;
typedef PyPtr<GroupTransform, Transform> PyGroupTransform;
typedef PyPtr<LogAffineTransform, Transform> PyLogAffineTransform;
typedef PyPtr<LogTransform, Transform> PyLogTransform;
typedef PyPtr<LookTransform, Transform> PyLookTransform;
typedef PyPtr<MatrixTransform, Transform> PyMatrixTransform;
typedef PyPtr<RangeTransform, Transform> PyRangeTransform;

void bindPyStatic(py::module & m);
void bindPyTransform(py::module & m);

}
OCIO_NAMESPACE_EXIT

#endif
