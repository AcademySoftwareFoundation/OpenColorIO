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

#include "HashUtils.h"
#include "Logging.h"
#include "LookParse.h"
#include "Display.h"
#include "MathUtils.h"
#include "Mutex.h"
#include "OpBuilders.h"
#include "PathUtils.h"
#include "ParseUtils.h"
#include "Processor.h"
#include "PrivateTypes.h"
#include "pystring/pystring.h"
#include "OCIOYaml.h"
#include "Platform.h"

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
        "      equalitygroup:\n"
        "      bitdepth: 32f\n"
        "      isdata: true\n"
        "      allocation: uniform\n"
        "      description: 'A raw color space. Conversions to and from this space are no-ops.'\n";
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    const char * GetVersion()
    {
        return OCIO_VERSION;
    }
    
    int GetVersionHex()
    {
        return OCIO_VERSION_HEX;
    }
    
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
    
    // Environment
    const char* LookupEnvironment(const StringMap & env,
                                        const std::string & name)
    {
        StringMap::const_iterator iter = env.find(name);
        if(iter == env.end()) return "";
        return iter->second.c_str();
    }
    
    // Roles
    // (lower case role name: colorspace name)
    const char* LookupRole(const StringMap & roles, const std::string & rolename)
    {
        StringMap::const_iterator iter = roles.find(pystring::lower(rolename));
        if(iter == roles.end()) return "";
        return iter->second.c_str();
    }
    
    
    void GetFileReferences(std::set<std::string> & files,
                           const ConstTransformRcPtr & transform)
    {
        if(!transform) return;
        
        if(ConstGroupTransformRcPtr groupTransform = \
            DynamicPtrCast<const GroupTransform>(transform))
        {
            for(int i=0; i<groupTransform->size(); ++i)
            {
                GetFileReferences(files, groupTransform->getTransform(i));
            }
        }
        else if(ConstFileTransformRcPtr fileTransform = \
            DynamicPtrCast<const FileTransform>(transform))
        {
            files.insert(fileTransform->getSrc());
        }
    }
    
    void GetColorSpaceReferences(std::set<std::string> & colorSpaceNames,
                                 const ConstTransformRcPtr & transform,
                                 const ConstContextRcPtr & context)
    {
        if(!transform) return;

        if(ConstGroupTransformRcPtr groupTransform = \
            DynamicPtrCast<const GroupTransform>(transform))
        {
            for(int i=0; i<groupTransform->size(); ++i)
            {
                GetColorSpaceReferences(colorSpaceNames, groupTransform->getTransform(i), context);
            }
        }
        else if(ConstColorSpaceTransformRcPtr colorSpaceTransform = \
            DynamicPtrCast<const ColorSpaceTransform>(transform))
        {
            colorSpaceNames.insert(context->resolveStringVar(colorSpaceTransform->getSrc()));
            colorSpaceNames.insert(context->resolveStringVar(colorSpaceTransform->getDst()));
        }
        else if(ConstDisplayTransformRcPtr displayTransform = \
            DynamicPtrCast<const DisplayTransform>(transform))
        {
            colorSpaceNames.insert(displayTransform->getInputColorSpaceName());
        }
        else if(ConstLookTransformRcPtr lookTransform = \
            DynamicPtrCast<const LookTransform>(transform))
        {
            colorSpaceNames.insert(context->resolveStringVar(colorSpaceTransform->getSrc()));
            colorSpaceNames.insert(context->resolveStringVar(colorSpaceTransform->getDst()));
        }
    }
    
    
    bool FindColorSpaceIndex(int * index,
                             const ColorSpaceSetRcPtr & colorspaces,
                             const std::string & csname)
    {
        *index = colorspaces->getIndexForColorSpace(csname.c_str());
        return *index!=-1;
    }
        
    } // namespace
    
    static const unsigned FirstSupportedMajorVersion_ = 1;
    static const unsigned LastSupportedMajorVersion_  = 2;

    class Config::Impl
    {
    public:
        unsigned int majorVersion_;
        unsigned int minorVersion_;
        StringMap env_;
        ContextRcPtr context_;
        std::string description_;
        ColorSpaceSetRcPtr colorspaces_;

        StringMap roles_;
        LookVec looksList_;
        
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
        
        mutable Mutex cacheidMutex_;
        mutable StringMap cacheids_;
        mutable std::string cacheidnocontext_;
        
        OCIOYaml io_;
        
        Impl() : 
            majorVersion_(FirstSupportedMajorVersion_),
            minorVersion_(0),
            context_(Context::Create()),
            colorspaces_(ColorSpaceSet::Create()),
            strictParsing_(true),
            sanity_(SANITY_UNKNOWN)
        {
            std::string activeDisplays;
            Platform::Getenv(OCIO_ACTIVE_DISPLAYS_ENVVAR, activeDisplays);
            activeDisplays = pystring::strip(activeDisplays);
            if (!activeDisplays.empty()) {
                SplitStringEnvStyle(activeDisplaysEnvOverride_, activeDisplays.c_str());
            }
            
            std::string activeViews;
            Platform::Getenv(OCIO_ACTIVE_VIEWS_ENVVAR, activeViews);
            activeViews = pystring::strip(activeViews);
            if (!activeViews.empty()) {
                SplitStringEnvStyle(activeViewsEnvOverride_, activeViews.c_str());
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
            if(this!=&rhs)
            {
                majorVersion_ = rhs.majorVersion_;
                minorVersion_ = rhs.minorVersion_;

                env_ = rhs.env_;
                context_ = rhs.context_->createEditableCopy();
                description_ = rhs.description_;
                
                // Deep copy the colorspaces
                colorspaces_ = rhs.colorspaces_;
                
                // Deep copy the looks
                looksList_.clear();
                looksList_.reserve(rhs.looksList_.size());
                for(unsigned int i=0; i<rhs.looksList_.size(); ++i)
                {
                    looksList_.push_back(
                        rhs.looksList_[i]->createEditableCopy());
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
                
                cacheids_ = rhs.cacheids_;
                cacheidnocontext_ = rhs.cacheidnocontext_;
            }
            return *this;
        }

        // Any time you modify the state of the config, you must call this
        // to reset internal cache states.  You also should do this in a
        // thread safe manner by acquiring the cacheidMutex_;
        void resetCacheIDs();
        
        // Get all internal transforms (to generate cacheIDs, validation, etc).
        // This currently crawls colorspaces + looks
        void getAllIntenalTransforms(ConstTransformVec & transformVec) const;
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
        std::string file;
        Platform::Getenv(OCIO_CONFIG_ENVVAR, file);
        if(!file.empty()) return CreateFromFile(file.c_str());
        
        std::ostringstream os;
        os << "Color management disabled. ";
        os << "(Specify the $OCIO environment variable to enable.)";
        LogInfo(os.str());
        
        std::istringstream istream;
        istream.str(INTERNAL_RAW_PROFILE);
        
        ConfigRcPtr config = Config::Create();
        config->getImpl()->io_.open(istream, config);
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
        config->getImpl()->io_.open(istream, config, filename);
        return config;
    }
    
    ConstConfigRcPtr Config::CreateFromStream(std::istream & istream)
    {
        ConfigRcPtr config = Config::Create();
        config->getImpl()->io_.open(istream, config);
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
    
    unsigned Config::getMajorVersion() const
    {
        return m_impl->majorVersion_;
    }

    void Config::setMajorVersion(unsigned int version)
    {
        if(version <  FirstSupportedMajorVersion_
            || version >  LastSupportedMajorVersion_)
        {
            std::ostringstream os;
             os << "The version is " << version 
                << " where supported versions start at " 
                << FirstSupportedMajorVersion_
                << " and end at "
                << LastSupportedMajorVersion_
                << ".";
             throw Exception(os.str().c_str());
        }
         m_impl->majorVersion_ = version;
    }

    unsigned Config::getMinorVersion() const
    {
        return m_impl->minorVersion_;
    }

    void Config::setMinorVersion(unsigned int version)
    {
         m_impl->minorVersion_ = version;
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

        ///// COLORSPACES
        StringSet existingColorSpaces;
        
        // Confirm all ColorSpaces are valid
        for(int i=0; i<getImpl()->colorspaces_->getNumColorSpaces(); ++i)
        {
            if(!getImpl()->colorspaces_->getColorSpaceByIndex(i))
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The colorspace at index " << i << " is null.";
                getImpl()->sanitytext_ = os.str();
                throw Exception(getImpl()->sanitytext_.c_str());
            }
            
            const char * name = getImpl()->colorspaces_->getColorSpaceByIndex(i)->getName();
            if(!name || strlen(name) == 0)
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The colorspace at index " << i << " is not named.";
                getImpl()->sanitytext_ = os.str();
                throw Exception(getImpl()->sanitytext_.c_str());
            }
            
            std::string namelower = pystring::lower(name);
            StringSet::const_iterator it = existingColorSpaces.find(namelower);
            if(it != existingColorSpaces.end())
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "Two colorspaces are defined with the same name, '";
                os << namelower << "'.";
                getImpl()->sanitytext_ = os.str();
                throw Exception(getImpl()->sanitytext_.c_str());
            }

            ConstTransformRcPtr toTrans 
                = getImpl()->colorspaces_->getColorSpaceByIndex(i)->getTransform(COLORSPACE_DIR_TO_REFERENCE);
            if (toTrans)
            {
                toTrans->validate();
            }

            ConstTransformRcPtr fromTrans 
                = getImpl()->colorspaces_->getColorSpaceByIndex(i)->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
            if (fromTrans)
            {
                fromTrans->validate();
            }

            existingColorSpaces.insert(namelower);
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
        
        ///// DISPLAYS
        
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
                
                // Confirm looks references exist
                LookParseResult looks;
                const LookParseResult::Options & options = looks.parse(views[i].looks);
                
                for(unsigned int optionindex=0;
                    optionindex<options.size();
                    ++optionindex)
                {
                    for(unsigned int tokenindex=0;
                        tokenindex<options[optionindex].size();
                        ++tokenindex)
                    {
                        std::string look = options[optionindex][tokenindex].name;
                        
                        if(!look.empty() && !getLook(look.c_str()))
                        {
                            std::ostringstream os;
                            os << "Config failed sanitycheck. ";
                            os << "The display '" << display << "' ";
                            os << "refers to a look, '" << look << "', ";
                            os << "which is not defined.";
                            getImpl()->sanitytext_ = os.str();
                            throw Exception(getImpl()->sanitytext_.c_str());
                        }
                    }
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
        
        // Confirm for all Transforms that reference internal colorspaces,
        // the named space exists and that all Transforms are valid.
        {
            ConstTransformVec allTransforms;
            getImpl()->getAllIntenalTransforms(allTransforms);
            
            std::set<std::string> colorSpaceNames;
            for(unsigned int i=0; i<allTransforms.size(); ++i)
            {
                allTransforms[i]->validate();

                ConstContextRcPtr context = getCurrentContext();       
                GetColorSpaceReferences(colorSpaceNames, allTransforms[i], context);
            }
            
            for(std::set<std::string>::iterator iter = colorSpaceNames.begin();
                iter != colorSpaceNames.end(); ++iter)
            {
                int csindex = -1;
                if(!FindColorSpaceIndex(&csindex, getImpl()->colorspaces_, *iter))
                {
                    std::ostringstream os;
                    os << "Config failed sanitycheck. ";
                    os << "This config references a ColorSpace, '" << *iter << "', ";
                    os << "which is not defined.";
                    getImpl()->sanitytext_ = os.str();
                    throw Exception(getImpl()->sanitytext_.c_str());
                }
            }
        }
        
        ///// LOOKS
        
        // For all looks, confirm the process space exists and the look is named
        for(unsigned int i=0; i<getImpl()->looksList_.size(); ++i)
        {
            std::string name = getImpl()->looksList_[i]->getName();
            if(name.empty())
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The look at index '" << i << "' ";
                os << "does not specify a name.";
                getImpl()->sanitytext_ = os.str();
                throw Exception(getImpl()->sanitytext_.c_str());
            }
            
            std::string processSpace = getImpl()->looksList_[i]->getProcessSpace();
            if(processSpace.empty())
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The look '" << name << "' ";
                os << "does not specify a process space.";
                getImpl()->sanitytext_ = os.str();
                throw Exception(getImpl()->sanitytext_.c_str());
            }
            
            int csindex=0;
            if(!FindColorSpaceIndex(&csindex, getImpl()->colorspaces_, processSpace))
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The look '" << name << "' ";
                os << "specifies a process color space, '";
                os << processSpace << "', which is not defined.";
                getImpl()->sanitytext_ = os.str();
                throw Exception(getImpl()->sanitytext_.c_str());
            }
        }

        // Validate all transforms
        ConstTransformVec allTransforms;
        getImpl()->getAllIntenalTransforms(allTransforms);
         for (unsigned int i = 0; i<allTransforms.size(); ++i)
        {
            allTransforms[i]->validate();
        }

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
        getImpl()->description_ = description;
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    
    // RESOURCES //////////////////////////////////////////////////////////////
    
    ConstContextRcPtr Config::getCurrentContext() const
    {
        return getImpl()->context_;
    }
    
    void Config::addEnvironmentVar(const char * name, const char * defaultValue)
    {
        if(defaultValue)
        {
            getImpl()->env_[std::string(name)] = std::string(defaultValue);
            getImpl()->context_->setStringVar(name, defaultValue);
        }
        else
        {
            StringMap::iterator iter = getImpl()->env_.find(std::string(name));
            if(iter != getImpl()->env_.end()) getImpl()->env_.erase(iter);
        }
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    int Config::getNumEnvironmentVars() const
    {
        return static_cast<int>(getImpl()->env_.size());
    }
    
    const char * Config::getEnvironmentVarNameByIndex(int index) const
    {
        if(index < 0 || index >= (int)getImpl()->env_.size()) return "";
        StringMap::const_iterator iter = getImpl()->env_.begin();
        for(int i = 0; i < index; ++i) ++iter;
        return iter->first.c_str();
    }
    
    const char * Config::getEnvironmentVarDefault(const char * name) const
    {
        return LookupEnvironment(getImpl()->env_, name);
    }
    
    void Config::clearEnvironmentVars()
    {
        getImpl()->env_.clear();
        getImpl()->context_->clearStringVars();
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    void Config::setEnvironmentMode(EnvironmentMode mode)
    {
        getImpl()->context_->setEnvironmentMode(mode);
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    EnvironmentMode Config::getEnvironmentMode() const
    {
        return getImpl()->context_->getEnvironmentMode();
    }
    
    void Config::loadEnvironment()
    {
        getImpl()->context_->loadEnvironment();
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    const char * Config::getSearchPath() const
    {
        return getImpl()->context_->getSearchPath();
    }
    
    void Config::setSearchPath(const char * path)
    {
        getImpl()->context_->setSearchPath(path);
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    int Config::getNumSearchPaths() const
    {
        return getImpl()->context_->getNumSearchPaths();
    }

    const char * Config::getSearchPath(int index) const
    {
        return getImpl()->context_->getSearchPath(index);
    }

    void Config::clearSearchPaths()
    {
        getImpl()->context_->clearSearchPaths();

        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }

    void Config::addSearchPath(const char * path)
    {
        getImpl()->context_->addSearchPath(path);

        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }

    const char * Config::getWorkingDir() const
    {
        return getImpl()->context_->getWorkingDir();
    }
    
    void Config::setWorkingDir(const char * dirname)
    {
        getImpl()->context_->setWorkingDir(dirname);
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    ColorSpaceSetRcPtr Config::getColorSpaces(const char * category) const
    {
        ColorSpaceSetRcPtr res = ColorSpaceSet::Create();

        for(int i=0; i<getImpl()->colorspaces_->getNumColorSpaces(); ++i)
        {
            ConstColorSpaceRcPtr cs = getImpl()->colorspaces_->getColorSpaceByIndex(i);
            if(!category || !*category || cs->hasCategory(category))
            {
                res->addColorSpace(cs);
            }
        }

        return res;
    }

    int Config::getNumColorSpaces() const
    {
        return getImpl()->colorspaces_->getNumColorSpaces();
    }
    
    const char * Config::getColorSpaceNameByIndex(int index) const
    {
        return getImpl()->colorspaces_->getColorSpaceNameByIndex(index);
    }
    
    ConstColorSpaceRcPtr Config::getColorSpace(const char * name) const
    {
        int index = getIndexForColorSpace(name);
        if(index<0 || index >= (int)getImpl()->colorspaces_->getNumColorSpaces())
        {
            return ColorSpaceRcPtr();
        }
        
        return getImpl()->colorspaces_->getColorSpaceByIndex(index);
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
        const char* csname = LookupRole(getImpl()->roles_, name);
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
        getImpl()->colorspaces_->addColorSpace(original);
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    void Config::clearColorSpaces()
    {
        getImpl()->colorspaces_->clearColorSpaces();
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    
    
    
    
    
    const char * Config::parseColorSpaceFromString(const char * str) const
    {
        if(!str) return "";
        
        // Search the entire filePath, including directory name (if provided)
        // convert the filename to lowercase.
        std::string fullstr = pystring::lower(std::string(str));
        
        // See if it matches a LUT name.
        // This is the position of the RIGHT end of the colorspace substring,
        // not the left
        int rightMostColorPos=-1;
        std::string rightMostColorspace = "";
        int rightMostColorSpaceIndex = -1;
        
        // Find the right-most occcurance within the string for each colorspace.
        for (int i=0; i<getImpl()->colorspaces_->getNumColorSpaces(); ++i)
        {
            std::string csname = pystring::lower(getImpl()->colorspaces_->getColorSpaceNameByIndex(i));
            
            // find right-most extension matched in filename
            int colorspacePos = pystring::rfind(fullstr, csname);
            if(colorspacePos < 0)
                continue;
            
            // If we have found a match, move the pointer over to the right end
            // of the substring.  This will allow us to find the longest name
            // that matches the rightmost colorspace
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
            return getImpl()->colorspaces_->getColorSpaceNameByIndex(rightMostColorSpaceIndex);
        }
        
        if(!getImpl()->strictParsing_)
        {
            // Is a default role defined?
            const char* csname = LookupRole(getImpl()->roles_, ROLE_DEFAULT);
            if(csname && *csname)
            {
                int csindex = -1;
                if( FindColorSpaceIndex(&csindex, getImpl()->colorspaces_, csname) )
                {
                    // This is necessary to not return a reference to
                    // a local variable.
                    return getImpl()->colorspaces_->getColorSpaceNameByIndex(csindex);
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
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    // Roles
    void Config::setRole(const char * role, const char * colorSpaceName)
    {
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
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    int Config::getNumRoles() const
    {
        return static_cast<int>(getImpl()->roles_.size());
    }
    
    bool Config::hasRole(const char * role) const
    {
        const char* rname = LookupRole(getImpl()->roles_, role);
        return  rname && *rname;
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
        
        if(!display) return "";
        
        DisplayMap::const_iterator iter = find_display_const(getImpl()->displays_, display);
        if(iter == getImpl()->displays_.end()) return "";
        
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
        if(iter == getImpl()->displays_.end()) return "";
        
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
    
    const char * Config::getDisplayLooks(const char * display, const char * view) const
    {
        if(!display || !view) return "";
        
        DisplayMap::const_iterator iter = find_display_const(getImpl()->displays_, display);
        if(iter == getImpl()->displays_.end()) return "";
        
        const ViewVec & views = iter->second;
        int index = find_view(views, view);
        if(index<0) return "";
        
        return views[index].looks.c_str();
    }
    
    void Config::addDisplay(const char * display, const char * view,
                            const char * colorSpaceName, const char * lookName)
    {
        
        if(!display || !view || !colorSpaceName || !lookName) return;
        
        AddDisplay(getImpl()->displays_,
                   display, view, colorSpaceName, lookName);
        getImpl()->displayCache_.clear();
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    void Config::clearDisplays()
    {
        getImpl()->displays_.clear();
        getImpl()->displayCache_.clear();
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    void Config::setActiveDisplays(const char * displays)
    {
        getImpl()->activeDisplays_.clear();
        SplitStringEnvStyle(getImpl()->activeDisplays_, displays);
        
        getImpl()->displayCache_.clear();
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }

    const char * Config::getActiveDisplays() const
    {
        getImpl()->activeDisplaysStr_ = JoinStringEnvStyle(getImpl()->activeDisplays_);
        return getImpl()->activeDisplaysStr_.c_str();
    }
    
    void Config::setActiveViews(const char * views)
    {
        getImpl()->activeViews_.clear();
        SplitStringEnvStyle(getImpl()->activeViews_, views);
        
        getImpl()->displayCache_.clear();
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
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
        memcpy(&getImpl()->defaultLumaCoefs_[0], c3, 3*sizeof(float));
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    
    ConstLookRcPtr Config::getLook(const char * name) const
    {
        std::string namelower = pystring::lower(name);
        
        for(unsigned int i=0; i<getImpl()->looksList_.size(); ++i)
        {
            if(pystring::lower(getImpl()->looksList_[i]->getName()) == namelower)
            {
                return getImpl()->looksList_[i];
            }
        }
        
        return ConstLookRcPtr();
    }
    
    int Config::getNumLooks() const
    {
        return static_cast<int>(getImpl()->looksList_.size());
    }
    
    const char * Config::getLookNameByIndex(int index) const
    {
        if(index<0 || index>=static_cast<int>(getImpl()->looksList_.size()))
        {
            return "";
        }
        
        return getImpl()->looksList_[index]->getName();
    }
    
    void Config::addLook(const ConstLookRcPtr & look)
    {
        std::string name = look->getName();
        if(name.empty())
            throw Exception("Cannot addLook with an empty name.");
        
        std::string namelower = pystring::lower(name);
        
        // If the look exists, replace it
        for(unsigned int i=0; i<getImpl()->looksList_.size(); ++i)
        {
            if(pystring::lower(getImpl()->looksList_[i]->getName()) == namelower)
            {
                getImpl()->looksList_[i] = look->createEditableCopy();
                return;
            }
        }
        
        // Otherwise, add it
        getImpl()->looksList_.push_back(look->createEditableCopy());
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    void Config::clearLooks()
    {
        getImpl()->looksList_.clear();
        
        AutoMutex lock(getImpl()->cacheidMutex_);
        getImpl()->resetCacheIDs();
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
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
    
    // Names can be colorspace name or role name
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
    //  CacheID
    
    const char * Config::getCacheID() const
    {
        return getCacheID(getCurrentContext());
    }
    
    const char * Config::getCacheID(const ConstContextRcPtr & context) const
    {
        AutoMutex lock(getImpl()->cacheidMutex_);
        
        // A null context will use the empty cacheid
        std::string contextcacheid = "";
        if(context) contextcacheid = context->getCacheID();
        
        StringMap::const_iterator cacheiditer = getImpl()->cacheids_.find(contextcacheid);
        if(cacheiditer != getImpl()->cacheids_.end())
        {
            return cacheiditer->second.c_str();
        }
        
        // Include the hash of the yaml config serialization
        if(getImpl()->cacheidnocontext_.empty())
        {
            std::stringstream cacheid;
            serialize(cacheid);
            std::string fullstr = cacheid.str();
            getImpl()->cacheidnocontext_ = CacheIDHash(fullstr.c_str(), (int)fullstr.size());
        }
        
        // Also include all file references, using the context (if specified)
        std::string fileReferencesFashHash = "";
        if(context)
        {
            std::ostringstream filehash;
            
            ConstTransformVec allTransforms;
            getImpl()->getAllIntenalTransforms(allTransforms);
            
            std::set<std::string> files;
            for(unsigned int i=0; i<allTransforms.size(); ++i)
            {
                GetFileReferences(files, allTransforms[i]);
            }
            
            for(std::set<std::string>::iterator iter = files.begin();
                iter != files.end(); ++iter)
            {
                if(iter->empty()) continue;
                filehash << *iter << "=";
                
                try
                {
                    std::string resolvedLocation = context->resolveFileLocation(iter->c_str());
                    filehash << GetFastFileHash(resolvedLocation) << " ";
                }
                catch(...)
                {
                    filehash << "? ";
                    continue;
                }
            }
            
            std::string fullstr = filehash.str();
            fileReferencesFashHash = CacheIDHash(fullstr.c_str(), (int)fullstr.size());
        }
        
        getImpl()->cacheids_[contextcacheid] = getImpl()->cacheidnocontext_ + ":" + fileReferencesFashHash;
        return getImpl()->cacheids_[contextcacheid].c_str();
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    //  Serialization
    
    void Config::serialize(std::ostream& os) const
    {
        try
        {
            getImpl()->io_.write(os, this);
        }
        catch( const std::exception & e)
        {
            std::ostringstream error;
            error << "Error building YAML: " << e.what();
            throw Exception(error.str().c_str());
        }
    }
    
    void Config::Impl::resetCacheIDs()
    {
        cacheids_.clear();
        cacheidnocontext_ = "";
        sanity_ = SANITY_UNKNOWN;
        sanitytext_ = "";
    }
    
    void Config::Impl::getAllIntenalTransforms(ConstTransformVec & transformVec) const
    {
        // Grab all transforms from the ColorSpaces
        for(int i=0; i<colorspaces_->getNumColorSpaces(); ++i)
        {
            if(colorspaces_->getColorSpaceByIndex(i)->getTransform(COLORSPACE_DIR_TO_REFERENCE))
            {
                transformVec.push_back(
                    colorspaces_->getColorSpaceByIndex(i)->getTransform(COLORSPACE_DIR_TO_REFERENCE));
            }
            if(colorspaces_->getColorSpaceByIndex(i)->getTransform(COLORSPACE_DIR_FROM_REFERENCE))
            {
                transformVec.push_back(
                    colorspaces_->getColorSpaceByIndex(i)->getTransform(COLORSPACE_DIR_FROM_REFERENCE));
            }
        }
        
        // Grab all transforms from the Looks
        for(unsigned int i=0; i<looksList_.size(); ++i)
        {
            if(looksList_[i]->getTransform())
                transformVec.push_back(looksList_[i]->getTransform());
            if(looksList_[i]->getInverseTransform())
                transformVec.push_back(looksList_[i]->getInverseTransform());
        }
    
    }
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

#include <sys/stat.h>
#include "pystring/pystring.h"

#if 0
OIIO_ADD_TEST(Config, test_searchpath_filesystem)
{
    
    OCIO::EnvMap env = OCIO::GetEnvMap();
    std::string OCIO_TEST_AREA("$OCIO_TEST_AREA");
    EnvExpand(&OCIO_TEST_AREA, &env);
    
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    
    // basic get/set/expand
    config->setSearchPath("."
                          ":$OCIO_TEST1"
                          ":/$OCIO_JOB/${OCIO_SEQ}/$OCIO_SHOT/ocio");
    
    OIIO_CHECK_ASSERT(strcmp(config->getSearchPath(),
        ".:$OCIO_TEST1:/$OCIO_JOB/${OCIO_SEQ}/$OCIO_SHOT/ocio") == 0);
    OIIO_CHECK_ASSERT(strcmp(config->getSearchPath(true),
        ".:foobar:/meatballs/cheesecake/mb-cc-001/ocio") == 0);
    
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
    OIIO_CHECK_ASSERT(strcmp(config->findFile("somelut1.lut"),
        lut1.c_str()) == 0);
    OIIO_CHECK_ASSERT(strcmp(config->findFile("somelut2.lut"),
        lut2.c_str()) == 0);
    OIIO_CHECK_ASSERT(strcmp(config->findFile("somelut3.lut"),
        lut3.c_str()) == 0);
    OIIO_CHECK_ASSERT(strcmp(config->findFile("lutdotdot.lut"),
        lutdotdot.c_str()) == 0);
    
}
#endif

OIIO_ADD_TEST(Config, internal_raw_profile)
{
    std::istringstream is;
    is.str(OCIO::INTERNAL_RAW_PROFILE);
    OIIO_CHECK_NO_THROW(OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromStream(is));
}

OIIO_ADD_TEST(Config, simple_config)
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
    "      equalitygroup: \n"
    "      bitdepth: 32f\n"
    "      description: |\n"
    "        A raw color space. Conversions to and from this space are no-ops.\n"
    "      isdata: true\n"
    "      allocation: uniform\n"
    "  - !<ColorSpace>\n"
    "      name: lnh\n"
    "      family: ln\n"
    "      equalitygroup: \n"
    "      bitdepth: 16f\n"
    "      description: |\n"
    "        The show reference space. This is a sensor referred linear\n"
    "        representation of the scene with primaries that correspond to\n"
    "        scanned film. 0.18 in this space corresponds to a properly\n"
    "        exposed 18% grey card.\n"
    "      isdata: false\n"
    "      allocation: lg2\n"
    "  - !<ColorSpace>\n"
    "      name: loads_of_transforms\n"
    "      family: vd8\n"
    "      equalitygroup: \n"
    "      bitdepth: 8ui\n"
    "      description: 'how many transforms can we use?'\n"
    "      isdata: false\n"
    "      allocation: uniform\n"
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
    OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
}

OIIO_ADD_TEST(Config, roles)
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
    OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    
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

OIIO_ADD_TEST(Config, serialize)
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
    "\n"
    "active_displays: []\n"
    "active_views: []\n"
    "\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "    name: testing\n"
    "    family: test\n"
    "    equalitygroup: \"\"\n"
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
    "    equalitygroup: \"\"\n"
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
    
    OIIO_CHECK_EQUAL(osvec.size(), PROFILE_OUTvec.size());
    for(unsigned int i = 0; i < PROFILE_OUTvec.size(); ++i)
        OIIO_CHECK_EQUAL(osvec[i], PROFILE_OUTvec[i]);
}

OIIO_ADD_TEST(Config, serialize_searchpath)
{
    {
        OCIO::ConfigRcPtr config = OCIO::Config::Create();

        std::ostringstream os;
        config->serialize(os);

        std::string PROFILE_OUT =
            "ocio_profile_version: 1\n"
            "\n"
            "search_path: \"\"\n"
            "strictparsing: true\n"
            "luma: [0.2126, 0.7152, 0.0722]\n"
            "\n"
            "roles:\n"
            "  {}\n"
            "\n"
            "displays:\n"
            "  {}\n"
            "\n"
            "active_displays: []\n"
            "active_views: []\n"
            "\n"
            "colorspaces:\n"
            "  []";

        std::vector<std::string> osvec;
        OCIO::pystring::splitlines(os.str(), osvec);
        std::vector<std::string> PROFILE_OUTvec;
        OCIO::pystring::splitlines(PROFILE_OUT, PROFILE_OUTvec);

        OIIO_CHECK_EQUAL(osvec.size(), PROFILE_OUTvec.size());
        for (unsigned int i = 0; i < PROFILE_OUTvec.size(); ++i)
            OIIO_CHECK_EQUAL(osvec[i], PROFILE_OUTvec[i]);
    }

    {
        OCIO::ConfigRcPtr config = OCIO::Config::Create();

        std::string searchPath("a:b:c");
        config->setSearchPath(searchPath.c_str());

        std::ostringstream os;
        config->serialize(os);

        std::vector<std::string> osvec;
        OCIO::pystring::splitlines(os.str(), osvec);

        const std::string expected1{ "search_path: a:b:c" };
        OIIO_CHECK_EQUAL(osvec[2], expected1);

        config->setMajorVersion(2);
        os.clear();
        os.str("");
        config->serialize(os);

        osvec.clear();
        OCIO::pystring::splitlines(os.str(), osvec);

        const std::string expected2[] = { "search_path:", "  - a", "  - b", "  - c" };
        OIIO_CHECK_EQUAL(osvec[2], expected2[0]);
        OIIO_CHECK_EQUAL(osvec[3], expected2[1]);
        OIIO_CHECK_EQUAL(osvec[4], expected2[2]);
        OIIO_CHECK_EQUAL(osvec[5], expected2[3]);

        std::istringstream is;
        is.str(os.str());
        OCIO::ConstConfigRcPtr configRead;
        OIIO_CHECK_NO_THROW(configRead = OCIO::Config::CreateFromStream(is));

        OIIO_CHECK_EQUAL(configRead->getNumSearchPaths(), 3);
        OIIO_CHECK_EQUAL(std::string(configRead->getSearchPath()), searchPath);
        OIIO_CHECK_EQUAL(std::string(configRead->getSearchPath(0)), std::string("a"));
        OIIO_CHECK_EQUAL(std::string(configRead->getSearchPath(1)), std::string("b"));
        OIIO_CHECK_EQUAL(std::string(configRead->getSearchPath(2)), std::string("c"));

        os.clear();
        os.str("");
        config->clearSearchPaths();
        const std::string sp0{ "a path with a - in it/" };
        const std::string sp1{ "/absolute/linux/path" };
        const std::string sp2{ "C:\\absolute\\windows\\path" };
        const std::string sp3{ "!<path> usiing /yaml/symbols" };
        config->addSearchPath(sp0.c_str());
        config->addSearchPath(sp1.c_str());
        config->addSearchPath(sp2.c_str());
        config->addSearchPath(sp3.c_str());
        config->serialize(os);

        osvec.clear();
        OCIO::pystring::splitlines(os.str(), osvec);

        const std::string expected3[] = { "search_path:",
                                          "  - a path with a - in it/",
                                          "  - /absolute/linux/path",
                                          "  - C:\\absolute\\windows\\path",
                                          "  - \"!<path> usiing /yaml/symbols\"" };
        OIIO_CHECK_EQUAL(osvec[2], expected3[0]);
        OIIO_CHECK_EQUAL(osvec[3], expected3[1]);
        OIIO_CHECK_EQUAL(osvec[4], expected3[2]);
        OIIO_CHECK_EQUAL(osvec[5], expected3[3]);
        OIIO_CHECK_EQUAL(osvec[6], expected3[4]);

        is.clear();
        is.str(os.str());
        OIIO_CHECK_NO_THROW(configRead = OCIO::Config::CreateFromStream(is));

        OIIO_CHECK_EQUAL(configRead->getNumSearchPaths(), 4);
        OIIO_CHECK_EQUAL(std::string(configRead->getSearchPath(0)), sp0);
        OIIO_CHECK_EQUAL(std::string(configRead->getSearchPath(1)), sp1);
        OIIO_CHECK_EQUAL(std::string(configRead->getSearchPath(2)), sp2);
        OIIO_CHECK_EQUAL(std::string(configRead->getSearchPath(3)), sp3);
    }
}

OIIO_ADD_TEST(Config, sanity_check)
{
    {
    std::string SIMPLE_PROFILE =
    "ocio_profile_version: 1\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "      name: raw\n"
    "  - !<ColorSpace>\n"
    "      name: raw\n"
    "strictparsing: false\n"
    "roles:\n"
    "  default: raw\n"
    "displays:\n"
    "  sRGB:\n"
    "  - !<View> {name: Raw, colorspace: raw}\n"
    "\n";
    
    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    OIIO_CHECK_THROW(config = OCIO::Config::CreateFromStream(is), OCIO::Exception);
    }
    
    {
    std::string SIMPLE_PROFILE =
    "ocio_profile_version: 1\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "      name: raw\n"
    "strictparsing: false\n"
    "roles:\n"
    "  default: raw\n"
    "displays:\n"
    "  sRGB:\n"
    "  - !<View> {name: Raw, colorspace: raw}\n"
    "\n";
    
    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    
    OIIO_CHECK_NO_THROW(config->sanityCheck());
    }
}


OIIO_ADD_TEST(config, env_check)
{
    {
    std::string SIMPLE_PROFILE =
    "ocio_profile_version: 1\n"
    "environment:\n"
    "  SHOW: super\n"
    "  SHOT: test\n"
    "  SEQ: foo\n"
    "  test: bar${cheese}\n"
    "  cheese: chedder\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "      name: raw\n"
    "strictparsing: false\n"
    "roles:\n"
    "  default: raw\n"
    "displays:\n"
    "  sRGB:\n"
    "  - !<View> {name: Raw, colorspace: raw}\n"
    "\n";
    
    std::string SIMPLE_PROFILE2 =
    "ocio_profile_version: 1\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "      name: raw\n"
    "strictparsing: false\n"
    "roles:\n"
    "  default: raw\n"
    "displays:\n"
    "  sRGB:\n"
    "  - !<View> {name: Raw, colorspace: raw}\n"
    "\n";
    
    
    const std::string test("SHOW=bar");
    putenv(const_cast<char *>(test.c_str()));
    const std::string test2("TASK=lighting");
    putenv(const_cast<char *>(test2.c_str()));
    
    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OIIO_CHECK_EQUAL(config->getNumEnvironmentVars(), 5);
    OIIO_CHECK_ASSERT(strcmp(config->getCurrentContext()->resolveStringVar("test${test}"),
        "testbarchedder") == 0);
    OIIO_CHECK_ASSERT(strcmp(config->getCurrentContext()->resolveStringVar("${SHOW}"),
        "bar") == 0);
    OIIO_CHECK_ASSERT(strcmp(config->getEnvironmentVarDefault("SHOW"), "super") == 0);
    
    OCIO::ConfigRcPtr edit = config->createEditableCopy();
    edit->clearEnvironmentVars();
    OIIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 0);
    
    edit->addEnvironmentVar("testing", "dupvar");
    edit->addEnvironmentVar("testing", "dupvar");
    edit->addEnvironmentVar("foobar", "testing");
    edit->addEnvironmentVar("blank", "");
    edit->addEnvironmentVar("dontadd", NULL);
    OIIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 3);
    edit->addEnvironmentVar("foobar", NULL); // remove
    OIIO_CHECK_EQUAL(edit->getNumEnvironmentVars(), 2);
    edit->clearEnvironmentVars();
    
    edit->addEnvironmentVar("SHOW", "super");
    edit->addEnvironmentVar("SHOT", "test");
    edit->addEnvironmentVar("SEQ", "foo");
    edit->addEnvironmentVar("test", "bar${cheese}");
    edit->addEnvironmentVar("cheese", "chedder");
    
    //Test
    OCIO::LoggingLevel loglevel = OCIO::GetLoggingLevel();
    OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_DEBUG);
    is.str(SIMPLE_PROFILE2);
    OCIO::ConstConfigRcPtr noenv;
    OIIO_CHECK_NO_THROW(noenv = OCIO::Config::CreateFromStream(is));
    OIIO_CHECK_ASSERT(strcmp(noenv->getCurrentContext()->resolveStringVar("${TASK}"),
        "lighting") == 0);
    OCIO::SetLoggingLevel(loglevel);
    
    OIIO_CHECK_EQUAL(edit->getEnvironmentMode(), OCIO::ENV_ENVIRONMENT_LOAD_PREDEFINED);
    edit->setEnvironmentMode(OCIO::ENV_ENVIRONMENT_LOAD_ALL);
    OIIO_CHECK_EQUAL(edit->getEnvironmentMode(), OCIO::ENV_ENVIRONMENT_LOAD_ALL);
    
    }
}

OIIO_ADD_TEST(Config, role_without_colorspace)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create()->createEditableCopy();
    config->setRole("reference", "UnknownColorSpace");

    std::ostringstream os;
    OIIO_CHECK_THROW(config->serialize(os), OCIO::Exception);
}

OIIO_ADD_TEST(Config, env_colorspace_name)
{
    const std::string MY_OCIO_CONFIG =
        "ocio_profile_version: 1\n"
        "\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  compositing_log: lgh\n"
        "  default: raw\n"
        "  scene_linear: lnh\n"
        "\n"
        "displays:\n"
        "  sRGB:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "\n"
        "active_displays: []\n"
        "active_views: []\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: lnh\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: lgh\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "    allocationvars: [-0.125, 1.125]\n";


    {
        // Test when the env. variable is missing

        const std::string 
            myConfigStr = MY_OCIO_CONFIG
                + "    from_reference: !<ColorSpaceTransform> {src: raw, dst: $MISSING_ENV}\n";

        std::istringstream is;
        is.str(myConfigStr);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_THROW(config->sanityCheck(), OCIO::Exception);
        OIIO_CHECK_THROW(config->getProcessor("raw", "lgh"), OCIO::Exception);
    }

    {
        // Test when the env. variable exists but its content is wrong
        const std::string env("OCIO_TEST=FaultyColorSpaceName");
        putenv(const_cast<char*>(env.c_str()));

        const std::string 
            myConfigStr = MY_OCIO_CONFIG
                + "    from_reference: !<ColorSpaceTransform> {src: raw, dst: $OCIO_TEST}\n";

        std::istringstream is;
        is.str(myConfigStr);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_THROW(config->sanityCheck(), OCIO::Exception);
        OIIO_CHECK_THROW(config->getProcessor("raw", "lgh"), OCIO::Exception);
    }

    {
        // Test when the env. variable exists and its content is right
        const std::string env("OCIO_TEST=lnh");
        putenv(const_cast<char*>(env.c_str()));

        const std::string 
            myConfigStr = MY_OCIO_CONFIG
                + "    from_reference: !<ColorSpaceTransform> {src: raw, dst: $OCIO_TEST}\n";

        std::istringstream is;
        is.str(myConfigStr);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());
        OIIO_CHECK_NO_THROW(config->getProcessor("raw", "lgh"));
    }

    {
        // Check that the serialization preserves the env. variable
        const std::string env("OCIO_TEST=lnh");
        putenv(const_cast<char*>(env.c_str()));

        const std::string 
            myConfigStr = MY_OCIO_CONFIG
                + "    from_reference: !<ColorSpaceTransform> {src: raw, dst: $OCIO_TEST}\n";

        std::istringstream is;
        is.str(myConfigStr);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), myConfigStr);
    }
}

OIIO_ADD_TEST(Config, version)
{
    const std::string SIMPLE_PROFILE =
        "ocio_profile_version: 2\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "      name: raw\n"
        "strictparsing: false\n"
        "roles:\n"
        "  default: raw\n"
        "displays:\n"
        "  sRGB:\n"
        "  - !<View> {name: Raw, colorspace: raw}\n"
        "\n";
    
    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConfigRcPtr config;
    OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    
    OIIO_CHECK_NO_THROW(config->sanityCheck());

    OIIO_CHECK_NO_THROW(config->setMajorVersion(1));
    OIIO_CHECK_THROW(config->setMajorVersion(20000), OCIO::Exception);

    {
        OIIO_CHECK_NO_THROW(config->setMinorVersion(2));
        OIIO_CHECK_NO_THROW(config->setMinorVersion(20));

        std::stringstream ss;
        ss << *config.get();   
        OCIO::pystring::startswith(
            OCIO::pystring::lower(ss.str()), "ocio_profile_version: 2.20");
    }

    {
        OIIO_CHECK_NO_THROW(config->setMinorVersion(0));

        std::stringstream ss;
        ss << *config.get();   
        OCIO::pystring::startswith(
            OCIO::pystring::lower(ss.str()), "ocio_profile_version: 2");
    }

    {
        OIIO_CHECK_NO_THROW(config->setMinorVersion(1));

        std::stringstream ss;
        ss << *config.get();   
        OCIO::pystring::startswith(
            OCIO::pystring::lower(ss.str()), "ocio_profile_version: 1");
    }
}

OIIO_ADD_TEST(Config, version_faulty_1)
{
    const std::string SIMPLE_PROFILE =
        "ocio_profile_version: 2.0.1\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "      name: raw\n"
        "strictparsing: false\n"
        "roles:\n"
        "  default: raw\n"
        "displays:\n"
        "  sRGB:\n"
        "  - !<View> {name: Raw, colorspace: raw}\n"
        "\n";
    
    std::istringstream is;
    is.str(SIMPLE_PROFILE);
    OCIO::ConstConfigRcPtr config;
    OIIO_CHECK_THROW(config = OCIO::Config::CreateFromStream(is), OCIO::Exception);
}

namespace
{

const std::string PROFILE_V1 = "ocio_profile_version: 1\n";

const std::string PROFILE_V2 = "ocio_profile_version: 2\n";

const std::string SIMPLE_PROFILE =
    "\n"
    "search_path: luts\n"
    "strictparsing: true\n"
    "luma: [0.2126, 0.7152, 0.0722]\n"
    "\n"
    "roles:\n"
    "  default: raw\n"
    "  scene_linear: lnh\n"
    "\n"
    "displays:\n"
    "  sRGB:\n"
    "    - !<View> {name: Raw, colorspace: raw}\n"
    "\n"
    "active_displays: []\n"
    "active_views: []\n"
    "\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "    name: raw\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: lnh\n"
    "    family: \"\"\n"
    "    equalitygroup: \"\"\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n";

};

OIIO_ADD_TEST(Config, range_serialization)
{
    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {direction: inverse}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {style: noClamp}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {style: noClamp, direction: inverse}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Test Range with clamp style (i.e. default one)
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.0109, "
            "maxInValue: 1.0505, minOutValue: 0.0009, maxOutValue: 2.5001, "
            "direction: inverse}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Test Range with clamp style
        const std::string in_strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.0109, "
            "maxInValue: 1.0505, minOutValue: 0.0009, maxOutValue: 2.5001, "
            "style: Clamp, direction: inverse}\n";
        const std::string in_str = PROFILE_V2 + SIMPLE_PROFILE + in_strEnd;

        std::istringstream is;
        is.str(in_str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        // Clamp style is not saved
        const std::string out_strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.0109, "
            "maxInValue: 1.0505, minOutValue: 0.0009, maxOutValue: 2.5001, "
            "direction: inverse}\n";
        const std::string out_str = PROFILE_V2 + SIMPLE_PROFILE + out_strEnd;

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), out_str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> "
            "{minInValue: 0, maxOutValue: 1}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_THROW_WHAT(config->sanityCheck(), 
                              OCIO::Exception, 
                              "must be both set or both missing");

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // maxInValue has an illegal second number.
        const std::string strEndFail =
            "    from_reference: !<RangeTransform> {minInValue: -0.01, "
            "maxInValue: 1.05  10, minOutValue: 0.0009, maxOutValue: 2.5}\n";
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.01, "
            "maxInValue: 1.05, minOutValue: 0.0009, maxOutValue: 2.5}\n";
        
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEndFail;
        const std::string strSaved = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception, "parsing double failed");

        is.str(strSaved);
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        // Re-serialize and test that it matches the expected text.
        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), strSaved);
    }

    {
        // maxInValue & maxOutValue have no value, they will not be defined.
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.01, "
            "maxInValue: , minOutValue: 0.0009, maxOutValue: }\n";
        const std::string strEndSaved =
            "    from_reference: !<RangeTransform> {minInValue: -0.01, "
            "minOutValue: 0.0009}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;
        const std::string strSaved = PROFILE_V2 + SIMPLE_PROFILE + strEndSaved;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        // Re-serialize and test that it matches the expected text.
        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), strSaved);
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> "
            "{minInValue: 0.12345678901234, maxOutValue: 1.23456789012345}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_THROW_WHAT(config->sanityCheck(),
            OCIO::Exception,
            "must be both set or both missing");

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.01, "
            "maxInValue: 1.05, minOutValue: 0.0009, maxOutValue: 2.5}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        // Re-serialize and test that it matches the original text.
        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minOutValue: 0.0009, "
            "maxOutValue: 2.5}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_THROW_WHAT(config->sanityCheck(),
                              OCIO::Exception,
                              "must be both set or both missing");

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<RangeTransform> {minInValue: -0.01, maxInValue: 1.05, "
            "minOutValue: 0.0009, maxOutValue: 2.5}\n"
            "        - !<RangeTransform> {minOutValue: 0.0009, maxOutValue: 2.1}\n"
            "        - !<RangeTransform> {minOutValue: 0.1, maxOutValue: 0.9}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_THROW_WHAT(config->sanityCheck(),
                              OCIO::Exception,
                              "must be both set or both missing");

        // Re-serialize and test that it matches the original text.
        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    // Some faulty cases

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            // missing { (and mInValue is wrong -> that's a warning)
            "        - !<RangeTransform> mInValue: -0.01, maxInValue: 1.05, "
            "minOutValue: 0.0009, maxOutValue: 2.5}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OIIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "Loading the OCIO profile failed");
    }

    {
        const std::string strEnd =
            // The comma is missing after the minInValue value.
            "    from_reference: !<RangeTransform> {minInValue: -0.01 "
            "maxInValue: 1.05, minOutValue: 0.0009, maxOutValue: 2.5}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OIIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "Loading the OCIO profile failed");
    }

    {
        const std::string strEnd =
            "    from_reference: !<RangeTransform> {minInValue: -0.01, "
            // The comma is missing between the minOutValue value and
            // the maxOutValue tag.
            "maxInValue: 1.05, minOutValue: 0.0009maxOutValue: 2.5}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OIIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "Loading the OCIO profile failed");
    }
}

OIIO_ADD_TEST(Config, exponent_serialization)   
{   
    {   
        const std::string strEnd =  
            "    from_reference: !<ExponentTransform> " 
            "{value: [1.101, 1.202, 1.303, 1.404]}\n";  
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

        std::istringstream is; 
        is.str(str);    
        OCIO::ConstConfigRcPtr config;  
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));   
        OIIO_CHECK_NO_THROW(config->sanityCheck()); 

        std::stringstream ss;  
        ss << *config.get();    
        OIIO_CHECK_EQUAL(ss.str(), str);    
    }   

     {  
        const std::string strEnd =  
            "    from_reference: !<ExponentTransform> " 
            "{value: [1.101, 1.202, 1.303, 1.404], direction: inverse}\n";  
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

        std::istringstream is; 
        is.str(str);    
        OCIO::ConstConfigRcPtr config;  
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));   
        OIIO_CHECK_NO_THROW(config->sanityCheck()); 

        std::stringstream ss;  
        ss << *config.get();    
        OIIO_CHECK_EQUAL(ss.str(), str);    
    }   

    // Errors

    {
        // Some gamma values are missing.
        const std::string strEnd =
            "    from_reference: !<ExponentTransform> "
            "{value: [1.1, 1.2, 1.3]}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "'value' values must be 4 floats. Found '3'");
    }
}

OIIO_ADD_TEST(Config, exponent_with_linear_serialization)
{
    {
        const std::string strEnd =
            "    from_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4], offset: [0.101, 0.102, 0.103, 0.1]}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4], offset: [0.101, 0.102, 0.103, 0.1], direction: inverse}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    // Errors

    {
        const std::string strEnd =
            "    from_reference: !<ExponentWithLinearTransform> {}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "ExponentWithLinear parse error, gamma and offset fields are missing");
    }

    {
        // Offset values are missing.
        const std::string strEnd =
            "    from_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4]}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "ExponentWithLinear parse error, offset field is missing");
    }

    {
        // Gamma values are missing.
        const std::string strEnd =
            "    from_reference: !<ExponentWithLinearTransform> "
            "{offset: [1.1, 1.2, 1.3, 1.4]}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "ExponentWithLinear parse error, gamma field is missing");
    }

    {
        // Some gamma values are missing.
        const std::string strEnd =
            "    from_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3]}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "ExponentWithLinear parse error, gamma field must be 4 floats");
    }
    {
        // Some offset values are missing.
        const std::string strEnd =
            "    from_reference: !<ExponentWithLinearTransform> "
            "{gamma: [1.1, 1.2, 1.3, 1.4], offset: [0.101, 0.102]}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "ExponentWithLinear parse error, offset field must be 4 floats");
    }
}

OIIO_ADD_TEST(Config, exponent_vs_config_version)
{
    // The config i.e. SIMPLE_PROFILE is a version 2.

    std::istringstream is;
    OCIO::ConstConfigRcPtr config;
    OCIO::ConstProcessorRcPtr processor;

    // OCIO config file version == 1  and exponent == 1

    const std::string strEnd =
        "    from_reference: !<ExponentTransform> {value: [1, 1, 1, 1]}\n";
    const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

    is.str(str);
    OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OIIO_CHECK_NO_THROW(config->sanityCheck());

    OIIO_CHECK_NO_THROW(processor = config->getProcessor("raw", "lnh"));

    float img1[4] = { -0.5f, 0.0f, 1.0f, 1.0f };
    OIIO_CHECK_NO_THROW(processor->applyRGBA(img1));

    OIIO_CHECK_EQUAL(img1[0], -0.5f);
    OIIO_CHECK_EQUAL(img1[1],  0.0f);
    OIIO_CHECK_EQUAL(img1[2],  1.0f);
    OIIO_CHECK_EQUAL(img1[3],  1.0f);

    // OCIO config file version == 1  and exponent != 1

    const std::string strEnd2 =
        "    from_reference: !<ExponentTransform> {value: [2, 2, 2, 1]}\n";
    const std::string str2 = PROFILE_V1 + SIMPLE_PROFILE + strEnd2;

    is.str(str2);
    OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OIIO_CHECK_NO_THROW(config->sanityCheck());
    OIIO_CHECK_NO_THROW(processor = config->getProcessor("raw", "lnh"));

    float img2[4] = { -0.5f, 0.0f, 1.0f, 1.0f };
    OIIO_CHECK_NO_THROW(processor->applyRGBA(img2));

    OIIO_CHECK_EQUAL(img2[0],  0.0f);
    OIIO_CHECK_EQUAL(img2[1],  0.0f);
    OIIO_CHECK_EQUAL(img2[2],  1.0f);
    OIIO_CHECK_EQUAL(img2[3],  1.0f);

    // OCIO config file version > 1  and exponent == 1

    std::string str3 = PROFILE_V2 + SIMPLE_PROFILE + strEnd;
    is.str(str3);
    OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OIIO_CHECK_NO_THROW(config->sanityCheck());
    OIIO_CHECK_NO_THROW(processor = config->getProcessor("raw", "lnh"));

    float img3[4] = { -0.5f, 0.0f, 1.0f, 1.0f };
    OIIO_CHECK_NO_THROW(processor->applyRGBA(img3));

    OIIO_CHECK_EQUAL(img3[0], 0.0f);
    OIIO_CHECK_EQUAL(img3[1], 0.0f);
    OIIO_CHECK_CLOSE(img3[2], 1.0f, 2e-5f); // Because of SSE optimizations.
    OIIO_CHECK_CLOSE(img3[3], 1.0f, 2e-5f); // Because of SSE optimizations.

    // OCIO config file version > 1  and exponent != 1

    std::string str4 = PROFILE_V2 + SIMPLE_PROFILE + strEnd2;
    is.str(str4);
    OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OIIO_CHECK_NO_THROW(config->sanityCheck());
    OIIO_CHECK_NO_THROW(processor = config->getProcessor("raw", "lnh"));

    float img4[4] = { -0.5f, 0.0f, 1.0f, 1.0f };
    OIIO_CHECK_NO_THROW(processor->applyRGBA(img4));

    OIIO_CHECK_EQUAL(img4[0], 0.0f);
    OIIO_CHECK_EQUAL(img4[1], 0.0f);
    OIIO_CHECK_CLOSE(img4[2], 1.0f, 3e-5f); // Because of SSE optimizations.
    OIIO_CHECK_CLOSE(img4[3], 1.0f, 2e-5f); // Because of SSE optimizations.
}

OIIO_ADD_TEST(Config, categories)
{
    static const std::string MY_OCIO_CONFIG =
        "ocio_profile_version: 1\n"
        "\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: raw1\n"
        "  scene_linear: raw1\n"
        "\n"
        "displays:\n"
        "  sRGB:\n"
        "    - !<View> {name: Raw, colorspace: raw1}\n"
        "\n"
        "active_displays: []\n"
        "active_views: []\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw1\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    categories: [rendering, linear]\n"
        "    allocation: uniform\n"
        "    allocationvars: [-0.125, 1.125]\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: raw2\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    categories: [rendering]\n"
        "    allocation: uniform\n"
        "    allocationvars: [-0.125, 1.125]\n";

    std::istringstream is;
    is.str(MY_OCIO_CONFIG);

    OCIO::ConstConfigRcPtr config;
    OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OIIO_CHECK_NO_THROW(config->sanityCheck());

    // Test the serialization & deserialization.

    std::stringstream ss;
    ss << *config.get();
    OIIO_CHECK_EQUAL(ss.str(), MY_OCIO_CONFIG);

    // Test the config content.

    OCIO::ColorSpaceSetRcPtr css = config->getColorSpaces(nullptr);
    OIIO_CHECK_EQUAL(css->getNumColorSpaces(), 2);
    OCIO::ConstColorSpaceRcPtr cs = css->getColorSpaceByIndex(0);
    OIIO_CHECK_EQUAL(cs->getNumCategories(), 2);
    OIIO_CHECK_EQUAL(std::string(cs->getCategory(0)), std::string("rendering"));
    OIIO_CHECK_EQUAL(std::string(cs->getCategory(1)), std::string("linear"));

    css = config->getColorSpaces("linear");
    OIIO_CHECK_EQUAL(css->getNumColorSpaces(), 1);
    cs = css->getColorSpaceByIndex(0);
    OIIO_CHECK_EQUAL(cs->getNumCategories(), 2);
    OIIO_CHECK_EQUAL(std::string(cs->getCategory(0)), std::string("rendering"));
    OIIO_CHECK_EQUAL(std::string(cs->getCategory(1)), std::string("linear"));

    css = config->getColorSpaces("rendering");
    OIIO_CHECK_EQUAL(css->getNumColorSpaces(), 2);

    OIIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);
    OIIO_CHECK_EQUAL(std::string(config->getColorSpaceNameByIndex(0)), std::string("raw1"));
    OIIO_CHECK_EQUAL(std::string(config->getColorSpaceNameByIndex(1)), std::string("raw2"));
    OIIO_CHECK_EQUAL(config->getIndexForColorSpace("raw1"), 0);
    OIIO_CHECK_EQUAL(config->getIndexForColorSpace("raw2"), 1);
    cs = config->getColorSpace("raw1");
    OIIO_CHECK_EQUAL(std::string(cs->getName()), std::string("raw1"));
    cs = config->getColorSpace("raw2");
    OIIO_CHECK_EQUAL(std::string(cs->getName()), std::string("raw2"));
}

OIIO_ADD_TEST(Config, display)
{
    static const std::string SIMPLE_PROFILE_HEADER =
        "ocio_profile_version: 1\n"
        "\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: raw\n"
        "  scene_linear: lnh\n"
        "\n"
        "displays:\n"
        "  sRGB_1:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "  sRGB_2:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "  sRGB_3:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "\n";

    static const std::string SIMPLE_PROFILE_FOOTER =
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: lnh\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n";

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_EQUAL(config->getNumDisplays(), 3);
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_1"));
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("sRGB_2"));
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(2)), std::string("sRGB_3"));
        OIIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_1");
    }

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_1]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_EQUAL(config->getNumDisplays(), 1);
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_1"));
        OIIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_1");
    }

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_2, sRGB_1]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_EQUAL(config->getNumDisplays(), 2);
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_2"));
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("sRGB_1"));
        OIIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_2");
    }

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        const std::string active_displays(
            std::string(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR) + "= sRGB_3, sRGB_2");
        putenv(const_cast<char *>(active_displays.c_str()));

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_EQUAL(config->getNumDisplays(), 2);
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_3"));
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("sRGB_2"));
        OIIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_3");
    }

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_2, sRGB_1]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        const std::string active_displays(
            std::string(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR) + "= sRGB_3, sRGB_2");
        putenv(const_cast<char *>(active_displays.c_str()));

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_EQUAL(config->getNumDisplays(), 2);
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_3"));
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("sRGB_2"));
        OIIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_3");
    }

    {
        const std::string active_displays(
            std::string(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR) + "="); // No value
        putenv(const_cast<char *>(active_displays.c_str()));

        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_2, sRGB_1]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_EQUAL(config->getNumDisplays(), 2);
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_2"));
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("sRGB_1"));
        OIIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_2");
    }

    {
        const std::string active_displays(
            std::string(OCIO::OCIO_ACTIVE_DISPLAYS_ENVVAR) + "= "); // No value, but misleading space
        putenv(const_cast<char *>(active_displays.c_str()));

        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: [sRGB_2, sRGB_1]\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_EQUAL(config->getNumDisplays(), 2);
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(0)), std::string("sRGB_2"));
        OIIO_CHECK_EQUAL(std::string(config->getDisplay(1)), std::string("sRGB_1"));
        OIIO_CHECK_EQUAL(std::string(config->getDefaultDisplay()), "sRGB_2");
    }
}

OIIO_ADD_TEST(Config, view)
{
    static const std::string SIMPLE_PROFILE_HEADER =
        "ocio_profile_version: 1\n"
        "\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: raw\n"
        "  scene_linear: lnh\n"
        "\n"
        "displays:\n"
        "  sRGB_1:\n"
        "    - !<View> {name: View_1, colorspace: raw}\n"
        "    - !<View> {name: View_2, colorspace: raw}\n"
        "  sRGB_2:\n"
        "    - !<View> {name: View_2, colorspace: raw}\n"
        "    - !<View> {name: View_3, colorspace: raw}\n"
        "  sRGB_3:\n"
        "    - !<View> {name: View_3, colorspace: raw}\n"
        "    - !<View> {name: View_1, colorspace: raw}\n"
        "\n";

    static const std::string SIMPLE_PROFILE_FOOTER =
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: lnh\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n";

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_1")), "View_1");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 0)), "View_1");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 1)), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_2")), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 0)), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 1)), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_3")), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 0)), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 1)), "View_1");
    }

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: [View_3]\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_1")), "View_1");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 0)), "View_1");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 1)), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_2")), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 0)), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 1)), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_3")), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 0)), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 1)), "View_1");
    }

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: [View_3, View_2, View_1]\n"
            + SIMPLE_PROFILE_FOOTER;

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_1")), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 0)), "View_1");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 1)), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_2")), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 0)), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 1)), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_3")), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 0)), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 1)), "View_1");
    }

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        const std::string active_displays(
            std::string(OCIO::OCIO_ACTIVE_VIEWS_ENVVAR) + "= View_3, View_2");
        putenv(const_cast<char *>(active_displays.c_str()));

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_1")), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 0)), "View_1");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 1)), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_2")), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 0)), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 1)), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_3")), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 0)), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 1)), "View_1");
    }

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        const std::string active_displays(
            std::string(OCIO::OCIO_ACTIVE_VIEWS_ENVVAR) + "="); // No value.
        putenv(const_cast<char *>(active_displays.c_str()));

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_1")), "View_1");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 0)), "View_1");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 1)), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_2")), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 0)), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 1)), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_3")), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 0)), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 1)), "View_1");
    }

    {
        std::string myProfile = 
            SIMPLE_PROFILE_HEADER
            +
            "active_displays: []\n"
            "active_views: []\n"
            + SIMPLE_PROFILE_FOOTER;

        const std::string active_displays(
            std::string(OCIO::OCIO_ACTIVE_VIEWS_ENVVAR) + "= "); // No value, but misleading space
        putenv(const_cast<char *>(active_displays.c_str()));

        std::istringstream is(myProfile);
        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_1")), "View_1");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 0)), "View_1");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_1", 1)), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_2")), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 0)), "View_2");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_2", 1)), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getDefaultView("sRGB_3")), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 0)), "View_3");
        OIIO_CHECK_EQUAL(std::string(config->getView("sRGB_3", 1)), "View_1");
    }
}

OIIO_ADD_TEST(Config, log_serialization)
{
    const std::string PROFILE_V1 = "ocio_profile_version: 1\n";
    const std::string PROFILE_V2 = "ocio_profile_version: 2\n";
    static const std::string SIMPLE_PROFILE =
        "\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: raw\n"
        "  scene_linear: lnh\n"
        "\n"
        "displays:\n"
        "  sRGB:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "\n"
        "active_displays: []\n"
        "active_views: []\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: lnh\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n";

    {
        // Log with default base value and default direction.
        const std::string strEnd =
            "    from_reference: !<LogTransform> {}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Log with default base value.
        const std::string strEnd =
            "    from_reference: !<LogTransform> {direction: inverse}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Log with specified base value.
        const std::string strEnd =
            "    from_reference: !<LogTransform> {base: 5}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // Log with specified base value and direction.
        const std::string strEnd =
            "    from_reference: !<LogTransform> {base: 7, direction: inverse}\n";
        const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with specified values 3 commponents.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "base: 10, "
            "logSideSlope: [1.3, 1.4, 1.5], "
            "logSideOffset: [0, 0, 0.1], "
            "linSideSlope: [1, 1, 1.1], "
            "linSideOffset: [0.1234567890123, 0.5, 0.1]}\n";
            const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with default value for base.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "logSideSlope: [1, 1, 1.1], "
            "logSideOffset: [0.1234567890123, 0.5, 0.1], "
            "linSideSlope: [1.3, 1.4, 1.5], "
            "linSideOffset: [0, 0, 0.1]}\n";
    
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with single value for linSideOffset.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "base: 10, "
            "logSideSlope: [1, 1, 1.1], "
            "logSideOffset: [0.1234567890123, 0.5, 0.1], "
            "linSideSlope: [1.3, 1.4, 1.5], "
            "linSideOffset: 0.5}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with single value for linSideSlope.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "logSideSlope: [1, 1, 1.1], "
            "linSideSlope: 1.3, "
            "linSideOffset: [0, 0, 0.1]}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with single value for logSideOffset.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "logSideSlope: [1, 1, 1.1], "
            "logSideOffset: 0.5, "
            "linSideSlope: [1.3, 1, 1], "
            "linSideOffset: [0, 0, 0.1]}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with single value for logSideSlope.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "logSideSlope: 1.1, "
            "logSideOffset: [0.5, 0, 0], "
            "linSideSlope: [1.3, 1, 1], "
            "linSideOffset: [0, 0, 0.1]}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with default value for logSideSlope.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "logSideOffset: [0.1234567890123, 0.5, 0.1], "
            "linSideSlope: [1.3, 1.4, 1.5], "
            "linSideOffset: [0.1, 0, 0]}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with default value for all but base.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {base: 10}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        // LogAffine with wrong size for logSideSlope.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "logSideSlope: [1, 1], "
            "logSideOffset: [0.1234567890123, 0.5, 0.1]}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "logSideSlope value field must have 3 components");
    }

    {
        // LogAffine with 3 values for base.
        const std::string strEnd =
            "    from_reference: !<LogAffineTransform> {"
            "base: [2, 2, 2], "
            "logSideOffset: [0.1234567890123, 0.5, 0.1]}\n";
        const std::string str = PROFILE_V2 + SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_THROW_WHAT(config = OCIO::Config::CreateFromStream(is),
            OCIO::Exception,
            "base must be a single double");
    }
}

OIIO_ADD_TEST(Config, key_value_error)
{
    // Check the line number contained in the parser error messages.

    const std::string SHORT_PROFILE =
        "ocio_profile_version: 2\n"
        "strictparsing: false\n"
        "roles:\n"
        "  default: raw\n"
        "displays:\n"
        "  sRGB:\n"
        "  - !<View> {name: Raw, colorspace: raw}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    to_reference: !<MatrixTransform> \n"
        "                      {\n"
        "                           matrix: [1, 0, 0, 0, 0, 1]\n" // Missing values.
        "                      }\n"
        "    allocation: uniform\n"
        "\n";
    
    std::istringstream is;
    is.str(SHORT_PROFILE);

    OIIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                          OCIO::Exception,
                          "Error: Loading the OCIO profile failed. At line 14, the value "
                          "parsing of the key 'matrix' from 'MatrixTransform' failed: "
                          "'matrix' values must be 16 numbers. Found '6'.");
}

namespace
{

// Redirect the std::cerr to catch the warning.
class Guard
{
public:      
    Guard()      
        :   m_oldBuf(std::cerr.rdbuf())      
    {        
        std::cerr.rdbuf(m_ss.rdbuf());       
    }        

    ~Guard()         
    {        
        std::cerr.rdbuf(m_oldBuf);       
        m_oldBuf = nullptr;      
    }        

    std::string output() { return m_ss.str(); }      

private:         
    std::stringstream m_ss;      
    std::streambuf *  m_oldBuf;      

    Guard(const Guard&) = delete;        
    Guard operator=(const Guard&) = delete;      
};

};

OIIO_ADD_TEST(Config, unknown_key_error)
{
    const std::string SHORT_PROFILE =
        "ocio_profile_version: 2\n"
        "strictparsing: false\n"
        "roles:\n"
        "  default: raw\n"
        "displays:\n"
        "  sRGB:\n"
        "  - !<View> {name: Raw, colorspace: raw}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    dummyKey: dummyValue\n"
        "    to_reference: !<MatrixTransform> {offset: [1, 0, 0, 0]}\n"
        "    allocation: uniform\n"
        "\n";
    
    std::istringstream is;
    is.str(SHORT_PROFILE);

    Guard g;
    OIIO_CHECK_NO_THROW(OCIO::Config::CreateFromStream(is));
    OIIO_CHECK_EQUAL(g.output(), 
                     "[OpenColorIO Warning]: At line 12, unknown key 'dummyKey' in 'ColorSpace'.\n");
}

OIIO_ADD_TEST(Config, fixed_function_serialization)
{
    static const std::string SIMPLE_PROFILE =
        "ocio_profile_version: 2\n"
        "\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: raw\n"
        "  scene_linear: raw\n"
        "\n"
        "displays:\n"
        "  sRGB:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "\n"
        "active_displays: []\n"
        "active_views: []\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n";

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<FixedFunctionTransform> {style: ACES_RedMod03}\n"
            "        - !<FixedFunctionTransform> {style: ACES_RedMod03, direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: ACES_RedMod10}\n"
            "        - !<FixedFunctionTransform> {style: ACES_RedMod10, direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: ACES_Glow03}\n"
            "        - !<FixedFunctionTransform> {style: ACES_Glow03, direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: ACES_Glow10}\n"
            "        - !<FixedFunctionTransform> {style: ACES_Glow10, direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: ACES_DarkToDim10}\n"
            "        - !<FixedFunctionTransform> {style: ACES_DarkToDim10, direction: inverse}\n"
            "        - !<FixedFunctionTransform> {style: REC2100_Surround, params: [0.75]}\n"
            "        - !<FixedFunctionTransform> {style: REC2100_Surround, params: [0.75], direction: inverse}\n";

        const std::string str = SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<FixedFunctionTransform> {style: ACES_DarkToDim10, params: [0.75]}\n";

        const std::string str = SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_THROW_WHAT(config->sanityCheck(), OCIO::Exception,
            "The style 'ACES_DarkToDim10 (Forward)' must have zero parameters but 1 found.");
    }

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<FixedFunctionTransform> {style: REC2100_Surround, direction: inverse}\n";

        const std::string str = SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_THROW_WHAT(config->sanityCheck(), OCIO::Exception, 
                              "The style 'REC2100_Surround' must "
                              "have one parameter but 0 found.");
    }
}

OIIO_ADD_TEST(Config, exposure_contrast_serialization)
{
    static const std::string SIMPLE_PROFILE =
        "ocio_profile_version: 2\n"
        "\n"
        "search_path: luts\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: raw\n"
        "  scene_linear: raw\n"
        "\n"
        "displays:\n"
        "  sRGB:\n"
        "    - !<View> {name: Raw, colorspace: raw}\n"
        "\n"
        "active_displays: []\n"
        "active_views: []\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n";

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<ExposureContrastTransform> {style: video, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: video,"
                       " exposure: {value: 1.5, dynamic: true}, contrast: 0.5,"
                       " gamma: 1.1, pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: video, exposure: -1.4,"
                       " contrast: 0.6, gamma: 1.2, pivot: 0.2,"
                       " direction: inverse}\n"
            "        - !<ExposureContrastTransform> {style: log, exposure: 1.5,"
                       " contrast: 0.6, gamma: 1.2, pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: log, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18,"
                       " direction: inverse}\n"
            "        - !<ExposureContrastTransform> {style: log, exposure: 1.5,"
                       " contrast: {value: 0.6, dynamic: true}, gamma: 1.2,"
                       " pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: linear, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18}\n"
            "        - !<ExposureContrastTransform> {style: linear, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18,"
                       " direction: inverse}\n"
            "        - !<ExposureContrastTransform> {style: linear, exposure: 1.5,"
                       " contrast: 0.5, gamma: {value: 1.1, dynamic: true},"
                       " pivot: 0.18}\n";

        const std::string str = SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), str);
    }

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n";
        
        const std::string strEndEC =
            "        - !<ExposureContrastTransform> {style: video,"
                       " exposure: {value: 1.5},"
                       " contrast: {value: 0.5, dynamic: false},"
                       " gamma: {value: 1.1}, pivot: 0.18}\n";

        const std::string strEndECExpected =
            "        - !<ExposureContrastTransform> {style: video, exposure: 1.5,"
                       " contrast: 0.5, gamma: 1.1, pivot: 0.18}\n";

        const std::string str = SIMPLE_PROFILE + strEnd + strEndEC;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        const std::string strExpected = SIMPLE_PROFILE + strEnd + strEndECExpected;

        std::stringstream ss;
        ss << *config.get();
        OIIO_CHECK_EQUAL(ss.str(), strExpected);

    }

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<ExposureContrastTransform> {style: wrong}\n";

        const std::string str = SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OIIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is),
                              OCIO::Exception,
                              "Unknown exposure contrast style");
    }
}

OIIO_ADD_TEST(Config, matrix_serialization)
{
    const std::string strEnd =
        "    from_reference: !<GroupTransform>\n"
        "      children:\n"
                 // Check the value serialization.
        "        - !<MatrixTransform> {matrix: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],"\
                                     " offset: [-1, -2, -3, -4]}\n"
                 // Check the value precision.
        "        - !<MatrixTransform> {offset: [0.123456789876, 1.23456789876, 12.3456789876, 123.456789876]}\n"
        "        - !<MatrixTransform> {matrix: [0.123456789876, 1.23456789876, 12.3456789876, 123.456789876, "\
                                                "1234.56789876, 12345.6789876, 123456.789876, 1234567.89876, "\
                                                "0, 0, 1, 0, 0, 0, 0, 1]}\n";

    const std::string str = PROFILE_V1 + SIMPLE_PROFILE + strEnd;

    std::istringstream is;
    is.str(str);

    OCIO::ConstConfigRcPtr config;
    OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OIIO_CHECK_NO_THROW(config->sanityCheck());

    std::stringstream ss;
    ss << *config.get();
    OIIO_CHECK_EQUAL(ss.str(), str);
}

#endif // OCIO_UNIT_TEST

