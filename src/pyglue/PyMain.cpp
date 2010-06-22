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

#include <OpenColorSpace/OpenColorSpace.h>
namespace OCS = OCS_NAMESPACE;

#include "PyColorSpace.h"
#include "PyConfig.h"
#include "PyFileTransform.h"
#include "PyGroupTransform.h"
#include "PyUtil.h"

namespace
{
    PyObject * PyOCS_GetCurrentConfig(PyObject * /* self */)
    {
        try
        {
            return OCS::BuildConstPyConfig( OCS::GetCurrentConfig() );
        }
        catch(...)
        {
            OCS::Python_Handle_Exception();
            return NULL;
        }
    }
    
    PyObject * PyOCS_SetCurrentConfig(PyObject * /*self*/, PyObject * args)
    {
        try
        {
            PyObject * pyconfig;
            if (!PyArg_ParseTuple(args, "O!:SetCurrentConfig",
                &OCS::PyOCS_ConfigType, &pyconfig))
            {
                return NULL;
            }
            
            OCS::ConstConfigRcPtr c = OCS::GetConstConfig(pyconfig, true);
            OCS::SetCurrentConfig(c);
            
            Py_RETURN_NONE;
        }
        catch(...)
        {
            OCS::Python_Handle_Exception();
            return NULL;
        }
    }
    
    
    PyMethodDef PyOCS_methods[] = {
        {"GetCurrentConfig",
            (PyCFunction) PyOCS_GetCurrentConfig,
            METH_NOARGS,
            ""},
        
        {"SetCurrentConfig",
            (PyCFunction) PyOCS_SetCurrentConfig,
            METH_VARARGS,
            ""},
        
        {NULL, NULL, 0, NULL} /* Sentinel */
    };
}

extern "C"
PyMODINIT_FUNC
initPyOpenColorSpace(void)
{
    PyObject * m;
    
    m = Py_InitModule3("PyOpenColorSpace", PyOCS_methods, "OpenColorSpace API");
    
    OCS::AddColorSpaceObjectToModule( m );
    OCS::AddConfigObjectToModule( m );
    OCS::AddFileTransformObjectToModule( m );
    OCS::AddGroupTransformObjectToModule( m );
    
    // Add Module Constants
    PyModule_AddStringConstant(m, "TRANSFORM_DIR_UNKNOWN",
        OCS::TransformDirectionToString(OCS::TRANSFORM_DIR_UNKNOWN));
    PyModule_AddStringConstant(m, "TRANSFORM_DIR_FORWARD",
        OCS::TransformDirectionToString(OCS::TRANSFORM_DIR_FORWARD));
    PyModule_AddStringConstant(m, "TRANSFORM_DIR_INVERSE",
        OCS::TransformDirectionToString(OCS::TRANSFORM_DIR_INVERSE));
    
    PyModule_AddStringConstant(m, "COLORSPACE_DIR_UNKNOWN",
        OCS::ColorSpaceDirectionToString(OCS::COLORSPACE_DIR_UNKNOWN));
    PyModule_AddStringConstant(m, "COLORSPACE_DIR_TO_REFERENCE",
        OCS::ColorSpaceDirectionToString(OCS::COLORSPACE_DIR_TO_REFERENCE));
    PyModule_AddStringConstant(m, "COLORSPACE_DIR_FROM_REFERENCE",
        OCS::ColorSpaceDirectionToString(OCS::COLORSPACE_DIR_FROM_REFERENCE));
    
    PyModule_AddStringConstant(m, "BIT_DEPTH_UNKNOWN",
        OCS::BitDepthToString(OCS::BIT_DEPTH_UNKNOWN));
    PyModule_AddStringConstant(m, "BIT_DEPTH_UINT8",
        OCS::BitDepthToString(OCS::BIT_DEPTH_UINT8));
    PyModule_AddStringConstant(m, "BIT_DEPTH_UINT10",
        OCS::BitDepthToString(OCS::BIT_DEPTH_UINT10));
    PyModule_AddStringConstant(m, "BIT_DEPTH_UINT12",
        OCS::BitDepthToString(OCS::BIT_DEPTH_UINT12));
    PyModule_AddStringConstant(m, "BIT_DEPTH_UINT14",
        OCS::BitDepthToString(OCS::BIT_DEPTH_UINT14));
    PyModule_AddStringConstant(m, "BIT_DEPTH_UINT16",
        OCS::BitDepthToString(OCS::BIT_DEPTH_UINT16));
    PyModule_AddStringConstant(m, "BIT_DEPTH_UINT32",
        OCS::BitDepthToString(OCS::BIT_DEPTH_UINT32));
    PyModule_AddStringConstant(m, "BIT_DEPTH_F16",
        OCS::BitDepthToString(OCS::BIT_DEPTH_F16));
    PyModule_AddStringConstant(m, "BIT_DEPTH_F32",
        OCS::BitDepthToString(OCS::BIT_DEPTH_F32));
    
    PyModule_AddStringConstant(m, "GPU_ALLOCATION_UNKNOWN",
        OCS::GpuAllocationToString(OCS::GPU_ALLOCATION_UNKNOWN));
    PyModule_AddStringConstant(m, "GPU_ALLOCATION_UNIFORM",
        OCS::GpuAllocationToString(OCS::GPU_ALLOCATION_UNIFORM));
    PyModule_AddStringConstant(m, "GPU_ALLOCATION_LG2",
        OCS::GpuAllocationToString(OCS::GPU_ALLOCATION_LG2));
    
    PyModule_AddStringConstant(m, "INTERP_UNKNOWN",
        OCS::InterpolationToString(OCS::INTERP_UNKNOWN));
    PyModule_AddStringConstant(m, "INTERP_NEAREST",
        OCS::InterpolationToString(OCS::INTERP_NEAREST));
    PyModule_AddStringConstant(m, "INTERP_LINEAR",
        OCS::InterpolationToString(OCS::INTERP_LINEAR));
    
    PyModule_AddStringConstant(m, "ROLE_REFERENCE", OCS::ROLE_REFERENCE);
    PyModule_AddStringConstant(m, "ROLE_DATA", OCS::ROLE_DATA);
    PyModule_AddStringConstant(m, "ROLE_COLOR_PICKING", OCS::ROLE_COLOR_PICKING);
    PyModule_AddStringConstant(m, "ROLE_SCENE_LINEAR", OCS::ROLE_SCENE_LINEAR);
    PyModule_AddStringConstant(m, "ROLE_COMPOSITING_LOG", OCS::ROLE_COMPOSITING_LOG);
    PyModule_AddStringConstant(m, "ROLE_COLOR_TIMING", OCS::ROLE_COLOR_TIMING);
}
