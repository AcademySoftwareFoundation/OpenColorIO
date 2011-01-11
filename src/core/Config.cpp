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
        enum Sanity
        {
            SANITY_UNKNOWN = 0,
            SANITY_SANE,
            SANITY_INSANE
        };
        
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
    typedef std::map<std::string, std::string> RoleMap; // (lower case role name: colorspace name)
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
    
    std::string LookupRole(const RoleMap & roles, const std::string & rolename)
    {
        RoleMap::const_iterator iter = roles.find(pystring::lower(rolename));
        if(iter == roles.end()) return "";
        return iter->second;
    }
    
    bool FindColorSpaceIndex(int * index,
                             const ColorSpacePtrVec & colorspaces,
                             const std::string & csname)
    {
        if(csname.empty()) return false;
        
        std::string csnamelower = pystring::lower(csname);
        
        for(unsigned int i = 0; i < colorspaces.size(); ++i)
        {
            if(csnamelower == pystring::lower(colorspaces[i]->getName()))
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
        ContextRcPtr context_;
        
        std::string description_;
        
        ColorSpacePtrVec colorspaces_;
        
        RoleMap roles_;
        
        std::vector<DisplayKey> displayDevices_;
        std::vector<float> defaultLumaCoefs_;
        bool strictParsing_;
        
        mutable Sanity sanity_;
        mutable std::string sanitytext_;
        
        Impl() : 
            context_(Context::Create()),
            strictParsing_(true),
            sanity_(SANITY_UNKNOWN)
        {
            context_->loadEnvironment();
            
            defaultLumaCoefs_.resize(3);
            defaultLumaCoefs_[0] = DEFAULT_LUMA_COEFF_R;
            defaultLumaCoefs_[1] = DEFAULT_LUMA_COEFF_G;
            defaultLumaCoefs_[2] = DEFAULT_LUMA_COEFF_B;
        }
        
        ~Impl()
        {
        
        }
        
        Impl& operator= (const Impl & rhs)
        {
            context_ = rhs.context_->createEditableCopy();
            description_ = rhs.description_;
            
            colorspaces_.clear();
            colorspaces_.reserve(rhs.colorspaces_.size());
            
            for(unsigned int i=0; i<rhs.colorspaces_.size(); ++i)
            {
                colorspaces_.push_back(rhs.colorspaces_[i]->createEditableCopy());
            }
            
            roles_ = rhs.roles_; // Map assignment operator will suffice for this
            displayDevices_ = rhs.displayDevices_; // Vector assignment operator will suffice for this
            defaultLumaCoefs_ = rhs.defaultLumaCoefs_; // Vector assignment operator will suffice for this
            
            strictParsing_ = rhs.strictParsing_;
            
            sanity_ = rhs.sanity_;
            sanitytext_ = rhs.sanitytext_;
            
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
        config->getImpl()->load(istream, "");
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
        config->getImpl()->load(istream, filename);
        return config;
    }
    
    ConstConfigRcPtr Config::CreateFromStream(std::istream & istream)
    {
        ConfigRcPtr config = Config::Create();
        config->getImpl()->load(istream, "");
        return config;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    Config::Config()
    : m_impl(new Config::Impl)
    {
    }
    
    Config::~Config()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    ConfigRcPtr Config::createEditableCopy() const
    {
        ConfigRcPtr config = Config::Create();
        *config->m_impl = *m_impl;
        return config;
    }
    
    void Config::sanityCheck() const
    {
        if(getImpl()->sanity_ == SANITY_SANE) return;
        if(getImpl()->sanity_ == SANITY_INSANE)
        {
            throw Exception(getImpl()->sanitytext_.c_str());
        }
        
        getImpl()->sanity_ = SANITY_INSANE;
        getImpl()->sanitytext_ = "";
        
        // Confirm all ColorSpaces are valid
        // TODO: Confirm there arent duplicate colorspaces
        for(unsigned int i=0; i<getImpl()->colorspaces_.size(); ++i)
        {
            if(!getImpl()->colorspaces_[i])
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The colorspace at index " << i << " is null.";
                getImpl()->sanitytext_ = os.str();
                throw Exception(getImpl()->sanitytext_.c_str());
            }
            
            const char * name = getImpl()->colorspaces_[i]->getName();
            if(!name || strlen(name) == 0)
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The colorspace at index " << i << " is not named.";
                getImpl()->sanitytext_ = os.str();
                throw Exception(getImpl()->sanitytext_.c_str());
            }
            
            const char * family = getImpl()->colorspaces_[i]->getFamily();
            if(!family || strlen(family) == 0)
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The colorspace named '" << name << "' ";
                os << "does not specify a family.";
                getImpl()->sanitytext_ = os.str();
                throw Exception(getImpl()->sanitytext_.c_str());
            }
        }
        
        // Confirm all roles are valid
        {
            for(RoleMap::const_iterator iter = getImpl()->roles_.begin(),
                end = getImpl()->roles_.end(); iter!=end; ++iter)
            {
                int csindex = -1;
                if(!FindColorSpaceIndex(&csindex, getImpl()->colorspaces_, iter->second))
                {
                    std::ostringstream os;
                    os << "Config failed sanitycheck. ";
                    os << "The role '" << iter->first << "' ";
                    os << "refers to a colorspace, '" << iter->second << "', ";
                    os << "which is not defined.";
                    getImpl()->sanitytext_ = os.str();
                    throw Exception(getImpl()->sanitytext_.c_str());
                }
                
                // Confirm no name conflicts between colorspaces and roles
                if(FindColorSpaceIndex(&csindex, getImpl()->colorspaces_, iter->first))
                {
                    std::ostringstream os;
                    os << "Config failed sanitycheck. ";
                    os << "The role '" << iter->first << "' ";
                    os << " is in conflict with a colorspace of the same name.";
                    getImpl()->sanitytext_ = os.str();
                    throw Exception(getImpl()->sanitytext_.c_str());
                }
            }
        }
        
        // Confirm all Displays transforms refer to colorspaces that exit
        for(unsigned int i=0; i<getImpl()->displayDevices_.size(); ++i)
        {
            int csindex = -1;
            if(!FindColorSpaceIndex(&csindex,
                getImpl()->colorspaces_, getImpl()->displayDevices_[i][2]))
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The display device at index " << i << " (";
                os << getImpl()->displayDevices_[i][0] << " / ";
                os << getImpl()->displayDevices_[i][1] << ") ";
                os << "refers to a colorspace '";
                os << getImpl()->displayDevices_[i][2] << " ' which is not defined.";
                getImpl()->sanitytext_ = os.str();
                throw Exception(getImpl()->sanitytext_.c_str());
            }
        }
        
        // TODO: Confirm at least one display entry exists.
        
        // Confirm for all ColorSpaceTransforms the names spaces exist
        
        // Potential enhancement: Confirm all files exist with read permissions
        
        // Everything is groovy.
        getImpl()->sanity_ = SANITY_SANE;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    const char * Config::getDescription() const
    {
        return getImpl()->description_.c_str();
    }
    
    void Config::setDescription(const char * description)
    {
        getImpl()->sanity_ = SANITY_UNKNOWN;
        getImpl()->sanitytext_ = "";
        
        getImpl()->description_ = description;
    }
    
    
    // RESOURCES //////////////////////////////////////////////////////////////
    
    ConstContextRcPtr Config::getCurrentContext() const
    {
        return getImpl()->context_;
    }
    
    const char * Config::getSearchPath() const
    {
        return getImpl()->context_->getSearchPath();
    }
    
    void Config::setSearchPath(const char * path)
    {
        getImpl()->context_->setSearchPath(path);
    }
    
    const char * Config::getWorkingDir() const
    {
        return getImpl()->context_->getWorkingDir();
    }
    
    void Config::setWorkingDir(const char * dirname)
    {
        getImpl()->context_->setWorkingDir(dirname);
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    int Config::getNumColorSpaces() const
    {
        return static_cast<int>(getImpl()->colorspaces_.size());
    }
    
    const char * Config::getColorSpaceNameByIndex(int index) const
    {
        if(index<0 || index >= (int)getImpl()->colorspaces_.size())
        {
            return "";
        }
        
        return getImpl()->colorspaces_[index]->getName();
    }
    
    ConstColorSpaceRcPtr Config::getColorSpace(const char * name) const
    {
        int index = getIndexForColorSpace(name);
        if(index<0 || index >= (int)getImpl()->colorspaces_.size())
        {
            return ColorSpaceRcPtr();
        }
        
        return getImpl()->colorspaces_[index];
    }
    
    int Config::getIndexForColorSpace(const char * name) const
    {
        int csindex = -1;
        
        // Check to see if the name is a color space
        if( FindColorSpaceIndex(&csindex, getImpl()->colorspaces_, name) )
        {
            return csindex;
        }
        
        // Check to see if the name is a role
        std::string csname = LookupRole(getImpl()->roles_, name);
        if( FindColorSpaceIndex(&csindex, getImpl()->colorspaces_, csname) )
        {
            return csindex;
        }
        
        // Is a default role defined?
        // (And, are we allowed to use it)
        if(!getImpl()->strictParsing_)
        {
            csname = LookupRole(getImpl()->roles_, ROLE_DEFAULT);
            if( FindColorSpaceIndex(&csindex, getImpl()->colorspaces_, csname) )
            {
                return csindex;
            }
        }
        
        return -1;
    }
    
    void Config::addColorSpace(const ConstColorSpaceRcPtr & original)
    {
        getImpl()->sanity_ = SANITY_UNKNOWN;
        getImpl()->sanitytext_ = "";
        
        ColorSpaceRcPtr cs = original->createEditableCopy();
        
        std::string name = cs->getName();
        if(name.empty())
            throw Exception("Cannot addColorSpace with an empty name.");
        
        // Check to see if the colorspace already exists
        int csindex = -1;
        if( FindColorSpaceIndex(&csindex, getImpl()->colorspaces_, name) )
        {
            getImpl()->colorspaces_[csindex] = cs;
        }
        else
        {
            // Otherwise, add it
            getImpl()->colorspaces_.push_back( cs );
        }
    }
    
    void Config::clearColorSpaces()
    {
        getImpl()->colorspaces_.clear();
    }
    
    
    
    
    
    
    const char * Config::parseColorSpaceFromString(const char * str) const
    {
        if(!str) return str;
        
        // Search the entire filePath, including directory name (if provided)
        // convert the filename to lowercase.
        std::string fullstr = pystring::lower(std::string(str));
        
        // See if it matches a lut name.
        // This is the position of the RIGHT end of the colorspace substring, not the left
        int rightMostColorPos=-1;
        std::string rightMostColorspace = "";
        int rightMostColorSpaceIndex = -1;
        
        // Find the right-most occcurance within the string for each colorspace.
        for (unsigned int i=0; i<getImpl()->colorspaces_.size(); ++i)
        {
            std::string csname = pystring::lower(getImpl()->colorspaces_[i]->getName());
            
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
            return getImpl()->colorspaces_[rightMostColorSpaceIndex]->getName();
        }
        
        if(!getImpl()->strictParsing_)
        {
            // Is a default role defined?
            std::string csname = LookupRole(getImpl()->roles_, ROLE_DEFAULT);
            if(!csname.empty())
            {
                int csindex = -1;
                if( FindColorSpaceIndex(&csindex, getImpl()->colorspaces_, csname) )
                {
                    // This is necessary to not return a reference to
                    // a local variable.
                    return getImpl()->colorspaces_[csindex]->getName();
                }
            }
        }
        
        return "";
    }
    
    bool Config::isStrictParsingEnabled() const
    {
        return getImpl()->strictParsing_;
    }
    
    void Config::setStrictParsingEnabled(bool enabled)
    {
        getImpl()->strictParsing_ = enabled;
    }
    
    // Roles
    void Config::setRole(const char * role, const char * colorSpaceName)
    {
        getImpl()->sanity_ = SANITY_UNKNOWN;
        getImpl()->sanitytext_ = "";
        
        // Set the role
        if(colorSpaceName)
        {
            getImpl()->roles_[pystring::lower(role)] = std::string(colorSpaceName);
        }
        // Unset the role
        else
        {
            RoleMap::iterator iter = getImpl()->roles_.find(pystring::lower(role));
            if(iter != getImpl()->roles_.end())
            {
                getImpl()->roles_.erase(iter);
            }
        }
    }
    
    int Config::getNumRoles() const
    {
        return static_cast<int>(getImpl()->roles_.size());
    }
    
    const char * Config::getRoleNameByIndex(int index) const
    {
        if(index<0 || index >= (int)getImpl()->roles_.size())
        {
            return "";
        }
        
        RoleMap::const_iterator iter = getImpl()->roles_.begin();
        for(int i=0; i<index; ++i)
        {
            ++iter;
        }
        
        return iter->second.c_str();
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
        
        for(unsigned int i=0; i<getImpl()->displayDevices_.size(); ++i)
        {
            if(getImpl()->displayDevices_[i].size() != 3) continue;
            devices.insert( getImpl()->displayDevices_[i][0] );
        }
        
        return static_cast<int>(devices.size());
    }
    
    const char * Config::getDisplayDeviceName(int index) const
    {
        std::set<std::string> devices;
        
        for(unsigned int i=0; i<getImpl()->displayDevices_.size(); ++i)
        {
            if(getImpl()->displayDevices_[i].size() != 3) continue;
            devices.insert( getImpl()->displayDevices_[i][0] );
            
            if((int)devices.size()-1 == index)
            {
                return getImpl()->displayDevices_[i][0].c_str();
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
        
        for(unsigned int i=0; i<getImpl()->displayDevices_.size(); ++i)
        {
            if(getImpl()->displayDevices_[i].size() != 3) continue;
            if(getImpl()->displayDevices_[i][0] != device) continue;
            names.insert( getImpl()->displayDevices_[i][1] );
        }
        
        return static_cast<int>(names.size());
    }
    
    const char * Config::getDisplayTransformName(const char * device,
        int index) const
    {
        std::set<std::string> names;
        
        for(unsigned int i=0; i<getImpl()->displayDevices_.size(); ++i)
        {
            if(getImpl()->displayDevices_[i].size() != 3) continue;
            if(getImpl()->displayDevices_[i][0] != device) continue;
            names.insert( getImpl()->displayDevices_[i][1] );
            
            if((int)names.size()-1 == index)
            {
                return getImpl()->displayDevices_[i][1].c_str();
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
        for(unsigned int i=0; i<getImpl()->displayDevices_.size(); ++i)
        {
            if(getImpl()->displayDevices_[i].size() != 3) continue;
            if(getImpl()->displayDevices_[i][0] != device) continue;
            if(getImpl()->displayDevices_[i][1] != displayTransformName) continue;
            return getImpl()->displayDevices_[i][2].c_str();
        }
        
        return "";
    }
    
    void Config::addDisplayDevice(const char * device,
                                        const char * displayTransformName,
                                        const char * csname)
    {
        getImpl()->sanity_ = SANITY_UNKNOWN;
        getImpl()->sanitytext_ = "";
        
        // Is this device / display already registered?
        // If so, set it to the potentially new value.
        
        for(unsigned int i=0; i<getImpl()->displayDevices_.size(); ++i)
        {
            if(getImpl()->displayDevices_[i].size() != 3) continue;
            if(getImpl()->displayDevices_[i][0] != device) continue;
            if(getImpl()->displayDevices_[i][1] != displayTransformName) continue;
            
            getImpl()->displayDevices_[i][2] = csname;
            return;
        }
        
        // Otherwise, add a new entry!
        DisplayKey displayKey;
        displayKey.push_back(std::string(device));
        displayKey.push_back(std::string(displayTransformName));
        displayKey.push_back(std::string(csname));
        getImpl()->displayDevices_.push_back(displayKey);
    }
    
    void Config::getDefaultLumaCoefs(float * c3) const
    {
        memcpy(c3, &getImpl()->defaultLumaCoefs_[0], 3*sizeof(float));
    }
    
    void Config::setDefaultLumaCoefs(const float * c3)
    {
        getImpl()->sanity_ = SANITY_UNKNOWN;
        getImpl()->sanitytext_ = "";
        
        memcpy(&getImpl()->defaultLumaCoefs_[0], c3, 3*sizeof(float));
    }
    
    ConstProcessorRcPtr Config::getProcessor(const ConstColorSpaceRcPtr & src,
                                             const ConstColorSpaceRcPtr & dst) const
    {
        ConstContextRcPtr context = getCurrentContext();
        return getProcessor(context, src, dst);
    }
    
    ConstProcessorRcPtr Config::getProcessor(const ConstContextRcPtr & context,
                                             const ConstColorSpaceRcPtr & src,
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
        processor->addColorSpaceConversion(*this, context, src, dst);
        processor->finalize();
        return processor;
    }
    
    ConstProcessorRcPtr Config::getProcessor(const char * srcName,
                                             const char * dstName) const
    {
        ConstContextRcPtr context = getCurrentContext();
        return getProcessor(context, srcName, dstName);
    }
    
    //! Names can be colorspace name or role name
    ConstProcessorRcPtr Config::getProcessor(const ConstContextRcPtr & context,
                                             const char * srcName,
                                             const char * dstName) const
    {
        ConstColorSpaceRcPtr src = getColorSpace(srcName);
        if(!src)
        {
            std::ostringstream os;
            os << "Could not find colorspace '" << srcName << "'.";
            throw Exception(os.str().c_str());
        }
        
        ConstColorSpaceRcPtr dst = getColorSpace(dstName);
        if(!dst)
        {
            std::ostringstream os;
            os << "Could not find colorspace '" << dstName << "'.";
            throw Exception(os.str().c_str());
        }
        
        return getProcessor(context, src, dst);
    }
    
    
    ConstProcessorRcPtr Config::getProcessor(const ConstTransformRcPtr& transform) const
    {
        return getProcessor(transform, TRANSFORM_DIR_FORWARD);
    }
    
    
    ConstProcessorRcPtr Config::getProcessor(const ConstTransformRcPtr& transform,
                                             TransformDirection direction) const
    {
        ConstContextRcPtr context = getCurrentContext();
        return getProcessor(context, transform, direction);
    }
    
    ConstProcessorRcPtr Config::getProcessor(const ConstContextRcPtr & context,
                                             const ConstTransformRcPtr& transform,
                                             TransformDirection direction) const
    {
        LocalProcessorRcPtr processor = LocalProcessor::Create();
        processor->addTransform(*this, context, transform, direction);
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
            out << YAML::Newline;
            
            std::string search_path = getSearchPath();
            if(search_path != "")
            {
                out << YAML::Key << "search_path" << YAML::Value << search_path;
            }
            
            out << YAML::Key << "strictparsing" << YAML::Value << getImpl()->strictParsing_;
            
            out << YAML::Key << "luma" << YAML::Value << YAML::Flow << getImpl()->defaultLumaCoefs_;
            
            if(getImpl()->description_ != "")
            {
                out << YAML::Newline;
                out << YAML::Key << "description" << YAML::Value << getImpl()->description_;
            }
            
            // Roles
            if(getImpl()->roles_.size() > 0)
            {
                out << YAML::Newline;
                out << YAML::Key << "roles" << YAML::Value;
                out << getImpl()->roles_;
            }
            
            // Displays
            if(getImpl()->displayDevices_.size() > 0)
            {
                out << YAML::Newline;
                out << YAML::Key << "displays" << YAML::Value;
                out << YAML::BeginSeq;
                for(unsigned int i=0; i<getImpl()->displayDevices_.size(); ++i)
                    out << getImpl()->displayDevices_[i];
                out << YAML::EndSeq;
            }
            
            // ColorSpaces
            if(getImpl()->colorspaces_.size() > 0)
            {
                out << YAML::Newline;
                out << YAML::Key << "colorspaces";
                out << YAML::Value << getImpl()->colorspaces_; // std::vector -> Seq
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
            
            // Query resource_path for compatibility
            if(node.FindValue("search_path") != NULL)
            {
                std::string path;
                node["search_path"] >> path;
                context_->setSearchPath(path.c_str());
            }
            else if(node.FindValue("resource_path") != NULL)
            {
                std::string path;
                node["resource_path"] >> path;
                context_->setSearchPath(path.c_str());
            }
            
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
                
                defaultLumaCoefs_ = value;
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
                    roles_[pystring::lower(key)] = value;
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
            
            if(filename)
            {
                std::string configrootdir = path::dirname(filename);
                context_->setWorkingDir(configrootdir.c_str());
            }
        }
        catch( const std::exception & e)
        {
            std::ostringstream os;
            os << "Error: Loading the OCIO profile ";
            if(filename) os << "'" << filename << "' ";
            os << "failed. " << e.what();
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
#include "pystring/pystring.h"

BOOST_AUTO_TEST_SUITE( Config_Unit_Tests )

#if 0
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
#endif

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
    "  - !<ColorSpace>\n"
    "      name: loads_of_transforms\n"
    "      family: vd8\n"
    "      bitdepth: 8ui\n"
    "      description: 'how many transforms can we use?'\n"
    "      isdata: false\n"
    "      gpuallocation: uniform\n"
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

BOOST_AUTO_TEST_CASE ( test_ser )
{
    
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("testing");
        cs->setFamily("test");
        OCIO::FileTransformRcPtr transform1 = \
            OCIO::FileTransform::Create();
        OCIO::GroupTransformRcPtr groupTransform = OCIO::GroupTransform::Create();
        groupTransform->push_back(transform1);
        cs->setTransform(groupTransform, OCIO::COLORSPACE_DIR_TO_REFERENCE);
        config->addColorSpace(cs);
        config->setRole( OCIO::ROLE_COMPOSITING_LOG, cs->getName() );
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("testing2");
        cs->setFamily("test");
        OCIO::ExponentTransformRcPtr transform1 = \
            OCIO::ExponentTransform::Create();
        OCIO::GroupTransformRcPtr groupTransform = OCIO::GroupTransform::Create();
        groupTransform->push_back(transform1);
        cs->setTransform(groupTransform, OCIO::COLORSPACE_DIR_TO_REFERENCE);
        config->addColorSpace(cs);
        config->setRole( OCIO::ROLE_COMPOSITING_LOG, cs->getName() );
    }
    
    // for testing
    //std::ofstream outfile("/tmp/test.ocio");
    //config->serialize(outfile);
    //outfile.close();
    
    std::ostringstream os;
    config->serialize(os);
    
    std::string PROFILE_OUT =
    "---\n"
    "ocio_profile_version: 1\n"
    "\n"
    "strictparsing: true\n"
    "luma: [0.2126, 0.7152, 0.0722]\n"
    "\n"
    "roles:\n"
    "  compositing_log: testing2\n"
    "\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "    name: testing\n"
    "    family: test\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "    to_reference: !<GroupTransform>\n"
    "      children:\n"
    "        - !<FileTransform> {src: \"\", interpolation: unknown}\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: testing2\n"
    "    family: test\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "    to_reference: !<GroupTransform>\n"
    "      children:\n"
    "        - !<ExponentTransform> {value: [1, 1, 1, 1]}\n";
    
    std::vector<std::string> osvec;
    OCIO::pystring::splitlines(os.str(), osvec);
    std::vector<std::string> PROFILE_OUTvec;
    OCIO::pystring::splitlines(PROFILE_OUT, PROFILE_OUTvec);
    
    BOOST_CHECK_EQUAL(osvec.size(), PROFILE_OUTvec.size());
    for(unsigned int i = 0; i < PROFILE_OUTvec.size(); ++i)
        BOOST_CHECK_EQUAL(osvec[i], PROFILE_OUTvec[i]);
}

BOOST_AUTO_TEST_SUITE_END()

#endif // OCIO_UNIT_TEST
