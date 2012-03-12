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

#include "PyLook.h"
#include "PyTransform.h"
#include "PyUtil.h"
#include "PyDoc.h"

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    bool AddLookObjectToModule( PyObject* m )
    {
        PyOCIO_LookType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_LookType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_LookType );
        PyModule_AddObject(m, "Look",
                (PyObject *)&PyOCIO_LookType);
        
        return true;
    }
    
    PyObject * BuildConstPyLook(ConstLookRcPtr look)
    {
        if (!look)
        {
            Py_RETURN_NONE;
        }
        
        PyOCIO_Look * pyLook = PyObject_New(
                PyOCIO_Look, (PyTypeObject * ) &PyOCIO_LookType);
        
        pyLook->constcppobj = new ConstLookRcPtr();
        *pyLook->constcppobj = look;
        
        pyLook->cppobj = new LookRcPtr();
        pyLook->isconst = true;
        
        return ( PyObject * ) pyLook;
    }
    
    PyObject * BuildEditablePyLook(LookRcPtr look)
    {
        if (!look)
        {
            Py_RETURN_NONE;
        }
        
        PyOCIO_Look * pyLook = PyObject_New(
                PyOCIO_Look, (PyTypeObject * ) &PyOCIO_LookType);
        
        pyLook->constcppobj = new ConstLookRcPtr();
        pyLook->cppobj = new LookRcPtr();
        *pyLook->cppobj = look;
        
        pyLook->isconst = false;
        
        return ( PyObject * ) pyLook;
    }
    
    bool IsPyLook(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return (PyObject_Type(pyobject) == (PyObject *) (&PyOCIO_LookType));
    }
    
    bool IsPyLookEditable(PyObject * pyobject)
    {
        if(!IsPyLook(pyobject))
        {
            throw Exception("PyObject must be an OCIO.Look.");
        }
        
        PyOCIO_Look * pyLook = reinterpret_cast<PyOCIO_Look *> (pyobject);
        return (!pyLook->isconst);
    }
    
    ConstLookRcPtr GetConstLook(PyObject * pyobject, bool allowCast)
    {
        if(!IsPyLook(pyobject))
        {
            throw Exception("PyObject must be an OCIO.Look.");
        }
        
        PyOCIO_Look * pylook = reinterpret_cast<PyOCIO_Look *> (pyobject);
        if(pylook->isconst && pylook->constcppobj)
        {
            return *pylook->constcppobj;
        }
        
        if(allowCast && !pylook->isconst && pylook->cppobj)
        {
            return *pylook->cppobj;
        }
        
        throw Exception("PyObject must be a valid OCIO.Look.");
    }
    
    LookRcPtr GetEditableLook(PyObject * pyobject)
    {
        if(!IsPyLook(pyobject))
        {
            throw Exception("PyObject must be an OCIO.Look.");
        }
        
        PyOCIO_Look * pylook = reinterpret_cast<PyOCIO_Look *> (pyobject);
        if(!pylook->isconst && pylook->cppobj)
        {
            return *pylook->cppobj;
        }
        
        throw Exception("PyObject must be an editable OCIO.Look.");
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    namespace
    {
        int PyOCIO_Look_init( PyOCIO_Look * self, PyObject * args, PyObject * kwds );
        void PyOCIO_Look_delete( PyOCIO_Look * self, PyObject * args );
        PyObject * PyOCIO_Look_isEditable( PyObject * self );
        PyObject * PyOCIO_Look_createEditableCopy( PyObject * self );
        
        PyObject * PyOCIO_Look_getName( PyObject * self );
        PyObject * PyOCIO_Look_setName( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_Look_getProcessSpace( PyObject * self );
        PyObject * PyOCIO_Look_setProcessSpace( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_Look_getTransform( PyObject * self );
        PyObject * PyOCIO_Look_setTransform( PyObject * self,  PyObject *args );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_Look_methods[] = {
            {"isEditable",
            (PyCFunction) PyOCIO_Look_isEditable, METH_NOARGS, LOOK_ISEDITABLE__DOC__ },
            {"createEditableCopy",
            (PyCFunction) PyOCIO_Look_createEditableCopy, METH_NOARGS, LOOK_CREATEEDITABLECOPY__DOC__ },
            {"getName",
            (PyCFunction) PyOCIO_Look_getName, METH_NOARGS, LOOK_GETNAME__DOC__ },
            {"setName",
            PyOCIO_Look_setName, METH_VARARGS, LOOK_SETNAME__DOC__ },
            {"getProcessSpace",
            (PyCFunction) PyOCIO_Look_getProcessSpace, METH_NOARGS, LOOK_GETPROCESSSPACE__DOC__ },
            {"setProcessSpace",
            PyOCIO_Look_setProcessSpace, METH_VARARGS, LOOK_SETPROCESSSPACE__DOC__ },
            {"getTransform",
            (PyCFunction) PyOCIO_Look_getTransform, METH_NOARGS, LOOK_GETTRANSFORM__DOC__ },
            {"setTransform",
            PyOCIO_Look_setTransform, METH_VARARGS, LOOK_SETTRANSFORM__DOC__ },
            {NULL, NULL, 0, NULL}
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_LookType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.Look",                           //tp_name
        sizeof(PyOCIO_Look),                   //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_Look_delete,        //tp_dealloc
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
        LOOK__DOC__,                                //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_Look_methods,                        //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_Look_init,                //tp_init 
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
        int PyOCIO_Look_init( PyOCIO_Look *self, PyObject * args, PyObject * kwds )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            
            self->constcppobj = new ConstLookRcPtr();
            self->cppobj = new LookRcPtr();
            self->isconst = true;
            
            // Parse optional kwargs
            char * name = NULL;
            char * processSpace = NULL;
            PyObject * pytransform = NULL;
            
            const char *kwlist[] = {
                "name", "processSpace", "transform",
                NULL
            };
            
            if(!PyArg_ParseTupleAndKeywords(args, kwds, "|ssO",
                const_cast<char **>(kwlist),
                &name, &processSpace, &pytransform)) return -1;
            
            try
            {
                LookRcPtr look = Look::Create();
                *self->cppobj = look;
                self->isconst = false;
                
                if(name) look->setName(name);
                if(processSpace) look->setProcessSpace(processSpace);
                
                if(pytransform)
                {
                    ConstTransformRcPtr transform = GetConstTransform(pytransform, true);
                    look->setTransform(transform);
                }
                
                return 0;
            }
            catch ( const std::exception & e )
            {
                std::string message = "Cannot create look: ";
                message += e.what();
                PyErr_SetString( PyExc_RuntimeError, message.c_str() );
                return -1;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        ///
        
        void PyOCIO_Look_delete( PyOCIO_Look *self, PyObject * /*args*/ )
        {
            delete self->constcppobj;
            delete self->cppobj;
            
            self->ob_type->tp_free((PyObject*)self);
        }
        
        ////////////////////////////////////////////////////////////////////////
        ///
        
        PyObject * PyOCIO_Look_isEditable( PyObject * self )
        {
            return PyBool_FromLong(IsPyLookEditable(self));
        }
        
        PyObject * PyOCIO_Look_createEditableCopy( PyObject * self )
        {
            try
            {
                ConstLookRcPtr look = GetConstLook(self, true);
                LookRcPtr copy = look->createEditableCopy();
                return BuildEditablePyLook( copy );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        ///
        
        PyObject * PyOCIO_Look_getName( PyObject * self )
        {
            try
            {
                ConstLookRcPtr look = GetConstLook(self, true);
                return PyString_FromString( look->getName() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Look_setName( PyObject * self, PyObject * args )
        {
            try
            {
                char * name = 0;
                if (!PyArg_ParseTuple(args,"s:setName", &name)) return NULL;
                
                LookRcPtr look = GetEditableLook(self);
                look->setName( name );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        ///
        
        PyObject * PyOCIO_Look_getProcessSpace( PyObject * self )
        {
            try
            {
                ConstLookRcPtr look = GetConstLook(self, true);
                return PyString_FromString( look->getProcessSpace() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Look_setProcessSpace( PyObject * self, PyObject * args )
        {
            try
            {
                char * processSpace = 0;
                if (!PyArg_ParseTuple(args,"s:setProcessSpace", &processSpace)) return NULL;
                
                LookRcPtr look = GetEditableLook(self);
                look->setProcessSpace( processSpace );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        ///
        
        PyObject * PyOCIO_Look_getTransform( PyObject * self )
        {
            try
            {
                ConstLookRcPtr look = GetConstLook(self, true);
                ConstTransformRcPtr transform = look->getTransform();
                return BuildConstPyTransform(transform);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Look_setTransform( PyObject * self,  PyObject *args )
        {
            try
            {
                PyObject * pytransform = 0;
                if (!PyArg_ParseTuple(args,"O:setTransform", &pytransform))
                    return NULL;
                
                ConstTransformRcPtr transform = GetConstTransform(pytransform, true);
                LookRcPtr look = GetEditableLook(self);
                look->setTransform(transform);
                
                Py_RETURN_NONE;
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
