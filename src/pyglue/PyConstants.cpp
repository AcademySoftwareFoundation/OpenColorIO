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

OCIO_NAMESPACE_ENTER
{
    
    namespace
    {
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyObject * PyOCIO_Constants_GetInverseTransformDirection(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_Constants_CombineTransformDirections(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_Constants_BitDepthIsFloat(PyObject * self,  PyObject * args);
        PyObject * PyOCIO_Constants_BitDepthToInt(PyObject * self,  PyObject * args);
        
        ///////////////////////////////////////////////////////////////////////
        ///
        
        PyMethodDef LocalModuleMethods[] = {
            { "GetInverseTransformDirection",
            PyOCIO_Constants_GetInverseTransformDirection, METH_VARARGS, CONSTANTS_GETINVERSETRANSFORMDIRECTION__DOC__ },
            { "CombineTransformDirections",
            PyOCIO_Constants_CombineTransformDirections, METH_VARARGS, CONSTANTS_COMBINETRANSFORMDIRECTIONS__DOC__ },
            { "BitDepthIsFloat",
            PyOCIO_Constants_BitDepthIsFloat, METH_VARARGS, CONSTANTS_BITDEPTHISFLOAT__DOC__ },
            { "BitDepthToInt",
            PyOCIO_Constants_BitDepthToInt, METH_VARARGS, CONSTANTS_BITDEPTHTOINT__DOC__ },
            { NULL, NULL, 0, NULL } /* Sentinel */
        };
    
    }
    
    void AddConstantsModule(PyObject * enclosingModule)
    {
        // Add sub-module
        std::string moduleName = PyModule_GetName(enclosingModule);
        moduleName += ".Constants";
        

        PyObject * m;
        MOD_DEF(m, const_cast<char*>(moduleName.c_str()),
            CONSTANTS__DOC__, LocalModuleMethods);


        Py_INCREF(m);
        
        // Add Module Constants
        PyModule_AddStringConstant(m, "LOGGING_LEVEL_NONE",
            const_cast<char*>(LoggingLevelToString(LOGGING_LEVEL_NONE)));
        PyModule_AddStringConstant(m, "LOGGING_LEVEL_WARNING",
            const_cast<char*>(LoggingLevelToString(LOGGING_LEVEL_WARNING)));
        PyModule_AddStringConstant(m, "LOGGING_LEVEL_INFO",
            const_cast<char*>(LoggingLevelToString(LOGGING_LEVEL_INFO)));
        PyModule_AddStringConstant(m, "LOGGING_LEVEL_DEBUG",
            const_cast<char*>(LoggingLevelToString(LOGGING_LEVEL_DEBUG)));
        PyModule_AddStringConstant(m, "LOGGING_LEVEL_UNKNOWN",
            const_cast<char*>(LoggingLevelToString(LOGGING_LEVEL_UNKNOWN)));
        
        PyModule_AddStringConstant(m, "TRANSFORM_DIR_UNKNOWN",
            const_cast<char*>(TransformDirectionToString(TRANSFORM_DIR_UNKNOWN)));
        PyModule_AddStringConstant(m, "TRANSFORM_DIR_FORWARD",
            const_cast<char*>(TransformDirectionToString(TRANSFORM_DIR_FORWARD)));
        PyModule_AddStringConstant(m, "TRANSFORM_DIR_INVERSE",
            const_cast<char*>(TransformDirectionToString(TRANSFORM_DIR_INVERSE)));
        
        PyModule_AddStringConstant(m, "COLORSPACE_DIR_UNKNOWN",
            const_cast<char*>(ColorSpaceDirectionToString(COLORSPACE_DIR_UNKNOWN)));
        PyModule_AddStringConstant(m, "COLORSPACE_DIR_TO_REFERENCE",
            const_cast<char*>(ColorSpaceDirectionToString(COLORSPACE_DIR_TO_REFERENCE)));
        PyModule_AddStringConstant(m, "COLORSPACE_DIR_FROM_REFERENCE",
            const_cast<char*>(ColorSpaceDirectionToString(COLORSPACE_DIR_FROM_REFERENCE)));
        
        PyModule_AddStringConstant(m, "BIT_DEPTH_UNKNOWN",
            const_cast<char*>(BitDepthToString(BIT_DEPTH_UNKNOWN)));
        PyModule_AddStringConstant(m, "BIT_DEPTH_UINT8",
            const_cast<char*>(BitDepthToString(BIT_DEPTH_UINT8)));
        PyModule_AddStringConstant(m, "BIT_DEPTH_UINT10",
            const_cast<char*>(BitDepthToString(BIT_DEPTH_UINT10)));
        PyModule_AddStringConstant(m, "BIT_DEPTH_UINT12",
            const_cast<char*>(BitDepthToString(BIT_DEPTH_UINT12)));
        PyModule_AddStringConstant(m, "BIT_DEPTH_UINT14",
            const_cast<char*>(BitDepthToString(BIT_DEPTH_UINT14)));
        PyModule_AddStringConstant(m, "BIT_DEPTH_UINT16",
            const_cast<char*>(BitDepthToString(BIT_DEPTH_UINT16)));
        PyModule_AddStringConstant(m, "BIT_DEPTH_UINT32",
            const_cast<char*>(BitDepthToString(BIT_DEPTH_UINT32)));
        PyModule_AddStringConstant(m, "BIT_DEPTH_F16",
            const_cast<char*>(BitDepthToString(BIT_DEPTH_F16)));
        PyModule_AddStringConstant(m, "BIT_DEPTH_F32",
            const_cast<char*>(BitDepthToString(BIT_DEPTH_F32)));
        
        PyModule_AddStringConstant(m, "ALLOCATION_UNKNOWN",
            const_cast<char*>(AllocationToString(ALLOCATION_UNKNOWN)));
        PyModule_AddStringConstant(m, "ALLOCATION_UNIFORM",
            const_cast<char*>(AllocationToString(ALLOCATION_UNIFORM)));
        PyModule_AddStringConstant(m, "ALLOCATION_LG2",
            const_cast<char*>(AllocationToString(ALLOCATION_LG2)));
        
        PyModule_AddStringConstant(m, "INTERP_UNKNOWN",
            const_cast<char*>(InterpolationToString(INTERP_UNKNOWN)));
        PyModule_AddStringConstant(m, "INTERP_NEAREST",
            const_cast<char*>(InterpolationToString(INTERP_NEAREST)));
        PyModule_AddStringConstant(m, "INTERP_LINEAR",
            const_cast<char*>(InterpolationToString(INTERP_LINEAR)));
        PyModule_AddStringConstant(m, "INTERP_TETRAHEDRAL",
            const_cast<char*>(InterpolationToString(INTERP_TETRAHEDRAL)));
        PyModule_AddStringConstant(m, "INTERP_BEST",
            const_cast<char*>(InterpolationToString(INTERP_BEST)));
        
        PyModule_AddStringConstant(m, "GPU_LANGUAGE_UNKNOWN",
            const_cast<char*>(GpuLanguageToString(GPU_LANGUAGE_UNKNOWN)));
        PyModule_AddStringConstant(m, "GPU_LANGUAGE_CG",
            const_cast<char*>(GpuLanguageToString(GPU_LANGUAGE_CG)));
        PyModule_AddStringConstant(m, "GPU_LANGUAGE_GLSL_1_0",
            const_cast<char*>(GpuLanguageToString(GPU_LANGUAGE_GLSL_1_0)));
        PyModule_AddStringConstant(m, "GPU_LANGUAGE_GLSL_1_3",
            const_cast<char*>(GpuLanguageToString(GPU_LANGUAGE_GLSL_1_3)));
        
        PyModule_AddStringConstant(m, "ENV_ENVIRONMENT_UNKNOWN",
            const_cast<char*>(EnvironmentModeToString(ENV_ENVIRONMENT_UNKNOWN)));
        PyModule_AddStringConstant(m, "ENV_ENVIRONMENT_LOAD_PREDEFINED",
            const_cast<char*>(EnvironmentModeToString(ENV_ENVIRONMENT_LOAD_PREDEFINED)));
        PyModule_AddStringConstant(m, "ENV_ENVIRONMENT_LOAD_ALL",
            const_cast<char*>(EnvironmentModeToString(ENV_ENVIRONMENT_LOAD_ALL)));
        
        PyModule_AddStringConstant(m, "ROLE_DEFAULT", const_cast<char*>(ROLE_DEFAULT));
        PyModule_AddStringConstant(m, "ROLE_REFERENCE", const_cast<char*>(ROLE_REFERENCE));
        PyModule_AddStringConstant(m, "ROLE_DATA", const_cast<char*>(ROLE_DATA));
        PyModule_AddStringConstant(m, "ROLE_COLOR_PICKING", const_cast<char*>(ROLE_COLOR_PICKING));
        PyModule_AddStringConstant(m, "ROLE_SCENE_LINEAR", const_cast<char*>(ROLE_SCENE_LINEAR));
        PyModule_AddStringConstant(m, "ROLE_COMPOSITING_LOG", const_cast<char*>(ROLE_COMPOSITING_LOG));
        PyModule_AddStringConstant(m, "ROLE_COLOR_TIMING", const_cast<char*>(ROLE_COLOR_TIMING));
        PyModule_AddStringConstant(m, "ROLE_TEXTURE_PAINT", const_cast<char*>(ROLE_TEXTURE_PAINT));
        PyModule_AddStringConstant(m, "ROLE_MATTE_PAINT", const_cast<char*>(ROLE_MATTE_PAINT));
        
        // Add the module
        PyModule_AddObject(enclosingModule, "Constants", m);
        
    }
    
    namespace
    {
        
        PyObject * PyOCIO_Constants_GetInverseTransformDirection(PyObject * /*module*/, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* s = 0;
            if (!PyArg_ParseTuple(args, "s:GetInverseTransformDirection",
                &s)) return NULL;
            TransformDirection dir = TransformDirectionFromString(s);
            dir = GetInverseTransformDirection(dir);
            return PyString_FromString(TransformDirectionToString(dir));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Constants_CombineTransformDirections(PyObject * /*module*/, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* s1 = 0;
            char* s2 = 0;
            if (!PyArg_ParseTuple(args, "ss:CombineTransformDirections",
                &s1, &s2)) return NULL;
            TransformDirection dir1 = TransformDirectionFromString(s1);
            TransformDirection dir2 = TransformDirectionFromString(s2);
            TransformDirection dir = CombineTransformDirections(dir1, dir2);
            return PyString_FromString(TransformDirectionToString(dir));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Constants_BitDepthIsFloat(PyObject * /*module*/, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* s = 0;
            if (!PyArg_ParseTuple(args, "s:BitDepthIsFloat",
                &s)) return NULL;
            BitDepth bitdepth = BitDepthFromString(s);
            return PyBool_FromLong(BitDepthIsFloat(bitdepth));
            OCIO_PYTRY_EXIT(NULL)
        }
        
        PyObject * PyOCIO_Constants_BitDepthToInt(PyObject * /*module*/, PyObject * args)
        {
            OCIO_PYTRY_ENTER()
            char* s = 0;
            if (!PyArg_ParseTuple(args, "s:BitDepthToInt",
                &s)) return NULL;
            BitDepth bitdepth = BitDepthFromString(s);
            return PyInt_FromLong(BitDepthToInt(bitdepth));
            OCIO_PYTRY_EXIT(NULL)
        }
        
    }

}
OCIO_NAMESPACE_EXIT
