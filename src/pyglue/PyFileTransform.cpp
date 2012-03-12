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
    
    bool AddFileTransformObjectToModule( PyObject* m )
    {
        PyOCIO_FileTransformType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_FileTransformType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_FileTransformType );
        PyModule_AddObject(m, "FileTransform",
                (PyObject *)&PyOCIO_FileTransformType);
        
        return true;
    }
    
    bool IsPyFileTransform(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return PyObject_TypeCheck(pyobject, &PyOCIO_FileTransformType);
    }
    
    ConstFileTransformRcPtr GetConstFileTransform(PyObject * pyobject, bool allowCast)
    {
        ConstFileTransformRcPtr transform = \
            DynamicPtrCast<const FileTransform>(GetConstTransform(pyobject, allowCast));
        if(!transform)
        {
            throw Exception("PyObject must be a valid OCIO.FileTransform.");
        }
        return transform;
    }
    
    FileTransformRcPtr GetEditableFileTransform(PyObject * pyobject)
    {
        FileTransformRcPtr transform = \
            DynamicPtrCast<FileTransform>(GetEditableTransform(pyobject));
        if(!transform)
        {
            throw Exception("PyObject must be a valid OCIO.FileTransform.");
        }
        return transform;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    namespace
    {
        int PyOCIO_FileTransform_init( PyOCIO_Transform * self, PyObject * args, PyObject * kwds );
        
        PyObject * PyOCIO_FileTransform_getSrc( PyObject * self );
        PyObject * PyOCIO_FileTransform_setSrc( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_FileTransform_getCCCId( PyObject * self );
        PyObject * PyOCIO_FileTransform_setCCCId( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_FileTransform_getInterpolation( PyObject * self );
        PyObject * PyOCIO_FileTransform_setInterpolation( PyObject * self,  PyObject *args );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_FileTransform_methods[] = {
            {"getSrc",
            (PyCFunction) PyOCIO_FileTransform_getSrc, METH_NOARGS, FILETRANSFORM_GETSRC__DOC__ },
            {"setSrc",
            PyOCIO_FileTransform_setSrc, METH_VARARGS, FILETRANSFORM_SETSRC__DOC__ },
            {"getCCCId",
            (PyCFunction) PyOCIO_FileTransform_getCCCId, METH_NOARGS, FILETRANSFORM_GETCCCID__DOC__ },
            {"setCCCId",
            PyOCIO_FileTransform_setCCCId, METH_VARARGS, FILETRANSFORM_SETCCCID__DOC__ },
            {"getInterpolation",
            (PyCFunction) PyOCIO_FileTransform_getInterpolation, METH_NOARGS, FILETRANSFORM_GETINTERPOLATION__DOC__ },
            {"setInterpolation",
            PyOCIO_FileTransform_setInterpolation, METH_VARARGS, FILETRANSFORM_SETINTERPOLATION__DOC__ },
            {NULL, NULL, 0, NULL}
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_FileTransformType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.FileTransform",                       //tp_name
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
        FILETRANSFORM__DOC__,                       //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_FileTransform_methods,               //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        &PyOCIO_TransformType,                      //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_FileTransform_init,       //tp_init 
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
        int PyOCIO_FileTransform_init( PyOCIO_Transform *self,
            PyObject * args, PyObject * kwds )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            
            self->constcppobj = new ConstTransformRcPtr();
            self->cppobj = new TransformRcPtr();
            self->isconst = true;
            
            // Parse optional kwargs
            char * src = NULL;
            char * cccid = NULL;
            char * interpolation = NULL;
            char * direction = NULL;
            
            static const char *kwlist[] = {
                "src",
                "cccid",
                "interpolation",
                "direction",
                NULL
            };
            
            if(!PyArg_ParseTupleAndKeywords(args, kwds, "|ssss",
                const_cast<char **>(kwlist),
                &src, &cccid, &interpolation, &direction )) return -1;
            
            try
            {
                FileTransformRcPtr transform = FileTransform::Create();
                *self->cppobj = transform;
                self->isconst = false;
                
                if(src) transform->setSrc(src);
                if(cccid) transform->setCCCId(cccid);
                if(interpolation) transform->setInterpolation(InterpolationFromString(interpolation));
                if(direction) transform->setDirection(TransformDirectionFromString(direction));
                
                return 0;
            }
            catch ( const std::exception & e )
            {
                std::string message = "Cannot create FileTransform: ";
                message += e.what();
                PyErr_SetString( PyExc_RuntimeError, message.c_str() );
                return -1;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        ///
        
        PyObject * PyOCIO_FileTransform_getSrc( PyObject * self )
        {
            try
            {
                ConstFileTransformRcPtr transform = GetConstFileTransform(self, true);
                return PyString_FromString( transform->getSrc() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_FileTransform_setSrc( PyObject * self, PyObject * args )
        {
            try
            {
                char * src = 0;
                if (!PyArg_ParseTuple(args,"s:setSrc", &src)) return NULL;
                
                FileTransformRcPtr transform = GetEditableFileTransform(self);
                transform->setSrc( src );
                
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
        
        PyObject * PyOCIO_FileTransform_getCCCId( PyObject * self )
        {
            try
            {
                ConstFileTransformRcPtr transform = GetConstFileTransform(self, true);
                return PyString_FromString( transform->getCCCId() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_FileTransform_setCCCId( PyObject * self, PyObject * args )
        {
            try
            {
                char * id = 0;
                if (!PyArg_ParseTuple(args,"s:setCCCId", &id)) return NULL;
                
                FileTransformRcPtr transform = GetEditableFileTransform(self);
                transform->setCCCId( id );
                
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
        
        PyObject * PyOCIO_FileTransform_getInterpolation( PyObject * self )
        {
            try
            {
                ConstFileTransformRcPtr transform = GetConstFileTransform(self, true);
                Interpolation interp = transform->getInterpolation();
                return PyString_FromString( InterpolationToString( interp ) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_FileTransform_setInterpolation( PyObject * self, PyObject * args )
        {
            try
            {
                Interpolation interp;
                if (!PyArg_ParseTuple(args,"O&:setInterpolation",
                    ConvertPyObjectToInterpolation, &interp)) return NULL;
                
                FileTransformRcPtr transform = GetEditableFileTransform(self);
                transform->setInterpolation(interp);
                
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
