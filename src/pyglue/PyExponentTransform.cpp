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

#include "PyTransform.h"
#include "PyUtil.h"
#include "PyDoc.h"

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    bool AddExponentTransformObjectToModule( PyObject* m )
    {
        PyOCIO_ExponentTransformType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_ExponentTransformType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_ExponentTransformType );
        PyModule_AddObject(m, "ExponentTransform",
                (PyObject *)&PyOCIO_ExponentTransformType);
        
        return true;
    }
    
    bool IsPyExponentTransform(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return PyObject_TypeCheck(pyobject, &PyOCIO_ExponentTransformType);
    }
    
    ConstExponentTransformRcPtr GetConstExponentTransform(PyObject * pyobject, bool allowCast)
    {
        ConstExponentTransformRcPtr transform = \
            DynamicPtrCast<const ExponentTransform>(GetConstTransform(pyobject, allowCast));
        if(!transform)
        {
            throw Exception("PyObject must be a valid OCIO.ExponentTransform.");
        }
        return transform;
    }
    
    ExponentTransformRcPtr GetEditableExponentTransform(PyObject * pyobject)
    {
        ExponentTransformRcPtr transform = \
            DynamicPtrCast<ExponentTransform>(GetEditableTransform(pyobject));
        if(!transform)
        {
            throw Exception("PyObject must be a valid OCIO.ExponentTransform.");
        }
        return transform;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    namespace
    {
        int PyOCIO_ExponentTransform_init( PyOCIO_Transform * self, PyObject * args, PyObject * kwds );
        
        PyObject * PyOCIO_ExponentTransform_getValue( PyObject * self );
        PyObject * PyOCIO_ExponentTransform_setValue( PyObject * self,  PyObject *args );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_ExponentTransform_methods[] = {
            {"getValue",
            (PyCFunction) PyOCIO_ExponentTransform_getValue, METH_NOARGS, EXPONENTTRANSFORM_GETVALUE__DOC__ },
            {"setValue",
            PyOCIO_ExponentTransform_setValue, METH_VARARGS, EXPONENTTRANSFORM_SETVALUE__DOC__ },
            {NULL, NULL, 0, NULL}
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ExponentTransformType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.ExponentTransform",                   //tp_name
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
        int PyOCIO_ExponentTransform_init( PyOCIO_Transform *self,
            PyObject * args, PyObject * kwds )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            
            self->constcppobj = new ConstTransformRcPtr();
            self->cppobj = new TransformRcPtr();
            self->isconst = true;
            
            // Parse optional kwargs
            PyObject * pyvalue = Py_None;
            char * direction = NULL;
            
            static const char *kwlist[] = {
                "value",
                "direction",
                NULL
            };
            
            if(!PyArg_ParseTupleAndKeywords(args, kwds, "|Os",
                const_cast<char **>(kwlist),
                &pyvalue, &direction )) return -1;
            
            try
            {
                ExponentTransformRcPtr transform = ExponentTransform::Create();
                *self->cppobj = transform;
                self->isconst = false;
                
                if(pyvalue != Py_None)
                {
                    std::vector<float> data;
                    if(!FillFloatVectorFromPySequence(pyvalue, data) || (data.size() != 4))
                    {
                        PyErr_SetString(PyExc_TypeError, "Value argument must be a float array, size 4");
                        return -1;
                    }
                    
                    transform->setValue( &data[0] );
                }
                if(direction) transform->setDirection(TransformDirectionFromString(direction));
                return 0;
            }
            catch ( const std::exception & e )
            {
                std::string message = "Cannot create ExponentTransform: ";
                message += e.what();
                PyErr_SetString( PyExc_RuntimeError, message.c_str() );
                return -1;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        ///
        
        PyObject * PyOCIO_ExponentTransform_getValue( PyObject * self )
        {
            try
            {
                ConstExponentTransformRcPtr transform = GetConstExponentTransform(self, true);
                std::vector<float> data(4);
                transform->getValue(&data[0]);
                return CreatePyListFromFloatVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ExponentTransform_setValue( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O:setValue", &pyData)) return NULL;
                ExponentTransformRcPtr transform = GetEditableExponentTransform(self);
                
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pyData, data) || (data.size() != 4))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 4");
                    return 0;
                }
                
                transform->setValue( &data[0] );
                
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
