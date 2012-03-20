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

#include "PyProcessor.h"
#include "PyUtil.h"
#include "PyDoc.h"

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
        if (!processor)
        {
            Py_RETURN_NONE;
        }
        
        PyOCIO_Processor * pyProcessor = PyObject_New(
                PyOCIO_Processor, (PyTypeObject * ) &PyOCIO_ProcessorType);
        
        pyProcessor->constcppobj = new ConstProcessorRcPtr();
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
            throw Exception("PyObject must be an OCIO.Processor.");
        }
        
        PyOCIO_Processor * pyProcessor = reinterpret_cast<PyOCIO_Processor *> (pyobject);
        if(pyProcessor->constcppobj)
        {
            return *pyProcessor->constcppobj;
        }
        
        throw Exception("PyObject must be a valid OCIO.Processor.");
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    namespace
    {
        int PyOCIO_Processor_init( PyOCIO_Processor * self, PyObject * args, PyObject * kwds );
        void PyOCIO_Processor_delete( PyOCIO_Processor * self, PyObject * args );
        
        PyObject * PyOCIO_Processor_isNoOp( PyObject * self );
        PyObject * PyOCIO_Processor_hasChannelCrosstalk( PyObject * self );
        PyObject * PyOCIO_Processor_getMetadata( PyObject * self );
        PyObject * PyOCIO_Processor_applyRGB( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Processor_applyRGBA( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Processor_getCpuCacheID( PyObject * self );
        
        PyObject * PyOCIO_Processor_getGpuShaderText( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Processor_getGpuShaderTextCacheID( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Processor_getGpuLut3D( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Processor_getGpuLut3DCacheID( PyObject * self, PyObject * args );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_Processor_methods[] = {
            {"isNoOp",
            (PyCFunction) PyOCIO_Processor_isNoOp, METH_NOARGS, PROCESSOR_ISNOOP__DOC__ },
            {"hasChannelCrosstalk",
            (PyCFunction) PyOCIO_Processor_hasChannelCrosstalk, METH_NOARGS, PROCESSOR_HASCHANNELCROSSTALK__DOC__ },
            {"getMetadata",
            (PyCFunction) PyOCIO_Processor_getMetadata, METH_NOARGS, PROCESSOR_GETMETADATA__DOC__ },
            {"applyRGB",
            PyOCIO_Processor_applyRGB, METH_VARARGS, PROCESSOR_APPLYRGB__DOC__ },
            {"applyRGBA",
            PyOCIO_Processor_applyRGBA, METH_VARARGS, PROCESSOR_APPLYRGBA__DOC__ },
            {"getCpuCacheID",
            (PyCFunction) PyOCIO_Processor_getCpuCacheID, METH_NOARGS, PROCESSOR_GETCPUCACHEID__DOC__ },
            {"getGpuShaderText",
            PyOCIO_Processor_getGpuShaderText, METH_VARARGS, PROCESSOR_GETGPUSHADERTEXT__DOC__ },
            {"getGpuShaderTextCacheID",
            PyOCIO_Processor_getGpuShaderTextCacheID, METH_VARARGS, PROCESSOR_GETGPUSHADERTEXTCACHEID__DOC__ },
            {"getGpuLut3D",
            PyOCIO_Processor_getGpuLut3D, METH_VARARGS, PROCESSOR_GETGPULUT3D__DOC__ },
            {"getGpuLut3DCacheID",
            PyOCIO_Processor_getGpuLut3DCacheID, METH_VARARGS, PROCESSOR_GETGPULUT3DCACHEID__DOC__ },
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
        PROCESSOR__DOC__,                           //tp_doc 
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
        // TODO: The use of a dict, rather than a native class is not ideal
        // in the next API revision, use a native type.
        
        void FillShaderDescFromPyDict(GpuShaderDesc & shaderDesc,
                                      PyObject * dict)
        {
            if(!PyDict_Check(dict))
            {
                throw Exception("GpuShaderDesc must be a dict type.");
            }
            
            PyObject *key, *value;
            Py_ssize_t pos = 0;
            
            while (PyDict_Next(dict, &pos, &key, &value))
            {
                std::string keystr;
                if(!GetStringFromPyObject(key, &keystr))
                    throw Exception("GpuShaderDesc keys must be strings.");
                
                if(keystr == "language")
                {
                    GpuLanguage language = GPU_LANGUAGE_UNKNOWN; 
                    if(ConvertPyObjectToGpuLanguage(value, &language) == 0)
                        throw Exception("GpuShaderDesc language must be a GpuLanguage.");
                    shaderDesc.setLanguage(language);
                }
                else if(keystr == "functionName")
                {
                    std::string functionName;
                    if(!GetStringFromPyObject(value, &functionName))
                        throw Exception("GpuShaderDesc functionName must be a string.");
                    shaderDesc.setFunctionName(functionName.c_str());
                }
                else if(keystr == "lut3DEdgeLen")
                {
                    int lut3DEdgeLen = 0;
                    if(!GetIntFromPyObject(value, &lut3DEdgeLen))
                        throw Exception("GpuShaderDesc lut3DEdgeLen must be an integer.");
                    shaderDesc.setLut3DEdgeLen(lut3DEdgeLen);
                }
                else
                {
                    std::ostringstream os;
                    os << "Unknown GpuShaderDesc key, '";
                    os << keystr << "'. ";
                    os << "Allowed keys: (";
                    os << "'language', 'functionName', 'lut3DEdgeLen').";
                    throw Exception(os.str().c_str());
                }
            }
        }
    }
    
    namespace
    {
        ///////////////////////////////////////////////////////////////////////
        ///
        int PyOCIO_Processor_init( PyOCIO_Processor */*self*/,
            PyObject * /*args*/, PyObject * /*kwds*/ )
        {
            PyErr_SetString( PyExc_RuntimeError, initMessage);
            return -1;
        }
        
        void PyOCIO_Processor_delete( PyOCIO_Processor *self, PyObject * /*args*/ )
        {
            delete self->constcppobj;
            self->ob_type->tp_free((PyObject*)self);
        }
        
        PyObject * PyOCIO_Processor_isNoOp( PyObject * self )
        {
            try
            {
                ConstProcessorRcPtr processor = GetConstProcessor(self);
                return PyBool_FromLong( processor->isNoOp() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Processor_hasChannelCrosstalk( PyObject * self )
        {
            try
            {
                ConstProcessorRcPtr processor = GetConstProcessor(self);
                return PyBool_FromLong( processor->hasChannelCrosstalk() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Processor_getMetadata( PyObject * self )
        {
            try
            {
                ConstProcessorRcPtr processor = GetConstProcessor(self);
                return BuildConstPyProcessorMetadata( processor->getMetadata() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Processor_applyRGB( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O:applyRGB", &pyData)) return NULL;
                
                ConstProcessorRcPtr processor = GetConstProcessor(self);
                if(processor->isNoOp())
                {
                    Py_INCREF(pyData);
                    return pyData;
                }
                
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pyData, data) || ((data.size()%3) != 0))
                {
                    std::ostringstream os;
                    os << "First argument must be a float array, size multiple of 3. ";
                    os << "Size: " << data.size() << ".";
                    PyErr_SetString(PyExc_TypeError, os.str().c_str());
                    return 0;
                }
                
                PackedImageDesc img(&data[0], data.size()/3, 1, 3);
                processor->apply(img);
                
                return CreatePyListFromFloatVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Processor_applyRGBA( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O:applyRGBA", &pyData)) return NULL;
                
                ConstProcessorRcPtr processor = GetConstProcessor(self);
                if(processor->isNoOp())
                {
                    Py_INCREF(pyData);
                    return pyData;
                }
                
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pyData, data) || ((data.size()%4) != 0))
                {
                    std::ostringstream os;
                    os << "First argument must be a float array, size multiple of 4. ";
                    os << "Size: " << data.size() << ".";
                    PyErr_SetString(PyExc_TypeError, os.str().c_str());
                    return 0;
                }
                
                PackedImageDesc img(&data[0], data.size()/4, 1, 4);
                processor->apply(img);
                
                return CreatePyListFromFloatVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Processor_getCpuCacheID( PyObject * self )
        {
            try
            {
                ConstProcessorRcPtr processor = GetConstProcessor(self);
                return PyString_FromString( processor->getCpuCacheID() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Processor_getGpuShaderText( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O!:getGpuShaderText",
                    &PyDict_Type, &pyData)) return NULL;
                ConstProcessorRcPtr processor = GetConstProcessor(self);
                
                GpuShaderDesc shaderDesc;
                FillShaderDescFromPyDict(shaderDesc, pyData);
                
                return PyString_FromString( processor->getGpuShaderText(shaderDesc) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Processor_getGpuShaderTextCacheID( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O!:getGpuShaderTextCacheID",
                    &PyDict_Type, &pyData)) return NULL;
                ConstProcessorRcPtr processor = GetConstProcessor(self);
                
                GpuShaderDesc shaderDesc;
                FillShaderDescFromPyDict(shaderDesc, pyData);
                
                return PyString_FromString( processor->getGpuShaderTextCacheID(shaderDesc) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Processor_getGpuLut3D( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O!:getGpuLut3D",
                    &PyDict_Type, &pyData)) return NULL;
                ConstProcessorRcPtr processor = GetConstProcessor(self);
                
                GpuShaderDesc shaderDesc;
                FillShaderDescFromPyDict(shaderDesc, pyData);
                
                int len = shaderDesc.getLut3DEdgeLen();
                std::vector<float> lut3d(3*len*len*len);
                
                // TODO: return more compact binary data? (ex array, ...)
                processor->getGpuLut3D(&lut3d[0], shaderDesc);
                return CreatePyListFromFloatVector(lut3d);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Processor_getGpuLut3DCacheID( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O!:getGpuLut3DCacheID",
                    &PyDict_Type, &pyData)) return NULL;
                ConstProcessorRcPtr processor = GetConstProcessor(self);
                
                GpuShaderDesc shaderDesc;
                FillShaderDescFromPyDict(shaderDesc, pyData);
                
                return PyString_FromString( processor->getGpuLut3DCacheID(shaderDesc) );
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
