// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShader.h"
#include "Mutex.h"
#include "pystring/pystring.h"


namespace OCIO_NAMESPACE
{
class GpuShaderDesc::Impl
{
public:
    GpuLanguage m_language;
    std::string m_functionName;
    std::string m_resourcePrefix;
    std::string m_pixelName;

    mutable std::string m_cacheID;
    mutable Mutex m_cacheIDMutex;

    Impl()
        :   m_language(GPU_LANGUAGE_UNKNOWN)
        ,   m_functionName("OCIOMain")
        ,   m_resourcePrefix("ocio")
        ,   m_pixelName("outColor")
    {
    }

    ~Impl()
    { }

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_language = rhs.m_language;
            m_functionName = rhs.m_functionName;
            m_resourcePrefix = rhs.m_resourcePrefix;
            m_pixelName = rhs.m_pixelName;
            m_cacheID = rhs.m_cacheID;
        }
        return *this;
    }
};

GpuShaderDescRcPtr GpuShaderDesc::CreateLegacyShaderDesc(unsigned edgelen)
{
    return LegacyGpuShaderDesc::Create(edgelen);
}

GpuShaderDescRcPtr GpuShaderDesc::CreateShaderDesc()
{
    return GenericGpuShaderDesc::Create();
}

GpuShaderDesc::GpuShaderDesc()
    :   m_impl(new GpuShaderDesc::Impl)
{
}

GpuShaderDesc::~GpuShaderDesc()
{
    delete m_impl;
    m_impl = NULL;
}

void GpuShaderDesc::setLanguage(GpuLanguage lang)
{
    AutoMutex lock(getImpl()->m_cacheIDMutex);
    getImpl()->m_language = lang;
    getImpl()->m_cacheID = "";
}

GpuLanguage GpuShaderDesc::getLanguage() const
{
    return getImpl()->m_language;
}

void GpuShaderDesc::setFunctionName(const char * name)
{
    AutoMutex lock(getImpl()->m_cacheIDMutex);
	// Note: Remove potentially problematic double underscores from GLSL resource names.
    getImpl()->m_functionName = pystring::replace(name, "__", "_");
    getImpl()->m_cacheID      = "";
}

const char * GpuShaderDesc::getFunctionName() const
{
    return getImpl()->m_functionName.c_str();
}

void GpuShaderDesc::setResourcePrefix(const char * prefix)
{
    AutoMutex lock(getImpl()->m_cacheIDMutex);
    // Note: Remove potentially problematic double underscores from GLSL resource names.
    getImpl()->m_resourcePrefix = pystring::replace(prefix, "__", "_");
    getImpl()->m_cacheID        = "";
}

const char * GpuShaderDesc::getResourcePrefix() const
{
    return getImpl()->m_resourcePrefix.c_str();
}

void GpuShaderDesc::setPixelName(const char * name)
{
    AutoMutex lock(getImpl()->m_cacheIDMutex);
    // Note: Remove potentially problematic double underscores from GLSL resource names.
    getImpl()->m_pixelName = pystring::replace(name, "__", "_");
    getImpl()->m_cacheID   = "";
}

const char * GpuShaderDesc::getPixelName() const
{
    return getImpl()->m_pixelName.c_str();
}

const char * GpuShaderDesc::getCacheID() const
{
    AutoMutex lock(getImpl()->m_cacheIDMutex);

    if(getImpl()->m_cacheID.empty())
    {
        std::ostringstream os;
        os << GpuLanguageToString(getImpl()->m_language) << " ";
        os << getImpl()->m_functionName << " ";
        os << getImpl()->m_resourcePrefix << " ";
        os << getImpl()->m_pixelName << " ";
        getImpl()->m_cacheID = os.str();
    }

    return getImpl()->m_cacheID.c_str();
}

} // namespace OCIO_NAMESPACE
