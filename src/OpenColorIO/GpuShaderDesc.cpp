// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShader.h"
#include "Mutex.h"

OCIO_NAMESPACE_ENTER
{
    class GpuShaderDesc::Impl
    {
    public:
        GpuLanguage language_;
        std::string functionName_;
        std::string resourcePrefix_;
        std::string pixelName_;
        
        mutable std::string cacheID_;
        mutable Mutex cacheIDMutex_;
        
        Impl()
            :   language_(GPU_LANGUAGE_UNKNOWN)
            ,   functionName_("OCIOMain")
            ,   resourcePrefix_("ocio")
            ,   pixelName_("outColor")
        {
        }
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                language_ = rhs.language_;
                functionName_ = rhs.functionName_;
                resourcePrefix_ = rhs.resourcePrefix_;
                pixelName_ = rhs.pixelName_;
                cacheID_ = rhs.cacheID_;
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
        AutoMutex lock(getImpl()->cacheIDMutex_);
        getImpl()->language_ = lang;
        getImpl()->cacheID_ = "";
    }
    
    GpuLanguage GpuShaderDesc::getLanguage() const
    {
        return getImpl()->language_;
    }
    
    void GpuShaderDesc::setFunctionName(const char * name)
    {
        AutoMutex lock(getImpl()->cacheIDMutex_);
        getImpl()->functionName_ = name;
        getImpl()->cacheID_ = "";
    }
    
    const char * GpuShaderDesc::getFunctionName() const
    {
        return getImpl()->functionName_.c_str();
    }
    
    void GpuShaderDesc::setResourcePrefix(const char * prefix)
    {
        AutoMutex lock(getImpl()->cacheIDMutex_);
        getImpl()->resourcePrefix_ = prefix;
        getImpl()->cacheID_    = "";
    }

    const char * GpuShaderDesc::getResourcePrefix() const
    {
        return getImpl()->resourcePrefix_.c_str();
    }

    void GpuShaderDesc::setPixelName(const char * name)
    {
        AutoMutex lock(getImpl()->cacheIDMutex_);
        getImpl()->pixelName_ = name;
        getImpl()->cacheID_   = "";
    }

    const char * GpuShaderDesc::getPixelName() const
    {
        return getImpl()->pixelName_.c_str();
    }

    const char * GpuShaderDesc::getCacheID() const
    {
        AutoMutex lock(getImpl()->cacheIDMutex_);
        
        if(getImpl()->cacheID_.empty())
        {
            std::ostringstream os;
            os << GpuLanguageToString(getImpl()->language_) << " ";
            os << getImpl()->functionName_ << " ";
            os << getImpl()->resourcePrefix_ << " ";
            os << getImpl()->pixelName_ << " ";
            getImpl()->cacheID_ = os.str();
        }
        
        return getImpl()->cacheID_.c_str();
    }
}
OCIO_NAMESPACE_EXIT
