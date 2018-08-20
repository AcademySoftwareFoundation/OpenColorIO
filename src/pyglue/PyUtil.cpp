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

#include <Python.h>
#include <sstream>
#include <OpenColorIO/OpenColorIO.h>

#include "PyUtil.h"

OCIO_NAMESPACE_ENTER
{
    
    ///////////////////////////////////////////////////////////////////////////
    
    // http://docs.python.org/c-api/object.html#PyObject_IsTrue
    int ConvertPyObjectToBool(PyObject *object, void *valuePtr)
    {
        bool *boolPtr = static_cast<bool*>(valuePtr);
        int status = PyObject_IsTrue(object);
        
        if (status == -1 || PyErr_Occurred())
        {
            if (!PyErr_Occurred())
            {
                PyErr_SetString(PyExc_ValueError, "could not convert object to bool.");
            }
            
            return 0;
        }

        *boolPtr = (status == 1) ? true : false;

        return 1;
    }
    
    int ConvertPyObjectToAllocation(PyObject *object, void *valuePtr)
    {
        Allocation* allocPtr = static_cast<Allocation*>(valuePtr);
        
        if(!PyString_Check(object))
        {
            PyErr_SetString(PyExc_ValueError, "Object is not a string.");
            return 0;
        }
        
        *allocPtr = AllocationFromString(PyString_AsString( object ));
        
        return 1;
    }
    
    
    
    int ConvertPyObjectToInterpolation(PyObject *object, void *valuePtr)
    {
        Interpolation* interpPtr = static_cast<Interpolation*>(valuePtr);
        
        if(!PyString_Check(object))
        {
            PyErr_SetString(PyExc_ValueError, "Object is not a string.");
            return 0;
        }
        
        *interpPtr = InterpolationFromString(PyString_AsString( object ));
        
        return 1;
    }
    
    int ConvertPyObjectToTransformDirection(PyObject *object, void *valuePtr)
    {
        TransformDirection* dirPtr = static_cast<TransformDirection*>(valuePtr);
        
        if(!PyString_Check(object))
        {
            PyErr_SetString(PyExc_ValueError, "Object is not a string.");
            return 0;
        }
        
        *dirPtr = TransformDirectionFromString(PyString_AsString( object ));
        
        return 1;
    }
    
    
    int ConvertPyObjectToColorSpaceDirection(PyObject *object, void *valuePtr)
    {
        ColorSpaceDirection* dirPtr = static_cast<ColorSpaceDirection*>(valuePtr);
        
        if(!PyString_Check(object))
        {
            PyErr_SetString(PyExc_ValueError, "Object is not a string.");
            return 0;
        }
        
        *dirPtr = ColorSpaceDirectionFromString(PyString_AsString( object ));
        
        return 1;
    }
    
    
    int ConvertPyObjectToGpuLanguage(PyObject *object, void *valuePtr)
    {
        GpuLanguage* gpuLanguagePtr = static_cast<GpuLanguage*>(valuePtr);
        
        if(!PyString_Check(object))
        {
            PyErr_SetString(PyExc_ValueError, "Object is not a string.");
            return 0;
        }
        
        *gpuLanguagePtr = GpuLanguageFromString(PyString_AsString( object ));
        
        return 1;
    }
    
    int ConvertPyObjectToEnvironmentMode(PyObject *object, void *valuePtr)
    {
        EnvironmentMode* environmentmodePtr = static_cast<EnvironmentMode*>(valuePtr);
        
        if(!PyString_Check(object))
        {
            PyErr_SetString(PyExc_ValueError, "Object is not a string.");
            return 0;
        }
        
        *environmentmodePtr = EnvironmentModeFromString(PyString_AsString( object ));
        
        return 1;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    bool GetIntFromPyObject(PyObject* object, int* val)
    {
        if(!val || !object) return false;
        
        if( PyInt_Check( object ) )
        {
            *val = static_cast<int>( PyInt_AS_LONG( object ) );
            return true;
        }
        
        if( PyFloat_Check( object ) )
        {
            *val = static_cast<int>( PyFloat_AS_DOUBLE( object ) );
            return true;
        }
        
        PyObject* intObject = PyNumber_Int(object);
        if(intObject)
        {
            *val = static_cast<int>( PyInt_AS_LONG( intObject ) );
            Py_DECREF(intObject);
            return true;
        }
        
        PyErr_Clear();
        return false;
    }
    
    bool GetFloatFromPyObject(PyObject* object, float* val)
    {
        if(!val || !object) return false;
        
        if( PyFloat_Check( object ) )
        {
            *val = static_cast<float>( PyFloat_AS_DOUBLE( object ) );
            return true;
        }
        
        if( PyInt_Check( object ) )
        {
            *val = static_cast<float>( PyInt_AS_LONG( object ) );
            return true;
        }
        
        PyObject* floatObject = PyNumber_Float(object);
        if(floatObject)
        {
            *val = static_cast<float>( PyFloat_AS_DOUBLE( floatObject ) );
            Py_DECREF(floatObject);
            return true;
        }
        
        PyErr_Clear();
        return false;
    }
    
    bool GetDoubleFromPyObject(PyObject* object, double* val)
    {
        if(!val || !object) return false;
        
        if( PyFloat_Check( object ) )
        {
            *val = PyFloat_AS_DOUBLE( object );
            return true;
        }
        
        if( PyInt_Check( object ) )
        {
            *val = static_cast<double>( PyInt_AS_LONG( object ) );
            return true;
        }
        
        PyObject* floatObject = PyNumber_Float(object);
        if(floatObject)
        {
            *val = PyFloat_AS_DOUBLE( floatObject );
            Py_DECREF(floatObject);
            return true;
        }
        
        PyErr_Clear();
        return false;
    }
    
    bool GetStringFromPyObject(PyObject* object, std::string* val)
    {
        if(!val || !object) return false;
        
        if( PyString_Check( object ) )
        {
            *val = std::string(PyString_AS_STRING(object));
            return true;
        }
        
        PyObject* strObject = PyObject_Str(object);
        if(strObject)
        {
            *val = std::string(PyString_AS_STRING(strObject));
            Py_DECREF(strObject);
            return true;
        }
        
        PyErr_Clear();
        return false;
    }
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    PyObject* CreatePyListFromIntVector(const std::vector<int> &data)
    {
        PyObject* returnlist = PyList_New( data.size() );
        if(!returnlist) return 0;
        
        for(unsigned int i =0; i<data.size(); ++i)
        {
            PyList_SET_ITEM(returnlist, i, PyInt_FromLong(data[i]));
        }
        
        return returnlist;
    }
    
    PyObject* CreatePyListFromFloatVector(const std::vector<float> &data)
    {
        PyObject* returnlist = PyList_New( data.size() );
        if(!returnlist) return 0;
        
        for(unsigned int i =0; i<data.size(); ++i)
        {
            PyList_SET_ITEM(returnlist, i, PyFloat_FromDouble(data[i]));
        }
        
        return returnlist;
    }
    
    PyObject* CreatePyListFromDoubleVector(const std::vector<double> &data)
    {
        PyObject* returnlist = PyList_New( data.size() );
        if(!returnlist) return 0;
        
        for(unsigned int i =0; i<data.size(); ++i)
        {
            PyList_SET_ITEM(returnlist, i, PyFloat_FromDouble(data[i]));
        }
        
        return returnlist;
    }
    
    PyObject* CreatePyListFromStringVector(const std::vector<std::string> &data)
    {
        PyObject* returnlist = PyList_New( data.size() );
        if(!returnlist) return 0;
        
        for(unsigned int i =0; i<data.size(); ++i)
        {
            PyObject *str =  PyString_FromString(data[i].c_str());
            if (str == NULL)
            {
                Py_DECREF(returnlist);
                return NULL;
            }
            PyList_SET_ITEM(returnlist, i, str);
        }
        
        return returnlist;
    }
    
    PyObject* CreatePyListFromTransformVector(const std::vector<ConstTransformRcPtr> &data)
    {
        PyObject* returnlist = PyList_New( data.size() );
        if(!returnlist) return 0;
        
        for(unsigned int i =0; i<data.size(); ++i)
        {
            PyList_SET_ITEM(returnlist, i, BuildConstPyTransform(data[i]));
        }
        
        return returnlist;
    }
    
    PyObject* CreatePyDictFromStringMap(const std::map<std::string, std::string> &data)
    {
        PyObject* returndict = PyDict_New();
        if(!returndict) return 0;
        
        std::map<std::string, std::string>::const_iterator iter;
        for(iter = data.begin(); iter != data.end(); ++iter)
        {
            int ret = PyDict_SetItem(returndict,
                PyString_FromString(iter->first.c_str()),
                PyString_FromString(iter->second.c_str()));
            if(ret)
            {
                Py_DECREF(returndict);
                return NULL;
            }
        }
        
        return returndict;
    }
    
    namespace
    {
        // These are safer than PySequence_Fast, as we can
        // Confirm that no exceptions will be set in the python runtime.
        
        inline bool PyListOrTuple_Check(PyObject* pyobj)
        {
            return (PyTuple_Check(pyobj) || PyList_Check(pyobj));
        }
        
        inline int PyListOrTuple_GET_SIZE(PyObject* pyobj)
        {
            if(PyList_Check(pyobj))
            {
                return static_cast<int>(PyList_GET_SIZE(pyobj));
            }
            else if(PyTuple_Check(pyobj))
            {
                return static_cast<int>(PyTuple_GET_SIZE(pyobj));
            }
            return -1;
        }
        
        // Return a boworrowed reference
        inline PyObject* PyListOrTuple_GET_ITEM(PyObject* pyobj, int index)
        {
            if(PyList_Check(pyobj))
            {
                return PyList_GET_ITEM(pyobj, index);
            }
            else if(PyTuple_Check(pyobj))
            {
                return PyTuple_GET_ITEM(pyobj, index);
            }
            return 0;
        }
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    /*
        A note on why PyErr_Clear is needed in multiple locations...
        
        Even though it's not immediately apparent, almost every function
        in the Abstract Objects Layer,
        http://www.python.org/doc/2.5/api/abstract.html
        can set a global exception under certain circumstances.
        
        For example, calling the equivalent of int( obj ) will set
        an exception if the object cannot be casted (such as None),
        or if it's a custom type that implements the number protocol
        but throws an exception during the cast.
        
        During iteration, even an object that implements the sequence
        protocol can raise an exception if the iteration fails.
        
        As we want to guarantee that an exception *never* remains on
        the stack after an internal failure, the simplest way to
        guarantee this is to always call PyErr_Clear() before
        returing the failure condition.
    */
    
    bool FillIntVectorFromPySequence(PyObject* datalist, std::vector<int> &data)
    {
        data.clear();
        
        // First, try list or tuple iteration (for speed).
        if(PyListOrTuple_Check(datalist))
        {
            int sequenceSize = PyListOrTuple_GET_SIZE(datalist);
            data.reserve(sequenceSize);
            
            for(int i=0; i < sequenceSize; i++)
            {
                PyObject* item = PyListOrTuple_GET_ITEM(datalist, i);
                
                int val;
                if (!GetIntFromPyObject(item, &val))
                {
                    data.clear();
                    return false;
                }
                data.push_back(val);
            }
            
            return true;
        }
        // As a fallback, try general iteration.
        else
        {
            PyObject *item;
            PyObject *iter = PyObject_GetIter(datalist);
            if (iter == NULL)
            {
                PyErr_Clear();
                return false;
            }
            while((item = PyIter_Next(iter)) != NULL)
            {
                int val;
                if (!GetIntFromPyObject(item, &val))
                {
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    
                    data.clear();
                    return false;
                }
                data.push_back(val);
                Py_DECREF(item);
            }
            
            Py_DECREF(iter);
            if (PyErr_Occurred())
            {
                PyErr_Clear();
                data.clear();
                return false;
            }
            return true;
        }
    }
    
    bool FillFloatVectorFromPySequence(PyObject* datalist, std::vector<float> &data)
    {
        data.clear();
        
        if(PyListOrTuple_Check(datalist))
        {
            int sequenceSize = PyListOrTuple_GET_SIZE(datalist);
            data.reserve(sequenceSize);
            
            for(int i=0; i < sequenceSize; i++)
            {
                PyObject* item = PyListOrTuple_GET_ITEM(datalist, i);
                
                float val;
                if (!GetFloatFromPyObject(item, &val))
                {
                    data.clear();
                    return false;
                }
                data.push_back(val);
            }
            return true;
        }
        else
        {
            PyObject *item;
            PyObject *iter = PyObject_GetIter(datalist);
            if (iter == NULL)
            {
                PyErr_Clear();
                return false;
            }
            while((item = PyIter_Next(iter)) != NULL)
            {
                float val;
                if (!GetFloatFromPyObject(item, &val))
                {
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    
                    data.clear();
                    return false;
                }
                data.push_back(val);
                Py_DECREF(item);
            }
            Py_DECREF(iter);
            if (PyErr_Occurred())
            {
                PyErr_Clear();
                data.clear();
                return false;
            }
            return true;
        }
    }
    
    bool FillDoubleVectorFromPySequence(PyObject* datalist, std::vector<double> &data)
    {
        data.clear();
        
        if(PyListOrTuple_Check(datalist))
        {
            int sequenceSize = PyListOrTuple_GET_SIZE(datalist);
            data.reserve(sequenceSize);
            
            for(int i=0; i < sequenceSize; i++)
            {
                PyObject* item = PyListOrTuple_GET_ITEM(datalist, i);
                double val;
                if (!GetDoubleFromPyObject(item, &val))
                {
                    data.clear();
                    return false;
                }
                data.push_back( val );
            }
            return true;
        }
        else
        {
            PyObject *item;
            PyObject *iter = PyObject_GetIter(datalist);
            if (iter == NULL)
            {
                PyErr_Clear();
                return false;
            }
            while((item = PyIter_Next(iter)) != NULL)
            {
                double val;
                if (!GetDoubleFromPyObject(item, &val))
                {
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    
                    data.clear();
                    return false;
                }
                data.push_back(val);
                Py_DECREF(item);
            }
            
            Py_DECREF(iter);
            if (PyErr_Occurred())
            {
                PyErr_Clear();
                data.clear();
                return false;
            }
            return true;
        }
    }
    
    
    bool FillStringVectorFromPySequence(PyObject* datalist, std::vector<std::string> &data)
    {
        data.clear();
        
        if(PyListOrTuple_Check(datalist))
        {
            int sequenceSize = PyListOrTuple_GET_SIZE(datalist);
            data.reserve(sequenceSize);
            
            for(int i=0; i < sequenceSize; i++)
            {
                PyObject* item = PyListOrTuple_GET_ITEM(datalist, i);
                std::string val;
                if (!GetStringFromPyObject(item, &val))
                {
                    data.clear();
                    return false;
                }
                data.push_back( val );
            }
            return true;
        }
        else
        {
            PyObject *item;
            PyObject *iter = PyObject_GetIter(datalist);
            if (iter == NULL)
            {
                PyErr_Clear();
                return false;
            }
            while((item = PyIter_Next(iter)) != NULL)
            {
                std::string val;
                if (!GetStringFromPyObject(item, &val))
                {
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    
                    data.clear();
                    return false;
                }
                data.push_back(val);
                Py_DECREF(item);
            }
            
            Py_DECREF(iter);
            if (PyErr_Occurred())
            {
                PyErr_Clear();
                data.clear();
                return false;
            }
            return true;
        }
    }
    
    
    
    bool FillTransformVectorFromPySequence(PyObject* datalist, std::vector<ConstTransformRcPtr> &data)
    {
        data.clear();
        
        if(PyListOrTuple_Check(datalist))
        {
            int sequenceSize = PyListOrTuple_GET_SIZE(datalist);
            data.reserve(sequenceSize);
            
            for(int i=0; i < sequenceSize; i++)
            {
                PyObject* item = PyListOrTuple_GET_ITEM(datalist, i);
                ConstTransformRcPtr val;
                try
                {
                    val = GetConstTransform(item, true);
                }
                catch(...)
                {
                    data.clear();
                    return false;
                }
                
                data.push_back( val );
            }
            return true;
        }
        else
        {
            PyObject *item;
            PyObject *iter = PyObject_GetIter(datalist);
            if (iter == NULL)
            {
                PyErr_Clear();
                return false;
            }
            while((item = PyIter_Next(iter)) != NULL)
            {
                ConstTransformRcPtr val;
                try
                {
                    val = GetConstTransform(item, true);
                }
                catch(...)
                {
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    
                    data.clear();
                    return false;
                }
                
                data.push_back(val);
                Py_DECREF(item);
            }
            
            Py_DECREF(iter);
            if (PyErr_Occurred())
            {
                PyErr_Clear();
                data.clear();
                return false;
            }
            
            return true;
        }
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    /* See the header for the justification for this function.
    
    The trick to making this technique work is that we know
    we've been called from within a catch block, so there
    is an exception on the stack.  We can re-throw this
    exception, using the throw statement.  By doing this
    inside a try...catch block, it's possible to use the
    standard catch mechanism to categorize whatever exception
    was actually caught by the caller.
    */
    
    void Python_Handle_Exception()
    {
        try
        {
            // Re-throw whatever exception is already on the stack.
            // This will fail horribly if no exception is already
            // on the stack, so this function must only be called
            // from inside an exception handler catch block!
            throw;
        }
        catch (ExceptionMissingFile & e)
        {
            PyErr_SetString(GetExceptionMissingFilePyType(), e.what());
        }
        catch (Exception & e)
        {
            PyErr_SetString(GetExceptionPyType(), e.what());
        }
        catch (std::exception& e)
        {
            PyErr_SetString(PyExc_RuntimeError, e.what());
        }
        catch (...)
        {
            PyErr_SetString(PyExc_RuntimeError, "Unknown C++ exception caught.");
        }
    }
    
}
OCIO_NAMESPACE_EXIT
