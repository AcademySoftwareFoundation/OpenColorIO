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


#define GetConstRangeTransform(pyobject) GetConstPyOCIO<PyOCIO_Transform, \
    ConstRangeTransformRcPtr, RangeTransform>(pyobject, PyOCIO_RangeTransformType)

#define GetEditableRangeTransform(pyobject) GetEditablePyOCIO<PyOCIO_Transform, \
    RangeTransformRcPtr, RangeTransform>(pyobject, PyOCIO_RangeTransformType)


OCIO_NAMESPACE_ENTER
{
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_RangeTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds);
        PyObject * PyOCIO_RangeTransform_equals(PyObject * self, PyObject * args);
        PyObject * PyOCIO_RangeTransform_validate(PyObject * self);

        PyObject * PyOCIO_RangeTransform_getMinInValue(PyObject * self);
        PyObject * PyOCIO_RangeTransform_setMinInValue(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_RangeTransform_hasMinInValue(PyObject * self);
        PyObject * PyOCIO_RangeTransform_unsetMinInValue(PyObject * self);

        PyObject * PyOCIO_RangeTransform_getMaxInValue(PyObject * self);
        PyObject * PyOCIO_RangeTransform_setMaxInValue(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_RangeTransform_hasMaxInValue(PyObject * self);
        PyObject * PyOCIO_RangeTransform_unsetMaxInValue(PyObject * self);

        PyObject * PyOCIO_RangeTransform_getMinOutValue(PyObject * self);
        PyObject * PyOCIO_RangeTransform_setMinOutValue(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_RangeTransform_hasMinOutValue(PyObject * self);
        PyObject * PyOCIO_RangeTransform_unsetMinOutValue(PyObject * self);

        PyObject * PyOCIO_RangeTransform_getMaxOutValue(PyObject * self);
        PyObject * PyOCIO_RangeTransform_setMaxOutValue(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_RangeTransform_hasMaxOutValue(PyObject * self);
        PyObject * PyOCIO_RangeTransform_unsetMaxOutValue(PyObject * self);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_RangeTransform_methods[] = {
            { "equals",
            PyOCIO_RangeTransform_equals, METH_VARARGS, RANGETRANSFORM_EQUALS__DOC__ },
            { "validate",
            (PyCFunction) PyOCIO_RangeTransform_validate, METH_NOARGS, RANGETRANSFORM_VALIDATE__DOC__ },

            { "getMinInValue",
            (PyCFunction) PyOCIO_RangeTransform_getMinInValue, METH_NOARGS, RANGETRANSFORM_GETMININVALUE__DOC__ },
            { "setMinInValue",
            PyOCIO_RangeTransform_setMinInValue, METH_VARARGS, RANGETRANSFORM_SETMININVALUE__DOC__ },
            { "hasMinInValue",
            (PyCFunction) PyOCIO_RangeTransform_hasMinInValue, METH_NOARGS, RANGETRANSFORM_HASMININVALUE__DOC__ },
            { "unsetMinInValue",
            (PyCFunction) PyOCIO_RangeTransform_unsetMinInValue, METH_NOARGS, RANGETRANSFORM_UNSETMININVALUE__DOC__ },

            { "getMaxInValue",
            (PyCFunction) PyOCIO_RangeTransform_getMaxInValue, METH_NOARGS, RANGETRANSFORM_GETMAXINVALUE__DOC__ },
            { "setMaxInValue",
            PyOCIO_RangeTransform_setMaxInValue, METH_VARARGS, RANGETRANSFORM_SETMAXINVALUE__DOC__ },
            { "hasMaxInValue",
            (PyCFunction) PyOCIO_RangeTransform_hasMaxInValue, METH_NOARGS, RANGETRANSFORM_HASMAXINVALUE__DOC__ },
            { "unsetMaxInValue",
            (PyCFunction) PyOCIO_RangeTransform_unsetMaxInValue, METH_NOARGS, RANGETRANSFORM_UNSETMAXINVALUE__DOC__ },

            { "getMinOutValue",
            (PyCFunction) PyOCIO_RangeTransform_getMinOutValue, METH_NOARGS, RANGETRANSFORM_GETMINOUTVALUE__DOC__ },
            { "setMinOutValue",
            PyOCIO_RangeTransform_setMinOutValue, METH_VARARGS, RANGETRANSFORM_SETMINOUTVALUE__DOC__ },
            { "hasMinOutValue",
            (PyCFunction) PyOCIO_RangeTransform_hasMinOutValue, METH_NOARGS, RANGETRANSFORM_HASMINOUTVALUE__DOC__ },
            { "unsetMinOutValue",
            (PyCFunction) PyOCIO_RangeTransform_unsetMinOutValue, METH_NOARGS, RANGETRANSFORM_UNSETMINOUTVALUE__DOC__ },

            { "getMaxOutValue",
            (PyCFunction) PyOCIO_RangeTransform_getMaxOutValue, METH_NOARGS, RANGETRANSFORM_GETMAXOUTVALUE__DOC__ },
            { "setMaxOutValue",
            PyOCIO_RangeTransform_setMaxOutValue, METH_VARARGS, RANGETRANSFORM_SETMAXOUTVALUE__DOC__ },
            { "hasMaxOutValue",
            (PyCFunction) PyOCIO_RangeTransform_hasMaxOutValue, METH_NOARGS, RANGETRANSFORM_HASMAXOUTVALUE__DOC__ },
            { "unsetMaxOutValue",
            (PyCFunction) PyOCIO_RangeTransform_unsetMaxOutValue, METH_NOARGS, RANGETRANSFORM_UNSETMAXOUTVALUE__DOC__ },

            { NULL, NULL, 0, NULL }
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_RangeTransformType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        OCIO_PYTHON_NAMESPACE(RangeTransform),      //tp_name
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
        RANGETRANSFORM__DOC__,                      //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_RangeTransform_methods,              //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        &PyOCIO_TransformType,                      //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_RangeTransform_init,      //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_RangeTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds)
        {
            OCIO_PYTRY_ENTER()
            RangeTransformRcPtr ptr = RangeTransform::Create();
            const int ret = BuildPyTransformObject<RangeTransformRcPtr>(self, ptr);
            // Set bounds to the default values
            char * direction = NULL;
            double minInValue   = ptr->getMinInValue();
            double maxInValue   = ptr->getMaxInValue();
            double minOutValue  = ptr->getMinOutValue();
            double maxOutValue  = ptr->getMaxOutValue();
            // Parse the bound values
            static const char *kwlist[] 
                = { "minInValue", "maxInValue", "minOutValue", "maxOutValue", "direction", NULL };
            if(!PyArg_ParseTupleAndKeywords(args, kwds, "|dddds",
                const_cast<char **>(kwlist),
                &direction, &minInValue, &maxInValue, &minOutValue, &maxOutValue)) return -1;
            // Set the bound values
            ptr->setMinInValue(minInValue);
            ptr->setMaxInValue(maxInValue);
            ptr->setMinOutValue(minOutValue);
            ptr->setMaxOutValue(maxOutValue);
            if(direction) ptr->setDirection(TransformDirectionFromString(direction));
            ptr->validate();
            return ret;
            OCIO_PYTRY_EXIT(-1)
        }

        PyObject * PyOCIO_RangeTransform_equals(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pyobject = 0;
            if (!PyArg_ParseTuple(args,"O:equals", &pyobject)) return NULL;
            if(!IsPyOCIOType(pyobject, PyOCIO_RangeTransformType))
                throw Exception("RangeTransform.equals requires a RangeTransform argument");
            ConstRangeTransformRcPtr transform = GetConstRangeTransform(self);
            ConstRangeTransformRcPtr in = GetConstRangeTransform(pyobject);
            return PyBool_FromLong(transform->equals(*in.get()));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_RangeTransform_validate(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            RangeTransformRcPtr transform = GetEditableRangeTransform(self);
            transform->validate();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_getMinInValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstRangeTransformRcPtr transform = GetConstRangeTransform(self);
            return PyFloat_FromDouble(transform->getMinInValue());
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_setMinInValue(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            double minInput;
            if (!PyArg_ParseTuple(args, "d:setMinInValue", &minInput)) return NULL;
            RangeTransformRcPtr transform = GetEditableRangeTransform(self);
            transform->setMinInValue(minInput);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_hasMinInValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstRangeTransformRcPtr transform = GetConstRangeTransform(self);
            return PyBool_FromLong(transform->hasMinInValue());
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_unsetMinInValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            RangeTransformRcPtr transform = GetEditableRangeTransform(self);
            transform->unsetMinInValue();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_getMaxInValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstRangeTransformRcPtr transform = GetConstRangeTransform(self);
            return PyFloat_FromDouble(transform->getMaxInValue());
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_setMaxInValue(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            double maxInput;
            if (!PyArg_ParseTuple(args, "d:setMaxInValue", &maxInput)) return NULL;
            RangeTransformRcPtr transform = GetEditableRangeTransform(self);
            transform->setMaxInValue(maxInput);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_hasMaxInValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstRangeTransformRcPtr transform = GetConstRangeTransform(self);
            return PyBool_FromLong(transform->hasMaxInValue());
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_unsetMaxInValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            RangeTransformRcPtr transform = GetEditableRangeTransform(self);
            transform->unsetMaxInValue();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_getMinOutValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstRangeTransformRcPtr transform = GetConstRangeTransform(self);
            return PyFloat_FromDouble(transform->getMinOutValue());
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_setMinOutValue(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            double minOutput;
            if (!PyArg_ParseTuple(args, "d:setMinOutValue", &minOutput)) return NULL;
            RangeTransformRcPtr transform = GetEditableRangeTransform(self);
            transform->setMinOutValue(minOutput);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_hasMinOutValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstRangeTransformRcPtr transform = GetConstRangeTransform(self);
            return PyBool_FromLong(transform->hasMinOutValue());
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_unsetMinOutValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            RangeTransformRcPtr transform = GetEditableRangeTransform(self);
            transform->unsetMinOutValue();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_getMaxOutValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstRangeTransformRcPtr transform = GetConstRangeTransform(self);
            return PyFloat_FromDouble(transform->getMaxOutValue());
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_setMaxOutValue(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            double maxOutput;
            if (!PyArg_ParseTuple(args, "d:setMaxOutValue", &maxOutput)) return NULL;
            RangeTransformRcPtr transform = GetEditableRangeTransform(self);
            transform->setMaxOutValue(maxOutput);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_hasMaxOutValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstRangeTransformRcPtr transform = GetConstRangeTransform(self);
            return PyBool_FromLong(transform->hasMaxOutValue());
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_RangeTransform_unsetMaxOutValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            RangeTransformRcPtr transform = GetEditableRangeTransform(self);
            transform->unsetMaxOutValue();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }

    }

}
OCIO_NAMESPACE_EXIT
