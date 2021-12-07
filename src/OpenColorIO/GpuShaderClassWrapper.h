// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_GPUSHADERCLASSWRAPPER_H
#define INCLUDED_OCIO_GPUSHADERCLASSWRAPPER_H

#include <sstream>
#include <utility>
#include <vector>
#include <memory>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"

namespace OCIO_NAMESPACE
{

class GpuShaderClassWrapper
{
public:
    virtual void prepareClassWrapper(const std::string& resourcePrefix,
                                     const std::string& functionName,
                                     const std::string& originalHeader) = 0;
    virtual std::string getClassWrapperHeader(const std::string& originalHeader) = 0;
    virtual std::string getClassWrapperFooter(const std::string& originalFooter) = 0;
    
    virtual std::unique_ptr<GpuShaderClassWrapper> clone() const = 0;
    
    virtual ~GpuShaderClassWrapper() = default;
};

class NullGpuShaderClassWrapper : public GpuShaderClassWrapper
{
public:
    void prepareClassWrapper(const std::string& /*resourcePrefix*/,
                             const std::string& /*functionName*/,
                             const std::string& /*originalHeader*/) final {}
    std::string getClassWrapperHeader(const std::string& originalHeader) final { return originalHeader; }
    std::string getClassWrapperFooter(const std::string& originalFooter) final { return originalFooter; }
    
    std::unique_ptr<GpuShaderClassWrapper> clone() const final
    {
        return std::unique_ptr<NullGpuShaderClassWrapper>(new NullGpuShaderClassWrapper());
    }
};

class MetalShaderClassWrapper : public GpuShaderClassWrapper
{
public:
    void prepareClassWrapper(const std::string& resourcePrefix,
                             const std::string& functionName,
                             const std::string& originalHeader) final;
    std::string getClassWrapperHeader(const std::string& originalHeader) final;
    std::string getClassWrapperFooter(const std::string& originalFooter) final;
    
    std::unique_ptr<GpuShaderClassWrapper> clone() const final;
    MetalShaderClassWrapper& operator=(const MetalShaderClassWrapper& rhs);
    
private:
    struct FunctionParam
    {
        FunctionParam(const std::string& type, const std::string& name) :
            m_type(type),
            m_name(name)
        {
            size_t openAngledBracketPos = name.find('[');
            m_isArray = openAngledBracketPos != std::string::npos;
        }

        std::string m_type;
        std::string m_name;
        bool m_isArray;
    };
    
    std::string getClassWrapperName(const std::string &resourcePrefix, const std::string& functionName);
    void extractFunctionParameters(const std::string& declaration);
    std::string generateClassWrapperHeader(GpuShaderText& st) const;
    std::string generateClassWrapperFooter(GpuShaderText& st, const std::string &ocioFunctionName) const;
    
    std::string m_className;
    std::string m_functionName;
    std::vector<FunctionParam> m_functionParameters;
};

} // namespace OCIO_NAMESPACE

#endif
