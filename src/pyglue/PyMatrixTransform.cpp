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

#define GetConstMatrixTransform(pyobject) GetConstPyOCIO<PyOCIO_Transform, \
    ConstMatrixTransformRcPtr, MatrixTransform>(pyobject, \
    PyOCIO_MatrixTransformType)

#define GetEditableMatrixTransform(pyobject) GetEditablePyOCIO<PyOCIO_Transform, \
    MatrixTransformRcPtr, MatrixTransform>(pyobject, \
    PyOCIO_MatrixTransformType)

OCIO_NAMESPACE_ENTER
{
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_MatrixTransform_init(PyOCIO_Transform * self, PyObject * args, PyObject * kwds);
        PyObject * PyOCIO_MatrixTransform_equals(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_MatrixTransform_getValue(PyObject * self);
        PyObject * PyOCIO_MatrixTransform_setValue(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_MatrixTransform_getMatrix(PyObject * self);
        PyObject * PyOCIO_MatrixTransform_setMatrix(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_MatrixTransform_getOffset(PyObject * self);
        PyObject * PyOCIO_MatrixTransform_setOffset(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_MatrixTransform_Identity(PyObject * cls);
        PyObject * PyOCIO_MatrixTransform_Fit(PyObject * cls, PyObject * args);
        PyObject * PyOCIO_MatrixTransform_Sat(PyObject * cls, PyObject * args);
        PyObject * PyOCIO_MatrixTransform_Scale(PyObject * cls, PyObject * args);
        PyObject * PyOCIO_MatrixTransform_View(PyObject * cls, PyObject * args);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_MatrixTransform_methods[] = {
            { "equals",
            PyOCIO_MatrixTransform_equals, METH_VARARGS, MATRIXTRANSFORM_EQUALS__DOC__ },
            { "getValue",
            (PyCFunction) PyOCIO_MatrixTransform_getValue, METH_NOARGS, MATRIXTRANSFORM_GETVALUE__DOC__ },
            { "setValue",
            PyOCIO_MatrixTransform_setValue, METH_VARARGS, MATRIXTRANSFORM_SETVALUE__DOC__ },
            { "getMatrix",
            (PyCFunction) PyOCIO_MatrixTransform_getMatrix, METH_NOARGS, MATRIXTRANSFORM_GETMATRIX__DOC__ },
            { "setMatrix",
            PyOCIO_MatrixTransform_setMatrix, METH_VARARGS, MATRIXTRANSFORM_SETMATRIX__DOC__ },
            { "getOffset",
            (PyCFunction) PyOCIO_MatrixTransform_getOffset, METH_NOARGS, MATRIXTRANSFORM_GETOFFSET__DOC__ },
            { "setOffset",
            PyOCIO_MatrixTransform_setOffset, METH_VARARGS, MATRIXTRANSFORM_SETOFFSET__DOC__ },
            { "Identity",
            (PyCFunction) PyOCIO_MatrixTransform_Identity, METH_NOARGS | METH_CLASS, MATRIXTRANSFORM_IDENTITY__DOC__ },
            { "Fit",
            PyOCIO_MatrixTransform_Fit, METH_VARARGS | METH_CLASS, MATRIXTRANSFORM_FIT__DOC__ },
            { "Sat",
            PyOCIO_MatrixTransform_Sat, METH_VARARGS | METH_CLASS, MATRIXTRANSFORM_SAT__DOC__ },
            { "Scale",
            PyOCIO_MatrixTransform_Scale, METH_VARARGS | METH_CLASS, MATRIXTRANSFORM_SCALE__DOC__ },
            { "View",
            PyOCIO_MatrixTransform_View, METH_VARARGS | METH_CLASS, MATRIXTRANSFORM_VIEW__DOC__ },
            { NULL, NULL, 0, NULL }
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_MatrixTransformType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        OCIO_PYTHON_NAMESPACE(MatrixTransform),     //tp_name
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
        MATRIXTRANSFORM__DOC__,                     //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_MatrixTransform_methods,             //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        &PyOCIO_TransformType,                      //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_MatrixTransform_init,     //tp_init 
        0,                                          //tp_alloc 
        0,                                          //tp_new 
        0,                                          //tp_free
        0,                                          //tp_is_gc
    };
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        int PyOCIO_MatrixTransform_init(PyOCIO_Transform * self, PyObject * /*args*/, PyObject * /*kwds*/)
        {
            OCIO_PYTRY_ENTER()
            return BuildPyTransformObject<MatrixTransformRcPtr>(self, MatrixTransform::Create());
            OCIO_PYTRY_EXIT(-1)
        }
        
        PyObject * PyOCIO_MatrixTransform_equals(PyObject * self,  PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pyobject = 0;
            if (!PyArg_ParseTuple(args,"O:equals",
                &pyobject)) return NULL;
            if(!IsPyOCIOType(pyobject, PyOCIO_MatrixTransformType))
                throw Exception("MatrixTransform.equals requires a MatrixTransform argument");
            ConstMatrixTransformRcPtr transform = GetConstMatrixTransform(self);
            ConstMatrixTransformRcPtr in = GetConstMatrixTransform(pyobject);
            return PyBool_FromLong(transform->equals(*in.get()));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_MatrixTransform_getValue(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstMatrixTransformRcPtr transform = GetConstMatrixTransform(self);
            std::vector<float> matrix(16);
            std::vector<float> offset(4);
            transform->getValue(&matrix[0], &offset[0]);
            PyObject* pymatrix = CreatePyListFromFloatVector(matrix);
            PyObject* pyoffset = CreatePyListFromFloatVector(offset);
            PyObject* pyreturnval = Py_BuildValue("(OO)", pymatrix, pyoffset);
            Py_DECREF(pymatrix);
            Py_DECREF(pyoffset);
            return pyreturnval;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_MatrixTransform_setValue(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pymatrix = 0;
            PyObject* pyoffset = 0;
            if (!PyArg_ParseTuple(args, "OO:setValue",
                &pymatrix, &pyoffset)) return NULL;
            std::vector<float> matrix;
            std::vector<float> offset;
            if(!FillFloatVectorFromPySequence(pymatrix, matrix) ||
                (matrix.size() != 16))
            {
                PyErr_SetString(PyExc_TypeError,
                    "First argument must be a float array, size 16");
                return 0;
            }
            if(!FillFloatVectorFromPySequence(pyoffset, offset) ||
                (offset.size() != 4))
            {
                PyErr_SetString(PyExc_TypeError,
                    "Second argument must be a float array, size 4");
                return 0;
            }
            MatrixTransformRcPtr transform = GetEditableMatrixTransform(self);
            transform->setValue(&matrix[0], &offset[0]);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_MatrixTransform_getMatrix(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstMatrixTransformRcPtr transform = GetConstMatrixTransform(self);
            std::vector<float> matrix(16);
            transform->getMatrix(&matrix[0]);
            return CreatePyListFromFloatVector(matrix);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_MatrixTransform_setMatrix(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pymatrix = 0;
            if (!PyArg_ParseTuple(args,"O:setValue",
                &pymatrix)) return NULL;
            std::vector<float> matrix;
            if(!FillFloatVectorFromPySequence(pymatrix, matrix) ||
                (matrix.size() != 16))
            {
                PyErr_SetString(PyExc_TypeError,
                    "First argument must be a float array, size 16");
                return 0;
            }
            MatrixTransformRcPtr transform = GetEditableMatrixTransform(self);
            transform->setMatrix(&matrix[0]);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_MatrixTransform_getOffset(PyObject * self)
        {
            OCIO_PYTRY_ENTER()
            ConstMatrixTransformRcPtr transform = GetConstMatrixTransform(self);
            std::vector<float> offset(4);
            transform->getOffset(&offset[0]);
            return CreatePyListFromFloatVector(offset);
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_MatrixTransform_setOffset(PyObject * self, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            PyObject* pyoffset = 0;
            if (!PyArg_ParseTuple(args, "O:setValue",
                &pyoffset)) return NULL;
            std::vector<float> offset;
            if(!FillFloatVectorFromPySequence(pyoffset, offset) ||
                (offset.size() != 4))
            {
                PyErr_SetString(PyExc_TypeError,
                    "First argument must be a float array, size 4");
                return 0;
            }
            MatrixTransformRcPtr transform = GetEditableMatrixTransform(self);
            transform->setOffset(&offset[0]);
            Py_RETURN_NONE;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_MatrixTransform_Identity(PyObject * /*self*/)
        {
            OCIO_PYTRY_ENTER()
            std::vector<float> matrix(16);
            std::vector<float> offset(4);
            MatrixTransform::Identity(&matrix[0], &offset[0]);
            PyObject* pymatrix = CreatePyListFromFloatVector(matrix);
            PyObject* pyoffset = CreatePyListFromFloatVector(offset);
            PyObject* pyreturnval = Py_BuildValue("(OO)", pymatrix, pyoffset);
            Py_DECREF(pymatrix);
            Py_DECREF(pyoffset);
            return pyreturnval;
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_MatrixTransform_Fit(PyObject * /*self*/, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            
            PyObject* pyoldmin = 0;
            PyObject* pyoldmax = 0;
            PyObject* pynewmin = 0;
            PyObject* pynewmax = 0;
            if (!PyArg_ParseTuple(args,"OOOO:Fit",
                &pyoldmin, &pyoldmax, &pynewmin, &pynewmax)) return NULL;
            
            std::vector<float> oldmin;
            if(!FillFloatVectorFromPySequence(pyoldmin, oldmin) ||
                (oldmin.size() != 4))
            {
                PyErr_SetString(PyExc_TypeError,
                    "First argument must be a float array, size 4");
                return 0;
            }
            
            std::vector<float> oldmax;
            if(!FillFloatVectorFromPySequence(pyoldmax, oldmax) ||
                (oldmax.size() != 4))
            {
                PyErr_SetString(PyExc_TypeError,
                    "Second argument must be a float array, size 4");
                return 0;
            }
            
            std::vector<float> newmin;
            if(!FillFloatVectorFromPySequence(pynewmin, newmin) ||
                (newmin.size() != 4))
            {
                PyErr_SetString(PyExc_TypeError,
                    "Third argument must be a float array, size 4");
                return 0;
            }
            
            std::vector<float> newmax;
            if(!FillFloatVectorFromPySequence(pynewmax, newmax) ||
                (newmax.size() != 4))
            {
                PyErr_SetString(PyExc_TypeError,
                    "Fourth argument must be a float array, size 4");
                return 0;
            }
            
            std::vector<float> matrix(16);
            std::vector<float> offset(4);
            MatrixTransform::Fit(&matrix[0], &offset[0],
                                 &oldmin[0], &oldmax[0],
                                 &newmin[0], &newmax[0]);
            PyObject* pymatrix = CreatePyListFromFloatVector(matrix);
            PyObject* pyoffset = CreatePyListFromFloatVector(offset);
            PyObject* pyreturnval = Py_BuildValue("(OO)", pymatrix, pyoffset);
            Py_DECREF(pymatrix);
            Py_DECREF(pyoffset);
            return pyreturnval;
            
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_MatrixTransform_Sat(PyObject * /*self*/, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            
            float sat = 0.0;
            PyObject* pyluma = 0;
            if (!PyArg_ParseTuple(args,"fO:Sat",
                &sat, &pyluma)) return NULL;
                
            std::vector<float> luma;
            if(!FillFloatVectorFromPySequence(pyluma, luma) ||
                (luma.size() != 3))
            {
                PyErr_SetString(PyExc_TypeError,
                    "Second argument must be a float array, size 3");
                return 0;
            }
            
            std::vector<float> matrix(16);
            std::vector<float> offset(4);
            MatrixTransform::Sat(&matrix[0], &offset[0],
                                 sat, &luma[0]);
            PyObject* pymatrix = CreatePyListFromFloatVector(matrix);
            PyObject* pyoffset = CreatePyListFromFloatVector(offset);
            PyObject* pyreturnval = Py_BuildValue("(OO)", pymatrix, pyoffset);
            Py_DECREF(pymatrix);
            Py_DECREF(pyoffset);
            return pyreturnval;
            
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_MatrixTransform_Scale(PyObject * /*self*/, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            
            PyObject* pyscale = 0;
            if (!PyArg_ParseTuple(args,"O:Scale",
                &pyscale)) return NULL;
            
            std::vector<float> scale;
            if(!FillFloatVectorFromPySequence(pyscale, scale) ||
                (scale.size() != 4))
            {
                PyErr_SetString(PyExc_TypeError,
                    "Second argument must be a float array, size 4");
                return 0;
            }
            
            std::vector<float> matrix(16);
            std::vector<float> offset(4);
            MatrixTransform::Scale(&matrix[0], &offset[0], &scale[0]);
            PyObject* pymatrix = CreatePyListFromFloatVector(matrix);
            PyObject* pyoffset = CreatePyListFromFloatVector(offset);
            PyObject* pyreturnval = Py_BuildValue("(OO)", pymatrix, pyoffset);
            Py_DECREF(pymatrix);
            Py_DECREF(pyoffset);
            return pyreturnval;
            
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_MatrixTransform_View(PyObject * /*self*/, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            
            PyObject* pychannelhot = 0;
            PyObject* pyluma = 0;
            if (!PyArg_ParseTuple(args,"OO:View",
                &pychannelhot, &pyluma)) return NULL;
            
            std::vector<int> channelhot;
            if(!FillIntVectorFromPySequence(pychannelhot, channelhot) ||
                (channelhot.size() != 4))
            {
                PyErr_SetString(PyExc_TypeError,
                    "First argument must be a bool/int array, size 4");
                return 0;
            }
            
            std::vector<float> luma;
            if(!FillFloatVectorFromPySequence(pyluma, luma) ||
                (luma.size() != 3))
            {
                PyErr_SetString(PyExc_TypeError,
                    "Second argument must be a float array, size 3");
                return 0;
            }
            
            std::vector<float> matrix(16);
            std::vector<float> offset(4);
            MatrixTransform::View(&matrix[0], &offset[0],
                                  &channelhot[0], &luma[0]);
            PyObject* pymatrix = CreatePyListFromFloatVector(matrix);
            PyObject* pyoffset = CreatePyListFromFloatVector(offset);
            PyObject* pyreturnval = Py_BuildValue("(OO)", pymatrix, pyoffset);
            Py_DECREF(pymatrix);
            Py_DECREF(pyoffset);
            return pyreturnval;
            
            OCIO_PYTRY_EXIT(NULL)
        }
        
    }

}
OCIO_NAMESPACE_EXIT
