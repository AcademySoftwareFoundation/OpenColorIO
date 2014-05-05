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
#include <sstream>

#include "PyUtil.h"
#include "PyDoc.h"

OCIO_NAMESPACE_ENTER
{
    
    PyObject * BuildConstPyColorSpace(ConstColorSpaceRcPtr colorSpace)
    {
        return BuildConstPyOCIO<PyOCIO_ColorSpace, ColorSpaceRcPtr,
            ConstColorSpaceRcPtr>(colorSpace, PyOCIO_ColorSpaceType);
    }
    
    PyObject * BuildEditablePyColorSpace(ColorSpaceRcPtr colorSpace)
    {
        return BuildEditablePyOCIO<PyOCIO_ColorSpace, ColorSpaceRcPtr,
            ConstColorSpaceRcPtr>(colorSpace, PyOCIO_ColorSpaceType);
    }
    
    bool IsPyColorSpace(PyObject * pyobject)
    {
        return IsPyOCIOType(pyobject, PyOCIO_ColorSpaceType);
    }
    
    bool IsPyColorSpaceEditable(PyObject * pyobject)
    {
        return IsPyEditable<PyOCIO_ColorSpace>(pyobject, PyOCIO_ColorSpaceType);
    }
    
    ConstColorSpaceRcPtr GetConstColorSpace(PyObject * pyobject, bool allowCast)
    {
        return GetConstPyOCIO<PyOCIO_ColorSpace, ConstColorSpaceRcPtr>(pyobject,
            PyOCIO_ColorSpaceType, allowCast);
    }
    
    ColorSpaceRcPtr GetEditableColorSpace(PyObject * pyobject)
    {
        return GetEditablePyOCIO<PyOCIO_ColorSpace, ColorSpaceRcPtr>(pyobject,
            PyOCIO_ColorSpaceType);
    }
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_ColorSpace_init(PyOCIO_ColorSpace * self, PyObject * args, PyObject * kwds);
        void PyOCIO_ColorSpace_delete(PyOCIO_ColorSpace * self, PyObject * args);
        PyObject * PyOCIO_ColorSpace_str(PyObject * self);
        PyObject * PyOCIO_ColorSpace_isEditable(PyObject * self);
        PyObject * PyOCIO_ColorSpace_createEditableCopy(PyObject * self);
        PyObject * PyOCIO_ColorSpace_getName(PyObject * self);
        PyObject * PyOCIO_ColorSpace_setName(PyObject * self, PyObject * args);
        PyObject * PyOCIO_ColorSpace_getFamily(PyObject * self);
        PyObject * PyOCIO_ColorSpace_setFamily(PyObject * self, PyObject * args);
        PyObject * PyOCIO_ColorSpace_getEqualityGroup(PyObject * self);
        PyObject * PyOCIO_ColorSpace_setEqualityGroup(PyObject * self, PyObject * args);
        PyObject * PyOCIO_ColorSpace_getDescription(PyObject * self);
        PyObject * PyOCIO_ColorSpace_setDescription(PyObject * self, PyObject * args);
        PyObject * PyOCIO_ColorSpace_getBitDepth(PyObject * self);
        PyObject * PyOCIO_ColorSpace_setBitDepth(PyObject * self, PyObject * args);
        PyObject * PyOCIO_ColorSpace_isData(PyObject * self);
        PyObject * PyOCIO_ColorSpace_setIsData(PyObject * self, PyObject * args);
        PyObject * PyOCIO_ColorSpace_getAllocation(PyObject * self);
        PyObject * PyOCIO_ColorSpace_setAllocation(PyObject * self, PyObject * args );
        PyObject * PyOCIO_ColorSpace_getAllocationVars(PyObject * self);
        PyObject * PyOCIO_ColorSpace_setAllocationVars(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_ColorSpace_getTransform(PyObject * self, PyObject * args);
        PyObject * PyOCIO_ColorSpace_setTransform(PyObject * self, PyObject * args);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_ColorSpace_methods[] = {
            { "isEditable",
            (PyCFunction) PyOCIO_ColorSpace_isEditable, METH_NOARGS, COLORSPACE_ISEDITABLE__DOC__ },
            { "createEditableCopy",
            (PyCFunction) PyOCIO_ColorSpace_createEditableCopy, METH_NOARGS, COLORSPACE_CREATEEDITABLECOPY__DOC__ },
            { "getName",
            (PyCFunction) PyOCIO_ColorSpace_getName, METH_NOARGS, COLORSPACE_GETNAME__DOC__ },
            { "setName",
            PyOCIO_ColorSpace_setName, METH_VARARGS, COLORSPACE_SETNAME__DOC__ },
            { "getFamily",
            (PyCFunction) PyOCIO_ColorSpace_getFamily, METH_NOARGS, COLORSPACE_GETFAMILY__DOC__ },
            { "setFamily",
            PyOCIO_ColorSpace_setFamily, METH_VARARGS, COLORSPACE_SETFAMILY__DOC__ },
            { "getEqualityGroup",
            (PyCFunction) PyOCIO_ColorSpace_getEqualityGroup, METH_NOARGS, COLORSPACE_GETEQUALITYGROUP__DOC__ },
            { "setEqualityGroup",
            PyOCIO_ColorSpace_setEqualityGroup, METH_VARARGS, COLORSPACE_SETEQUALITYGROUP__DOC__ },
            { "getDescription",
            (PyCFunction) PyOCIO_ColorSpace_getDescription, METH_NOARGS, COLORSPACE_GETDESCRIPTION__DOC__ },
            { "setDescription",
            PyOCIO_ColorSpace_setDescription, METH_VARARGS, COLORSPACE_SETDESCRIPTION__DOC__ },
            { "getBitDepth",
            (PyCFunction) PyOCIO_ColorSpace_getBitDepth, METH_NOARGS, COLORSPACE_GETBITDEPTH__DOC__ },
            { "setBitDepth",
            PyOCIO_ColorSpace_setBitDepth, METH_VARARGS, COLORSPACE_SETBITDEPTH__DOC__ },
            { "isData",
            (PyCFunction) PyOCIO_ColorSpace_isData, METH_NOARGS, COLORSPACE_ISDATA__DOC__ },
            { "setIsData",
            PyOCIO_ColorSpace_setIsData, METH_VARARGS, COLORSPACE_SETISDATA__DOC__ },
            { "getAllocation",
            (PyCFunction) PyOCIO_ColorSpace_getAllocation, METH_NOARGS, COLORSPACE_GETALLOCATION__DOC__ },
            { "setAllocation",
            PyOCIO_ColorSpace_setAllocation, METH_VARARGS, COLORSPACE_SETALLOCATION__DOC__ },
            { "getAllocationVars",
            (PyCFunction) PyOCIO_ColorSpace_getAllocationVars, METH_NOARGS, COLORSPACE_GETALLOCATIONVARS__DOC__ },
            { "setAllocationVars",
            PyOCIO_ColorSpace_setAllocationVars, METH_VARARGS, COLORSPACE_SETALLOCATIONVARS__DOC__ },
            { "getTransform",
            PyOCIO_ColorSpace_getTransform, METH_VARARGS, COLORSPACE_GETTRANSFORM__DOC__ },
            { "setTransform",
            PyOCIO_ColorSpace_setTransform, METH_VARARGS, COLORSPACE_SETTRANSFORM__DOC__ },
            { NULL, NULL, 0, NULL }
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ColorSpaceType = {
        PyVarObject_HEAD_INIT(NULL, 0)              //ob_size 
        OCIO_PYTHON_NAMESPACE(ColorSpace),          //tp_name
        sizeof(PyOCIO_ColorSpace),                  //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_ColorSpace_delete,       //tp_dealloc
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
        PyOCIO_ColorSpace_str,                      //tp_str
        0,                                          //tp_getattro
        0,                                          //tp_setattro
        0,                                          //tp_as_buffer
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   //tp_flags
        COLORSPACE__DOC__,                          //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_ColorSpace_methods,                  //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_ColorSpace_init,          //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_ColorSpace_init(PyOCIO_ColorSpace * self, PyObject * args, PyObject * kwds)
        {
            OCIO_PYTRY_ENTER()
            
            ColorSpaceRcPtr ptr = ColorSpace::Create();
            int ret = BuildPyObject<PyOCIO_ColorSpace, ConstColorSpaceRcPtr, ColorSpaceRcPtr>(self, ptr);
            
            char* name = NULL;
            char* family = NULL;
            char* equalityGroup = NULL;
            char* description = NULL;
            char* bitDepth = NULL;
            bool isData = false; // TODO: Do not rely on the default value
            char* allocation = NULL;
            PyObject* allocationVars = NULL;
            PyObject* toRefTransform = NULL;
            PyObject* fromRefTransform = NULL;
            
            const char* toRefStr = 
                ColorSpaceDirectionToString(COLORSPACE_DIR_TO_REFERENCE);
            const char* fromRefStr = 
                ColorSpaceDirectionToString(COLORSPACE_DIR_FROM_REFERENCE);
            const char* kwlist[] = { "name", "family", "equalityGroup",
                "description", "bitDepth", "isData", "allocation",
                "allocationVars", toRefStr, fromRefStr, NULL };
            if(!PyArg_ParseTupleAndKeywords(args, kwds, "|sssssO&sOOO",
                const_cast<char **>(kwlist),
                &name, &family, &equalityGroup, &description, &bitDepth,
                ConvertPyObjectToBool, &isData,
                &allocation, &allocationVars,
                &toRefTransform, &fromRefTransform)) return -1;
            
            if(name) ptr->setName(name);
            if(family) ptr->setFamily(family);
            if(equalityGroup) ptr->setEqualityGroup(equalityGroup);
            if(description) ptr->setDescription(description);
            if(bitDepth) ptr->setBitDepth(BitDepthFromString(bitDepth));
            ptr->setIsData(isData); // TODO: Do not rely on the default value
            if(allocation) ptr->setAllocation(AllocationFromString(allocation));
            if(allocationVars)
            {
                std::vector<float> vars;
                if(!FillFloatVectorFromPySequence(allocationVars, vars))
                {
                    PyErr_SetString(PyExc_TypeError, "allocationVars kwarg must be a float array.");
                    return -1;
                }
                ptr->setAllocationVars(static_cast<int>(vars.size()), &vars[0]);
            }
            if(toRefTransform)
            {
                ConstTransformRcPtr transform = GetConstTransform(toRefTransform, true);
                ptr->setTransform(transform, COLORSPACE_DIR_TO_REFERENCE);
            }
            if(fromRefTransform)
            {
                ConstTransformRcPtr transform = GetConstTransform(fromRefTransform, true);
                ptr->setTransform(transform, COLORSPACE_DIR_FROM_REFERENCE);
            }
            return ret;
            
            OCIO_PYTRY_EXIT(-1)
        }
        
        void PyOCIO_ColorSpace_delete(PyOCIO_ColorSpace *self, PyObject * /*args*/)
        {
            DeletePyObject<PyOCIO_ColorSpace>(self);
        }
        
        PyObject * PyOCIO_ColorSpace_str(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
            std::ostringstream out;
            out << *colorSpace;
            return PyString_FromString(out.str().c_str());
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_ColorSpace_isEditable(PyObject * self)
        {
            return PyBool_FromLong(IsPyColorSpaceEditable(self));
        }
        
        PyObject * PyOCIO_ColorSpace_createEditableCopy(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
            ColorSpaceRcPtr copy = colorSpace->createEditableCopy();
            return BuildEditablePyColorSpace(copy);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_getName(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
            return PyString_FromString(colorSpace->getName());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_setName(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            if (!PyArg_ParseTuple(args, "s:setName", &name)) return NULL;
            ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
            colorSpace->setName(name);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_getFamily(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
            return PyString_FromString(colorSpace->getFamily());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_setFamily(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            if (!PyArg_ParseTuple(args, "s:setFamily", &name)) return NULL;
            ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
            colorSpace->setFamily(name);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_getEqualityGroup(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
            return PyString_FromString(colorSpace->getEqualityGroup());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_setEqualityGroup(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            if (!PyArg_ParseTuple(args, "s:setEqualityGroup", &name)) return NULL;
            ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
            colorSpace->setEqualityGroup(name);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_getDescription(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
            return PyString_FromString(colorSpace->getDescription());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_setDescription(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            if (!PyArg_ParseTuple(args, "s:setDescription", &name)) return NULL;
            ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
            colorSpace->setDescription(name);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_getBitDepth(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
            return PyString_FromString( BitDepthToString(colorSpace->getBitDepth()));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_setBitDepth(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            if (!PyArg_ParseTuple(args, "s:setBitDepth", &name)) return NULL;
            ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
            colorSpace->setBitDepth(BitDepthFromString(name));
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_isData(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
            return PyBool_FromLong(colorSpace->isData());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_setIsData(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            bool isData = false;
            if (!PyArg_ParseTuple(args, "O&:setIsData",
                ConvertPyObjectToBool, &isData)) return NULL;
            ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
            colorSpace->setIsData(isData);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_getAllocation(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
            return PyString_FromString(AllocationToString(colorSpace->getAllocation()));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_setAllocation(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            Allocation hwalloc;
            if (!PyArg_ParseTuple(args, "O&:setAllocation",
                ConvertPyObjectToAllocation, &hwalloc)) return NULL;
            ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
            colorSpace->setAllocation(hwalloc);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_getAllocationVars(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
            std::vector<float> allocationvars(colorSpace->getAllocationNumVars());
            if(!allocationvars.empty())
                colorSpace->getAllocationVars(&allocationvars[0]);
            return CreatePyListFromFloatVector(allocationvars);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_setAllocationVars(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pyvars = 0;
            if (!PyArg_ParseTuple(args, "O:setAllocationVars", &pyvars)) return NULL;
            std::vector<float> vars;
            if(!FillFloatVectorFromPySequence(pyvars, vars))
            {
                PyErr_SetString(PyExc_TypeError, "First argument must be a float array.");
                return 0;
            }
            ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
            if(!vars.empty())
                colorSpace->setAllocationVars(static_cast<int>(vars.size()), &vars[0]);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_getTransform(PyObject * self,  PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            ColorSpaceDirection dir;
            if (!PyArg_ParseTuple(args, "O&:getTransform",
                 ConvertPyObjectToColorSpaceDirection, &dir)) return NULL;
            ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
            ConstTransformRcPtr transform = colorSpace->getTransform(dir);
            return BuildConstPyTransform(transform);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_ColorSpace_setTransform(PyObject * self,  PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pytransform = 0;
            ColorSpaceDirection dir;
            if (!PyArg_ParseTuple(args, "OO&:setTransform", &pytransform,
                ConvertPyObjectToColorSpaceDirection, &dir)) return NULL;
            ConstTransformRcPtr transform = GetConstTransform(pytransform, true);
            ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
            colorSpace->setTransform(transform, dir);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
    }

}
OCIO_NAMESPACE_EXIT
