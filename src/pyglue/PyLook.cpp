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
    
    PyObject * BuildConstPyLook(ConstLookRcPtr look)
    {
        return BuildConstPyOCIO<PyOCIO_Look, LookRcPtr,
            ConstLookRcPtr>(look, PyOCIO_LookType);
    }
    
    PyObject * BuildEditablePyLook(LookRcPtr look)
    {
        return BuildEditablePyOCIO<PyOCIO_Look, LookRcPtr,
            ConstLookRcPtr>(look, PyOCIO_LookType);
    }
    
    bool IsPyLook(PyObject * pyobject)
    {
        return IsPyOCIOType(pyobject, PyOCIO_LookType);
    }
    
    bool IsPyLookEditable(PyObject * pyobject)
    {
        return IsPyEditable<PyOCIO_Look>(pyobject, PyOCIO_LookType);
    }
    
    ConstLookRcPtr GetConstLook(PyObject * pyobject, bool allowCast)
    {
        return GetConstPyOCIO<PyOCIO_Look, ConstLookRcPtr>(pyobject,
            PyOCIO_LookType, allowCast);
    }
    
    LookRcPtr GetEditableLook(PyObject * pyobject)
    {
        return GetEditablePyOCIO<PyOCIO_Look, LookRcPtr>(pyobject,
            PyOCIO_LookType);
    }
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_Look_init(PyOCIO_Look * self, PyObject * args, PyObject * kwds);
        void PyOCIO_Look_delete(PyOCIO_Look * self, PyObject * args);
        PyObject * PyOCIO_Look_str(PyObject * self);
        PyObject * PyOCIO_Look_isEditable(PyObject * self);
        PyObject * PyOCIO_Look_createEditableCopy(PyObject * self);
        PyObject * PyOCIO_Look_getName(PyObject * self);
        PyObject * PyOCIO_Look_setName(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Look_getProcessSpace(PyObject * self);
        PyObject * PyOCIO_Look_setProcessSpace(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Look_getDescription(PyObject * self);
        PyObject * PyOCIO_Look_setDescription(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Look_getTransform(PyObject * self);
        PyObject * PyOCIO_Look_setTransform(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Look_getInverseTransform(PyObject * self);
        PyObject * PyOCIO_Look_setInverseTransform(PyObject * self, PyObject * args);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_Look_methods[] = {
            { "isEditable",
            (PyCFunction) PyOCIO_Look_isEditable, METH_NOARGS, LOOK_ISEDITABLE__DOC__ },
            { "createEditableCopy",
            (PyCFunction) PyOCIO_Look_createEditableCopy, METH_NOARGS, LOOK_CREATEEDITABLECOPY__DOC__ },
            { "getName",
            (PyCFunction) PyOCIO_Look_getName, METH_NOARGS, LOOK_GETNAME__DOC__ },
            { "setName",
            PyOCIO_Look_setName, METH_VARARGS, LOOK_SETNAME__DOC__ },
            { "getProcessSpace",
            (PyCFunction) PyOCIO_Look_getProcessSpace, METH_NOARGS, LOOK_GETPROCESSSPACE__DOC__ },
            { "setProcessSpace",
            PyOCIO_Look_setProcessSpace, METH_VARARGS, LOOK_SETPROCESSSPACE__DOC__ },
            { "getDescription",
            (PyCFunction) PyOCIO_Look_getDescription, METH_NOARGS, LOOK_GETDESCRIPTION__DOC__ },
            { "setDescription",
            PyOCIO_Look_setDescription, METH_VARARGS, LOOK_SETDESCRIPTION__DOC__ },
            { "getTransform",
            (PyCFunction) PyOCIO_Look_getTransform, METH_NOARGS, LOOK_GETTRANSFORM__DOC__ },
            { "setTransform",
            PyOCIO_Look_setTransform, METH_VARARGS, LOOK_SETTRANSFORM__DOC__ },
            { "getInverseTransform",
            (PyCFunction) PyOCIO_Look_getInverseTransform, METH_VARARGS, LOOK_GETINVERSETRANSFORM__DOC__ },
            { "setInverseTransform",
            PyOCIO_Look_setInverseTransform, METH_VARARGS, LOOK_SETINVERSETRANSFORM__DOC__ },
            { NULL, NULL, 0, NULL }
        };
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_LookType = {
        PyVarObject_HEAD_INIT(NULL, 0)              //ob_size
        OCIO_PYTHON_NAMESPACE(Look),                //tp_name
        sizeof(PyOCIO_Look),                        //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_Look_delete,             //tp_dealloc
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
        PyOCIO_Look_str,                            //tp_str
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
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_Look_init(PyOCIO_Look *self, PyObject * args, PyObject * kwds)
        {
            OCIO_PYTRY_ENTER()
            LookRcPtr ptr = Look::Create();
            /*int ret =*/ BuildPyObject<PyOCIO_Look, ConstLookRcPtr, LookRcPtr>(self, ptr);
            char* name = NULL;
            char* processSpace = NULL;
            char* description = NULL;
            PyObject* pytransform = NULL;
            const char* kwlist[] = { "name", "processSpace", "transform", "description", NULL };
            if(!PyArg_ParseTupleAndKeywords(args, kwds, "|ssOs",
                const_cast<char **>(kwlist),
                &name, &processSpace, &pytransform, &description)) return -1;
            if(name) ptr->setName(name);
            if(processSpace) ptr->setProcessSpace(processSpace);
            if(pytransform)
            {
                ConstTransformRcPtr transform = GetConstTransform(pytransform, true);
                ptr->setTransform(transform);
            }
            if(description) ptr->setDescription(description);
            return 0;
            OCIO_PYTRY_EXIT(-1)
        }
        
        void PyOCIO_Look_delete(PyOCIO_Look *self, PyObject * /*args*/)
        {
            DeletePyObject<PyOCIO_Look>(self);
        }
        
        PyObject * PyOCIO_Look_str(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstLookRcPtr look = GetConstLook(self, true);
            std::ostringstream out;
            out << *look;
            return PyString_FromString(out.str().c_str());
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_Look_isEditable(PyObject * self)
        {
            return PyBool_FromLong(IsPyLookEditable(self));
        }
        
        PyObject * PyOCIO_Look_createEditableCopy(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstLookRcPtr look = GetConstLook(self, true);
            LookRcPtr copy = look->createEditableCopy();
            return BuildEditablePyLook(copy);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Look_getName(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstLookRcPtr look = GetConstLook(self, true);
            return PyString_FromString(look->getName());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Look_setName(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            if (!PyArg_ParseTuple(args, "s:setName",
                &name)) return NULL;
            LookRcPtr look = GetEditableLook(self);
            look->setName(name);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Look_getProcessSpace(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstLookRcPtr look = GetConstLook(self, true);
            return PyString_FromString(look->getProcessSpace());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Look_setProcessSpace(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* processSpace = 0;
            if (!PyArg_ParseTuple(args,"s:setProcessSpace",
                &processSpace)) return NULL;
            LookRcPtr look = GetEditableLook(self);
            look->setProcessSpace(processSpace);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Look_getDescription(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstLookRcPtr look = GetConstLook(self, true);
            return PyString_FromString(look->getDescription());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Look_setDescription(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* description = 0;
            if (!PyArg_ParseTuple(args, "s:setDescription", &description)) return NULL;
            LookRcPtr look = GetEditableLook(self);
            look->setDescription(description);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Look_getTransform(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstLookRcPtr look = GetConstLook(self, true);
            ConstTransformRcPtr transform = look->getTransform();
            return BuildConstPyTransform(transform);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Look_setTransform(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pytransform = 0;
            if (!PyArg_ParseTuple(args, "O:setTransform",
                &pytransform))
                return NULL;
            ConstTransformRcPtr transform = GetConstTransform(pytransform, true);
            LookRcPtr look = GetEditableLook(self);
            look->setTransform(transform);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Look_getInverseTransform(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstLookRcPtr look = GetConstLook(self, true);
            ConstTransformRcPtr transform = look->getInverseTransform();
            return BuildConstPyTransform(transform);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Look_setInverseTransform(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pytransform = 0;
            if (!PyArg_ParseTuple(args, "O:setTransform",
                &pytransform))
                return NULL;
            ConstTransformRcPtr transform = GetConstTransform(pytransform, true);
            LookRcPtr look = GetEditableLook(self);
            look->setInverseTransform(transform);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
    }

}
OCIO_NAMESPACE_EXIT
