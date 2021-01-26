// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <math.h>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "MathUtils.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{
// This method converts a float/double to a string adding a dot when
// the float does not have a fractional part. Hence, it ensures
// that the shader understand that number as a float and not as an integer.
// 
// Note: The template selects the appropriate number of digits for either single 
// or double arguments to losslessly represent the value as a string.
// 
template<typename T>
std::string getFloatString(T v, GpuLanguage lang)
{
    static_assert(!std::numeric_limits<T>::is_integer, "Only floating point values");

    const T value = (lang == GPU_LANGUAGE_CG) ? (T)ClampToNormHalf(v) : v;

    T integerpart = (T)0;
    const T fracpart = std::modf(value, &integerpart);

    std::ostringstream oss;
    oss.precision(std::numeric_limits<T>::max_digits10);
    oss << value << ((fracpart == (T)0) && std::isfinite(value) ? "." : "");
    return oss.str();
}

template<int N>
std::string getVecKeyword(GpuLanguage lang)
{
    std::ostringstream kw;
    switch (lang)
    {
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        case GPU_LANGUAGE_GLSL_4_0:
        {
            kw << "vec" << N;
            break;
        }
        case GPU_LANGUAGE_CG:
        {
            kw << "half" << N;
            break;
        }
        case GPU_LANGUAGE_HLSL_DX11:
        {
            kw << "float" << N;
            break;
        }

        default:
        {
            throw Exception("Unknown GPU shader language.");
        }
    }
    return kw.str();
}

template<int N>
void getTexDecl(GpuLanguage lang,
                const std::string & textureName, 
                const std::string & samplerName,
                std::string & textureDecl, 
                std::string & samplerDecl)
{
    switch (lang)
    {
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        case GPU_LANGUAGE_CG:
        case GPU_LANGUAGE_GLSL_4_0:
        {
            textureDecl = "";

            std::ostringstream kw;
            kw << "uniform sampler" << N << "D " << samplerName << ";";
            samplerDecl = kw.str();
            break;
        }
        case GPU_LANGUAGE_HLSL_DX11:
        {
            std::ostringstream t;
            t << "Texture" << N << "D " << textureName << ";";
            textureDecl = t.str();

            t.str("");
            t << "SamplerState" << " " << samplerName << ";";
            samplerDecl = t.str();
            break;
        }

        default:
        {
            throw Exception("Unknown GPU shader language.");
        }
    }
}

template<int N>
std::string getTexSample(GpuLanguage lang,
                         const std::string & textureName,
                         const std::string & samplerName,
                         const std::string & coords)
{
    std::ostringstream kw;

    switch (lang)
    {
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        {
            kw << "texture" << N << "D(" << samplerName << ", " << coords << ")";
            break;
        }
        case GPU_LANGUAGE_CG:
        {
            kw << "tex" << N << "D(" << samplerName << ", " << coords << ")";
            break;
        }
        case GPU_LANGUAGE_HLSL_DX11:
        {
            kw << textureName << ".Sample(" << samplerName << ", " << coords << ")";
            break;
        }
        case GPU_LANGUAGE_GLSL_4_0:
        {
            kw << "texture(" << samplerName << ", " << coords << ")";
            break;
        }

        default:
        {
            throw Exception("Unknown GPU shader language.");
        }
    }

    return kw.str();
}

template<typename T, int N>
std::string getMatrixValues(const T * mtx, GpuLanguage lang, bool transpose)
{
    std::string vals;

    for(int i=0 ; i<N*N-1 ; ++i)
    {
        const int line = i/N;
        const int col = i%N;
        const int idx = transpose ? col*N+line : line*N+col;

        vals += getFloatString(mtx[idx], lang) + ", ";
    }
    vals += getFloatString(mtx[N*N-1], lang);

    return vals;
}

GpuShaderText::GpuShaderLine::GpuShaderLine(GpuShaderText * text)
    :   m_text(text)
{
}

GpuShaderText::GpuShaderLine::~GpuShaderLine()
{
    if(m_text)
    {
        m_text->flushLine();
    }
}

GpuShaderText::GpuShaderLine& GpuShaderText::GpuShaderLine::operator<<(const char* str)
{
    if (str)
    {
        m_text->m_ossLine << str;
    }
    return *this;
}

GpuShaderText::GpuShaderLine& GpuShaderText::GpuShaderLine::operator<<(float value)
{
    m_text->m_ossLine << getFloatString(value, m_text->m_lang);
    return *this;
}

GpuShaderText::GpuShaderLine& GpuShaderText::GpuShaderLine::operator<<(double value)
{
    m_text->m_ossLine << getFloatString(value, m_text->m_lang);
    return *this;
}

GpuShaderText::GpuShaderLine& GpuShaderText::GpuShaderLine::operator<<(unsigned value)
{
    m_text->m_ossLine << value;
    return *this;
}

GpuShaderText::GpuShaderLine& GpuShaderText::GpuShaderLine::operator<<(int value)
{
    m_text->m_ossLine << value;
    return *this;
}

GpuShaderText::GpuShaderLine& GpuShaderText::GpuShaderLine::operator<<(const std::string& str)
{
    m_text->m_ossLine << str;
    return *this;
}

GpuShaderText::GpuShaderLine& GpuShaderText::GpuShaderLine::operator=(const GpuShaderText::GpuShaderLine& rhs)
{
    if (this != &rhs)
    {
        m_text = rhs.m_text;
    }
    return *this;
}

GpuShaderText::GpuShaderText(GpuLanguage lang)
    :   m_lang(lang)
    ,   m_indent(0)
{
    m_ossText.precision(16);
    m_ossLine.precision(16);
}

void GpuShaderText::setIndent(unsigned i)
{
    m_indent = i;
}

void GpuShaderText::indent()
{
    m_indent++;
}

void GpuShaderText::dedent()
{
    m_indent--;
}

GpuShaderText::GpuShaderLine GpuShaderText::newLine()
{
    return GpuShaderText::GpuShaderLine(this);
}

std::string GpuShaderText::string() const
{
    return m_ossText.str();
}

void GpuShaderText::flushLine()
{
    static const unsigned tabSize = 2;

    m_ossText << std::string(tabSize * m_indent, ' ')
              << m_ossLine.str()
              << std::endl;

    m_ossLine.str("");
    m_ossLine.clear();
}

std::string GpuShaderText::floatKeyword() const
{
    return (m_lang == GPU_LANGUAGE_CG ? "half" : "float");
}

std::string GpuShaderText::intKeyword() const
{
    return "int";
}

void GpuShaderText::declareVar(const std::string & name, float v)
{
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }
    newLine() << floatKeyword()
              << " " << name << " = " << getFloatString(v, m_lang) << ";";
}

void GpuShaderText::declareVar(const std::string & name, bool v)
{
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }
    newLine() << "bool " << name << " = " << (v ? "true;" : "false;");
}

void GpuShaderText::declareFloatArrayConst(const std::string & name, int size, const float * v)
{
    if (size == 0)
    {
        throw Exception("GPU array size is 0.");
    }
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }

    auto nl = newLine();
    if (m_lang != GPU_LANGUAGE_HLSL_DX11)
    {
        nl << "const " << floatKeyword() << " " << name << "[" << size << "]" << " = ";
        nl << floatKeyword() << "[" << size << "](";
        for (int i = 0; i < size; ++i)
        {
            nl << getFloatString(v[i], m_lang);
            if (i + 1 != size)
            {
                nl << ", ";
            }
        }
        nl << ");";
    }
    else
    {
        nl << "vector<float, "<< size << "> " << name << " = {";
        for (int i = 0; i < size; ++i)
        {
            nl << getFloatString(v[i], m_lang);
            if (i + 1 != size)
            {
                nl << ", ";
            }
        }
        nl << "};";
    }
}

void GpuShaderText::declareIntArrayConst(const std::string & name, int size, const int * v)
{
    if (size == 0)
    {
        throw Exception("GPU array size is 0.");
    }
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }

    auto nl = newLine();
    if (m_lang != GPU_LANGUAGE_HLSL_DX11)
    {
        nl << "const " << intKeyword() << " " << name << "[" << size << "]"
           << " = " << intKeyword() << "[" << size << "](";
        for (int i = 0; i < size; ++i)
        {
            nl << v[i];
            if (i + 1 != size)
            {
                nl << ", ";
            }
        }
        nl << ");";
    }
    else
    {
        nl << "vector<" << intKeyword() << ", " << size << "> " << name << " = {";
        for (int i = 0; i < size; ++i)
        {
            nl << v[i];
            if (i + 1 != size)
            {
                nl << ", ";
            }
        }
        nl << "};";
    }
}

std::string GpuShaderText::float2Keyword() const
{
    return getVecKeyword<2>(m_lang);
}

std::string GpuShaderText::float2Decl(const std::string & name) const
{
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }

    return float2Keyword() + " " + name;
}

std::string GpuShaderText::float3Keyword() const
{
    return getVecKeyword<3>(m_lang);
}

std::string GpuShaderText::float3Const(float x, float y, float z) const
{
    return float3Const(getFloatString(x, m_lang),
                      getFloatString(y, m_lang), 
                      getFloatString(z, m_lang));
}

std::string GpuShaderText::float3Const(double x, double y, double z) const
{
    return float3Const(getFloatString(x, m_lang),
                      getFloatString(y, m_lang), 
                      getFloatString(z, m_lang));
}

std::string GpuShaderText::float3Const(const std::string& x,
                                      const std::string& y,
                                      const std::string& z) const
{
    std::ostringstream kw;
    kw << float3Keyword() << "(" << x << ", " << y << ", " << z << ")";
    return kw.str();
}

std::string GpuShaderText::float3Const(const float v) const
{
    return float3Const(getFloatString(v, m_lang));
}

std::string GpuShaderText::float3Const(const double v) const
{
    return float3Const(getFloatString(v, m_lang));
}

std::string GpuShaderText::float3Const(const std::string & v) const
{
    return float3Const(v, v, v);
}

std::string GpuShaderText::float3Decl(const std::string &  name) const
{
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }

    return float3Keyword() + " " + name;
}

void GpuShaderText::declareFloat3(const std::string & name,
                                 float x, float y, float z)
{
    declareFloat3(name, getFloatString(x, m_lang), 
                       getFloatString(y, m_lang), 
                       getFloatString(z, m_lang));
}

void GpuShaderText::declareFloat3(const std::string& name, const Float3 & vec3)
{
    declareFloat3(name, vec3[0], vec3[1], vec3[2]);
}

void GpuShaderText::declareFloat3(const std::string & name, 
                                 double x, double y, double z)
{
    declareFloat3(name, getFloatString(x, m_lang), 
                       getFloatString(y, m_lang), 
                       getFloatString(z, m_lang));
}

void GpuShaderText::declareFloat3(const std::string & name, 
                                 const std::string & x,
                                 const std::string & y,
                                 const std::string & z)
{
    newLine() << float3Decl(name) << " = " << float3Const(x, y, z) << ";";
}

std::string GpuShaderText::float4Keyword() const
{
    return getVecKeyword<4>(m_lang);
}

std::string GpuShaderText::float4Const(const float x, const float y, const float z, const float w) const
{
    return float4Const(getFloatString(x, m_lang),
                      getFloatString(y, m_lang), 
                      getFloatString(z, m_lang),
                      getFloatString(w, m_lang));
}

std::string GpuShaderText::float4Const(const double x, const double y, const double z, const double w) const
{
    return float4Const(getFloatString(x, m_lang),
                      getFloatString(y, m_lang), 
                      getFloatString(z, m_lang),
                      getFloatString(w, m_lang));
}

std::string GpuShaderText::float4Const(const std::string& x,
                                      const std::string& y,
                                      const std::string& z,
                                      const std::string& w) const
{
    std::ostringstream kw;
    kw << float4Keyword() << "("
       << x << ", "
       << y << ", "
       << z << ", "
       << w << ")";

    return kw.str();
}

std::string GpuShaderText::float4Const(const float v) const
{
    return float4Const(getFloatString(v, m_lang));
}

std::string GpuShaderText::float4Const(const std::string & v) const
{
    return float4Const(v, v, v, v);
}

std::string GpuShaderText::float4Decl(const std::string & name) const
{
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }

    return float4Keyword() + " " + name;
}

void GpuShaderText::declareFloat4(const std::string & name,
                                  const float x,
                                  const float y, 
                                  const float z,
                                  const float w)
{
    declareFloat4(name, getFloatString(x, m_lang), 
                        getFloatString(y, m_lang), 
                        getFloatString(z, m_lang),
                        getFloatString(w, m_lang));
}

void GpuShaderText::declareFloat4(const std::string & name,
                                  const double x,
                                  const double y, 
                                  const double z,
                                  const double w)
{
    declareFloat4(name, getFloatString(x, m_lang), 
                        getFloatString(y, m_lang), 
                        getFloatString(z, m_lang),
                        getFloatString(w, m_lang));
}

void GpuShaderText::declareFloat4(const std::string & name,
                                  const std::string & x, 
                                  const std::string & y,
                                  const std::string & z,
                                  const std::string & w)
{
    newLine() << float4Decl(name) << " = " << float4Const(x, y, z, w) << ";";
}

std::string GpuShaderText::getSamplerName(const std::string& textureName)
{
    return textureName + "Sampler";
}

void GpuShaderText::declareTex1D(const std::string & textureName)
{
    std::string textureDecl, samplerDecl;
    getTexDecl<1>(m_lang, textureName, getSamplerName(textureName), textureDecl, samplerDecl);

    if (!textureDecl.empty())
    {
        newLine() << textureDecl;
    }

    if (!samplerDecl.empty())
    {
        newLine() << samplerDecl;
    }
}

void GpuShaderText::declareTex2D(const std::string & textureName)
{
    std::string textureDecl, samplerDecl;
    getTexDecl<2>(m_lang, textureName, getSamplerName(textureName), textureDecl, samplerDecl);

    if (!textureDecl.empty())
    {
        newLine() << textureDecl;
    }

    if (!samplerDecl.empty())
    {
        newLine() << samplerDecl;
    }
}

void GpuShaderText::declareTex3D(const std::string& textureName)
{
    std::string textureDecl, samplerDecl;
    getTexDecl<3>(m_lang, textureName, getSamplerName(textureName), textureDecl, samplerDecl);

    if (!textureDecl.empty())
    {
        newLine() << textureDecl;
    }

    if (!samplerDecl.empty())
    {
        newLine() << samplerDecl;
    }
}

std::string GpuShaderText::sampleTex1D(const std::string& textureName, 
                                        const std::string& coords) const
{
    return getTexSample<1>(m_lang, textureName, getSamplerName(textureName), coords);
}

std::string GpuShaderText::sampleTex2D(const std::string& textureName, 
                                        const std::string& coords) const
{
    return getTexSample<2>(m_lang, textureName, getSamplerName(textureName), coords);
}

std::string GpuShaderText::sampleTex3D(const std::string& textureName,
                                        const std::string& coords) const
{
    return getTexSample<3>(m_lang, textureName, getSamplerName(textureName), coords);
}


void GpuShaderText::declareUniformFloat(const std::string & uniformName)
{
    newLine() << "uniform " << floatKeyword() << " " << uniformName << ";";
}

void GpuShaderText::declareUniformBool(const std::string & uniformName)
{
    newLine() << "uniform bool " << uniformName << ";";
}

void GpuShaderText::declareUniformFloat3(const std::string & uniformName)
{
    newLine() << "uniform " << float3Keyword() << " " << uniformName << ";";
}

void GpuShaderText::declareUniformArrayFloat(const std::string & uniformName, unsigned int size)
{
    newLine() << "uniform " << floatKeyword() << " " << uniformName << "[" << size << "];";
}

void GpuShaderText::declareUniformArrayInt(const std::string & uniformName, unsigned int size)
{
    newLine() << "uniform " << intKeyword() << " " << uniformName << "[" << size << "];";
}

// Keep the method private as only float & double types are expected
template<typename T>
std::string matrix4Mul(const T * m4x4, const std::string & vecName, GpuLanguage lang)
{
    if (vecName.empty())
    {
        throw Exception("GPU variable name is empty.");
    }

    std::ostringstream kw;
    switch (lang)
    {
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        case GPU_LANGUAGE_GLSL_4_0:
        {
            // OpenGL shader program requests a transposed matrix
            kw << "mat4(" 
                << getMatrixValues<T, 4>(m4x4, lang, true) << ") * " << vecName;
            break;
        }
        case GPU_LANGUAGE_CG:
        {
            kw << "mul(half4x4(" 
                << getMatrixValues<T, 4>(m4x4, lang, false) << "), " << vecName << ")";
            break;
        }
        case GPU_LANGUAGE_HLSL_DX11:
        {
            kw << "mul(" << vecName 
                << ", float4x4(" << getMatrixValues<T, 4>(m4x4, lang, true) << "))";
            break;
        }

        default:
        {
            throw Exception("Unknown GPU shader language.");
        }
    }
    return kw.str();
}

std::string GpuShaderText::mat4fMul(const float * m4x4, 
                                    const std::string & vecName) const
{
    return matrix4Mul<float>(m4x4, vecName, m_lang);
}

std::string GpuShaderText::mat4fMul(const double * m4x4, 
                                    const std::string & vecName) const
{
    return matrix4Mul<double>(m4x4, vecName, m_lang);
}

std::string GpuShaderText::lerp(const std::string & x, 
                                const std::string & y, 
                                const std::string & a) const
{
    std::ostringstream kw;
    switch (m_lang)
    {
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        case GPU_LANGUAGE_GLSL_4_0:
        {
            kw << "mix(" << x << ", " << y << ", " << a << ")";
            break;
        }
        case GPU_LANGUAGE_CG:
        case GPU_LANGUAGE_HLSL_DX11:
        {
            kw << "lerp(" << x << ", " << y << ", " << a << ")";
            break;
        }

        default:
        {
            throw Exception("Unknown GPU shader language.");
        }
    }
    return kw.str();
}

std::string GpuShaderText::float3GreaterThan(const std::string & a,
                                             const std::string & b) const
{
    std::ostringstream kw;
    switch (m_lang)
    {
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        case GPU_LANGUAGE_GLSL_4_0:
        case GPU_LANGUAGE_CG:
        {
            kw << float3Keyword() << "(greaterThan( " << a << ", " << b << "))";
            break;
        }
        case GPU_LANGUAGE_HLSL_DX11:
        {
            kw << float3Keyword() << "((" << a << " > " << b << ") ? "
                << float3Const(1.0f) << " : " << float3Const(0.0f) << ")";
            break;
        }

        default:
        {
            throw Exception("Unknown GPU shader language.");
        }
    }
    return kw.str();
}

std::string GpuShaderText::float4GreaterThan(const std::string & a,
                                             const std::string & b) const
{
    std::ostringstream kw;
    switch (m_lang)
    {
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        case GPU_LANGUAGE_GLSL_4_0:
        case GPU_LANGUAGE_CG:
        {
            kw << float4Keyword() << "(greaterThan( " << a << ", " << b << "))";
            break;
        }
        case GPU_LANGUAGE_HLSL_DX11:
        {
            kw << float4Keyword() << "((" << a << " > " << b << ") ? "
                << float4Const(1.0f) << " : " << float4Const(0.0f) << ")";
            break;
        }

        default:
        {
            throw Exception("Unknown GPU shader language.");
        }
    }
    return kw.str();
}

std::string GpuShaderText::atan2(const std::string & y,
                                 const std::string & x) const
{
    std::ostringstream kw;
    switch(m_lang)
    {
        case GPU_LANGUAGE_CG:
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        case GPU_LANGUAGE_GLSL_4_0:
        {
            // note: "atan" not "atan2"
            kw << "atan(" << y << ", " << x << ")";
            break;
        }
        case GPU_LANGUAGE_HLSL_DX11:
        {
            // note: operand order is swapped in HLSL
            kw << "atan2(" << x << ", " << y << ")";
            break;
        }

        default:
        {
            throw Exception("Unknown GPU shader language.");
        }
    }
    return kw.str();
}

std::string BuildResourceName(GpuShaderCreatorRcPtr & shaderCreator, const std::string & prefix,
                              const std::string & base)
{
    std::string name = shaderCreator->getResourcePrefix();
    name += "_";
    name += prefix;
    name += "_";
    name += base;

    // Note: Remove potentially problematic double underscores from GLSL resource names.
    StringUtils::ReplaceInPlace(name, "__", "_");
    return name;
}

//
// Convert scene-linear values to "grading log".  Grading Log is in units of F-Stops
// with 0 being 18% grey.  Above about -5, it is pretty much exactly F-Stops but below
// that it is a pseudo-log so that 0.0 is at -7 stops rather than -Inf (as in a pure log).
//
void AddLinToLogShader(GpuShaderText & st)
{
    st.newLine() << "{";   // establish scope so local variable names won't conflict
    st.indent();
    st.newLine() << "const float xbrk = 0.0041318374739483946;";
    st.newLine() << "const float shift = -0.000157849851665374;";
    st.newLine() << "const float m = 1. / (0.18 + shift);";
    st.newLine() << "const float base2 = 1.4426950408889634;";  // 1/log(2)
    st.newLine() << "const float gain = 363.034608563;";
    st.newLine() << "const float offs = -7.;";
    st.newLine() << st.float3Decl("ylin") << " = outColor.rgb * gain + offs;";
    st.newLine() << st.float3Decl("ylog") << " = base2 * log( ( outColor.rgb + shift ) * m );";
    st.newLine() << "outColor.r = (outColor.r < xbrk) ? ylin.r : ylog.r;";
    st.newLine() << "outColor.g = (outColor.g < xbrk) ? ylin.g : ylog.g;";
    st.newLine() << "outColor.b = (outColor.b < xbrk) ? ylin.b : ylog.b;";
    st.dedent();
    st.newLine() << "}";
}

void AddLogToLinShader(GpuShaderText & st)
{
    st.newLine() << "{";   // establish scope so local variable names won't conflict
    st.indent();
    st.newLine() << "const float ybrk = -5.5;";
    st.newLine() << "const float shift = -0.000157849851665374;";
    st.newLine() << "const float gain = 363.034608563;";
    st.newLine() << "const float offs = -7.;";
    st.newLine() << st.float3Decl("xlin") << " = (outColor.rgb - offs) / gain;";
    st.newLine() << st.float3Decl("xlog") << " = pow( " << st.float3Const(2.0f)
                                          <<        ", outColor.rgb ) * (0.18 + shift) - shift;";
    st.newLine() << "outColor.r = (outColor.r < ybrk) ? xlin.r : xlog.r;";
    st.newLine() << "outColor.g = (outColor.g < ybrk) ? xlin.g : xlog.g;";
    st.newLine() << "outColor.b = (outColor.b < ybrk) ? xlin.b : xlog.b;";
    st.dedent();
    st.newLine() << "}";
}

} // namespace OCIO_NAMESPACE

