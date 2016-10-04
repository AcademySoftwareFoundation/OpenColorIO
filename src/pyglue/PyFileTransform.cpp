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

#define GetConstFileTransform(pyobject) GetConstPyOCIO<PyOCIO_Transform, \
    ConstFileTransformRcPtr, FileTransform>(pyobject, \
    PyOCIO_FileTransformType)

#define GetEditableFileTransform(pyobject) GetEditablePyOCIO<PyOCIO_Transform, \
    FileTransformRcPtr, FileTransform>(pyobject, \
    PyOCIO_FileTransformType);

OCIO_NAMESPACE_ENTER
{
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_FileTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds);
        PyObject * PyOCIO_FileTransform_getSrc(PyObject * self);
        PyObject * PyOCIO_FileTransform_setSrc(PyObject * self, PyObject * args);
        PyObject * PyOCIO_FileTransform_getCCCId(PyObject * self);
        PyObject * PyOCIO_FileTransform_setCCCId(PyObject * self, PyObject * args);
        PyObject * PyOCIO_FileTransform_getInterpolation(PyObject * self);
        PyObject * PyOCIO_FileTransform_setInterpolation(PyObject * self, PyObject * args);
        PyObject * PyOCIO_FileTransform_getNumFormats(PyObject * self);
        PyObject * PyOCIO_FileTransform_getFormatNameByIndex(PyObject * self, PyObject * args);
        PyObject * PyOCIO_FileTransform_getFormatExtensionByIndex(PyObject * self, PyObject * args);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_FileTransform_methods[] = {
            { "getSrc",
            (PyCFunction) PyOCIO_FileTransform_getSrc, METH_NOARGS, FILETRANSFORM_GETSRC__DOC__ },
            { "setSrc",
            PyOCIO_FileTransform_setSrc, METH_VARARGS, FILETRANSFORM_SETSRC__DOC__ },
            { "getCCCId",
            (PyCFunction) PyOCIO_FileTransform_getCCCId, METH_NOARGS, FILETRANSFORM_GETCCCID__DOC__ },
            { "setCCCId",
            PyOCIO_FileTransform_setCCCId, METH_VARARGS, FILETRANSFORM_SETCCCID__DOC__ },
            { "getInterpolation",
            (PyCFunction) PyOCIO_FileTransform_getInterpolation, METH_NOARGS, FILETRANSFORM_GETINTERPOLATION__DOC__ },
            { "setInterpolation",
            PyOCIO_FileTransform_setInterpolation, METH_VARARGS, FILETRANSFORM_SETINTERPOLATION__DOC__ },
            { "getNumFormats",
            (PyCFunction) PyOCIO_FileTransform_getNumFormats, METH_VARARGS, FILETRANSFORM_GETNUMFORMATS__DOC__ },
            { "getFormatNameByIndex",
            PyOCIO_FileTransform_getFormatNameByIndex, METH_VARARGS, FILETRANSFORM_GETFORMATNAMEBYINDEX__DOC__ },
            { "getFormatExtensionByIndex",
            PyOCIO_FileTransform_getFormatExtensionByIndex, METH_VARARGS, FILETRANSFORM_GETFORMATEXTENSIONBYINDEX__DOC__ },
            { NULL, NULL, 0, NULL }
        };
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_FileTransformType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        OCIO_PYTHON_NAMESPACE(FileTransform),       //tp_name
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
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_FileTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds)
        {
            OCIO_PYTRY_ENTER()
            FileTransformRcPtr ptr = FileTransform::Create();
            int ret = BuildPyTransformObject<FileTransformRcPtr>(self, ptr);
            char* src = NULL;
            char* cccid = NULL;
            char* interpolation = NULL;
            char* direction = NULL;
            static const char *kwlist[] = { "src", "cccid", "interpolation",
                "direction", NULL };
            if(!PyArg_ParseTupleAndKeywords(args, kwds, "|ssss",
                const_cast<char **>(kwlist),
                &src, &cccid, &interpolation, &direction)) return -1;
            if(src) ptr->setSrc(src);
            if(cccid) ptr->setCCCId(cccid);
            if(interpolation) ptr->setInterpolation(InterpolationFromString(interpolation));
            if(direction) ptr->setDirection(TransformDirectionFromString(direction));
            return ret;
            OCIO_PYTRY_EXIT(-1)
        }
        
        PyObject * PyOCIO_FileTransform_getSrc(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstFileTransformRcPtr transform = GetConstFileTransform(self);
            return PyString_FromString(transform->getSrc());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_FileTransform_setSrc(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* src = 0;
            if (!PyArg_ParseTuple(args, "s:setSrc",
                &src)) return NULL;
            FileTransformRcPtr transform = GetEditableFileTransform(self);
            transform->setSrc(src);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_FileTransform_getCCCId(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstFileTransformRcPtr transform = GetConstFileTransform(self);
            return PyString_FromString(transform->getCCCId());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_FileTransform_setCCCId(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* id = 0;
            if (!PyArg_ParseTuple(args,"s:setCCCId",
                &id)) return NULL;
            FileTransformRcPtr transform = GetEditableFileTransform(self);
            transform->setCCCId(id);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_FileTransform_getInterpolation(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstFileTransformRcPtr transform = GetConstFileTransform(self);
            Interpolation interp = transform->getInterpolation();
            return PyString_FromString(InterpolationToString(interp));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_FileTransform_setInterpolation(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            Interpolation interp;
            if (!PyArg_ParseTuple(args,"O&:setInterpolation",
                 ConvertPyObjectToInterpolation, &interp)) return NULL;
            FileTransformRcPtr transform = GetEditableFileTransform(self);
            transform->setInterpolation(interp);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_FileTransform_getNumFormats(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstFileTransformRcPtr transform = GetConstFileTransform(self);
            return PyInt_FromLong(transform->getNumFormats());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_FileTransform_getFormatNameByIndex(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int index = 0;
            if (!PyArg_ParseTuple(args,"i:getFormatNameByIndex",
                &index)) return NULL;
            ConstFileTransformRcPtr transform = GetConstFileTransform(self);
            return PyString_FromString(transform->getFormatNameByIndex(index));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_FileTransform_getFormatExtensionByIndex(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int index = 0;
            if (!PyArg_ParseTuple(args,"i:getFormatExtensionByIndex",
                &index)) return NULL;
            ConstFileTransformRcPtr transform = GetConstFileTransform(self);
            return PyString_FromString(transform->getFormatExtensionByIndex(index));
            OCIO_PYTRY_EXIT(NULL)
        }
        
    }

}
OCIO_NAMESPACE_EXIT
