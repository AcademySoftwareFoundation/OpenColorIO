// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYOPENCOLORIO_H
#define INCLUDED_OCIO_PYOPENCOLORIO_H

#include <vector>
#include <typeinfo>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <OpenColorIO/OpenColorIO.h>

namespace py = pybind11;
using namespace pybind11::literals;

OCIO_NAMESPACE_ENTER
{

// Wrapper for OCIO shared pointer (RcPtr).
// Each instance wraps an editable OR constant OCIO object. Constness at 
// construction is enforced for the wrapper's lifetime.
template<typename T>
class PyOCIOBase
{
protected:
    bool m_isconst;
    OCIO_SHARED_PTR<T> m_eptr;
    OCIO_SHARED_PTR<const T> m_cptr;

    void checkPtr(bool checkEditable=false) const
    {
        if((!m_isconst && !m_eptr) || (m_isconst && !m_cptr))
        {
            throw Exception("OCIO object is invalid");
        }
        if(checkEditable && m_isconst)
        {
            throw Exception("OCIO object is not editable");
        }
    }

public:
    PyOCIOBase() : m_isconst(false), m_eptr(T()) {}
    PyOCIOBase(OCIO_SHARED_PTR<T> e) : m_isconst(false), m_eptr(e) { checkPtr(); }
    PyOCIOBase(OCIO_SHARED_PTR<const T> c) : m_isconst(true), m_cptr(c) { checkPtr(); }

    PyOCIOBase(nullptr_t) = delete;
    PyOCIOBase(const PyOCIOBase<T> &) = delete;
    PyOCIOBase<T> &operator=(const PyOCIOBase<T> &) = delete;

    virtual ~PyOCIOBase() {}

    virtual bool isConst() const { return m_isconst; }

    virtual OCIO_SHARED_PTR<T> getRcPtr() const 
    { 
        checkPtr(true);
        return m_eptr; 
    }
    
    virtual OCIO_SHARED_PTR<const T> getConstRcPtr() const 
    { 
        checkPtr();
        return (m_isconst ? m_cptr : m_eptr); 
    }

    virtual PyOCIOBase<T> createEditableCopy() const 
    {
        checkPtr();
        OCIO_SHARED_PTR<T> rcPtr = (m_isconst ? m_cptr : m_eptr)->createEditableCopy();
        return PyOCIOBase<T>(rcPtr);
    }

    virtual const std::string repr() const
    {
        std::ostringstream os;
        os << (*(m_isconst ? m_cptr : m_eptr));
        return os.str();
    }
};

template<typename T, typename B>
class PyOCIOPtr : public PyOCIOBase<B>
{
public:
    PyOCIOPtr() : m_isconst(false), m_eptr(T::Create()) {}
    PyOCIOPtr(OCIO_SHARED_PTR<T> e) : PyOCIOBase(e) {}
    PyOCIOPtr(OCIO_SHARED_PTR<const T> c) : PyOCIOBase(c) {}

    PyOCIOPtr(nullptr_t) = delete;
    PyOCIOPtr(const PyOCIOPtr<T> &) = delete;
    PyOCIOPtr<T> &operator=(const PyOCIOPtr<T> &) = delete;

    PyOCIOPtr<T> createEditableCopy() const override
    {
        checkPtr();
        OCIO_SHARED_PTR<T> rcPtr = (m_isconst ? m_cptr : m_eptr)->createEditableCopy();
        return PyOCIOPtr<T>(rcPtr);
    }
};

// Core
typedef PyOCIOPtr<Config> PyConfig;
typedef PyOCIOPtr<ColorSpace> PyColorSpace;
typedef PyOCIOPtr<ColorSpaceSet> PyColorSpaceSet;
typedef PyOCIOPtr<Look> PyLook;
typedef PyOCIOPtr<Context> PyContext;
typedef PyOCIOPtr<Processor> PyProcessor;
typedef PyOCIOPtr<CPUProcessor> PyCPUProcessor;
typedef PyOCIOPtr<GPUProcessor> PyGPUProcessor;
typedef PyOCIOPtr<ProcessorMetadata> PyProcessorMetadata;
typedef PyOCIOPtr<Baker> PyBaker;
typedef PyOCIOPtr<ImageDesc> PyImageDesc;
typedef PyOCIOPtr<GpuShaderDesc> PyGpuShaderDesc;

// Transforms
typedef PyOCIOBase<Transform> PyTransform;
typedef PyOCIOPtr<AllocationTransform, Transform> PyAllocationTransform;
typedef PyOCIOPtr<CDLTransform, Transform> PyCDLTransform;
typedef PyOCIOPtr<ColorSpaceTransform, Transform> PyColorSpaceTransform;
typedef PyOCIOPtr<DisplayTransform, Transform> PyDisplayTransform;
typedef PyOCIOPtr<ExponentTransform, Transform> PyExponentTransform;
typedef PyOCIOPtr<ExponentWithLinearTransform, Transform> PyExponentWithLinearTransform;
typedef PyOCIOPtr<ExposureContrastTransform, Transform> PyExposureContrastTransform;
typedef PyOCIOPtr<FileTransform, Transform> PyFileTransform;
typedef PyOCIOPtr<FixedFunctionTransform, Transform> PyFixedFunctionTransform;
typedef PyOCIOPtr<GroupTransform, Transform> PyGroupTransform;
typedef PyOCIOPtr<LogAffineTransform, Transform> PyLogAffineTransform;
typedef PyOCIOPtr<LogTransform, Transform> PyLogTransform;
typedef PyOCIOPtr<LookTransform, Transform> PyLookTransform;
typedef PyOCIOPtr<MatrixTransform, Transform> PyMatrixTransform;
typedef PyOCIOPtr<RangeTransform, Transform> PyRangeTransform;

void bindPyStatic(py::module & m);
void bindPyTransform(py::module & m);

}
OCIO_NAMESPACE_EXIT

#endif
