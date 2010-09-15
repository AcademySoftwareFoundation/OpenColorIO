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

#include "PyColorSpace.h"
#include "PyTransform.h"
#include "PyUtil.h"

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    bool AddColorSpaceObjectToModule( PyObject* m )
    {
        PyOCIO_ColorSpaceType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_ColorSpaceType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_ColorSpaceType );
        PyModule_AddObject(m, "ColorSpace",
                (PyObject *)&PyOCIO_ColorSpaceType);
        
        return true;
    }
    
    PyObject * BuildConstPyColorSpace(ConstColorSpaceRcPtr colorSpace)
    {
        if (!colorSpace)
        {
            Py_RETURN_NONE;
        }
        
        PyOCIO_ColorSpace * pycolorSpace = PyObject_New(
                PyOCIO_ColorSpace, (PyTypeObject * ) &PyOCIO_ColorSpaceType);
        
        pycolorSpace->constcppobj = new ConstColorSpaceRcPtr();
        *pycolorSpace->constcppobj = colorSpace;
        
        pycolorSpace->cppobj = new ColorSpaceRcPtr();
        pycolorSpace->isconst = true;
        
        return ( PyObject * ) pycolorSpace;
    }
    
    PyObject * BuildEditablePyColorSpace(ColorSpaceRcPtr colorSpace)
    {
        if (!colorSpace)
        {
            Py_RETURN_NONE;
        }
        
        PyOCIO_ColorSpace * pycolorSpace = PyObject_New(
                PyOCIO_ColorSpace, (PyTypeObject * ) &PyOCIO_ColorSpaceType);
        
        pycolorSpace->constcppobj = new ConstColorSpaceRcPtr();
        pycolorSpace->cppobj = new ColorSpaceRcPtr();
        *pycolorSpace->cppobj = colorSpace;
        
        pycolorSpace->isconst = false;
        
        return ( PyObject * ) pycolorSpace;
    }
    
    bool IsPyColorSpace(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return (PyObject_Type(pyobject) == (PyObject *) (&PyOCIO_ColorSpaceType));
    }
    
    bool IsPyColorSpaceEditable(PyObject * pyobject)
    {
        if(!IsPyColorSpace(pyobject))
        {
            throw Exception("PyObject must be an OCIO.ColorSpace.");
        }
        
        PyOCIO_ColorSpace * pycolorSpace = reinterpret_cast<PyOCIO_ColorSpace *> (pyobject);
        return (!pycolorSpace->isconst);
    }
    
    ConstColorSpaceRcPtr GetConstColorSpace(PyObject * pyobject, bool allowCast)
    {
        if(!IsPyColorSpace(pyobject))
        {
            throw Exception("PyObject must be an OCIO.ColorSpace.");
        }
        
        PyOCIO_ColorSpace * pycolorspace = reinterpret_cast<PyOCIO_ColorSpace *> (pyobject);
        if(pycolorspace->isconst && pycolorspace->constcppobj)
        {
            return *pycolorspace->constcppobj;
        }
        
        if(allowCast && !pycolorspace->isconst && pycolorspace->cppobj)
        {
            return *pycolorspace->cppobj;
        }
        
        throw Exception("PyObject must be a valid OCIO.ColorSpace.");
    }
    
    ColorSpaceRcPtr GetEditableColorSpace(PyObject * pyobject)
    {
        if(!IsPyColorSpace(pyobject))
        {
            throw Exception("PyObject must be an OCIO.ColorSpace.");
        }
        
        PyOCIO_ColorSpace * pycolorspace = reinterpret_cast<PyOCIO_ColorSpace *> (pyobject);
        if(!pycolorspace->isconst && pycolorspace->cppobj)
        {
            return *pycolorspace->cppobj;
        }
        
        throw Exception("PyObject must be an editable OCIO.ColorSpace.");
    }
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    
    
    
    
    
    
    
    
    
    namespace
    {
        int PyOCIO_ColorSpace_init( PyOCIO_ColorSpace * self, PyObject * args, PyObject * kwds );
        void PyOCIO_ColorSpace_delete( PyOCIO_ColorSpace * self, PyObject * args );
        PyObject * PyOCIO_ColorSpace_isEditable( PyObject * self );
        PyObject * PyOCIO_ColorSpace_createEditableCopy( PyObject * self );
        
        PyObject * PyOCIO_ColorSpace_getName( PyObject * self );
        PyObject * PyOCIO_ColorSpace_setName( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColorSpace_getFamily( PyObject * self );
        PyObject * PyOCIO_ColorSpace_setFamily( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColorSpace_getDescription( PyObject * self );
        PyObject * PyOCIO_ColorSpace_setDescription( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_ColorSpace_getBitDepth( PyObject * self );
        PyObject * PyOCIO_ColorSpace_setBitDepth( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColorSpace_isData( PyObject * self );
        PyObject * PyOCIO_ColorSpace_setIsData( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColorSpace_getGpuAllocation( PyObject * self );
        PyObject * PyOCIO_ColorSpace_setGpuAllocation( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColorSpace_getGpuMin( PyObject * self );
        PyObject * PyOCIO_ColorSpace_setGpuMin( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColorSpace_getGpuMax( PyObject * self );
        PyObject * PyOCIO_ColorSpace_setGpuMax( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_ColorSpace_getTransform( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColorSpace_getEditableTransform( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColorSpace_setTransform( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColorSpace_isTransformSpecified( PyObject * self,  PyObject *args );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_ColorSpace_methods[] = {
            {"isEditable", (PyCFunction) PyOCIO_ColorSpace_isEditable, METH_NOARGS, "" },
            {"createEditableCopy", (PyCFunction) PyOCIO_ColorSpace_createEditableCopy, METH_NOARGS, "" },
            
            {"getName", (PyCFunction) PyOCIO_ColorSpace_getName, METH_NOARGS, "" },
            {"setName", PyOCIO_ColorSpace_setName, METH_VARARGS, "" },
            {"getFamily", (PyCFunction) PyOCIO_ColorSpace_getFamily, METH_NOARGS, "" },
            {"setFamily", PyOCIO_ColorSpace_setFamily, METH_VARARGS, "" },
            {"getDescription", (PyCFunction) PyOCIO_ColorSpace_getDescription, METH_NOARGS, "" },
            {"setDescription", PyOCIO_ColorSpace_setDescription, METH_VARARGS, "" },
            
            {"getBitDepth", (PyCFunction) PyOCIO_ColorSpace_getBitDepth, METH_NOARGS, "" },
            {"setBitDepth", PyOCIO_ColorSpace_setBitDepth, METH_VARARGS, "" },
            {"isData", (PyCFunction) PyOCIO_ColorSpace_isData, METH_NOARGS, "" },
            {"setIsData", PyOCIO_ColorSpace_setIsData, METH_VARARGS, "" },
            {"getGpuAllocation", (PyCFunction) PyOCIO_ColorSpace_getGpuAllocation, METH_NOARGS, "" },
            {"setGpuAllocation", PyOCIO_ColorSpace_setGpuAllocation, METH_VARARGS, "" },
            {"getGpuMin", (PyCFunction) PyOCIO_ColorSpace_getGpuMin, METH_NOARGS, "" },
            {"setGpuMin", PyOCIO_ColorSpace_setGpuMin, METH_VARARGS, "" },
            {"getGpuMax", (PyCFunction) PyOCIO_ColorSpace_getGpuMax, METH_NOARGS, "" },
            {"setGpuMax", PyOCIO_ColorSpace_setGpuMax, METH_VARARGS, "" },
            
            {"getTransform", PyOCIO_ColorSpace_getTransform, METH_VARARGS, "" },
            {"getEditableTransform", PyOCIO_ColorSpace_getEditableTransform, METH_VARARGS, "" },
            {"setTransform", PyOCIO_ColorSpace_setTransform, METH_VARARGS, "" },
            {"isTransformSpecified", PyOCIO_ColorSpace_isTransformSpecified, METH_VARARGS, "" },
            
            {NULL, NULL, 0, NULL}
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ColorSpaceType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.ColorSpace",                           //tp_name
        sizeof(PyOCIO_ColorSpace),                   //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_ColorSpace_delete,        //tp_dealloc
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
        "ColorSpace",                               //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_ColorSpace_methods,                   //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_ColorSpace_init,           //tp_init 
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
        int PyOCIO_ColorSpace_init( PyOCIO_ColorSpace *self, PyObject * /*args*/, PyObject * /*kwds*/ )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            
            self->constcppobj = new ConstColorSpaceRcPtr();
            self->cppobj = new ColorSpaceRcPtr();
            self->isconst = true;
            
            try
            {
                *self->cppobj = ColorSpace::Create();
                self->isconst = false;
                return 0;
            }
            catch ( const std::exception & e )
            {
                std::string message = "Cannot create colorSpace: ";
                message += e.what();
                PyErr_SetString( PyExc_RuntimeError, message.c_str() );
                return -1;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        void PyOCIO_ColorSpace_delete( PyOCIO_ColorSpace *self, PyObject * /*args*/ )
        {
            delete self->constcppobj;
            delete self->cppobj;
            
            self->ob_type->tp_free((PyObject*)self);
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColorSpace_isEditable( PyObject * self )
        {
            return PyBool_FromLong(IsPyColorSpaceEditable(self));
        }
        
        PyObject * PyOCIO_ColorSpace_createEditableCopy( PyObject * self )
        {
            try
            {
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
                ColorSpaceRcPtr copy = colorSpace->createEditableCopy();
                return BuildEditablePyColorSpace( copy );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColorSpace_getName( PyObject * self )
        {
            try
            {
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
                return PyString_FromString( colorSpace->getName() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColorSpace_setName( PyObject * self, PyObject * args )
        {
            try
            {
                char * name = 0;
                if (!PyArg_ParseTuple(args,"s:setName", &name)) return NULL;
                
                ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
                colorSpace->setName( name );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColorSpace_getFamily( PyObject * self )
        {
            try
            {
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
                return PyString_FromString( colorSpace->getFamily() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColorSpace_setFamily( PyObject * self, PyObject * args )
        {
            try
            {
                char * name = 0;
                if (!PyArg_ParseTuple(args,"s:setFamily", &name)) return NULL;
                
                ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
                colorSpace->setFamily( name );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColorSpace_getDescription( PyObject * self )
        {
            try
            {
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
                return PyString_FromString( colorSpace->getDescription() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColorSpace_setDescription( PyObject * self, PyObject * args )
        {
            try
            {
                char * name = 0;
                if (!PyArg_ParseTuple(args,"s:setDescription", &name)) return NULL;
                
                ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
                colorSpace->setDescription( name );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColorSpace_getBitDepth( PyObject * self )
        {
            try
            {
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
                return PyString_FromString( BitDepthToString( colorSpace->getBitDepth()) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColorSpace_setBitDepth( PyObject * self, PyObject * args )
        {
            try
            {
                char * name = 0;
                if (!PyArg_ParseTuple(args,"s:setBitDepth", &name)) return NULL;
                
                ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
                colorSpace->setBitDepth( BitDepthFromString( name ) );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColorSpace_isData( PyObject * self )
        {
            try
            {
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
                return PyBool_FromLong( colorSpace->isData() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColorSpace_setIsData( PyObject * self, PyObject * args )
        {
            try
            {
                bool isData = false;
                if (!PyArg_ParseTuple(args,"O&:setIsData",
                    ConvertPyObjectToBool, &isData)) return NULL;
                
                ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
                colorSpace->setIsData( isData );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColorSpace_getGpuAllocation( PyObject * self )
        {
            try
            {
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
                return PyString_FromString( GpuAllocationToString( colorSpace->getGpuAllocation()) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColorSpace_setGpuAllocation( PyObject * self, PyObject * args )
        {
            try
            {
                GpuAllocation hwalloc;
                if (!PyArg_ParseTuple(args,"O&:setGpuAllocation",
                    ConvertPyObjectToGpuAllocation, &hwalloc)) return NULL;
                
                ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
                colorSpace->setGpuAllocation( hwalloc );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColorSpace_getGpuMin( PyObject * self )
        {
            try
            {
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
                return PyFloat_FromDouble( colorSpace->getGpuMin() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColorSpace_setGpuMin( PyObject * self, PyObject * args )
        {
            try
            {
                float val = 0.0;
                if (!PyArg_ParseTuple(args,"f:setGpuMin", &val)) return NULL;
                
                ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
                colorSpace->setGpuMin( val );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColorSpace_getGpuMax( PyObject * self )
        {
            try
            {
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
                return PyFloat_FromDouble( colorSpace->getGpuMax() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColorSpace_setGpuMax( PyObject * self, PyObject * args )
        {
            try
            {
                float val = 0.0;
                if (!PyArg_ParseTuple(args,"f:setGpuMax", &val)) return NULL;
                
                ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
                colorSpace->setGpuMax( val );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColorSpace_getTransform( PyObject * self,  PyObject *args )
        {
            try
            {
                ColorSpaceDirection dir;
                if (!PyArg_ParseTuple(args,"O&:getEditableTransform",
                    ConvertPyObjectToColorSpaceDirection, &dir)) return NULL;
                
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
                ConstTransformRcPtr transform = colorSpace->getTransform(dir);
                return BuildConstPyTransform(transform);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColorSpace_getEditableTransform( PyObject * self,  PyObject *args )
        {
            try
            {
                ColorSpaceDirection dir;
                if (!PyArg_ParseTuple(args,"O&:getEditableTransform",
                    ConvertPyObjectToColorSpaceDirection, &dir)) return NULL;
                
                ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
                TransformRcPtr transform = colorSpace->getEditableTransform(dir);
                return BuildEditablePyTransform(transform);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColorSpace_setTransform( PyObject * self,  PyObject *args )
        {
        
            try
            {
                PyObject * pytransform = 0;
                ColorSpaceDirection dir;
                if (!PyArg_ParseTuple(args,"OO&:setTransform", &pytransform,
                    ConvertPyObjectToColorSpaceDirection, &dir)) return NULL;
                
                ConstGroupTransformRcPtr transform = GetConstGroupTransform(pytransform, true);
                ColorSpaceRcPtr colorSpace = GetEditableColorSpace(self);
                colorSpace->setTransform(transform, dir);
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColorSpace_isTransformSpecified( PyObject * self,  PyObject *args )
        {
            try
            {
                ColorSpaceDirection dir;
                if (!PyArg_ParseTuple(args,"O&:getEditableTransform",
                    ConvertPyObjectToColorSpaceDirection, &dir)) return NULL;
                
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(self, true);
                
                return PyBool_FromLong(colorSpace->isTransformSpecified(dir));
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
