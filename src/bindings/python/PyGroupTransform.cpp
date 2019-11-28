// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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

namespace OCIO_NAMESPACE
{

namespace
{

///////////////////////////////////////////////////////////////////////
///

int PyOCIO_GroupTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds);
PyObject * PyOCIO_GroupTransform_getTransform(PyObject * self,  PyObject * args);
PyObject * PyOCIO_GroupTransform_getNumTransforms(PyObject * self, PyObject *);
PyObject * PyOCIO_GroupTransform_appendTransform(PyObject * self, PyObject * args);

///////////////////////////////////////////////////////////////////////
///

PyMethodDef PyOCIO_GroupTransform_methods[] = {
    { "getTransform",
    PyOCIO_GroupTransform_getTransform, METH_VARARGS, GROUPTRANSFORM_GETTRANSFORM__DOC__ },
    { "getNumTransforms",
    (PyCFunction) PyOCIO_GroupTransform_getNumTransforms, METH_NOARGS, GROUPTRANSFORM_GETNUMTRANSFORMS__DOC__ },
    { "appendTransform",
    PyOCIO_GroupTransform_appendTransform, METH_VARARGS, GROUPTRANSFORM_APPENDTRANSFORM__DOC__ },
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
        std::vector<TransformRcPtr> data;
        if(!FillTransformVectorFromPySequence(pytransforms, data))
        {
            PyErr_SetString(PyExc_TypeError,
                "Kwarg 'transforms' must be a transform array.");
            return -1;
        }
        for(unsigned int i=0; i<data.size(); ++i)
            ptr->appendTransform(data[i]);
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
    GroupTransformRcPtr transform = GetEditableGroupTransform(self);
    TransformRcPtr childTransform = transform->getTransform(index);
    return BuildConstPyTransform(childTransform);
    OCIO_PYTRY_EXIT(NULL)
}

PyObject * PyOCIO_GroupTransform_getNumTransforms(PyObject * self, PyObject *)
{
    OCIO_PYTRY_ENTER()
    ConstGroupTransformRcPtr transform = GetConstGroupTransform(self);
    return PyInt_FromLong(transform->getNumTransforms());
    OCIO_PYTRY_EXIT(NULL)
}

PyObject * PyOCIO_GroupTransform_appendTransform(PyObject * self, PyObject * args)
{
    OCIO_PYTRY_ENTER()
    PyObject* pytransform = 0;
    if (!PyArg_ParseTuple(args,"O:appendTransform",
        &pytransform)) return NULL;
    GroupTransformRcPtr transform = GetEditableGroupTransform(self);
    if(!IsPyTransform(pytransform))
        throw Exception("GroupTransform.appendTransform requires a transform as the first arg.");
    transform->appendTransform(GetEditableTransform(pytransform));
    Py_RETURN_NONE;
    OCIO_PYTRY_EXIT(NULL)
}

}

} // namespace OCIO_NAMESPACE
