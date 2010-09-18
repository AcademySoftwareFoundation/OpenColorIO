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


#include <OpenColorIO/OpenColorIO.h>

#include "PyTransform.h"
#include "PyUtil.h"

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    bool AddMatrixTransformObjectToModule( PyObject* m )
    {
        PyOCIO_MatrixTransformType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_MatrixTransformType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_MatrixTransformType );
        PyModule_AddObject(m, "MatrixTransform",
                (PyObject *)&PyOCIO_MatrixTransformType);
        
        return true;
    }
    
    bool IsPyMatrixTransform(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return PyObject_TypeCheck(pyobject, &PyOCIO_MatrixTransformType);
    }
    
    ConstMatrixTransformRcPtr GetConstMatrixTransform(PyObject * pyobject, bool allowCast)
    {
        ConstMatrixTransformRcPtr transform = \
            DynamicPtrCast<const MatrixTransform>(GetConstTransform(pyobject, allowCast));
        if(!transform)
        {
            throw Exception("PyObject must be a valid OCIO.MatrixTransform.");
        }
        return transform;
    }
    
    MatrixTransformRcPtr GetEditableMatrixTransform(PyObject * pyobject)
    {
        MatrixTransformRcPtr transform = \
            DynamicPtrCast<MatrixTransform>(GetEditableTransform(pyobject));
        if(!transform)
        {
            throw Exception("PyObject must be a valid OCIO.MatrixTransform.");
        }
        return transform;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    
    
    namespace
    {
        int PyOCIO_MatrixTransform_init( PyOCIO_Transform * self, PyObject * args, PyObject * kwds );
        
        PyObject * PyOCIO_MatrixTransform_getValue( PyObject * self );
        PyObject * PyOCIO_MatrixTransform_setValue( PyObject * self,  PyObject *args );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_MatrixTransform_methods[] = {
            {"getValue", (PyCFunction) PyOCIO_MatrixTransform_getValue, METH_NOARGS, "" },
            {"setValue", PyOCIO_MatrixTransform_setValue, METH_VARARGS, "" },
            {NULL, NULL, 0, NULL}
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_MatrixTransformType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.MatrixTransform",                     //tp_name
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
        "MatrixTransform",                          //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_MatrixTransform_methods,             //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        &PyOCIO_TransformType,                      //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_MatrixTransform_init,     //tp_init 
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
        int PyOCIO_MatrixTransform_init( PyOCIO_Transform *self, PyObject * /*args*/, PyObject * /*kwds*/ )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            
            self->constcppobj = new ConstTransformRcPtr();
            self->cppobj = new TransformRcPtr();
            self->isconst = true;
            
            try
            {
                *self->cppobj = MatrixTransform::Create();
                self->isconst = false;
                return 0;
            }
            catch ( const std::exception & e )
            {
                std::string message = "Cannot create MatrixTransform: ";
                message += e.what();
                PyErr_SetString( PyExc_RuntimeError, message.c_str() );
                return -1;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        
        PyObject * PyOCIO_MatrixTransform_getValue( PyObject * self )
        {
            try
            {
                ConstMatrixTransformRcPtr transform = GetConstMatrixTransform(self, true);
                
                std::vector<float> matrix(16);
                std::vector<float> offset(4);
                
                transform->getValue(&matrix[0], &offset[0]);
                
                PyObject* pymatrix = CreatePyListFromFloatVector(matrix);
                PyObject* pyoffset = CreatePyListFromFloatVector(offset);
                
                PyObject* pyreturnval = Py_BuildValue("(OO)", pymatrix, pyoffset);
                Py_DECREF(pymatrix);
                Py_DECREF(pyoffset);
                
                return pyreturnval;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_MatrixTransform_setValue( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pymatrix = 0;
                PyObject * pyoffset = 0;
                if (!PyArg_ParseTuple(args,"OO:setValue",
                    &pymatrix, &pyoffset)) return NULL;
                
                std::vector<float> matrix;
                std::vector<float> offset;
                
                if(!FillFloatVectorFromPySequence(pymatrix, matrix) || (matrix.size() != 16))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 16");
                    return 0;
                }
                
                if(!FillFloatVectorFromPySequence(pyoffset, offset) || (offset.size() != 4))
                {
                    PyErr_SetString(PyExc_TypeError, "Second argument must be a float array, size 4");
                    return 0;
                }
                
                MatrixTransformRcPtr transform = GetEditableMatrixTransform(self);
                transform->setValue(&matrix[0], &offset[0]);
                
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
