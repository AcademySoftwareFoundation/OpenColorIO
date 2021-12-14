// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cctype>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderClassWrapper.h"

namespace OCIO_NAMESPACE
{

std::string GetArrayLengthVariableName(const std::string& variableName)
{
    return variableName + "_count";
}

std::unique_ptr<GpuShaderClassWrapper> GpuShaderClassWrapper::CreateClassWrapper(GpuLanguage language)
{
    switch(language)
    {
        case GPU_LANGUAGE_MSL_2_0:
            return std::unique_ptr<MetalShaderClassWrapper>(new MetalShaderClassWrapper);

        case LANGUAGE_OSL_1:
            return std::unique_ptr<OSLShaderClassWrapper>(new OSLShaderClassWrapper);

        // Most of the supported GPU shader languages do not have needs imposing a custom class
        // wrapper so, the default class wrapper does nothing.
        case GPU_LANGUAGE_CG:
        case GPU_LANGUAGE_GLSL_1_2:
        case GPU_LANGUAGE_GLSL_1_3:
        case GPU_LANGUAGE_GLSL_4_0:
        case GPU_LANGUAGE_HLSL_DX11:
        case GPU_LANGUAGE_GLSL_ES_1_0:
        case GPU_LANGUAGE_GLSL_ES_3_0:
        default:
            return std::unique_ptr<NullGpuShaderClassWrapper>(new NullGpuShaderClassWrapper);
    }
}

std::unique_ptr<GpuShaderClassWrapper> NullGpuShaderClassWrapper::clone() const
{
    return std::unique_ptr<NullGpuShaderClassWrapper>(new NullGpuShaderClassWrapper());
}

std::unique_ptr<GpuShaderClassWrapper> OSLShaderClassWrapper::clone() const
{
    return std::unique_ptr<OSLShaderClassWrapper>(new OSLShaderClassWrapper());
}

std::string OSLShaderClassWrapper::getClassWrapperHeader(const std::string& originalHeader)
{
    GpuShaderText st(LANGUAGE_OSL_1);

    st.newLine() << "";
    st.newLine() << "/* All the includes */";
    st.newLine() << "";
    st.newLine() << "#include \"vector4.h\"";
    st.newLine() << "#include \"color4.h\"";

    st.newLine() << "";
    st.newLine() << "/* All the generic helper methods */";

    st.newLine() << "";
    st.newLine() << "vector4 __operator__mul__(matrix m, vector4 v)";
    st.newLine() << "{";
    st.indent();
    st.newLine() << "return vector4(v.x * m[0][0] + v.y * m[0][1] + v.z * m[0][2] + v.w * m[0][3], ";
    st.newLine() << "               v.x * m[1][0] + v.y * m[1][1] + v.z * m[1][2] + v.w * m[1][3], ";
    st.newLine() << "               v.x * m[2][0] + v.y * m[2][1] + v.z * m[2][2] + v.w * m[2][3], ";
    st.newLine() << "               v.x * m[3][0] + v.y * m[3][1] + v.z * m[3][2] + v.w * m[3][3]);";
    st.dedent();
    st.newLine() << "}";

    st.newLine() << "";
    st.newLine() << "vector4 __operator__mul__(color4 c, vector4 v)";
    st.newLine() << "{";
    st.indent();
    st.newLine() << "return vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a) * v;";
    st.dedent();
    st.newLine() << "}";

    st.newLine() << "";
    st.newLine() << "vector4 __operator__mul__(vector4 v, color4 c)";
    st.newLine() << "{";
    st.indent();
    st.newLine() << "return v * vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a);";
    st.dedent();
    st.newLine() << "}";

    st.newLine() << "";
    st.newLine() << "vector4 __operator__sub__(color4 c, vector4 v)";
    st.newLine() << "{";
    st.indent();
    st.newLine() << "return vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a) - v;";
    st.dedent();
    st.newLine() << "}";

    st.newLine() << "";
    st.newLine() << "vector4 __operator__add__(vector4 v, color4 c)";
    st.newLine() << "{";
    st.indent();
    st.newLine() << "return v + vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a);";
    st.dedent();
    st.newLine() << "}";

    st.newLine() << "";
    st.newLine() << "vector4 __operator__add__(color4 c, vector4 v)";
    st.newLine() << "{";
    st.indent();
    st.newLine() << "return vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a) + v;";
    st.dedent();
    st.newLine() << "}";

    st.newLine() << "";
    st.newLine() << "vector4 pow(color4 c, vector4 v)";
    st.newLine() << "{";
    st.indent();
    st.newLine() << "return pow(vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a), v);";
    st.dedent();
    st.newLine() << "}";

    st.newLine() << "";
    st.newLine() << "vector4 max(vector4 v, color4 c)";
    st.newLine() << "{";
    st.indent();
    st.newLine() << "return max(v, vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a));";
    st.dedent();
    st.newLine() << "}";

    st.newLine() << "";
    st.newLine() << "/* The shader implementation */";
    st.newLine() << "";
    st.newLine() << "shader " << "OSL_" << m_functionName
                 << "(color4 inColor = {color(0), 1}, output color4 outColor = {color(0), 1})";
    st.newLine() << "{";

    return st.string() + originalHeader;
}

std::string OSLShaderClassWrapper::getClassWrapperFooter(const std::string & originalFooter)
{
    GpuShaderText st(LANGUAGE_OSL_1);

    st.newLine() << "";
    st.newLine() << "outColor = " << m_functionName << "(inColor);";
    st.newLine() << "}";

    return originalFooter + st.string();
}

std::string MetalShaderClassWrapper::generateClassWrapperHeader(GpuShaderText& kw) const
{
    if(m_className.empty())
    {
        throw Exception("Struct name must include at least 1 character");
    }
    if(std::isdigit(m_className[0]))
    {
        throw Exception(("Struct name must not start with a digit. Invalid className passed in: " + m_className).c_str());
    }

    kw.newLine() << "struct " << m_className;
    kw.newLine() << "{";
    kw.newLine() << m_className <<"(";
    kw.indent();

    std::string separator = "";
    for(const auto& param : m_functionParameters)
    {
        kw.newLine() << separator << (param.m_isArray ? "constant " : "") << param.m_type << " " << param.m_name;
        if(param.m_isArray)
        {
            kw.newLine() << ", int " << GetArrayLengthVariableName(param.m_name.substr(0, param.m_name.find('[')));
        }
        separator = ", ";
    }
    kw.dedent();
    kw.newLine() << ")";
    kw.newLine() << "{";
    
    kw.indent();
    for(const auto& param : m_functionParameters)
    {
        size_t openAngledBracketPos = param.m_name.find('[');
        if(!param.m_isArray)
        {
            kw.newLine()    << "this->" << param.m_name  << " = " << param.m_name  << ";";
        }
        else
        {
            size_t closeAngledBracketPos = param.m_name.find(']');
            std::string variableName = param.m_name.substr(0, openAngledBracketPos);
            
            kw.newLine()    << "for(int i = 0; i < "
                            << GetArrayLengthVariableName(variableName)
                            << "; ++i)";
            kw.newLine()    << "{";
            kw.indent();
            kw.newLine()    << "this->" << variableName << "[i] = " << variableName << "[i];";
            kw.dedent();
            kw.newLine()    << "}";
            
            kw.newLine()    << "for(int i = "
                            << GetArrayLengthVariableName(variableName)
                            << "; i < "
                            << param.m_name.substr(openAngledBracketPos+1, closeAngledBracketPos-openAngledBracketPos-1)
                            << "; ++i)";
            kw.newLine()    << "{";
            kw.indent();
            kw.newLine()    << "this->" << variableName << "[i] = 0;";
            kw.dedent();
            kw.newLine()    << "}";
        }
    }
    kw.dedent();
    kw.newLine() <<"}";
    return kw.string();
}
    
std::string MetalShaderClassWrapper::generateClassWrapperFooter(GpuShaderText& kw, const std::string &ocioFunctionName) const
{
    if(m_className.empty())
    {
        throw Exception("Struct name must include at least 1 character");
    }
    if(std::isdigit(m_className[0]))
    {
        throw Exception(("Struct name must not start with a digit. Invalid className passed in: " + m_className).c_str());
    }

    kw.newLine() << "};";
    
    kw.newLine() << kw.float4Keyword() << " " << ocioFunctionName<< "(";
    std::string texParamOut;
    
    kw.indent();
    std::string separator = "";
    for(const auto& param : m_functionParameters)
    {
        kw.newLine() << separator << (param.m_isArray ? "constant " : "") << param.m_type << " " << param.m_name;
        if(param.m_isArray)
        {
            kw.newLine() << ", int " << GetArrayLengthVariableName(param.m_name.substr(0, param.m_name.find('[')));
        }
        separator = ", ";
    }
    kw.newLine() << separator << kw.float4Keyword() << " inPixel)";
    kw.dedent();
    kw.newLine() << "{";
    kw.indent();
    kw.newLine() << "return " << m_className << "(";

    kw.indent();
    separator = "";
    for(const auto& param : m_functionParameters)
    {
        size_t openAngledBracketPos = param.m_name.find('[');
        bool isArray = openAngledBracketPos != std::string::npos;
        
        if(!isArray)
        {
            kw.newLine() << separator << param.m_name;
        }
        else
        {
            kw.newLine() << separator << param.m_name.substr(0, openAngledBracketPos);
            kw.newLine() << ", " << GetArrayLengthVariableName(param.m_name.substr(0, openAngledBracketPos));
        }
        separator = ", ";
    }
    kw.dedent();
    
    kw.newLine() << ")." << ocioFunctionName << "(inPixel);";
    kw.dedent();
    kw.newLine() << "}";
    
    return kw.string();
}

std::string MetalShaderClassWrapper::getClassWrapperName(const std::string &resourcePrefix, const std::string& functionName)
{
    return (resourcePrefix.length() == 0 ? "OCIO_" : resourcePrefix) + functionName;
}

void MetalShaderClassWrapper::extractFunctionParameters(const std::string& declaration)
{
    // We want the caller to always pass 3d luts first with their samplers, and then pass other luts.
    std::vector<std::tuple<std::string, std::string, std::string>> lut3DTextures;
    std::vector<std::tuple<std::string, std::string, std::string>> lutTextures;
    std::vector<std::pair <std::string, std::string             >> uniforms;

    m_functionParameters.clear();
    
    std::string lineBuffer;
    
    std::istringstream is(declaration);
    while(!is.eof())
    {
        std::getline(is, lineBuffer);
        
        if(lineBuffer.empty())
            continue;
        
        size_t i = 0;
        
        // Skip spaces
        while(std::isspace(lineBuffer[i])) ++i;
        
        // if the line was all skippable characters
        if(i >= lineBuffer.size())
            continue;
        
        // if the line is a comment
        if(lineBuffer[i + 0] == '/' && lineBuffer[i + 1] == '/')
            continue;

        if(lineBuffer.compare(i, 7, "texture") == 0)
        {
            int  textureDim = static_cast<int>(lineBuffer[i+7] - '0');
            
            size_t endTextureType = lineBuffer.find('>');
            std::string textureType = lineBuffer.substr(i, (endTextureType - i + 1));
            
            i = endTextureType + 1;
            while(std::isspace(lineBuffer[i])) ++i;
            
            size_t endTextureName = lineBuffer.find_first_of(" \t;", i);
            std::string textureName = lineBuffer.substr(i, (endTextureName - i));

            std::getline(is, lineBuffer);
            
            i = lineBuffer.find("sampler") + 7;
            while(std::isspace(lineBuffer[i])) ++i;
            size_t endSamplerName = lineBuffer.find_first_of(" \t;", i);
            std::string samplerName = lineBuffer.substr(i, endSamplerName - i);
            
            if(textureDim == 3)
                lut3DTextures.emplace_back(textureType, textureName, samplerName);
            else
                lutTextures.emplace_back(textureType, textureName, samplerName);
        }
        else
        {
            size_t endTypeName     = lineBuffer.find_first_of(" \t", i);
            std::string variableType = lineBuffer.substr(i, (endTypeName - i));
            
            i = endTypeName + 1;
            while(std::isspace(lineBuffer[i])) ++i;
            
            size_t endVariableName = lineBuffer.find_first_of(" \t;", i);
            std::string variableName = lineBuffer.substr(i, (endVariableName - i));
            uniforms.emplace_back(variableType, variableName);
        }
    }
        
    for(const auto& lut3D : lut3DTextures)
    {
        m_functionParameters.emplace_back(std::get<0>(lut3D).c_str(), std::get<1>(lut3D).c_str());
        m_functionParameters.emplace_back("sampler", std::get<2>(lut3D).c_str());
    }
    
    for(const auto& lut : lutTextures)
    {
        m_functionParameters.emplace_back(std::get<0>(lut).c_str(), std::get<1>(lut).c_str());
        m_functionParameters.emplace_back("sampler", std::get<2>(lut).c_str());
    }
    
    for(const auto& uniform : uniforms)
    {
        m_functionParameters.emplace_back(uniform.first, uniform.second);
    }
}

void MetalShaderClassWrapper::prepareClassWrapper(const std::string& resourcePrefix, const std::string& functionName, const std::string& originalHeader)
{
    m_functionName = functionName;
    m_className = getClassWrapperName(resourcePrefix, functionName);
    extractFunctionParameters(originalHeader);
}

std::string MetalShaderClassWrapper::getClassWrapperHeader(const std::string& originalHeader)
{
    GpuShaderText st(GPU_LANGUAGE_MSL_2_0);

    generateClassWrapperHeader(st);
    st.newLine();
    
    std::string classWrapHeader = "\n// Declaration of class wrapper\n\n";
    classWrapHeader += st.string();
    
    return classWrapHeader + originalHeader;
}

std::string MetalShaderClassWrapper::getClassWrapperFooter(const std::string& originalFooter)
{
    GpuShaderText st(GPU_LANGUAGE_MSL_2_0);
    
    st.newLine();
    generateClassWrapperFooter(st, m_functionName);
    
    std::string classWrapFooter = "\n// Close class wrapper\n\n";
    classWrapFooter += st.string();
    
    return originalFooter + classWrapFooter;
}

std::unique_ptr<GpuShaderClassWrapper> MetalShaderClassWrapper::clone() const
{
    std::unique_ptr<MetalShaderClassWrapper> clonedWrapper = std::unique_ptr<MetalShaderClassWrapper>(new MetalShaderClassWrapper);
    *clonedWrapper = *this;
    return clonedWrapper;
}

MetalShaderClassWrapper& MetalShaderClassWrapper::operator=(const MetalShaderClassWrapper& rhs)
{
    this->m_className          = rhs.m_className;
    this->m_functionName       = rhs.m_functionName;
    this->m_functionParameters = rhs.m_functionParameters;
    return *this;
}

} // namespace OCIO_NAMESPACE
