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

OCIO_NAMESPACE_ENTER
{
    
    int ConvertPyObjectToBool(PyObject *object, void *valuePtr);
    
    int ConvertPyObjectToAllocation(PyObject *object, void *valuePtr);
    
    int ConvertPyObjectToInterpolation(PyObject *object, void *valuePtr);
    
    int ConvertPyObjectToTransformDirection(PyObject *object, void *valuePtr);
    
    int ConvertPyObjectToColorSpaceDirection(PyObject *object, void *valuePtr);
    
    int ConvertPyObjectToGpuLanguage(PyObject *object, void *valuePtr);
    
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
