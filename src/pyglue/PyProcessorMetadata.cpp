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
        void PyOCIO_ProcessorMetadata_delete(PyOCIO_ProcessorMetadata * self, PyObject * args);
        PyObject * PyOCIO_ProcessorMetadata_getFiles(PyObject * self);
        PyObject * PyOCIO_ProcessorMetadata_getLooks(PyObject * self);
        
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
            "Please use processor.getMetadata() instead.";
        
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
        
        void PyOCIO_ProcessorMetadata_delete(PyOCIO_ProcessorMetadata *self, PyObject * /*args*/)
        {
            DeletePyObject<PyOCIO_ProcessorMetadata>(self);
        }
        
        PyObject * PyOCIO_ProcessorMetadata_getFiles(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstProcessorMetadataRcPtr metadata = GetConstProcessorMetadata(self);
            std::vector<std::string> data;
            for(int i = 0; i < metadata->getNumFiles(); ++i)
                data.push_back(metadata->getFile(i));
            return CreatePyListFromStringVector(data);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ProcessorMetadata_getLooks(PyObject * self)
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
