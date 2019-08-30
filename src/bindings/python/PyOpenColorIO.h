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

// Wrapper for OCIO shared pointer.
//
// Each instance wraps an editable OR constant OCIO object reference. Constness 
// at construction is enforced for the wrapper's lifetime.
template<typename T>
class PyOCIOObject
{
private:
    bool m_isConst;
    OCIO_SHARED_PTR<T> m_rcPtr;
    OCIO_SHARED_PTR<const T> m_constRcPtr;

    PyOCIOObject() = default;

protected:
    // Check that the wrapper has a valid OCIO object reference. Throws 
    // OCIO::Exception if it doesn't. Called prior to pointer access via 
    // public interface.
    void checkPtr(bool checkEditable=false) const
    {
        if((!m_isConst && !m_rcPtr) || (m_isConst && !m_constRcPtr))
        {
            throw Exception("OCIO object is invalid");
        }
        if(checkEditable && m_isConst)
        {
            throw Exception("OCIO object is not editable");
        }
    }

public:
    PyOCIOObject(OCIO_SHARED_PTR<T> ptr) 
        : m_isConst(false)
        , m_rcPtr(ptr)
    { 
        checkPtr(true);
    }

    PyOCIOObject(OCIO_SHARED_PTR<const T> ptr) 
        : m_isConst(true)
        , m_constRcPtr(ptr)
    { 
        checkPtr();
    }

    PyOCIOObject(const PyOCIOObject<T> & rhs)
    {
        if(this != &rhs)
        {
            m_isConst = rhs.m_isConst;
            m_rcPtr = rhs.m_rcPtr;
            m_constRcPtr = rhs.m_constRcPtr;
        }
    }

    // Prevent constuction from uninitialized pointer.
    PyOCIOObject(nullptr_t) = delete;

    virtual ~PyOCIOObject() {}

    PyOCIOObject<T>& operator= (const PyOCIOObject<T> & rhs)
    {
        if (this != &rhs)
        {
            m_isConst = rhs.m_isConst;
            m_rcPtr = rhs.m_rcPtr;
            m_constRcPtr = rhs.m_constRcPtr;
        }
        return *this;
    }

    bool isValid() const
    {
        return ((!m_isConst && m_rcPtr) || (m_isConst && m_constRcPtr));
    }

    bool isEditable() const
    {
        return (isValid() && !m_isConst);
    }

    // Get editable OCIO object. Throws OCIO::Exception if the pointer is 
    // invalid or not editable.
    OCIO_SHARED_PTR<T> getRcPtr() const
    {
        checkPtr(true);
        return m_rcPtr; 
    }

    // Get immutable OCIO object. Throws OCIO::Exception if the pointer is 
    // invalid.
    OCIO_SHARED_PTR<const T> getConstRcPtr() const
    {
        checkPtr();
        return (m_isConst ? m_constRcPtr : m_rcPtr); 
    }

    // Implementation of Python __repr__.
    const std::string repr() const
    {
        std::ostringstream os;
        os << (*(m_isConst ? m_constRcPtr : m_rcPtr));
        return os.str();
    }
};

// Wrapper for OCIO::Transform derived shared pointer.
template<typename T>
class PyOCIOTransform : public PyOCIOObject<Transform>
{
public:
    // Create new editable OCIO object and wrapper.
    PyOCIOTransform()
        : PyOCIOObject<Transform>(DynamicPtrCast<Transform>(T::Create()))
    {}

    PyOCIOTransform(OCIO_SHARED_PTR<T> ptr) 
        : PyOCIOObject<Transform>(DynamicPtrCast<Transform>(ptr))
    {}

    PyOCIOTransform(OCIO_SHARED_PTR<const T> ptr) 
        : PyOCIOObject<Transform>(DynamicPtrCast<const Transform>(ptr))
    {}

    PyOCIOTransform(const PyOCIOTransform<T> & rhs)
        : PyOCIOObject<Transform>(rhs)
    {}

    PyOCIOTransform(nullptr_t) = delete;

    PyOCIOTransform<T>& operator= (const PyOCIOTransform<T> & rhs)
    {
        PyOCIOObject<Transform>::operator=(rhs);
        return *this;
    }

    // Get editable OCIO::Transform derived object. Throws OCIO::Exception if 
    // the pointer is invalid or not editable.
    OCIO_SHARED_PTR<T> getTransformRcPtr() const
    {
        return DynamicPtrCast<T>(getRcPtr());
    }

    // Get immutable OCIO::Transform derived object. Throws OCIO::Exception if 
    // the pointer is invalid.
    OCIO_SHARED_PTR<const T> getConstTransformRcPtr() const
    {
        return DynamicPtrCast<const T>(getConstRcPtr());
    }

    // Create editable copy of OCIO object and wrapper.
    PyOCIOTransform<T> createEditableCopy() const
    {
        return PyOCIOTransform<T>(
            DynamicPtrCast<T>(getConstRcPtr()->createEditableCopy()));
    }

    void validate() const
    {
        getConstRcPtr()->validate();
    }

    TransformDirection getDirection() const
    {
        return getConstRcPtr()->getDirection();
    }

    void setDirection(TransformDirection direction)
    {
        getRcPtr()->setDirection(direction);
    }
};

// Core wrapper types
typedef PyOCIOObject<Config> PyConfig;
typedef PyOCIOObject<ColorSpace> PyColorSpace;
typedef PyOCIOObject<ColorSpaceSet> PyColorSpaceSet;
typedef PyOCIOObject<Look> PyLook;
typedef PyOCIOObject<Context> PyContext;
typedef PyOCIOObject<Processor> PyProcessor;
typedef PyOCIOObject<CPUProcessor> PyCPUProcessor;
typedef PyOCIOObject<GPUProcessor> PyGPUProcessor;
typedef PyOCIOObject<ProcessorMetadata> PyProcessorMetadata;
typedef PyOCIOObject<Baker> PyBaker;
typedef PyOCIOObject<ImageDesc> PyImageDesc;
typedef PyOCIOObject<GpuShaderDesc> PyGpuShaderDesc;
typedef PyOCIOObject<Transform> PyTransform;

// Transform wrapper types
typedef PyOCIOTransform<AllocationTransform> PyAllocationTransform;
typedef PyOCIOTransform<CDLTransform> PyCDLTransform;
typedef PyOCIOTransform<ColorSpaceTransform> PyColorSpaceTransform;
typedef PyOCIOTransform<DisplayTransform> PyDisplayTransform;
typedef PyOCIOTransform<ExponentTransform> PyExponentTransform;
typedef PyOCIOTransform<ExponentWithLinearTransform> PyExponentWithLinearTransform;
typedef PyOCIOTransform<ExposureContrastTransform> PyExposureContrastTransform;
typedef PyOCIOTransform<FileTransform> PyFileTransform;
typedef PyOCIOTransform<FixedFunctionTransform> PyFixedFunctionTransform;
typedef PyOCIOTransform<GroupTransform> PyGroupTransform;
typedef PyOCIOTransform<LogAffineTransform> PyLogAffineTransform;
typedef PyOCIOTransform<LogTransform> PyLogTransform;
typedef PyOCIOTransform<LookTransform> PyLookTransform;
typedef PyOCIOTransform<MatrixTransform> PyMatrixTransform;
typedef PyOCIOTransform<RangeTransform> PyRangeTransform;

// pybind module binding declarations
void bindPyStatic(py::module & m);
void bindPyTransform(py::module & m);

}
OCIO_NAMESPACE_EXIT

namespace OCIO = OCIO_NAMESPACE;

namespace pybind11 {

// Automatic PyTransform downcaster
template <>
struct polymorphic_type_hook<OCIO::PyTransform>
{
    static const void *get(const OCIO::PyTransform *src, const std::type_info*& type)
    {
        if(static_cast<const OCIO::PyAllocationTransform*>(src))
        {
            type = &typeid(OCIO::PyAllocationTransform);
        }
        else if(static_cast<const OCIO::PyCDLTransform*>(src))
        {
            type = &typeid(OCIO::PyCDLTransform);
        }
        else if(static_cast<const OCIO::PyColorSpaceTransform*>(src))
        {
            type = &typeid(OCIO::PyColorSpaceTransform);
        }
        else if(static_cast<const OCIO::PyDisplayTransform*>(src))
        {
            type = &typeid(OCIO::PyDisplayTransform);
        }
        else if(static_cast<const OCIO::PyExponentTransform*>(src))
        {
            type = &typeid(OCIO::PyExponentTransform);
        }
        else if(static_cast<const OCIO::PyExponentWithLinearTransform*>(src))
        {
            type = &typeid(OCIO::PyExponentWithLinearTransform);
        }
        else if(static_cast<const OCIO::PyExposureContrastTransform*>(src))
        {
            type = &typeid(OCIO::PyExposureContrastTransform);
        }
        else if(static_cast<const OCIO::PyFileTransform*>(src))
        {
            type = &typeid(OCIO::PyFileTransform);
        }
        else if(static_cast<const OCIO::PyFixedFunctionTransform*>(src))
        {
            type = &typeid(OCIO::PyFixedFunctionTransform);
        }
        else if(static_cast<const OCIO::PyGroupTransform*>(src))
        {
            type = &typeid(OCIO::PyGroupTransform);
        }
        else if(static_cast<const OCIO::PyLogAffineTransform*>(src))
        {
            type = &typeid(OCIO::PyLogAffineTransform);
        }
        else if(static_cast<const OCIO::PyLogTransform*>(src))
        {
            type = &typeid(OCIO::PyLogTransform);
        }
        else if(static_cast<const OCIO::PyLookTransform*>(src))
        {
            type = &typeid(OCIO::PyLookTransform);
        }
        else if(static_cast<const OCIO::PyMatrixTransform*>(src))
        {
            type = &typeid(OCIO::PyMatrixTransform);
        }
        else if(static_cast<const OCIO::PyRangeTransform*>(src))
        {
            type = &typeid(OCIO::PyRangeTransform);
        }
        return src;
    }
};

}  // pybind11

#endif  // INCLUDED_OCIO_PYOPENCOLORIO_H
