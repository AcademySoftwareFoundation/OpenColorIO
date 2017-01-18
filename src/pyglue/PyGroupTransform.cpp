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

#define GetConstGroupTransform(pyobject) GetConstPyOCIO<PyOCIO_Transform, \
    ConstGroupTransformRcPtr, GroupTransform>(pyobject, \
    PyOCIO_GroupTransformType)

#define GetEditableGroupTransform(pyobject) GetEditablePyOCIO<PyOCIO_Transform, \
    GroupTransformRcPtr, GroupTransform>(pyobject, \
    PyOCIO_GroupTransformType)

OCIO_NAMESPACE_ENTER
{
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_GroupTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds);
        PyObject * PyOCIO_GroupTransform_getTransform(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_GroupTransform_getTransforms(PyObject * self);
        PyObject * PyOCIO_GroupTransform_setTransforms(PyObject * self,  PyObject * args);
        // TODO: make these appear more like a pysequence. .append, len(), etc
        PyObject * PyOCIO_GroupTransform_size(PyObject * self);
        PyObject * PyOCIO_GroupTransform_push_back(PyObject * self, PyObject * args);
        PyObject * PyOCIO_GroupTransform_clear(PyObject * self);
        PyObject * PyOCIO_GroupTransform_empty(PyObject * self);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_GroupTransform_methods[] = {
            { "getTransform",
            PyOCIO_GroupTransform_getTransform, METH_VARARGS, GROUPTRANSFORM_GETTRANSFORM__DOC__ },
            { "getTransforms",
            (PyCFunction) PyOCIO_GroupTransform_getTransforms, METH_NOARGS, GROUPTRANSFORM_GETTRANSFORMS__DOC__ },
            { "setTransforms",
            PyOCIO_GroupTransform_setTransforms, METH_VARARGS, GROUPTRANSFORM_SETTRANSFORMS__DOC__ },
            { "size",
            (PyCFunction) PyOCIO_GroupTransform_size, METH_NOARGS, GROUPTRANSFORM_SIZE__DOC__ },
            { "push_back",
            PyOCIO_GroupTransform_push_back, METH_VARARGS, GROUPTRANSFORM_PUSH_BACK__DOC__ },
            { "clear",
            (PyCFunction) PyOCIO_GroupTransform_clear, METH_NOARGS, GROUPTRANSFORM_CLEAR__DOC__ },
            { "empty",
            (PyCFunction) PyOCIO_GroupTransform_empty, METH_NOARGS, GROUPTRANSFORM_EMPTY__DOC__ },
            { NULL, NULL, 0, NULL }
        };
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_GroupTransformType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        OCIO_PYTHON_NAMESPACE(GroupTransform),      //tp_name
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
        GROUPTRANSFORM__DOC__,                      //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_GroupTransform_methods,              //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        &PyOCIO_TransformType,                      //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_GroupTransform_init,      //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_GroupTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds)
        {
            OCIO_PYTRY_ENTER()
            GroupTransformRcPtr ptr = GroupTransform::Create();
            int ret = BuildPyTransformObject<GroupTransformRcPtr>(self, ptr);
            PyObject* pytransforms = Py_None;
            char* direction = NULL;
            static const char *kwlist[] = { "transforms", "direction", NULL };
            if(!PyArg_ParseTupleAndKeywords(args, kwds, "|Os",
                const_cast<char **>(kwlist),
                &pytransforms, &direction )) return -1;
            if(pytransforms != Py_None)
            {
                std::vector<ConstTransformRcPtr> data;
                if(!FillTransformVectorFromPySequence(pytransforms, data))
                {
                    PyErr_SetString(PyExc_TypeError,
                        "Kwarg 'transforms' must be a transform array.");
                    return -1;
                }
                for(unsigned int i=0; i<data.size(); ++i)
                    ptr->push_back(data[i]);
            }
            if(direction)
                ptr->setDirection(TransformDirectionFromString(direction));
            return ret;
            OCIO_PYTRY_EXIT(-1)
        }
        
        PyObject * PyOCIO_GroupTransform_getTransform(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            int index = 0;
            if (!PyArg_ParseTuple(args,"i:getTransform",
                &index)) return NULL;
            ConstGroupTransformRcPtr transform = GetConstGroupTransform(self);
            ConstTransformRcPtr childTransform = transform->getTransform(index);
            return BuildConstPyTransform(childTransform);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_GroupTransform_getTransforms(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstGroupTransformRcPtr transform = GetConstGroupTransform(self);
            std::vector<ConstTransformRcPtr> transforms;
            for(int i = 0; i < transform->size(); ++i)
                transforms.push_back(transform->getTransform(i));
            return CreatePyListFromTransformVector(transforms);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_GroupTransform_setTransforms(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pytransforms = 0;
            if (!PyArg_ParseTuple(args,"O:setTransforms",
                &pytransforms)) return NULL;
            GroupTransformRcPtr transform = GetEditableGroupTransform(self);
            std::vector<ConstTransformRcPtr> data;
            if(!FillTransformVectorFromPySequence(pytransforms, data))
            {
                PyErr_SetString(PyExc_TypeError, "First argument must be a transform array.");
                return 0;
            }
            transform->clear();
            for(unsigned int i = 0; i < data.size(); ++i)
                transform->push_back(data[i]);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_GroupTransform_size(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstGroupTransformRcPtr transform = GetConstGroupTransform(self);
            return PyInt_FromLong(transform->size());
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_GroupTransform_push_back(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pytransform = 0;
            if (!PyArg_ParseTuple(args,"O:push_back",
                &pytransform)) return NULL;
            GroupTransformRcPtr transform = GetEditableGroupTransform(self);
            if(!IsPyTransform(pytransform))
                throw Exception("GroupTransform.push_back requires a transform as the first arg.");
            transform->push_back(GetConstTransform(pytransform, true));
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_GroupTransform_clear(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            GroupTransformRcPtr transform = GetEditableGroupTransform(self);
            transform->clear();
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_GroupTransform_empty(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstGroupTransformRcPtr transform = GetConstGroupTransform(self);
            return PyBool_FromLong(transform->empty());
            OCIO_PYTRY_EXIT(NULL)
        }
        
    }

}
OCIO_NAMESPACE_EXIT
