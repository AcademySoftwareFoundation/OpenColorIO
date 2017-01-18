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

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "PyUtil.h"
#include "PyDoc.h"

namespace
{
    
    PyObject * PyOCIO_ClearAllCaches(PyObject * /* self */)
    {
        OCIO_PYTRY_ENTER()
        OCIO::ClearAllCaches();
        Py_RETURN_NONE;
        OCIO_PYTRY_EXIT(NULL)
    }
    
    PyObject * PyOCIO_GetLoggingLevel(PyObject * /* self */)
    {
        OCIO_PYTRY_ENTER()
        return PyString_FromString(
            OCIO::LoggingLevelToString(OCIO::GetLoggingLevel()));
        OCIO_PYTRY_EXIT(NULL)
    }
    
    PyObject * PyOCIO_SetLoggingLevel(PyObject * /*self*/, PyObject * args)
    {
        OCIO_PYTRY_ENTER()
        PyObject* pylevel;
        if (!PyArg_ParseTuple(args, "O:SetLoggingLevel",
            &pylevel)) return NULL;
        // We explicitly cast to a str to handle both the str and int cases.
        PyObject* pystr = PyObject_Str(pylevel);
        if(!pystr) throw OCIO::Exception("Fist argument must be a LOGGING_LEVEL");
        OCIO::LoggingLevel level = OCIO::LoggingLevelFromString(PyString_AsString(pystr));
        OCIO::SetLoggingLevel(level);
        Py_DECREF(pystr);
        Py_RETURN_NONE;
        OCIO_PYTRY_EXIT(NULL)
    }
    
    PyObject * PyOCIO_GetCurrentConfig(PyObject * /* self */)
    {
        OCIO_PYTRY_ENTER()
        return OCIO::BuildConstPyConfig(OCIO::GetCurrentConfig());
        OCIO_PYTRY_EXIT(NULL)
    }
    
    PyObject * PyOCIO_SetCurrentConfig(PyObject * /*self*/, PyObject * args)
    {
        OCIO_PYTRY_ENTER()
        PyObject * pyconfig;
        if (!PyArg_ParseTuple(args, "O!:SetCurrentConfig",
            &OCIO::PyOCIO_ConfigType, &pyconfig)) return NULL;
        OCIO::ConstConfigRcPtr c = OCIO::GetConstConfig(pyconfig, true);
        OCIO::SetCurrentConfig(c);
        Py_RETURN_NONE;
        OCIO_PYTRY_EXIT(NULL)
    }
    
    PyMethodDef PyOCIO_methods[] = {
        { "ClearAllCaches",
        (PyCFunction) PyOCIO_ClearAllCaches, METH_NOARGS, OCIO::OPENCOLORIO_CLEARALLCACHES__DOC__ },
        { "GetLoggingLevel",
        (PyCFunction) PyOCIO_GetLoggingLevel, METH_NOARGS, OCIO::OPENCOLORIO_GETLOGGINGLEVEL__DOC__ },
        { "SetLoggingLevel",
        (PyCFunction) PyOCIO_SetLoggingLevel, METH_VARARGS, OCIO::OPENCOLORIO_SETLOGGINGLEVEL__DOC__ },
        { "GetCurrentConfig",
        (PyCFunction) PyOCIO_GetCurrentConfig, METH_NOARGS, OCIO::OPENCOLORIO_GETCURRENTCONFIG__DOC__ },
        { "SetCurrentConfig",
        (PyCFunction) PyOCIO_SetCurrentConfig, METH_VARARGS, OCIO::OPENCOLORIO_SETCURRENTCONFIG__DOC__ },
        { NULL, NULL, 0, NULL } /* Sentinel */
    };
    
}

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        PyObject * g_exceptionType = NULL;
        PyObject * g_exceptionMissingFileType = NULL;
    }
    
    // These are explicitly initialized in the init function
    // to make sure they're not initialized until after the module is
    
    PyObject * GetExceptionPyType()
    {
        return g_exceptionType;
    }
    
    void SetExceptionPyType(PyObject * pytypeobj)
    {
        g_exceptionType = pytypeobj;
    }
    
    PyObject * GetExceptionMissingFilePyType()
    {
        return g_exceptionMissingFileType;
    }
    
    void SetExceptionMissingFilePyType(PyObject * pytypeobj)
    {
        g_exceptionMissingFileType = pytypeobj;
    }
    
    inline bool AddObjectToModule(PyTypeObject& o, const char* n, PyObject* m)
    {
        o.tp_new = PyType_GenericNew;
        if(PyType_Ready(&o) < 0) return false;
        Py_INCREF(&o);
        PyModule_AddObject(m, n, (PyObject *)&o);
        return true;
    }
    
    // fwd declare
    void AddConstantsModule(PyObject *enclosingModule);
    
}
OCIO_NAMESPACE_EXIT

MOD_INIT(PyOpenColorIO)
{
    PyObject * m;
    MOD_DEF(m, OCIO_STRINGIFY(PYOCIO_NAME), OCIO::OPENCOLORIO__DOC__, PyOCIO_methods);
    
    PyModule_AddStringConstant(m, "version", OCIO::GetVersion());
    PyModule_AddIntConstant(m, "hexversion", OCIO::GetVersionHex());
    
    // Create Exceptions, and add to the module
    char Exception[] = OCIO_PYTHON_NAMESPACE(Exception);
    char ExceptionMissingFile[] = OCIO_PYTHON_NAMESPACE(ExceptionMissingFile);
    
#if PY_MAJOR_VERSION >= 2 && PY_MINOR_VERSION >= 7
    OCIO::SetExceptionPyType(PyErr_NewExceptionWithDoc(Exception,
        (char*)OCIO::EXCEPTION__DOC__, OCIO::GetExceptionPyType(), NULL));
    OCIO::SetExceptionMissingFilePyType(PyErr_NewExceptionWithDoc(ExceptionMissingFile,
        (char*)OCIO::EXCEPTIONMISSINGFILE__DOC__, OCIO::GetExceptionMissingFilePyType(), NULL));
#else
    OCIO::SetExceptionPyType(PyErr_NewException(Exception,
        OCIO::GetExceptionPyType(), NULL));
    OCIO::SetExceptionMissingFilePyType(PyErr_NewException(ExceptionMissingFile,
        OCIO::GetExceptionMissingFilePyType(), NULL));
#endif
    
    PyModule_AddObject(m, "Exception", OCIO::GetExceptionPyType());
    PyModule_AddObject(m, "ExceptionMissingFile", OCIO::GetExceptionMissingFilePyType());
    
    // Register Classes
    OCIO::AddObjectToModule(OCIO::PyOCIO_ColorSpaceType, "ColorSpace", m);
    OCIO::AddObjectToModule(OCIO::PyOCIO_ConfigType, "Config", m);
    OCIO::AddConstantsModule(m);
    OCIO::AddObjectToModule(OCIO::PyOCIO_ContextType, "Context", m);
    OCIO::AddObjectToModule(OCIO::PyOCIO_LookType, "Look", m);
    OCIO::AddObjectToModule(OCIO::PyOCIO_ProcessorType, "Processor", m);
    OCIO::AddObjectToModule(OCIO::PyOCIO_ProcessorMetadataType, "ProcessorMetadata", m);
    OCIO::AddObjectToModule(OCIO::PyOCIO_GpuShaderDescType, "GpuShaderDesc", m);
    OCIO::AddObjectToModule(OCIO::PyOCIO_BakerType, "Baker", m);
    OCIO::AddObjectToModule(OCIO::PyOCIO_TransformType, "Transform", m);
    {
        OCIO::AddObjectToModule(OCIO::PyOCIO_AllocationTransformType, "AllocationTransform", m);
        OCIO::AddObjectToModule(OCIO::PyOCIO_CDLTransformType, "CDLTransform", m);
        OCIO::AddObjectToModule(OCIO::PyOCIO_ColorSpaceTransformType, "ColorSpaceTransform", m);
        OCIO::AddObjectToModule(OCIO::PyOCIO_DisplayTransformType, "DisplayTransform", m);
        OCIO::AddObjectToModule(OCIO::PyOCIO_ExponentTransformType, "ExponentTransform", m);
        OCIO::AddObjectToModule(OCIO::PyOCIO_FileTransformType, "FileTransform", m);
        OCIO::AddObjectToModule(OCIO::PyOCIO_GroupTransformType, "GroupTransform", m);
        OCIO::AddObjectToModule(OCIO::PyOCIO_LogTransformType, "LogTransform", m);
        OCIO::AddObjectToModule(OCIO::PyOCIO_LookTransformType, "LookTransform", m);
        OCIO::AddObjectToModule(OCIO::PyOCIO_MatrixTransformType, "MatrixTransform", m);
    }

    return MOD_SUCCESS_VAL(m);
}
