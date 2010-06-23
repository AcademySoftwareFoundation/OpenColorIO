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

#include "GpuShaderDesc.h"

OCIO_NAMESPACE_ENTER
{
    
        
    GpuShaderDesc::GpuShaderDesc()
    : m_impl(new GpuShaderDesc::Impl)
    {
    }
    
    GpuShaderDesc::~GpuShaderDesc()
    {
    }
    
    
    void GpuShaderDesc::setLanguage(GpuLanguage lang)
    {
        m_impl->setLanguage(lang);
    }
    
    GpuLanguage GpuShaderDesc::getLanguage() const
    {
        return m_impl->getLanguage();
    }
    
    void GpuShaderDesc::setFunctionName(const char * name)
    {
        m_impl->setFunctionName(name);
    }
    
    const char * GpuShaderDesc::getFunctionName() const
    {
        return m_impl->getFunctionName();
    }
    
    void GpuShaderDesc::setLut3DEdgeLen(int len)
    {
        m_impl->setLut3DEdgeLen(len);
    }
    
    int GpuShaderDesc::getLut3DEdgeLen() const
    {
        return m_impl->getLut3DEdgeLen();
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    GpuShaderDesc::Impl::Impl() :
        m_language(GPU_LANGUAGE_UNKNOWN),
        m_lut3DEdgeLen(0)
    {
    }
    
    GpuShaderDesc::Impl::~Impl()
    {
    
    }
    
    void GpuShaderDesc::Impl::setLanguage(GpuLanguage lang)
    {
        m_language = lang;
    }
    
    GpuLanguage GpuShaderDesc::Impl::getLanguage() const
    {
        return m_language;
    }
    
    void GpuShaderDesc::Impl::setFunctionName(const char * name)
    {
        m_functionName = name;
    }
    
    const char * GpuShaderDesc::Impl::getFunctionName() const
    {
        return m_functionName.c_str();
    }
    
    void GpuShaderDesc::Impl::setLut3DEdgeLen(int len)
    {
        m_lut3DEdgeLen = len;
    }
    
    int GpuShaderDesc::Impl::getLut3DEdgeLen() const
    {
        return m_lut3DEdgeLen;
    }
    
}
OCIO_NAMESPACE_EXIT
