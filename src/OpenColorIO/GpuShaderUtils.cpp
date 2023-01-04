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
        case GPU_LANGUAGE_GLSL_ES_1_0:
        case GPU_LANGUAGE_GLSL_ES_3_0:
        {
            kw << "vec" << N;
            break;
        }
        case GPU_LANGUAGE_CG:
        {
            kw << "half" << N;
            break;
        }

        case GPU_LANGUAGE_MSL_2_0:
        case GPU_LANGUAGE_HLSL_DX11:
        {
            kw << "float" << N;
            break;
        }
        case LANGUAGE_OSL_1:
        {
            kw << "vector" << N;
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
        case GPU_LANGUAGE_GLSL_ES_1_0:
        case GPU_LANGUAGE_GLSL_ES_3_0:
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
        case LANGUAGE_OSL_1:
        {
            throw Exception("Unsupported by the Open Shading language (OSL) translation.");
        }
        case GPU_LANGUAGE_MSL_2_0:
        {
            std::ostringstream t;
            t << "texture" << N << "d<float> " << textureName << ";";
            textureDecl = t.str();

            t.str("");
            t << "sampler" << " " << samplerName << ";";
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
        {
            kw << "texture" << N << "D(" << samplerName << ", " << coords << ")";
            break;
        }
        case GPU_LANGUAGE_GLSL_1_3:
        {
            kw << "texture(" << samplerName << ", " << coords << ")";
            break;
        }
        case GPU_LANGUAGE_GLSL_ES_1_0:
        {
            if (N == 1) {
                throw Exception("1D textures are unsupported by OpenGL ES.");
            }

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
        case GPU_LANGUAGE_GLSL_ES_3_0:
        {
            if (N == 1) {
                throw Exception("1D textures are unsupported by OpenGL ES.");
            }

            kw << "texture(" << samplerName << ", " << coords << ")";
            break;
        }
        case LANGUAGE_OSL_1:
        {
            throw Exception("Unsupported by the Open Shading language (OSL) translation.");
        }

        case GPU_LANGUAGE_MSL_2_0:
        {
            kw << textureName << ".sample(" << samplerName << ", " << coords << ")";
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
    static constexpr unsigned tabSize = 2;

    m_ossText << std::string(tabSize * m_indent, ' ')
              << m_ossLine.str()
              << std::endl;

    m_ossLine.str("");
    m_ossLine.clear();
}

std::string GpuShaderText::constKeyword() const
{
    std::string str;

    switch (m_lang)
    {
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        case GPU_LANGUAGE_GLSL_4_0:
        case GPU_LANGUAGE_GLSL_ES_1_0:
        case GPU_LANGUAGE_GLSL_ES_3_0:
        case GPU_LANGUAGE_MSL_2_0:
        {
            str += "const";
            str += " ";
            break;
        }
        case GPU_LANGUAGE_HLSL_DX11:
        {
            str += "static const";
            str += " ";
            break;
        }
        case LANGUAGE_OSL_1:
        case GPU_LANGUAGE_CG:
            break;
    }

    return str;
}

std::string GpuShaderText::floatKeyword() const
{
    return (m_lang == GPU_LANGUAGE_CG ? "half" : "float");
}

std::string GpuShaderText::floatKeywordConst() const
{
    std::string str;

    str += constKeyword();
    str += floatKeyword();

    return str;
}

std::string GpuShaderText::floatDecl(const std::string & name) const
{
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }

    return floatKeyword() + " " + name;
}

std::string GpuShaderText::intKeyword() const
{
    return "int";
}

std::string GpuShaderText::intKeywordConst() const
{
    std::string str;

    str += constKeyword();
    str += intKeyword();

    return str;
}

std::string GpuShaderText::colorDecl(const std::string & name) const
{
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }

    return (m_lang==LANGUAGE_OSL_1 ? "color" : float3Keyword()) + " " + name;
}

void GpuShaderText::declareVarConst(const std::string & name, float v)
{
    newLine() << constKeyword() << declareVarStr(name, v) << ";";
}

void GpuShaderText::declareVar(const std::string & name, float v)
{
    newLine() << declareVarStr(name, v) << ";";
}

// TODO: OSL: The method only solves the problem for constant float values. The code must also
// support the in-place declarations (like res = t + vec3(...) for example).

std::string GpuShaderText::declareVarStr(const std::string & name, float v)
{
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }

    // Note that OSL does not support inf and -inf so the code below adjusts the value to float max.

    if (std::isinf(v))
    {
        float newVal = v;

        if (std::signbit(v))
        {
            newVal = -1 * std::numeric_limits<float>::max();
        }
        else
        {
            newVal = std::numeric_limits<float>::max();
        }

        std::ostringstream oss;
        oss.precision(std::numeric_limits<float>::max_digits10);
        oss << newVal;

        return floatDecl(name) + " = " + oss.str();
    }

    return floatDecl(name) + " = " + getFloatString(v, m_lang);
}

std::string GpuShaderText::vectorCompareExpression(const std::string& lhs, const std::string& op, const std::string& rhs)
{
    std::string ret = lhs + " " + op + " " + rhs;
    if(m_lang == GPU_LANGUAGE_MSL_2_0)
    {
        ret = "any( " + ret + " )";
    }
    return ret;
}

void GpuShaderText::declareVarConst(const std::string & name, bool v)
{
    newLine() << constKeyword() << declareVarStr(name, v) << ";";
}

void GpuShaderText::declareVar(const std::string & name, bool v)
{
    newLine() << declareVarStr(name, v) << ";";
}

std::string GpuShaderText::declareVarStr(const std::string & name, bool v)
{
    if (name.empty())
    {
        throw Exception("GPU variable name is empty.");
    }

    if (m_lang==LANGUAGE_OSL_1)
    {
        return intKeyword() + " " + name + " = " + (v ? "1" : "0");
    }
    else
    {
        return "bool " + name + " = " + (v ? "true" : "false");
    }
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

    switch (m_lang)
    {
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        case GPU_LANGUAGE_GLSL_4_0:
        case GPU_LANGUAGE_GLSL_ES_1_0:
        case GPU_LANGUAGE_GLSL_ES_3_0:
        {
            nl << floatKeywordConst() << " " << name << "[" << size << "] = ";
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
            break;
        }
        case LANGUAGE_OSL_1:
        case GPU_LANGUAGE_CG:
        case GPU_LANGUAGE_HLSL_DX11:
        case GPU_LANGUAGE_MSL_2_0:
        {
            nl << floatKeywordConst() << " " << name << "[" << size << "] = {";
            for (int i = 0; i < size; ++i)
            {
                nl << getFloatString(v[i], m_lang);
                if (i + 1 != size)
                {
                    nl << ", ";
                }
            }
            nl << "};";
            break;
        }
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

    switch (m_lang)
    {
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        case GPU_LANGUAGE_GLSL_4_0:
        case GPU_LANGUAGE_GLSL_ES_1_0:
        case GPU_LANGUAGE_GLSL_ES_3_0:
        {
            nl << intKeywordConst() << " " << name << "[" << size << "] = "
               << intKeyword() << "[" << size << "](";
            for (int i = 0; i < size; ++i)
            {
                nl << v[i];
                if (i + 1 != size)
                {
                    nl << ", ";
                }
            }
            nl << ");";
            break;
        }
        case GPU_LANGUAGE_HLSL_DX11:
        case GPU_LANGUAGE_MSL_2_0:
        {
            nl << intKeywordConst() << " " << name << "[" << size << "] = {";
            for (int i = 0; i < size; ++i)
            {
                nl << v[i];
                if (i + 1 != size)
                {
                    nl << ", ";
                }
            }
            nl << "};";
            break;
        }
        case LANGUAGE_OSL_1:
        case GPU_LANGUAGE_CG:
        {
            nl << intKeyword() << " " << name << "[" << size << "] = {";
            for (int i = 0; i < size; ++i)
            {
                nl << v[i];
                if (i + 1 != size)
                {
                    nl << ", ";
                }
            }
            nl << "};";
            break;
        }
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
    return m_lang == LANGUAGE_OSL_1 ? "vector" : getVecKeyword<3>(m_lang);
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

void GpuShaderText::declareFloat3(const std::string & name, float x, float y, float z)
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

std::string GpuShaderText::float4Const(const std::string & x,
                                       const std::string & y,
                                       const std::string & z,
                                       const std::string & w) const
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
    newLine() << (m_lang == GPU_LANGUAGE_MSL_2_0 ? "" : "uniform ") << floatKeyword() << " " << uniformName << ";";
}

void GpuShaderText::declareUniformBool(const std::string & uniformName)
{
    newLine() << (m_lang == GPU_LANGUAGE_MSL_2_0 ? "" : "uniform ") << "bool " << uniformName << ";";
}

void GpuShaderText::declareUniformFloat3(const std::string & uniformName)
{
    newLine() << (m_lang == GPU_LANGUAGE_MSL_2_0 ? "" : "uniform ") << float3Keyword() << " " << uniformName << ";";
}

void GpuShaderText::declareUniformArrayFloat(const std::string & uniformName, unsigned int size)
{
    newLine() << (m_lang == GPU_LANGUAGE_MSL_2_0 ? "" : "uniform ") << floatKeyword() << " " << uniformName << "[" << size << "];";
}

void GpuShaderText::declareUniformArrayInt(const std::string & uniformName, unsigned int size)
{
    newLine() << (m_lang == GPU_LANGUAGE_MSL_2_0 ? "" : "uniform ") << intKeyword() << " " << uniformName << "[" << size << "];";
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
        case GPU_LANGUAGE_GLSL_ES_1_0:
        case GPU_LANGUAGE_GLSL_ES_3_0:
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
        case LANGUAGE_OSL_1:
        {
            kw << "matrix(" << getMatrixValues<T, 4>(m4x4, lang, false) << ") * " << vecName;
            break;
        }
        case GPU_LANGUAGE_MSL_2_0:
        {
            kw << "float4x4(" << getMatrixValues<T, 4>(m4x4, lang, true) << ") * " << vecName;
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
        case LANGUAGE_OSL_1:
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        case GPU_LANGUAGE_GLSL_4_0:
        case GPU_LANGUAGE_GLSL_ES_1_0:
        case GPU_LANGUAGE_GLSL_ES_3_0:
        case GPU_LANGUAGE_MSL_2_0:
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
        case GPU_LANGUAGE_GLSL_ES_1_0:
        case GPU_LANGUAGE_GLSL_ES_3_0:
        case GPU_LANGUAGE_CG:
        {
            kw << float3Keyword() << "(greaterThan( " << a << ", " << b << "))";
            break;
        }
        case LANGUAGE_OSL_1:
        case GPU_LANGUAGE_MSL_2_0:
        case GPU_LANGUAGE_HLSL_DX11:
        {
            kw << float3Keyword() << "(" 
               << "(" << a << "[0] > " << b << "[0]) ? 1.0 : 0.0, "
               << "(" << a << "[1] > " << b << "[1]) ? 1.0 : 0.0, "
               << "(" << a << "[2] > " << b << "[2]) ? 1.0 : 0.0)";
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
        case GPU_LANGUAGE_GLSL_ES_1_0:
        case GPU_LANGUAGE_GLSL_ES_3_0:
        case GPU_LANGUAGE_CG:
        {
            kw << float4Keyword() << "(greaterThan( " << a << ", " << b << "))";
            break;
        }
        case GPU_LANGUAGE_MSL_2_0:
        case GPU_LANGUAGE_HLSL_DX11:
        {
            kw << float4Keyword() << "(" 
               << "(" << a << "[0] > " << b << "[0]) ? 1.0 : 0.0, "
               << "(" << a << "[1] > " << b << "[1]) ? 1.0 : 0.0, "
               << "(" << a << "[2] > " << b << "[2]) ? 1.0 : 0.0, "
               << "(" << a << "[3] > " << b << "[3]) ? 1.0 : 0.0)";
            break;
        }
        case LANGUAGE_OSL_1:
        {
            kw << float4Keyword() << "(" 
               << "(" << a << ".rgb.r > " << b << ".x) ? 1.0 : 0.0, "
               << "(" << a << ".rgb.g > " << b << ".y) ? 1.0 : 0.0, "
               << "(" << a << ".rgb.b > " << b << ".z) ? 1.0 : 0.0, "
               << "(" << a << ".a > "     << b << ".w) ? 1.0 : 0.0)";
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
        case GPU_LANGUAGE_GLSL_ES_1_0:
        case GPU_LANGUAGE_GLSL_ES_3_0:
        {
            // note: "atan" not "atan2"
            kw << "atan(" << y << ", " << x << ")";
            break;
        }
        case GPU_LANGUAGE_HLSL_DX11:
        {
            // note: Various internet sources claim that the x & y arguments need to be
            // swapped for HLSL (relative to GLSL).  However, recent testing on Windows
            // has revealed that the argument order needs to be the same as GLSL.
            kw << "atan2(" << y << ", " << x << ")";
            break;
        }
        case LANGUAGE_OSL_1:
        case GPU_LANGUAGE_MSL_2_0:
        {
            kw << "atan2(" << y << ", " << x << ")";
            break;
        }

        default:
        {
            throw Exception("Unknown GPU shader language.");
        }
    }
    return kw.str();
}

std::string GpuShaderText::sign(const std::string & v) const
{
    std::ostringstream kw;
    switch(m_lang)
    {
        case GPU_LANGUAGE_CG:
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        case GPU_LANGUAGE_GLSL_4_0:
        case GPU_LANGUAGE_GLSL_ES_1_0:
        case GPU_LANGUAGE_GLSL_ES_3_0:
        case GPU_LANGUAGE_HLSL_DX11:
        case GPU_LANGUAGE_MSL_2_0:
        {
            kw << "sign(" << v << ");";
            break;
        }
        case LANGUAGE_OSL_1:
        {
            kw << "sign(" << float4Const(v + ".rgb.r", v + ".rgb.g",
                                         v + ".rgb.b", v + ".a") << ");";
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
void AddLinToLogShader(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & st)
{
    const std::string pix(shaderCreator->getPixelName());

    st.newLine() << "{";   // establish scope so local variable names won't conflict
    st.indent();
    st.newLine() << st.floatKeywordConst() << " xbrk = 0.0041318374739483946;";
    st.newLine() << st.floatKeywordConst() << " shift = -0.000157849851665374;";
    st.newLine() << st.floatKeywordConst() << " m = 1. / (0.18 + shift);";
    st.newLine() << st.floatKeywordConst() << " base2 = 1.4426950408889634;";  // 1/log(2)
    st.newLine() << st.floatKeywordConst() << " gain = 363.034608563;";
    st.newLine() << st.floatKeywordConst() << " offs = -7.;";
    st.newLine() << st.float3Decl("ylin") << " = " << pix << ".rgb * gain + offs;";
    st.newLine() << st.float3Decl("ylog") << " = base2 * log( ( " << pix << ".rgb + shift ) * m );";
    st.newLine() << pix << ".rgb.r = (" << pix << ".rgb.r < xbrk) ? ylin.x : ylog.x;";
    st.newLine() << pix << ".rgb.g = (" << pix << ".rgb.g < xbrk) ? ylin.y : ylog.y;";
    st.newLine() << pix << ".rgb.b = (" << pix << ".rgb.b < xbrk) ? ylin.z : ylog.z;";
    st.dedent();
    st.newLine() << "}";
}

void AddLogToLinShader(GpuShaderCreatorRcPtr & shaderCreator, GpuShaderText & st)
{
    const std::string pix(shaderCreator->getPixelName());

    st.newLine() << "{";   // establish scope so local variable names won't conflict
    st.indent();
    st.newLine() << st.floatKeywordConst() << " ybrk = -5.5;";
    st.newLine() << st.floatKeywordConst() << " shift = -0.000157849851665374;";
    st.newLine() << st.floatKeywordConst() << " gain = 363.034608563;";
    st.newLine() << st.floatKeywordConst() << " offs = -7.;";
    st.newLine() << st.float3Decl("xlin") << " = (" << pix << ".rgb - offs) / gain;";
    st.newLine() << st.float3Decl("xlog") << " = pow( " << st.float3Const(2.0f)
                                          <<          ", " << pix << ".rgb ) * (0.18 + shift) - shift;";
    st.newLine() << pix << ".rgb.r = (" << pix << ".rgb.r < ybrk) ? xlin.x : xlog.x;";
    st.newLine() << pix << ".rgb.g = (" << pix << ".rgb.g < ybrk) ? xlin.y : xlog.y;";
    st.newLine() << pix << ".rgb.b = (" << pix << ".rgb.b < ybrk) ? xlin.z : xlog.z;";
    st.dedent();
    st.newLine() << "}";
}

} // namespace OCIO_NAMESPACE
