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

#include <OpenColourIO/OpenColourIO.h>

#include "PyColourSpace.h"
#include "PyTransform.h"
#include "PyUtil.h"
#include "PyDoc.h"

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    bool AddColourSpaceObjectToModule( PyObject* m )
    {
        PyOCIO_ColourSpaceType.tp_new = PyType_GenericNew;
        if ( PyType_Ready(&PyOCIO_ColourSpaceType) < 0 ) return false;
        
        Py_INCREF( &PyOCIO_ColourSpaceType );
        PyModule_AddObject(m, "ColourSpace",
                (PyObject *)&PyOCIO_ColourSpaceType);
        
        return true;
    }
    
    PyObject * BuildConstPyColourSpace(ConstColourSpaceRcPtr colourSpace)
    {
        if (!colourSpace)
        {
            Py_RETURN_NONE;
        }
        
        PyOCIO_ColourSpace * pycolourSpace = PyObject_New(
                PyOCIO_ColourSpace, (PyTypeObject * ) &PyOCIO_ColourSpaceType);
        
        pycolourSpace->constcppobj = new ConstColourSpaceRcPtr();
        *pycolourSpace->constcppobj = colourSpace;
        
        pycolourSpace->cppobj = new ColourSpaceRcPtr();
        pycolourSpace->isconst = true;
        
        return ( PyObject * ) pycolourSpace;
    }
    
    PyObject * BuildEditablePyColourSpace(ColourSpaceRcPtr colourSpace)
    {
        if (!colourSpace)
        {
            Py_RETURN_NONE;
        }
        
        PyOCIO_ColourSpace * pycolourSpace = PyObject_New(
                PyOCIO_ColourSpace, (PyTypeObject * ) &PyOCIO_ColourSpaceType);
        
        pycolourSpace->constcppobj = new ConstColourSpaceRcPtr();
        pycolourSpace->cppobj = new ColourSpaceRcPtr();
        *pycolourSpace->cppobj = colourSpace;
        
        pycolourSpace->isconst = false;
        
        return ( PyObject * ) pycolourSpace;
    }
    
    bool IsPyColourSpace(PyObject * pyobject)
    {
        if(!pyobject) return false;
        return (PyObject_Type(pyobject) == (PyObject *) (&PyOCIO_ColourSpaceType));
    }
    
    bool IsPyColourSpaceEditable(PyObject * pyobject)
    {
        if(!IsPyColourSpace(pyobject))
        {
            throw Exception("PyObject must be an OCIO.ColourSpace.");
        }
        
        PyOCIO_ColourSpace * pycolourSpace = reinterpret_cast<PyOCIO_ColourSpace *> (pyobject);
        return (!pycolourSpace->isconst);
    }
    
    ConstColourSpaceRcPtr GetConstColourSpace(PyObject * pyobject, bool allowCast)
    {
        if(!IsPyColourSpace(pyobject))
        {
            throw Exception("PyObject must be an OCIO.ColourSpace.");
        }
        
        PyOCIO_ColourSpace * pycolourspace = reinterpret_cast<PyOCIO_ColourSpace *> (pyobject);
        if(pycolourspace->isconst && pycolourspace->constcppobj)
        {
            return *pycolourspace->constcppobj;
        }
        
        if(allowCast && !pycolourspace->isconst && pycolourspace->cppobj)
        {
            return *pycolourspace->cppobj;
        }
        
        throw Exception("PyObject must be a valid OCIO.ColourSpace.");
    }
    
    ColourSpaceRcPtr GetEditableColourSpace(PyObject * pyobject)
    {
        if(!IsPyColourSpace(pyobject))
        {
            throw Exception("PyObject must be an OCIO.ColourSpace.");
        }
        
        PyOCIO_ColourSpace * pycolourspace = reinterpret_cast<PyOCIO_ColourSpace *> (pyobject);
        if(!pycolourspace->isconst && pycolourspace->cppobj)
        {
            return *pycolourspace->cppobj;
        }
        
        throw Exception("PyObject must be an editable OCIO.ColourSpace.");
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    namespace
    {
        int PyOCIO_ColourSpace_init( PyOCIO_ColourSpace * self, PyObject * args, PyObject * kwds );
        void PyOCIO_ColourSpace_delete( PyOCIO_ColourSpace * self, PyObject * args );
        PyObject * PyOCIO_ColourSpace_isEditable( PyObject * self );
        PyObject * PyOCIO_ColourSpace_createEditableCopy( PyObject * self );
        
        PyObject * PyOCIO_ColourSpace_getName( PyObject * self );
        PyObject * PyOCIO_ColourSpace_setName( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColourSpace_getFamily( PyObject * self );
        PyObject * PyOCIO_ColourSpace_setFamily( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColourSpace_getEqualityGroup( PyObject * self );
        PyObject * PyOCIO_ColourSpace_setEqualityGroup( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColourSpace_getDescription( PyObject * self );
        PyObject * PyOCIO_ColourSpace_setDescription( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_ColourSpace_getBitDepth( PyObject * self );
        PyObject * PyOCIO_ColourSpace_setBitDepth( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColourSpace_isData( PyObject * self );
        PyObject * PyOCIO_ColourSpace_setIsData( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColourSpace_getAllocation( PyObject * self );
        PyObject * PyOCIO_ColourSpace_setAllocation( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColourSpace_getAllocationVars( PyObject * self );
        PyObject * PyOCIO_ColourSpace_setAllocationVars( PyObject * self,  PyObject *args );
        
        PyObject * PyOCIO_ColourSpace_getTransform( PyObject * self,  PyObject *args );
        PyObject * PyOCIO_ColourSpace_setTransform( PyObject * self,  PyObject *args );
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef PyOCIO_ColourSpace_methods[] = {
            {"isEditable",
            (PyCFunction) PyOCIO_ColourSpace_isEditable, METH_NOARGS, COLOURSPACE_ISEDITABLE__DOC__ },
            {"createEditableCopy",
            (PyCFunction) PyOCIO_ColourSpace_createEditableCopy, METH_NOARGS, COLOURSPACE_CREATEEDITABLECOPY__DOC__ },
            {"getName",
            (PyCFunction) PyOCIO_ColourSpace_getName, METH_NOARGS, COLOURSPACE_GETNAME__DOC__ },
            {"setName",
            PyOCIO_ColourSpace_setName, METH_VARARGS, COLOURSPACE_SETNAME__DOC__ },
            {"getFamily",
            (PyCFunction) PyOCIO_ColourSpace_getFamily, METH_NOARGS, COLOURSPACE_GETFAMILY__DOC__ },
            {"setFamily",
            PyOCIO_ColourSpace_setFamily, METH_VARARGS, COLOURSPACE_SETFAMILY__DOC__ },
            {"getEqualityGroup",
            (PyCFunction) PyOCIO_ColourSpace_getEqualityGroup, METH_NOARGS, COLOURSPACE_GETEQUALITYGROUP__DOC__ },
            {"setEqualityGroup",
            PyOCIO_ColourSpace_setEqualityGroup, METH_VARARGS, COLOURSPACE_SETEQUALITYGROUP__DOC__ },
            {"getDescription",
            (PyCFunction) PyOCIO_ColourSpace_getDescription, METH_NOARGS, COLOURSPACE_GETDESCRIPTION__DOC__ },
            {"setDescription",
            PyOCIO_ColourSpace_setDescription, METH_VARARGS, COLOURSPACE_SETDESCRIPTION__DOC__ },
            {"getBitDepth",
            (PyCFunction) PyOCIO_ColourSpace_getBitDepth, METH_NOARGS, COLOURSPACE_GETBITDEPTH__DOC__ },
            {"setBitDepth",
            PyOCIO_ColourSpace_setBitDepth, METH_VARARGS, COLOURSPACE_SETBITDEPTH__DOC__ },
            {"isData",
            (PyCFunction) PyOCIO_ColourSpace_isData, METH_NOARGS, COLOURSPACE_ISDATA__DOC__ },
            {"setIsData",
            PyOCIO_ColourSpace_setIsData, METH_VARARGS, COLOURSPACE_SETISDATA__DOC__ },
            {"getAllocation",
            (PyCFunction) PyOCIO_ColourSpace_getAllocation, METH_NOARGS, COLOURSPACE_GETALLOCATION__DOC__ },
            {"setAllocation",
            PyOCIO_ColourSpace_setAllocation, METH_VARARGS, COLOURSPACE_SETALLOCATION__DOC__ },
            {"getAllocationVars",
            (PyCFunction) PyOCIO_ColourSpace_getAllocationVars, METH_NOARGS, COLOURSPACE_GETALLOCATIONVARS__DOC__ },
            {"setAllocationVars",
            PyOCIO_ColourSpace_setAllocationVars, METH_VARARGS, COLOURSPACE_SETALLOCATIONVARS__DOC__ },
            {"getTransform",
            PyOCIO_ColourSpace_getTransform, METH_VARARGS, COLOURSPACE_GETTRANSFORM__DOC__ },
            {"setTransform",
            PyOCIO_ColourSpace_setTransform, METH_VARARGS, COLOURSPACE_SETTRANSFORM__DOC__ },
            {NULL, NULL, 0, NULL}
        };
    }
    
    ///////////////////////////////////////////////////////////////////////////
    ///
    
    PyTypeObject PyOCIO_ColourSpaceType = {
        PyObject_HEAD_INIT(NULL)
        0,                                          //ob_size
        "OCIO.ColourSpace",                           //tp_name
        sizeof(PyOCIO_ColourSpace),                   //tp_basicsize
        0,                                          //tp_itemsize
        (destructor)PyOCIO_ColourSpace_delete,        //tp_dealloc
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
        COLOURSPACE__DOC__,                          //tp_doc 
        0,                                          //tp_traverse 
        0,                                          //tp_clear 
        0,                                          //tp_richcompare 
        0,                                          //tp_weaklistoffset 
        0,                                          //tp_iter 
        0,                                          //tp_iternext 
        PyOCIO_ColourSpace_methods,                   //tp_methods 
        0,                                          //tp_members 
        0,                                          //tp_getset 
        0,                                          //tp_base 
        0,                                          //tp_dict 
        0,                                          //tp_descr_get 
        0,                                          //tp_descr_set 
        0,                                          //tp_dictoffset 
        (initproc) PyOCIO_ColourSpace_init,           //tp_init 
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
        ///////////////////////////////////////////////////////////////////////
        ///
        int PyOCIO_ColourSpace_init( PyOCIO_ColourSpace *self, PyObject * args, PyObject * kwds )
        {
            ///////////////////////////////////////////////////////////////////
            /// init pyobject fields
            
            self->constcppobj = new ConstColourSpaceRcPtr();
            self->cppobj = new ColourSpaceRcPtr();
            self->isconst = true;
            
            // Parse optional kwargs
            char * name = NULL;
            char * family = NULL;
            char * equalityGroup = NULL;
            char * description = NULL;
            char * bitDepth = NULL;
            bool isData = false; // TODO: Do not rely on the default value
            char * allocation = NULL;
            PyObject * allocationVars = NULL;
            PyObject * toRefTransform = NULL;
            PyObject * fromRefTransform = NULL;
            
            
            const char * toRefStr = 
                ColourSpaceDirectionToString(COLOURSPACE_DIR_TO_REFERENCE);
            const char * fromRefStr = 
                ColourSpaceDirectionToString(COLOURSPACE_DIR_FROM_REFERENCE);
            const char *kwlist[] = {
                "name", "family", "equalityGroup",
                "description", "bitDepth",
                "isData",
                "allocation", "allocationVars",
                toRefStr, fromRefStr,
                NULL
            };
            
            if(!PyArg_ParseTupleAndKeywords(args, kwds, "|sssssO&sOOO",
                const_cast<char **>(kwlist),
                &name, &family, &equalityGroup, &description, &bitDepth,
                ConvertPyObjectToBool, &isData,
                &allocation, &allocationVars,
                &toRefTransform, &fromRefTransform)) return -1;
            
            try
            {
                ColourSpaceRcPtr colourSpace = ColourSpace::Create();
                *self->cppobj = colourSpace;
                self->isconst = false;
                
                if(name) colourSpace->setName(name);
                if(family) colourSpace->setFamily(family);
                if(equalityGroup) colourSpace->setEqualityGroup(equalityGroup);
                if(description) colourSpace->setDescription(description);
                if(bitDepth) colourSpace->setBitDepth(BitDepthFromString(bitDepth));
                colourSpace->setIsData(isData); // TODO: Do not rely on the default value
                if(allocation) colourSpace->setAllocation(AllocationFromString(allocation));
                if(allocationVars)
                {
                    std::vector<float> vars;
                    if(!FillFloatVectorFromPySequence(allocationVars, vars))
                    {
                        PyErr_SetString(PyExc_TypeError, "allocationVars kwarg must be a float array.");
                        return -1;
                    }
                    colourSpace->setAllocationVars(static_cast<int>(vars.size()), &vars[0]);
                }
                if(toRefTransform)
                {
                    ConstTransformRcPtr transform = GetConstTransform(toRefTransform, true);
                    colourSpace->setTransform(transform, COLOURSPACE_DIR_TO_REFERENCE);
                }
                if(fromRefTransform)
                {
                    ConstTransformRcPtr transform = GetConstTransform(fromRefTransform, true);
                    colourSpace->setTransform(transform, COLOURSPACE_DIR_FROM_REFERENCE);
                }
                
                return 0;
            }
            catch ( const std::exception & e )
            {
                std::string message = "Cannot create colourSpace: ";
                message += e.what();
                PyErr_SetString( PyExc_RuntimeError, message.c_str() );
                return -1;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        void PyOCIO_ColourSpace_delete( PyOCIO_ColourSpace *self, PyObject * /*args*/ )
        {
            delete self->constcppobj;
            delete self->cppobj;
            
            self->ob_type->tp_free((PyObject*)self);
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColourSpace_isEditable( PyObject * self )
        {
            return PyBool_FromLong(IsPyColourSpaceEditable(self));
        }
        
        PyObject * PyOCIO_ColourSpace_createEditableCopy( PyObject * self )
        {
            try
            {
                ConstColourSpaceRcPtr colourSpace = GetConstColourSpace(self, true);
                ColourSpaceRcPtr copy = colourSpace->createEditableCopy();
                return BuildEditablePyColourSpace( copy );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColourSpace_getName( PyObject * self )
        {
            try
            {
                ConstColourSpaceRcPtr colourSpace = GetConstColourSpace(self, true);
                return PyString_FromString( colourSpace->getName() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColourSpace_setName( PyObject * self, PyObject * args )
        {
            try
            {
                char * name = 0;
                if (!PyArg_ParseTuple(args,"s:setName", &name)) return NULL;
                
                ColourSpaceRcPtr colourSpace = GetEditableColourSpace(self);
                colourSpace->setName( name );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColourSpace_getFamily( PyObject * self )
        {
            try
            {
                ConstColourSpaceRcPtr colourSpace = GetConstColourSpace(self, true);
                return PyString_FromString( colourSpace->getFamily() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColourSpace_setFamily( PyObject * self, PyObject * args )
        {
            try
            {
                char * name = 0;
                if (!PyArg_ParseTuple(args,"s:setFamily", &name)) return NULL;
                
                ColourSpaceRcPtr colourSpace = GetEditableColourSpace(self);
                colourSpace->setFamily( name );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColourSpace_getEqualityGroup( PyObject * self )
        {
            try
            {
                ConstColourSpaceRcPtr colourSpace = GetConstColourSpace(self, true);
                return PyString_FromString( colourSpace->getEqualityGroup() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColourSpace_setEqualityGroup( PyObject * self, PyObject * args )
        {
            try
            {
                char * name = 0;
                if (!PyArg_ParseTuple(args,"s:setEqualityGroup", &name)) return NULL;
                
                ColourSpaceRcPtr colourSpace = GetEditableColourSpace(self);
                colourSpace->setEqualityGroup( name );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColourSpace_getDescription( PyObject * self )
        {
            try
            {
                ConstColourSpaceRcPtr colourSpace = GetConstColourSpace(self, true);
                return PyString_FromString( colourSpace->getDescription() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColourSpace_setDescription( PyObject * self, PyObject * args )
        {
            try
            {
                char * name = 0;
                if (!PyArg_ParseTuple(args,"s:setDescription", &name)) return NULL;
                
                ColourSpaceRcPtr colourSpace = GetEditableColourSpace(self);
                colourSpace->setDescription( name );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColourSpace_getBitDepth( PyObject * self )
        {
            try
            {
                ConstColourSpaceRcPtr colourSpace = GetConstColourSpace(self, true);
                return PyString_FromString( BitDepthToString( colourSpace->getBitDepth()) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColourSpace_setBitDepth( PyObject * self, PyObject * args )
        {
            try
            {
                char * name = 0;
                if (!PyArg_ParseTuple(args,"s:setBitDepth", &name)) return NULL;
                
                ColourSpaceRcPtr colourSpace = GetEditableColourSpace(self);
                colourSpace->setBitDepth( BitDepthFromString( name ) );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColourSpace_isData( PyObject * self )
        {
            try
            {
                ConstColourSpaceRcPtr colourSpace = GetConstColourSpace(self, true);
                return PyBool_FromLong( colourSpace->isData() );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColourSpace_setIsData( PyObject * self, PyObject * args )
        {
            try
            {
                bool isData = false;
                if (!PyArg_ParseTuple(args,"O&:setIsData",
                    ConvertPyObjectToBool, &isData)) return NULL;
                
                ColourSpaceRcPtr colourSpace = GetEditableColourSpace(self);
                colourSpace->setIsData( isData );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColourSpace_getAllocation( PyObject * self )
        {
            try
            {
                ConstColourSpaceRcPtr colourSpace = GetConstColourSpace(self, true);
                return PyString_FromString( AllocationToString( colourSpace->getAllocation()) );
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColourSpace_setAllocation( PyObject * self, PyObject * args )
        {
            try
            {
                Allocation hwalloc;
                if (!PyArg_ParseTuple(args,"O&:setAllocation",
                    ConvertPyObjectToAllocation, &hwalloc)) return NULL;
                
                ColourSpaceRcPtr colourSpace = GetEditableColourSpace(self);
                colourSpace->setAllocation( hwalloc );
                
                Py_RETURN_NONE;
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColourSpace_getAllocationVars( PyObject * self )
        {
            try
            {
                ConstColourSpaceRcPtr colourSpace = GetConstColourSpace(self, true);
                
                std::vector<float> allocationvars(colourSpace->getAllocationNumVars());
                if(!allocationvars.empty())
                {
                    colourSpace->getAllocationVars(&allocationvars[0]);
                }
                
                return CreatePyListFromFloatVector(allocationvars);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        PyObject * PyOCIO_ColourSpace_setAllocationVars( PyObject * self, PyObject * args )
        {
            try
            {
                PyObject * pyvars = 0;
                if (!PyArg_ParseTuple(args,"O:setAllocationVars", &pyvars)) return NULL;
                
                std::vector<float> vars;
                if(!FillFloatVectorFromPySequence(pyvars, vars))
                {
                    PyErr_SetString(PyExc_TypeError, "First argument must be a float array.");
                    return 0;
                }
                
                ColourSpaceRcPtr colourSpace = GetEditableColourSpace(self);
                if(!vars.empty())
                {
                    colourSpace->setAllocationVars(static_cast<int>(vars.size()), &vars[0]);
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
        
        PyObject * PyOCIO_ColourSpace_getTransform( PyObject * self,  PyObject *args )
        {
            try
            {
                ColourSpaceDirection dir;
                if (!PyArg_ParseTuple(args,"O&:getTransform",
                    ConvertPyObjectToColourSpaceDirection, &dir)) return NULL;
                
                ConstColourSpaceRcPtr colourSpace = GetConstColourSpace(self, true);
                ConstTransformRcPtr transform = colourSpace->getTransform(dir);
                return BuildConstPyTransform(transform);
            }
            catch(...)
            {
                Python_Handle_Exception();
                return NULL;
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        
        PyObject * PyOCIO_ColourSpace_setTransform( PyObject * self,  PyObject *args )
        {
        
            try
            {
                PyObject * pytransform = 0;
                ColourSpaceDirection dir;
                if (!PyArg_ParseTuple(args,"OO&:setTransform", &pytransform,
                    ConvertPyObjectToColourSpaceDirection, &dir)) return NULL;
                
                ConstTransformRcPtr transform = GetConstTransform(pytransform, true);
                ColourSpaceRcPtr colourSpace = GetEditableColourSpace(self);
                colourSpace->setTransform(transform, dir);
                
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
