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


/*+doc
Python: Config
========

Usage
^^^^^
.. code-block:: python

    import PyOpenColorIO as OCIO
    
    # Load an existing configuration from the environment
    # The resulting configuration is read-only
    config = OCIO.GetCurrentConfig()
    
    # What color spaces exist?
    colorSpaceNames = [ cs.getName() for cs in config.getColorSpaces() ]
    
    # Given a string, can we parse a color space name from it?
    inputstring = 'myname_linear.exr'
    colorSpaceName = config.parseColorSpaceFromString(inputString)
    if colorSpaceName:
        print 'Found color space',colorSpaceName
    else:
        print 'Could not get colorspace from string',inputString
    
    # What is the name of scene-linear, in the existing configuration?
    colorSpace = config.getColorSpace(OCIO.Constants.ROLE_SCENE_LINEAR)
    if colorSpace:
        print colorSpace.getName()
    else:
        print 'The role of Scene Linear is not defined in the existing configuration'
    
    # For examples of how to actually perform the color transform math,
    # see PyProcessor docs.
    
    # Create a new, empty, editable configuration
    config = OCIO.Config()
    
    # Create a new colorspace, and add it
    cs = OCIO.ColorSpace(...)
    # (See ColorSpace for details)
    config.addColorSpace(cs)
    
    # For additional examples of config manipulation, see
    # https://github.com/imageworks/OpenColorIO-Configs/blob/master/nuke-default/make.py

Description
^^^^^
A color configuration (:py:class:`Config`) defines all the color spaces to be available at runtime.

(:py:class:`Config`)  is the main object for interacting with this library. It encapsulates all
of the information necessary to use customized
:py:class:`ColorSpaceTransform` and :py:class:`DisplayTransform` operations.

See the :ref:`user-guide` for more information on selecting, creating, and working with custom color configurations.

For applications interested in using only one color config at
a time (this is the vast majority of apps), their API would
traditionally get the global configuration and use that, as
opposed to creating a new one. This simplifies the use case
for plugins and bindings, as it alleviates the need to pass
around configuration handles.

An example of an application where this would not be
sufficient would be a multi-threaded image proxy server
(daemon), which wished to handle multiple show configurations
in a single process concurrently. This app would need to keep
multiple configurations alive, and to manage them appropriately.

Roughly speaking, a novice user should select a default
configuration that most closely approximates the use case
(animation, visual effects, etc.), and set the :envvar:`OCIO`
environment variable to point at the root of that configuration.

.. note::
Initialization using environment variables is typically
preferable in a multi-app ecosystem, as it allows all
applications to be consistently configured.

See :ref:`developers-usageexamples`
*/


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
        PyObject * PyOCIO_Config_addDisplay( PyObject * self, PyObject * args, PyObject * kwargs );
        PyObject * PyOCIO_Config_clearDisplays( PyObject * self );
        PyObject * PyOCIO_Config_setActiveDisplays( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Config_getActiveDisplays( PyObject * self );
        PyObject * PyOCIO_Config_setActiveViews( PyObject * self, PyObject * args );
        PyObject * PyOCIO_Config_getActiveViews( PyObject * self );
        
        PyObject * PyOCIO_Config_getDefaultLumaCoefs( PyObject * self );
        PyObject * PyOCIO_Config_setDefaultLumaCoefs( PyObject * self, PyObject * args );
        
        PyObject * PyOCIO_Config_getLook( PyObject * self, PyObject * args );
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
            {"addDisplay", (PyCFunction) PyOCIO_Config_addDisplay, METH_KEYWORDS, "" },
            {"clearDisplays", (PyCFunction) PyOCIO_Config_clearDisplays, METH_NOARGS, "" },
            {"setActiveDisplays", PyOCIO_Config_setActiveDisplays, METH_VARARGS, "" },
            {"getActiveDisplays", (PyCFunction) PyOCIO_Config_getActiveDisplays, METH_NOARGS, "" },
            {"setActiveViews", PyOCIO_Config_setActiveViews, METH_VARARGS, "" },
            {"getActiveViews", (PyCFunction) PyOCIO_Config_getActiveViews, METH_NOARGS, "" },
            
            {"getDefaultLumaCoefs", (PyCFunction) PyOCIO_Config_getDefaultLumaCoefs, METH_NOARGS, "" },
            {"setDefaultLumaCoefs", PyOCIO_Config_setDefaultLumaCoefs, METH_VARARGS, "" },
            
            {"getLook", PyOCIO_Config_getLook, METH_VARARGS, "" },
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
    /// static methods
    
    namespace
    {
        /*+doc
        Functions
        ^^^^^
        .. py:function:: Config.CreateFromEnv()
                     
           Create a :py:class:`Config` object using the environment variable.
                     
           :returns: Config object
        */
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
        
        /*+doc
        .. py:function:: Config.CreateFromFile(filename)
                     
           Create a :py:class:`Config` object using the information in a file.
           
           :param filename: name of file
           :type filename: string
           :return: Config object
        */        
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
        // Insert class markup here
        //+doc WHAT CLASS INFO GOES HERE?
        int PyOCIO_Config_init( PyOCIO_Config *self, PyObject * /*args*/, PyObject * /*kwds*/ )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            /*+doc
            Class
            ^^^^^
            */
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
        // method markup
        /*+doc
        Initialization Methods
        ^^^^^
        
        .. py:method:: Config.isEditable()
                     
           Returns whether Config is editable.
           
           The configurations returned from
           :py:method:`getCurrentConfig()` are
           not editable, and if you want to edit them you can
           use :py:method:`createEditableCopy()`.
           
           If you attempt to call any of the set functions on
           a noneditable Config, an exception will be thrown.
           
           :return: state of :py:class:`Config`'s editability
           :rtype: bool
        */           
        PyObject * PyOCIO_Config_isEditable( PyObject * self )
        {
            return PyBool_FromLong(IsPyConfigEditable(self));
        }
        
        /*+doc
        .. py:method:: Config.createEditableCopy()
                     
           Returns an editable copy of :py:class:`Config`.
           
           :return: editable copy of :py:class:`Config`
           :rtype: Config object
        */
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
        
        /*+doc
        .. py:method:: Config.sanityCheck()
                     
           This will throw an exception if :py:class:`Config` is
           malformed. The most common error occurs when
           references are made to colorspaces that do not exist.
        */        
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
        
        /*+doc
        Resource Methods
        ^^^^^
        
        .. py:method:: Config.getDescription()
                     
           Returns the stored description of
           :py:class:`Config`.
           
           :return: stored description of :py:class:`Config`
           :rtype: string
        */
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
        
        /*+doc
        .. py:method:: Config.setDescription(desc)
                     
           Sets the description of :py:class:`Config`.
           
           :param desc: description of :py:class:`Config`
           :type desc: string
        */        
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
        
        /*+doc
        .. py:method:: Config.serialize()
                     
           Returns the string representation of
           :py:class:`Config`
           in YAML text form. This is typically
           stored on disk in a file with the .ocio extension.
           
           :return: :py:class:`Config` in YAML text form
           :rtype: string
        */        
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
        
        /*+doc
        .. py:method:: Config.getCacheID([, pycontext])
                     
           This will produce a hash of the all colorspace definitions, etc.
           
           All external references, such as files used in FileTransforms, etc.,
           will be incorporated into the cacheID. While the contents of
           the files are not read, the file system is queried for relavent
           information (mtime, inode) so that the ::py:class:`Config`'s cacheID will
           change when the underlying luts are updated.
           
           If a context is not provided, the current Context will be used.
           If a null context is provided, file references will not be taken into
           account (this is essentially a hash of :py:method:`Config.serialize()`).
           
           :param pycontext: optional
           :type pycontext: object
           :return: hash of :py:class:`Config`
           :rtype: string
        */
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
        /*+doc
        .. py:method:: Config.getSearchPath()
                     
           Returns the search path???
           
           :return: search path
           :rtype: string
        */
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
        
        /*+doc
        .. py:method:: Config.setSearchPath(path)
                     
           Sets the search path???
           
           :param path: the search path
           :type path: string
        */
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
        
        /*+doc
        .. py:method:: Config.getWorkingDir()
                     
           Returns the working directory.
           
           :return: the working directory
           :rtype path: string
        */
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
        
        /*+doc
        .. py:method:: Config.setWorkingDir()
                     
           Sets the working directory.
           
           :param path: the working directory
           :type path: string
        */
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
        /*+doc
        ColorSpace Methods
        ^^^^^
        
        .. py:method:: Config.getColorSpaces()
                     
           Returns all the ColorSpaces defined in
           :py:class:`Config`.
           
           :return: ColorSpaces in :py:class:`Config`
           :rtype: tuple
        */
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
        
        /*+doc
        .. py:method:: Config.getColorSpace(name)
                     
           Returns the data for the specified color space in
           :py:class:`Config`.
           
           This will return null if the specified name is not found.
           
           :param name: name of color space
           :type name: string
           :return: data for specified color space
           :rtype: pyColorSpace object
        */
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
        
        /*+doc
        .. py:method:: Config.addColorSpace(pyColorSpace)
                     
           Add a specified color space to :py:class:`Config`, and stores it.
           
           :param pyColorSpace: color space
           :type pyColorSpace: object

        .. note::
           If another color space is already registered with the same name, this will overwrite it.
        */
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
        
        /*+doc
        .. py:method:: Config.clearColorSpaces()
                     
           Clear the color spaces in :py:class:`Config`. ?????
        */
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
        
        /*+doc
        .. py:method:: Config.parseColorSpaceFromString(str)
                     
           Parses the color space data from a string.
           
           Given the specified string, gets the longest, right-most color space substring.
           * If strict parsing is enabled, and no color space is found, return an empty string.
           * If strict parsing is disabled, return the default role, if defined.
           * If the default role is not defined, return an empty string.

           :param str: color space data
           :type str: string
           :return: parsed data
           :rtype: string
        */
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
        
        /*+doc
           Methods for roles
           ^^^^^
           
           A role acts as an alias for a color space. You can query the color space corresponding to a role using
           :py:function:`getColorSpace()`.
        
        .. py:method:: Config.setRole(role, csname)
                     
           Set the role of a color space???
           
           Setting the colorSpaceName name to a null string unsets it.

           :param role: 
           :type role: 
           :param csname: 
           :type csname: 
        */
        PyObject * PyOCIO_Config_setRole( PyObject * self, PyObject * args )
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                
                char * role = 0;
                char * csname = 0;
                
                if (!PyArg_ParseTuple(args,"ss:setRole",
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
        
        
        /*+doc
        Display/View Registration Methods
        ^^^^^
        
        Looks is a comma- or colon-delimited list of lookNames, where optional '+' and '-' prefixes
        denote forward and inverse look specifications, respectively. A forward look is assumed in
        the absence of a prefix.
        
        .. py:method:: Config.getDefaultDisplay()
                     
           Returns the default display???

           :return: default display
           :rtype: string 
        */
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
        
        /*+doc
        .. py:method:: Config.getDisplays()
                     
           Returns all the displays listed in
           :py:class:`Config`.

           :return: displays in :py:class:`Config`
           :rtype: list of strings
        */
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
        
        /*+doc
        .. py:method:: Config.getDefaultView(display)
                     
           Returns the default view of :py:class:`Config`. ???

           :param display: default view
           :type display: string
           :return: view
           :rtype: string
        */
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
        
        /*+doc
        .. py:method:: Config.getViews(display)
                     
           Returns all the views listed in :py:class:`Config`.

           :param display: views in :py:class:`Config`
           :type display: string
           :return: views
           :rtype: list of strings
        */
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
        
        /*+doc
        .. py:method:: Config.getDisplayColorSpaceName(display, view)
                     
           Returns the color space name corresponding to the display and
           view combination in :py:class:`Config`. ???

           :param display: display
           :type display: string
           :param view: view
           :type view: string
           :return: display color space name
           :rtype: string
        */
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
        
        /*+doc
        .. py:method:: Config.getDisplayLooks(display, view)
                     
           Returns the looks corresponding to the display and
           view combination in :py:class:`Config`. ???

           :param display: display
           :type display: string
           :param view: view
           :type view: string
           :return: looks
           :rtype: string
        */  
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
        
        /*+doc
        .. py:method:: Config.addDisplay(display, view, colorSpaceName[, looks])
                     
           NEEDS WORK

           :param display:
           :type display: string
           :param view: 
           :type view: string
           :param colorSpaceName: 
           :type colorSpaceName: string
           :param looks: optional
           :type looks: string
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
        
        /*+doc
        .. py:method:: Config.clearDisplays()
                     
           ???
        */  
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
        
        /*+doc
        .. py:method:: Config.setActiveDisplays()
                     
           Sets the active displays in :py:class:`Config`.
           
           :param displays: active displays
           :type displays: string
        */  
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
         
        /*+doc
        .. py:method:: Config.getActiveDisplays()
                     
           Returns the active displays in :py:class:`Config`.
           
           :return: active displays
           :rtype: string
        */  
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
        
        /*+doc
        .. py:method:: Config.setActiveViews()
                     
           Sets the active views in :py:class:`Config`.
           
           :param displays: active views
           :type displays: string
        */  
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
        
        /*+doc
        .. py:method:: Config.getActiveViews()
                     
           Returns the active views in :py:class:`Config`.
           
           :return: active views
           :rtype: string
        */  
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
        
        
        
        /*+doc
        Luma Methods
        ^^^^^
        
        Manage the default coefficients for computing luma.
        
        .. note::
           There is no one-size-fits-all set of luma coefficients. The values are typically
           different for each color space, and the application of them may be nonsensical 
           depending on the intensity coding. Thus, the right answer is to make these functions
           on the :py:class:`Config` class. However, it's often useful to have a config-wide
           default so here it is. We will add the color space specific luma call when another
           client is interesting in using it.
        
        .. py:method:: Config.setDefaultLumaCoefs()
                     
           Sets the default luma coefficients in :py:class:`Config`.
           
           :param pyCoef: luma coefficients
           :type pyCoef: object
        */  
        PyObject * PyOCIO_Config_setDefaultLumaCoefs( PyObject * self, PyObject * args )
        {
            try
            {
                ConfigRcPtr config = GetEditableConfig(self);
                
                PyObject* pyCoef = 0;
                if (!PyArg_ParseTuple(args, "O:setDefaultLumaCoefs", &pyCoef))
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
        
        /*+doc
        .. py:method:: Config.getDefaultLumaCoefs()
                     
           Returns the default luma coefficients in :py:class:`Config`.
           
           :return: luma coefficients
           :rtype: list of floats
        */  
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
        
        /*+doc
        Look Methods
        ^^^^^
        
        Manage per-shot look settings.
        
        .. py:method:: Config.getLook()
                     
           Returns the information of a specified look in :py:class:`Config`.
           
           :param str: look
           :type str: string
           :return: specified look
           :rtype: look object
        */  
         PyObject * PyOCIO_Config_getLook( PyObject * self, PyObject * args )
        {
            try
            {
                ConstConfigRcPtr config = GetConstConfig(self, true);
                
                char * str = 0;
                if (!PyArg_ParseTuple(args,"s:getLook",
                    &str)) return NULL;
                
                return BuildConstPyLook(config->getLook(str));
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        /*+doc
        .. py:method:: Config.getLooks()
                     
           Returns a list of all the looks defined in :py:class:`Config`.
           
           :return: looks
           :rtype: tuple of look objects
        */  
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
        
        /*+doc
        .. py:method:: Config.addLook()
                     
           Adds a look to :py:class:`Config`.
           
           :param pylook: look
           :type pylook: look object
        */  
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
        
        /*+doc
        .. py:method:: Config.clearLook()
                     
           ??? :py:class:`Config`.
        */  
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
        
        /*+doc
        Processors Methods
        ^^^^^
        
        Convert from inputColorSpace to outputColorSpace.
        
        .. note::
           This may provide higher fidelity than anticipated due to internal optimizations. 
           For example, if inputColorSpace and outputColorSpace are members of the same family,
           no conversion will be applied, even though, strictly speaking, quantization should be added.
           
        If you wish to test these calls for quantization characteristics, apply in two steps; 
        the image must contain RGB triples (though arbitrary numbers of additional channels 
        can be optionally supported using the pixelStrideBytes arg). HUH???
        
        .. py:method:: Config.getProcessor(arg1[, arg2[, direction[, context]])
           
           Get processor for the specified transform.
           
           Not often needed, but will allow for the reuse of atomic OCIO functionality (such as to apply an individual LUT file).
           
           This will fail if either the source or destination color space is null.
           
           ??? :py:class:`Config`.
           
           :param arg1: 
           :type arg1: object
           :param arg2: optional
           :type arg2: object
           :param direction: optional
           :type direction: string
           :param context: optional
           :type context: object
         
        */  
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
