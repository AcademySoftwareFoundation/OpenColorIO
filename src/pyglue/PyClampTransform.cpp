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

#define GetConstClampTransform(pyobject) GetConstPyOCIO<PyOCIO_Transform, \
    ConstClampTransformRcPtr, ClampTransform>(pyobject, PyOCIO_ClampTransformType)

#define GetEditableClampTransform(pyobject) GetEditablePyOCIO<PyOCIO_Transform, \
    ClampTransformRcPtr, ClampTransform>(pyobject, PyOCIO_ClampTransformType)

OCIO_NAMESPACE_ENTER
{

    namespace
    {

        ///////////////////////////////////////////////////////////////////////
        ///

        int PyOCIO_ClampTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds);
        PyObject * PyOCIO_ClampTransform_getMin(PyObject * self);
        PyObject * PyOCIO_ClampTransform_getMax(PyObject * self);
        PyObject * PyOCIO_ClampTransform_setMin(PyObject * self, PyObject * args);
        PyObject * PyOCIO_ClampTransform_setMax(PyObject * self, PyObject * args);

        ///////////////////////////////////////////////////////////////////////
        ///

        PyMethodDef PyOCIO_ClampTransform_methods[] = {
            { "getMin",
            (PyCFunction) PyOCIO_ClampTransform_getMin, METH_NOARGS, CLAMPTRANSFORM_GETMIN__DOC__ },
            { "getMax",
            (PyCFunction) PyOCIO_ClampTransform_getMax, METH_NOARGS, CLAMPTRANSFORM_GETMAX__DOC__ },
            { "setMin",
            (PyCFunction) PyOCIO_ClampTransform_setMin, METH_NOARGS, CLAMPTRANSFORM_SETMIN__DOC__ },
            { "setMax",
            (PyCFunction) PyOCIO_ClampTransform_setMax, METH_NOARGS, CLAMPTRANSFORM_SETMAX__DOC__ },
            { NULL, NULL, 0, NULL }
        };

    }

    ///////////////////////////////////////////////////////////////////////////
    ///

    PyTypeObject PyOCIO_ClampTransformType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "OCIO.ClampTransform",                      //tp_name
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
        CLAMPTRANSFORM__DOC__,                      //tp_doc
        0,                                          //tp_traverse
        0,                                          //tp_clear
        0,                                          //tp_richcompare
        0,                                          //tp_weaklistoffset
        0,                                          //tp_iter
        0,                                          //tp_iternext
        PyOCIO_ClampTransform_methods,              //tp_methods
        0,                                          //tp_members
        0,                                          //tp_getset
        &PyOCIO_TransformType,                      //tp_base
        0,                                          //tp_dict
        0,                                          //tp_descr_get
        0,                                          //tp_descr_set
        0,                                          //tp_dictoffset
        (initproc) PyOCIO_ClampTransform_init,      //tp_init
        0,                                          //tp_alloc
        0,                                          //tp_new
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };

    namespace
    {

        ///////////////////////////////////////////////////////////////////////
        ///

        int PyOCIO_ClampTransform_init(PyOCIO_Transform *self, PyObject *args, PyObject *kwds)
        {
            OCIO_PYTRY_ENTER()
            static const char *kwlist[] = { "min", "max", "direction", NULL };
            PyObject* pymin = Py_None;
            PyObject* pymax = Py_None;
            char* direction = NULL;
            if(!PyArg_ParseTupleAndKeywords(args, kwds, "|OOs",
                const_cast<char **>(kwlist),
                &pymin, &pymax, &direction )) return -1;
            ClampTransformRcPtr ptr(ClampTransform::Create());
            int ret = BuildPyTransformObject<ClampTransformRcPtr>(self, ptr);
            if(pymin != Py_None)
            {
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pymin, data) ||
                    (data.size() != 4))
                {
                    PyErr_SetString(PyExc_TypeError, "Min argument must be a float array, size 4");
                    return -1;
                }
                ptr->setMin(&data[0]);
            }
            if(pymax != Py_None)
            {
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pymax, data) ||
                    (data.size() != 4))
                {
                    PyErr_SetString(PyExc_TypeError, "Max argument must be a float array, size 4");
                    return -1;
                }
                ptr->setMax(&data[0]);
            }
            if(direction) ptr->setDirection(TransformDirectionFromString(direction));
            return ret;
            OCIO_PYTRY_EXIT(-1)
        }

        PyObject * PyOCIO_ClampTransform_getMin(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstClampTransformRcPtr transform = GetConstClampTransform(self);
            std::vector<float> data(4);
            transform->getMin(&data[0]);
            return CreatePyListFromFloatVector(data);
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_ClampTransform_getMax(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstClampTransformRcPtr transform = GetConstClampTransform(self);
            std::vector<float> data(4);
            transform->getMax(&data[0]);
            return CreatePyListFromFloatVector(data);
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_ClampTransform_setMin(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pyData = 0;
            if (!PyArg_ParseTuple(args, "O:setMin", &pyData)) return NULL;
            ClampTransformRcPtr transform = GetEditableClampTransform(self);
            std::vector<float> data;
            if(!FillFloatVectorFromPySequence(pyData, data) || (data.size() != 4))
            {
                PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 4");
                return 0;
            }
            transform->setMin(&data[0]);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_ClampTransform_setMax(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pyData = 0;
            if (!PyArg_ParseTuple(args, "O:setMax", &pyData)) return NULL;
            ClampTransformRcPtr transform = GetEditableClampTransform(self);
            std::vector<float> data;
            if(!FillFloatVectorFromPySequence(pyData, data) || (data.size() != 4))
            {
                PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 4");
                return 0;
            }
            transform->setMax(&data[0]);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
    }
}
OCIO_NAMESPACE_EXIT
