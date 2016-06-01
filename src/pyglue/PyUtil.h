/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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

#ifndef INCLUDED_PYOCIO_PYUTIL_H
#define INCLUDED_PYOCIO_PYUTIL_H

#include <PyOpenColorIO/PyOpenColorIO.h>

#include <vector>
#include <map>
#include <iostream>

#define OCIO_PYTRY_ENTER() try {
#define OCIO_PYTRY_EXIT(ret) } catch(...) { OCIO_NAMESPACE::Python_Handle_Exception(); return ret; }
#define OCIO_STRINGIFY(str) OCIO_STRINGIFY_IMPL(str)
#define OCIO_STRINGIFY_IMPL(str) #str
#define OCIO_PYTHON_NAMESPACE(obj) OCIO_STRINGIFY(PYOCIO_NAME) "." #obj

// Some utilities macros for python 2.5 to 3.3 compatibility
#if PY_MAJOR_VERSION >= 3
#define PyString_FromString PyUnicode_FromString
#define PyString_AsString   PyUnicode_AsUTF8
#define PyString_AS_STRING  PyUnicode_AsUTF8
#define PyString_Check      PyUnicode_Check
#define PyInt_Check         PyLong_Check
#define PyInt_AS_LONG       PyLong_AS_LONG
#define PyInt_FromLong      PyLong_FromLong
#define PyNumber_Int        PyNumber_Long
#endif

#if PY_MAJOR_VERSION >= 3
  #define MOD_ERROR_VAL NULL
  #define MOD_SUCCESS_VAL(val) val
  #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
  #define MOD_DEF(ob, name, doc, methods) \
          static struct PyModuleDef moduledef = { \
            PyModuleDef_HEAD_INIT, name, doc, -1, methods, NULL, NULL, NULL, NULL}; \
          ob = PyModule_Create(&moduledef);
#else
  #define MOD_ERROR_VAL
  #define MOD_SUCCESS_VAL(val)
  #define MOD_INIT(name) PyMODINIT_FUNC init##name(void)
  #define MOD_DEF(ob, name, doc, methods) \
          ob = Py_InitModule3(name, methods, doc);
#endif

#ifndef PyVarObject_HEAD_INIT
    #define PyVarObject_HEAD_INIT(type, size) \
        PyObject_HEAD_INIT(type) size,
#endif

#ifndef Py_TYPE
    #define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif

OCIO_NAMESPACE_ENTER
{
    
    typedef OCIO_SHARED_PTR<const GpuShaderDesc> ConstGpuShaderDescRcPtr;
    typedef OCIO_SHARED_PTR<GpuShaderDesc> GpuShaderDescRcPtr;
    //typedef OCIO_SHARED_PTR<const ImageDesc> ConstImageDescRcPtr;
    //typedef OCIO_SHARED_PTR<ImageDesc> ImageDescRcPtr;
    //typedef OCIO_SHARED_PTR<const PackedImageDesc> ConstPackedImageDescRcPtr;
    //typedef OCIO_SHARED_PTR<PackedImageDesc> PackedImageDescRcPtr;
    //typedef OCIO_SHARED_PTR<const PlanarImageDesc> ConstPlanarImageDescRcPtr;
    //typedef OCIO_SHARED_PTR<PlanarImageDesc> PlanarImageDescRcPtr;
    
    ///////////////////////////////////////////////////////////////////////////
    
    template<typename C, typename E>
    struct PyOCIOObject
    {
        PyObject_HEAD
        C * constcppobj;
        E * cppobj;
        bool isconst;
    };
    
    typedef PyOCIOObject <ConstConfigRcPtr, ConfigRcPtr> PyOCIO_Config;
    extern PyTypeObject PyOCIO_ConfigType;
    
    typedef PyOCIOObject <ConstContextRcPtr, ContextRcPtr> PyOCIO_Context;
    extern PyTypeObject PyOCIO_ContextType;
    
    typedef PyOCIOObject <ConstColorSpaceRcPtr, ColorSpaceRcPtr> PyOCIO_ColorSpace;
    extern PyTypeObject PyOCIO_ColorSpaceType;
    
    typedef PyOCIOObject <ConstLookRcPtr, LookRcPtr> PyOCIO_Look;
    extern PyTypeObject PyOCIO_LookType;
    
    typedef PyOCIOObject <ConstProcessorRcPtr, ProcessorRcPtr> PyOCIO_Processor;
    extern PyTypeObject PyOCIO_ProcessorType;
    
    typedef PyOCIOObject <ConstProcessorMetadataRcPtr, ProcessorMetadataRcPtr> PyOCIO_ProcessorMetadata;
    extern PyTypeObject PyOCIO_ProcessorMetadataType;
    
    typedef PyOCIOObject <ConstGpuShaderDescRcPtr, GpuShaderDescRcPtr> PyOCIO_GpuShaderDesc;
    extern PyTypeObject PyOCIO_GpuShaderDescType;
    
    typedef PyOCIOObject <ConstBakerRcPtr, BakerRcPtr> PyOCIO_Baker;
    extern PyTypeObject PyOCIO_BakerType;
    
    typedef PyOCIOObject <ConstTransformRcPtr, TransformRcPtr> PyOCIO_Transform;
    extern PyTypeObject PyOCIO_TransformType;
    
    ///////////////////////////////////////////////////////////////////////////
    
    extern PyTypeObject PyOCIO_AllocationTransformType;
    extern PyTypeObject PyOCIO_CDLTransformType;
    extern PyTypeObject PyOCIO_ClampTransformType;
    extern PyTypeObject PyOCIO_ColorSpaceTransformType;
    extern PyTypeObject PyOCIO_DisplayTransformType;
    extern PyTypeObject PyOCIO_ExponentTransformType;
    extern PyTypeObject PyOCIO_FileTransformType;
    extern PyTypeObject PyOCIO_GroupTransformType;
    extern PyTypeObject PyOCIO_LogTransformType;
    extern PyTypeObject PyOCIO_LookTransformType;
    extern PyTypeObject PyOCIO_MatrixTransformType;
    
    ///////////////////////////////////////////////////////////////////////////
    
    ConstGpuShaderDescRcPtr GetConstGpuShaderDesc(PyObject * pyobject);
    
    ///////////////////////////////////////////////////////////////////////////
    
    template<typename P, typename T, typename C>
    inline PyObject * BuildConstPyOCIO(C ptr, PyTypeObject& type)
    {
        if(!ptr) return Py_INCREF(Py_None), Py_None;
        P * obj = (P *)_PyObject_New(&type);
        obj->constcppobj = new C ();
        *obj->constcppobj = ptr;
        obj->cppobj = new T ();
        obj->isconst = true;
        return ( PyObject * ) obj;
    }
    
    template<typename P, typename T, typename C>
    inline PyObject * BuildEditablePyOCIO(T ptr, PyTypeObject& type)
    {
        if(!ptr) return Py_INCREF(Py_None), Py_None;
        P * obj = (P *) _PyObject_New(&type);
        obj->constcppobj = new C ();
        obj->cppobj = new T ();
        *obj->cppobj = ptr;
        obj->isconst = false;
        return ( PyObject * ) obj;
    }
    
    template<typename P, typename C, typename T>
    inline int BuildPyObject(P * self, T ptr)
    {
        self->constcppobj = new C ();
        self->cppobj = new T ();
        *self->cppobj = ptr;
        self->isconst = false;
        return 0;
    }
    
    template<typename T>
    inline int BuildPyTransformObject(PyOCIO_Transform* self, T ptr)
    {
        self->constcppobj = new ConstTransformRcPtr();
        self->cppobj = new TransformRcPtr();
        *self->cppobj = ptr;
        self->isconst = false;
        return 0;
    }
    
    template<typename T>
    void DeletePyObject(T * self)
    {
        if(self->constcppobj != NULL) delete self->constcppobj;
        if(self->cppobj != NULL) delete self->cppobj;
        Py_TYPE(self)->tp_free((PyObject*)self);
    }
    
    inline bool IsPyOCIOType(PyObject* pyobject, PyTypeObject& type)
    {
        if(!pyobject) return false;
        return PyObject_TypeCheck(pyobject, &type);
    }
    
    template<typename T>
    inline bool IsPyEditable(PyObject * pyobject, PyTypeObject& type)
    {
        if(!IsPyOCIOType(pyobject, type)) return false;
        T * pyobj = reinterpret_cast<T *> (pyobject);
        return (!pyobj->isconst);
    }
    
    template<typename P, typename C>
    inline C GetConstPyOCIO(PyObject* pyobject, PyTypeObject& type, bool allowCast = true)
    {
        if(!IsPyOCIOType(pyobject, type))
            throw Exception("PyObject must be an OCIO type");
        P * ptr = reinterpret_cast<P *> (pyobject);
        if(ptr->isconst && ptr->constcppobj)
            return *ptr->constcppobj;
        if(allowCast && !ptr->isconst && ptr->cppobj)
            return *ptr->cppobj;
        throw Exception("PyObject must be a valid OCIO type");
    }
    
    template<typename P, typename C, typename T>
    inline C GetConstPyOCIO(PyObject* pyobject, PyTypeObject& type, bool allowCast = true)
    {
        if(!IsPyOCIOType(pyobject, type))
            throw Exception("PyObject must be an OCIO type");
        P * ptr = reinterpret_cast<P *> (pyobject);
        C cptr;
        if(ptr->isconst && ptr->constcppobj)
            cptr = DynamicPtrCast<const T>(*ptr->constcppobj);
        if(allowCast && !ptr->isconst && ptr->cppobj)
            cptr = DynamicPtrCast<const T>(*ptr->cppobj);
        if(!cptr) throw Exception("PyObject must be a valid OCIO type");
        return cptr;
    }
    
    template<typename P, typename C>
    inline C GetEditablePyOCIO(PyObject* pyobject, PyTypeObject& type)
    {
        if(!IsPyOCIOType(pyobject, type))
            throw Exception("PyObject must be an OCIO type");
        P * ptr = reinterpret_cast<P *> (pyobject);
        if(!ptr->isconst && ptr->cppobj)
            return *ptr->cppobj;
        throw Exception("PyObject must be a editable OCIO type");
    }
    
    template<typename P, typename C, typename T>
    inline C GetEditablePyOCIO(PyObject* pyobject, PyTypeObject& type)
    {
        if(!IsPyOCIOType(pyobject, type))
            throw Exception("PyObject must be an OCIO type");
        P * ptr = reinterpret_cast<P *> (pyobject);
        C cptr;
        if(!ptr->isconst && ptr->cppobj)
            cptr = DynamicPtrCast<T>(*ptr->cppobj);
        if(!cptr) throw Exception("PyObject must be a editable OCIO type");
        return cptr;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    int ConvertPyObjectToBool(PyObject *object, void *valuePtr);
    int ConvertPyObjectToAllocation(PyObject *object, void *valuePtr);
    int ConvertPyObjectToInterpolation(PyObject *object, void *valuePtr);
    int ConvertPyObjectToTransformDirection(PyObject *object, void *valuePtr);
    int ConvertPyObjectToColorSpaceDirection(PyObject *object, void *valuePtr);
    int ConvertPyObjectToGpuLanguage(PyObject *object, void *valuePtr);
    int ConvertPyObjectToEnvironmentMode(PyObject *object, void *valuePtr);
    
    ///////////////////////////////////////////////////////////////////////////
    
    // Generics.
    // None of these allow an interpreter error to leak
    
    //! Use a variety of methods to get the value, as the specified type
    //! from the specified PyObject. (Return true on success, false on failure)
    //!
    //! 1. See if object is PyFloat, return value
    //! 2. See if object is PyInt, return value
    //! 3. Attempt to cast to PyFloat / PyInt (equivalent to calling float(obj)/int(obj) in python)
    
    bool GetIntFromPyObject(PyObject* object, int* val);
    bool GetFloatFromPyObject(PyObject* object, float* val);
    bool GetDoubleFromPyObject(PyObject* object, double* val);
    
    //! 1. See if object is a PyString, return value
    //! 2. Attempt to cast to a PyString (equivalent to calling str(obj) in python
    //! Note: This will basically always succeed, even if the object is not string-like
    //! (such as passing Py_None as val), so you cannot use this to check str type.
    
    bool GetStringFromPyObject(PyObject* object, std::string* val);
    
    // Can return a null pointer if PyList_New(size) fails.
    PyObject* CreatePyListFromIntVector(const std::vector<int> &data);
    PyObject* CreatePyListFromFloatVector(const std::vector<float> &data);
    PyObject* CreatePyListFromDoubleVector(const std::vector<double> &data);
    PyObject* CreatePyListFromStringVector(const std::vector<std::string> &data);
    PyObject* CreatePyListFromTransformVector(const std::vector<ConstTransformRcPtr> &data);
    PyObject* CreatePyDictFromStringMap(const std::map<std::string, std::string> &data);
    
    //! Fill the specified vector type from the given pyobject
    //! Return true on success, false on failure.
    //! The PyObject must be a tuple or list, filled with the appropriate data types
    //! (either PyInt, PyFloat, or something convertible to one)
    
    bool FillIntVectorFromPySequence(PyObject* datalist, std::vector<int> &data);
    bool FillFloatVectorFromPySequence(PyObject* datalist, std::vector<float> &data);
    bool FillDoubleVectorFromPySequence(PyObject* datalist, std::vector<double> &data);
    bool FillStringVectorFromPySequence(PyObject* datalist, std::vector<std::string> &data);
    bool FillTransformVectorFromPySequence(PyObject* datalist, std::vector<ConstTransformRcPtr> &data);
    
    ///////////////////////////////////////////////////////////////////////////
    
    void Python_Handle_Exception();
    
}
OCIO_NAMESPACE_EXIT

#endif
