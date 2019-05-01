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

#include <math.h>


OCIO_NAMESPACE_ENTER
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
        oss << value << ((fracpart == (T)0) ? "." : "");
        return oss.str();
    }

    template<int N>
    std::string getVecKeyword(GpuLanguage lang)
    {
        std::ostringstream kw;
        switch (lang)
        {
            case GPU_LANGUAGE_GLSL_1_0:
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
                throw Exception("Unknown Gpu shader language");
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
            case GPU_LANGUAGE_GLSL_1_0:
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
                throw Exception("Unknown Gpu shader language");
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
            case GPU_LANGUAGE_GLSL_1_0:
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
                throw Exception("Unknown Gpu shader language");
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

    void GpuShaderText::declareVar(const std::string & name, float v)
    {
        declareVar(name, getFloatString(v, m_lang));
    }
    
    void GpuShaderText::declareVar(const std::string & name, const std::string & v)
    {
        if(name.empty())
        {
           throw Exception("Gpu variable name is empty");
        }

        newLine() << (m_lang==GPU_LANGUAGE_CG ? "half " : "float ") 
                  << name << " = " << v << ";";
    }

    std::string GpuShaderText::vec2fKeyword() const
    {
        return getVecKeyword<2>(m_lang);
    }

    std::string GpuShaderText::vec2fDecl(const std::string & name) const
    {
        if (name.empty())
        {
           throw Exception("Gpu variable name is empty");
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
           throw Exception("Gpu variable name is empty");
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
           throw Exception("Gpu variable name is empty");
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
        newLine() << "uniform float " << uniformName << ";";
    }

    // Keep the method private as only float & double types are expected
    template<typename T>
    std::string matrix4Mul(const T * m4x4, const std::string & vecName, GpuLanguage lang)
    {
        if (vecName.empty())
        {
           throw Exception("Gpu variable name is empty");
        }

        std::ostringstream kw;
        switch (lang)
        {
            case GPU_LANGUAGE_GLSL_1_0:
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
                throw Exception("Unknown Gpu shader language");
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
            case GPU_LANGUAGE_GLSL_1_0:
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
                throw Exception("Unknown Gpu shader language");
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
            case GPU_LANGUAGE_GLSL_1_0:
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
                throw Exception("Unknown Gpu shader language");
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
            case GPU_LANGUAGE_GLSL_1_0:
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
                throw Exception("Unknown Gpu shader language");
            }
        }
        return kw.str();
    }
}
OCIO_NAMESPACE_EXIT



///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(GpuShaderUtils, FloatToString)
{
    OIIO_CHECK_EQUAL(OCIO::getFloatString(1.0f, OCIO::GPU_LANGUAGE_GLSL_1_3), "1.");
    OIIO_CHECK_EQUAL(OCIO::getFloatString(-11.0f, OCIO::GPU_LANGUAGE_GLSL_1_3), "-11.");
    OIIO_CHECK_EQUAL(OCIO::getFloatString(-1.0f, OCIO::GPU_LANGUAGE_GLSL_1_3), "-1.");
    OIIO_CHECK_EQUAL(OCIO::getFloatString((float)-1, OCIO::GPU_LANGUAGE_GLSL_1_3), "-1.");
    OIIO_CHECK_EQUAL(OCIO::getFloatString((float)1, OCIO::GPU_LANGUAGE_GLSL_1_3), "1.");
}

#endif // OCIO_UNIT_TEST
