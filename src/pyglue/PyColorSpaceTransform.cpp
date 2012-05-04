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
    
    bool AddColorSpaceTransformObjectToModule( PyObject* m )
    {
        PyOCIO_ColorSpaceTransformType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_ColorSpaceTransformType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_ColorSpaceTransformType );
        PyModule_AddObject(m, "ColorSpaceTransform",
                (PyObject *)&PyOCIO_ColorSpaceTransformType);
        
        return true;
    }
    
    bool IsPyColorSpaceTransform(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return PyObject_TypeCheck(pyobject, &PyOCIO_ColorSpaceTransformType);
    }
    
    ConstColorSpaceTransformRcPtr GetConstColorSpaceTransform(PyObject * pyobject, bool allowCast)
    {
        ConstColorSpaceTransformRcPtr transform = \
            DynamicPtrCast<const ColorSpaceTransform>(GetConstTransform(pyobject, allowCast));
        if(!transform)
        {
            throw Exception("PyObject must be a valid OCIO.ColorSpaceTransform.");
        }
        return transform;
    }
    
    ColorSpaceTransformRcPtr GetEditableColorSpaceTransform(PyObject * pyobject)
    {
        ColorSpaceTransformRcPtr transform = \
            DynamicPtrCast<ColorSpaceTransform>(GetEditableTransform(pyobject));
        if(!transform)
        {
            throw Exception("PyObject must be a valid OCIO.ColorSpaceTransform.");
        }
        return transform;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    namespace
    {
        int PyOCIO_ColorSpaceTransform_init( PyOCIO_Transform * self, PyObject * args, PyObject * kwds );
        
        PyObject * PyOCIO_ColorSpaceTransform_getSrc( PyObject * self );
        PyObject * PyOCIO_ColorSpaceTransform_setSrc( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColorSpaceTransform_getDst( PyObject * self );
        PyObject * PyOCIO_ColorSpaceTransform_setDst( PyObject * self,  PyObject *args );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_ColorSpaceTransform_methods[] = {
            {"getSrc",
            (PyCFunction) PyOCIO_ColorSpaceTransform_getSrc, METH_NOARGS, COLORSPACETRANSFORM_GETSRC__DOC__ },
            {"setSrc",
            PyOCIO_ColorSpaceTransform_setSrc, METH_VARARGS, COLORSPACETRANSFORM_SETSRC__DOC__ },
            {"getDst",
            (PyCFunction) PyOCIO_ColorSpaceTransform_getDst, METH_NOARGS, COLORSPACETRANSFORM_GETDST__DOC__ },
            {"setDst",
            PyOCIO_ColorSpaceTransform_setDst, METH_VARARGS, COLORSPACETRANSFORM_SETDST__DOC__ },
            {NULL, NULL, 0, NULL}
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ColorSpaceTransformType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.ColorSpaceTransform",                 //tp_name
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
        COLORSPACETRANSFORM__DOC__,                 //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_ColorSpaceTransform_methods,         //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        &PyOCIO_TransformType,                      //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_ColorSpaceTransform_init, //tp_init 
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
        int PyOCIO_ColorSpaceTransform_init( PyOCIO_Transform *self,
            PyObject * args, PyObject * kwds )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            
            self->constcppobj = new ConstTransformRcPtr();
            self->cppobj = new TransformRcPtr();
            self->isconst = true;
            
            // Parse optional kwargs
            char * src = NULL;
            char * dst = NULL;
            char * direction = NULL;
            
            static const char *kwlist[] = {
                "src",
                "dst",
                "direction",
                NULL
            };
            
            if(!PyArg_ParseTupleAndKeywords(args, kwds, "|sss",
                const_cast<char **>(kwlist),
                &src, &dst, &direction )) return -1;
            
            try
            {
                ColorSpaceTransformRcPtr transform = ColorSpaceTransform::Create();
                *self->cppobj = transform;
                self->isconst = false;
                
                if(src) transform->setSrc(src);
                if(dst) transform->setDst(dst);
                if(direction) transform->setDirection(TransformDirectionFromString(direction));
                
                return 0;
            }
            catch ( const std::exception & e )
            {
                std::string message = "Cannot create ColorSpaceTransform: ";
                message += e.what();
                PyErr_SetString( PyExc_RuntimeError, message.c_str() );
                return -1;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        ///
        
        PyObject * PyOCIO_ColorSpaceTransform_getSrc( PyObject * self )
        {
            try
            {
                ConstColorSpaceTransformRcPtr transform = GetConstColorSpaceTransform(self, true);
                return PyString_FromString( transform->getSrc() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColorSpaceTransform_setSrc( PyObject * self, PyObject * args )
        {
            try
            {
                const char * str = 0;
                if (!PyArg_ParseTuple(args,"s:setSrc",
                    &str)) return NULL;
                
                ColorSpaceTransformRcPtr transform = GetEditableColorSpaceTransform(self);
                transform->setSrc( str );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColorSpaceTransform_getDst( PyObject * self )
        {
            try
            {
                ConstColorSpaceTransformRcPtr transform = GetConstColorSpaceTransform(self, true);
                return PyString_FromString( transform->getDst() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColorSpaceTransform_setDst( PyObject * self, PyObject * args )
        {
            try
            {
                const char * str = 0;
                if (!PyArg_ParseTuple(args,"s:setDst",
                    &str)) return NULL;
                
                ColorSpaceTransformRcPtr transform = GetEditableColorSpaceTransform(self);
                transform->setDst( str );
                
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
