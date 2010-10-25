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


#include <cstdlib>
#include <cstring>
#include <set>
#include <sstream>
#include <fstream>
#include <utility>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "Mutex.h"
#include "OpBuilders.h"
#include "PathUtils.h"
#include "ParseUtils.h"
#include "Processor.h"
#include "pystring/pystring.h"
#include "OCIOYaml.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        
        const char* DEFAULT_SEARCH_DIR = "$OCIO_DATA_ROOT";
        
        // These are the 709 primaries specified by the ASC.
        const float DEFAULT_LUMA_COEFF_R = 0.2126f;
        const float DEFAULT_LUMA_COEFF_G = 0.7152f;
        const float DEFAULT_LUMA_COEFF_B = 0.0722f;
        
        const char * INTERNAL_RAW_PROFILE = 
        "ocio_profile_version: 1\n"
        "strictparsing: false\n"
        "roles:\n"
        "  default: raw\n"
        "displays:\n"
        "  - !<Display> {device: sRGB, name: Raw, colorspace: raw}\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "      name: raw\n"
        "      family: raw\n"
        "      bitdepth: 32f\n"
        "      isdata: true\n"
        "      gpuallocation: uniform\n"
        "      gpumin: 0\n"
        "      gpumax: 1\n"
        "      description: 'A raw color space. Conversions to and from this space are no-ops.'\n";
        
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
    
    
    typedef std::vector<ColorSpaceRcPtr> ColorSpacePtrVec;
    typedef std::vector< std::pair<std::string, std::string> > RoleVec; // (lowercase role name, colorspace)
    typedef std::vector<std::string> DisplayKey; // (device, name, colorspace)
    
    void operator >> (const YAML::Node& node, DisplayKey& k)
    {
        if(node.GetTag() != "Display")
            return; // not a !<Display> tag
        if(node.FindValue("device")     != NULL &&
           node.FindValue("name")       != NULL &&
           node.FindValue("colorspace") != NULL)
        {
            k.push_back(node["device"].Read<std::string>());
            k.push_back(node["name"].Read<std::string>());
            k.push_back(node["colorspace"].Read<std::string>());
        }
        // else ignore node
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, DisplayKey disp) {
        if(disp.size() != 3) return out; // invalid DisplayKey
        out << YAML::VerbatimTag("Display");
        out << YAML::Flow; // on one line
        out << YAML::BeginMap;
        out << YAML::Key << "device" << YAML::Value << disp[0];
        out << YAML::Key << "name" << YAML::Value << disp[1];
        out << YAML::Key << "colorspace" << YAML::Value << disp[2];
        out << YAML::EndMap;
        return out;
    }
    
    std::string LookupRole(const RoleVec & roleVec, const std::string & rolename)
    {
        std::string s = pystring::lower(rolename);
        
        for(unsigned int i=0; i<roleVec.size(); ++i)
        {
            if(s == roleVec[i].first)
            {
                return roleVec[i].second;
            }
        }
        
        return "";
    }
    
    bool FindColorSpaceIndex(int * index,
                             const ColorSpacePtrVec & colorspaces,
                             const std::string & csname)
    {
        if(csname.empty()) return false;
        for(unsigned int i = 0; i < colorspaces.size(); ++i)
        {
            if(csname == colorspaces[i]->getName())
            {
                if(index) *index = i;
                return true;
            }
        }
        
        return false;
    }
    
    class Config::Impl
    {
    public:
        std::string originalFileDir_;
        std::string resourcePath_;
        std::string resolvedResourcePath_;
        std::string searchPath_;
        
        std::string description_;
        
        ColorSpacePtrVec colorspaces_;
        
        RoleVec roleVec_;
        
        std::vector<DisplayKey> displayDevices_;
        
        float defaultLumaCoefs_[3];
        bool strictParsing_;
        
        Impl() : 
            strictParsing_(true)
        {
            originalFileDir_ = DEFAULT_SEARCH_DIR;
            defaultLumaCoefs_[0] = DEFAULT_LUMA_COEFF_R;
            defaultLumaCoefs_[1] = DEFAULT_LUMA_COEFF_G;
            defaultLumaCoefs_[2] = DEFAULT_LUMA_COEFF_B;
        }
        
        ~Impl()
        {
        
        }
        
        Impl& operator= (const Impl & rhs)
        {
            resourcePath_ = rhs.resourcePath_;
            originalFileDir_ = rhs.originalFileDir_;
            resolvedResourcePath_ = rhs.resolvedResourcePath_;
            description_ = rhs.description_;
            
            colorspaces_.clear();
            colorspaces_.reserve(rhs.colorspaces_.size());
            
            for(unsigned int i=0; i<rhs.colorspaces_.size(); ++i)
            {
                colorspaces_.push_back(rhs.colorspaces_[i]->createEditableCopy());
            }
            
            roleVec_ = rhs.roleVec_; // Vector assignment operator will suffice for this
            displayDevices_ = rhs.displayDevices_; // Vector assignment operator will suffice for this
            
            memcpy(defaultLumaCoefs_, rhs.defaultLumaCoefs_, 3*sizeof(float));
            
            strictParsing_ = rhs.strictParsing_;
            
            return *this;
        }
        
        void load(std::istream & istream, const char * name);
    };
    
    
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
        char* file = std::getenv("OCIO");
        if(file) return CreateFromFile(file);
        
        std::ostringstream os;
        os << "Color management disabled. ";
        os << "(Specify the $OCIO environment variable to enable.)";
        ReportInfo(os.str());
        
        std::istringstream istream;
        istream.str(INTERNAL_RAW_PROFILE);
        
        ConfigRcPtr config = Config::Create();
        config->m_impl->load(istream, 0x0);
        return config;
    }
    
    ConstConfigRcPtr Config::CreateFromFile(const char * filename)
    {
        std::ifstream istream(filename);
        if(istream.fail()) {
            std::ostringstream os;
            os << "Error could not read '" << filename;
            os << "' OCIO profile.";
            throw Exception (os.str().c_str());
        }
        
        ConfigRcPtr config = Config::Create();
        config->m_impl->load(istream, filename);
        return config;
    }
    
    ConstConfigRcPtr Config::CreateFromStream(std::istream & istream)
    {
        ConfigRcPtr config = Config::Create();
        config->m_impl->load(istream, 0x0);
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
        return m_impl->resourcePath_.c_str();
    }
    
    void Config::setResourcePath(const char * path)
    {
        m_impl->resourcePath_ = path;
        m_impl->resolvedResourcePath_ = path::join(m_impl->originalFileDir_, m_impl->resourcePath_);
    }
    
    const char * Config::getResolvedResourcePath() const
    {
        return m_impl->resolvedResourcePath_.c_str();
    }
    
    ///////////////////////////////////////////////////////////////////////////
    // Search Path
    
    void Config::setSearchPath(const char * path)
    {
        m_impl->searchPath_ = path;
    }
    
    const char * Config::getSearchPath(bool expand) const
    {
        if(!expand)
            return m_impl->searchPath_.c_str();
        // expand the env vars in the search path
        EnvMap env = GetEnvMap();
        std::string searchPathExpand = m_impl->searchPath_;
        EnvExpand(&searchPathExpand, &env);
        return searchPathExpand.c_str();
    }
    
    // TODO: look at caching this look up so that it is faster, for now
    // this lookup could be slow when you have many searchpath entires and
    // many files inside these directories.
    const char * Config::findFile(const char * filename) const
    {
        // just check abs filenames
        if(pystring::startswith(filename, "/"))
        {
            if(FileExists(filename))
                return filename;
            else
                return "";
        }
        
        // expanded the searchpath string
        EnvMap env = GetEnvMap();
        std::string searchpath = m_impl->searchPath_;
        std::string profilecwd = m_impl->originalFileDir_;
        EnvExpand(&searchpath, &env);
        EnvExpand(&profilecwd, &env);
        
        // split the searchpath
        std::vector<std::string> searchpaths;
        pystring::split(searchpath, searchpaths, ":");
        
        // loop over each path and try to find the file
        for (unsigned int i = 0; i < searchpaths.size(); ++i) {
            
            // resolve '::' empty entry
            if(searchpaths[i].size() == 0)
            {
                searchpaths[i] = profilecwd;
            }
            // resolve '..'
            else if(pystring::startswith(searchpaths[i], ".."))
            {
                std::vector<std::string> result;
                pystring::rsplit(profilecwd, result, "/", 1);
                if(result.size() == 2)
                    searchpaths[i] = result[0];
            }
            // resolve '.'
            else if(pystring::startswith(searchpaths[i], "."))
            {
                searchpaths[i] = pystring::strip(searchpaths[i], ".");
                if(pystring::endswith(profilecwd, "/"))
                    searchpaths[i] = profilecwd + searchpaths[i];
                else
                    searchpaths[i] = profilecwd + "/" + searchpaths[i];
            }
            // resolve relative
            else if(!pystring::startswith(searchpaths[i], "/"))
            {
                searchpaths[i] = path::join(profilecwd, searchpaths[i]);
            }
            
            // find the file?
            std::string findfile = path::join(searchpaths[i], filename);
            if(FileExists(findfile))
                return findfile.c_str();
        }
        return ""; // didn't find a file
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    const char * Config::getDescription() const
    {
        return m_impl->description_.c_str();
    }
    
    void Config::setDescription(const char * description)
    {
        m_impl->description_ = description;
    }
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    int Config::getNumColorSpaces() const
    {
        return static_cast<int>(m_impl->colorspaces_.size());
    }
    
    const char * Config::getColorSpaceNameByIndex(int index) const
    {
        if(index<0 || index >= (int)m_impl->colorspaces_.size())
        {
            return 0x0;
        }
        
        return m_impl->colorspaces_[index]->getName();
    }
    
    ConstColorSpaceRcPtr Config::getColorSpace(const char * name) const
    {
        int index = getIndexForColorSpace(name);
        if(index<0 || index >= (int)m_impl->colorspaces_.size())
        {
            return ColorSpaceRcPtr();
        }
        
        return m_impl->colorspaces_[index];
    }
    
    ColorSpaceRcPtr Config::getEditableColorSpace(const char * name)
    {
        int index = getIndexForColorSpace(name);
        if(index<0 || index >= (int)m_impl->colorspaces_.size())
        {
            return ColorSpaceRcPtr();
        }
        
        return m_impl->colorspaces_[index];
    }
    
    int Config::getIndexForColorSpace(const char * name) const
    {
        int csindex = -1;
        
        // Check to see if the name is a color space
        if( FindColorSpaceIndex(&csindex, m_impl->colorspaces_, name) )
        {
            return csindex;
        }
        
        // Check to see if the name is a role
        std::string csname = LookupRole(m_impl->roleVec_, name);
        if( FindColorSpaceIndex(&csindex, m_impl->colorspaces_, csname) )
        {
            return csindex;
        }
        
        // Is a default role defined?
        csname = LookupRole(m_impl->roleVec_, ROLE_DEFAULT);
        if( FindColorSpaceIndex(&csindex, m_impl->colorspaces_, csname) )
        {
            return csindex;
        }
        
        return -1;
    }
    
    void Config::addColorSpace(const ConstColorSpaceRcPtr & original)
    {
        ColorSpaceRcPtr cs = original->createEditableCopy();
        
        std::string name = cs->getName();
        if(name.empty())
            throw Exception("Cannot addColorSpace with an empty name.");
        
        // Check to see if the colorspace already exists
        int csindex = -1;
        if( FindColorSpaceIndex(&csindex, m_impl->colorspaces_, name) )
        {
            m_impl->colorspaces_[csindex] = cs;
        }
        else
        {
            // Otherwise, add it
            m_impl->colorspaces_.push_back( cs );
        }
    }
    
    void Config::clearColorSpaces()
    {
        m_impl->colorspaces_.clear();
    }
    
    
    
    
    
    
    const char * Config::parseColorSpaceFromString(const char * str) const
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
        for (unsigned int i=0; i<m_impl->colorspaces_.size(); ++i)
        {
            std::string csname = pystring::lower(m_impl->colorspaces_[i]->getName());
            
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
        
        if(rightMostColorSpaceIndex>=0)
        {
            return m_impl->colorspaces_[rightMostColorSpaceIndex]->getName();
        }
        
        if(!m_impl->strictParsing_)
        {
            // Is a default role defined?
            std::string csname = LookupRole(m_impl->roleVec_, ROLE_DEFAULT);
            if(!csname.empty())
            {
                int csindex = -1;
                if( FindColorSpaceIndex(&csindex, m_impl->colorspaces_, csname) )
                {
                    // This is necessary to not return a reference to
                    // a local variable.
                    return m_impl->colorspaces_[csindex]->getName();
                }
            }
        }
        
        return 0x0;
    }
    
    bool Config::isStrictParsingEnabled() const
    {
        return m_impl->strictParsing_;
    }
    
    void Config::setStrictParsingEnabled(bool enabled)
    {
        m_impl->strictParsing_ = enabled;
    }
    
    // Roles
    void Config::setRole(const char * role, const char * colorSpaceName)
    {
        std::string rolelower = pystring::lower(role);
        
        // Set the role
        if(colorSpaceName)
        {
            for(unsigned int i=0; i<m_impl->roleVec_.size(); ++i)
            {
                if(m_impl->roleVec_[i].first == rolelower)
                {
                    m_impl->roleVec_[i].second = colorSpaceName;
                    return;
                }
            }
            m_impl->roleVec_.push_back( std::make_pair(rolelower, std::string(colorSpaceName) ) );
        }
        // Unset the role
        else
        {
            for(RoleVec::iterator iter = m_impl->roleVec_.begin();
                iter != m_impl->roleVec_.end();
                ++iter)
            {
                if(iter->first == rolelower)
                {
                    m_impl->roleVec_.erase(iter);
                    return;
                }
            }
        }
    }
    
    int Config::getNumRoles() const
    {
        return static_cast<int>(m_impl->roleVec_.size());
    }
    
    const char * Config::getRoleNameByIndex(int index) const
    {
        if(index<0 || index >= (int)m_impl->roleVec_.size())
        {
            return 0x0;
        }
        
        return m_impl->roleVec_[index].first.c_str();
    }
    
    
    // Display Transforms
    
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
    
    int Config::getNumDisplayDeviceNames() const
    {
        std::set<std::string> devices;
        
        for(unsigned int i=0; i<m_impl->displayDevices_.size(); ++i)
        {
            if(m_impl->displayDevices_[i].size() != 3) continue;
            devices.insert( m_impl->displayDevices_[i][0] );
        }
        
        return static_cast<int>(devices.size());
    }
    
    const char * Config::getDisplayDeviceName(int index) const
    {
        std::set<std::string> devices;
        
        for(unsigned int i=0; i<m_impl->displayDevices_.size(); ++i)
        {
            if(m_impl->displayDevices_[i].size() != 3) continue;
            devices.insert( m_impl->displayDevices_[i][0] );
            
            if((int)devices.size()-1 == index)
            {
                return m_impl->displayDevices_[i][0].c_str();
            }
        }
        
        return "";
    }
    
    const char * Config::getDefaultDisplayDeviceName() const
    {
        if(getNumDisplayDeviceNames()>=1)
        {
            return getDisplayDeviceName(0);
        }
        
        return "";
    }
    
    int Config::getNumDisplayTransformNames(const char * device) const
    {
        std::set<std::string> names;
        
        for(unsigned int i=0; i<m_impl->displayDevices_.size(); ++i)
        {
            if(m_impl->displayDevices_[i].size() != 3) continue;
            if(m_impl->displayDevices_[i][0] != device) continue;
            names.insert( m_impl->displayDevices_[i][1] );
        }
        
        return static_cast<int>(names.size());
    }
    
    const char * Config::getDisplayTransformName(const char * device,
        int index) const
    {
        std::set<std::string> names;
        
        for(unsigned int i=0; i<m_impl->displayDevices_.size(); ++i)
        {
            if(m_impl->displayDevices_[i].size() != 3) continue;
            if(m_impl->displayDevices_[i][0] != device) continue;
            names.insert( m_impl->displayDevices_[i][1] );
            
            if((int)names.size()-1 == index)
            {
                return m_impl->displayDevices_[i][1].c_str();
            }
        }
        
        return "";
    }
    
    const char * Config::getDefaultDisplayTransformName(const char * device) const
    {
        if(getNumDisplayTransformNames(device)>=1)
        {
            return getDisplayTransformName(device, 0);
        }
        
        return "";
    }
    
    const char * Config::getDisplayColorSpaceName(const char * device,
        const char * displayTransformName) const
    {
        for(unsigned int i=0; i<m_impl->displayDevices_.size(); ++i)
        {
            if(m_impl->displayDevices_[i].size() != 3) continue;
            if(m_impl->displayDevices_[i][0] != device) continue;
            if(m_impl->displayDevices_[i][1] != displayTransformName) continue;
            return m_impl->displayDevices_[i][2].c_str();
        }
        
        return "";
    }
    
    void Config::addDisplayDevice(const char * device,
                                        const char * displayTransformName,
                                        const char * csname)
    {
        // Is this device / display already registered?
        // If so, set it to the potentially new value.
        
        for(unsigned int i=0; i<m_impl->displayDevices_.size(); ++i)
        {
            if(m_impl->displayDevices_[i].size() != 3) continue;
            if(m_impl->displayDevices_[i][0] != device) continue;
            if(m_impl->displayDevices_[i][1] != displayTransformName) continue;
            
            m_impl->displayDevices_[i][2] = csname;
            return;
        }
        
        // Otherwise, add a new entry!
        DisplayKey displayKey;
        displayKey.push_back(std::string(device));
        displayKey.push_back(std::string(displayTransformName));
        displayKey.push_back(std::string(csname));
        m_impl->displayDevices_.push_back(displayKey);
    }
    
    void Config::getDefaultLumaCoefs(float * c3) const
    {
        memcpy(c3, m_impl->defaultLumaCoefs_, 3*sizeof(float));
    }
    
    void Config::setDefaultLumaCoefs(const float * c3)
    {
        memcpy(m_impl->defaultLumaCoefs_, c3, 3*sizeof(float));
    }
    
    ConstProcessorRcPtr Config::getProcessor(const ConstColorSpaceRcPtr & src,
                                             const ConstColorSpaceRcPtr & dst) const
    {
        if(!src)
        {
            throw Exception("Config::GetProcessor failed. Source colorspace is null.");
        }
        if(!dst)
        {
            throw Exception("Config::GetProcessor failed. Destination colorspace is null.");
        }
        
        LocalProcessorRcPtr processor = LocalProcessor::Create();
        processor->addColorSpaceConversion(*this, src, dst);
        processor->finalize();
        return processor;
    }
    
    //! Names can be colorspace name or role name
    ConstProcessorRcPtr Config::getProcessor(const char * srcName,
                                             const char * dstName) const
    {
        // TODO: Confirm !RcPtr works
        ConstColorSpaceRcPtr src = getColorSpace(srcName);
        if(!src)
        {
            std::ostringstream os;
            os << "Could not find colorspace '" << srcName << ".";
            throw Exception(os.str().c_str());
        }
        
        ConstColorSpaceRcPtr dst = getColorSpace(dstName);
        if(!dst)
        {
            std::ostringstream os;
            os << "Could not find colorspace '" << dstName << ".";
            throw Exception(os.str().c_str());
        }
        
        return getProcessor(src, dst);
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
        config.serialize(os);
        return os;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //  Serialization
    
    void Config::serialize(std::ostream& os) const
    {
        try
        {
            // serialize the config
            YAML::Emitter out;
            out << YAML::Block;
            out << YAML::BeginMap;
            out << YAML::Key << "ocio_profile_version" << YAML::Value << 1;
            if(!m_impl->resourcePath_.empty())
            {
                out << YAML::Key << "resource_path" << YAML::Value << m_impl->resourcePath_;
            }
            out << YAML::Key << "strictparsing" << YAML::Value << m_impl->strictParsing_;
            // TODO: should we make defaultLumaCoefs_ a std::vector<float> to make it easier
            //       to serialize?
            out << YAML::Key << "luma" << YAML::Value;
            {
                out << YAML::Flow;
                out << YAML::BeginSeq;
                for(unsigned int i = 0; i < 3; ++i)
                    out << m_impl->defaultLumaCoefs_[i];
                out << YAML::EndSeq;
            }
            if(m_impl->description_.empty())
                out << YAML::Key << "description" << YAML::Value << m_impl->description_;
            
            // Roles
            if(m_impl->roleVec_.size() > 0)
            {
                out << YAML::Key << "roles" << YAML::Value;
                out << YAML::BeginMap;
                for(unsigned int i=0; i<m_impl->roleVec_.size(); ++i)
                {
                    out << YAML::Key   << m_impl->roleVec_[i].first;
                    out << YAML::Value << m_impl->roleVec_[i].second;
                }
                out << YAML::EndMap;
            }
            
            // Displays
            if(m_impl->displayDevices_.size() > 0)
            {
                out << YAML::Key << "displays" << YAML::Value;
                out << YAML::BeginSeq;
                for(unsigned int i=0; i<m_impl->displayDevices_.size(); ++i)
                    out << m_impl->displayDevices_[i];
                out << YAML::EndSeq;
            }
            
            // ColorSpaces
            if(m_impl->colorspaces_.size() > 0)
            {
                out << YAML::Key << "colorspaces";
                out << YAML::Value << m_impl->colorspaces_; // std::vector -> Seq
            }
            
            out << YAML::EndMap;
            os << out.c_str();
        
        }
        catch( const std::exception & e)
        {
            std::ostringstream error;
            error << "Error building YAML: " << e.what();
            throw Exception(error.str().c_str());
        }
    }
    
    void Config::Impl::load(std::istream & istream, const char * filename)
    {
        try
        {
            YAML::Parser parser(istream);
            YAML::Node node;
            parser.GetNextDocument(node);
            
            // check profile version
            int profile_version;
            if(node.FindValue("ocio_profile_version") == NULL)
            {
                std::ostringstream os;
                os << "The specified file ";
                os << "does not appear to be an OCIO configuration.";
                throw Exception (os.str().c_str());
            }
            
            node["ocio_profile_version"] >> profile_version;
            if(profile_version != 1)
            {
                std::ostringstream os;
                os << "version " << profile_version << " is not ";
                os << "supported by OCIO v" << OCIO_VERSION ".";
                throw Exception (os.str().c_str());
            }
            
            // cast YAML nodes to Impl members
            if(node.FindValue("resource_path") != NULL)
                node["resource_path"] >> resourcePath_;
            if(node.FindValue("strictparsing") != NULL)
                node["strictparsing"] >> strictParsing_;
            if(node.FindValue("description") != NULL)
                node["description"] >> description_;
            
            // Luma
            if(node.FindValue("luma") != NULL)
            {
                if(node["luma"].GetType() != YAML::CT_SEQUENCE)
                {
                    std::ostringstream os;
                    os << "'luma' field needs to be a (luma: [0, 0, 0]) list.";
                    throw Exception(os.str().c_str());
                }
                std::vector<float> value;
                node["luma"] >> value;
                if(value.size() != 3)
                {
                    std::ostringstream os;
                    os << "'luma' field must be 3 ";
                    os << "floats. Found '" << value.size() << "'.";
                    throw Exception(os.str().c_str());
                }
                defaultLumaCoefs_[0] = value[0];
                defaultLumaCoefs_[1] = value[1];
                defaultLumaCoefs_[2] = value[2];
            }
            
            // Roles
            // TODO: We should really output roles in a dictionary
            // (alphabetical) order, or a roundtrip yaml file wont
            // agree with the original, in role ordering. (Is it
            //  precious or not?). Alternative is to store roles
            // in a map, rather than a vec.
            
            if(node.FindValue("roles") != NULL) {
                if(node["roles"].GetType() != YAML::CT_MAP)
                {
                    std::ostringstream os;
                    os << "'roles' field needs to be a (name: key) map.";
                    throw Exception(os.str().c_str());
                }
                for (YAML::Iterator it  = node["roles"].begin();
                                    it != node["roles"].end(); ++it)
                {
                    const std::string key = it.first();
                    const std::string value = it.second();
                    roleVec_.push_back(std::make_pair(key, value));
                }
            
            } else {
                // TODO: does it matter if there are no roles defined?
            }
            
            // Displays
            if(node.FindValue("displays") != NULL) {
                if(node["displays"].GetType() != YAML::CT_SEQUENCE)
                {
                    std::ostringstream os;
                    os << "'displays' field needs to be a (- !<Display>) list.";
                    throw Exception(os.str().c_str());
                }
                for(unsigned i = 0; i < node["displays"].size(); ++i)
                {
                    if(node["displays"][i].GetTag() == "Display")
                    {
                        DisplayKey tmp;
                        node["displays"][i] >> tmp;
                        displayDevices_.push_back(tmp);
                    }
                    // ignore other tags
                    // TODO: print something or set some defaults in this case
                }
            }
            else
            {
                // TODO: does it matter if there are no displays defined?
            }
            
            // ColorSpaces
            if(node.FindValue("colorspaces") != NULL) {
                if(node["colorspaces"].GetType() != YAML::CT_SEQUENCE)
                {
                    std::ostringstream os;
                    os << "'colorspaces' field needs to be a (- !<ColorSpace>) list.";
                    throw Exception(os.str().c_str());
                }
                for(unsigned i = 0; i < node["colorspaces"].size(); ++i)
                {
                    if(node["colorspaces"][i].GetTag() == "ColorSpace")
                    {
                        ColorSpaceRcPtr cs = ColorSpace::Create();
                        node["colorspaces"][i] >> cs;
                        colorspaces_.push_back( cs );
                    }
                    // ignore other tags
                    // TODO: print something in this case
                }
            }
            else
            {
                // TODO: does it matter if there are no colorspaces defined?
            }
            
            // These are defined to allow for relative resource paths
            // in FileTransforms.
            if(filename)
            {
                originalFileDir_ = path::dirname(filename);
                resolvedResourcePath_ = path::join(originalFileDir_, resourcePath_);
            }
            else
            {
                originalFileDir_ = "";
                resolvedResourcePath_ = "";
            }
        }
        catch( const std::exception & e)
        {
            std::ostringstream os;
            os << "Error: OCIO profile '";
            if(filename) os << ", " << filename << "', ";
            os << "load failed: " << e.what();
            throw Exception(os.str().c_str());
        }
    }
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

#include <sys/stat.h>

BOOST_AUTO_TEST_SUITE( Config_Unit_Tests )

BOOST_AUTO_TEST_CASE ( test_searchpath_filesystem )
{
    
    OCIO::EnvMap env = OCIO::GetEnvMap();
    std::string OCIO_TEST_AREA("$OCIO_TEST_AREA");
    EnvExpand(&OCIO_TEST_AREA, &env);
    
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    
    // basic get/set/expand
    config->setSearchPath("."
                          ":$OCIO_TEST1"
                          ":/$OCIO_JOB/${OCIO_SEQ}/$OCIO_SHOT/ocio");
    
    BOOST_CHECK_EQUAL(config->getSearchPath(),
        ".:$OCIO_TEST1:/$OCIO_JOB/${OCIO_SEQ}/$OCIO_SHOT/ocio");
    BOOST_CHECK_EQUAL(config->getSearchPath(true),
        ".:foobar:/meatballs/cheesecake/mb-cc-001/ocio");
    
    // find some files
    config->setSearchPath(".."
                          ":$OCIO_TEST1"
                          ":${OCIO_TEST_AREA}/test_search/one"
                          ":$OCIO_TEST_AREA/test_search/two");
    
    // setup for search test
    std::string base_dir("$OCIO_TEST_AREA/test_search/");
    EnvExpand(&base_dir, &env);
    mkdir(base_dir.c_str(), 0777);
    
    std::string one_dir("$OCIO_TEST_AREA/test_search/one/");
    EnvExpand(&one_dir, &env);
    mkdir(one_dir.c_str(), 0777);
    
    std::string two_dir("$OCIO_TEST_AREA/test_search/two/");
    EnvExpand(&two_dir, &env);
    mkdir(two_dir.c_str(), 0777);
    
    std::string lut1(one_dir+"somelut1.lut");
    std::ofstream somelut1(lut1.c_str());
    somelut1.close();
    
    std::string lut2(two_dir+"somelut2.lut");
    std::ofstream somelut2(lut2.c_str());
    somelut2.close();
    
    std::string lut3(two_dir+"somelut3.lut");
    std::ofstream somelut3(lut3.c_str());
    somelut3.close();
    
    std::string lutdotdot(OCIO_TEST_AREA+"/lutdotdot.lut");
    std::ofstream somelutdotdot(lutdotdot.c_str());
    somelutdotdot.close();
    
    // basic search test
    BOOST_CHECK_EQUAL(config->findFile("somelut1.lut"), lut1.c_str());
    BOOST_CHECK_EQUAL(config->findFile("somelut2.lut"), lut2.c_str());
    BOOST_CHECK_EQUAL(config->findFile("somelut3.lut"), lut3.c_str());
    BOOST_CHECK_EQUAL(config->findFile("lutdotdot.lut"), lutdotdot.c_str());
    
}

BOOST_AUTO_TEST_CASE ( test_INTERNAL_RAW_PROFILE )
{
    std::istringstream is;
    is.str(OCIO::INTERNAL_RAW_PROFILE);
    
    BOOST_CHECK_NO_THROW(OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromStream(is));
}

BOOST_AUTO_TEST_CASE ( test_simpleConfig )
{
    
    std::string SIMPLE_PROFILE =
    "ocio_profile_version: 1\n"
    "resource_path: luts\n"
    "strictparsing: false\n"
    "luma: [0.2126, 0.7152, 0.0722]\n"
    "roles:\n"
    "  compositing_log: lgh\n"
    "  default: raw\n"
    "  scene_linear: lnh\n"
    "displays:\n"
    "  - !<Display> {device: sRGB, name: Film1D, colorspace: vd8}\n"
    "  - !<Display> {device: sRGB, name: Log, colorspace: lg10}\n"
    "  - !<Display> {device: sRGB, name: Raw, colorspace: raw}\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "      name: raw\n"
    "      family: raw\n"
    "      bitdepth: 32f\n"
    "      description: |\n"
    "        A raw color space. Conversions to and from this space are no-ops.\n"
    "      isdata: true\n"
    "      gpuallocation: uniform\n"
    "      gpumin: 0\n"
    "      gpumax: 1\n"
    "  - !<ColorSpace>\n"
    "      name: lnh\n"
    "      family: ln\n"
    "      bitdepth: 16f\n"
    "      description: |\n"
    "        The show reference space. This is a sensor referred linear\n"
    "        representation of the scene with primaries that correspond to\n"
    "        scanned film. 0.18 in this space corresponds to a properly\n"
    "        exposed 18% grey card.\n"
    "      isdata: false\n"
    "      gpuallocation: lg2\n"
    "      gpumin: -15\n"
    "      gpumax: 6\n"
    "  - !<ColorSpace>\n"
    "      name: loads_of_transforms\n"
    "      family: vd8\n"
    "      bitdepth: 8ui\n"
    "      description: 'how many transforms can we use?'\n"
    "      isdata: false\n"
    "      gpuallocation: uniform\n"
    "      gpumin: 0\n"
    "      gpumax: 1\n"
    "      to_reference: !<GroupTransform>\n"
    "        direction: forward\n"
    "        children:\n"
    "          - !<FileTransform>\n"
    "            src: diffusemult.spimtx\n"
    "            interpolation: unknown\n"
    "          - !<ColorSpaceTransform>\n"
    "            src: vd8\n"
    "            dst: lnh\n"
    "          - !<ExponentTransform>\n"
    "            value: [2.2, 2.2, 2.2, 1]\n"
    "          - !<CineonLogToLinTransform>\n"
    "            max_aim_density: [2.046, 2.046, 2.046]\n"
    "            neg_gamma: [0.6, 0.6, 0.6]\n"
    "            neg_gray_reference: [0.435, 0.435, 0.435]\n"
    "            linear_gray_reference: [0.18, 0.18, 0.18]\n"
    "          - !<MatrixTransform>\n"
    "            matrix: [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]\n"
    "            offset: [0, 0, 0, 0]\n"
    "          - !<CDLTransform>\n"
    "            slope: [1, 1, 1]\n"
    "            offset: [0, 0, 0]\n"
    "            power: [1, 1, 1]\n"
    "            saturation: 1\n"
    "\n";
    
    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    BOOST_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
}

BOOST_AUTO_TEST_SUITE_END()

#endif // OCIO_UNIT_TEST
