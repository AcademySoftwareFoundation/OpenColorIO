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

#define GetConstDisplayTransform(pyobject) GetConstPyOCIO<PyOCIO_Transform, \
    ConstDisplayTransformRcPtr, DisplayTransform>(pyobject, \
    PyOCIO_DisplayTransformType)

#define GetEditableDisplayTransform(pyobject) GetEditablePyOCIO<PyOCIO_Transform, \
    DisplayTransformRcPtr, DisplayTransform>(pyobject, \
    PyOCIO_DisplayTransformType)

OCIO_NAMESPACE_ENTER
{
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_DisplayTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds);
        PyObject * PyOCIO_DisplayTransform_getInputColorSpaceName(PyObject * self);
        PyObject * PyOCIO_DisplayTransform_setInputColorSpaceName(PyObject * self, PyObject * args);
        PyObject * PyOCIO_DisplayTransform_getLinearCC(PyObject * self);
        PyObject * PyOCIO_DisplayTransform_setLinearCC(PyObject * self, PyObject * args);
        PyObject * PyOCIO_DisplayTransform_getColorTimingCC(PyObject * self);
        PyObject * PyOCIO_DisplayTransform_setColorTimingCC(PyObject * self, PyObject * args);
        PyObject * PyOCIO_DisplayTransform_getChannelView(PyObject * self);
        PyObject * PyOCIO_DisplayTransform_setChannelView(PyObject * self, PyObject * args);
        PyObject * PyOCIO_DisplayTransform_getDisplay(PyObject * self);
        PyObject * PyOCIO_DisplayTransform_setDisplay(PyObject * self, PyObject * args);
        PyObject * PyOCIO_DisplayTransform_getView(PyObject * self);
        PyObject * PyOCIO_DisplayTransform_setView(PyObject * self, PyObject * args);
        PyObject * PyOCIO_DisplayTransform_getDisplayCC(PyObject * self);
        PyObject * PyOCIO_DisplayTransform_setDisplayCC(PyObject * self, PyObject * args );
        PyObject * PyOCIO_DisplayTransform_getLooksOverride(PyObject * self );
        PyObject * PyOCIO_DisplayTransform_setLooksOverride(PyObject * self, PyObject * args);
        PyObject * PyOCIO_DisplayTransform_getLooksOverrideEnabled(PyObject * self );
        PyObject * PyOCIO_DisplayTransform_setLooksOverrideEnabled(PyObject * self,  PyObject * args);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_DisplayTransform_methods[] = {
            { "getInputColorSpaceName",
            (PyCFunction) PyOCIO_DisplayTransform_getInputColorSpaceName, METH_NOARGS, DISPLAYTRANSFORM_GETINPUTCOLORSPACENAME__DOC__ },
            { "setInputColorSpaceName",
            PyOCIO_DisplayTransform_setInputColorSpaceName, METH_VARARGS, DISPLAYTRANSFORM_SETINPUTCOLORSPACENAME__DOC__ },
            { "getLinearCC",
            (PyCFunction) PyOCIO_DisplayTransform_getLinearCC, METH_NOARGS, DISPLAYTRANSFORM_GETLINEARCC__DOC__ },
            { "setLinearCC",
            PyOCIO_DisplayTransform_setLinearCC, METH_VARARGS, DISPLAYTRANSFORM_SETLINEARCC__DOC__ },
            { "getColorTimingCC",
            (PyCFunction) PyOCIO_DisplayTransform_getColorTimingCC, METH_NOARGS, DISPLAYTRANSFORM_GETCOLORTIMINGCC__DOC__ },
            { "setColorTimingCC",
            PyOCIO_DisplayTransform_setColorTimingCC, METH_VARARGS, DISPLAYTRANSFORM_SETCOLORTIMINGCC__DOC__ },
            { "getChannelView",
            (PyCFunction) PyOCIO_DisplayTransform_getChannelView, METH_NOARGS, DISPLAYTRANSFORM_GETCHANNELVIEW__DOC__ },
            { "setChannelView",
            PyOCIO_DisplayTransform_setChannelView, METH_VARARGS, DISPLAYTRANSFORM_SETCHANNELVIEW__DOC__ },
            { "getDisplay",
            (PyCFunction) PyOCIO_DisplayTransform_getDisplay, METH_NOARGS, DISPLAYTRANSFORM_GETDISPLAY__DOC__ },
            { "setDisplay",
            PyOCIO_DisplayTransform_setDisplay, METH_VARARGS, DISPLAYTRANSFORM_SETDISPLAY__DOC__ },
            { "getView",
            (PyCFunction) PyOCIO_DisplayTransform_getView, METH_NOARGS, DISPLAYTRANSFORM_GETVIEW__DOC__ },
            { "setView",
            PyOCIO_DisplayTransform_setView, METH_VARARGS, DISPLAYTRANSFORM_SETVIEW__DOC__ },
            { "getDisplayCC",
            (PyCFunction) PyOCIO_DisplayTransform_getDisplayCC, METH_NOARGS, DISPLAYTRANSFORM_GETDISPLAYCC__DOC__ },
            { "setDisplayCC",
            PyOCIO_DisplayTransform_setDisplayCC, METH_VARARGS, DISPLAYTRANSFORM_SETDISPLAYCC__DOC__ },
            { "getLooksOverride",
            (PyCFunction) PyOCIO_DisplayTransform_getLooksOverride, METH_NOARGS, DISPLAYTRANSFORM_GETLOOKSOVERRIDE__DOC__ },
            { "setLooksOverride",
            PyOCIO_DisplayTransform_setLooksOverride, METH_VARARGS, DISPLAYTRANSFORM_SETLOOKSOVERRIDE__DOC__ },
            { "getLooksOverrideEnabled",
            (PyCFunction) PyOCIO_DisplayTransform_getLooksOverrideEnabled, METH_NOARGS, DISPLAYTRANSFORM_GETLOOKSOVERRIDEENABLED__DOC__ },
            { "setLooksOverrideEnabled",
            PyOCIO_DisplayTransform_setLooksOverrideEnabled, METH_VARARGS, DISPLAYTRANSFORM_SETLOOKSOVERRIDEENABLED__DOC__ },
            { NULL, NULL, 0, NULL }
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_DisplayTransformType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        OCIO_PYTHON_NAMESPACE(DisplayTransform),    //tp_name
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
        DISPLAYTRANSFORM__DOC__,                    //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_DisplayTransform_methods,            //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        &PyOCIO_TransformType,                      //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_DisplayTransform_init,    //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_DisplayTransform_init(PyOCIO_Transform * self, PyObject * /*args*/, PyObject * /*kwds*/)
        {
            OCIO_PYTRY_ENTER()
            return BuildPyTransformObject<DisplayTransformRcPtr>(self, DisplayTransform::Create());
            OCIO_PYTRY_EXIT(-1)
        }
        
        PyObject * PyOCIO_DisplayTransform_getInputColorSpaceName(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstDisplayTransformRcPtr transform = GetConstDisplayTransform(self);
            return PyString_FromString(transform->getInputColorSpaceName());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_setInputColorSpaceName(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* name = 0;
            if (!PyArg_ParseTuple(args, "s:setInputColorSpaceName",
                &name)) return NULL;
            DisplayTransformRcPtr transform = GetEditableDisplayTransform(self);
            transform->setInputColorSpaceName(name);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_getLinearCC(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstDisplayTransformRcPtr transform = GetConstDisplayTransform(self);
            return BuildConstPyTransform(transform->getLinearCC());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_setLinearCC(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject * pyCC = 0;
            if (!PyArg_ParseTuple(args, "O:setLinearCC",
                &pyCC)) return NULL;
            DisplayTransformRcPtr transform = GetEditableDisplayTransform(self);
            ConstTransformRcPtr cc = GetConstTransform(pyCC, true);
            transform->setLinearCC(cc);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_getColorTimingCC(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstDisplayTransformRcPtr transform = GetConstDisplayTransform(self);
            return BuildConstPyTransform(transform->getColorTimingCC());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_setColorTimingCC(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pyCC = 0;
            if (!PyArg_ParseTuple(args, "O:setColorTimingCC",
                &pyCC)) return NULL;
            DisplayTransformRcPtr transform = GetEditableDisplayTransform(self);
            ConstTransformRcPtr cc = GetConstTransform(pyCC, true);
            transform->setColorTimingCC(cc);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_getChannelView(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstDisplayTransformRcPtr transform = GetConstDisplayTransform(self);
            return BuildConstPyTransform(transform->getChannelView());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_setChannelView(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pyCC = 0;
            if (!PyArg_ParseTuple(args, "O:setChannelView",
                &pyCC)) return NULL;
            DisplayTransformRcPtr transform = GetEditableDisplayTransform(self);
            ConstTransformRcPtr t = GetConstTransform(pyCC, true);
            transform->setChannelView(t);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_getDisplay(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstDisplayTransformRcPtr transform = GetConstDisplayTransform(self);
            return PyString_FromString(transform->getDisplay());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_setDisplay(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* str = 0;
            if (!PyArg_ParseTuple(args, "s:setDisplay",
                &str)) return NULL;
            DisplayTransformRcPtr transform = GetEditableDisplayTransform(self);
            transform->setDisplay(str);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_getView(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstDisplayTransformRcPtr transform = GetConstDisplayTransform(self);
            return PyString_FromString(transform->getView());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_setView(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* str = 0;
            if (!PyArg_ParseTuple(args, "s:setView",
                &str)) return NULL;
            DisplayTransformRcPtr transform = GetEditableDisplayTransform(self);
            transform->setView(str);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_getDisplayCC(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstDisplayTransformRcPtr transform = GetConstDisplayTransform(self);
            return BuildConstPyTransform(transform->getDisplayCC());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_setDisplayCC(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pyCC = 0;
            if (!PyArg_ParseTuple(args,"O:setDisplayCC",
                &pyCC)) return NULL;
            DisplayTransformRcPtr transform = GetEditableDisplayTransform(self);
            ConstTransformRcPtr cc = GetConstTransform(pyCC, true);
            transform->setDisplayCC(cc);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_getLooksOverride(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstDisplayTransformRcPtr transform = GetConstDisplayTransform(self);
            return PyString_FromString(transform->getLooksOverride());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_setLooksOverride(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* str = 0;
            if (!PyArg_ParseTuple(args,"s:setLooksOverride",
                &str)) return NULL;
            DisplayTransformRcPtr transform = GetEditableDisplayTransform(self);
            transform->setLooksOverride(str);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_getLooksOverrideEnabled(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstDisplayTransformRcPtr transform = GetConstDisplayTransform(self);
            return PyBool_FromLong(transform->getLooksOverrideEnabled());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_DisplayTransform_setLooksOverrideEnabled(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            bool enabled = false;
            if (!PyArg_ParseTuple(args, "O&:setLooksOverrideEnabled",
                ConvertPyObjectToBool, &enabled)) return NULL;
            DisplayTransformRcPtr transform = GetEditableDisplayTransform(self);
            transform->setLooksOverrideEnabled(enabled);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
    }

}
OCIO_NAMESPACE_EXIT
