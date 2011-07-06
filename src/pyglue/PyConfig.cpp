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
        if (!config.get())
        {
            Py_RETURN_NONE;
        }
        
        PyOCIO_Config * pyconfig = PyObject_New(
                PyOCIO_Config, (PyTypeObject * ) &PyOCIO_ConfigType);
        
        pyconfig->constcppobj = new ConstConfigRcPtr();
        *pyconfig->constcppobj = config;
        
        pyconfig->cppobj = new ConfigRcPtr();
        pyconfig->isconst = true;
        
        return ( PyObject * ) pyconfig;
    }
    
    PyObject * BuildEditablePyConfig(ConfigRcPtr config)
    {
        if (!config)
        {
            Py_RETURN_NONE;
        }
        
        PyOCIO_Config * pyconfig = PyObject_New(
                PyOCIO_Config, (PyTypeObject * ) &PyOCIO_ConfigType);
        
        pyconfig->constcppobj = new ConstConfigRcPtr();
        pyconfig->cppobj = new ConfigRcPtr();
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
            throw Exception("PyObject must be an OCIO.Config.");
        }
        
        PyOCIO_Config * pyconfig = reinterpret_cast<PyOCIO_Config *> (pyobject);
        return (!pyconfig->isconst);
    }
    
    ConstConfigRcPtr GetConstConfig(PyObject * pyobject, bool allowCast)
    {
        if(!IsPyConfig(pyobject))
        {
            throw Exception("PyObject must be an OCIO.Config.");
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
        
        throw Exception("PyObject must be a valid OCIO.Config.");
    }
    
    ConfigRcPtr GetEditableConfig(PyObject * pyobject)
    {
        if(!IsPyConfig(pyobject))
        {
            throw Exception("PyObject must be an OCIO.Config.");
        }
        
        PyOCIO_Config * pyconfig = reinterpret_cast<PyOCIO_Config *> (pyobject);
        if(!pyconfig->isconst && pyconfig->cppobj)
        {
            return *pyconfig->cppobj;
        }
        
        throw Exception("PyObject must be an editable OCIO.Config.");
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    
    
    
    
    
    
    
    
    
    namespace
    {
        PyObject * PyOCIO_Config_CreateFromEnv( PyObject * cls );
        PyObject * PyOCIO_Config_CreateFromFile( PyObject * cls, PyObject * args );
        PyObject * PyOCIO_Config_CreateFromText( PyObject * cls, PyObject * args );
        
        int PyOCIO_Config_init( PyOCIO_Config * self, PyObject * args, PyObject * kwds );
        void PyOCIO_Config_delete( PyOCIO_Config * self, PyObject * args );
        
        PyObject * PyOCIO_Config_isEditable( PyObject * self );
        PyObject * PyOCIO_Config_createEditableCopy( PyObject * self );
        
        PyObject * PyOCIO_Config_sanityCheck( PyObject * self );
        
        PyObject * PyOCIO_Config_getDescription( PyObject * self );
        PyObject * PyOCIO_Config_setDescription( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_Config_serialize( PyObject * self );
        PyObject * PyOCIO_Config_getCacheID( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_Config_getSearchPath( PyObject * self );
        PyObject * PyOCIO_Config_setSearchPath( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_Config_getWorkingDir( PyObject * self );
        PyObject * PyOCIO_Config_setWorkingDir( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_Config_getColorSpaces( PyObject * self );
        PyObject * PyOCIO_Config_getColorSpace( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Config_addColorSpace( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Config_clearColorSpaces( PyObject * self );
        PyObject * PyOCIO_Config_parseColorSpaceFromString( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Config_setRole( PyObject * self, PyObject * args );
        
        PyObject * PyOCIO_Config_getDefaultDisplay( PyObject * self );
        PyObject * PyOCIO_Config_getDisplays( PyObject * self );
        PyObject * PyOCIO_Config_getDefaultView( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Config_getViews( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Config_getDisplayColorSpaceName( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Config_getDisplayLooks( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Config_setDisplayColorSpaceName( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Config_addDisplay( PyObject * self, PyObject * args, PyObject * kwargs );
        PyObject * PyOCIO_Config_clearDisplays( PyObject * self );
        PyObject * PyOCIO_Config_setActiveDisplays( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Config_getActiveDisplays( PyObject * self );
        PyObject * PyOCIO_Config_setActiveViews( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Config_getActiveViews( PyObject * self );
        
        PyObject * PyOCIO_Config_getDefaultLumaCoefs( PyObject * self );
        PyObject * PyOCIO_Config_setDefaultLumaCoefs( PyObject * self, PyObject * args );
        
        PyObject * PyOCIO_Config_getLooks( PyObject * self );
        PyObject * PyOCIO_Config_addLook( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Config_clearLook( PyObject * self );
        
        PyObject * PyOCIO_Config_getProcessor( PyObject * self, PyObject * args, PyObject * kwargs );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_Config_methods[] = {
            {"CreateFromEnv", (PyCFunction) PyOCIO_Config_CreateFromEnv, METH_NOARGS | METH_CLASS, "" },
            {"CreateFromFile", PyOCIO_Config_CreateFromFile, METH_VARARGS | METH_CLASS, "" },
            {"isEditable", (PyCFunction) PyOCIO_Config_isEditable, METH_NOARGS, "" },
            {"createEditableCopy", (PyCFunction) PyOCIO_Config_createEditableCopy, METH_NOARGS, "" },
            
            {"sanityCheck", (PyCFunction) PyOCIO_Config_sanityCheck, METH_NOARGS, "" },
            
            {"getDescription", (PyCFunction) PyOCIO_Config_getDescription, METH_NOARGS, "" },
            {"setDescription", PyOCIO_Config_setDescription, METH_VARARGS, "" },
            {"serialize", (PyCFunction) PyOCIO_Config_serialize, METH_NOARGS, "" },
            {"getCacheID", PyOCIO_Config_getCacheID, METH_VARARGS, "" },
            
            {"getSearchPath", (PyCFunction) PyOCIO_Config_getSearchPath, METH_NOARGS, "" },
            {"setSearchPath", PyOCIO_Config_setSearchPath, METH_VARARGS, "" },
            {"getWorkingDir", (PyCFunction) PyOCIO_Config_getWorkingDir, METH_NOARGS, "" },
            {"setWorkingDir", PyOCIO_Config_setWorkingDir, METH_VARARGS, "" },
            
            {"getColorSpaces", (PyCFunction) PyOCIO_Config_getColorSpaces, METH_NOARGS, "" },
            {"getColorSpace", PyOCIO_Config_getColorSpace, METH_VARARGS, "" },
            {"addColorSpace", PyOCIO_Config_addColorSpace, METH_VARARGS, "" },
            {"clearColorSpaces", (PyCFunction) PyOCIO_Config_clearColorSpaces, METH_NOARGS, "" },
            {"parseColorSpaceFromString", PyOCIO_Config_parseColorSpaceFromString, METH_VARARGS, "" },
            {"setRole", PyOCIO_Config_setRole, METH_VARARGS, "" },
            
            {"getDefaultDisplay", (PyCFunction) PyOCIO_Config_getDefaultDisplay, METH_NOARGS, "" },
            {"getDisplays", (PyCFunction) PyOCIO_Config_getDisplays, METH_NOARGS, "" },
            {"getDefaultView", PyOCIO_Config_getDefaultView, METH_VARARGS, "" },
            {"getViews", PyOCIO_Config_getViews, METH_VARARGS, "" },
            {"getDisplayColorSpaceName", PyOCIO_Config_getDisplayColorSpaceName, METH_VARARGS, "" },
            {"getDisplayLooks", PyOCIO_Config_getDisplayLooks, METH_VARARGS, "" },
            {"setDisplayColorSpaceName", PyOCIO_Config_setDisplayColorSpaceName, METH_VARARGS, "" },
            {"addDisplay", (PyCFunction) PyOCIO_Config_addDisplay, METH_KEYWORDS, "" },
            {"clearDisplays", (PyCFunction) PyOCIO_Config_clearDisplays, METH_NOARGS, "" },
            {"setActiveDisplays", PyOCIO_Config_setActiveDisplays, METH_VARARGS, "" },
            {"getActiveDisplays", (PyCFunction) PyOCIO_Config_getActiveDisplays, METH_NOARGS, "" },
            {"setActiveViews", PyOCIO_Config_setActiveViews, METH_VARARGS, "" },
            {"getActiveViews", (PyCFunction) PyOCIO_Config_getActiveViews, METH_NOARGS, "" },
            
            {"getDefaultLumaCoefs", (PyCFunction) PyOCIO_Config_getDefaultLumaCoefs, METH_NOARGS, "" },
            {"setDefaultLumaCoefs", PyOCIO_Config_setDefaultLumaCoefs, METH_VARARGS, "" },
            
            {"getLooks", (PyCFunction) PyOCIO_Config_getLooks, METH_NOARGS, "" },
            {"addLook", PyOCIO_Config_addLook, METH_VARARGS, "" },
            {"clearLook", (PyCFunction) PyOCIO_Config_clearLook, METH_NOARGS, "" },
            
            {"getProcessor", (PyCFunction) PyOCIO_Config_getProcessor, METH_KEYWORDS, "" },
            
            {NULL, NULL, 0, NULL}
        };
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
            
            self->constcppobj = new ConstConfigRcPtr();
            self->cppobj = new ConfigRcPtr();
            self->isconst = true;
            
            try
            {
                *self->cppobj = Config::Create();
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
        
        PyObject * PyOCIO_Config_sanityCheck( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                config->sanityCheck();
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
        
        
        PyObject * PyOCIO_Config_serialize( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                
                std::ostringstream os;
                
                config->serialize(os);
                
                return PyString_FromString( os.str().c_str() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        
        }
        
        
        PyObject * PyOCIO_Config_getCacheID( PyObject * self, PyObject * args )
        {
            try
            {   PyObject * pycontext = NULL;
                if (!PyArg_ParseTuple(args,"|O:getCacheID", &pycontext)) return NULL;
                
                ConstConfigRcPtr config = GetConstConfig(self, true);
                
                // Parse the context
                ConstContextRcPtr context;
                if(pycontext != NULL)
                {
                    context = GetConstContext(pycontext, true);
                }
                else
                {
                    context = config->getCurrentContext();
                }
                
                return PyString_FromString( config->getCacheID(context) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_Config_getSearchPath( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                return PyString_FromString( config->getSearchPath() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Config_setSearchPath( PyObject * self, PyObject * args )
        {
            try
            {
                char * path = 0;
                if (!PyArg_ParseTuple(args,"s:setSearchPath", &path)) return NULL;
                
                ConfigRcPtr config = GetEditableConfig(self);
                config->setSearchPath( path );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Config_getWorkingDir( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                return PyString_FromString( config->getWorkingDir() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Config_setWorkingDir( PyObject * self, PyObject * args )
        {
            try
            {
                char * path = 0;
                if (!PyArg_ParseTuple(args,"s:setWorkingDir", &path)) return NULL;
                
                ConfigRcPtr config = GetEditableConfig(self);
                config->setWorkingDir( path );
                
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
                    const char * name = config->getColorSpaceNameByIndex(i);
                    ConstColorSpaceRcPtr cs = config->getColorSpace(name);
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
        
        PyObject * PyOCIO_Config_getColorSpace( PyObject * self, PyObject * args )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                
                char * name = 0;
                if (!PyArg_ParseTuple(args,"s:getColorSpace", &name)) return NULL;
                
                return BuildConstPyColorSpace(config->getColorSpace(name));
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
                
                config->addColorSpace( GetConstColorSpace(pyColorSpace, true) );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        PyObject * PyOCIO_Config_clearColorSpaces( PyObject * self )
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                config->clearColorSpaces();
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Config_parseColorSpaceFromString( PyObject * self, PyObject * args )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                
                char * str = 0;
                
                if (!PyArg_ParseTuple(args,"s:parseColorSpaceFromString",
                    &str)) return NULL;
                
                const char * cs = config->parseColorSpaceFromString(str);
                if(cs)
                {
                    return PyString_FromString( cs );
                }
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        PyObject * PyOCIO_Config_setRole( PyObject * self, PyObject * args )
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                
                char * role = 0;
                char * csname = 0;
                
                if (!PyArg_ParseTuple(args,"ss:setColorSpaceForRole",
                    &role, &csname)) return NULL;
                
                config->setRole(role, csname);
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        
        ////////////////////////////////////////////////////////////////////////
        
        
        PyObject * PyOCIO_Config_getDefaultDisplay( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                return PyString_FromString( config->getDefaultDisplay() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        PyObject * PyOCIO_Config_getDisplays( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                
                std::vector<std::string> data;
                int numDevices = config->getNumDisplays();
                
                for(int i=0; i<numDevices; ++i)
                {
                    data.push_back( config->getDisplay(i) );
                }
                
                return CreatePyListFromStringVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        PyObject * PyOCIO_Config_getDefaultView( PyObject * self, PyObject * args )
        {
            try
            {
                char * display = 0;
                if (!PyArg_ParseTuple(args,"s:getDefaultView",
                    &display)) return NULL;
                
                ConstConfigRcPtr config = GetConstConfig(self, true);
                return PyString_FromString( config->getDefaultView(display) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        PyObject * PyOCIO_Config_getViews( PyObject * self, PyObject * args )
        {
            try
            {
                char * display = 0;
                if (!PyArg_ParseTuple(args,"s:getViews",
                    &display)) return NULL;
                
                ConstConfigRcPtr config = GetConstConfig(self, true);
                std::vector<std::string> data;
                int num = config->getNumViews(display);
                for(int i=0; i<num; ++i)
                {
                    data.push_back( config->getView(display, i) );
                }
                
                return CreatePyListFromStringVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        PyObject * PyOCIO_Config_getDisplayColorSpaceName( PyObject * self, PyObject * args )
        {
            try
            {
                char * display = 0;
                char * view = 0;
                if (!PyArg_ParseTuple(args,"ss:getDisplayColorSpaceName",
                    &display, &view)) return NULL;
                
                ConstConfigRcPtr config = GetConstConfig(self, true);
                return PyString_FromString( config->getDisplayColorSpaceName(display, view) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        PyObject * PyOCIO_Config_getDisplayLooks( PyObject * self, PyObject * args )
        {
            try
            {
                char * display = 0;
                char * view = 0;
                if (!PyArg_ParseTuple(args,"ss:getDisplayLooks",
                    &display, &view)) return NULL;
                
                ConstConfigRcPtr config = GetConstConfig(self, true);
                return PyString_FromString( config->getDisplayLooks(display, view) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        PyObject * PyOCIO_Config_setDisplayColorSpaceName( PyObject * self, PyObject * args )
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                
                char * display = 0;
                char * view = 0;
                char * csname = 0;
                
                if (!PyArg_ParseTuple(args,"sss:setDisplayColorSpaceName",
                    &display, &view, &csname)) return NULL;
                
                config->setDisplayColorSpaceName(display, view, csname);
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        /*
        PyObject * PyOCIO_Config_getProcessor( PyObject * self, PyObject * args, PyObject * kwargs)
        {
            try
            {
                // We want this call to be as flexible as possible.
                // arg1 will either be a PyTransform
                // or arg1, arg2 will be {str, ColorSpace}
                
                PyObject * arg1 = Py_None;
                PyObject * arg2 = Py_None;
                
                const char * direction = 0;
                PyObject * pycontext = Py_None;
                
                const char * kwlist[] = {"arg1", "arg2", "direction", "context",  NULL};
                
                if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OsO",
                    const_cast<char**>(kwlist),
                    &arg1, &arg2, &direction, &pycontext))
                    return 0;
        */
        
        PyObject * PyOCIO_Config_addDisplay( PyObject * self, PyObject * args, PyObject * kwargs)
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                
                char * display = 0;
                char * view = 0;
                char * colorSpaceName = 0;
                char * looks = 0;
                
                const char * kwlist[] = {"display", "view", "colorSpaceName", "looks",  NULL};
                
                if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sss|s",
                    const_cast<char**>(kwlist),
                    &display, &view, &colorSpaceName, &looks))
                    return 0;
                
                std::string lookStr;
                if(looks) lookStr = looks;
                
                config->addDisplay(display, view, colorSpaceName, lookStr.c_str());
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        
        
        PyObject * PyOCIO_Config_clearDisplays( PyObject * self )
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                config->clearDisplays();
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        
        
        PyObject * PyOCIO_Config_setActiveDisplays( PyObject * self, PyObject * args )
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                
                char * displays = 0;
                
                if (!PyArg_ParseTuple(args,"s:setActiveDisplays",
                    &displays)) return NULL;
                
                config->setActiveDisplays(displays);
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        PyObject * PyOCIO_Config_getActiveDisplays( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                return PyString_FromString( config->getActiveDisplays() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Config_setActiveViews( PyObject * self, PyObject * args )
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                
                char * views = 0;
                
                if (!PyArg_ParseTuple(args,"s:setActiveViews",
                    &views)) return NULL;
                
                config->setActiveViews(views);
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        PyObject * PyOCIO_Config_getActiveViews( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                return PyString_FromString( config->getActiveViews() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        
        
        
        
        
        ////////////////////////////////////////////////////////////////////////
        
        
        
        PyObject * PyOCIO_Config_setDefaultLumaCoefs( PyObject * self, PyObject * args )
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                
                PyObject* pyCoef = 0;
                if (!PyArg_ParseTuple(args, "O", &pyCoef))
                {
                    return 0;
                }
                
                std::vector<float> coef;
                if(!FillFloatVectorFromPySequence(pyCoef, coef) || (coef.size() != 3))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 3");
                    return 0;
                }
                
                config->setDefaultLumaCoefs(&coef[0]);
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        
        
        PyObject * PyOCIO_Config_getDefaultLumaCoefs( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                
                std::vector<float> coef(3);
                config->getDefaultLumaCoefs(&coef[0]);
                
                return CreatePyListFromFloatVector(coef);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        
        PyObject * PyOCIO_Config_getLooks( PyObject * self )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                int num = config->getNumLooks();
                
                PyObject* tuple = PyTuple_New( num );
                for(int i = 0; i<num; ++i)
                {
                    const char * name = config->getLookNameByIndex(i);
                    ConstLookRcPtr look = config->getLook(name);
                    PyObject * pylook = BuildConstPyLook(look);
                    PyTuple_SetItem(tuple, i, pylook);
                }
                
                return tuple;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Config_addLook( PyObject * self, PyObject * args )
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                
                PyObject * pyLook = 0;
                if (!PyArg_ParseTuple(args,"O:addLook", &pyLook)) return NULL;
                
                config->addLook( GetConstLook(pyLook, true) );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_Config_clearLook( PyObject * self )
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                config->clearLooks();
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_Config_getProcessor( PyObject * self, PyObject * args, PyObject * kwargs)
        {
            try
            {
                // We want this call to be as flexible as possible.
                // arg1 will either be a PyTransform
                // or arg1, arg2 will be {str, ColorSpace}
                
                PyObject * arg1 = Py_None;
                PyObject * arg2 = Py_None;
                
                const char * direction = 0;
                PyObject * pycontext = Py_None;
                
                const char * kwlist[] = {"arg1", "arg2", "direction", "context",  NULL};
                
                if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OsO",
                    const_cast<char**>(kwlist),
                    &arg1, &arg2, &direction, &pycontext))
                    return 0;
                
                ConstConfigRcPtr config = GetConstConfig(self, true);
                
                // Parse the direction string
                TransformDirection dir = TRANSFORM_DIR_FORWARD;
                if(direction) dir = TransformDirectionFromString( direction );
                
                // Parse the context
                ConstContextRcPtr context;
                if(pycontext != Py_None) context = GetConstContext(pycontext, true);
                if(!context) context = config->getCurrentContext();
                
                if(IsPyTransform(arg1))
                {
                    ConstTransformRcPtr transform = GetConstTransform(arg1, true);
                    return BuildConstPyProcessor(config->getProcessor(context, transform, dir));
                }
                
                // Any two (Colorspaces, colorspace name, roles)
                ConstColorSpaceRcPtr cs1, cs2;
                
                if(IsPyColorSpace(arg1))
                {
                    cs1 = GetConstColorSpace(arg1, true);
                }
                else if(PyString_Check(arg1))
                {
                    cs1 = config->getColorSpace(PyString_AsString(arg1));
                }
                if(!cs1)
                {
                    PyErr_SetString(PyExc_ValueError,
                        "Could not parse first arg. Allowed types include ColorSpace, ColorSpace name, Role.");
                    return NULL;
                }
                
                if(IsPyColorSpace(arg2))
                {
                    cs2 = GetConstColorSpace(arg2, true);
                }
                else if(PyString_Check(arg2))
                {
                    cs2 = config->getColorSpace(PyString_AsString(arg2));
                }
                if(!cs2)
                {
                    PyErr_SetString(PyExc_ValueError,
                        "Could not parse second arg. Allowed types include ColorSpace, ColorSpace name, Role.");
                    return NULL;
                }
                
                return BuildConstPyProcessor(config->getProcessor(context, cs1, cs2));
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
