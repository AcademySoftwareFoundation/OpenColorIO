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

#include "Config.h"
#include "pystring/pystring.h"
#include "Mutex.h"
#include "PathUtils.h"
#include "Processor.h"

#include <cstdlib>
#include <sstream>

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        ConstConfigRcPtr g_currentConfig;
        Mutex g_currentConfigLock;
    }
    
    ConstConfigRcPtr GetCurrentConfig()
    {
        AutoMutex lock(g_currentConfigLock);
        
        if(!g_currentConfig)
        {
            g_currentConfig = Config::CreateFromEnv();
        }
        
        return g_currentConfig;
    }
    
    void SetCurrentConfig(const ConstConfigRcPtr & config)
    {
        AutoMutex lock(g_currentConfigLock);
        
        g_currentConfig = config->createEditableCopy();
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    ConfigRcPtr Config::Create()
    {
        return ConfigRcPtr(new Config(), &deleter);
    }
    
    void Config::deleter(Config* c)
    {
        delete c;
    }
    
    ConstConfigRcPtr Config::CreateFromEnv()
    {
        ConfigRcPtr config = Config::Create();
        config->m_impl->createFromEnv();
        return config;
    }
    
    ConstConfigRcPtr Config::CreateFromFile(const char * filename)
    {
        ConfigRcPtr config = Config::Create();
        config->m_impl->createFromFile(filename);
        return config;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    Config::Config()
    : m_impl(new Config::Impl)
    {
    }
    
    Config::~Config()
    {
    }
    
    ConfigRcPtr Config::createEditableCopy() const
    {
        ConfigRcPtr config = Config::Create();
        *config->m_impl = *m_impl;
        return config;
    }
    
    const char * Config::getResourcePath() const
    {
        return m_impl->getResourcePath();
    }
    
    void Config::setResourcePath(const char * path)
    {
        m_impl->setResourcePath(path);
    }
    
    const char * Config::getResolvedResourcePath() const
    {
        return m_impl->getResolvedResourcePath();
    }
    
    void Config::writeXML(std::ostream& os) const
    {
        m_impl->writeXML(os);
    }
    
    int Config::getNumColorSpaces() const
    {
        return m_impl->getNumColorSpaces();
    }
    
    int Config::getIndexForColorSpace(const char * name) const
    {
        return m_impl->getIndexForColorSpace(name);
    }
    
    // if another colorspace was already registered with the
    // same name, this will overwrite it.
    // Stores the live reference to this colorspace
    
    void Config::addColorSpace(ColorSpaceRcPtr cs)
    {
        m_impl->addColorSpace(cs);
    }
    
    void Config::addColorSpace(const ConstColorSpaceRcPtr & cs)
    {
        m_impl->addColorSpace(cs);
    }
    
    void Config::clearColorSpaces()
    {
        m_impl->clearColorSpaces();
    }
    
    ConstColorSpaceRcPtr Config::getColorSpaceByIndex(int index) const
    {
        return m_impl->getColorSpaceByIndex(index);
    }
    
    ColorSpaceRcPtr Config::getEditableColorSpaceByIndex(int index)
    {
        return m_impl->getEditableColorSpaceByIndex(index);
    }
    
    ConstColorSpaceRcPtr Config::getColorSpaceByName(const char * name) const
    {
        return m_impl->getColorSpaceByName(name);
    }
    
    ColorSpaceRcPtr Config::getEditableColorSpaceByName(const char * name)
    {
        return m_impl->getEditableColorSpaceByName(name);
    }
    
    
    
    /*
    int Config::getNumColorSpaceFamilies() const
    {
        return m_impl->getNumColorSpaceFamilies();
    }
    
    const char * Config::getColorSpaceFamily(int index) const
    {
        return m_impl->getColorSpaceFamily(index);
    }

    int Config::getNumColorSpacesInFamily(const char * family) const
    {
        return m_impl->getNumColorSpacesInFamily(family);
    }
    
    const ColorSpace& Config::getColorSpaceInFamily(const char * family, int index) const
    {
        return m_impl->getColorSpaceInFamily(family, index);
    }
    
    const ColorSpace& Config::parseColorSpace(const char * userString,
                                              const char * tokensToSplit) const
    {
        return m_impl->parseColorSpace(userString, tokensToSplit);
    }
    
    */
    
    // Roles
    
    ConstColorSpaceRcPtr Config::getColorSpaceForRole(const char * role) const
    {
        return m_impl->getColorSpaceForRole(role);
    }
    void Config::setColorSpaceForRole(const char * role, const char * name)
    {
        m_impl->setColorSpaceForRole(role, name);
    }
    
    void Config::unsetRole(const char * role)
    {
        m_impl->unsetRole(role);
    }
    
    int Config::getNumRoles() const
    {
        return m_impl->getNumRoles();
    }
    
    const char * Config::getRole(int index) const
    {
        return m_impl->getRole(index);
    }
    
    
    // TODO: Add GPU Allocation hints into opstream. Possible to autocompute?
    
    ConstProcessorRcPtr Config::getProcessor(const ConstColorSpaceRcPtr & srcColorSpace,
                                             const ConstColorSpaceRcPtr & dstColorSpace) const
    {
        // All colorspace conversions within the same family are no-ops
        std::string srcFamily = srcColorSpace->getFamily();
        std::string dstFamily = dstColorSpace->getFamily();
        
        if(srcFamily == dstFamily)
        {
            OpRcPtrVec opVec;
            return LocalProcessor::Create(opVec);
        }
        
        // Create a combined transform group
        GroupTransformRcPtr combinedTransform = GroupTransform::Create();
        combinedTransform->push_back( srcColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE) );
        combinedTransform->push_back( dstColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE) );
        
        return getProcessor( combinedTransform );
    }
    
    // Individual lut application functions
    // Can be used to apply a .lut, .dat, .lut3d, or .3dl file.
    // Not generally needed, but useful in testing.
    
    ConstProcessorRcPtr Config::getProcessor(const ConstTransformRcPtr& transform,
                                             TransformDirection direction) const
    {
        // TODO: thread safety during build ops?
        OpRcPtrVec opVec;
        BuildOps(&opVec, *this, transform, direction);
        
        // TODO: Perform smart optimizations / collapsing on the OpVec
        
        // After construction, finalize the setup
        for(unsigned int i=0; i<opVec.size(); ++i)
        {
            opVec[i]->setup();
        }
        
        return LocalProcessor::Create(opVec);
    }


    
    
    
    
    std::ostream& operator<< (std::ostream& os, const Config& config)
    {
        config.writeXML(os);
        return os;
    }
    
    
    
    
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // Impl
    
    
    Config::Impl::Impl()
    {
    }
    
    Config::Impl::~Impl()
    {
    }
    
    Config::Impl& Config::Impl::operator= (const Config::Impl & rhs)
    {
        m_resourcePath = rhs.m_resourcePath;
        m_originalFileDir = rhs.m_originalFileDir;
        m_resolvedResourcePath = rhs.m_resolvedResourcePath;
        
        m_colorspaces.clear();
        m_colorspaces.reserve(rhs.m_colorspaces.size());
        
        for(unsigned int i=0; i< rhs.m_colorspaces.size(); ++i)
        {
            m_colorspaces.push_back(rhs.m_colorspaces[i]->createEditableCopy());
        }
        
        // Vector assignment operator will suffice for the
        // role data types
        
        m_roleVec = rhs.m_roleVec;
        
        return *this;
    }
    
    
    void Config::Impl::createFromEnv()
    {
        char * file = std::getenv("OCIO");
        if(file)
        {
            createFromFile(file);
        }
        else
        {
            std::ostringstream os;
            os << "'OCIO' environment variable not set. ";
            os << "Please specify a valid OpenColorIO (.ocio) configuration file.";
            throw OCIOException(os.str().c_str());
        }
    }
    
    
    const char * Config::Impl::getResourcePath() const
    {
        return m_resourcePath.c_str();
    }
    
    void Config::Impl::setResourcePath(const char * path)
    {
        m_resourcePath = path;
        m_resolvedResourcePath = path::join(m_originalFileDir, m_resourcePath);
    }
    
    const char * Config::Impl::getResolvedResourcePath() const
    {
        return m_resolvedResourcePath.c_str();
    }
    
    int Config::Impl::getNumColorSpaces() const
    {
        return static_cast<int>(m_colorspaces.size());
    }
    
    
    int Config::Impl::getIndexForColorSpace(const char * name) const
    {
        std::string strname = name;
        if(strname.empty()) return -1;
        
        // Check to see if the colorspace already exists at a known index.
        for(unsigned int index = 0; index < m_colorspaces.size(); ++index)
        {
            if(strname != m_colorspaces[index]->getName())
                continue;
            
            return index;
        }
        
        return -1;
    }
    
    void Config::Impl::addColorSpace(ColorSpaceRcPtr cs)
    {
        std::string name = cs->getName();
        if(name.empty())
            throw OCIOException("Cannot addColorSpace with an empty name.");
        
        // Check to see if the colorspace already exists at a known index.
        int index = getIndexForColorSpace( cs->getName() );
        if(index != -1)
        {
            m_colorspaces[index] = cs;
            return;
        }
        
        m_colorspaces.push_back( cs );
    }
    
    void Config::Impl::addColorSpace(const ConstColorSpaceRcPtr & cs)
    {
        addColorSpace(cs->createEditableCopy());
    }
    
    void Config::Impl::clearColorSpaces()
    {
        m_colorspaces.clear();
    }
    
    ConstColorSpaceRcPtr Config::Impl::getColorSpaceByIndex(int index) const
    {
        if(index<0 || index >= (int)m_colorspaces.size())
        {
            std::ostringstream os;
            os << "Invalid ColorSpace index " << index << ".";
            throw OCIOException(os.str().c_str());
        }
        
        return m_colorspaces[index];
    }
    
    ColorSpaceRcPtr Config::Impl::getEditableColorSpaceByIndex(int index)
    {
        if(index<0 || index >= (int)m_colorspaces.size())
        {
            std::ostringstream os;
            os << "Invalid ColorSpace index " << index << ".";
            throw OCIOException(os.str().c_str());
        }
        
        return m_colorspaces[index];
    }
    
    ConstColorSpaceRcPtr Config::Impl::getColorSpaceByName(const char * name) const
    {
        int index = getIndexForColorSpace( name );
        if(index == -1)
        {
            std::ostringstream os;
            os << "Cannot find ColorSpace named '";
            os << name << "'.";
            throw OCIOException(os.str().c_str());
        }
        
        return m_colorspaces[index];
    }
    
    ColorSpaceRcPtr Config::Impl::getEditableColorSpaceByName(const char * name)
    {
        int index = getIndexForColorSpace( name );
        if(index == -1)
        {
            std::ostringstream os;
            os << "Cannot find ColorSpace named '";
            os << name << "'.";
            throw OCIOException(os.str().c_str());
        }
        
        return m_colorspaces[index];
    }
    
    /*
    int Config::Impl::getNumColorSpaceFamilies() const
    {
        return 0;
    }
    
    const char * Config::Impl::getColorSpaceFamily(int index) const
    {
        return "";
    }
    
    int Config::Impl::getNumColorSpacesInFamily(const char * family) const
    {
        return 0;
    }
    
    const ColorSpace& Config::Impl::getColorSpaceInFamily(const char * family,
                                                        int index) const
    {
    }
    
    const ColorSpace& Config::Impl::parseColorSpace(const char * userString,
                                                  const char * tokensToSplit) const
    {
    }
    */
    
    
    // Roles
    ConstColorSpaceRcPtr Config::Impl::getColorSpaceForRole(const char * role) const
    {
        std::string rolelower = pystring::lower(role);
        
        for(unsigned int i=0; i<m_roleVec.size(); ++i)
        {
            if(m_roleVec[i].first == rolelower)
            {
                return getColorSpaceByName(m_roleVec[i].second.c_str());
            }
        }
        
        std::ostringstream os;
        os << "The specified role ";
        os  << role ;
        os << " has not been defined.";
        throw OCIOException(os.str().c_str());
    }
    
    void Config::Impl::setColorSpaceForRole(const char * role, const char * csname)
    {
        std::string rolelower = pystring::lower(role);
        
        for(unsigned int i=0; i<m_roleVec.size(); ++i)
        {
            if(m_roleVec[i].first == rolelower)
            {
                m_roleVec[i].second = csname;
                return;
            }
        }
        
        m_roleVec.push_back( std::make_pair(rolelower, std::string(csname) ) );
    }
    
    void Config::Impl::unsetRole(const char * role)
    {
        std::string rolelower = pystring::lower(role);
        
        for(RoleVec::iterator iter = m_roleVec.begin();
            iter != m_roleVec.end();
            ++iter)
        {
            if(iter->first == rolelower)
            {
                m_roleVec.erase(iter);
                return;
            }
        }
    }
    
    int Config::Impl::getNumRoles() const
    {
        return static_cast<int>(m_roleVec.size());
    }
    
    const char * Config::Impl::getRole(int index) const
    {
        if(index<0 || index >= (int)m_roleVec.size())
        {
            std::ostringstream os;
            os << "Invalid role index " << index << ".";
            throw OCIOException(os.str().c_str());
        }
        
        return m_roleVec[index].first.c_str();
    }
}
OCIO_NAMESPACE_EXIT
