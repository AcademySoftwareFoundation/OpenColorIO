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

#include "PyContext.h"
#include "PyTransform.h"
#include "PyUtil.h"
#include "PyDoc.h"

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    bool AddContextObjectToModule( PyObject* m )
    {
        PyOCIO_ContextType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_ContextType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_ContextType );
        PyModule_AddObject(m, "Context",
                (PyObject *)&PyOCIO_ContextType);
        
        return true;
    }
    
    PyObject * BuildConstPyContext(ConstContextRcPtr context)
    {
        if (!context)
        {
            Py_RETURN_NONE;
        }
        
        PyOCIO_Context * pycontext = PyObject_New(
                PyOCIO_Context, (PyTypeObject * ) &PyOCIO_ContextType);
        
        pycontext->constcppobj = new ConstContextRcPtr();
        *pycontext->constcppobj = context;
        
        pycontext->cppobj = new ContextRcPtr();
        pycontext->isconst = true;
        
        return ( PyObject * ) pycontext;
    }
    
    PyObject * BuildEditablePyContext(ContextRcPtr context)
    {
        if (!context)
        {
            Py_RETURN_NONE;
        }
        
        PyOCIO_Context * pycontext = PyObject_New(
                PyOCIO_Context, (PyTypeObject * ) &PyOCIO_ContextType);
        
        pycontext->constcppobj = new ConstContextRcPtr();
        pycontext->cppobj = new ContextRcPtr();
        *pycontext->cppobj = context;
        
        pycontext->isconst = false;
        
        return ( PyObject * ) pycontext;
    }
    
    bool IsPyContext(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return (PyObject_Type(pyobject) == (PyObject *) (&PyOCIO_ContextType));
    }
    
    bool IsPyContextEditable(PyObject * pyobject)
    {
        if(!IsPyContext(pyobject))
        {
            throw Exception("PyObject must be an OCIO.Context.");
        }
        
        PyOCIO_Context * pycontext = reinterpret_cast<PyOCIO_Context *> (pyobject);
        return (!pycontext->isconst);
    }
    
    ConstContextRcPtr GetConstContext(PyObject * pyobject, bool allowCast)
    {
        if(!IsPyContext(pyobject))
        {
            throw Exception("PyObject must be an OCIO.Context.");
        }
        
        PyOCIO_Context * pycontext = reinterpret_cast<PyOCIO_Context *> (pyobject);
        if(pycontext->isconst && pycontext->constcppobj)
        {
            return *pycontext->constcppobj;
        }
        
        if(allowCast && !pycontext->isconst && pycontext->cppobj)
        {
            return *pycontext->cppobj;
        }
        
        throw Exception("PyObject must be a valid OCIO.Context.");
    }
    
    ContextRcPtr GetEditableContext(PyObject * pyobject)
    {
        if(!IsPyContext(pyobject))
        {
            throw Exception("PyObject must be an OCIO.Context.");
        }
        
        PyOCIO_Context * pycontext = reinterpret_cast<PyOCIO_Context *> (pyobject);
        if(!pycontext->isconst && pycontext->cppobj)
        {
            return *pycontext->cppobj;
        }
        
        throw Exception("PyObject must be an editable OCIO.Context.");
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    namespace
    {
        int PyOCIO_Context_init( PyOCIO_Context * self, PyObject * args, PyObject * kwds );
        void PyOCIO_Context_delete( PyOCIO_Context * self, PyObject * args );
        PyObject * PyOCIO_Context_isEditable( PyObject * self );
        PyObject * PyOCIO_Context_createEditableCopy( PyObject * self );
        PyObject * PyOCIO_Context_getCacheID( PyObject * self );
        
        PyObject * PyOCIO_Context_getSearchPath( PyObject * self );
        PyObject * PyOCIO_Context_setSearchPath( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_Context_getWorkingDir( PyObject * self );
        PyObject * PyOCIO_Context_setWorkingDir( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_Context_getStringVar( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_Context_setStringVar( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_Context_loadEnvironment( PyObject * self );
        
        PyObject * PyOCIO_Context_resolveStringVar( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_Context_resolveFileLocation( PyObject * self,  PyObject *args );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_Context_methods[] = {
            {"isEditable",
            (PyCFunction) PyOCIO_Context_isEditable, METH_NOARGS, CONTEXT_ISEDITABLE__DOC__ },
            {"createEditableCopy",
            (PyCFunction) PyOCIO_Context_createEditableCopy, METH_NOARGS, CONTEXT_CREATEEDITABLECOPY__DOC__ },
            {"getCacheID",
            (PyCFunction) PyOCIO_Context_getCacheID, METH_NOARGS, CONTEXT_GETCACHEID__DOC__ },
            {"getSearchPath",
            (PyCFunction) PyOCIO_Context_getSearchPath, METH_NOARGS, CONTEXT_GETSEARCHPATH__DOC__ },
            {"setSearchPath",
            PyOCIO_Context_setSearchPath, METH_VARARGS, CONTEXT_SETSEARCHPATH__DOC__ },
            {"getWorkingDir",
            (PyCFunction) PyOCIO_Context_getWorkingDir, METH_NOARGS, CONTEXT_GETWORKINGDIR__DOC__ },
            {"setWorkingDir",
            PyOCIO_Context_setWorkingDir, METH_VARARGS, CONTEXT_SETWORKINGDIR__DOC__ },
            {"getStringVar",
            PyOCIO_Context_getStringVar, METH_VARARGS, CONTEXT_GETSTRINGVAR__DOC__ },
            {"setStringVar",
            PyOCIO_Context_setStringVar, METH_VARARGS, CONTEXT_SETSTRINGVAR__DOC__ },
            {"loadEnvironment",
            (PyCFunction) PyOCIO_Context_loadEnvironment, METH_NOARGS, CONTEXT_LOADENVIRONMENT__DOC__ },
            {"resolveStringVar",
            PyOCIO_Context_resolveStringVar, METH_VARARGS, CONTEXT_RESOLVESTRINGVAR__DOC__ },
            {"resolveFileLocation",
            PyOCIO_Context_resolveFileLocation, METH_VARARGS, CONTEXT_RESOLVEFILELOCATION__DOC__ },
            {NULL, NULL, 0, NULL}
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ContextType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.Context",                             //tp_name
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
        0,                                          //tp_str
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
        0,                                          //tp_bases
        0,                                          //tp_mro
        0,                                          //tp_cache
        0,                                          //tp_subclasses
        0,                                          //tp_weaklist
        0,                                          //tp_del
        #if PY_VERSION_HEX > 0x02060000
        0,                                          //tp_version_tag
        #endif
    };
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    namespace
    {
        ///////////////////////////////////////////////////////////////////////
        ///
        int PyOCIO_Context_init( PyOCIO_Context *self, PyObject * /*args*/, PyObject * /*kwds*/ )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            
            self->constcppobj = new ConstContextRcPtr();
            self->cppobj = new ContextRcPtr();
            self->isconst = true;
            
            try
            {
                *self->cppobj = Context::Create();
                self->isconst = false;
                return 0;
            }
            catch ( const std::exception & e )
            {
                std::string message = "Cannot create context: ";
                message += e.what();
                PyErr_SetString( PyExc_RuntimeError, message.c_str() );
                return -1;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        void PyOCIO_Context_delete( PyOCIO_Context *self, PyObject * /*args*/ )
        {
            delete self->constcppobj;
            delete self->cppobj;
            
            self->ob_type->tp_free((PyObject*)self);
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_Context_isEditable( PyObject * self )
        {
            return PyBool_FromLong(IsPyContextEditable(self));
        }
        
        PyObject * PyOCIO_Context_createEditableCopy( PyObject * self )
        {
            try
            {
                ConstContextRcPtr context = GetConstContext(self, true);
                ContextRcPtr copy = context->createEditableCopy();
                return BuildEditablePyContext( copy );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Context_getCacheID( PyObject * self )
        {
            try
            {
                ConstContextRcPtr context = GetConstContext(self, true);
                return PyString_FromString( context->getCacheID() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_Context_getSearchPath( PyObject * self )
        {
            try
            {
                ConstContextRcPtr context = GetConstContext(self, true);
                return PyString_FromString( context->getSearchPath() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Context_setSearchPath( PyObject * self, PyObject * args )
        {
            try
            {
                char * path = 0;
                if (!PyArg_ParseTuple(args,"s:setSearchPath", &path)) return NULL;
                
                ContextRcPtr context = GetEditableContext(self);
                context->setSearchPath( path );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Context_getWorkingDir( PyObject * self )
        {
            try
            {
                ConstContextRcPtr context = GetConstContext(self, true);
                return PyString_FromString( context->getWorkingDir() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Context_setWorkingDir( PyObject * self, PyObject * args )
        {
            try
            {
                char * dirname = 0;
                if (!PyArg_ParseTuple(args,"s:setWorkingDir", &dirname)) return NULL;
                
                ContextRcPtr context = GetEditableContext(self);
                context->setWorkingDir( dirname );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Context_getStringVar( PyObject * self, PyObject * args )
        {
            try
            {
                char * name = 0;
                if (!PyArg_ParseTuple(args,"s:getStringVar", &name)) return NULL;
                
                ConstContextRcPtr context = GetConstContext(self, true);
                return PyString_FromString( context->getStringVar(name) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Context_setStringVar( PyObject * self, PyObject * args )
        {
            try
            {
                char * name = 0;
                char * value = 0;
                if (!PyArg_ParseTuple(args,"ss:setStringVar", &name, &value)) return NULL;
                
                ContextRcPtr context = GetEditableContext(self);
                context->setStringVar( name, value );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Context_loadEnvironment( PyObject * self )
        {
            try
            {
                ContextRcPtr context = GetEditableContext(self);
                context->loadEnvironment();
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Context_resolveStringVar( PyObject * self, PyObject * args )
        {
            try
            {
                char * str = 0;
                if (!PyArg_ParseTuple(args,"s:resolveStringVar", &str)) return NULL;
                
                ConstContextRcPtr context = GetConstContext(self, true);
                return PyString_FromString( context->resolveStringVar(str) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Context_resolveFileLocation( PyObject * self, PyObject * args )
        {
            try
            {
                char * filename = 0;
                if (!PyArg_ParseTuple(args,"s:resolveFileLocation", &filename)) return NULL;
                
                ConstContextRcPtr context = GetConstContext(self, true);
                return PyString_FromString( context->resolveFileLocation(filename) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
    }

}
OCIO_NAMESPACE_EXIT
