// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GPUSHADERUTILS_H
#define INCLUDED_OCIO_GPUSHADERUTILS_H

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{
// Helper class to create shader programs
class GpuShaderText
{

public:

    // Helper class to create shader lines
    class GpuShaderLine
    {
    public:
        GpuShaderLine() = delete;
        GpuShaderLine(const GpuShaderLine &) = default;
        ~GpuShaderLine();

        GpuShaderLine& operator<<(const char * str);
        GpuShaderLine& operator<<(float value);
        GpuShaderLine& operator<<(double value);
        GpuShaderLine& operator<<(unsigned value);
        GpuShaderLine& operator<<(int value);
        GpuShaderLine& operator<<(const std::string & str);
        GpuShaderLine& operator=(const GpuShaderLine & rhs);

        friend class GpuShaderText;

    private:
        explicit GpuShaderLine(GpuShaderText * text);

        // The ShaderText instance associated with this line.
        // Note: Not owned.
        GpuShaderText * m_text = nullptr;
    };

public:

    GpuShaderText() = delete;
    explicit GpuShaderText(GpuLanguage lang);

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
    // Scalar & arrays helper functions.
    //

    // Declare a float variable.
    void declareVar(const std::string & name, float v);

    // Declare a bool variable.
    void declareVar(const std::string & name, bool v);

    // Declare a float array variable.
    void declareFloatArrayConst(const std::string & name, int size, const float * v);

    // Declare a int array variable.
    void declareIntArrayConst(const std::string & name, int size, const int * v);

    //
    // Float2 helper functions
    //

    std::string float2Keyword() const;
    std::string float2Decl(const std::string& name) const;

    //
    // Float3 helper functions
    //

    // Get the keyword for declaring/using vectors with three elements
    std::string float3Keyword() const;
    // Get the string for creating constant vector with three elements
    std::string float3Const(float x, float y, float z) const;
    std::string float3Const(double x, double y, double z) const;
    // Get the string for creating constant vector with three elements
    std::string float3Const(const std::string& x, const std::string& y,
                           const std::string& z) const;
    // Get the string for creating constant vector with three elements
    std::string float3Const(float v) const;
    std::string float3Const(double v) const;
    // Get the string for creating constant vector with three elements
    std::string float3Const(const std::string& v) const;
    // Get the declaration for a vector with three elements
    std::string float3Decl(const std::string& name) const;

    // Declare and initialize a vector with three elements
    void declareFloat3(const std::string& name,
                      float x, float y, float z);
    void declareFloat3(const std::string& name, const Float3 & vec3);
    void declareFloat3(const std::string& name,
                      double x, double y, double z);
    // Declare and initialize a vector with three elements
    void declareFloat3(const std::string& name,
                      const std::string& x, const std::string& y, const std::string& z);

    //
    // Float4 helper functions
    //

    // Get the keyword for declaring/using vectors with four elements
    std::string float4Keyword() const;
    // Get the string for creating constant vector with four elements
    std::string float4Const(float x, float y, float z, float w) const;
    std::string float4Const(double x, double y, double z, double w) const;
    // Get the string for creating constant vector with four elements
    std::string float4Const(const std::string& x, const std::string& y,
                            const std::string& z, const std::string& w) const;
    // Get the string for creating constant vector with four elements
    std::string float4Const(float v) const;
    // Get the string for creating constant vector with four elements
    std::string float4Const(const std::string& v) const;
    // Get the declaration for a vector with four elements
    std::string float4Decl(const std::string& name) const;

    // Declare and initialize a vector with four elements
    void declareFloat4(const std::string& name,
                        float x, float y, float z, float w);
    void declareFloat4(const std::string& name,
                        double x, double y, double z, double w);
    void declareFloat4(const std::string& name,
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

    //
    // Uniform helpers
    //

    void declareUniformFloat(const std::string & uniformName);
    void declareUniformBool(const std::string & uniformName);
    void declareUniformFloat3(const std::string & uniformName);
    void declareUniformArrayFloat(const std::string & uniformName, unsigned int size);
    void declareUniformArrayInt(const std::string & uniformName, unsigned int size);

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

    // Get the string for creating a three or four-elements 'greater than' comparison
    //    Each element i in the resulting vector is 1 if a>b, or 0 otherwise.
    std::string float3GreaterThan(const std::string& a, const std::string& b) const;
    std::string float4GreaterThan(const std::string& a, const std::string& b) const;

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

    std::string floatKeyword() const;
    std::string intKeyword() const;

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

// Create a resource name prepending the prefix of the shaderCreator to base.
std::string BuildResourceName(GpuShaderCreatorRcPtr & shaderCreator, const std::string & prefix,
                              const std::string & base);

//
// Math functions used by multiple GPU renderers.
//

// Convert scene-linear values to "grading log".
void AddLinToLogShader(GpuShaderText & st);
// Convert "grading log" values to scene-linear.
void AddLogToLinShader(GpuShaderText & st);

} // namespace OCIO_NAMESPACE

#endif
