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

#define GetConstExponentTransform(pyobject) GetConstPyOCIO<PyOCIO_Transform, \
    ConstExponentTransformRcPtr, ExponentTransform>(pyobject, \
    PyOCIO_ExponentTransformType)

#define GetEditableExponentTransform(pyobject) GetEditablePyOCIO<PyOCIO_Transform, \
    ExponentTransformRcPtr, ExponentTransform>(pyobject, \
    PyOCIO_ExponentTransformType)

OCIO_NAMESPACE_ENTER
{
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_ExponentTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds);
        PyObject * PyOCIO_ExponentTransform_getValue(PyObject * self);
        PyObject * PyOCIO_ExponentTransform_setValue(PyObject * self, PyObject * args);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_ExponentTransform_methods[] = {
            { "getValue",
            (PyCFunction) PyOCIO_ExponentTransform_getValue, METH_NOARGS, EXPONENTTRANSFORM_GETVALUE__DOC__ },
            { "setValue",
            PyOCIO_ExponentTransform_setValue, METH_VARARGS, EXPONENTTRANSFORM_SETVALUE__DOC__ },
            { NULL, NULL, 0, NULL }
        };
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ExponentTransformType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        OCIO_PYTHON_NAMESPACE(ExponentTransform),   //tp_name
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
        EXPONENTTRANSFORM__DOC__,                   //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_ExponentTransform_methods,           //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        &PyOCIO_TransformType,                      //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_ExponentTransform_init,   //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_ExponentTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds)
        {
            OCIO_PYTRY_ENTER()
            static const char *kwlist[] = { "value", "direction", NULL };
            PyObject* pyvalue = Py_None;
            char* direction = NULL;
            if(!PyArg_ParseTupleAndKeywords(args, kwds, "|Os",
                const_cast<char **>(kwlist),
                &pyvalue, &direction )) return -1;
            ExponentTransformRcPtr ptr = ExponentTransform::Create();
            int ret = BuildPyTransformObject<ExponentTransformRcPtr>(self, ptr);
            if(pyvalue != Py_None)
            {
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pyvalue, data) ||
                    (data.size() != 4))
                {
                    PyErr_SetString(PyExc_TypeError, "Value argument must be a float array, size 4");
                    return -1;
                }
                ptr->setValue(&data[0]);
            }
            if(direction) ptr->setDirection(TransformDirectionFromString(direction));
            return ret;
            OCIO_PYTRY_EXIT(-1)
        }
        
        PyObject * PyOCIO_ExponentTransform_getValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstExponentTransformRcPtr transform = GetConstExponentTransform(self);
            std::vector<float> data(4);
            transform->getValue(&data[0]);
            return CreatePyListFromFloatVector(data);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ExponentTransform_setValue(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pyData = 0;
            if (!PyArg_ParseTuple(args, "O:setValue",
                &pyData)) return NULL;
            ExponentTransformRcPtr transform = GetEditableExponentTransform(self);
            std::vector<float> data;
            if(!FillFloatVectorFromPySequence(pyData, data) || (data.size() != 4)) {
                PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 4");
                return 0;
            }
            transform->setValue(&data[0]);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
    }

}
OCIO_NAMESPACE_EXIT
