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

#include "PyProcessor.h"
#include "PyUtil.h"

#include <sstream>

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    bool AddProcessorObjectToModule( PyObject* m )
    {
        PyOCIO_ProcessorType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_ProcessorType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_ProcessorType );
        PyModule_AddObject(m, "Processor",
                (PyObject *)&PyOCIO_ProcessorType);
        
        return true;
    }
    
    
    PyObject * BuildConstPyProcessor(ConstProcessorRcPtr processor)
    {
        if (processor.get() == 0x0)
        {
            PyErr_SetString(PyExc_ValueError, "Cannot create PyProcessor from null object.");
            return NULL;
        }
        
        PyOCIO_Processor * pyProcessor = PyObject_New(
                PyOCIO_Processor, (PyTypeObject * ) &PyOCIO_ProcessorType);
        
        pyProcessor->constcppobj = new OCIO::ConstProcessorRcPtr();
        *pyProcessor->constcppobj = processor;
        
        return ( PyObject * ) pyProcessor;
    }
    
    bool IsPyProcessor(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return (PyObject_Type(pyobject) == (PyObject *) (&PyOCIO_ProcessorType));
    }
    
    ConstProcessorRcPtr GetConstProcessor(PyObject * pyobject)
    {
        if(!IsPyProcessor(pyobject))
        {
            throw Exception("PyObject must be an OCIO::Processor.");
        }
        
        PyOCIO_Processor * pyProcessor = reinterpret_cast<PyOCIO_Processor *> (pyobject);
        if(pyProcessor->constcppobj)
        {
            return *pyProcessor->constcppobj;
        }
        
        throw Exception("PyObject must be a valid OCIO::Processor.");
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    
    
    
    
    
    
    
    
    
    namespace
    {
        int PyOCIO_Processor_init( PyOCIO_Processor * self, PyObject * args, PyObject * kwds );
        void PyOCIO_Processor_delete( PyOCIO_Processor * self, PyObject * args );
        
        PyObject * PyOCIO_Processor_applyRGB( PyObject * self, PyObject * args );
        
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_Processor_methods[] = {
            {"applyRGB", PyOCIO_Processor_applyRGB, METH_VARARGS, "" },
            
            {NULL, NULL, 0, NULL}
        };
        
        const char initMessage[] =
            "Processor objects cannot be instantiated directly. "
            "Please use config.getProcessor() instead.";
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ProcessorType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.Processor",                               //tp_name
        sizeof(PyOCIO_Processor),                       //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_Processor_delete,            //tp_dealloc
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
        "Processor",                                   //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_Processor_methods,                       //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_Processor_init,               //tp_init 
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
        int PyOCIO_Processor_init( PyOCIO_Processor *self, PyObject * /*args*/, PyObject * /*kwds*/ )
        {
            PyErr_SetString( PyExc_RuntimeError, initMessage);
            return -1;
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        void PyOCIO_Processor_delete( PyOCIO_Processor *self, PyObject * /*args*/ )
        {
            delete self->constcppobj;
            self->ob_type->tp_free((PyObject*)self);
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_Processor_applyRGB( PyObject * self, PyObject * args )
        {
            try
            {
                /*
                char * path = 0;
                if (!PyArg_ParseTuple(args,"s:setResourcePath", &path)) return NULL;
                */
                
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
