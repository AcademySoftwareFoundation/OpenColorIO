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

#include "PyColorSpace.h"
#include "PyConfig.h"
#include "PyContext.h"
#include "PyConstants.h"
#include "PyLook.h"
#include "PyProcessor.h"
#include "PyTransform.h"
#include "PyUtil.h"

namespace
{
    PyObject * PyOCIO_ClearAllCaches(PyObject * /* self */)
    {
        try
        {
            OCIO::ClearAllCaches();
            Py_RETURN_NONE;
        }
        catch(...)
        {
            OCIO::Python_Handle_Exception();
            return NULL;
        }
    }
    
    PyObject * PyOCIO_GetCurrentConfig(PyObject * /* self */)
    {
        try
        {
            return OCIO::BuildConstPyConfig( OCIO::GetCurrentConfig() );
        }
        catch(...)
        {
            OCIO::Python_Handle_Exception();
            return NULL;
        }
    }
    
    PyObject * PyOCIO_SetCurrentConfig(PyObject * /*self*/, PyObject * args)
    {
        try
        {
            PyObject * pyconfig;
            if (!PyArg_ParseTuple(args, "O!:SetCurrentConfig",
                &OCIO::PyOCIO_ConfigType, &pyconfig))
            {
                return NULL;
            }
            
            OCIO::ConstConfigRcPtr c = OCIO::GetConstConfig(pyconfig, true);
            OCIO::SetCurrentConfig(c);
            
            Py_RETURN_NONE;
        }
        catch(...)
        {
            OCIO::Python_Handle_Exception();
            return NULL;
        }
    }
    
    
    PyMethodDef PyOCIO_methods[] = {
        {"ClearAllCaches",
            (PyCFunction) PyOCIO_ClearAllCaches,
            METH_NOARGS,
            ""},
        {"GetCurrentConfig",
            (PyCFunction) PyOCIO_GetCurrentConfig,
            METH_NOARGS,
            ""},
        {"SetCurrentConfig",
            (PyCFunction) PyOCIO_SetCurrentConfig,
            METH_VARARGS,
            ""},
        
        {NULL, NULL, 0, NULL} /* Sentinel */
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
}
OCIO_NAMESPACE_EXIT

extern "C"
PyMODINIT_FUNC
initPyOpenColorIO(void)
{
    PyObject * m;
    m = Py_InitModule3("PyOpenColorIO", PyOCIO_methods, "OpenColorIO API");
    
    PyModule_AddStringConstant(m, "version", OCIO::GetVersion());
    PyModule_AddIntConstant(m, "hexversion", OCIO::GetVersionHex());
    
    // Create Exceptions, and add to the module
    OCIO::SetExceptionPyType(
        PyErr_NewException(const_cast<char*>("PyOpenColorIO.Exception"),
                                                  PyExc_RuntimeError, NULL));
    PyModule_AddObject(m, "Exception", OCIO::GetExceptionPyType());
    
    OCIO::SetExceptionMissingFilePyType(
        PyErr_NewException(const_cast<char*>("PyOpenColorIO.ExceptionMissingFile"),
                           OCIO::GetExceptionPyType(), NULL));
    PyModule_AddObject(m, "ExceptionMissingFile", OCIO::GetExceptionMissingFilePyType());
    
    // Register Classes
    
    OCIO::AddColorSpaceObjectToModule( m );
    OCIO::AddConfigObjectToModule( m );
    OCIO::AddConstantsModule( m );
    OCIO::AddContextObjectToModule( m );
    OCIO::AddLookObjectToModule( m );
    OCIO::AddProcessorObjectToModule( m );
    
    OCIO::AddTransformObjectToModule( m );
    {
        OCIO::AddAllocationTransformObjectToModule( m );
        OCIO::AddCDLTransformObjectToModule( m );
        OCIO::AddColorSpaceTransformObjectToModule( m );
        OCIO::AddDisplayTransformObjectToModule( m );
        OCIO::AddExponentTransformObjectToModule( m );
        OCIO::AddFileTransformObjectToModule( m );
        OCIO::AddGroupTransformObjectToModule( m );
        OCIO::AddLogTransformObjectToModule( m );
        OCIO::AddLookTransformObjectToModule( m );
        OCIO::AddMatrixTransformObjectToModule( m );
    }
}
