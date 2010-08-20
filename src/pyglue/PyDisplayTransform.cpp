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
#include "PyDisplayTransform.h"
#include "PyUtil.h"

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    bool AddDisplayTransformObjectToModule( PyObject* m )
    {
        PyOCIO_DisplayTransformType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_DisplayTransformType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_DisplayTransformType );
        PyModule_AddObject(m, "DisplayTransform",
                (PyObject *)&PyOCIO_DisplayTransformType);
        
        return true;
    }
    
    PyObject * BuildConstPyDisplayTransform(ConstDisplayTransformRcPtr transform)
    {
        if (transform.get() == 0x0)
        {
            PyErr_SetString(PyExc_ValueError, "Cannot create PyDisplayTransform from null object.");
            return NULL;
        }
        
        PyOCIO_DisplayTransform * pytransform = PyObject_New(
                PyOCIO_DisplayTransform, (PyTypeObject * ) &PyOCIO_DisplayTransformType);
        
        pytransform->constcppobj = new OCIO::ConstDisplayTransformRcPtr();
        *pytransform->constcppobj = transform;
        
        pytransform->cppobj = new OCIO::DisplayTransformRcPtr();
        pytransform->isconst = true;
        
        return ( PyObject * ) pytransform;
    }
    
    PyObject * BuildEditablePyDisplayTransform(DisplayTransformRcPtr transform)
    {
        if (transform.get() == 0x0)
        {
            PyErr_SetString(PyExc_ValueError, "Cannot create PyDisplayTransform from null object.");
            return NULL;
        }
        
        PyOCIO_DisplayTransform * pytransform = PyObject_New(
                PyOCIO_DisplayTransform, (PyTypeObject * ) &PyOCIO_DisplayTransformType);
        
        pytransform->constcppobj = new OCIO::ConstDisplayTransformRcPtr();
        pytransform->cppobj = new OCIO::DisplayTransformRcPtr();
        *pytransform->cppobj = transform;
        
        pytransform->isconst = false;
        
        return ( PyObject * ) pytransform;
    }
    
    bool IsPyDisplayTransform(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return (PyObject_Type(pyobject) == (PyObject *) (&PyOCIO_DisplayTransformType));
    }
    
    bool IsPyDisplayTransformEditable(PyObject * pyobject)
    {
        if(!IsPyDisplayTransform(pyobject))
        {
            throw Exception("PyObject must be an OCIO::DisplayTransform.");
        }
        
        PyOCIO_DisplayTransform * pytransform = reinterpret_cast<PyOCIO_DisplayTransform *> (pyobject);
        return (!pytransform->isconst);
    }
    
    ConstDisplayTransformRcPtr GetConstDisplayTransform(PyObject * pyobject, bool allowCast)
    {
        if(!IsPyDisplayTransform(pyobject))
        {
            throw Exception("PyObject must be an OCIO::DisplayTransform.");
        }
        
        PyOCIO_DisplayTransform * pytransform = reinterpret_cast<PyOCIO_DisplayTransform *> (pyobject);
        if(pytransform->isconst && pytransform->constcppobj)
        {
            return *pytransform->constcppobj;
        }
        
        if(allowCast && !pytransform->isconst && pytransform->cppobj)
        {
            return *pytransform->cppobj;
        }
        
        throw Exception("PyObject must be a valid OCIO::DisplayTransform.");
    }
    
    DisplayTransformRcPtr GetEditableDisplayTransform(PyObject * pyobject)
    {
        if(!IsPyDisplayTransform(pyobject))
        {
            throw Exception("PyObject must be an OCIO::DisplayTransform.");
        }
        
        PyOCIO_DisplayTransform * pytransform = reinterpret_cast<PyOCIO_DisplayTransform *> (pyobject);
        if(!pytransform->isconst && pytransform->cppobj)
        {
            return *pytransform->cppobj;
        }
        
        throw Exception("PyObject must be an editable OCIO::DisplayTransform.");
    }
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    
    
    
    
    
    
    
    
    
    namespace
    {
        int PyOCIO_DisplayTransform_init( PyOCIO_DisplayTransform * self, PyObject * args, PyObject * kwds );
        void PyOCIO_DisplayTransform_delete( PyOCIO_DisplayTransform * self, PyObject * args );
        PyObject * PyOCIO_DisplayTransform_isEditable( PyObject * self );
        PyObject * PyOCIO_DisplayTransform_createEditableCopy( PyObject * self );
        
        PyObject * PyOCIO_DisplayTransform_getDirection( PyObject * self );
        PyObject * PyOCIO_DisplayTransform_setDirection( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_DisplayTransform_getInputColorSpace( PyObject * self );
        PyObject * PyOCIO_DisplayTransform_setInputColorSpace( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_DisplayTransform_getDisplayColorSpace( PyObject * self );
        PyObject * PyOCIO_DisplayTransform_setDisplayColorSpace( PyObject * self,  PyObject *args );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_DisplayTransform_methods[] = {
            {"isEditable", (PyCFunction) PyOCIO_DisplayTransform_isEditable, METH_NOARGS, "" },
            {"createEditableCopy", (PyCFunction) PyOCIO_DisplayTransform_createEditableCopy, METH_NOARGS, "" },
            
            {"getDirection", (PyCFunction) PyOCIO_DisplayTransform_getDirection, METH_NOARGS, "" },
            {"setDirection", PyOCIO_DisplayTransform_setDirection, METH_VARARGS, "" },
            
            {"getInputColorSpace", (PyCFunction) PyOCIO_DisplayTransform_getInputColorSpace, METH_NOARGS, "" },
            {"setInputColorSpace", PyOCIO_DisplayTransform_setInputColorSpace, METH_VARARGS, "" },
            
            {"getDisplayColorSpace", (PyCFunction) PyOCIO_DisplayTransform_getDisplayColorSpace, METH_NOARGS, "" },
            {"setDisplayColorSpace", PyOCIO_DisplayTransform_setDisplayColorSpace, METH_VARARGS, "" },
            /*
            {"getInterpolation", (PyCFunction) PyOCIO_DisplayTransform_getInterpolation, METH_NOARGS, "" },
            {"setInterpolation", PyOCIO_DisplayTransform_setInterpolation, METH_VARARGS, "" },
            */
            {NULL, NULL, 0, NULL}
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_DisplayTransformType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.DisplayTransform",                    //tp_name
        sizeof(PyOCIO_DisplayTransform),            //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_DisplayTransform_delete, //tp_dealloc
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
        "DisplayTransform",                         //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_DisplayTransform_methods,            //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_DisplayTransform_init,    //tp_init 
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
        int PyOCIO_DisplayTransform_init( PyOCIO_DisplayTransform *self, PyObject * /*args*/, PyObject * /*kwds*/ )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            
            self->constcppobj = new OCIO::ConstDisplayTransformRcPtr();
            self->cppobj = new OCIO::DisplayTransformRcPtr();
            self->isconst = true;
            
            try
            {
                *self->cppobj = OCIO::DisplayTransform::Create();
                self->isconst = false;
                return 0;
            }
            catch ( const std::exception & e )
            {
                std::string message = "Cannot create DisplayTransform: ";
                message += e.what();
                PyErr_SetString( PyExc_RuntimeError, message.c_str() );
                return -1;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        void PyOCIO_DisplayTransform_delete( PyOCIO_DisplayTransform *self, PyObject * /*args*/ )
        {
            delete self->constcppobj;
            delete self->cppobj;
            
            self->ob_type->tp_free((PyObject*)self);
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_DisplayTransform_isEditable( PyObject * self )
        {
            return PyBool_FromLong(IsPyDisplayTransformEditable(self));
        }
        
        PyObject * PyOCIO_DisplayTransform_createEditableCopy( PyObject * self )
        {
            try
            {
                ConstDisplayTransformRcPtr transform = GetConstDisplayTransform(self, true);
                DisplayTransformRcPtr copy = DynamicPtrCast<DisplayTransform>(transform->createEditableCopy());
                return BuildEditablePyDisplayTransform( copy );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        
        PyObject * PyOCIO_DisplayTransform_getDirection( PyObject * self )
        {
            try
            {
                ConstDisplayTransformRcPtr transform = GetConstDisplayTransform(self, true);
                TransformDirection dir = transform->getDirection();
                return PyString_FromString( TransformDirectionToString( dir ) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_DisplayTransform_setDirection( PyObject * self, PyObject * args )
        {
            try
            {
                TransformDirection dir;
                if (!PyArg_ParseTuple(args,"O&:setDirection",
                    ConvertPyObjectToTransformDirection, &dir)) return NULL;
                
                DisplayTransformRcPtr transform = GetEditableDisplayTransform(self);
                transform->setDirection( dir );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_DisplayTransform_getInputColorSpace( PyObject * self )
        {
            try
            {
                ConstDisplayTransformRcPtr transform = GetConstDisplayTransform(self, true);
                ConstColorSpaceRcPtr colorSpace = transform->getInputColorSpace();
                
                return BuildConstPyColorSpace(colorSpace);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_DisplayTransform_setInputColorSpace( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyColorSpace = 0;
                if (!PyArg_ParseTuple(args,"O:setInputColorSpace", &pyColorSpace)) return NULL;
                
                DisplayTransformRcPtr transform = GetEditableDisplayTransform(self);
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(pyColorSpace, true);
                transform->setInputColorSpace( colorSpace );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_DisplayTransform_getDisplayColorSpace( PyObject * self )
        {
            try
            {
                ConstDisplayTransformRcPtr transform = GetConstDisplayTransform(self, true);
                ConstColorSpaceRcPtr colorSpace = transform->getDisplayColorSpace();
                
                return BuildConstPyColorSpace(colorSpace);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_DisplayTransform_setDisplayColorSpace( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyColorSpace = 0;
                if (!PyArg_ParseTuple(args,"O:setDisplayColorSpace", &pyColorSpace)) return NULL;
                
                DisplayTransformRcPtr transform = GetEditableDisplayTransform(self);
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(pyColorSpace, true);
                transform->setDisplayColorSpace( colorSpace );
                
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
