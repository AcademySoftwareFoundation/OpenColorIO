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

OCIO_NAMESPACE_ENTER
{
    
    ConstGpuShaderDescRcPtr GetConstGpuShaderDesc(PyObject * pyobject)
    {
        return GetConstPyOCIO<PyOCIO_GpuShaderDesc,
            ConstGpuShaderDescRcPtr>(pyobject, PyOCIO_GpuShaderDescType);
    }
    
    GpuShaderDescRcPtr GetEditableGpuShaderDesc(PyObject * pyobject)
    {
        return GetEditablePyOCIO<PyOCIO_GpuShaderDesc,
            GpuShaderDescRcPtr>(pyobject, PyOCIO_GpuShaderDescType);
    }
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_GpuShaderDesc_init(PyOCIO_GpuShaderDesc * self, PyObject * args, PyObject * kwds);
        void PyOCIO_GpuShaderDesc_delete(PyOCIO_GpuShaderDesc * self, PyObject * args);
        PyObject * PyOCIO_GpuShaderDesc_setLanguage(PyObject * self, PyObject * args);
        PyObject * PyOCIO_GpuShaderDesc_getLanguage(PyObject * self);
        PyObject * PyOCIO_GpuShaderDesc_setFunctionName(PyObject * self, PyObject * args);
        PyObject * PyOCIO_GpuShaderDesc_getFunctionName(PyObject * self);
        PyObject * PyOCIO_GpuShaderDesc_setLut3DEdgeLen(PyObject * self, PyObject * args);
        PyObject * PyOCIO_GpuShaderDesc_getLut3DEdgeLen(PyObject * self);
        PyObject * PyOCIO_GpuShaderDesc_getCacheID(PyObject * self);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_GpuShaderDesc_methods[] = {
            { "setLanguage",
            PyOCIO_GpuShaderDesc_setLanguage, METH_VARARGS, GPUSHADERDESC_SETLANGUAGE__DOC__ },
            { "getLanguage",
            (PyCFunction) PyOCIO_GpuShaderDesc_getLanguage, METH_NOARGS, GPUSHADERDESC_GETLANGUAGE__DOC__ },
            { "setFunctionName",
            PyOCIO_GpuShaderDesc_setFunctionName, METH_VARARGS, GPUSHADERDESC_SETFUNCTIONNAME__DOC__ },            
            { "getFunctionName",
            (PyCFunction) PyOCIO_GpuShaderDesc_getFunctionName, METH_NOARGS, GPUSHADERDESC_GETFUNCTIONNAME__DOC__ },
            { "setLut3DEdgeLen",
            PyOCIO_GpuShaderDesc_setLut3DEdgeLen, METH_VARARGS, GPUSHADERDESC_SETLUT3DEDGELEN__DOC__ },
            { "getLut3DEdgeLen",
            (PyCFunction) PyOCIO_GpuShaderDesc_getLut3DEdgeLen, METH_NOARGS, GPUSHADERDESC_GETLUT3DEDGELEN__DOC__ },
            { "getCacheID",
            (PyCFunction) PyOCIO_GpuShaderDesc_getCacheID, METH_NOARGS, GPUSHADERDESC_GETCACHEID__DOC__ },
            { NULL, NULL, 0, NULL }
        };
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_GpuShaderDescType = {
        PyVarObject_HEAD_INIT(NULL, 0)              //ob_size
        OCIO_PYTHON_NAMESPACE(GpuShaderDesc),       //tp_name
        sizeof(PyOCIO_GpuShaderDesc),               //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_GpuShaderDesc_delete,    //tp_dealloc
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
        GPUSHADERDESC__DOC__,                       //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_GpuShaderDesc_methods,               //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_GpuShaderDesc_init,       //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        void GpuShaderDesc_deleter(GpuShaderDesc* d)
        {
            delete d;
        }
        
        int PyOCIO_GpuShaderDesc_init(PyOCIO_GpuShaderDesc* self, PyObject * /*args*/, PyObject * /*kwds*/)
        {
            OCIO_PYTRY_ENTER()
            return BuildPyObject<PyOCIO_GpuShaderDesc, ConstGpuShaderDescRcPtr,
                GpuShaderDescRcPtr>(self, GpuShaderDescRcPtr(new GpuShaderDesc(),
                    GpuShaderDesc_deleter));
            OCIO_PYTRY_EXIT(-1)
        }
        
        void PyOCIO_GpuShaderDesc_delete(PyOCIO_GpuShaderDesc *self, PyObject * /*args*/)
        {
            DeletePyObject<PyOCIO_GpuShaderDesc>(self);
        }
        
        PyObject * PyOCIO_GpuShaderDesc_setLanguage(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* lang = 0;
            if (!PyArg_ParseTuple(args, "s:setLanguage",
                &lang)) return NULL;
            GpuShaderDescRcPtr desc = GetEditableGpuShaderDesc(self);
            desc->setLanguage(GpuLanguageFromString(lang));
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_GpuShaderDesc_getLanguage(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstGpuShaderDescRcPtr desc = GetConstGpuShaderDesc(self);
            GpuLanguage lang = desc->getLanguage();
            return PyString_FromString(GpuLanguageToString(lang));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_GpuShaderDesc_setFunctionName(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            if (!PyArg_ParseTuple(args, "s:setFunctionName",
                &name)) return NULL;
            GpuShaderDescRcPtr desc = GetEditableGpuShaderDesc(self);
            desc->setFunctionName(name);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_GpuShaderDesc_getFunctionName(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstGpuShaderDescRcPtr desc = GetConstGpuShaderDesc(self);
            return PyString_FromString(desc->getFunctionName());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_GpuShaderDesc_setLut3DEdgeLen(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int len = 0;
            if (!PyArg_ParseTuple(args,"i:setLut3DEdgeLen",
                &len)) return NULL;
            GpuShaderDescRcPtr desc = GetEditableGpuShaderDesc(self);
            desc->setLut3DEdgeLen(len);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_GpuShaderDesc_getLut3DEdgeLen(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstGpuShaderDescRcPtr desc = GetConstGpuShaderDesc(self);
            return PyInt_FromLong(desc->getLut3DEdgeLen());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_GpuShaderDesc_getCacheID(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstGpuShaderDescRcPtr desc = GetConstGpuShaderDesc(self);
            return PyString_FromString(desc->getCacheID());
            OCIO_PYTRY_EXIT(NULL)
        }
        
    } // anon namespace
    
}
OCIO_NAMESPACE_EXIT
