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
