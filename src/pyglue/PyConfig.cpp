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
#include "PyConfig.h"
#include "PyUtil.h"

#include <sstream>

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    bool AddConfigObjectToModule( PyObject* m )
    {
        PyOCIO_ConfigType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_ConfigType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_ConfigType );
        PyModule_AddObject(m, "Config",
                (PyObject *)&PyOCIO_ConfigType);
        
        return true;
    }
    
    
    PyObject * BuildConstPyConfig(ConstConfigRcPtr config)
    {
        if (config.get() == 0x0)
        {
            PyErr_SetString(PyExc_ValueError, "Cannot create PyConfig from null object.");
            return NULL;
        }
        
        PyOCIO_Config * pyconfig = PyObject_New(
                PyOCIO_Config, (PyTypeObject * ) &PyOCIO_ConfigType);
        
        pyconfig->constcppobj = new OCIO::ConstConfigRcPtr();
        *pyconfig->constcppobj = config;
        
        pyconfig->cppobj = new OCIO::ConfigRcPtr();
        pyconfig->isconst = true;
        
        return ( PyObject * ) pyconfig;
    }
    
    PyObject * BuildEditablePyConfig(ConfigRcPtr config)
    {
        if (config.get() == 0x0)
        {
            PyErr_SetString(PyExc_ValueError, "Cannot create PyConfig from null object.");
            return NULL;
        }
        
        PyOCIO_Config * pyconfig = PyObject_New(
                PyOCIO_Config, (PyTypeObject * ) &PyOCIO_ConfigType);
        
        pyconfig->constcppobj = new OCIO::ConstConfigRcPtr();
        pyconfig->cppobj = new OCIO::ConfigRcPtr();
        *pyconfig->cppobj = config;
        
        pyconfig->isconst = false;
        
        return ( PyObject * ) pyconfig;
    }
    
    
    bool IsPyConfig(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return (PyObject_Type(pyobject) == (PyObject *) (&PyOCIO_ConfigType));
    }
    
    bool IsPyConfigEditable(PyObject * pyobject)
    {
        if(!IsPyConfig(pyobject))
        {
            throw Exception("PyObject must be an OCIO::Config.");
        }
        
        PyOCIO_Config * pyconfig = reinterpret_cast<PyOCIO_Config *> (pyobject);
        return (!pyconfig->isconst);
    }
    
    ConstConfigRcPtr GetConstConfig(PyObject * pyobject, bool allowCast)
    {
        if(!IsPyConfig(pyobject))
        {
            throw Exception("PyObject must be an OCIO::Config.");
        }
        
        PyOCIO_Config * pyconfig = reinterpret_cast<PyOCIO_Config *> (pyobject);
        if(pyconfig->isconst && pyconfig->constcppobj)
        {
            return *pyconfig->constcppobj;
        }
        
        if(allowCast && !pyconfig->isconst && pyconfig->cppobj)
        {
            return *pyconfig->cppobj;
        }
        
        throw Exception("PyObject must be a valid OCIO::Config.");
    }
    
    ConfigRcPtr GetEditableConfig(PyObject * pyobject)
    {
        if(!IsPyConfig(pyobject))
        {
            throw Exception("PyObject must be an OCIO::Config.");
        }
        
        PyOCIO_Config * pyconfig = reinterpret_cast<PyOCIO_Config *> (pyobject);
        if(!pyconfig->isconst && pyconfig->cppobj)
        {
            return *pyconfig->cppobj;
        }
        
        throw Exception("PyObject must be an editable OCIO::Config.");
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    
    
    
    
    
    
    
    
    
    namespace
    {
        PyObject * PyOCIO_Config_CreateFromEnv( PyObject * cls );
        PyObject * PyOCIO_Config_CreateFromFile( PyObject * cls, PyObject * args );
        int PyOCIO_Config_init( PyOCIO_Config * self, PyObject * args, PyObject * kwds );
        void PyOCIO_Config_delete( PyOCIO_Config * self, PyObject * args );
        PyObject * PyOCIO_Config_isEditable( PyObject * self );
        PyObject * PyOCIO_Config_createEditableCopy( PyObject * self );
        
        PyObject * PyOCIO_Config_getResourcePath( PyObject * self );
        PyObject * PyOCIO_Config_setResourcePath( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_Config_getDescription( PyObject * self );
        PyObject * PyOCIO_Config_setDescription( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_Config_getColorSpaces( PyObject * self );
        PyObject * PyOCIO_Config_addColorSpace( PyObject * self, PyObject * args );
        
        PyObject * PyOCIO_Config_setColorSpaceForRole( PyObject * self, PyObject * args );
        
        PyObject * PyOCIO_Config_getXML( PyObject * self );
        
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_Config_methods[] = {
            {"CreateFromEnv", (PyCFunction) PyOCIO_Config_CreateFromEnv, METH_NOARGS | METH_CLASS, "" },
            {"CreateFromFile", PyOCIO_Config_CreateFromFile, METH_VARARGS | METH_CLASS, "" },
            {"isEditable", (PyCFunction) PyOCIO_Config_isEditable, METH_NOARGS, "" },
            {"createEditableCopy", (PyCFunction) PyOCIO_Config_createEditableCopy, METH_NOARGS, "" },
            
            {"getResourcePath", (PyCFunction) PyOCIO_Config_getResourcePath, METH_NOARGS, "" },
            {"setResourcePath", PyOCIO_Config_setResourcePath, METH_VARARGS, "" },
            
            {"getDescription", (PyCFunction) PyOCIO_Config_getDescription, METH_NOARGS, "" },
            {"setDescription", PyOCIO_Config_setDescription, METH_VARARGS, "" },
            
            {"getColorSpaces", (PyCFunction) PyOCIO_Config_getColorSpaces, METH_NOARGS, "" },
            {"addColorSpace", PyOCIO_Config_addColorSpace, METH_VARARGS, "" },
            
            {"setColorSpaceForRole", PyOCIO_Config_setColorSpaceForRole, METH_VARARGS, "" },
            
            {"getXML", (PyCFunction) PyOCIO_Config_getXML, METH_NOARGS, "" },
            
            {NULL, NULL, 0, NULL}
        };
        
        const char underlyingObjectBadMessage[] =
                "Underlying OCIO::Config object is not valid.";
        
        const char underlyingObjectConstMessage[] =
                "Underlying OCIO::Config is not editable.";
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ConfigType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.Config",                               //tp_name
        sizeof(PyOCIO_Config),                       //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_Config_delete,            //tp_dealloc
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
        "Config",                                   //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_Config_methods,                       //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_Config_init,               //tp_init 
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
        PyObject * PyOCIO_Config_CreateFromEnv( PyObject * /*cls*/ )
        {
            try
            {
                return BuildConstPyConfig( Config::CreateFromEnv() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Config_CreateFromFile( PyObject * /*cls*/, PyObject * args )
        {
            try
            {
                char * filename = 0;
                if (!PyArg_ParseTuple(args,"s:CreateFromFile", &filename)) return NULL;
                
                return BuildConstPyConfig( Config::CreateFromFile(filename) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        
        ///////////////////////////////////////////////////////////////////////
        ///
        int PyOCIO_Config_init( PyOCIO_Config *self, PyObject * /*args*/, PyObject * /*kwds*/ )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            
            self->constcppobj = new OCIO::ConstConfigRcPtr();
            self->cppobj = new OCIO::ConfigRcPtr();
            self->isconst = true;
            
            try
            {
                *self->cppobj = OCIO::Config::Create();
                self->isconst = false;
                return 0;
            }
            catch ( const std::exception & e )
            {
                std::string message = "Cannot create config: ";
                message += e.what();
                PyErr_SetString( PyExc_RuntimeError, message.c_str() );
                return -1;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        void PyOCIO_Config_delete( PyOCIO_Config *self, PyObject * /*args*/ )
        {
            delete self->constcppobj;
            delete self->cppobj;
            
            self->ob_type->tp_free((PyObject*)self);
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_Config_isEditable( PyObject * self )
        {
            return PyBool_FromLong(IsPyConfigEditable(self));
        }
        
        PyObject * PyOCIO_Config_createEditableCopy( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                ConfigRcPtr copy = config->createEditableCopy();
                return BuildEditablePyConfig( copy );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_Config_getResourcePath( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                return PyString_FromString( config->getResourcePath() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Config_setResourcePath( PyObject * self, PyObject * args )
        {
            try
            {
                char * path = 0;
                if (!PyArg_ParseTuple(args,"s:setResourcePath", &path)) return NULL;
                
                ConfigRcPtr config = GetEditableConfig(self);
                config->setResourcePath( path );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_Config_getDescription( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                return PyString_FromString( config->getDescription() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Config_setDescription( PyObject * self, PyObject * args )
        {
            try
            {
                char * desc = 0;
                if (!PyArg_ParseTuple(args,"s:setDescription", &desc)) return NULL;
                
                ConfigRcPtr config = GetEditableConfig(self);
                config->setDescription( desc );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_Config_getColorSpaces( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                int numColorSpaces = config->getNumColorSpaces();
                
                PyObject* tuple = PyTuple_New( numColorSpaces );
                for(int i = 0; i<numColorSpaces; ++i)
                {
                    ConstColorSpaceRcPtr cs = config->getColorSpaceByIndex(i);
                    PyObject * pycs = BuildConstPyColorSpace(cs);
                    PyTuple_SetItem(tuple, i, pycs);
                }
                
                return tuple;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_Config_addColorSpace( PyObject * self, PyObject * args )
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                
                PyObject * pyColorSpace = 0;
                if (!PyArg_ParseTuple(args,"O:addColorSpace", &pyColorSpace)) return NULL;
                
                if(IsPyColorSpaceEditable(pyColorSpace))
                {
                    config->addColorSpace( GetEditableColorSpace(pyColorSpace) );
                }
                else
                {
                    config->addColorSpace( GetConstColorSpace(pyColorSpace, true) );
                }
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_Config_setColorSpaceForRole( PyObject * self, PyObject * args )
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                
                char * role = 0;
                char * csname = 0;
                
                if (!PyArg_ParseTuple(args,"ss:setColorSpaceForRole",
                    &role, &csname)) return NULL;
                
                config->setColorSpaceForRole(role, csname);
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        ////////////////////////////////////////////////////////////////////////
        
        
        PyObject * PyOCIO_Config_getXML( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                
                std::ostringstream os;
                
                config->writeXML(os);
                
                return PyString_FromString( os.str().c_str() );
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
