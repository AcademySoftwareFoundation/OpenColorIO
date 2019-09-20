// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
        PyObject * PyOCIO_GpuShaderDesc_getCacheID(PyObject * self);
        PyObject * PyOCIO_GpuShaderDesc_finalize(PyObject * self);
        
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
            { "getCacheID",
            (PyCFunction) PyOCIO_GpuShaderDesc_getCacheID, METH_NOARGS, GPUSHADERDESC_GETCACHEID__DOC__ },
            { "finalize",
            (PyCFunction) PyOCIO_GpuShaderDesc_finalize, METH_NOARGS, GPUSHADERDESC_FINALIZE__DOC__ },
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
        
        int PyOCIO_GpuShaderDesc_init(PyOCIO_GpuShaderDesc* self, PyObject * /*args*/, PyObject * /*kwds*/)
        {
            OCIO_PYTRY_ENTER()
            return BuildPyObject<PyOCIO_GpuShaderDesc, ConstGpuShaderDescRcPtr,
                GpuShaderDescRcPtr>(self, GpuShaderDesc::CreateLegacyShaderDesc(32));
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
        
        PyObject * PyOCIO_GpuShaderDesc_getCacheID(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstGpuShaderDescRcPtr desc = GetConstGpuShaderDesc(self);
            return PyString_FromString(desc->getCacheID());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_GpuShaderDesc_finalize(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            GpuShaderDescRcPtr desc = GetEditableGpuShaderDesc(self);
            desc->finalize();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
    } // anon namespace
    
}
OCIO_NAMESPACE_EXIT
