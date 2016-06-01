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

#include "PyUtil.h"
#include "PyDoc.h"

#define GetConstLookTransform(pyobject) GetConstPyOCIO<PyOCIO_Transform, \
    ConstLookTransformRcPtr, LookTransform>(pyobject, PyOCIO_LookTransformType)

#define GetEditableLookTransform(pyobject) GetEditablePyOCIO<PyOCIO_Transform, \
    LookTransformRcPtr, LookTransform>(pyobject, PyOCIO_LookTransformType)

OCIO_NAMESPACE_ENTER
{
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_LookTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds);
        PyObject * PyOCIO_LookTransform_getSrc(PyObject * self);
        PyObject * PyOCIO_LookTransform_setSrc(PyObject * self, PyObject * args);
        PyObject * PyOCIO_LookTransform_getDst(PyObject * self);
        PyObject * PyOCIO_LookTransform_setDst(PyObject * self, PyObject * args);
        PyObject * PyOCIO_LookTransform_getLooks(PyObject * self);
        PyObject * PyOCIO_LookTransform_setLooks(PyObject * self, PyObject * args);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_LookTransform_methods[] = {
            { "getSrc",
            (PyCFunction) PyOCIO_LookTransform_getSrc, METH_NOARGS, LOOKTRANSFORM_GETSRC__DOC__ },
            { "setSrc",
            PyOCIO_LookTransform_setSrc, METH_VARARGS, LOOKTRANSFORM_SETSRC__DOC__ },
            { "getDst",
            (PyCFunction) PyOCIO_LookTransform_getDst, METH_NOARGS, LOOKTRANSFORM_GETDST__DOC__ },
            { "setDst",
            PyOCIO_LookTransform_setDst, METH_VARARGS, LOOKTRANSFORM_SETDST__DOC__ },
            { "getLooks",
            (PyCFunction) PyOCIO_LookTransform_getLooks, METH_NOARGS, LOOKTRANSFORM_GETLOOKS__DOC__ },
            { "setLooks",
            PyOCIO_LookTransform_setLooks, METH_VARARGS, LOOKTRANSFORM_SETLOOKS__DOC__ },
            { NULL, NULL, 0, NULL }
        };
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_LookTransformType = {
        PyVarObject_HEAD_INIT(NULL, 0)              //ob_size
        OCIO_PYTHON_NAMESPACE(LookTransform),       //tp_name
        sizeof(PyOCIO_Transform),                   //tp_basicsize
        0,                                          //tp_itemsize
        0,                                          //tp_dealloc
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
        LOOKTRANSFORM__DOC__,                       //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_LookTransform_methods,               //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        &PyOCIO_TransformType,                      //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_LookTransform_init,       //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_LookTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds)
        {
            OCIO_PYTRY_ENTER()
            LookTransformRcPtr ptr = LookTransform::Create();
            int ret = BuildPyTransformObject<LookTransformRcPtr>(self, ptr);
            char* src = NULL;
            char* dst = NULL;
            char* looks = NULL;
            char* direction = NULL;
            static const char *kwlist[] = { "src", "dst", "looks", "direction",
                NULL };
            if(!PyArg_ParseTupleAndKeywords(args, kwds, "|ssss",
                const_cast<char **>(kwlist),
                &src, &dst, &looks, &direction)) return -1;
            if(src) ptr->setSrc(src);
            if(dst) ptr->setDst(dst);
            if(looks) ptr->setLooks(looks);
            if(direction) ptr->setDirection(TransformDirectionFromString(direction));
            return ret;
            OCIO_PYTRY_EXIT(-1)
        }
        
        PyObject * PyOCIO_LookTransform_getSrc(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstLookTransformRcPtr transform = GetConstLookTransform(self);
            return PyString_FromString(transform->getSrc());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_LookTransform_setSrc(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            const char* str = 0;
            if (!PyArg_ParseTuple(args, "s:setSrc",
                &str)) return NULL;
            LookTransformRcPtr transform = GetEditableLookTransform(self);
            transform->setSrc(str);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_LookTransform_getDst(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstLookTransformRcPtr transform = GetConstLookTransform(self);
            return PyString_FromString(transform->getDst());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_LookTransform_setDst(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            const char* str = 0;
            if (!PyArg_ParseTuple(args, "s:setDst",
                &str)) return NULL;
            LookTransformRcPtr transform = GetEditableLookTransform(self);
            transform->setDst(str);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_LookTransform_getLooks(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstLookTransformRcPtr transform = GetConstLookTransform(self);
            return PyString_FromString( transform->getLooks() );
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_LookTransform_setLooks(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            const char* str = 0;
            if (!PyArg_ParseTuple(args, "s:setLooks",
                &str)) return NULL;
            LookTransformRcPtr transform = GetEditableLookTransform(self);
            transform->setLooks(str);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
    }

}
OCIO_NAMESPACE_EXIT
