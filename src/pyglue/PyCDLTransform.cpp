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
#include "PyCDLTransform.h"
#include "PyUtil.h"

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    bool AddCDLTransformObjectToModule( PyObject* m )
    {
        PyOCIO_CDLTransformType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_CDLTransformType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_CDLTransformType );
        PyModule_AddObject(m, "CDLTransform",
                (PyObject *)&PyOCIO_CDLTransformType);
        
        return true;
    }
    
    PyObject * BuildConstPyCDLTransform(ConstCDLTransformRcPtr transform)
    {
        if (transform.get() == 0x0)
        {
            PyErr_SetString(PyExc_ValueError, "Cannot create PyCDLTransform from null object.");
            return NULL;
        }
        
        PyOCIO_CDLTransform * pytransform = PyObject_New(
                PyOCIO_CDLTransform, (PyTypeObject * ) &PyOCIO_CDLTransformType);
        
        pytransform->constcppobj = new OCIO::ConstCDLTransformRcPtr();
        *pytransform->constcppobj = transform;
        
        pytransform->cppobj = new OCIO::CDLTransformRcPtr();
        pytransform->isconst = true;
        
        return ( PyObject * ) pytransform;
    }
    
    PyObject * BuildEditablePyCDLTransform(CDLTransformRcPtr transform)
    {
        if (transform.get() == 0x0)
        {
            PyErr_SetString(PyExc_ValueError, "Cannot create PyCDLTransform from null object.");
            return NULL;
        }
        
        PyOCIO_CDLTransform * pytransform = PyObject_New(
                PyOCIO_CDLTransform, (PyTypeObject * ) &PyOCIO_CDLTransformType);
        
        pytransform->constcppobj = new OCIO::ConstCDLTransformRcPtr();
        pytransform->cppobj = new OCIO::CDLTransformRcPtr();
        *pytransform->cppobj = transform;
        
        pytransform->isconst = false;
        
        return ( PyObject * ) pytransform;
    }
    
    bool IsPyCDLTransform(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return (PyObject_Type(pyobject) == (PyObject *) (&PyOCIO_CDLTransformType));
    }
    
    bool IsPyCDLTransformEditable(PyObject * pyobject)
    {
        if(!IsPyCDLTransform(pyobject))
        {
            throw Exception("PyObject must be an OCIO::CDLTransform.");
        }
        
        PyOCIO_CDLTransform * pytransform = reinterpret_cast<PyOCIO_CDLTransform *> (pyobject);
        return (!pytransform->isconst);
    }
    
    ConstCDLTransformRcPtr GetConstCDLTransform(PyObject * pyobject, bool allowCast)
    {
        if(!IsPyCDLTransform(pyobject))
        {
            throw Exception("PyObject must be an OCIO::CDLTransform.");
        }
        
        PyOCIO_CDLTransform * pytransform = reinterpret_cast<PyOCIO_CDLTransform *> (pyobject);
        if(pytransform->isconst && pytransform->constcppobj)
        {
            return *pytransform->constcppobj;
        }
        
        if(allowCast && !pytransform->isconst && pytransform->cppobj)
        {
            return *pytransform->cppobj;
        }
        
        throw Exception("PyObject must be a valid OCIO::CDLTransform.");
    }
    
    CDLTransformRcPtr GetEditableCDLTransform(PyObject * pyobject)
    {
        if(!IsPyCDLTransform(pyobject))
        {
            throw Exception("PyObject must be an OCIO::CDLTransform.");
        }
        
        PyOCIO_CDLTransform * pytransform = reinterpret_cast<PyOCIO_CDLTransform *> (pyobject);
        if(!pytransform->isconst && pytransform->cppobj)
        {
            return *pytransform->cppobj;
        }
        
        throw Exception("PyObject must be an editable OCIO::CDLTransform.");
    }
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    
    
    
    
    
    
    
    
    
    namespace
    {
        int PyOCIO_CDLTransform_init( PyOCIO_CDLTransform * self, PyObject * args, PyObject * kwds );
        void PyOCIO_CDLTransform_delete( PyOCIO_CDLTransform * self, PyObject * args );
        PyObject * PyOCIO_CDLTransform_isEditable( PyObject * self );
        PyObject * PyOCIO_CDLTransform_createEditableCopy( PyObject * self );
        
        PyObject * PyOCIO_CDLTransform_getDirection( PyObject * self );
        PyObject * PyOCIO_CDLTransform_setDirection( PyObject * self,  PyObject *args );
        /*
        PyObject * PyOCIO_CDLTransform_getInputColorSpace( PyObject * self );
        PyObject * PyOCIO_CDLTransform_setInputColorSpace( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_CDLTransform_getLinearExposure( PyObject * self );
        PyObject * PyOCIO_CDLTransform_setLinearExposure( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_CDLTransform_getCDLColorSpace( PyObject * self );
        PyObject * PyOCIO_CDLTransform_setCDLColorSpace( PyObject * self,  PyObject *args );
        */
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_CDLTransform_methods[] = {
            {"isEditable", (PyCFunction) PyOCIO_CDLTransform_isEditable, METH_NOARGS, "" },
            {"createEditableCopy", (PyCFunction) PyOCIO_CDLTransform_createEditableCopy, METH_NOARGS, "" },
            
            {"getDirection", (PyCFunction) PyOCIO_CDLTransform_getDirection, METH_NOARGS, "" },
            {"setDirection", PyOCIO_CDLTransform_setDirection, METH_VARARGS, "" },
            
            /*
            {"getInputColorSpace", (PyCFunction) PyOCIO_CDLTransform_getInputColorSpace, METH_NOARGS, "" },
            {"setInputColorSpace", PyOCIO_CDLTransform_setInputColorSpace, METH_VARARGS, "" },
            
            {"getLinearExposure", (PyCFunction) PyOCIO_CDLTransform_getLinearExposure, METH_NOARGS, "" },
            {"setLinearExposure", PyOCIO_CDLTransform_setLinearExposure, METH_VARARGS, "" },
            
            {"getCDLColorSpace", (PyCFunction) PyOCIO_CDLTransform_getCDLColorSpace, METH_NOARGS, "" },
            {"setCDLColorSpace", PyOCIO_CDLTransform_setCDLColorSpace, METH_VARARGS, "" },
            */
            /*
            {"getInterpolation", (PyCFunction) PyOCIO_CDLTransform_getInterpolation, METH_NOARGS, "" },
            {"setInterpolation", PyOCIO_CDLTransform_setInterpolation, METH_VARARGS, "" },
            */
            {NULL, NULL, 0, NULL}
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_CDLTransformType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.CDLTransform",                    //tp_name
        sizeof(PyOCIO_CDLTransform),            //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_CDLTransform_delete, //tp_dealloc
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
        "CDLTransform",                         //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_CDLTransform_methods,            //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_CDLTransform_init,    //tp_init 
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
        int PyOCIO_CDLTransform_init( PyOCIO_CDLTransform *self, PyObject * /*args*/, PyObject * /*kwds*/ )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            
            self->constcppobj = new OCIO::ConstCDLTransformRcPtr();
            self->cppobj = new OCIO::CDLTransformRcPtr();
            self->isconst = true;
            
            try
            {
                *self->cppobj = OCIO::CDLTransform::Create();
                self->isconst = false;
                return 0;
            }
            catch ( const std::exception & e )
            {
                std::string message = "Cannot create CDLTransform: ";
                message += e.what();
                PyErr_SetString( PyExc_RuntimeError, message.c_str() );
                return -1;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        void PyOCIO_CDLTransform_delete( PyOCIO_CDLTransform *self, PyObject * /*args*/ )
        {
            delete self->constcppobj;
            delete self->cppobj;
            
            self->ob_type->tp_free((PyObject*)self);
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_CDLTransform_isEditable( PyObject * self )
        {
            return PyBool_FromLong(IsPyCDLTransformEditable(self));
        }
        
        PyObject * PyOCIO_CDLTransform_createEditableCopy( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                CDLTransformRcPtr copy = DynamicPtrCast<CDLTransform>(transform->createEditableCopy());
                return BuildEditablePyCDLTransform( copy );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        
        PyObject * PyOCIO_CDLTransform_getDirection( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                TransformDirection dir = transform->getDirection();
                return PyString_FromString( TransformDirectionToString( dir ) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CDLTransform_setDirection( PyObject * self, PyObject * args )
        {
            try
            {
                TransformDirection dir;
                if (!PyArg_ParseTuple(args,"O&:setDirection",
                    ConvertPyObjectToTransformDirection, &dir)) return NULL;
                
                CDLTransformRcPtr transform = GetEditableCDLTransform(self);
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
        
        /*
        PyObject * PyOCIO_CDLTransform_getInputColorSpace( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                ConstColorSpaceRcPtr colorSpace = transform->getInputColorSpace();
                
                return BuildConstPyColorSpace(colorSpace);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CDLTransform_setInputColorSpace( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyColorSpace = 0;
                if (!PyArg_ParseTuple(args,"O:setInputColorSpace", &pyColorSpace)) return NULL;
                
                CDLTransformRcPtr transform = GetEditableCDLTransform(self);
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
        
        PyObject * PyOCIO_CDLTransform_getLinearExposure( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                
                std::vector<float> coef(4);
                transform->getLinearExposure(&coef[0]);
                
                return CreatePyListFromFloatVector(coef);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CDLTransform_setLinearExposure( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyVec = 0;
                if (!PyArg_ParseTuple(args,"O:setLinearExposure", &pyVec)) return NULL;
                
                CDLTransformRcPtr transform = GetEditableCDLTransform(self);
                
                std::vector<float> vec;
                if(!FillFloatVectorFromPySequence(pyVec, vec) || (vec.size() != 4))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 4");
                    return 0;
                }
                
                transform->setLinearExposure(&vec[0]);
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_CDLTransform_getCDLColorSpace( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                ConstColorSpaceRcPtr colorSpace = transform->getCDLColorSpace();
                
                return BuildConstPyColorSpace(colorSpace);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CDLTransform_setCDLColorSpace( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyColorSpace = 0;
                if (!PyArg_ParseTuple(args,"O:setCDLColorSpace", &pyColorSpace)) return NULL;
                
                CDLTransformRcPtr transform = GetEditableCDLTransform(self);
                ConstColorSpaceRcPtr colorSpace = GetConstColorSpace(pyColorSpace, true);
                transform->setCDLColorSpace( colorSpace );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
    */
    }

}
OCIO_NAMESPACE_EXIT
