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
    
    bool AddCineonLogToLinTransformObjectToModule( PyObject* m )
    {
        PyOCIO_CineonLogToLinTransformType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_CineonLogToLinTransformType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_CineonLogToLinTransformType );
        PyModule_AddObject(m, "CineonLogToLinTransform",
                (PyObject *)&PyOCIO_CineonLogToLinTransformType);
        
        return true;
    }
    
    bool IsPyCineonLogToLinTransform(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return PyObject_TypeCheck(pyobject, &PyOCIO_CineonLogToLinTransformType);
    }
    
    ConstCineonLogToLinTransformRcPtr GetConstCineonLogToLinTransform(PyObject * pyobject, bool allowCast)
    {
        ConstCineonLogToLinTransformRcPtr transform = \
            DynamicPtrCast<const CineonLogToLinTransform>(GetConstTransform(pyobject, allowCast));
        if(!transform)
        {
            throw Exception("PyObject must be a valid OCIO.CineonLogToLinTransform.");
        }
        return transform;
    }
    
    CineonLogToLinTransformRcPtr GetEditableCineonLogToLinTransform(PyObject * pyobject)
    {
        CineonLogToLinTransformRcPtr transform = \
            DynamicPtrCast<CineonLogToLinTransform>(GetEditableTransform(pyobject));
        if(!transform)
        {
            throw Exception("PyObject must be a valid OCIO.CineonLogToLinTransform.");
        }
        return transform;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    namespace
    {
        int PyOCIO_CineonLogToLinTransform_init( PyOCIO_Transform * self, PyObject * args, PyObject * kwds );
        
        PyObject * PyOCIO_CineonLogToLinTransform_getMaxAimDensity( PyObject * self );
        PyObject * PyOCIO_CineonLogToLinTransform_getNegGamma( PyObject * self );
        PyObject * PyOCIO_CineonLogToLinTransform_getNegGammaAsLogOffset( PyObject * self );
        PyObject * PyOCIO_CineonLogToLinTransform_getNegGrayReference( PyObject * self );
        PyObject * PyOCIO_CineonLogToLinTransform_getLinearGrayReference( PyObject * self );
        
        PyObject * PyOCIO_CineonLogToLinTransform_setMaxAimDensity( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_CineonLogToLinTransform_setNegGamma( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_CineonLogToLinTransform_setNegGammaAsLogOffset( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_CineonLogToLinTransform_setNegGrayReference( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_CineonLogToLinTransform_setLinearGrayReference( PyObject * self,  PyObject *args );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_CineonLogToLinTransform_methods[] = {
            {"getMaxAimDensity", (PyCFunction) PyOCIO_CineonLogToLinTransform_getMaxAimDensity, METH_NOARGS, "" },
            {"getNegGamma", (PyCFunction) PyOCIO_CineonLogToLinTransform_getNegGamma, METH_NOARGS, "" },
            {"getNegGammaAsLogOffset", (PyCFunction) PyOCIO_CineonLogToLinTransform_getNegGammaAsLogOffset, METH_NOARGS, "" },
            {"getNegGrayReference", (PyCFunction) PyOCIO_CineonLogToLinTransform_getNegGrayReference, METH_NOARGS, "" },
            {"getLinearGrayReference", (PyCFunction) PyOCIO_CineonLogToLinTransform_getLinearGrayReference, METH_NOARGS, "" },
            
            {"setMaxAimDensity", PyOCIO_CineonLogToLinTransform_setMaxAimDensity, METH_VARARGS, "" },
            {"setNegGamma", PyOCIO_CineonLogToLinTransform_setNegGamma, METH_VARARGS, "" },
            {"setNegGammaAsLogOffset", PyOCIO_CineonLogToLinTransform_setNegGammaAsLogOffset, METH_VARARGS, "" },
            {"setNegGrayReference", PyOCIO_CineonLogToLinTransform_setNegGrayReference, METH_VARARGS, "" },
            {"setLinearGrayReference", PyOCIO_CineonLogToLinTransform_setLinearGrayReference, METH_VARARGS, "" },
            
            {NULL, NULL, 0, NULL}
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_CineonLogToLinTransformType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.CineonLogToLinTransform",                        //tp_name
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
        "CineonLogToLinTransform",                             //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_CineonLogToLinTransform_methods,                //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        &PyOCIO_TransformType,                      //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_CineonLogToLinTransform_init,        //tp_init 
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
        int PyOCIO_CineonLogToLinTransform_init( PyOCIO_Transform *self, PyObject * /*args*/, PyObject * /*kwds*/ )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            
            self->constcppobj = new ConstTransformRcPtr();
            self->cppobj = new TransformRcPtr();
            self->isconst = true;
            
            try
            {
                *self->cppobj = CineonLogToLinTransform::Create();
                self->isconst = false;
                return 0;
            }
            catch ( const std::exception & e )
            {
                std::string message = "Cannot create CineonLogToLinTransform: ";
                message += e.what();
                PyErr_SetString( PyExc_RuntimeError, message.c_str() );
                return -1;
            }
        }
        
        
        
        
        ////////////////////////////////////////////////////////////////////////
        
        
        PyObject * PyOCIO_CineonLogToLinTransform_getMaxAimDensity( PyObject * self )
        {
            try
            {
                ConstCineonLogToLinTransformRcPtr transform = GetConstCineonLogToLinTransform(self, true);
                std::vector<float> data(3);
                transform->getMaxAimDensity(&data[0]);
                return CreatePyListFromFloatVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CineonLogToLinTransform_getNegGamma( PyObject * self )
        {
            try
            {
                ConstCineonLogToLinTransformRcPtr transform = GetConstCineonLogToLinTransform(self, true);
                std::vector<float> data(3);
                transform->getNegGamma(&data[0]);
                return CreatePyListFromFloatVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CineonLogToLinTransform_getNegGammaAsLogOffset( PyObject * self )
        {
            try
            {
                ConstCineonLogToLinTransformRcPtr transform = GetConstCineonLogToLinTransform(self, true);
                std::vector<float> data(3);
                transform->getNegGammaAsLogOffset(&data[0]);
                return CreatePyListFromFloatVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CineonLogToLinTransform_getNegGrayReference( PyObject * self )
        {
            try
            {
                ConstCineonLogToLinTransformRcPtr transform = GetConstCineonLogToLinTransform(self, true);
                std::vector<float> data(3);
                transform->getNegGrayReference(&data[0]);
                return CreatePyListFromFloatVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CineonLogToLinTransform_getLinearGrayReference( PyObject * self )
        {
            try
            {
                ConstCineonLogToLinTransformRcPtr transform = GetConstCineonLogToLinTransform(self, true);
                std::vector<float> data(3);
                transform->getLinearGrayReference(&data[0]);
                return CreatePyListFromFloatVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        
        ////////////////////////////////////////////////////////////////////////
        
        
        PyObject * PyOCIO_CineonLogToLinTransform_setMaxAimDensity( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O:setMaxAimDensity", &pyData)) return NULL;
                CineonLogToLinTransformRcPtr transform = GetEditableCineonLogToLinTransform(self);
                
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pyData, data) || (data.size() != 3))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 3");
                    return 0;
                }
                
                transform->setMaxAimDensity( &data[0] );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CineonLogToLinTransform_setNegGamma( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O:setNegGamma", &pyData)) return NULL;
                CineonLogToLinTransformRcPtr transform = GetEditableCineonLogToLinTransform(self);
                
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pyData, data) || (data.size() != 3))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 3");
                    return 0;
                }
                
                transform->setNegGamma( &data[0] );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CineonLogToLinTransform_setNegGammaAsLogOffset( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O:setNegGammaAsLogOffset", &pyData)) return NULL;
                CineonLogToLinTransformRcPtr transform = GetEditableCineonLogToLinTransform(self);
                
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pyData, data) || (data.size() != 3))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 3");
                    return 0;
                }
                
                transform->setNegGammaAsLogOffset( &data[0] );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CineonLogToLinTransform_setNegGrayReference( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O:setNegGrayReference", &pyData)) return NULL;
                CineonLogToLinTransformRcPtr transform = GetEditableCineonLogToLinTransform(self);
                
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pyData, data) || (data.size() != 3))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 3");
                    return 0;
                }
                
                transform->setNegGrayReference( &data[0] );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CineonLogToLinTransform_setLinearGrayReference( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O:setLinearGrayReference", &pyData)) return NULL;
                CineonLogToLinTransformRcPtr transform = GetEditableCineonLogToLinTransform(self);
                
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pyData, data) || (data.size() != 3))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 3");
                    return 0;
                }
                
                transform->setLinearGrayReference( &data[0] );
                
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
