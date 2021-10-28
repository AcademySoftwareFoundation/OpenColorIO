// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "DynamicProperty.h"
#include "GpuShader.h"
#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "Logging.h"
#include "Mutex.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

class GpuShaderCreator::Impl
{
public:
    std::string m_uid; // Custom uid if needed.
    GpuLanguage m_language = GPU_LANGUAGE_GLSL_1_2;
    std::string m_functionName;
    std::string m_resourcePrefix;
    std::string m_pixelName;
    unsigned m_numResources = 0;

    mutable std::string m_cacheID;
    mutable Mutex m_cacheIDMutex;

    std::string m_declarations;
    std::string m_helperMethods;
    std::string m_functionHeader;
    std::string m_functionBody;
    std::string m_functionFooter;

    std::string m_shaderCode;
    std::string m_shaderCodeID;

    std::vector<DynamicPropertyRcPtr> m_dynamicProperties;

    Impl()
        :   m_functionName("OCIOMain")
        ,   m_resourcePrefix("ocio")
        ,   m_pixelName("outColor")
    {
    }

    ~Impl()
    { }

    Impl(const Impl & rhs) = delete;

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_uid            = rhs.m_uid;
            m_language       = rhs.m_language;
            m_functionName   = rhs.m_functionName;
            m_resourcePrefix = rhs.m_resourcePrefix;
            m_pixelName      = rhs.m_pixelName;
            m_numResources   = rhs.m_numResources;
            m_cacheID        = rhs.m_cacheID;

            m_declarations   = rhs.m_declarations;
            m_helperMethods  = rhs.m_helperMethods;
            m_functionHeader = rhs.m_functionHeader;
            m_functionBody   = rhs.m_functionBody;
            m_functionFooter = rhs.m_functionFooter;

            m_shaderCode.clear();
            m_shaderCodeID.clear();
        }
        return *this;
    }
};

GpuShaderCreator::GpuShaderCreator()
    :   m_impl(new GpuShaderDesc::Impl)
{
}

GpuShaderCreator::~GpuShaderCreator()
{
    delete m_impl;
    m_impl = nullptr;
}

void GpuShaderCreator::setUniqueID(const char * uid) noexcept
{
    AutoMutex lock(getImpl()->m_cacheIDMutex);
    getImpl()->m_uid = uid ? uid : "";
    getImpl()->m_cacheID.clear();
}

const char * GpuShaderCreator::getUniqueID() const noexcept
{
    return getImpl()->m_uid.c_str();
}

void GpuShaderCreator::setLanguage(GpuLanguage lang) noexcept
{
    AutoMutex lock(getImpl()->m_cacheIDMutex);
    getImpl()->m_language = lang;
    getImpl()->m_cacheID.clear();
}

GpuLanguage GpuShaderCreator::getLanguage() const noexcept
{
    return getImpl()->m_language;
}

void GpuShaderCreator::setFunctionName(const char * name) noexcept
{
    AutoMutex lock(getImpl()->m_cacheIDMutex);
    // Note: Remove potentially problematic double underscores from GLSL resource names.
    getImpl()->m_functionName = StringUtils::Replace(name, "__", "_");
    getImpl()->m_cacheID.clear();
}

const char * GpuShaderCreator::getFunctionName() const noexcept
{
    return getImpl()->m_functionName.c_str();
}

void GpuShaderCreator::setResourcePrefix(const char * prefix) noexcept
{
    AutoMutex lock(getImpl()->m_cacheIDMutex);
    // Note: Remove potentially problematic double underscores from GLSL resource names.
    getImpl()->m_resourcePrefix = StringUtils::Replace(prefix, "__", "_");
    getImpl()->m_cacheID.clear();
}

const char * GpuShaderCreator::getResourcePrefix() const noexcept
{
    return getImpl()->m_resourcePrefix.c_str();
}

void GpuShaderCreator::setPixelName(const char * name) noexcept
{
    AutoMutex lock(getImpl()->m_cacheIDMutex);
    // Note: Remove potentially problematic double underscores from GLSL resource names.
    getImpl()->m_pixelName = StringUtils::Replace(name, "__", "_");
    getImpl()->m_cacheID.clear();
}

const char * GpuShaderCreator::getPixelName() const noexcept
{
    return getImpl()->m_pixelName.c_str();
}

unsigned GpuShaderCreator::getNextResourceIndex() noexcept
{
    return getImpl()->m_numResources++;
}

bool GpuShaderCreator::hasDynamicProperty(DynamicPropertyType type) const
{
    for (auto dp : getImpl()->m_dynamicProperties)
    {
        if (dp->getType() == type)
        {
            // Dynamic property is already there.
            return true;
        }
    }
    return false;
}

void GpuShaderCreator::addDynamicProperty(DynamicPropertyRcPtr & prop)
{
    if (hasDynamicProperty(prop->getType()))
    {
        // Dynamic property is already there.
        std::ostringstream oss;
        oss << "Dynamic property already here: " << prop->getType() << ".";
        throw Exception(oss.str().c_str());
    }

    getImpl()->m_dynamicProperties.push_back(prop);
}

unsigned GpuShaderCreator::getNumDynamicProperties() const noexcept
{
    return (unsigned)getImpl()->m_dynamicProperties.size();
}

DynamicPropertyRcPtr GpuShaderCreator::getDynamicProperty(unsigned index) const
{
    if (index >= (unsigned)getImpl()->m_dynamicProperties.size())
    {
        std::ostringstream oss;
        oss << "Dynamic properties access error: index = " << index
            << " where size = " << getImpl()->m_dynamicProperties.size();
        throw Exception(oss.str().c_str());
    }
    return getImpl()->m_dynamicProperties[index];
}

DynamicPropertyRcPtr GpuShaderCreator::getDynamicProperty(DynamicPropertyType type) const
{
    for (auto dp : getImpl()->m_dynamicProperties)
    {
        if (dp->getType() == type)
        {
            return dp;
        }
    }
    throw Exception("Dynamic property not found.");
}

void GpuShaderCreator::begin(const char *)
{
}

void GpuShaderCreator::end()
{
}

const char * GpuShaderCreator::getCacheID() const noexcept
{
    AutoMutex lock(getImpl()->m_cacheIDMutex);

    if(getImpl()->m_cacheID.empty())
    {
        std::ostringstream os;
        os << GpuLanguageToString(getImpl()->m_language) << " ";
        os << getImpl()->m_functionName << " ";
        os << getImpl()->m_resourcePrefix << " ";
        os << getImpl()->m_pixelName << " ";
        os << getImpl()->m_numResources << " ";
        os << getImpl()->m_shaderCodeID;
        getImpl()->m_cacheID = os.str();
    }

    return getImpl()->m_cacheID.c_str();
}

void GpuShaderCreator::addToDeclareShaderCode(const char * shaderCode)
{
    if(getImpl()->m_declarations.empty())
    {
        getImpl()->m_declarations += "\n// Declaration of all variables\n\n";
    }
    getImpl()->m_declarations += (shaderCode && *shaderCode) ? shaderCode : "";
}

void GpuShaderCreator::addToHelperShaderCode(const char * shaderCode)
{
    if(getImpl()->m_helperMethods.empty())
    {
        getImpl()->m_helperMethods += "\n// Declaration of all helper methods\n\n";
    }
    getImpl()->m_helperMethods += (shaderCode && *shaderCode) ? shaderCode : "";
}

void GpuShaderCreator::addToFunctionShaderCode(const char * shaderCode)
{
    getImpl()->m_functionBody += (shaderCode && *shaderCode) ? shaderCode : "";
}

void GpuShaderCreator::addToFunctionHeaderShaderCode(const char * shaderCode)
{
    getImpl()->m_functionHeader += (shaderCode && *shaderCode) ? shaderCode : "";
}

void GpuShaderCreator::addToFunctionFooterShaderCode(const char * shaderCode)
{
    getImpl()->m_functionFooter += (shaderCode && *shaderCode) ? shaderCode : "";
}

void GpuShaderCreator::createShaderText(const char * shaderDeclarations,
                                        const char * shaderHelperMethods,
                                        const char * shaderFunctionHeader,
                                        const char * shaderFunctionBody,
                                        const char * shaderFunctionFooter)
{
    AutoMutex lock(getImpl()->m_cacheIDMutex);

    getImpl()->m_shaderCode.clear();
    
    getImpl()->m_shaderCode += (shaderDeclarations   && *shaderDeclarations)   ? shaderDeclarations   : "";
    getImpl()->m_shaderCode += (shaderHelperMethods  && *shaderHelperMethods)  ? shaderHelperMethods  : "";
    getImpl()->m_shaderCode += (shaderFunctionHeader && *shaderFunctionHeader) ? shaderFunctionHeader : "";
    getImpl()->m_shaderCode += (shaderFunctionBody   && *shaderFunctionBody)   ? shaderFunctionBody   : "";
    getImpl()->m_shaderCode += (shaderFunctionFooter && *shaderFunctionFooter) ? shaderFunctionFooter : "";

    getImpl()->m_shaderCodeID = CacheIDHash(getImpl()->m_shaderCode.c_str(),
                                            unsigned(getImpl()->m_shaderCode.length()));

    getImpl()->m_cacheID.clear();
}

void GpuShaderCreator::finalize()
{
    if (getLanguage() == LANGUAGE_OSL_1)
    {
        GpuShaderText kw(getLanguage());

        kw.newLine() << "";
        kw.newLine() << "/* All the includes */";
        kw.newLine() << "";
        kw.newLine() << "#include \"vector4.h\"";
        kw.newLine() << "#include \"color4.h\"";

        kw.newLine() << "";
        kw.newLine() << "/* All the generic helper methods */";

        kw.newLine() << "";
        kw.newLine() << "vector4 __operator__mul__(matrix m, vector4 v)";
        kw.newLine() << "{";
        kw.indent();
        kw.newLine() << "return vector4(v.x * m[0][0] + v.y * m[0][1] + v.z * m[0][2] + v.w * m[0][3], ";
        kw.newLine() << "               v.x * m[1][0] + v.y * m[1][1] + v.z * m[1][2] + v.w * m[1][3], ";
        kw.newLine() << "               v.x * m[2][0] + v.y * m[2][1] + v.z * m[2][2] + v.w * m[2][3], ";
        kw.newLine() << "               v.x * m[3][0] + v.y * m[3][1] + v.z * m[3][2] + v.w * m[3][3]);";
        kw.dedent();
        kw.newLine() << "}";

        kw.newLine() << "";
        kw.newLine() << "vector4 __operator__mul__(color4 c, vector4 v)";
        kw.newLine() << "{";
        kw.indent();
        kw.newLine() << "return vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a) * v;";
        kw.dedent();
        kw.newLine() << "}";

        kw.newLine() << "";
        kw.newLine() << "vector4 __operator__mul__(vector4 v, color4 c)";
        kw.newLine() << "{";
        kw.indent();
        kw.newLine() << "return v * vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a);";
        kw.dedent();
        kw.newLine() << "}";

        kw.newLine() << "";
        kw.newLine() << "vector4 __operator__sub__(color4 c, vector4 v)";
        kw.newLine() << "{";
        kw.indent();
        kw.newLine() << "return vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a) - v;";
        kw.dedent();
        kw.newLine() << "}";

        kw.newLine() << "";
        kw.newLine() << "vector4 __operator__add__(vector4 v, color4 c)";
        kw.newLine() << "{";
        kw.indent();
        kw.newLine() << "return v + vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a);";
        kw.dedent();
        kw.newLine() << "}";

        kw.newLine() << "";
        kw.newLine() << "vector4 __operator__add__(color4 c, vector4 v)";
        kw.newLine() << "{";
        kw.indent();
        kw.newLine() << "return vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a) + v;";
        kw.dedent();
        kw.newLine() << "}";

        kw.newLine() << "";
        kw.newLine() << "vector4 pow(color4 c, vector4 v)";
        kw.newLine() << "{";
        kw.indent();
        kw.newLine() << "return pow(vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a), v);";
        kw.dedent();
        kw.newLine() << "}";

        kw.newLine() << "";
        kw.newLine() << "vector4 max(vector4 v, color4 c)";
        kw.newLine() << "{";
        kw.indent();
        kw.newLine() << "return max(v, vector4(c.rgb.r, c.rgb.g, c.rgb.b, c.a));";
        kw.dedent();
        kw.newLine() << "}";

        kw.newLine() << "";
        kw.newLine() << "/* The shader implementation */";
        kw.newLine() << "";
        kw.newLine() << "shader " << "OSL_" << getFunctionName() 
                     << "(color4 inColor = {color(0), 1}, output color4 outColor = {color(0), 1})";
        kw.newLine() << "{";

        const std::string str = kw.string() + getImpl()->m_declarations;
        getImpl()->m_declarations = str;

        // Change the footer part.

        GpuShaderText kw1(getLanguage());
        kw1.newLine() << "";
        kw1.newLine() << "outColor = " << getFunctionName() << "(inColor);";
        kw1.newLine() << "}";

        getImpl()->m_functionFooter += kw1.string();
    }


    createShaderText(getImpl()->m_declarations.c_str(),
                     getImpl()->m_helperMethods.c_str(),
                     getImpl()->m_functionHeader.c_str(),
                     getImpl()->m_functionBody.c_str(),
                     getImpl()->m_functionFooter.c_str());


    if(IsDebugLoggingEnabled())
    {
        std::ostringstream oss;
        oss << std::endl
            << "**" << std::endl
            << "GPU Fragment Shader program" << std::endl
            << getImpl()->m_shaderCode << std::endl;

        LogDebug(oss.str());
    }
}



GpuShaderDescRcPtr GpuShaderDesc::CreateShaderDesc()
{
    return GenericGpuShaderDesc::Create();
}

GpuShaderDesc::GpuShaderDesc()
    :   GpuShaderCreator()
{
}

GpuShaderDesc::~GpuShaderDesc()
{
}

GpuShaderCreatorRcPtr GpuShaderDesc::clone() const
{
    GpuShaderDescRcPtr gpuDesc = CreateShaderDesc();
    *(gpuDesc->getImpl()) = *getImpl();

    return DynamicPtrCast<GpuShaderCreator>(gpuDesc);
}

const char * GpuShaderDesc::getShaderText() const noexcept
{
    return getImpl()->m_shaderCode.c_str();
}

} // namespace OCIO_NAMESPACE
