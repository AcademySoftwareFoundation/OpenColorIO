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


#ifndef INCLUDED_OCIO_GPUSHADERUTILS_H
#define INCLUDED_OCIO_GPUSHADERUTILS_H

#include <OpenColorIO/OpenColorIO.h>

#include <sstream>


OCIO_NAMESPACE_ENTER
{
    // Helper class to create shader programs
    class GpuShaderText
    {

    public:

        // Helper class to create shader lines
        class GpuShaderLine
        {
        public:
            ~GpuShaderLine();

            GpuShaderLine& operator<<(const char * str);
            GpuShaderLine& operator<<(float value);
            GpuShaderLine& operator<<(double value);
            GpuShaderLine& operator<<(unsigned value);
            GpuShaderLine& operator<<(const std::string & str);
            GpuShaderLine& operator=(const GpuShaderLine & rhs);

            friend class GpuShaderText;

        private:

            explicit GpuShaderLine(GpuShaderText * text);

            GpuShaderLine();
            GpuShaderLine(const GpuShaderLine &);

            // The ShaderText instance associated with this line.
            GpuShaderText * m_text;
        };

    public:

        GpuShaderText(GpuLanguage lang);

        // Create a new GpuShaderLine instance and associate it with the GpuShaderText object.
        GpuShaderLine newLine();

        // Get the shader string produced so far
        std::string string() const;

        //
        // Indentation helper functions
        //

        void setIndent(unsigned indent);
        void indent();
        void dedent();

        //
        // Scalar helper functions
        //

        // Declare a float variable
        void declareVar(const std::string& name, float v);
        // Declare a float variable
        void declareVar(const std::string& name, const std::string& v);

        //
        // Vec2f helper functions
        //

        std::string vec2fKeyword() const;
        std::string vec2fDecl(const std::string& name) const;

        //
        // Vec3f helper functions
        //

        // Get the keyword for declaring/using vectors with three elements
        std::string vec3fKeyword() const;
        // Get the string for creating constant vector with three elements
        std::string vec3fConst(float x, float y, float z) const;
        std::string vec3fConst(double x, double y, double z) const;
        // Get the string for creating constant vector with three elements
        std::string vec3fConst(const std::string& x, const std::string& y,
                               const std::string& z) const;
        // Get the string for creating constant vector with three elements
        std::string vec3fConst(float v) const;
        std::string vec3fConst(double v) const;
        // Get the string for creating constant vector with three elements
        std::string vec3fConst(const std::string& v) const;
        // Get the declaration for a vector with three elements
        std::string vec3fDecl(const std::string& name) const;

        // Declare and initialize a vector with three elements
        void declareVec3f(const std::string& name,
                          float x, float y, float z);
        void declareVec3f(const std::string& name,
                          double x, double y, double z);
        // Declare and initialize a vector with three elements
        void declareVec3f(const std::string& name,
                          const std::string& x, const std::string& y, const std::string& z);

        //
        // Vec4f helper functions
        //

        // Get the keyword for declaring/using vectors with four elements
        std::string vec4fKeyword() const;
        // Get the string for creating constant vector with four elements
        std::string vec4fConst(float x, float y, float z, float w) const;
        std::string vec4fConst(double x, double y, double z, double w) const;
        // Get the string for creating constant vector with four elements
        std::string vec4fConst(const std::string& x, const std::string& y,
                               const std::string& z, const std::string& w) const;
        // Get the string for creating constant vector with four elements
        std::string vec4fConst(float v) const;
        // Get the string for creating constant vector with four elements
        std::string vec4fConst(const std::string& v) const;
        // Get the declaration for a vector with four elements
        std::string vec4fDecl(const std::string& name) const;

        // Declare and initialize a vector with four elements
        void declareVec4f(const std::string& name,
                          float x, float y, float z, float w);
        void declareVec4f(const std::string& name,
                          double x, double y, double z, double w);
        void declareVec4f(const std::string& name,
                          const std::string& x, const std::string& y,
                          const std::string& z, const std::string& w);

        //
        // Texture helpers
        //
        static std::string getSamplerName(const std::string& textureName);

        // Declare the global texture and sampler information for a 1D texture.
        void declareTex1D(const std::string& textureName);
        // Declare the global texture and sampler information for a 2D texture.
        void declareTex2D(const std::string& textureName);
        // Declare the global texture and sampler information for a 3D texture.
        void declareTex3D(const std::string& textureName);

        // Get the texture lookup call for a 1D texture.
        std::string sampleTex1D(const std::string& textureName, const std::string& coords) const;
        // Get the texture lookup call for a 2D texture.
        std::string sampleTex2D(const std::string& textureName, const std::string& coords) const;
        // Get the texture lookup call for a 3D texture.
        std::string sampleTex3D(const std::string& textureName, const std::string& coords) const;

        void declareUniformFloat(const std::string & uniformName);

        //
        // Matrix multiplication helpers
        //

        // Get the string for multiplying a 4x4 matrix and a four-element vector
        std::string mat4fMul(const float * m4x4, const std::string & vecName) const;
        std::string mat4fMul(const double * m4x4, const std::string & vecName) const;

        //
        // Special function helpers
        //

        // Get the string for linearly interpolating two quantities
        std::string lerp(const std::string& x, const std::string& y, 
                         const std::string& a) const;

        // Get the string for creating a four-element 'greater than' comparison.
        //    Each element i in the resulting vector is 1 if a>b, or 0 otherwise.
        std::string vec4fGreaterThan(const std::string& a, const std::string& b) const;

        // Get the string for taking the four-quadrant arctangent 
        // (similar to atan(y/x) but takes into account the signs of the arguments).
        std::string atan2(const std::string& y, const std::string& x) const;

        friend class GpuShaderLine;

    private:
        // Flush the current shader line to the shader text
        //    This includes the leading indentation and the trailing newline
        //    to the current line before adding it to the shader text, and
        //    resets the current line.
        void flushLine();

    private:
        // Shader language to use in the various shader text builder methods.
        GpuLanguage m_lang; 
        // String stream containing the current shader text.
        std::ostringstream m_ossText;

        // In order to avoid repeated allocations of an output string stream 
        // for multiple shader lines, create a single ostringstream instance 
        // on the shader text and just reset it after a line has  been added 
        // to the text. This should not pose a racing problem since we're only 
        // creating a single line at a time for a given shader text.
        
        // String stream containing the current shader line.
        std::ostringstream m_ossLine;

        // Indentation level to use for the next line.
        unsigned m_indent;
    };
}
OCIO_NAMESPACE_EXIT

#endif
