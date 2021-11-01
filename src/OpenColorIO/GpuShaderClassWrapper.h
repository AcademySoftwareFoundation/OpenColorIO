// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include <utility>
#include <vector>

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
    
    virtual bool operator=(const GpuShaderClassWrapper& rhs) = 0;
    
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
    
    bool operator=(const GpuShaderClassWrapper& rhs) final
    {
        if(auto* nullshader_rhs = dynamic_cast<const NullGpuShaderClassWrapper*>(&rhs))
        {
            return true;
        }
        return false;
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
    
    bool operator=(const GpuShaderClassWrapper& rhs) final;
    
private:
    struct FunctionParam
    {
        FunctionParam(const std::string& type, const std::string& name) :
            type(type),
            name(name)
        {
        }

        const std::string type;
        const std::string name;
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
