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
        else if(lang == GPU_LANGUAGE_GLES_2_0)
        {
            os << "mediump mat4(";
            for(int i=0; i<16; i++)
            {
                if(i!=0) os << ", ";
                os << ClampToNormHalf(m44[i]);
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
        else if(lang == GPU_LANGUAGE_GLES_2_0)
        {
            os << "mediump vec4(";
            for(int i=0; i<4; i++)
            {
                if(i!=0) os << ", ";
                os << ClampToNormHalf(v4[i]);
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
        else if(lang == GPU_LANGUAGE_GLES_2_0)
        {
            os << "mediump vec3(";
            for(int i=0; i<3; i++)
            {
                if(i!=0) os << ", ";
                os << ClampToNormHalf(v3[i]);
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
        else if(lang == GPU_LANGUAGE_GLSL_1_0 || lang == GPU_LANGUAGE_GLSL_1_3 ||
                lang == GPU_LANGUAGE_GLES_2_0)
        {
            os << vec << " * " << mtx;
        }
        else
        {
            throw Exception("Unsupported shader language.");
        }
    }

    namespace
    {
        void Write_half_decl(std::ostream & os, GpuLanguage lang)
        {
            if(lang == GPU_LANGUAGE_CG)
            {
                os << "half";
            }
            else if(lang == GPU_LANGUAGE_GLSL_1_0 || lang == GPU_LANGUAGE_GLSL_1_3)
            {
                os << "float";
            }
            else if(lang == GPU_LANGUAGE_GLES_2_0)
            {
                os << "mediump float";
            }
            else
            {
                throw Exception("Unsupported shader language.");
            }
        }

        void Write_half2_decl(std::ostream & os, GpuLanguage lang)
        {
            if(lang == GPU_LANGUAGE_CG)
            {
                os << "half2";
            }
            else if(lang == GPU_LANGUAGE_GLSL_1_0 || lang == GPU_LANGUAGE_GLSL_1_3)
            {
                os << "vec2";
            }
            else if(lang == GPU_LANGUAGE_GLES_2_0)
            {
                os << "mediump vec2";
            }
            else
            {
                throw Exception("Unsupported shader language.");
            }
        }

        void Write_half3_decl(std::ostream & os, GpuLanguage lang)
        {
            if(lang == GPU_LANGUAGE_CG)
            {
                os << "half3";
            }
            else if(lang == GPU_LANGUAGE_GLSL_1_0 || lang == GPU_LANGUAGE_GLSL_1_3)
            {
                os << "vec3";
            }
            else if(lang == GPU_LANGUAGE_GLES_2_0)
            {
                os << "mediump vec3";
            }
            else
            {
                throw Exception("Unsupported shader language.");
            }
        }

        void Write_texture2D(std::ostream & os, GpuLanguage lang,
                             const std::string &lutName,
                             const std::string &lookupVar)
        {
            if(lang == GPU_LANGUAGE_CG)
            {
                os << "tex2D(" << lutName << ", " << lookupVar << ")";
            }
            else if(lang == GPU_LANGUAGE_GLSL_1_0 || lang == GPU_LANGUAGE_GLSL_1_3 ||
                    lang == GPU_LANGUAGE_GLES_2_0)
            {
                os << "texture2D(" << lutName << ", " << lookupVar << ")";
            }
            else
            {
                throw Exception("Unsupported shader language.");
            }
        }
    }

    void Write_sampleLut3D_rgb(std::ostream & os,
                               const std::string & inputVariableName,
                               const std::string & outputVariableName,
                               const std::string & lutName, int lut3DEdgeLen,
                               GpuLanguage lang, bool lut3DEmulation)
    {
        float m = ((float) lut3DEdgeLen-1.0f) / (float) lut3DEdgeLen;
        float b = 1.0f / (2.0f * (float) lut3DEdgeLen);

        if (lut3DEmulation)
        {
            // looks a little ugly (3 is written as 3.0000000), but it guarantees
            // that numbers will be interpretted as floats by shading language.
            os.precision(8);
            os.setf(std::ios::fixed, std::ios::floatfield);

            float edgeLen = lut3DEdgeLen;
            float edgeLenMinusOne = lut3DEdgeLen-1.0f;
            float oneOverEdgeLen = 1.0f / lut3DEdgeLen;
            float oneOverEdgeLenMinusOne = 1.0f / edgeLenMinusOne;

            os << "\n// Emulating 3D texture using 2D texture\n\n";

            Write_half_decl(os, lang);
            os << " zIndex = min(floor(" << inputVariableName << ".b*" << edgeLen << "), " << edgeLenMinusOne << ");\n\n";
            
            Write_half2_decl(os, lang);
            os << " lookup;\n";
            os << "lookup.x = " << inputVariableName << ".r * " << m << " + " << b << ";\n";

            Write_half_decl(os, lang);
            os << " yLookup = (" << inputVariableName << ".g * " << m << " + " << b << ") * " << oneOverEdgeLen << ";\n\n";
        
            // faking 3d texture lookup (find lower and upper "z" tiles in y direction, and lerp between them)
            
            //calc zLerp: 0-1, based on distance between low and high z sections
            Write_half_decl(os, lang);
            os << " zLowStep = zIndex * " << oneOverEdgeLenMinusOne << ";\n";
            Write_half_decl(os, lang);
            os << " zHighStep = (zIndex + 1.0) * " << oneOverEdgeLenMinusOne << ";\n";
            
            ///// fit(inPixel.b, zLowStep, zHighStep, 0, 1)
            Write_half_decl(os, lang);
            os << " zLerp = (" << inputVariableName << ".b-zLowStep)/(zHighStep-zLowStep);\n\n";
            
            // z0Color
            Write_half_decl(os, lang);
            os << " z0Offset = zIndex * " << oneOverEdgeLen << ";\n";
            os << "lookup.y = yLookup + z0Offset;\n";
            Write_half3_decl(os, lang);
            os << " z0Color = ";
            Write_texture2D(os, lang, lutName, "lookup");
            os << ".rgb;\n\n";
            
            // z1Color
            Write_half_decl(os, lang);
            os << " z1Offset = min((zIndex + 1.0) * " << oneOverEdgeLen << ", ";
            os << edgeLenMinusOne*oneOverEdgeLen << ");\n";
            os << "lookup.y = yLookup + z1Offset;\n";
            Write_half3_decl(os, lang);
            os << " z1Color = ";
            Write_texture2D(os, lang, lutName, "lookup");
            os << ".rgb;\n\n";
            
            if (!outputVariableName.empty())
            {
                os << outputVariableName << ".rgb = mix(z0Color, z1Color, zLerp);\n";
            }
        }
        else
        {
            if (!outputVariableName.empty())
            {
                os << outputVariableName << ".rgb = ";
            }
        
            if(lang == GPU_LANGUAGE_CG)
            {
                os << "tex3D(";
                os << lutName << ", ";
                os << m << " * " << inputVariableName << ".rgb + " << b << ").rgb;" << std::endl;
            }
            else if(lang == GPU_LANGUAGE_GLSL_1_0 || lang == GPU_LANGUAGE_GLSL_1_3)
            {
                os << "texture3D(";
                os << lutName << ", ";
                os << m << " * " << inputVariableName << ".rgb + " << b << ").rgb;" << std::endl;
            }
            else
            {
                throw Exception("Unsupported feature (3D texture) for shader language.");
            }
        }
    }
}
OCIO_NAMESPACE_EXIT
