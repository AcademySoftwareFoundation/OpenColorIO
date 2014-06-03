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
#include <sstream>
#include <OpenColorIO/OpenColorIO.h>

#include "PyUtil.h"
#include "PyDoc.h"

OCIO_NAMESPACE_ENTER
{
    
    PyObject * BuildConstPyConfig(ConstConfigRcPtr config)
    {
        return BuildConstPyOCIO<PyOCIO_Config, ConfigRcPtr,
            ConstConfigRcPtr>(config, PyOCIO_ConfigType);
    }
    
    PyObject * BuildEditablePyConfig(ConfigRcPtr config)
    {
        return BuildEditablePyOCIO<PyOCIO_Config, ConfigRcPtr,
            ConstConfigRcPtr>(config, PyOCIO_ConfigType);
    }
    
    bool IsPyConfig(PyObject * pyobject)
    {
        return IsPyOCIOType(pyobject, PyOCIO_ConfigType);
    }
    
    bool IsPyConfigEditable(PyObject * pyobject)
    {
        return IsPyEditable<PyOCIO_Config>(pyobject, PyOCIO_ConfigType);
    }
    
    ConstConfigRcPtr GetConstConfig(PyObject * pyobject, bool allowCast)
    {
        return GetConstPyOCIO<PyOCIO_Config, ConstConfigRcPtr>(pyobject,
            PyOCIO_ConfigType, allowCast);
    }
    
    ConfigRcPtr GetEditableConfig(PyObject * pyobject)
    {
        return GetEditablePyOCIO<PyOCIO_Config, ConfigRcPtr>(pyobject,
            PyOCIO_ConfigType);
    }
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyObject * PyOCIO_Config_CreateFromEnv(PyObject * cls);
        PyObject * PyOCIO_Config_CreateFromFile(PyObject * cls, PyObject * args);
        PyObject * PyOCIO_Config_CreateFromStream(PyObject * cls, PyObject * args);
        int PyOCIO_Config_init(PyOCIO_Config * self, PyObject * args, PyObject * kwds);
        void PyOCIO_Config_delete(PyOCIO_Config * self, PyObject * args);
        PyObject * PyOCIO_Config_isEditable(PyObject * self);
        PyObject * PyOCIO_Config_createEditableCopy(PyObject * self);
        PyObject * PyOCIO_Config_sanityCheck(PyObject * self);
        PyObject * PyOCIO_Config_getDescription(PyObject * self);
        PyObject * PyOCIO_Config_setDescription(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_serialize(PyObject * self);
        PyObject * PyOCIO_Config_getCacheID(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getCurrentContext(PyObject * self);
        PyObject * PyOCIO_Config_addEnvironmentVar(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getNumEnvironmentVars(PyObject * self);
        PyObject * PyOCIO_Config_getEnvironmentVarNameByIndex(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getEnvironmentVarDefault(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getEnvironmentVarDefaults(PyObject * self);
        PyObject * PyOCIO_Config_clearEnvironmentVars(PyObject * self);
        PyObject * PyOCIO_Config_getSearchPath(PyObject * self);
        PyObject * PyOCIO_Config_setSearchPath(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getWorkingDir(PyObject * self);
        PyObject * PyOCIO_Config_setWorkingDir(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_Config_getNumColorSpaces(PyObject * self);
        PyObject * PyOCIO_Config_getColorSpaceNameByIndex(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_Config_getColorSpaces(PyObject * self); // py interface only
        PyObject * PyOCIO_Config_getColorSpace(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getIndexForColorSpace(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_Config_addColorSpace(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_clearColorSpaces(PyObject * self);
        PyObject * PyOCIO_Config_parseColorSpaceFromString(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_isStrictParsingEnabled(PyObject * self);
        PyObject * PyOCIO_Config_setStrictParsingEnabled(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_setRole(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getNumRoles(PyObject * self);
        PyObject * PyOCIO_Config_hasRole(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getRoleName(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getDefaultDisplay(PyObject * self);
        PyObject * PyOCIO_Config_getDisplay(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getNumDisplaysActive(PyObject * self);
        PyObject * PyOCIO_Config_getDisplayActive(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getDisplaysActive(PyObject * self);
        PyObject * PyOCIO_Config_getNumDisplaysAll(PyObject * self);
        PyObject * PyOCIO_Config_getDisplayAll(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getDisplaysAll(PyObject * self);
        PyObject * PyOCIO_Config_getDefaultView(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getNumViews(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getView(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getViews(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getDisplayColorSpaceName(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getDisplayLooks(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_addDisplay(PyObject * self, PyObject * args, PyObject * kwargs);
        PyObject * PyOCIO_Config_clearDisplays(PyObject * self);
        PyObject * PyOCIO_Config_setActiveDisplays(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getActiveDisplays(PyObject * self);
        PyObject * PyOCIO_Config_setActiveViews(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getActiveViews(PyObject * self);
        PyObject * PyOCIO_Config_getDefaultLumaCoefs(PyObject * self);
        PyObject * PyOCIO_Config_setDefaultLumaCoefs(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getLook( PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_getNumLooks(PyObject * self);
        PyObject * PyOCIO_Config_getLookNameByIndex(PyObject * self, PyObject * args);        
        PyObject * PyOCIO_Config_getLooks(PyObject * self);
        PyObject * PyOCIO_Config_addLook(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Config_clearLooks(PyObject * self);
        PyObject * PyOCIO_Config_getProcessor(PyObject * self, PyObject * args, PyObject * kwargs);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_Config_methods[] = {
            { "CreateFromEnv",
            (PyCFunction) PyOCIO_Config_CreateFromEnv, METH_NOARGS | METH_CLASS, CONFIG_CREATEFROMENV__DOC__ },
            { "CreateFromFile",
            PyOCIO_Config_CreateFromFile, METH_VARARGS | METH_CLASS, CONFIG_CREATEFROMFILE__DOC__ },
            { "CreateFromStream",
            PyOCIO_Config_CreateFromStream, METH_VARARGS | METH_CLASS, CONFIG_CREATEFROMSTREAM__DOC__ },
            { "isEditable",
            (PyCFunction) PyOCIO_Config_isEditable, METH_NOARGS, CONFIG_ISEDITABLE__DOC__ },
            { "createEditableCopy",
            (PyCFunction) PyOCIO_Config_createEditableCopy, METH_NOARGS, CONFIG_CREATEEDITABLECOPY__DOC__ },
            { "sanityCheck",
            (PyCFunction) PyOCIO_Config_sanityCheck, METH_NOARGS, CONFIG_SANITYCHECK__DOC__ },
            { "getDescription",
            (PyCFunction) PyOCIO_Config_getDescription, METH_NOARGS, CONFIG_GETDESCRIPTION__DOC__ },
            { "setDescription",
            PyOCIO_Config_setDescription, METH_VARARGS, CONFIG_SETDESCRIPTION__DOC__ },
            { "serialize",
            (PyCFunction) PyOCIO_Config_serialize, METH_NOARGS, CONFIG_SERIALIZE__DOC__ },
            { "getCacheID",
            PyOCIO_Config_getCacheID, METH_VARARGS, CONFIG_GETCACHEID__DOC__ },
            { "getCurrentContext",
            (PyCFunction) PyOCIO_Config_getCurrentContext, METH_NOARGS, CONFIG_GETCURRENTCONTEXT__DOC__ },
            { "addEnvironmentVar",
            PyOCIO_Config_addEnvironmentVar, METH_VARARGS, CONFIG_ADDENVIRONMENTVAR__DOC__ },
            { "getNumEnvironmentVars",
            (PyCFunction)PyOCIO_Config_getNumEnvironmentVars, METH_NOARGS, CONFIG_GETNUMENVIRONMENTVARS__DOC__ },
            { "getEnvironmentVarNameByIndex",
            PyOCIO_Config_getEnvironmentVarNameByIndex, METH_VARARGS, CONFIG_GETENVIRONMENTVARNAMEBYINDEX__DOC__ },
            { "getEnvironmentVarDefault",
            PyOCIO_Config_getEnvironmentVarDefault, METH_VARARGS, CONFIG_GETENVIRONMENTVARDEFAULT__DOC__ },
            { "getEnvironmentVarDefaults",
            (PyCFunction)PyOCIO_Config_getEnvironmentVarDefaults, METH_NOARGS, CONFIG_GETENVIRONMENTVARDEFAULTS__DOC__ },
            { "clearEnvironmentVars",
            (PyCFunction)PyOCIO_Config_clearEnvironmentVars, METH_NOARGS, CONFIG_CLEARENVIRONMENTVARS__DOC__ },
            { "getSearchPath",
            (PyCFunction) PyOCIO_Config_getSearchPath, METH_NOARGS, CONFIG_GETSEARCHPATH__DOC__ },
            { "setSearchPath",
            PyOCIO_Config_setSearchPath, METH_VARARGS, CONFIG_SETSEARCHPATH__DOC__ },
            { "getWorkingDir",
            (PyCFunction) PyOCIO_Config_getWorkingDir, METH_NOARGS, CONFIG_GETWORKINGDIR__DOC__ },
            { "setWorkingDir",
            PyOCIO_Config_setWorkingDir, METH_VARARGS, CONFIG_SETWORKINGDIR__DOC__ },
            { "getNumColorSpaces",
            (PyCFunction) PyOCIO_Config_getNumColorSpaces, METH_VARARGS, CONFIG_GETNUMCOLORSPACES__DOC__ },
            { "getColorSpaceNameByIndex",
            PyOCIO_Config_getColorSpaceNameByIndex, METH_VARARGS, CONFIG_GETCOLORSPACENAMEBYINDEX__DOC__ },
            { "getColorSpaces",
            (PyCFunction) PyOCIO_Config_getColorSpaces, METH_NOARGS, CONFIG_GETCOLORSPACES__DOC__ },
            { "getColorSpace",
            PyOCIO_Config_getColorSpace, METH_VARARGS, CONFIG_GETCOLORSPACE__DOC__ },
            { "getIndexForColorSpace",
            PyOCIO_Config_getIndexForColorSpace, METH_VARARGS, CONFIG_GETINDEXFORCOLORSPACE__DOC__ },
            { "addColorSpace",
            PyOCIO_Config_addColorSpace, METH_VARARGS, CONFIG_ADDCOLORSPACE__DOC__ },
            { "clearColorSpaces",
            (PyCFunction) PyOCIO_Config_clearColorSpaces, METH_NOARGS, CONFIG_CLEARCOLORSPACES__DOC__ },
            { "parseColorSpaceFromString",
            PyOCIO_Config_parseColorSpaceFromString, METH_VARARGS, CONFIG_PARSECOLORSPACEFROMSTRING__DOC__ },
            { "isStrictParsingEnabled",
            (PyCFunction) PyOCIO_Config_isStrictParsingEnabled, METH_NOARGS, CONFIG_ISSTRICTPARSINGENABLED__DOC__ },
            { "setStrictParsingEnabled",
            PyOCIO_Config_setStrictParsingEnabled, METH_VARARGS, CONFIG_SETSTRICTPARSINGENABLED__DOC__ },
            { "setRole",
            PyOCIO_Config_setRole, METH_VARARGS, CONFIG_SETROLE__DOC__ },
            { "getNumRoles",
            (PyCFunction) PyOCIO_Config_getNumRoles, METH_NOARGS, CONFIG_GETNUMROLES__DOC__ },
            { "hasRole",
            PyOCIO_Config_hasRole, METH_VARARGS, CONFIG_HASROLE__DOC__ },
            { "getRoleName",
            PyOCIO_Config_getRoleName, METH_VARARGS, CONFIG_GETROLENAME__DOC__ },
            { "getDefaultDisplay",
            (PyCFunction) PyOCIO_Config_getDefaultDisplay, METH_NOARGS, CONFIG_GETDEFAULTDISPLAY__DOC__ },
            { "getNumDisplays",
            (PyCFunction) PyOCIO_Config_getNumDisplaysActive, METH_NOARGS, CONFIG_GETNUMDISPLAYS__DOC__ },
            { "getDisplay",
            (PyCFunction) PyOCIO_Config_getDisplay, METH_VARARGS, CONFIG_GETDISPLAY__DOC__ },
            { "getDisplays",
            (PyCFunction) PyOCIO_Config_getDisplaysActive, METH_NOARGS, CONFIG_GETDISPLAYS__DOC__ },
            { "getNumDisplaysActive",
            (PyCFunction) PyOCIO_Config_getNumDisplaysActive, METH_NOARGS, CONFIG_GETNUMDISPLAYSACTIVE__DOC__ },
            { "getDisplayActive",
            (PyCFunction) PyOCIO_Config_getDisplayActive, METH_VARARGS, CONFIG_GETDISPLAYACTIVE__DOC__ },
            { "getDisplaysActive",
            (PyCFunction) PyOCIO_Config_getDisplaysActive, METH_NOARGS, CONFIG_GETDISPLAYSACTIVE__DOC__ },
            { "getNumDisplaysAll",
            (PyCFunction) PyOCIO_Config_getNumDisplaysAll, METH_NOARGS, CONFIG_GETNUMDISPLAYSALL__DOC__ },
            { "getDisplayAll",
            (PyCFunction) PyOCIO_Config_getDisplayAll, METH_VARARGS, CONFIG_GETDISPLAYALL__DOC__ },
            { "getDisplaysAll",
            (PyCFunction) PyOCIO_Config_getDisplaysAll, METH_NOARGS, CONFIG_GETDISPLAYSALL__DOC__ },
            { "getDefaultView",
            PyOCIO_Config_getDefaultView, METH_VARARGS, CONFIG_GETDEFAULTVIEW__DOC__ },
            { "getNumViews",
            PyOCIO_Config_getNumViews, METH_VARARGS, CONFIG_GETNUMVIEWS__DOC__ },
            { "getView",
            PyOCIO_Config_getView, METH_VARARGS, CONFIG_GETVIEW__DOC__ },
            { "getViews",
            PyOCIO_Config_getViews, METH_VARARGS, CONFIG_GETVIEWS__DOC__ },
            { "getDisplayColorSpaceName",
            PyOCIO_Config_getDisplayColorSpaceName, METH_VARARGS, CONFIG_GETDISPLAYCOLORSPACENAME__DOC__ },
            { "getDisplayLooks",
            PyOCIO_Config_getDisplayLooks, METH_VARARGS, CONFIG_GETDISPLAYLOOKS__DOC__ },
            { "addDisplay",
            (PyCFunction) PyOCIO_Config_addDisplay, METH_VARARGS|METH_KEYWORDS, CONFIG_ADDDISPLAY__DOC__ },
            { "clearDisplays",
            (PyCFunction) PyOCIO_Config_clearDisplays, METH_NOARGS, CONFIG_CLEARDISPLAYS__DOC__ },
            { "setActiveDisplays",
            PyOCIO_Config_setActiveDisplays, METH_VARARGS, CONFIG_SETACTIVEDISPLAYS__DOC__ },
            { "getActiveDisplays",
            (PyCFunction) PyOCIO_Config_getActiveDisplays, METH_NOARGS, CONFIG_GETACTIVEDISPLAYS__DOC__ },
            { "setActiveViews",
            PyOCIO_Config_setActiveViews, METH_VARARGS, CONFIG_SETACTIVEVIEWS__DOC__ },
            { "getActiveViews",
            (PyCFunction) PyOCIO_Config_getActiveViews, METH_NOARGS, CONFIG_GETACTIVEVIEWS__DOC__ },
            { "getDefaultLumaCoefs",
            (PyCFunction) PyOCIO_Config_getDefaultLumaCoefs, METH_NOARGS, CONFIG_GETDEFAULTLUMACOEFS__DOC__ },
            { "setDefaultLumaCoefs",
            PyOCIO_Config_setDefaultLumaCoefs, METH_VARARGS, CONFIG_SETDEFAULTLUMACOEFS__DOC__ },
            { "getLook",
            PyOCIO_Config_getLook, METH_VARARGS, CONFIG_GETLOOK__DOC__ },
            { "getNumLooks",
            (PyCFunction) PyOCIO_Config_getNumLooks, METH_NOARGS, CONFIG_GETNUMLOOKS__DOC__ },
            { "getLookNameByIndex",
            PyOCIO_Config_getLookNameByIndex, METH_VARARGS, CONFIG_GETLOOKNAMEBYINDEX__DOC__ },
            { "getLooks",
            (PyCFunction) PyOCIO_Config_getLooks, METH_NOARGS, CONFIG_GETLOOKS__DOC__ },
            { "addLook",
            PyOCIO_Config_addLook, METH_VARARGS, CONFIG_ADDLOOK__DOC__ },
            { "clearLook", // THIS SHOULD BE REMOVED IN THE NEXT BINARY INCOMPATIBLE VERSION
            (PyCFunction) PyOCIO_Config_clearLooks, METH_NOARGS, CONFIG_CLEARLOOKS__DOC__ },
            { "clearLooks",
            (PyCFunction) PyOCIO_Config_clearLooks, METH_NOARGS, CONFIG_CLEARLOOKS__DOC__ },
            { "getProcessor",
            (PyCFunction) PyOCIO_Config_getProcessor, METH_VARARGS|METH_KEYWORDS, CONFIG_GETPROCESSOR__DOC__ },
            { NULL, NULL, 0, NULL }
        };
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ConfigType = {
        PyVarObject_HEAD_INIT(NULL, 0)              //obsize
        "OCIO.Config",                              //tp_name
        sizeof(PyOCIO_Config),                      //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_Config_delete,           //tp_dealloc
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
        CONFIG__DOC__,                              //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_Config_methods,                      //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_Config_init,              //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyObject * PyOCIO_Config_CreateFromEnv(PyObject * /*self*/)
        {
            OCIO_PYTRY_ENTER()
            return BuildConstPyConfig(Config::CreateFromEnv());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_CreateFromFile(PyObject * /*self*/, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* filename = 0;
            if (!PyArg_ParseTuple(args,"s:CreateFromFile", &filename)) return NULL;
            return BuildConstPyConfig(Config::CreateFromFile(filename));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_CreateFromStream(PyObject * /*self*/, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* stream = 0;
            if (!PyArg_ParseTuple(args,"s:CreateFromStream", &stream)) return NULL;
            std::istringstream is;
            is.str(stream);
            return BuildConstPyConfig(Config::CreateFromStream(is));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        int PyOCIO_Config_init(PyOCIO_Config *self, PyObject * /*args*/, PyObject * /*kwds*/)
        {
            OCIO_PYTRY_ENTER()
            return BuildPyObject<PyOCIO_Config, ConstConfigRcPtr, ConfigRcPtr>(self, Config::Create());
            OCIO_PYTRY_EXIT(-1)
        }
        
        void PyOCIO_Config_delete(PyOCIO_Config *self, PyObject * /*args*/)
        {
            DeletePyObject<PyOCIO_Config>(self);
        }
        
        PyObject * PyOCIO_Config_isEditable(PyObject * self)
        {
            return PyBool_FromLong(IsPyConfigEditable(self));
        }
        
        PyObject * PyOCIO_Config_createEditableCopy(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            ConfigRcPtr copy = config->createEditableCopy();
            return BuildEditablePyConfig(copy);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_sanityCheck(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            config->sanityCheck();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getDescription(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getDescription());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_setDescription(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* desc = 0;
            if (!PyArg_ParseTuple(args, "s:setDescription",
                &desc)) return NULL;
            ConfigRcPtr config = GetEditableConfig(self);
            config->setDescription(desc);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_serialize(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            std::ostringstream os;
            config->serialize(os);
            return PyString_FromString(os.str().c_str());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getCacheID(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pycontext = NULL;
            if (!PyArg_ParseTuple(args, "|O:getCacheID",
                &pycontext)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            ConstContextRcPtr context;
            if(pycontext != NULL) context = GetConstContext(pycontext, true);
            else context = config->getCurrentContext();
            return PyString_FromString(config->getCacheID(context));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getCurrentContext(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return BuildConstPyContext(config->getCurrentContext());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_addEnvironmentVar(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            char* value = 0;
            if (!PyArg_ParseTuple(args, "ss:addEnvironmentVar",
                &name, &value)) return NULL;
            ConfigRcPtr config = GetEditableConfig(self);
            config->addEnvironmentVar(name, value);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getNumEnvironmentVars(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyInt_FromLong(config->getNumEnvironmentVars());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getEnvironmentVarNameByIndex(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int index = 0;
            if (!PyArg_ParseTuple(args,"i:getEnvironmentVarNameByIndex",
                &index)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getEnvironmentVarNameByIndex(index));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getEnvironmentVarDefault(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            if (!PyArg_ParseTuple(args, "s:getEnvironmentVarDefault",
                &name)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getEnvironmentVarDefault(name));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getEnvironmentVarDefaults(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            std::map<std::string, std::string> data;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            for(int i = 0; i < config->getNumEnvironmentVars(); ++i) {
                const char* name = config->getEnvironmentVarNameByIndex(i);
                const char* value = config->getEnvironmentVarDefault(name);
                data.insert(std::pair<std::string, std::string>(name, value));
            }
            return CreatePyDictFromStringMap(data);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_clearEnvironmentVars(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConfigRcPtr config = GetEditableConfig(self);
            config->clearEnvironmentVars();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getSearchPath(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getSearchPath());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_setSearchPath(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* path = 0;
            if (!PyArg_ParseTuple(args, "s:setSearchPath",
                &path)) return NULL;
            ConfigRcPtr config = GetEditableConfig(self);
            config->setSearchPath(path);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getWorkingDir(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getWorkingDir());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_setWorkingDir(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* path = 0;
            if (!PyArg_ParseTuple(args, "s:setWorkingDir",
                &path)) return NULL;
            ConfigRcPtr config = GetEditableConfig(self);
            config->setWorkingDir(path);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getNumColorSpaces(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyInt_FromLong(config->getNumColorSpaces());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getColorSpaceNameByIndex(PyObject * self,  PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int index = 0;
            if (!PyArg_ParseTuple(args,"i:getColorSpaceNameByIndex",
                &index)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getColorSpaceNameByIndex(index));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getColorSpaces(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            int numColorSpaces = config->getNumColorSpaces();
            PyObject* tuple = PyTuple_New(numColorSpaces);
            for(int i = 0; i < numColorSpaces; ++i)
            {
                const char* name = config->getColorSpaceNameByIndex(i);
                ConstColorSpaceRcPtr cs = config->getColorSpace(name);
                PyObject* pycs = BuildConstPyColorSpace(cs);
                PyTuple_SetItem(tuple, i, pycs);
            }
            return tuple;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getColorSpace(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            if (!PyArg_ParseTuple(args,"s:getColorSpace",
                &name)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return BuildConstPyColorSpace(config->getColorSpace(name));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getIndexForColorSpace(PyObject * self,  PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            if (!PyArg_ParseTuple(args,"s:getIndexForColorSpace",
                &name)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyInt_FromLong(config->getIndexForColorSpace(name));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_addColorSpace(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            ConfigRcPtr config = GetEditableConfig(self);
            PyObject* pyColorSpace = 0;
            if (!PyArg_ParseTuple(args, "O:addColorSpace",
                &pyColorSpace)) return NULL;
            config->addColorSpace(GetConstColorSpace(pyColorSpace, true));
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_clearColorSpaces(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConfigRcPtr config = GetEditableConfig(self);
            config->clearColorSpaces();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_parseColorSpaceFromString(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            char* str = 0;
            if (!PyArg_ParseTuple(args, "s:parseColorSpaceFromString",
                &str)) return NULL;
            const char* cs = config->parseColorSpaceFromString(str);
            if(cs) return PyString_FromString(cs);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_isStrictParsingEnabled(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyBool_FromLong(config->isStrictParsingEnabled());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_setStrictParsingEnabled(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            bool enabled = false;
            if (!PyArg_ParseTuple(args, "O&:setStrictParsingEnabled",
                ConvertPyObjectToBool, &enabled)) return NULL;
            ConfigRcPtr config = GetEditableConfig(self);
            config->setStrictParsingEnabled(enabled);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_setRole(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            ConfigRcPtr config = GetEditableConfig(self);
            char* role = 0;
            char* csname = 0;
            if (!PyArg_ParseTuple(args, "ss:setRole",
                &role, &csname)) return NULL;
            config->setRole(role, csname);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getNumRoles(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyInt_FromLong(config->getNumRoles());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_hasRole(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* str = 0;
            if (!PyArg_ParseTuple(args, "s:hasRole",
                &str)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyBool_FromLong(config->hasRole(str));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getRoleName(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int index = 0;
            if (!PyArg_ParseTuple(args,"i:getRoleName",
                &index)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getRoleName(index));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getDefaultDisplay(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getDefaultDisplay());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getNumDisplaysActive(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyInt_FromLong(config->getNumDisplaysActive());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getDisplay(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int index = 0;
            if (!PyArg_ParseTuple(args,"i:getDisplay",
                &index)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getDisplayActive(index));
            OCIO_PYTRY_EXIT(NULL)
        }

        PyObject * PyOCIO_Config_getDisplayActive(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int index = 0;
            if (!PyArg_ParseTuple(args,"i:getDisplayActive",
                &index)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getDisplayActive(index));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getDisplaysActive(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            std::vector<std::string> data;
            int numDevices = config->getNumDisplaysActive();
            for(int i = 0; i < numDevices; ++i)
                data.push_back(config->getDisplayActive(i));
            return CreatePyListFromStringVector(data);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getNumDisplaysAll(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyInt_FromLong(config->getNumDisplaysAll());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getDisplayAll(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int index = 0;
            if (!PyArg_ParseTuple(args,"i:getDisplayAll",
                &index)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getDisplayAll(index));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getDisplaysAll(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            std::vector<std::string> data;
            int numDevices = config->getNumDisplaysAll();
            for(int i = 0; i < numDevices; ++i)
                data.push_back(config->getDisplayAll(i));
            return CreatePyListFromStringVector(data);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getDefaultView(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* display = 0;
            if (!PyArg_ParseTuple(args, "s:getDefaultView",
                &display)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getDefaultView(display));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getNumViews(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* display = 0;
            if (!PyArg_ParseTuple(args, "s:getNumViews",
                &display)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyInt_FromLong(config->getNumViews(display));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getView(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* display = 0;
            int index = 0;
            if (!PyArg_ParseTuple(args, "si:getNumViews",
                &display, &index)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getView(display, index));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getViews(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* display = 0;
            if (!PyArg_ParseTuple(args, "s:getViews",
                &display)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            std::vector<std::string> data;
            int num = config->getNumViews(display);
            for(int i=0; i < num; ++i)
                data.push_back(config->getView(display, i));
            return CreatePyListFromStringVector(data);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getDisplayColorSpaceName(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* display = 0;
            char* view = 0;
            if (!PyArg_ParseTuple(args, "ss:getDisplayColorSpaceName",
                &display, &view)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getDisplayColorSpaceName(display, view));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getDisplayLooks(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* display = 0;
            char* view = 0;
            if (!PyArg_ParseTuple(args, "ss:getDisplayLooks",
                &display, &view)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getDisplayLooks(display, view));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_addDisplay(PyObject * self, PyObject * args, PyObject * kwargs)
        {
            OCIO_PYTRY_ENTER()
            ConfigRcPtr config = GetEditableConfig(self);
            char* display = 0;
            char* view = 0;
            char* colorSpaceName = 0;
            char* looks = 0;
            const char * kwlist[] = { "display", "view", "colorSpaceName", "looks",  NULL };
            if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sss|s",
                const_cast<char**>(kwlist),
                &display, &view, &colorSpaceName, &looks))
                return 0;
            std::string lookStr;
            if(looks) lookStr = looks;
            config->addDisplay(display, view, colorSpaceName, lookStr.c_str());
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_clearDisplays(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConfigRcPtr config = GetEditableConfig(self);
            config->clearDisplays();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_setActiveDisplays(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            ConfigRcPtr config = GetEditableConfig(self);
            char* displays = 0;
            if (!PyArg_ParseTuple(args, "s:setActiveDisplays",
                &displays)) return NULL;
            config->setActiveDisplays(displays);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getActiveDisplays(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getActiveDisplays());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_setActiveViews(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            ConfigRcPtr config = GetEditableConfig(self);
            char* views = 0;
            if (!PyArg_ParseTuple(args, "s:setActiveViews",
                &views)) return NULL;
            config->setActiveViews(views);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getActiveViews(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getActiveViews());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_setDefaultLumaCoefs(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            ConfigRcPtr config = GetEditableConfig(self);
            PyObject* pyCoef = 0;
            if (!PyArg_ParseTuple(args, "O:setDefaultLumaCoefs",
                &pyCoef)) return 0;
            std::vector<float> coef;
            if(!FillFloatVectorFromPySequence(pyCoef, coef) || (coef.size() != 3))
            {
                PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 3");
                return 0;
            }
            config->setDefaultLumaCoefs(&coef[0]);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getDefaultLumaCoefs(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            std::vector<float> coef(3);
            config->getDefaultLumaCoefs(&coef[0]);
            return CreatePyListFromFloatVector(coef);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getLook(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            char* str = 0;
            if (!PyArg_ParseTuple(args, "s:getLook",
                &str)) return NULL;
            return BuildConstPyLook(config->getLook(str));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getNumLooks(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyInt_FromLong(config->getNumLooks());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getLookNameByIndex(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int index = 0;
            if (!PyArg_ParseTuple(args,"i:getLookNameByIndex",
                &index)) return NULL;
            ConstConfigRcPtr config = GetConstConfig(self, true);
            return PyString_FromString(config->getLookNameByIndex(index));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getLooks(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstConfigRcPtr config = GetConstConfig(self, true);
            int num = config->getNumLooks();
            PyObject* tuple = PyTuple_New(num);
            for(int i = 0; i < num; ++i)
            {
                const char* name = config->getLookNameByIndex(i);
                ConstLookRcPtr look = config->getLook(name);
                PyObject* pylook = BuildConstPyLook(look);
                PyTuple_SetItem(tuple, i, pylook);
            }
            return tuple;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_addLook(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            ConfigRcPtr config = GetEditableConfig(self);
            PyObject * pyLook = 0;
            if (!PyArg_ParseTuple(args, "O:addLook",
                &pyLook)) return NULL;
            config->addLook(GetConstLook(pyLook, true));
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_clearLooks(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConfigRcPtr config = GetEditableConfig(self);
            config->clearLooks();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Config_getProcessor(PyObject * self, PyObject * args, PyObject * kwargs)
        {
            OCIO_PYTRY_ENTER()
            
            // We want this call to be as flexible as possible.
            // arg1 will either be a PyTransform
            // or arg1, arg2 will be {str, ColorSpace}
            PyObject* arg1 = Py_None;
            PyObject* arg2 = Py_None;
            
            const char* direction = 0;
            PyObject* pycontext = Py_None;
            
            const char* kwlist[] = { "arg1", "arg2", "direction", "context",  NULL };
            
            if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OsO",
                const_cast<char**>(kwlist),
                &arg1, &arg2, &direction, &pycontext)) return 0;
            
            ConstConfigRcPtr config = GetConstConfig(self, true);
            
            // Parse the direction string
            TransformDirection dir = TRANSFORM_DIR_FORWARD;
            if(direction) dir = TransformDirectionFromString(direction);
            
            // Parse the context
            ConstContextRcPtr context;
            if(pycontext != Py_None) context = GetConstContext(pycontext, true);
            if(!context) context = config->getCurrentContext();
                
            if(IsPyTransform(arg1)) {
                ConstTransformRcPtr transform = GetConstTransform(arg1, true);
                return BuildConstPyProcessor(config->getProcessor(context, transform, dir));
            }
            
            // Any two (Colorspaces, colorspace name, roles)
            ConstColorSpaceRcPtr cs1;
            if(IsPyColorSpace(arg1))
                cs1 = GetConstColorSpace(arg1, true);
            else if(PyString_Check(arg1))
                cs1 = config->getColorSpace(PyString_AsString(arg1));
            if(!cs1)
            {
                PyErr_SetString(PyExc_ValueError,
                    "Could not parse first arg. Allowed types include ColorSpace, ColorSpace name, Role.");
                return NULL;
            }
            
            ConstColorSpaceRcPtr cs2;
            if(IsPyColorSpace(arg2))
                cs2 = GetConstColorSpace(arg2, true);
            else if(PyString_Check(arg2))
                cs2 = config->getColorSpace(PyString_AsString(arg2));
            if(!cs2)
            {
                PyErr_SetString(PyExc_ValueError,
                    "Could not parse second arg. Allowed types include ColorSpace, ColorSpace name, Role.");
                return NULL;
            }
            
            return BuildConstPyProcessor(config->getProcessor(context, cs1, cs2));
            OCIO_PYTRY_EXIT(NULL)
        }
        
    }

}
OCIO_NAMESPACE_EXIT
