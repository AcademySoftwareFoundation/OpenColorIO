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
#include <pystring.h>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "Mutex.h"
#include "OpBuilders.h"
#include "PathUtils.h"
#include "ParseUtils.h"
#include "Processor.h"
#include "OCIOYaml.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        const char * OCIO_CONFIG_ENVVAR = "OCIO";
        const char * OCIO_ACTIVE_DISPLAYS_ENVVAR = "OCIO_ACTIVE_DISPLAYS";
        const char * OCIO_ACTIVE_VIEWS_ENVVAR = "OCIO_ACTIVE_VIEWS";
        
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
        "  sRGB:\n"
        "  - !<View> {name: Raw, colorspace: raw}\n"
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
    
    namespace
    {
    
    typedef std::map<std::string, std::string> StringMap;
    typedef std::vector<std::string> StringVec;
    
    // Roles
    // (lower case role name: colorspace name)
    std::string LookupRole(const StringMap & roles, const std::string & rolename)
    {
        StringMap::const_iterator iter = roles.find(pystring::lower(rolename));
        if(iter == roles.end()) return "";
        return iter->second;
    }
    
    
    // Colorspaces
    typedef std::vector<ColorSpaceRcPtr> ColorSpaceVec;
    
    bool FindColorSpaceIndex(int * index,
                             const ColorSpaceVec & colorspaces,
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
    
    
    // Displays
    struct View
    {
        std::string name;
        std::string colorspace;
        
        View() { }
        
        View(const std::string & name_,
             const std::string & colorspace_) :
                name(name_),
                colorspace(colorspace_)
        { }
    };
    
    typedef std::vector<View> ViewVec;
    typedef std::map<std::string, ViewVec> DisplayMap;  // (display name : ViewVec)
    
    void operator >> (const YAML::Node& node, View& v)
    {
        if(node.GetTag() != "View")
            return; // not a !<Display> tag
        
        if(node.FindValue("name") == NULL)
        {
            throw Exception("View does not specify 'name'.");
        }
        
        v.name = node["name"].Read<std::string>();
        
        if(node.FindValue("colorspace") == NULL)
        {
            std::ostringstream os;
            os << "View '" << v.name << "' ";
            os << "does not specify colorspace.";
            throw Exception(os.str().c_str());
        }
        
        v.colorspace = node["colorspace"].Read<std::string>();
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, View view)
    {
        out << YAML::VerbatimTag("View");
        out << YAML::Flow;
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << view.name;
        out << YAML::Key << "colorspace" << YAML::Value << view.colorspace;
        out << YAML::EndMap;
        return out;
    }
    
    DisplayMap::iterator find_display(DisplayMap & displays, const std::string & display)
    {
        for(DisplayMap::iterator iter = displays.begin();
            iter != displays.end();
            ++iter)
        {
            if(StrEqualsCaseIgnore(display, iter->first)) return iter;
        }
        return displays.end();
    }
    
    DisplayMap::const_iterator find_display_const(const DisplayMap & displays, const std::string & display)
    {
        for(DisplayMap::const_iterator iter = displays.begin();
            iter != displays.end();
            ++iter)
        {
            if(StrEqualsCaseIgnore(display, iter->first)) return iter;
        }
        return displays.end();
    }
    
    int find_view(const ViewVec & vec, const std::string & name)
    {
        for(unsigned int i=0; i<vec.size(); ++i)
        {
            if(StrEqualsCaseIgnore(name, vec[i].name)) return i;
        }
        return -1;
    }
    
    void SetDisplay(DisplayMap & displays,
                    const std::string & display,
                    const std::string & view,
                    const std::string & colorspace)
    {
        DisplayMap::iterator iter = find_display(displays, display);
        if(iter == displays.end())
        {
            ViewVec views;
            views.push_back( View(view, colorspace) );
            displays[display] = views;
        }
        else
        {
            ViewVec & views = iter->second;
            int index = find_view(views, view);
            if(index<0)
            {
                views.push_back( View(view, colorspace) );
            }
            else
            {
                views[index].colorspace = colorspace;
            }
        }
    }
    
    void ComputeDisplays(StringVec & displayCache,
                         const DisplayMap & displays,
                         const StringVec & activeDisplays,
                         const StringVec & activeDisplaysEnvOverride)
    {
        displayCache.clear();
        
        StringVec displayMasterList;
        for(DisplayMap::const_iterator iter = displays.begin();
            iter != displays.end();
            ++iter)
        {
            displayMasterList.push_back(iter->first);
        }
        
        // Apply the env override if it's not empty.
        if(!activeDisplaysEnvOverride.empty())
        {
            displayCache = IntersectStringVecsCaseIgnore(displayMasterList, activeDisplaysEnvOverride);
            if(!displayCache.empty()) return;
        }
        // Otherwise, aApply the active displays if it's not empty.
        else if(!activeDisplays.empty())
        {
            displayCache = IntersectStringVecsCaseIgnore(displayMasterList, activeDisplays);
            if(!displayCache.empty()) return;
        }
        
        displayCache = displayMasterList;
    }
    
    
    
    } // namespace
    
    class Config::Impl
    {
    public:
        ContextRcPtr context_;
        std::string description_;
        ColorSpaceVec colorspaces_;
        StringMap roles_;
        
        DisplayMap displays_;
        StringVec activeDisplays_;
        StringVec activeDisplaysEnvOverride_;
        StringVec activeViews_;
        StringVec activeViewsEnvOverride_;
        
        mutable std::string activeDisplaysStr_;
        mutable std::string activeViewsStr_;
        mutable StringVec displayCache_;
        
        // Misc
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
            
            char* activeDisplays = std::getenv(OCIO_ACTIVE_DISPLAYS_ENVVAR);
            if(activeDisplays)
            {
                SplitStringEnvStyle(activeDisplaysEnvOverride_, activeDisplays);
            }
            
            char * activeViews = std::getenv(OCIO_ACTIVE_VIEWS_ENVVAR);
            if(activeViews)
            {
                SplitStringEnvStyle(activeViewsEnvOverride_, activeViews);
            }
            
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
            
            // Assignment operator will suffice for these
            roles_ = rhs.roles_;
            
            displays_ = rhs.displays_;
            activeDisplays_ = rhs.activeDisplays_;
            activeViews_ = rhs.activeViews_;
            activeViewsEnvOverride_ = rhs.activeViewsEnvOverride_;
            activeDisplaysEnvOverride_ = rhs.activeDisplaysEnvOverride_;
            activeDisplaysStr_ = rhs.activeDisplaysStr_;
            displayCache_ = rhs.displayCache_;
            
            defaultLumaCoefs_ = rhs.defaultLumaCoefs_;
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
        char* file = std::getenv(OCIO_CONFIG_ENVVAR);
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
            for(StringMap::const_iterator iter = getImpl()->roles_.begin(),
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
        
        int numviews = 0;
        
        // Confirm all Displays transforms refer to colorspaces that exit
        for(DisplayMap::const_iterator iter = getImpl()->displays_.begin();
            iter != getImpl()->displays_.end();
            ++iter)
        {
            std::string display = iter->first;
            const ViewVec & views = iter->second;
            if(views.empty())
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The display '" << display << "' ";
                os << "does not define any views.";
                getImpl()->sanitytext_ = os.str();
                throw Exception(getImpl()->sanitytext_.c_str());
            }
            
            for(unsigned int i=0; i<views.size(); ++i)
            {
                if(views[i].name.empty() || views[i].colorspace.empty())
                {
                    std::ostringstream os;
                    os << "Config failed sanitycheck. ";
                    os << "The display '" << display << "' ";
                    os << "defines a view with an empty name and/or colorspace.";
                    getImpl()->sanitytext_ = os.str();
                    throw Exception(getImpl()->sanitytext_.c_str());
                }
                
                int csindex = -1;
                if(!FindColorSpaceIndex(&csindex, getImpl()->colorspaces_, views[i].colorspace))
                {
                    std::ostringstream os;
                    os << "Config failed sanitycheck. ";
                    os << "The display '" << display << "' ";
                    os << "refers to a colorspace, '" << views[i].colorspace << "', ";
                    os << "which is not defined.";
                    getImpl()->sanitytext_ = os.str();
                    throw Exception(getImpl()->sanitytext_.c_str());
                }
                
                ++numviews;
            }
        }
        
        // Confirm at least one display entry exists.
        if(numviews == 0)
        {
            std::ostringstream os;
            os << "Config failed sanitycheck. ";
            os << "No displays are specified.";
            getImpl()->sanitytext_ = os.str();
            throw Exception(getImpl()->sanitytext_.c_str());
        }
        
        // TODO: Confirm for all ColorSpaceTransforms the names spaces exist
        
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
        if(!str) return "";
        
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
            StringMap::iterator iter = getImpl()->roles_.find(pystring::lower(role));
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
    
    bool Config::hasRole(const char * role) const
    {
        return LookupRole(getImpl()->roles_, role) == "" ? false : true;
    }
    
    const char * Config::getRoleName(int index) const
    {
        if(index < 0 || index >= (int)getImpl()->roles_.size()) return "";
        StringMap::const_iterator iter = getImpl()->roles_.begin();
        for(int i = 0; i < index; ++i) ++iter;
        return iter->first.c_str();
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // Display/View Registration
    
    
    const char * Config::getDefaultDisplay() const
    {
        if(getImpl()->displayCache_.empty())
        {
            ComputeDisplays(getImpl()->displayCache_,
                            getImpl()->displays_,
                            getImpl()->activeDisplays_,
                            getImpl()->activeDisplaysEnvOverride_);
        }
        
        int index = -1;
        
        if(!getImpl()->activeDisplaysEnvOverride_.empty())
        {
            StringVec orderedDisplays = IntersectStringVecsCaseIgnore(getImpl()->activeDisplaysEnvOverride_,
                                                           getImpl()->displayCache_);
            if(!orderedDisplays.empty())
            {
                index = FindInStringVecCaseIgnore(getImpl()->displayCache_, orderedDisplays[0]);
            }
        }
        else if(!getImpl()->activeDisplays_.empty())
        {
            StringVec orderedDisplays = IntersectStringVecsCaseIgnore(getImpl()->activeDisplays_,
                                                           getImpl()->displayCache_);
            if(!orderedDisplays.empty())
            {
                index = FindInStringVecCaseIgnore(getImpl()->displayCache_, orderedDisplays[0]);
            }
        }
        
        if(index >= 0)
        {
            return getImpl()->displayCache_[index].c_str();
        }
        
        if(!getImpl()->displayCache_.empty())
        {
            return getImpl()->displayCache_[0].c_str();
        }
        
        return "";
    }


    int Config::getNumDisplays() const
    {
        if(getImpl()->displayCache_.empty())
        {
            ComputeDisplays(getImpl()->displayCache_,
                            getImpl()->displays_,
                            getImpl()->activeDisplays_,
                            getImpl()->activeDisplaysEnvOverride_);
        }
        
        return static_cast<int>(getImpl()->displayCache_.size());
    }

    const char * Config::getDisplay(int index) const
    {
        if(getImpl()->displayCache_.empty())
        {
            ComputeDisplays(getImpl()->displayCache_,
                            getImpl()->displays_,
                            getImpl()->activeDisplays_,
                            getImpl()->activeDisplaysEnvOverride_);
        }
        
        if(index>=0 || index < static_cast<int>(getImpl()->displayCache_.size()))
        {
            return getImpl()->displayCache_[index].c_str();
        }
        
        return "";
    }
    
    const char * Config::getDefaultView(const char * display) const
    {
        if(getImpl()->displayCache_.empty())
        {
            ComputeDisplays(getImpl()->displayCache_,
                            getImpl()->displays_,
                            getImpl()->activeDisplays_,
                            getImpl()->activeDisplaysEnvOverride_);
        }
        
        if(!display) return 0;
        
        DisplayMap::const_iterator iter = find_display_const(getImpl()->displays_, display);
        if(iter == getImpl()->displays_.end()) return 0;
        
        const ViewVec & views = iter->second;
        
        StringVec masterViews;
        for(unsigned int i=0; i<views.size(); ++i)
        {
            masterViews.push_back(views[i].name);
        }
        
        int index = -1;
        
        if(!getImpl()->activeViewsEnvOverride_.empty())
        {
            StringVec orderedViews = IntersectStringVecsCaseIgnore(getImpl()->activeViewsEnvOverride_,
                                                           masterViews);
            if(!orderedViews.empty())
            {
                index = FindInStringVecCaseIgnore(masterViews, orderedViews[0]);
            }
        }
        else if(!getImpl()->activeViews_.empty())
        {
            StringVec orderedViews = IntersectStringVecsCaseIgnore(getImpl()->activeViews_,
                                                           masterViews);
            if(!orderedViews.empty())
            {
                index = FindInStringVecCaseIgnore(masterViews, orderedViews[0]);
            }
        }
        
        if(index >= 0)
        {
            return views[index].name.c_str();
        }
        
        if(!views.empty())
        {
            return views[0].name.c_str();
        }
        
        return "";
    }

    int Config::getNumViews(const char * display) const
    {
        if(getImpl()->displayCache_.empty())
        {
            ComputeDisplays(getImpl()->displayCache_,
                            getImpl()->displays_,
                            getImpl()->activeDisplays_,
                            getImpl()->activeDisplaysEnvOverride_);
        }
        
        if(!display) return 0;
        
        DisplayMap::const_iterator iter = find_display_const(getImpl()->displays_, display);
        if(iter == getImpl()->displays_.end()) return 0;
        
        const ViewVec & views = iter->second;
        return static_cast<int>(views.size());
    }

    const char * Config::getView(const char * display, int index) const
    {
        if(getImpl()->displayCache_.empty())
        {
            ComputeDisplays(getImpl()->displayCache_,
                            getImpl()->displays_,
                            getImpl()->activeDisplays_,
                            getImpl()->activeDisplaysEnvOverride_);
        }
        
        if(!display) return "";
        
        DisplayMap::const_iterator iter = find_display_const(getImpl()->displays_, display);
        if(iter == getImpl()->displays_.end()) return 0;
        
        const ViewVec & views = iter->second;
        return views[index].name.c_str();
    }

    const char * Config::getDisplayColorSpaceName(const char * display, const char * view) const
    {
        if(!display || !view) return "";
        
        DisplayMap::const_iterator iter = find_display_const(getImpl()->displays_, display);
        if(iter == getImpl()->displays_.end()) return "";
        
        const ViewVec & views = iter->second;
        int index = find_view(views, view);
        if(index<0) return "";
        
        return views[index].colorspace.c_str();
    }
    
    void Config::setDisplayColorSpaceName(const char * display, const char * view,
                                          const char * colorSpaceName)
    {
        if(!display || !view || !colorSpaceName) return;
        
        SetDisplay(getImpl()->displays_,
                   display, view, colorSpaceName);
        getImpl()->displayCache_.clear();
    }

    void Config::setActiveDisplays(const char * displays)
    {
        if(!displays)
        {
            getImpl()->activeDisplays_.clear();
        }
        else
        {
            SplitStringEnvStyle(getImpl()->activeDisplays_, displays);
        }
        
        getImpl()->displayCache_.clear();
    }

    const char * Config::getActiveDisplays() const
    {
        getImpl()->activeDisplaysStr_ = JoinStringEnvStyle(getImpl()->activeDisplays_);
        return getImpl()->activeDisplaysStr_.c_str();
    }
    
    void Config::setActiveViews(const char * views)
    {
        if(!views)
        {
            getImpl()->activeViews_.clear();
        }
        else
        {
            SplitStringEnvStyle(getImpl()->activeViews_, views);
        }
        
        getImpl()->displayCache_.clear();
    }

    const char * Config::getActiveViews() const
    {
        getImpl()->activeViewsStr_ = JoinStringEnvStyle(getImpl()->activeViews_);
        return getImpl()->activeViewsStr_.c_str();
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    
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
        
        ProcessorRcPtr processor = Processor::Create();
        processor->getImpl()->addColorSpaceConversion(*this, context, src, dst);
        processor->getImpl()->finalize();
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
        ProcessorRcPtr processor = Processor::Create();
        processor->getImpl()->addTransform(*this, context, transform, direction);
        processor->getImpl()->finalize();
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
            YAML::Emitter out;
            out << YAML::Block;
            out << YAML::BeginMap;
            out << YAML::Key << "ocio_profile_version" << YAML::Value << 1;
            out << YAML::Newline;
            
            out << YAML::Key << "search_path" << YAML::Value << getImpl()->context_->getSearchPath();
            out << YAML::Key << "strictparsing" << YAML::Value << getImpl()->strictParsing_;
            out << YAML::Key << "luma" << YAML::Value << YAML::Flow << getImpl()->defaultLumaCoefs_;
            
            if(getImpl()->description_ != "")
            {
                out << YAML::Newline;
                out << YAML::Key << "description";
                out << YAML::Value << getImpl()->description_;
            }
            
            // Roles
            out << YAML::Newline;
            out << YAML::Key << "roles";
            out << YAML::Value << getImpl()->roles_;
            
            // Displays
            out << YAML::Newline;
            out << YAML::Key << "displays";
            out << YAML::Value << getImpl()->displays_;
            out << YAML::Key << "active_displays";
            out << YAML::Value << YAML::Flow << getImpl()->activeDisplays_;
            out << YAML::Key << "active_views";
            out << YAML::Value << YAML::Flow << getImpl()->activeViews_;
            
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
            
            if(node.FindValue("roles") != NULL)
            {
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
            }
            
            // Displays
            if(node.FindValue("displays") != NULL)
            {
                // Backwards Compatibility: load the sequence
                if(node["displays"].GetType() == YAML::CT_SEQUENCE)
                {
                    for (unsigned i = 0; i < node["displays"].size(); ++i)
                    {
                        if(node["displays"][i].GetType() != YAML::CT_MAP)
                        {
                            std::ostringstream os;
                            os << "Display entries must be a map.";
                            throw Exception(os.str().c_str());
                        }
                        
                        if(node["displays"][i].FindValue("device") == NULL ||
                           node["displays"][i].FindValue("name") == NULL ||
                           node["displays"][i].FindValue("colorspace") == NULL)
                        {
                            std::ostringstream os;
                            os << "Display entries must define 'device', 'name', and 'colorspace'.";
                            throw Exception(os.str().c_str());
                        }
                        
                        std::string display = node["displays"][i]["device"].Read<std::string>();
                        std::string view = node["displays"][i]["name"].Read<std::string>();
                        std::string colorspace = node["displays"][i]["colorspace"].Read<std::string>();
                        
                        SetDisplay(displays_,
                                   display, view, colorspace);
                    }
                }
                else if(node["displays"].GetType() == YAML::CT_MAP)
                {
                    node["displays"] >> displays_;
                }
            }
            
            if(node.FindValue("active_displays") != NULL)
            {
                if(node["active_displays"].GetType() != YAML::CT_SEQUENCE)
                {
                    std::ostringstream os;
                    os << "'active_displays' field needs to be a list.";
                    throw Exception(os.str().c_str());
                }
                
                node["active_displays"] >> activeDisplays_;
            }
            
            if(node.FindValue("active_views") != NULL)
            {
                if(node["active_views"].GetType() != YAML::CT_SEQUENCE)
                {
                    std::ostringstream os;
                    os << "'active_views' field needs to be a list.";
                    throw Exception(os.str().c_str());
                }
                
                node["active_views"] >> activeViews_;
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
                std::string realfilename = path::realpath(filename);
                std::string configrootdir = path::dirname(realfilename);
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

#if 0
OIIO_ADD_TEST(Config_Unit_Tests, test_searchpath_filesystem)
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

OIIO_ADD_TEST(Config_Unit_Tests, test_INTERNAL_RAW_PROFILE)
{
    std::istringstream is;
    is.str(OCIO::INTERNAL_RAW_PROFILE);
    OIIO_CHECK_NO_THOW(OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromStream(is));
}

OIIO_ADD_TEST(Config_Unit_Tests, test_simpleConfig)
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
    "  sRGB:\n"
    "  - !<View> {name: Film1D, colorspace: vd8}\n"
    "  - !<View> {name: Log, colorspace: lg10}\n"
    "  - !<View> {name: Raw, colorspace: raw}\n"
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
    OIIO_CHECK_NO_THOW(config = OCIO::Config::CreateFromStream(is));
}

OIIO_ADD_TEST(Config_Unit_Tests, test_roleAccess)
{
    
    std::string SIMPLE_PROFILE =
    "ocio_profile_version: 1\n"
    "strictparsing: false\n"
    "roles:\n"
    "  compositing_log: lgh\n"
    "  default: raw\n"
    "  scene_linear: lnh\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "      name: raw\n"
    "  - !<ColorSpace>\n"
    "      name: lnh\n"
    "  - !<ColorSpace>\n"
    "      name: lgh\n"
    "\n";
    
    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    OIIO_CHECK_NO_THOW(config = OCIO::Config::CreateFromStream(is));
    
    OIIO_CHECK_EQUAL(config->getNumRoles(), 3);
    
    OIIO_CHECK_ASSERT(config->hasRole("compositing_log") == true);
    OIIO_CHECK_ASSERT(config->hasRole("cheese") == false);
    OIIO_CHECK_ASSERT(config->hasRole("") == false);
    
    OIIO_CHECK_ASSERT(strcmp(config->getRoleName(2), "scene_linear") == 0);
    OIIO_CHECK_ASSERT(strcmp(config->getRoleName(0), "compositing_log") == 0);
    OIIO_CHECK_ASSERT(strcmp(config->getRoleName(1), "default") == 0);
    OIIO_CHECK_ASSERT(strcmp(config->getRoleName(10), "") == 0);
    OIIO_CHECK_ASSERT(strcmp(config->getRoleName(-4), "") == 0);
    
}

OIIO_ADD_TEST(Config_Unit_Tests, test_ser)
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
    "search_path: \"\"\n"
    "strictparsing: true\n"
    "luma: [0.2126, 0.7152, 0.0722]\n"
    "\n"
    "roles:\n"
    "  compositing_log: testing2\n"
    "\n"
    "displays:\n"
    "  {}\n"
    "active_displays: []\n"
    "active_views: []\n"
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
    pystring::splitlines(os.str(), osvec);
    std::vector<std::string> PROFILE_OUTvec;
    pystring::splitlines(PROFILE_OUT, PROFILE_OUTvec);
    
    OIIO_CHECK_EQUAL(osvec.size(), PROFILE_OUTvec.size());
    for(unsigned int i = 0; i < PROFILE_OUTvec.size(); ++i)
        OIIO_CHECK_EQUAL(osvec[i], PROFILE_OUTvec[i]);
}

#endif // OCIO_UNIT_TEST
