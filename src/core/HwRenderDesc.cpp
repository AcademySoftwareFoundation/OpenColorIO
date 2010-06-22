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

#include <OpenColorSpace/OpenColorSpace.h>

#include "HwRenderDesc.h"

OCS_NAMESPACE_ENTER
{
    
        
    HwRenderDesc::HwRenderDesc()
    : m_impl(new HwRenderDesc::Impl)
    {
    }
    
    HwRenderDesc::~HwRenderDesc()
    {
    }
    
    
    void HwRenderDesc::setLut3DEdgeSize(int size)
    {
        m_impl->setLut3DEdgeSize(size);
    }
    
    int HwRenderDesc::getLut3DEdgeSize() const
    {
        return m_impl->getLut3DEdgeSize();
    }
    
    void HwRenderDesc::setShaderFunctionName(const char * name)
    {
        m_impl->setShaderFunctionName(name);
    }
    
    const char * HwRenderDesc::getShaderFunctionName() const
    {
        return m_impl->getShaderFunctionName();
    }
    
    void HwRenderDesc::setHwLanguage(HwLanguage lang)
    {
        m_impl->setHwLanguage(lang);
    }
    
    HwLanguage HwRenderDesc::getHwLanguage() const
    {
        return m_impl->getHwLanguage();
    }
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    HwRenderDesc::Impl::Impl() :
        m_lut3DEdgeSize(0),
        m_hwLanguage(HW_LANGUAGE_UNKNOWN)
    {
    }
    
    HwRenderDesc::Impl::~Impl()
    {
    
    }
    
    void HwRenderDesc::Impl::setLut3DEdgeSize(int size)
    {
        m_lut3DEdgeSize = size;
    }
    
    int HwRenderDesc::Impl::getLut3DEdgeSize() const
    {
        return m_lut3DEdgeSize;
    }
    
    void HwRenderDesc::Impl::setShaderFunctionName(const char * name)
    {
        m_shaderFunctionName = name;
    }
    
    const char * HwRenderDesc::Impl::getShaderFunctionName() const
    {
        return m_shaderFunctionName.c_str();
    }
    
    void HwRenderDesc::Impl::setHwLanguage(HwLanguage lang)
    {
        m_hwLanguage = lang;
    }
    
    HwLanguage HwRenderDesc::Impl::getHwLanguage() const
    {
        return m_hwLanguage;
    }
    
    
}
OCS_NAMESPACE_EXIT
