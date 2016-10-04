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

#define GetConstBaker(pyobject) GetConstPyOCIO<PyOCIO_Baker, \
    ConstBakerRcPtr>(pyobject, PyOCIO_BakerType)

#define GetEditableBaker(pyobject) GetEditablePyOCIO<PyOCIO_Baker, \
    BakerRcPtr>(pyobject, PyOCIO_BakerType)

OCIO_NAMESPACE_ENTER
{
    
    PyObject * BuildEditablePyBaker(BakerRcPtr desc)
    {
        return BuildEditablePyOCIO<PyOCIO_Baker, BakerRcPtr,
            ConstBakerRcPtr>(desc, PyOCIO_BakerType);
    }
    
    bool IsPyBaker(PyObject * pyobject)
    {
        return IsPyOCIOType(pyobject, PyOCIO_BakerType);
    }
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_Baker_init(PyOCIO_Baker * self, PyObject * args, PyObject * kwds);
        void PyOCIO_Baker_delete(PyOCIO_Baker * self, PyObject * args);
        PyObject * PyOCIO_Baker_isEditable(PyObject * self);
        PyObject * PyOCIO_Baker_createEditableCopy(PyObject * self);
        PyObject * PyOCIO_Baker_setConfig(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Baker_getConfig(PyObject * self);
        PyObject * PyOCIO_Baker_setFormat(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Baker_getFormat(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Baker_setType(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Baker_getType(PyObject * self);
        PyObject * PyOCIO_Baker_setMetadata(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Baker_getMetadata(PyObject * self);
        PyObject * PyOCIO_Baker_setInputSpace(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Baker_getInputSpace(PyObject * self);
        PyObject * PyOCIO_Baker_setShaperSpace(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Baker_getShaperSpace(PyObject * self);
        PyObject * PyOCIO_Baker_setLooks(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Baker_getLooks(PyObject * self);
        PyObject * PyOCIO_Baker_setTargetSpace(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Baker_getTargetSpace(PyObject * self);
        PyObject * PyOCIO_Baker_setShaperSize(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Baker_getShaperSize(PyObject * self);
        PyObject * PyOCIO_Baker_setCubeSize(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Baker_getCubeSize(PyObject * self);
        PyObject * PyOCIO_Baker_bake(PyObject * self);
        PyObject * PyOCIO_Baker_getNumFormats(PyObject * self);
        PyObject * PyOCIO_Baker_getFormatNameByIndex(PyObject * self, PyObject * args);
        PyObject * PyOCIO_Baker_getFormatExtensionByIndex(PyObject * self, PyObject * args);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_Baker_methods[] = {
            { "isEditable",
            (PyCFunction) PyOCIO_Baker_isEditable, METH_NOARGS, BAKER_ISEDITABLE__DOC__ },
            { "createEditableCopy",
            (PyCFunction) PyOCIO_Baker_createEditableCopy, METH_NOARGS, BAKER_CREATEEDITABLECOPY__DOC__ },
            { "setConfig",
            PyOCIO_Baker_setConfig, METH_VARARGS, BAKER_SETCONFIG__DOC__ },
            { "getConfig",
            (PyCFunction) PyOCIO_Baker_getConfig, METH_NOARGS, BAKER_GETCONFIG__DOC__ },
            { "setFormat",
            PyOCIO_Baker_setFormat, METH_VARARGS, BAKER_SETFORMAT__DOC__ },
            { "getFormat",
            (PyCFunction) PyOCIO_Baker_getFormat, METH_NOARGS, BAKER_GETFORMAT__DOC__ },
            { "setType",
            PyOCIO_Baker_setType, METH_VARARGS, BAKER_SETTYPE__DOC__ },
            { "getType",
            (PyCFunction) PyOCIO_Baker_getType, METH_NOARGS, BAKER_GETTYPE__DOC__ },
            { "setMetadata",
            PyOCIO_Baker_setMetadata, METH_VARARGS, BAKER_SETMETADATA__DOC__ },
            { "getMetadata",
            (PyCFunction) PyOCIO_Baker_getMetadata, METH_NOARGS, BAKER_GETMETADATA__DOC__ },
            { "setInputSpace",
            PyOCIO_Baker_setInputSpace, METH_VARARGS, BAKER_SETINPUTSPACE__DOC__ },
            { "getInputSpace",
            (PyCFunction) PyOCIO_Baker_getInputSpace, METH_NOARGS, BAKER_GETINPUTSPACE__DOC__ },
            { "setShaperSpace",
            PyOCIO_Baker_setShaperSpace, METH_VARARGS, BAKER_SETSHAPERSPACE__DOC__ },
            { "getShaperSpace",
            (PyCFunction) PyOCIO_Baker_getShaperSpace, METH_NOARGS, BAKER_GETSHAPERSPACE__DOC__ },
            { "setLooks",
            PyOCIO_Baker_setLooks, METH_VARARGS, BAKER_SETLOOKS__DOC__ },
            { "getLooks",
            (PyCFunction) PyOCIO_Baker_getLooks, METH_NOARGS, BAKER_GETLOOKS__DOC__ },
            { "setTargetSpace",
            PyOCIO_Baker_setTargetSpace, METH_VARARGS, BAKER_SETTARGETSPACE__DOC__ },
            { "getTargetSpace",
            (PyCFunction) PyOCIO_Baker_getTargetSpace, METH_NOARGS, BAKER_GETTARGETSPACE__DOC__ },
            { "setShaperSize",
            PyOCIO_Baker_setShaperSize, METH_VARARGS, BAKER_SETSHAPERSIZE__DOC__ },
            { "getShaperSize",
            (PyCFunction) PyOCIO_Baker_getShaperSize, METH_NOARGS, BAKER_GETSHAPERSIZE__DOC__ },
            { "setCubeSize",
            PyOCIO_Baker_setCubeSize, METH_VARARGS, BAKER_SETCUBESIZE__DOC__ },
            { "getCubeSize",
            (PyCFunction) PyOCIO_Baker_getCubeSize, METH_NOARGS, BAKER_GETCUBESIZE__DOC__ },
            { "bake",
            (PyCFunction) PyOCIO_Baker_bake, METH_NOARGS, BAKER_BAKE__DOC__ },
            { "getNumFormats",
            (PyCFunction) PyOCIO_Baker_getNumFormats, METH_NOARGS, BAKER_GETNUMFORMATS__DOC__ },
            { "getFormatNameByIndex",
            PyOCIO_Baker_getFormatNameByIndex, METH_VARARGS, BAKER_GETFORMATNAMEBYINDEX__DOC__ },
            { "getFormatExtensionByIndex",
            PyOCIO_Baker_getFormatExtensionByIndex, METH_VARARGS, BAKER_GETFORMATEXTENSIONBYINDEX__DOC__ },
            { NULL, NULL, 0, NULL }
        };
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_BakerType = {
        PyVarObject_HEAD_INIT(NULL, 0)              //ob_size
        OCIO_PYTHON_NAMESPACE(Baker),               //tp_name
        sizeof(PyOCIO_Baker),                       //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_Baker_delete,            //tp_dealloc
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
        BAKER__DOC__,                               //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_Baker_methods,                       //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_Baker_init,               //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_Baker_init(PyOCIO_Baker* self, PyObject * /*args*/, PyObject * /*kwds*/)
        {
            OCIO_PYTRY_ENTER()
            return BuildPyObject<PyOCIO_Baker, ConstBakerRcPtr, BakerRcPtr>(self, Baker::Create());
            OCIO_PYTRY_EXIT(-1)
        }
        
        void PyOCIO_Baker_delete(PyOCIO_Baker *self, PyObject * /*args*/)
        {
            DeletePyObject<PyOCIO_Baker>(self);
        }
        
        PyObject * PyOCIO_Baker_isEditable(PyObject * self)
        {
            return PyBool_FromLong(IsPyConfigEditable(self));
        }
        
        PyObject * PyOCIO_Baker_createEditableCopy(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstBakerRcPtr baker = GetConstBaker(self);
            BakerRcPtr copy = baker->createEditableCopy();
            return BuildEditablePyBaker(copy);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_setConfig(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject * pyconfig;
            if (!PyArg_ParseTuple(args, "O!:SetCurrentConfig",
                &PyOCIO_ConfigType, &pyconfig)) return NULL;
            BakerRcPtr baker = GetEditableBaker(self);
            ConstConfigRcPtr config = GetConstConfig(pyconfig, true);
            baker->setConfig(config);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_getConfig(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstBakerRcPtr baker = GetConstBaker(self);
            return BuildConstPyConfig(baker->getConfig());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_setFormat(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* str = 0;
            if (!PyArg_ParseTuple(args, "s:setFormat",
                &str)) return NULL;
            BakerRcPtr baker = GetEditableBaker(self);
            baker->setFormat(str);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_getFormat(PyObject * self, PyObject * /*args*/)
        {
            OCIO_PYTRY_ENTER()
            ConstBakerRcPtr baker = GetConstBaker(self);
            return PyString_FromString(baker->getFormat());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_setType(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* str = 0;
            if (!PyArg_ParseTuple(args, "s:setType",
                &str)) return NULL;
            BakerRcPtr baker = GetEditableBaker(self);
            baker->setType(str);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_getType(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstBakerRcPtr baker = GetConstBaker(self);
            return PyString_FromString(baker->getType());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_setMetadata(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* str = 0;
            if (!PyArg_ParseTuple(args, "s:setMetadata",
                &str)) return NULL;
            BakerRcPtr baker = GetEditableBaker(self);
            baker->setMetadata(str);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_getMetadata(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstBakerRcPtr baker = GetConstBaker(self);
            return PyString_FromString(baker->getMetadata());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_setInputSpace(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* str = 0;
            if (!PyArg_ParseTuple(args, "s:setInputSpace",
                &str)) return NULL;
            BakerRcPtr baker = GetEditableBaker(self);
            baker->setInputSpace(str);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_getInputSpace(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstBakerRcPtr baker = GetConstBaker(self);
            return PyString_FromString(baker->getInputSpace());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_setShaperSpace(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* str = 0;
            if (!PyArg_ParseTuple(args, "s:setShaperSpace",
                &str)) return NULL;
            BakerRcPtr baker = GetEditableBaker(self);
            baker->setShaperSpace(str);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_getShaperSpace(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstBakerRcPtr baker = GetConstBaker(self);
            return PyString_FromString(baker->getShaperSpace());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_setLooks(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* str = 0;
            if (!PyArg_ParseTuple(args, "s:setLooks",
                &str)) return NULL;
            BakerRcPtr baker = GetEditableBaker(self);
            baker->setLooks(str);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_getLooks(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstBakerRcPtr baker = GetConstBaker(self);
            return PyString_FromString(baker->getLooks());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_setTargetSpace(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* str = 0;
            if (!PyArg_ParseTuple(args, "s:setTargetSpace",
                &str)) return NULL;
            BakerRcPtr baker = GetEditableBaker(self);
            baker->setTargetSpace(str);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_getTargetSpace(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstBakerRcPtr baker = GetConstBaker(self);
            return PyString_FromString(baker->getTargetSpace());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_setShaperSize(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int size = 0;
            if (!PyArg_ParseTuple(args,"i:setShaperSize",
                &size)) return NULL;
            BakerRcPtr baker = GetEditableBaker(self);
            baker->setShaperSize(size);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_getShaperSize(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstBakerRcPtr baker = GetConstBaker(self);
            return PyInt_FromLong(baker->getShaperSize());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_setCubeSize(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int size = 0;
            if (!PyArg_ParseTuple(args,"i:setCubeSize",
                &size)) return NULL;
            BakerRcPtr baker = GetEditableBaker(self);
            baker->setCubeSize(size);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_getCubeSize(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstBakerRcPtr baker = GetConstBaker(self);
            return PyInt_FromLong(baker->getCubeSize());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_bake(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstBakerRcPtr baker = GetConstBaker(self);
            std::ostringstream os;
            baker->bake(os);
            return PyString_FromString(os.str().c_str());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_getNumFormats(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstBakerRcPtr baker = GetConstBaker(self);
            return PyInt_FromLong(baker->getNumFormats());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_getFormatNameByIndex(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int index = 0;
            if (!PyArg_ParseTuple(args,"i:getFormatNameByIndex",
                &index)) return NULL;
            ConstBakerRcPtr baker = GetConstBaker(self);
            return PyString_FromString(baker->getFormatNameByIndex(index));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Baker_getFormatExtensionByIndex(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int index = 0;
            if (!PyArg_ParseTuple(args,"i:getFormatExtensionByIndex",
                &index)) return NULL;
            ConstBakerRcPtr baker = GetConstBaker(self);
            return PyString_FromString(baker->getFormatExtensionByIndex(index));
            OCIO_PYTRY_EXIT(NULL)
        }
        
    } // anon namespace
    
}
OCIO_NAMESPACE_EXIT
