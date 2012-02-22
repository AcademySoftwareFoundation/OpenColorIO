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

#include "PyTransform.h"
#include "PyUtil.h"


/*+doc
Python: CDLTransform
====================

Examples of Use
^^^^^^^^^^^^^^^
.. code-block:: python

    import PyOpenColorIO as OCIO
    
    cdl = OCIO.CDLTransform()
    
    # Set the slope, offset, power, and saturation for each channel.
    cdl.setSOP([, , , , , , , , ])
    cdl.setSat([, , ])
    
    cdl.getSatLumaCoefs()
*/

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
    
    bool IsPyCDLTransform(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return PyObject_TypeCheck(pyobject, &PyOCIO_CDLTransformType);
    }
    
    ConstCDLTransformRcPtr GetConstCDLTransform(PyObject * pyobject, bool allowCast)
    {
        ConstCDLTransformRcPtr transform = \
            DynamicPtrCast<const CDLTransform>(GetConstTransform(pyobject, allowCast));
        if(!transform)
        {
            throw Exception("PyObject must be a valid OCIO.CDLTransform.");
        }
        return transform;
    }
    
    CDLTransformRcPtr GetEditableCDLTransform(PyObject * pyobject)
    {
        CDLTransformRcPtr transform = \
            DynamicPtrCast<CDLTransform>(GetEditableTransform(pyobject));
        if(!transform)
        {
            throw Exception("PyObject must be a valid OCIO.CDLTransform.");
        }
        return transform;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    namespace
    {
        int PyOCIO_CDLTransform_init( PyOCIO_Transform * self, PyObject * args, PyObject * kwds );
        
        PyObject * PyOCIO_CDLTransform_equals( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_CDLTransform_getXML( PyObject * self );
        PyObject * PyOCIO_CDLTransform_setXML( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_CDLTransform_getSlope( PyObject * self );
        PyObject * PyOCIO_CDLTransform_getOffset( PyObject * self );
        PyObject * PyOCIO_CDLTransform_getPower( PyObject * self );
        PyObject * PyOCIO_CDLTransform_getSOP( PyObject * self );
        PyObject * PyOCIO_CDLTransform_getSat( PyObject * self );
        
        PyObject * PyOCIO_CDLTransform_setSlope( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_CDLTransform_setOffset( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_CDLTransform_setPower( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_CDLTransform_setSOP( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_CDLTransform_setSat( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_CDLTransform_getSatLumaCoefs( PyObject * self );
        
        PyObject * PyOCIO_CDLTransform_getID( PyObject * self );
        PyObject * PyOCIO_CDLTransform_setID( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_CDLTransform_getDescription( PyObject * self );
        PyObject * PyOCIO_CDLTransform_setDescription( PyObject * self,  PyObject *args );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_CDLTransform_methods[] = {
            {"equals", PyOCIO_CDLTransform_equals, METH_VARARGS, "" },
            
            {"getXML", (PyCFunction) PyOCIO_CDLTransform_getXML, METH_NOARGS, "" },
            {"setXML", PyOCIO_CDLTransform_setXML, METH_VARARGS, "" },
            
            {"getSlope", (PyCFunction) PyOCIO_CDLTransform_getSlope, METH_NOARGS, "" },
            {"getOffset", (PyCFunction) PyOCIO_CDLTransform_getOffset, METH_NOARGS, "" },
            {"getPower", (PyCFunction) PyOCIO_CDLTransform_getPower, METH_NOARGS, "" },
            {"getSOP", (PyCFunction) PyOCIO_CDLTransform_getSOP, METH_NOARGS, "" },
            {"getSat", (PyCFunction) PyOCIO_CDLTransform_getSat, METH_NOARGS, "" },
            
            {"setSlope", PyOCIO_CDLTransform_setSlope, METH_VARARGS, "" },
            {"setOffset", PyOCIO_CDLTransform_setOffset, METH_VARARGS, "" },
            {"setPower", PyOCIO_CDLTransform_setPower, METH_VARARGS, "" },
            {"setSOP", PyOCIO_CDLTransform_setSOP, METH_VARARGS, "" },
            {"setSat", PyOCIO_CDLTransform_setSat, METH_VARARGS, "" },
            
            {"getSatLumaCoefs", (PyCFunction) PyOCIO_CDLTransform_getSatLumaCoefs, METH_NOARGS, "" },
            
            {"getID", (PyCFunction) PyOCIO_CDLTransform_getID, METH_NOARGS, "" },
            {"setID", PyOCIO_CDLTransform_setID, METH_VARARGS, "" },
            {"getDescription", (PyCFunction) PyOCIO_CDLTransform_getDescription, METH_NOARGS, "" },
            {"setDescription", PyOCIO_CDLTransform_setDescription, METH_VARARGS, "" },
            
            {NULL, NULL, 0, NULL}
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_CDLTransformType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.CDLTransform",                        //tp_name
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
        "CDLTransform",                             //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_CDLTransform_methods,                //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        &PyOCIO_TransformType,                      //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_CDLTransform_init,        //tp_init 
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
    
    /*+doc
    .. py:class::CDLTransform()
    
    Description
    ^^^^^^^^^^^
       Used to define a transform based on a color decision list (CDL), 
       based on the 9 numbers defined in SOP (slope, offset, and power)
       and Sat (saturation). Each element in SOP and Sat are characterized 
       by three floats (for RGB).
    */
    
    namespace
    {
        ///////////////////////////////////////////////////////////////////////
        ///
        int PyOCIO_CDLTransform_init( PyOCIO_Transform *self, PyObject * /*args*/, PyObject * /*kwds*/ )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            
            self->constcppobj = new ConstTransformRcPtr();
            self->cppobj = new TransformRcPtr();
            self->isconst = true;
            
            try
            {
                *self->cppobj = CDLTransform::Create();
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
        
        PyObject * PyOCIO_CDLTransform_equals( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyother = 0;
                
                if (!PyArg_ParseTuple(args,"O:equals",
                    &pyother)) return NULL;
                
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                if(!IsPyCDLTransform(pyother))
                {
                    return PyBool_FromLong(false);
                }
                
                ConstCDLTransformRcPtr other = GetConstCDLTransform(pyother, true);
                
                return PyBool_FromLong(transform->equals(other));
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        ////////////////////////////////////////////////////////////////////////
        
        
        PyObject * PyOCIO_CDLTransform_getXML( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                return PyString_FromString( transform->getXML() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CDLTransform_setXML( PyObject * self, PyObject * args )
        {
            try
            {
                const char * str = 0;
                if (!PyArg_ParseTuple(args,"s:setXML",
                    &str)) return NULL;
                
                CDLTransformRcPtr transform = GetEditableCDLTransform(self);
                transform->setXML( str );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        ////////////////////////////////////////////////////////////////////////
        
        
        PyObject * PyOCIO_CDLTransform_getSlope( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                std::vector<float> data(3);
                transform->getSlope(&data[0]);
                return CreatePyListFromFloatVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CDLTransform_getOffset( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                std::vector<float> data(3);
                transform->getOffset(&data[0]);
                return CreatePyListFromFloatVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CDLTransform_getPower( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                std::vector<float> data(3);
                transform->getPower(&data[0]);
                return CreatePyListFromFloatVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CDLTransform_getSOP( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                std::vector<float> data(9);
                transform->getSOP(&data[0]);
                return CreatePyListFromFloatVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_CDLTransform_getSat( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                return PyFloat_FromDouble(transform->getSat());
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        
        ////////////////////////////////////////////////////////////////////////
        
        /*+doc
        .. py:method:: Config.setSlope(pyData)
                     
           Sets the slope ('S' part of SOP) in :py:class:`CDLTransform`.

           :param pyData: 
           :type pyData: object
        */        
        PyObject * PyOCIO_CDLTransform_setSlope( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O:setSlope", &pyData)) return NULL;
                CDLTransformRcPtr transform = GetEditableCDLTransform(self);
                
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pyData, data) || (data.size() != 3))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 3");
                    return 0;
                }
                
                transform->setSlope( &data[0] );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        /*+doc
        .. py:method:: Config.setOffset(pyData)
                     
           Sets the offset ('O' part of SOP) in :py:class:`CDLTransform`.

           :param pyData: list of three floats
           :type pyData: object
        */        
        PyObject * PyOCIO_CDLTransform_setOffset( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O:setOffset", &pyData)) return NULL;
                CDLTransformRcPtr transform = GetEditableCDLTransform(self);
                
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pyData, data) || (data.size() != 3))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 3");
                    return 0;
                }
                
                transform->setOffset( &data[0] );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        /*+doc
        .. py:method:: Config.setPower(pyData)
                     
           Sets the power ('P' part of SOP) in :py:class:`CDLTransform`.

           :param pyData: list of three floats
           :type pyData: object
        */        
        PyObject * PyOCIO_CDLTransform_setPower( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O:setPower", &pyData)) return NULL;
                CDLTransformRcPtr transform = GetEditableCDLTransform(self);
                
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pyData, data) || (data.size() != 3))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 3");
                    return 0;
                }
                
                transform->setPower( &data[0] );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        /*+doc
        .. py:method:: Config.setSOP(pyData)
                     
           Sets SOP in :py:class:`CDLTransform`.

           :param pyData: list of nine floats
           :type pyData: object
        */        
        PyObject * PyOCIO_CDLTransform_setSOP( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyData = 0;
                if (!PyArg_ParseTuple(args,"O:setSOP", &pyData)) return NULL;
                CDLTransformRcPtr transform = GetEditableCDLTransform(self);
                
                std::vector<float> data;
                if(!FillFloatVectorFromPySequence(pyData, data) || (data.size() != 9))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array, size 9");
                    return 0;
                }
                
                transform->setSOP( &data[0] );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        /*+doc
        .. py:method:: Config.setSAT(pyData)
                     
           Sets SAT (saturation) in :py:class:`CDLTransform`.

           :param pyData: saturation
           :type pyData: float
        */        
        PyObject * PyOCIO_CDLTransform_setSat( PyObject * self, PyObject * args )
        {
            try
            {
                float sat;
                if (!PyArg_ParseTuple(args,"f:setSat", &sat)) return NULL;
                CDLTransformRcPtr transform = GetEditableCDLTransform(self);
                
                transform->setSat( sat );
                
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
        .. py:method:: Config.getSatLumaCoefs(pyData)
                     
           Returns the SAT (saturation) and luma coefficients in :py:class:`CDLTransform`.

           :return: saturation and luma coefficients
           :rtype: list of floats
        */        
        PyObject * PyOCIO_CDLTransform_getSatLumaCoefs( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                std::vector<float> data(3);
                transform->getSatLumaCoefs(&data[0]);
                return CreatePyListFromFloatVector(data);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        
        /*+doc
        .. py:method:: Config.getID()
                     
           Returns the ID from :py:class:`CDLTransform`.

           :return: ID
           :rtype: string
        */        
        PyObject * PyOCIO_CDLTransform_getID( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                return PyString_FromString( transform->getID() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        /*+doc
        .. py:method:: Config.setID(str)
                     
           Sets the ID in :py:class:`CDLTransform`.

           :param str: ID
           :type str: string
        */        
        PyObject * PyOCIO_CDLTransform_setID( PyObject * self, PyObject * args )
        {
            try
            {
                const char * str = 0;
                if (!PyArg_ParseTuple(args,"s:setID",
                    &str)) return NULL;
                
                CDLTransformRcPtr transform = GetEditableCDLTransform(self);
                transform->setID( str );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        /*+doc
        .. py:method:: Config.getDescription()
                     
           Returns the description of :py:class:`CDLTransform`.

           :return: description
           :rtype: string
        */        
        PyObject * PyOCIO_CDLTransform_getDescription( PyObject * self )
        {
            try
            {
                ConstCDLTransformRcPtr transform = GetConstCDLTransform(self, true);
                return PyString_FromString( transform->getDescription() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        /*+doc
        .. py:method:: Config.setDescription(str)
                     
           Sets the description of :py:class:`CDLTransform`.

           :param str: description
           :type str: string
        */        
        PyObject * PyOCIO_CDLTransform_setDescription( PyObject * self, PyObject * args )
        {
            try
            {
                const char * str = 0;
                if (!PyArg_ParseTuple(args,"s:setDescription",
                    &str)) return NULL;
                
                CDLTransformRcPtr transform = GetEditableCDLTransform(self);
                transform->setDescription( str );
                
                Py_RETURN_NONE;
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
