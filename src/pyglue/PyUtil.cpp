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


#include <OpenColorIO/OpenColorIO.h>

#include "PyFileTransform.h"
#include "PyGroupTransform.h"
#include "PyUtil.h"

#include <sstream>

OCIO_NAMESPACE_ENTER
{
    PyObject * BuildConstPyTransform(ConstTransformRcPtr transform)
    {
        if(ConstGroupTransformRcPtr groupTransform = \
            DynamicPtrCast<const GroupTransform>(transform))
        {
            return BuildConstPyGroupTransform(groupTransform);
        }
        else if(ConstFileTransformRcPtr fileTransform = \
            DynamicPtrCast<const FileTransform>(transform))
        {
            return BuildConstPyFileTransform(fileTransform);
        }
        else
        {
            std::ostringstream os;
            os << "Unknown transform type for BuildConstPyTransform.";
            throw Exception(os.str().c_str());
        }
    }
    
    PyObject * BuildEditablePyTransform(TransformRcPtr transform)
    {
        if(GroupTransformRcPtr groupTransform = \
            DynamicPtrCast<GroupTransform>(transform))
        {
            return BuildEditablePyGroupTransform(groupTransform);
        }
        else if(FileTransformRcPtr fileTransform = \
            DynamicPtrCast<FileTransform>(transform))
        {
            return BuildEditablePyFileTransform(fileTransform);
        }
        else
        {
            std::ostringstream os;
            os << "Unknown transform type for BuildEditablePyTransform.";
            throw Exception(os.str().c_str());
        }
    }
    
    bool IsPyTransform(PyObject * pyobject)
    {
        return ( IsPyGroupTransform(pyobject) ||
                 IsPyFileTransform(pyobject) );
    }
    
    ConstTransformRcPtr GetConstTransform(PyObject * pyobject, bool allowCast)
    {
        if(IsPyGroupTransform(pyobject))
        {
            return GetConstGroupTransform(pyobject, allowCast);
        }
        else if(IsPyFileTransform(pyobject))
        {
            return GetConstFileTransform(pyobject, allowCast);
        }
        else
        {
            throw Exception("PyObject must be a known OCIO::Transform type.");
        }
    }


    // http://docs.python.org/c-api/object.html#PyObject_IsTrue
    int ConvertPyObjectToBool(PyObject *object, void *valuePtr)
    {
        bool *boolPtr = static_cast<bool*>(valuePtr);
        int status = PyObject_IsTrue(object);
        
        if (status == -1 || PyErr_Occurred())
        {
            if (!PyErr_Occurred())
            {
                PyErr_SetString(PyExc_ValueError, "could not convert object to bool.");
            }
            
            return 0;
        }

        *boolPtr = (status == 1) ? true : false;

        return 1;
    }
    
    int ConvertPyObjectToGpuAllocation(PyObject *object, void *valuePtr)
    {
        GpuAllocation* gpuallocPtr = static_cast<GpuAllocation*>(valuePtr);
        
        if(!PyString_Check(object))
        {
            PyErr_SetString(PyExc_ValueError, "Object is not a string.");
            return 0;
        }
        
        *gpuallocPtr = OCIO::GpuAllocationFromString(PyString_AsString( object ));
        
        return 1;
    }
    
    
    
    int ConvertPyObjectToInterpolation(PyObject *object, void *valuePtr)
    {
        Interpolation* interpPtr = static_cast<Interpolation*>(valuePtr);
        
        if(!PyString_Check(object))
        {
            PyErr_SetString(PyExc_ValueError, "Object is not a string.");
            return 0;
        }
        
        *interpPtr = OCIO::InterpolationFromString(PyString_AsString( object ));
        
        return 1;
    }
    
    int ConvertPyObjectToTransformDirection(PyObject *object, void *valuePtr)
    {
        TransformDirection* dirPtr = static_cast<TransformDirection*>(valuePtr);
        
        if(!PyString_Check(object))
        {
            PyErr_SetString(PyExc_ValueError, "Object is not a string.");
            return 0;
        }
        
        *dirPtr = OCIO::TransformDirectionFromString(PyString_AsString( object ));
        
        return 1;
    }
    
    
    int ConvertPyObjectToColorSpaceDirection(PyObject *object, void *valuePtr)
    {
        ColorSpaceDirection* dirPtr = static_cast<ColorSpaceDirection*>(valuePtr);
        
        if(!PyString_Check(object))
        {
            PyErr_SetString(PyExc_ValueError, "Object is not a string.");
            return 0;
        }
        
        *dirPtr = OCIO::ColorSpaceDirectionFromString(PyString_AsString( object ));
        
        return 1;
    }
    

    
    /* See the header for the justification for this function.
    
    The trick to making this technique work is that we know
    we've been called from within a catch block, so there
    is an exception on the stack.  We can re-throw this
    exception, using the throw statement.  By doing this
    inside a try...catch block, it's possible to use the
    standard catch mechanism to categorize whatever exception
    was actually caught by the caller.
    */
    
    void Python_Handle_Exception()
    {
        try
        {
            // Re-throw whatever exception is already on the stack.
            // This will fail horribly if no exception is already
            // on the stack, so this function must only be called
            // from inside an exception handler catch block!
            throw;
        }
        catch (Exception & e)
        {
            PyErr_SetString(PyExc_RuntimeError, e.what());
        }
        catch (std::exception& e)
        {
            PyErr_SetString(PyExc_RuntimeError, e.what());
        }
        catch (...)
        {
            PyErr_SetString(PyExc_RuntimeError, "Unknown C++ exception caught.");
        }
    }
}
OCIO_NAMESPACE_EXIT
