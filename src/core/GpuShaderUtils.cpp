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

#include "GpuShaderUtils.h"
#include "MathUtils.h"

#include <cmath>
#include <sstream>

OCIO_NAMESPACE_ENTER
{
    void Write_half4x4(std::ostream & os, const float * m44, GpuLanguage lang)
    {
        if(lang == GPU_LANGUAGE_CG)
        {
            os << "half4x4(";
            for(int i=0; i<16; i++)
            {
                if(i!=0) os << ", ";
                os << ClampToNormHalf(m44[i]);
            }
            os << ")";
        }
        else if(lang == GPU_LANGUAGE_GLSL_1_0 || lang == GPU_LANGUAGE_GLSL_1_3)
        {
            os << "mat4(";
            for(int i=0; i<16; i++)
            {
                if(i!=0) os << ", ";
                os << m44[i]; // Clamping to half is not necessary
            }
            os << ")";
        }
        else
        {
            throw Exception("Unsupported shader language.");
        }
    }
    
    void Write_half4(std::ostream & os, const float * v4,  GpuLanguage lang)
    {
        if(lang == GPU_LANGUAGE_CG)
        {
            os << "half4(";
            for(int i=0; i<4; i++)
            {
                if(i!=0) os << ", ";
                os << ClampToNormHalf(v4[i]);
            }
            os << ")";
        }
        else if(lang == GPU_LANGUAGE_GLSL_1_0 || lang == GPU_LANGUAGE_GLSL_1_3)
        {
            os << "vec4(";
            for(int i=0; i<4; i++)
            {
                if(i!=0) os << ", ";
                os << v4[i]; // Clamping to half is not necessary
            }
            os << ")";
        }
        else
        {
            throw Exception("Unsupported shader language.");
        }
    }
    
    void Write_half3(std::ostream & os, const float * v3,  GpuLanguage lang)
    {
        if(lang == GPU_LANGUAGE_CG)
        {
            os << "half3(";
            for(int i=0; i<3; i++)
            {
                if(i!=0) os << ", ";
                os << ClampToNormHalf(v3[i]);
            }
            os << ")";
        }
        else if(lang == GPU_LANGUAGE_GLSL_1_0 || lang == GPU_LANGUAGE_GLSL_1_3)
        {
            os << "vec3(";
            for(int i=0; i<3; i++)
            {
                if(i!=0) os << ", ";
                os << v3[i]; // Clamping to half is not necessary
            }
            os << ")";
        }
        else
        {
            throw Exception("Unsupported shader language.");
        }
    }
    
    
    
    
    std::string GpuTextHalf4x4(const float * m44, GpuLanguage lang)
    {
        std::ostringstream os;
        Write_half4x4(os, m44, lang);
        return os.str();
    }
    
    std::string GpuTextHalf4(const float * v4, GpuLanguage lang)
    {
        std::ostringstream os;
        Write_half4(os, v4, lang);
        return os.str();
    }
    
    std::string GpuTextHalf3(const float * v3, GpuLanguage lang)
    {
        std::ostringstream os;
        Write_half3(os, v3, lang);
        return os.str();
    }
    
    // Note that Cg and GLSL have opposite ordering for vec/mtx multiplication
    void Write_mtx_x_vec(std::ostream & os,
                         const std::string & mtx, const std::string & vec,
                         GpuLanguage lang)
    {
        if(lang == GPU_LANGUAGE_CG)
        {
            os << "mul( " << mtx << ", " << vec << ")";
        }
        else if(lang == GPU_LANGUAGE_GLSL_1_0 || lang == GPU_LANGUAGE_GLSL_1_3)
        {
            os << vec << " * " << mtx;
        }
        else
        {
            throw Exception("Unsupported shader language.");
        }
    }
    
    
    void Write_sampleLut3D_rgb(std::ostream & os, const std::string& variableName,
                               const std::string& lutName, int lut3DEdgeLen,
                               GpuLanguage lang)
    {
        float m = ((float) lut3DEdgeLen-1.0f) / (float) lut3DEdgeLen;
        float b = 1.0f / (2.0f * (float) lut3DEdgeLen);
        
        if(lang == GPU_LANGUAGE_CG)
        {
            os << "tex3D(";
            os << lutName << ", ";
            os << m << " * " << variableName << ".rgb + " << b << ").rgb;" << std::endl;
        }
        else if(lang == GPU_LANGUAGE_GLSL_1_0 || lang == GPU_LANGUAGE_GLSL_1_3)
        {
            os << "texture3D(";
            os << lutName << ", ";
            os << m << " * " << variableName << ".rgb + " << b << ").rgb;" << std::endl;
        }
        else
        {
            throw Exception("Unsupported shader language.");
        }
    }

}
OCIO_NAMESPACE_EXIT
