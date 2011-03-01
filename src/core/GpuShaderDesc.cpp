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

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "Mutex.h"

OCIO_NAMESPACE_ENTER
{
    class GpuShaderDesc::Impl
    {
    public:
        GpuLanguage language_;
        std::string functionName_;
        int lut3DEdgeLen_;
        
        mutable std::string cacheID_;
        mutable Mutex cacheIDMutex_;
        
        Impl() :
            language_(GPU_LANGUAGE_UNKNOWN),
            lut3DEdgeLen_(0)
        {
        }
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            language_ = rhs.language_;
            functionName_ = rhs.functionName_;
            lut3DEdgeLen_ = rhs.lut3DEdgeLen_;
            cacheID_ = rhs.cacheID_;
            return *this;
        }
    };
    
    
    GpuShaderDesc::GpuShaderDesc()
    : m_impl(new GpuShaderDesc::Impl)
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
    
    void GpuShaderDesc::setLut3DEdgeLen(int len)
    {
        AutoMutex lock(getImpl()->cacheIDMutex_);
        getImpl()->lut3DEdgeLen_ = len;
        getImpl()->cacheID_ = "";
    }
    
    int GpuShaderDesc::getLut3DEdgeLen() const
    {
        return getImpl()->lut3DEdgeLen_;
    }
    
    const char * GpuShaderDesc::getCacheID() const
    {
        AutoMutex lock(getImpl()->cacheIDMutex_);
        
        if(getImpl()->cacheID_.empty())
        {
            std::ostringstream os;
            os << GpuLanguageToString(getImpl()->language_) << " ";
            os << getImpl()->functionName_ << " ";
            os << getImpl()->lut3DEdgeLen_;
            getImpl()->cacheID_ = os.str();
        }
        
        return getImpl()->cacheID_.c_str();
    }
}
OCIO_NAMESPACE_EXIT
