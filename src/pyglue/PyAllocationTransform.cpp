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

// Rarely used. could use a log transform instead. This can sample by log when
// doing the offset to make best use of the data.

#define GetConstAllocationTransform(pyobject) GetConstPyOCIO<PyOCIO_Transform, \
    ConstAllocationTransformRcPtr, AllocationTransform>(pyobject, \
    PyOCIO_AllocationTransformType)

#define GetEditableAllocationTransform(pyobject) GetEditablePyOCIO<PyOCIO_Transform, \
    AllocationTransformRcPtr, AllocationTransform>(pyobject, \
    PyOCIO_AllocationTransformType)

OCIO_NAMESPACE_ENTER
{
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_AllocationTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds);
        PyObject * PyOCIO_AllocationTransform_equals(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_AllocationTransform_getAllocation(PyObject * self);
        PyObject * PyOCIO_AllocationTransform_setAllocation(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_AllocationTransform_getNumVars(PyObject * self);
        PyObject * PyOCIO_AllocationTransform_getVars(PyObject * self);
        PyObject * PyOCIO_AllocationTransform_setVars(PyObject * self,  PyObject * args);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_AllocationTransform_methods[] = {
            { "getAllocation",
            (PyCFunction) PyOCIO_AllocationTransform_getAllocation, METH_NOARGS, ALLOCATIONTRANSFORM_GETALLOCATION__DOC__ },
            { "setAllocation",
            PyOCIO_AllocationTransform_setAllocation, METH_VARARGS, ALLOCATIONTRANSFORM_SETALLOCATION__DOC__ },
            { "getNumVars",
            (PyCFunction) PyOCIO_AllocationTransform_getNumVars, METH_VARARGS, ALLOCATIONTRANSFORM_GETNUMVARS__DOC__ },
            { "getVars",
            (PyCFunction) PyOCIO_AllocationTransform_getVars, METH_NOARGS, ALLOCATIONTRANSFORM_GETVARS__DOC__ },
            { "setVars",
            PyOCIO_AllocationTransform_setVars, METH_VARARGS, ALLOCATIONTRANSFORM_SETVARS__DOC__ },
            { NULL, NULL, 0, NULL }
        };
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_AllocationTransformType = {
        PyVarObject_HEAD_INIT(NULL, 0)              //ob_size
        OCIO_PYTHON_NAMESPACE(AllocationTransform), //tp_name
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
        ALLOCATIONTRANSFORM__DOC__,                 //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_AllocationTransform_methods,         //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        &PyOCIO_TransformType,                      //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_AllocationTransform_init, //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_AllocationTransform_init(PyOCIO_Transform * self, PyObject * /*args*/, PyObject * /*kwds*/)
        {
            OCIO_PYTRY_ENTER()
            return BuildPyTransformObject<AllocationTransformRcPtr>(self, AllocationTransform::Create());
            OCIO_PYTRY_EXIT(-1)
        }
        
        PyObject * PyOCIO_AllocationTransform_getAllocation(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstAllocationTransformRcPtr transform = GetConstAllocationTransform(self);
            return PyString_FromString( AllocationToString( transform->getAllocation()) );
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_AllocationTransform_setAllocation(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            Allocation hwalloc;
            if (!PyArg_ParseTuple(args,"O&:setAllocation",
                ConvertPyObjectToAllocation, &hwalloc)) return NULL;
            AllocationTransformRcPtr transform = GetEditableAllocationTransform(self);
            transform->setAllocation(hwalloc);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_AllocationTransform_getNumVars(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstAllocationTransformRcPtr transform = GetConstAllocationTransform(self);
            return PyInt_FromLong(transform->getNumVars());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_AllocationTransform_getVars(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstAllocationTransformRcPtr transform = GetConstAllocationTransform(self);
            std::vector<float> vars(transform->getNumVars());
            if(!vars.empty()) transform->getVars(&vars[0]);
            return CreatePyListFromFloatVector(vars);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_AllocationTransform_setVars(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject * pyvars = 0;
            if (!PyArg_ParseTuple(args,"O:setVars", &pyvars)) return NULL;
            std::vector<float> vars;
            if(!FillFloatVectorFromPySequence(pyvars, vars))
            {
                PyErr_SetString(PyExc_TypeError, "First argument must be a float array.");
                return 0;
            }
            AllocationTransformRcPtr transform = GetEditableAllocationTransform(self);
            if(!vars.empty()) transform->setVars(static_cast<int>(vars.size()), &vars[0]);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
    }

}
OCIO_NAMESPACE_EXIT
