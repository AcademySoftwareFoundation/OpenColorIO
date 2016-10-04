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
#include <sstream>

#include "PyUtil.h"
#include "PyDoc.h"

OCIO_NAMESPACE_ENTER
{
    
    PyObject * BuildConstPyContext(ConstContextRcPtr context)
    {
        return BuildConstPyOCIO<PyOCIO_Context, ContextRcPtr,
            ConstContextRcPtr>(context, PyOCIO_ContextType);
    }
    
    PyObject * BuildEditablePyContext(ContextRcPtr context)
    {
        return BuildEditablePyOCIO<PyOCIO_Context, ContextRcPtr,
            ConstContextRcPtr>(context, PyOCIO_ContextType);
    }
    
    bool IsPyContext(PyObject * pyobject)
    {
        return IsPyOCIOType(pyobject, PyOCIO_ContextType);
    }
    
    bool IsPyContextEditable(PyObject * pyobject)
    {
        return IsPyEditable<PyOCIO_Context>(pyobject, PyOCIO_ContextType);
    }
    
    ConstContextRcPtr GetConstContext(PyObject * pyobject, bool allowCast)
    {
        return GetConstPyOCIO<PyOCIO_Context, ConstContextRcPtr>(pyobject,
            PyOCIO_ContextType, allowCast);
    }
    
    ContextRcPtr GetEditableContext(PyObject * pyobject)
    {
        return GetEditablePyOCIO<PyOCIO_Context, ContextRcPtr>(pyobject,
            PyOCIO_ContextType);
    }
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_Context_init(PyOCIO_Context * self, PyObject * args, PyObject * kwds);
        void PyOCIO_Context_delete(PyOCIO_Context * self, PyObject * args);
        PyObject * PyOCIO_Context_str(PyObject * self);
        PyObject * PyOCIO_Context_isEditable(PyObject * self);
        PyObject * PyOCIO_Context_createEditableCopy(PyObject * self);
        PyObject * PyOCIO_Context_getCacheID(PyObject * self);
        PyObject * PyOCIO_Context_getSearchPath(PyObject * self);
        PyObject * PyOCIO_Context_setSearchPath(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Context_getWorkingDir(PyObject * self);
        PyObject * PyOCIO_Context_setWorkingDir(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Context_getStringVar(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Context_setStringVar(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Context_getNumStringVars(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Context_getStringVarNameByIndex(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Context_clearStringVars(PyObject * self);
        PyObject * PyOCIO_Context_setEnvironmentMode(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Context_getEnvironmentMode(PyObject * self);
        PyObject * PyOCIO_Context_loadEnvironment(PyObject * self);
        PyObject * PyOCIO_Context_resolveStringVar(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Context_resolveFileLocation(PyObject * self, PyObject * args);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_Context_methods[] = {
            { "isEditable",
            (PyCFunction) PyOCIO_Context_isEditable, METH_NOARGS, CONTEXT_ISEDITABLE__DOC__ },
            { "createEditableCopy",
            (PyCFunction) PyOCIO_Context_createEditableCopy, METH_NOARGS, CONTEXT_CREATEEDITABLECOPY__DOC__ },
            { "getCacheID",
            (PyCFunction) PyOCIO_Context_getCacheID, METH_NOARGS, CONTEXT_GETCACHEID__DOC__ },
            { "getSearchPath",
            (PyCFunction) PyOCIO_Context_getSearchPath, METH_NOARGS, CONTEXT_GETSEARCHPATH__DOC__ },
            { "setSearchPath",
            PyOCIO_Context_setSearchPath, METH_VARARGS, CONTEXT_SETSEARCHPATH__DOC__ },
            { "getWorkingDir",
            (PyCFunction) PyOCIO_Context_getWorkingDir, METH_NOARGS, CONTEXT_GETWORKINGDIR__DOC__ },
            { "setWorkingDir",
            PyOCIO_Context_setWorkingDir, METH_VARARGS, CONTEXT_SETWORKINGDIR__DOC__ },
            { "getStringVar",
            PyOCIO_Context_getStringVar, METH_VARARGS, CONTEXT_GETSTRINGVAR__DOC__ },
            { "setStringVar",
            PyOCIO_Context_setStringVar, METH_VARARGS, CONTEXT_SETSTRINGVAR__DOC__ },
            { "getNumStringVars",
            PyOCIO_Context_getNumStringVars, METH_VARARGS, CONTEXT_GETNUMSTRINGVARS__DOC__ },
            { "getStringVarNameByIndex",
            PyOCIO_Context_getStringVarNameByIndex, METH_VARARGS, CONTEXT_GETSTRINGVARNAMEBYINDEX__DOC__ },
            { "clearStringVars",
            (PyCFunction) PyOCIO_Context_clearStringVars, METH_NOARGS, CONTEXT_CLEARSTRINGVARS__DOC__ },
            { "setEnvironmentMode",
            PyOCIO_Context_setEnvironmentMode, METH_VARARGS, CONTEXT_SETENVIRONMENTMODE__DOC__ },
            { "getEnvironmentMode",
            (PyCFunction) PyOCIO_Context_getEnvironmentMode, METH_NOARGS, CONTEXT_GETENVIRONMENTMODE__DOC__ },
            { "loadEnvironment",
            (PyCFunction) PyOCIO_Context_loadEnvironment, METH_NOARGS, CONTEXT_LOADENVIRONMENT__DOC__ },
            { "resolveStringVar",
            PyOCIO_Context_resolveStringVar, METH_VARARGS, CONTEXT_RESOLVESTRINGVAR__DOC__ },
            { "resolveFileLocation",
            PyOCIO_Context_resolveFileLocation, METH_VARARGS, CONTEXT_RESOLVEFILELOCATION__DOC__ },
            { NULL, NULL, 0, NULL }
        };
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ContextType = {
        PyVarObject_HEAD_INIT(NULL, 0)              //ob_size
        OCIO_PYTHON_NAMESPACE(Context),             //tp_name
        sizeof(PyOCIO_Context),                     //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_Context_delete,          //tp_dealloc
        0,                                          //tp_print
        0,                                          //tp_getattr
        0,                                          //tp_setattr
        0,                                          //tp_compare
        0,                                          //tp_repr
        0,                                          //tp_as_number
        0,                                          //tp_as_sequence
        0,                                          //tp_as_mapping
        0,                                          //tp_hash 
        0,                                          //tp_call
        PyOCIO_Context_str,                         //tp_str
        0,                                          //tp_getattro
        0,                                          //tp_setattro
        0,                                          //tp_as_buffer
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   //tp_flags
        CONTEXT__DOC__,                             //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_Context_methods,                     //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_Context_init,             //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_Context_init(PyOCIO_Context *self, PyObject * /*args*/, PyObject * /*kwds*/)
        {
            OCIO_PYTRY_ENTER()
            return BuildPyObject<PyOCIO_Context, ConstContextRcPtr, ContextRcPtr>(self, Context::Create());
            OCIO_PYTRY_EXIT(-1)
        }
        
        void PyOCIO_Context_delete(PyOCIO_Context *self, PyObject * /*args*/)
        {
            DeletePyObject<PyOCIO_Context>(self);
        }
        
        PyObject * PyOCIO_Context_str(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstContextRcPtr context = GetConstContext(self, true);
            std::ostringstream out;
            out << *context;
            return PyString_FromString(out.str().c_str());
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_Context_isEditable(PyObject * self)
        {
            return PyBool_FromLong(IsPyContextEditable(self));
        }
        
        PyObject * PyOCIO_Context_createEditableCopy(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstContextRcPtr context = GetConstContext(self, true);
            ContextRcPtr copy = context->createEditableCopy();
            return BuildEditablePyContext(copy);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_getCacheID(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstContextRcPtr context = GetConstContext(self, true);
            return PyString_FromString(context->getCacheID());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_getSearchPath(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstContextRcPtr context = GetConstContext(self, true);
            return PyString_FromString(context->getSearchPath());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_setSearchPath(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* path = 0;
            if (!PyArg_ParseTuple(args, "s:setSearchPath",
                &path)) return NULL;
            ContextRcPtr context = GetEditableContext(self);
            context->setSearchPath(path);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_getWorkingDir(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstContextRcPtr context = GetConstContext(self, true);
            return PyString_FromString(context->getWorkingDir());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_setWorkingDir(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* dirname = 0;
            if (!PyArg_ParseTuple(args, "s:setWorkingDir",
                &dirname)) return NULL;
            ContextRcPtr context = GetEditableContext(self);
            context->setWorkingDir(dirname);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_getStringVar(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            if (!PyArg_ParseTuple(args, "s:getStringVar",
                &name)) return NULL;
            ConstContextRcPtr context = GetConstContext(self, true);
            return PyString_FromString(context->getStringVar(name));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_setStringVar(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            char* value = 0;
            if (!PyArg_ParseTuple(args, "ss:setStringVar",
                &name, &value)) return NULL;
            ContextRcPtr context = GetEditableContext(self);
            context->setStringVar(name, value);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_getNumStringVars(PyObject * self, PyObject * /*args*/)
        {
            OCIO_PYTRY_ENTER()
            ConstContextRcPtr context = GetConstContext(self, true);
            return PyInt_FromLong(context->getNumStringVars());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_getStringVarNameByIndex(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int index = 0;
            if (!PyArg_ParseTuple(args,"i:getStringVarNameByIndex",
                &index)) return NULL;
            ConstContextRcPtr context = GetConstContext(self, true);
            return PyString_FromString(context->getStringVarNameByIndex(index));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_clearStringVars(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ContextRcPtr context = GetEditableContext(self);
            context->clearStringVars();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_setEnvironmentMode(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            EnvironmentMode mode;
            if (!PyArg_ParseTuple(args, "O&:setEnvironmentMode",
                ConvertPyObjectToEnvironmentMode, &mode)) return NULL;
            ContextRcPtr context = GetEditableContext(self);
            context->setEnvironmentMode(mode);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_getEnvironmentMode(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstContextRcPtr context = GetConstContext(self, true);
            EnvironmentMode mode = context->getEnvironmentMode();
            return PyString_FromString(EnvironmentModeToString(mode));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_loadEnvironment(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ContextRcPtr context = GetEditableContext(self);
            context->loadEnvironment();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_resolveStringVar(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* str = 0;
            if (!PyArg_ParseTuple(args,"s:resolveStringVar",
                &str)) return NULL;
            ConstContextRcPtr context = GetConstContext(self, true);
            return PyString_FromString(context->resolveStringVar(str));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Context_resolveFileLocation(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* filename = 0;
            if (!PyArg_ParseTuple(args,"s:resolveFileLocation",
                &filename)) return NULL;
            ConstContextRcPtr context = GetConstContext(self, true);
            return PyString_FromString(context->resolveFileLocation(filename));
            OCIO_PYTRY_EXIT(NULL)
        }
        
    }

}
OCIO_NAMESPACE_EXIT
