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
#include "Mutex.h"
#include "OpBuilders.h"
#include "PathUtils.h"
#include "Processor.h"
#include "pystring/pystring.h"

#include <cstdlib>
#include <cstring>
#include <set>
#include <sstream>

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        // These are the 709 primaries specified by the ASC.
        const float DEFAULT_LUMA_COEFF_R = 0.2126f;
        const float DEFAULT_LUMA_COEFF_G = 0.7152f;
        const float DEFAULT_LUMA_COEFF_B = 0.0722f;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
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
    
    const char * Config::getDescription() const
    {
        return m_impl->getDescription();
    }
    
    void Config::setDescription(const char * description)
    {
        m_impl->setDescription(description);
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
    
    const char * Config::parseColorSpaceFromString(const char * str) const
    {
        return m_impl->parseColorSpaceFromString(str);
    }
    
    
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
    
    
    // Display Transforms
    
    int Config::getNumDisplayDeviceNames() const
    {
        return m_impl->getNumDisplayDeviceNames();
    }
    
    const char * Config::getDisplayDeviceName(int index) const
    {
        return m_impl->getDisplayDeviceName(index);
    }
    
    const char * Config::getDefaultDisplayDeviceName() const
    {
        return m_impl->getDefaultDisplayDeviceName();
    }
    
    int Config::getNumDisplayTransformNames(const char * device) const
    {
        return m_impl->getNumDisplayTransformNames(device);
    }
    
    const char * Config::getDisplayTransformName(const char * device, int index) const
    {
        return m_impl->getDisplayTransformName(device, index);
    }
    
    const char * Config::getDefaultDisplayTransformName(const char * device) const
    {
        return m_impl->getDefaultDisplayTransformName(device);
    }
    
    const char * Config::getDisplayColorSpaceName(const char * device, const char * displayTransformName) const
    {
        return m_impl->getDisplayColorSpaceName(device, displayTransformName);
    }
    
    
    
    
    void Config::getDefaultLumaCoefs(float * c3) const
    {
        m_impl->getDefaultLumaCoefs(c3);
    }
    
    void Config::setDefaultLumaCoefs(const float * c3)
    {
        m_impl->setDefaultLumaCoefs(c3);
    }
    
    ConstProcessorRcPtr Config::getProcessor(const ConstColorSpaceRcPtr & srcColorSpace,
                                             const ConstColorSpaceRcPtr & dstColorSpace) const
    {
        LocalProcessorRcPtr processor = LocalProcessor::Create();
        processor->addColorSpaceConversion(*this, srcColorSpace, dstColorSpace);
        processor->finalize();
        return processor;
    }
    
    ConstProcessorRcPtr Config::getProcessor(const ConstTransformRcPtr& transform,
                                             TransformDirection direction) const
    {
        LocalProcessorRcPtr processor = LocalProcessor::Create();
        processor->addTransform(*this, transform, direction);
        processor->finalize();
        return processor;
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
        m_defaultLumaCoefs[0] = DEFAULT_LUMA_COEFF_R;
        m_defaultLumaCoefs[1] = DEFAULT_LUMA_COEFF_G;
        m_defaultLumaCoefs[2] = DEFAULT_LUMA_COEFF_B;
    }
    
    Config::Impl::~Impl()
    {
    }
    
    Config::Impl& Config::Impl::operator= (const Config::Impl & rhs)
    {
        m_resourcePath = rhs.m_resourcePath;
        m_originalFileDir = rhs.m_originalFileDir;
        m_resolvedResourcePath = rhs.m_resolvedResourcePath;
        m_description = rhs.m_description;
        
        m_colorspaces.clear();
        m_colorspaces.reserve(rhs.m_colorspaces.size());
        
        for(unsigned int i=0; i< rhs.m_colorspaces.size(); ++i)
        {
            m_colorspaces.push_back(rhs.m_colorspaces[i]->createEditableCopy());
        }
        
        m_roleVec = rhs.m_roleVec; // Vector assignment operator will suffice for this
        m_displayDevices = rhs.m_displayDevices; // Vector assignment operator will suffice for this
        
        memcpy(m_defaultLumaCoefs, rhs.m_defaultLumaCoefs, 3*sizeof(float));
        
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
            throw Exception(os.str().c_str());
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
    
    const char * Config::Impl::getDescription() const
    {
        return m_description.c_str();
    }
    
    void Config::Impl::setDescription(const char * description)
    {
        m_description = description;
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
            throw Exception("Cannot addColorSpace with an empty name.");
        
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
            throw Exception(os.str().c_str());
        }
        
        return m_colorspaces[index];
    }
    
    ColorSpaceRcPtr Config::Impl::getEditableColorSpaceByIndex(int index)
    {
        if(index<0 || index >= (int)m_colorspaces.size())
        {
            std::ostringstream os;
            os << "Invalid ColorSpace index " << index << ".";
            throw Exception(os.str().c_str());
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
            throw Exception(os.str().c_str());
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
            throw Exception(os.str().c_str());
        }
        
        return m_colorspaces[index];
    }
    
    
    const char * Config::Impl::parseColorSpaceFromString(const char * str) const
    {
        // Search the entire filePath, including directory name (if provided)
        // convert the filename to lowercase.
        std::string fullstr = pystring::lower(std::string(str));
        
        // See if it matches a lut name.
        // This is the position of the RIGHT end of the colorspace substring, not the left
        int rightMostColorPos=-1;
        std::string rightMostColorspace = "";
        int rightMostColorSpaceIndex = -1;
        
        // Find the right-most occcurance within the string for each colorspace.
        for (unsigned int i=0; i<m_colorspaces.size(); ++i)
        {
            std::string csname = pystring::lower(m_colorspaces[i]->getName());
            
            // find right-most extension matched in filename
            int colorspacePos = pystring::rfind(fullstr, csname);
            if(colorspacePos < 0)
                continue;
            
            // If we have found a match, move the pointer over to the right end of the substring
            // This will allow us to find the longest name that matches the rightmost colorspace
            colorspacePos += (int)csname.size();
            
            if ( (colorspacePos > rightMostColorPos) ||
                 ((colorspacePos == rightMostColorPos) && (csname.size() > rightMostColorspace.size()))
                )
            {
                rightMostColorPos = colorspacePos;
                rightMostColorspace = csname;
                rightMostColorSpaceIndex = i;
            }
        }
        
        if(rightMostColorSpaceIndex<0) return "";
        return m_colorspaces[rightMostColorSpaceIndex]->getName();
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
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
        throw Exception(os.str().c_str());
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
            throw Exception(os.str().c_str());
        }
        
        return m_roleVec[index].first.c_str();
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // TODO: Use maps rather than vectors as storage for display options
    // These implementations suck in terms of scalability.
    // (If you had 100s of display devices, this would be criminally inefficient.)
    // But this can be ignored in the short-term, and doing so makes serialization
    // much simpler.
    //
    // (Alternatively, an ordered map would solve the issue too).
    //
    // m_displayDevices = [(device, name, colorspace),...]
    
    int Config::Impl::getNumDisplayDeviceNames() const
    {
        std::set<std::string> devices;
        
        for(unsigned int i=0; i<m_displayDevices.size(); ++i)
        {
            if(m_displayDevices[i].size() != 3) continue;
            devices.insert( m_displayDevices[i][0] );
        }
        
        return static_cast<int>(devices.size());
    }
    
    const char * Config::Impl::getDisplayDeviceName(int index) const
    {
        std::set<std::string> devices;
        
        for(unsigned int i=0; i<m_displayDevices.size(); ++i)
        {
            if(m_displayDevices[i].size() != 3) continue;
            devices.insert( m_displayDevices[i][0] );
            
            if((int)devices.size()-1 == index)
            {
                return m_displayDevices[i][0].c_str();
            }
        }
        
        std::ostringstream os;
        os << "Could not find display device with the specified index ";
        os << index << ".";
        throw Exception(os.str().c_str());
    }
    
    const char * Config::Impl::getDefaultDisplayDeviceName() const
    {
        if(getNumDisplayDeviceNames()>=1)
        {
            return getDisplayDeviceName(0);
        }
        
        return "";
    }
    
    int Config::Impl::getNumDisplayTransformNames(const char * device) const
    {
        std::set<std::string> names;
        
        for(unsigned int i=0; i<m_displayDevices.size(); ++i)
        {
            if(m_displayDevices[i].size() != 3) continue;
            if(m_displayDevices[i][0] != device) continue;
            names.insert( m_displayDevices[i][1] );
        }
        
        return static_cast<int>(names.size());
    }
    
    const char * Config::Impl::getDisplayTransformName(const char * device, int index) const
    {
        std::set<std::string> names;
        
        for(unsigned int i=0; i<m_displayDevices.size(); ++i)
        {
            if(m_displayDevices[i].size() != 3) continue;
            if(m_displayDevices[i][0] != device) continue;
            names.insert( m_displayDevices[i][1] );
            
            if((int)names.size()-1 == index)
            {
                return m_displayDevices[i][1].c_str();
            }
        }
        
        std::ostringstream os;
        os << "Could not find display colorspace for device ";
        os << device << " with the specified index ";
        os << index << ".";
        throw Exception(os.str().c_str());
    }
    
    const char * Config::Impl::getDefaultDisplayTransformName(const char * device) const
    {
        if(getNumDisplayTransformNames(device)>=1)
        {
            return getDisplayTransformName(device, 0);
        }
        
        return "";
    }
    
    
    const char * Config::Impl::getDisplayColorSpaceName(const char * device, const char * displayTransformName) const
    {
        for(unsigned int i=0; i<m_displayDevices.size(); ++i)
        {
            if(m_displayDevices[i].size() != 3) continue;
            if(m_displayDevices[i][0] != device) continue;
            if(m_displayDevices[i][1] != displayTransformName) continue;
            return m_displayDevices[i][2].c_str();
        }
        
        std::ostringstream os;
        os << "Could not find display colorspace for device ";
        os << device << " with the specified transform name ";
        os << displayTransformName << ".";
        throw Exception(os.str().c_str());
    }
    
    void Config::Impl::addDisplayDevice(const char * device,
                                        const char * displayTransformName,
                                        const char * csname)
    {
        // Is this device / display already registered?
        // If so, set it to the potentially new value.
        
        for(unsigned int i=0; i<m_displayDevices.size(); ++i)
        {
            if(m_displayDevices[i].size() != 3) continue;
            if(m_displayDevices[i][0] != device) continue;
            if(m_displayDevices[i][1] != displayTransformName) continue;
            
            m_displayDevices[i][2] = csname;
            return;
        }
        
        // Otherwise, add a new entry!
        DisplayKey displayKey;
        displayKey.push_back(std::string(device));
        displayKey.push_back(std::string(displayTransformName));
        displayKey.push_back(std::string(csname));
        m_displayDevices.push_back(displayKey);
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    void Config::Impl::getDefaultLumaCoefs(float * c3) const
    {
        memcpy(c3, m_defaultLumaCoefs, 3*sizeof(float));
    }
    
    void Config::Impl::setDefaultLumaCoefs(const float * c3)
    {
        memcpy(m_defaultLumaCoefs, c3, 3*sizeof(float));
    }
    
}
OCIO_NAMESPACE_EXIT
