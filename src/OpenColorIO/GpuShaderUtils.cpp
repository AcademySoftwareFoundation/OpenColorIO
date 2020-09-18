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

        case GPU_LANGUAGE_UNKNOWN:
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

        case GPU_LANGUAGE_UNKNOWN:
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

        case GPU_LANGUAGE_UNKNOWN:
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

std::string GpuShaderText::int2Keyword() const
{
    return (m_lang == GPU_LANGUAGE_HLSL_DX11 ? "int2" : "ivec2");
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

void GpuShaderText::declareInt2ArrayConst(const std::string & name, int size, const int * v)
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
        nl << "const " << int2Keyword() << " " << name << "[" << size << "]" << " = " << int2Keyword() << "[" << size << "](";
        for (int i = 0; i < size; ++i)
        {
            nl << int2Keyword() << "(" << v[2 * i] << ", " << v[2 * i + 1] << ")";
            if (i + 1 != size)
            {
                nl << ", ";
            }
        }
        nl << ");";
    }
    else
    {
        nl << "vector<int2, " << size << "> " << name << " = {";
        for (int i = 0; i < size; ++i)
        {
            nl << "{" << v[2 * i] << ", " << v[2 * i + 1] << "}";
            if (i + 1 != size)
            {
                nl << ", ";
            }
        }
        nl << "};";
    }
}

std::string GpuShaderText::vec2fKeyword() const
{
    return getVecKeyword<2>(m_lang);
}

std::string GpuShaderText::vec2fDecl(const std::string & name) const
{
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }

    return vec2fKeyword() + " " + name;
}

std::string GpuShaderText::vec3fKeyword() const
{
    return getVecKeyword<3>(m_lang);
}

std::string GpuShaderText::vec3fConst(float x, float y, float z) const
{
    return vec3fConst(getFloatString(x, m_lang), 
                      getFloatString(y, m_lang), 
                      getFloatString(z, m_lang));
}

std::string GpuShaderText::vec3fConst(double x, double y, double z) const
{
    return vec3fConst(getFloatString(x, m_lang), 
                      getFloatString(y, m_lang), 
                      getFloatString(z, m_lang));
}

std::string GpuShaderText::vec3fConst(const std::string& x, 
                                      const std::string& y,
                                      const std::string& z) const
{
    std::ostringstream kw;
    kw << vec3fKeyword() << "(" << x << ", " << y << ", " << z << ")";
    return kw.str();
}

std::string GpuShaderText::vec3fConst(const float v) const
{
    return vec3fConst(getFloatString(v, m_lang));
}

std::string GpuShaderText::vec3fConst(const double v) const
{
    return vec3fConst(getFloatString(v, m_lang));
}

std::string GpuShaderText::vec3fConst(const std::string & v) const
{
    return vec3fConst(v, v, v);
}

std::string GpuShaderText::vec3fDecl(const std::string &  name) const
{
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }

    return vec3fKeyword() + " " + name;
}

void GpuShaderText::declareVec3f(const std::string & name, 
                                 float x, float y, float z)
{
    declareVec3f(name, getFloatString(x, m_lang), 
                       getFloatString(y, m_lang), 
                       getFloatString(z, m_lang));
}

void GpuShaderText::declareVec3f(const std::string & name, 
                                 double x, double y, double z)
{
    declareVec3f(name, getFloatString(x, m_lang), 
                       getFloatString(y, m_lang), 
                       getFloatString(z, m_lang));
}

void GpuShaderText::declareVec3f(const std::string & name, 
                                 const std::string & x,
                                 const std::string & y,
                                 const std::string & z)
{
    newLine() << vec3fDecl(name) << " = " << vec3fConst(x, y, z) << ";";
}

std::string GpuShaderText::vec4fKeyword() const
{
    return getVecKeyword<4>(m_lang);
}

std::string GpuShaderText::vec4fConst(const float x, const float y, const float z, const float w) const
{
    return vec4fConst(getFloatString(x, m_lang), 
                      getFloatString(y, m_lang), 
                      getFloatString(z, m_lang),
                      getFloatString(w, m_lang));
}

std::string GpuShaderText::vec4fConst(const double x, const double y, const double z, const double w) const
{
    return vec4fConst(getFloatString(x, m_lang), 
                      getFloatString(y, m_lang), 
                      getFloatString(z, m_lang),
                      getFloatString(w, m_lang));
}

std::string GpuShaderText::vec4fConst(const std::string& x, 
                                      const std::string& y,
                                      const std::string& z,
                                      const std::string& w) const
{
    std::ostringstream kw;
    kw << vec4fKeyword() << "(" 
       << x << ", "
       << y << ", "
       << z << ", "
       << w << ")";

    return kw.str();
}

std::string GpuShaderText::vec4fConst(const float v) const
{
    return vec4fConst(getFloatString(v, m_lang));
}

std::string GpuShaderText::vec4fConst(const std::string & v) const
{
    return vec4fConst(v, v, v, v);
}

std::string GpuShaderText::vec4fDecl(const std::string & name) const
{
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }

    return vec4fKeyword() + " " + name;
}

void GpuShaderText::declareVec4f(const std::string & name,
                                 const float x,
                                 const float y, 
                                 const float z,
                                 const float w)
{
    declareVec4f(name, getFloatString(x, m_lang), 
                       getFloatString(y, m_lang), 
                       getFloatString(z, m_lang),
                       getFloatString(w, m_lang));
}

void GpuShaderText::declareVec4f(const std::string & name,
                                 const double x,
                                 const double y, 
                                 const double z,
                                 const double w)
{
    declareVec4f(name, getFloatString(x, m_lang), 
                       getFloatString(y, m_lang), 
                       getFloatString(z, m_lang),
                       getFloatString(w, m_lang));
}

void GpuShaderText::declareVec4f(const std::string & name,
                                 const std::string & x, 
                                 const std::string & y,
                                 const std::string & z,
                                 const std::string & w)
{
    newLine() << vec4fDecl(name) << " = " << vec4fConst(x, y, z, w) << ";";
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

void GpuShaderText::declareUniformArrayFloat(const std::string & uniformName, unsigned int size)
{
    newLine() << "uniform " << floatKeyword() << " " << uniformName << "[" << size << "];";
}

void GpuShaderText::declareUniformArrayInt2(const std::string & uniformName, unsigned int size)
{
    newLine() << "uniform " << int2Keyword() << " " << uniformName << "[" << size << "];";
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

        case GPU_LANGUAGE_UNKNOWN:
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

        case GPU_LANGUAGE_UNKNOWN:
        default:
        {
            throw Exception("Unknown GPU shader language.");
        }
    }
    return kw.str();
}

std::string GpuShaderText::vec3fGreaterThan(const std::string & a,
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
        kw << vec3fKeyword() << "(greaterThan( " << a << ", " << b << "))";
        break;
    }
    case GPU_LANGUAGE_HLSL_DX11:
    {
        kw << vec3fKeyword() << "((" << a << " > " << b << ") ? "
            << vec3fConst(1.0f) << " : " << vec3fConst(0.0f) << ")";
        break;
    }

    case GPU_LANGUAGE_UNKNOWN:
    default:
    {
        throw Exception("Unknown GPU shader language.");
    }
    }
    return kw.str();
}

std::string GpuShaderText::vec4fGreaterThan(const std::string & a, 
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
            kw << vec4fKeyword() << "(greaterThan( " << a << ", " << b << "))";
            break;
        }
        case GPU_LANGUAGE_HLSL_DX11:
        {
            kw << vec4fKeyword() << "((" << a << " > " << b << ") ? " 
                << vec4fConst(1.0f) << " : " << vec4fConst(0.0f) << ")";
            break;
        }

        case GPU_LANGUAGE_UNKNOWN:
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

        case GPU_LANGUAGE_UNKNOWN:
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
    st.newLine() << st.vec3fDecl("ylin") << " = outColor.rgb * gain + offs;";
    st.newLine() << st.vec3fDecl("ylog") << " = base2 * log( ( outColor.rgb + shift ) * m );";
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
    st.newLine() << st.vec3fDecl("xlin") << " = (outColor.rgb - offs) / gain;";
    st.newLine() << st.vec3fDecl("xlog") << " = pow( " << st.vec3fConst(2.0f)
                                         <<        ", outColor.rgb ) * (0.18 + shift) - shift;";
    st.newLine() << "outColor.r = (outColor.r < ybrk) ? xlin.r : xlog.r;";
    st.newLine() << "outColor.g = (outColor.g < ybrk) ? xlin.g : xlog.g;";
    st.newLine() << "outColor.b = (outColor.b < ybrk) ? xlin.b : xlog.b;";
    st.dedent();
    st.newLine() << "}";
}

} // namespace OCIO_NAMESPACE

