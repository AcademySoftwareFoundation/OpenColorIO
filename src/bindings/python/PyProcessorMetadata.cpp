// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <Python.h>
#include <OpenColorIO/OpenColorIO.h>

#include "PyUtil.h"
#include "PyDoc.h"

OCIO_NAMESPACE_ENTER
{
    
    PyObject * BuildConstPyProcessorMetadata(ConstProcessorMetadataRcPtr metadata)
    {
        return BuildConstPyOCIO<PyOCIO_ProcessorMetadata, ProcessorMetadataRcPtr,
            ConstProcessorMetadataRcPtr>(metadata, PyOCIO_ProcessorMetadataType);
    }
    
    bool IsPyProcessorMetadata(PyObject * pyobject)
    {
        return IsPyOCIOType(pyobject, PyOCIO_ProcessorMetadataType);
    }
    
    ConstProcessorMetadataRcPtr GetConstProcessorMetadata(PyObject * pyobject)
    {
        return GetConstPyOCIO<PyOCIO_ProcessorMetadata,
            ConstProcessorMetadataRcPtr>(pyobject, PyOCIO_ProcessorMetadataType);
    }
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_ProcessorMetadata_init(PyOCIO_ProcessorMetadata * self, PyObject * args, PyObject * kwds);
        void PyOCIO_ProcessorMetadata_delete(PyOCIO_ProcessorMetadata * self);
        PyObject * PyOCIO_ProcessorMetadata_getFiles(PyObject * self, PyObject *);
        PyObject * PyOCIO_ProcessorMetadata_getLooks(PyObject * self, PyObject *);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_ProcessorMetadata_methods[] = {
            { "getFiles",
            (PyCFunction) PyOCIO_ProcessorMetadata_getFiles, METH_NOARGS, PROCESSORMETADATA_GETFILES__DOC__ },
            { "getLooks",
            (PyCFunction) PyOCIO_ProcessorMetadata_getLooks, METH_NOARGS, PROCESSORMETADATA_GETLOOKS__DOC__ },
            { NULL, NULL, 0, NULL }
        };
        
        const char initMessage[] =
            "ProcessorMetadata objects cannot be instantiated directly. "
            "Please use processor.getProcessorMetadata() instead.";
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ProcessorMetadataType = {
        PyVarObject_HEAD_INIT(NULL, 0)              //ob_size
        OCIO_PYTHON_NAMESPACE(ProcessorMetadata),   //tp_name
        sizeof(PyOCIO_ProcessorMetadata),           //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_ProcessorMetadata_delete,//tp_dealloc
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
        PROCESSORMETADATA__DOC__,                   //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_ProcessorMetadata_methods,           //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_ProcessorMetadata_init,   //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_ProcessorMetadata_init(PyOCIO_ProcessorMetadata * /*self*/, PyObject * /*args*/, PyObject * /*kwds*/)
        {
            PyErr_SetString( PyExc_RuntimeError, initMessage);
            return -1;
        }
        
        void PyOCIO_ProcessorMetadata_delete(PyOCIO_ProcessorMetadata *self)
        {
            DeletePyObject<PyOCIO_ProcessorMetadata>(self);
        }
        
        PyObject * PyOCIO_ProcessorMetadata_getFiles(PyObject * self, PyObject *)
        {
            OCIO_PYTRY_ENTER()
            ConstProcessorMetadataRcPtr metadata = GetConstProcessorMetadata(self);
            std::vector<std::string> data;
            for(int i = 0; i < metadata->getNumFiles(); ++i)
                data.push_back(metadata->getFile(i));
            return CreatePyListFromStringVector(data);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ProcessorMetadata_getLooks(PyObject * self, PyObject *)
        {
            OCIO_PYTRY_ENTER()
            ConstProcessorMetadataRcPtr metadata = GetConstProcessorMetadata(self);
            std::vector<std::string> data;
            for(int i = 0; i < metadata->getNumLooks(); ++i)
                data.push_back(metadata->getLook(i));
            return CreatePyListFromStringVector(data);
            OCIO_PYTRY_EXIT(NULL)
        }
        
    } // anon namespace
    
}
OCIO_NAMESPACE_EXIT
