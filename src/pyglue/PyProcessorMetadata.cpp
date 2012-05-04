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

#include "PyProcessorMetadata.h"
#include "PyUtil.h"
#include "PyDoc.h"

#include <sstream>

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    bool AddProcessorMetadataObjectToModule( PyObject* m )
    {
        PyOCIO_ProcessorMetadataType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_ProcessorMetadataType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_ProcessorMetadataType );
        PyModule_AddObject(m, "ProcessorMetadata",
                (PyObject *)&PyOCIO_ProcessorMetadataType);
        
        return true;
    }
    
    PyObject * BuildConstPyProcessorMetadata(ConstProcessorMetadataRcPtr metadata)
    {
        if (!metadata)
        {
            Py_RETURN_NONE;
        }
        
        PyOCIO_ProcessorMetadata * pyMetadata = PyObject_New(
                PyOCIO_ProcessorMetadata, (PyTypeObject * ) &PyOCIO_ProcessorMetadataType);
        
        pyMetadata->constcppobj = new ConstProcessorMetadataRcPtr();
        *pyMetadata->constcppobj = metadata;
        
        return ( PyObject * ) pyMetadata;
    }
    
    bool IsPyProcessorMetadata(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return (PyObject_Type(pyobject) == (PyObject *) (&PyOCIO_ProcessorMetadataType));
    }
    
    ConstProcessorMetadataRcPtr GetConstProcessorMetadata(PyObject * pyobject)
    {
        if(!IsPyProcessorMetadata(pyobject))
        {
            throw Exception("PyObject must be an OCIO.ProcessorMetadata.");
        }
        
        PyOCIO_ProcessorMetadata * pyMetadata = reinterpret_cast<PyOCIO_ProcessorMetadata *> (pyobject);
        if(pyMetadata->constcppobj)
        {
            return *pyMetadata->constcppobj;
        }
        
        throw Exception("PyObject must be a valid OCIO.ProcessorMetadata.");
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    namespace
    {
        int PyOCIO_ProcessorMetadata_init( PyOCIO_ProcessorMetadata * self, PyObject * args, PyObject * kwds );
        void PyOCIO_ProcessorMetadata_delete( PyOCIO_ProcessorMetadata * self, PyObject * args );
        
        PyObject * PyOCIO_ProcessorMetadata_getFiles( PyObject * self );
        PyObject * PyOCIO_ProcessorMetadata_getLooks( PyObject * self );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_ProcessorMetadata_methods[] = {
            {"getFiles",
                (PyCFunction) PyOCIO_ProcessorMetadata_getFiles, METH_NOARGS,
                PROCESSORMETADATA_GETFILES__DOC__ },
            {"getLooks",
                (PyCFunction) PyOCIO_ProcessorMetadata_getLooks, METH_NOARGS,
                PROCESSORMETADATA_GETLOOKS__DOC__ },
            {NULL, NULL, 0, NULL}
        };
        
        const char initMessage[] =
            "ProcessorMetadata objects cannot be instantiated directly. "
            "Please use processor.getMetadata() instead.";
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ProcessorMetadataType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.ProcessorMetadata",                   //tp_name
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
        int PyOCIO_ProcessorMetadata_init( PyOCIO_ProcessorMetadata */*self*/,
            PyObject * /*args*/, PyObject * /*kwds*/ )
        {
            PyErr_SetString( PyExc_RuntimeError, initMessage);
            return -1;
        }
        
        void PyOCIO_ProcessorMetadata_delete( PyOCIO_ProcessorMetadata *self, PyObject * /*args*/ )
        {
            delete self->constcppobj;
            self->ob_type->tp_free((PyObject*)self);
        }
        
        PyObject * PyOCIO_ProcessorMetadata_getFiles( PyObject * self )
        {
            try
            {
                ConstProcessorMetadataRcPtr metadata = GetConstProcessorMetadata(self);
                
                std::vector<std::string> data;
                for(int i=0; i<metadata->getNumFiles(); ++i)
                {
                    data.push_back(metadata->getFile(i));
                }
                
                return CreatePyListFromStringVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ProcessorMetadata_getLooks( PyObject * self )
        {
            try
            {
                ConstProcessorMetadataRcPtr metadata = GetConstProcessorMetadata(self);
                
                std::vector<std::string> data;
                for(int i=0; i<metadata->getNumLooks(); ++i)
                {
                    data.push_back(metadata->getLook(i));
                }
                
                return CreatePyListFromStringVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
    } // anon namespace
    
}
OCIO_NAMESPACE_EXIT
