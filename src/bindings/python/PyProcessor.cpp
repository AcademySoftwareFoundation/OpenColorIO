// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <Python.h>
#include <sstream>
#include <OpenColorIO/OpenColorIO.h>

#include "PyUtil.h"
#include "PyDoc.h"

OCIO_NAMESPACE_ENTER
{
    
    PyObject * BuildConstPyProcessor(ConstProcessorRcPtr processor)
    {
        return BuildConstPyOCIO<PyOCIO_Processor, ProcessorRcPtr,
            ConstProcessorRcPtr>(processor, PyOCIO_ProcessorType);
    }
    
    bool IsPyProcessor(PyObject * pyobject)
    {
        return IsPyOCIOType(pyobject, PyOCIO_ProcessorType);
    }
    
    ConstProcessorRcPtr GetConstProcessor(PyObject * pyobject)
    {
        return GetConstPyOCIO<PyOCIO_Processor, ConstProcessorRcPtr>(pyobject,
            PyOCIO_ProcessorType);
    }
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_Processor_init(PyOCIO_Processor * self, PyObject * args, PyObject * kwds);
        void PyOCIO_Processor_delete(PyOCIO_Processor * self);
        PyObject * PyOCIO_Processor_isNoOp(PyObject * self, PyObject *);
        PyObject * PyOCIO_Processor_hasChannelCrosstalk(PyObject * self, PyObject *);
        PyObject * PyOCIO_Processor_getProcessorMetadata(PyObject * self, PyObject *);
        PyObject * PyOCIO_Processor_applyRGB(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Processor_applyRGBA(PyObject * self, PyObject * args);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_Processor_methods[] = {
            { "isNoOp",
            (PyCFunction) PyOCIO_Processor_isNoOp, METH_NOARGS, PROCESSOR_ISNOOP__DOC__ },
            { "hasChannelCrosstalk",
            (PyCFunction) PyOCIO_Processor_hasChannelCrosstalk, METH_NOARGS, PROCESSOR_HASCHANNELCROSSTALK__DOC__ },
            { "getProcessorMetadata",
            (PyCFunction) PyOCIO_Processor_getProcessorMetadata, METH_NOARGS, PROCESSOR_GETPROCESSORMETADATA__DOC__ },
            { "applyRGB",
            PyOCIO_Processor_applyRGB, METH_VARARGS, PROCESSOR_APPLYRGB__DOC__ },
            { "applyRGBA",
            PyOCIO_Processor_applyRGBA, METH_VARARGS, PROCESSOR_APPLYRGBA__DOC__ },
            { NULL, NULL, 0, NULL }
        };
        
        const char initMessage[] =
            "Processor objects cannot be instantiated directly. "
            "Please use config.getProcessor() instead.";
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ProcessorType = {
        PyVarObject_HEAD_INIT(NULL, 0)              //ob_size
        OCIO_PYTHON_NAMESPACE(Processor),           //tp_name
        sizeof(PyOCIO_Processor),                   //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_Processor_delete,        //tp_dealloc
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
        PyOCIO_Processor_methods,                   //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_Processor_init,           //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_Processor_init(PyOCIO_Processor * /*self*/, PyObject * /*args*/, PyObject * /*kwds*/)
        {
            PyErr_SetString( PyExc_RuntimeError, initMessage);
            return -1;
        }
        
        void PyOCIO_Processor_delete(PyOCIO_Processor *self)
        {
            DeletePyObject<PyOCIO_Processor>(self);
        }
        
        PyObject * PyOCIO_Processor_isNoOp(PyObject * self, PyObject *)
        {
            OCIO_PYTRY_ENTER()
            ConstProcessorRcPtr processor = GetConstProcessor(self);
            return PyBool_FromLong(processor->isNoOp());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Processor_hasChannelCrosstalk(PyObject * self, PyObject *)
        {
            OCIO_PYTRY_ENTER()
            ConstProcessorRcPtr processor = GetConstProcessor(self);
            return PyBool_FromLong(processor->hasChannelCrosstalk());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Processor_getProcessorMetadata(PyObject * self, PyObject *)
        {
            OCIO_PYTRY_ENTER()
            ConstProcessorRcPtr processor = GetConstProcessor(self);
            return BuildConstPyProcessorMetadata(processor->getProcessorMetadata());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Processor_applyRGB(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pyData = 0;
            if (!PyArg_ParseTuple(args, "O:applyRGB",
                &pyData)) return NULL;
            ConstProcessorRcPtr processor = GetConstProcessor(self);
            if(processor->isNoOp())
            {
                Py_INCREF(pyData);
                return pyData;
            }
            std::vector<float> data;
            if(!FillFloatVectorFromPySequence(pyData, data) ||
                ((data.size() % 3) != 0))
            {
                std::ostringstream os;
                os << "First argument must be a float array, size multiple of 3. ";
                os << "Size: " << data.size() << ".";
                PyErr_SetString(PyExc_TypeError, os.str().c_str());
                return 0;
            }
            PackedImageDesc img(&data[0], long(data.size()/3), 1, 3);
            processor->getDefaultCPUProcessor()->apply(img);
            return CreatePyListFromFloatVector(data);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Processor_applyRGBA(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pyData = 0;
            if (!PyArg_ParseTuple(args, "O:applyRGBA",
                &pyData)) return NULL;
            ConstProcessorRcPtr processor = GetConstProcessor(self);
            if(processor->isNoOp())
            {
                Py_INCREF(pyData);
                return pyData;
            }
            std::vector<float> data;
            if(!FillFloatVectorFromPySequence(pyData, data) ||
                ((data.size() % 4) != 0))
            {
                std::ostringstream os;
                os << "First argument must be a float array, size multiple of 4. ";
                os << "Size: " << data.size() << ".";
                PyErr_SetString(PyExc_TypeError, os.str().c_str());
                return 0;
            }
            PackedImageDesc img(&data[0], long(data.size()/4), 1, 4);
            processor->getDefaultCPUProcessor()->apply(img);
            return CreatePyListFromFloatVector(data);
            OCIO_PYTRY_EXIT(NULL)
        }
        
    }
    
}
OCIO_NAMESPACE_EXIT
