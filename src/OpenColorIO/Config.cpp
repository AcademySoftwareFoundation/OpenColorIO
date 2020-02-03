// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


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
#include "PrivateTypes.h"
#include "Processor.h"
#include "pystring/pystring.h"
#include "OCIOYaml.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

namespace
{

constexpr char OCIO_CONFIG_ENVVAR[]               = "OCIO";
constexpr char OCIO_ACTIVE_DISPLAYS_ENVVAR[]      = "OCIO_ACTIVE_DISPLAYS";
constexpr char OCIO_ACTIVE_VIEWS_ENVVAR[]         = "OCIO_ACTIVE_VIEWS";
constexpr char OCIO_INACTIVE_COLORSPACES_ENVVAR[] = "OCIO_INACTIVE_COLORSPACES";


// These are the 709 primaries specified by the ASC.
constexpr double DEFAULT_LUMA_COEFF_R = 0.2126;
constexpr double DEFAULT_LUMA_COEFF_G = 0.7152;
constexpr double DEFAULT_LUMA_COEFF_B = 0.0722;


constexpr char INTERNAL_RAW_PROFILE[] = 
    "ocio_profile_version: 2\n"
    "strictparsing: false\n"
    "roles:\n"
    "  default: raw\n"
    "file_rules:\n"
    "  - !<Rule> {name: ColorSpaceNamePathSearch}\n"
    "  - !<Rule> {name: Default, colorspace: default}\n"
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

} // anon.


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
const char* LookupEnvironment(const StringMap & env, const std::string & name)
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

void GetFileReferences(std::set<std::string> & files, const ConstTransformRcPtr & transform)
{
    if(!transform) return;

    if(ConstGroupTransformRcPtr groupTransform = \
        DynamicPtrCast<const GroupTransform>(transform))
    {
        for(int i=0; i<groupTransform->getNumTransforms(); ++i)
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
        for(int i=0; i<groupTransform->getNumTransforms(); ++i)
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

static constexpr char AddedRaw[]{ "added_default_rule_colorspace" };

void FindAvailableName(const ColorSpaceSetRcPtr & colorspaces,
                       std::string & csname)
{
    int i = 0;
    csname = AddedRaw;
    while (1)
    {
        if (-1 == colorspaces->getIndexForColorSpace(csname.c_str()))
        {
            break;
        }
        csname = "added_default_rule_colorspace" + std::to_string(i);
    }
}

} // namespace


static constexpr unsigned FirstSupportedMajorVersion = 1;
static constexpr unsigned LastSupportedMajorVersion = OCIO_VERSION_MAJOR;
static constexpr unsigned LastSupportedMinorVersion = OCIO_VERSION_MINOR;

class Config::Impl
{
public:
    enum Sanity
    {
        SANITY_UNKNOWN = 0,
        SANITY_SANE,
        SANITY_INSANE
    };

    unsigned int m_majorVersion;
    unsigned int m_minorVersion;
    StringMap m_env;
    ContextRcPtr m_context;
    std::string m_description;

    // The final list of inactive color spaces is built from several inputs.
    // An API request (using Config::setInactiveColorSpaces()) always supersedes
    // the list from the env. variable (i.e. OCIO_INACTIVE_COLORSPACES_ENVVAR) which
    // supersedes the list from the config. file (i.e. inactive_colorspaces).
    //
    // Refer to Config::Impl::refreshActiveColorSpaces() to have the implementation details.

    ColorSpaceSetRcPtr m_allColorSpaces; // All the color spaces (i.e. no filtering).
    StringVec m_activeColorSpaceNames;   // A built list of active color space names.

    std::string m_inactiveColorSpaceNamesAPI;  // Inactive color space filter from API request. 
    std::string m_inactiveColorSpaceNamesEnv;  // Inactive color space filter from env. variable.
    std::string m_inactiveColorSpaceNamesConf; // Inactive color space filter from config. file. 

    StringMap m_roles;
    LookVec m_looksList;

    DisplayMap m_displays;
    StringVec m_activeDisplays;
    StringVec m_activeDisplaysEnvOverride;
    StringVec m_activeViews;
    StringVec m_activeViewsEnvOverride;

    mutable std::string m_activeDisplaysStr;
    mutable std::string m_activeViewsStr;
    mutable StringVec m_displayCache;

    // Misc
    std::vector<double> m_defaultLumaCoefs;
    bool m_strictParsing;

    mutable Sanity m_sanity;
    mutable std::string m_sanitytext;

    mutable Mutex m_cacheidMutex;
    mutable StringMap m_cacheids;
    mutable std::string m_cacheidnocontext;
    FileRulesRcPtr m_fileRules;

    Impl() :
        m_majorVersion(FirstSupportedMajorVersion),
        m_minorVersion(0),
        m_context(Context::Create()),
        m_allColorSpaces(ColorSpaceSet::Create()),
        m_strictParsing(true),
        m_sanity(SANITY_UNKNOWN),
        m_fileRules(FileRules::Create())
    {
        std::string activeDisplays;
        Platform::Getenv(OCIO_ACTIVE_DISPLAYS_ENVVAR, activeDisplays);
        activeDisplays = pystring::strip(activeDisplays);
        if (!activeDisplays.empty())
        {
            SplitStringEnvStyle(m_activeDisplaysEnvOverride, activeDisplays.c_str());
        }

        std::string activeViews;
        Platform::Getenv(OCIO_ACTIVE_VIEWS_ENVVAR, activeViews);
        activeViews = pystring::strip(activeViews);
        if (!activeViews.empty())
        {
            SplitStringEnvStyle(m_activeViewsEnvOverride, activeViews.c_str());
        }

        m_defaultLumaCoefs.resize(3);
        m_defaultLumaCoefs[0] = DEFAULT_LUMA_COEFF_R;
        m_defaultLumaCoefs[1] = DEFAULT_LUMA_COEFF_G;
        m_defaultLumaCoefs[2] = DEFAULT_LUMA_COEFF_B;

        Platform::Getenv(OCIO_INACTIVE_COLORSPACES_ENVVAR, m_inactiveColorSpaceNamesEnv);
        m_inactiveColorSpaceNamesEnv = pystring::strip(m_inactiveColorSpaceNamesEnv);
    }

    ~Impl() = default;
    Impl(const Impl&) = delete;

    Impl& operator= (const Impl & rhs)
    {
        if(this!=&rhs)
        {
            m_majorVersion = rhs.m_majorVersion;
            m_minorVersion = rhs.m_minorVersion;

            m_env = rhs.m_env;
            m_context = rhs.m_context->createEditableCopy();
            m_description = rhs.m_description;

            m_allColorSpaces = rhs.m_allColorSpaces; // Deep copy the colorspaces
            m_activeColorSpaceNames       = rhs.m_activeColorSpaceNames;
            m_inactiveColorSpaceNamesConf = rhs.m_inactiveColorSpaceNamesConf;
            m_inactiveColorSpaceNamesEnv  = rhs.m_inactiveColorSpaceNamesEnv;
            m_inactiveColorSpaceNamesAPI  = rhs.m_inactiveColorSpaceNamesAPI;

            // Deep copy the looks
            m_looksList.clear();
            m_looksList.reserve(rhs.m_looksList.size());
            for(unsigned int i=0; i<rhs.m_looksList.size(); ++i)
            {
                m_looksList.push_back(
                    rhs.m_looksList[i]->createEditableCopy());
            }

            // Assignment operator will suffice for these
            m_roles = rhs.m_roles;

            m_displays = rhs.m_displays;
            m_activeDisplays = rhs.m_activeDisplays;
            m_activeViews = rhs.m_activeViews;
            m_activeViewsEnvOverride = rhs.m_activeViewsEnvOverride;
            m_activeDisplaysEnvOverride = rhs.m_activeDisplaysEnvOverride;
            m_activeDisplaysStr = rhs.m_activeDisplaysStr;
            m_displayCache = rhs.m_displayCache;

            m_defaultLumaCoefs = rhs.m_defaultLumaCoefs;
            m_strictParsing = rhs.m_strictParsing;

            m_sanity = rhs.m_sanity;
            m_sanitytext = rhs.m_sanitytext;

            m_cacheids = rhs.m_cacheids;
            m_cacheidnocontext = rhs.m_cacheidnocontext;

            m_fileRules = rhs.m_fileRules->createEditableCopy();
        }
        return *this;
    }

    bool findColorSpaceIndex(int * index, const std::string & csname) const
    {
        *index = m_allColorSpaces->getIndexForColorSpace(csname.c_str());
        return *index!=-1;
    }

    std::vector<std::string> buildInactiveColorSpaceList() const;
    void refreshActiveColorSpaces();

    // Any time you modify the state of the config, you must call this
    // to reset internal cache states.  You also should do this in a
    // thread safe manner by acquiring the m_cacheidMutex.
    void resetCacheIDs();

    // Get all internal transforms (to generate cacheIDs, validation, etc).
    // This currently crawls colorspaces + looks
    void getAllInternalTransforms(ConstTransformVec & transformVec) const;

    static ConstConfigRcPtr Read(std::istream & istream, const char * filename);

    // Upgrade from v1 to v2.
    void upgradeFromVersion1ToVersion2()
    {
        // V2 adds file_rules and these require a default rule. We try to initialize the default
        // rule using the default role. If the default role doesn't exist, we look for a Raw
        // ColorSpace with isdata true. If that is not found either, we add new ColorSpace
        // named "added_default_rule_colorspace".

        m_majorVersion = 2;
        m_minorVersion = 0;

        int csindex = -1;
        const char * rname = LookupRole(m_roles, ROLE_DEFAULT);
        if (!FindColorSpaceIndex(&csindex, m_allColorSpaces, rname))
        {
            std::string defaultCS{ "" };
            bool addNewDefault = true;
            // The default role doesn't exist so look for a color space named "raw"
            // (not case-sensitive) with isdata true.
            if (FindColorSpaceIndex(&csindex, m_allColorSpaces, "raw"))
            {
                auto cs = m_allColorSpaces->getColorSpaceByIndex(csindex);
                if (cs->isData())
                {
                    // "Raw" color space can be used for default.
                    addNewDefault = false;
                    defaultCS = cs->getName();
                }
            }

            if (addNewDefault)
            {
                FindAvailableName(m_allColorSpaces, defaultCS);
                auto newCS = ColorSpace::Create();
                newCS->setName(defaultCS.c_str());
                newCS->setIsData(true);
                m_allColorSpaces->addColorSpace(newCS);
                // Put the added CS in the inactive list to avoid it showing up in user menus.
                if (!m_inactiveColorSpaceNamesConf.empty())
                {
                    m_inactiveColorSpaceNamesConf += ",";
                }
                m_inactiveColorSpaceNamesConf += defaultCS;
                setInactiveColorSpaces(m_inactiveColorSpaceNamesConf.c_str());
            }

            m_fileRules->setColorSpace(m_fileRules->getNumEntries() - 1, defaultCS.c_str());
        }
    }

    void setInactiveColorSpaces(const char * inactiveColorSpaces)
    {
        m_inactiveColorSpaceNamesConf
            = pystring::strip(std::string(inactiveColorSpaces ? inactiveColorSpaces : ""));

        // An API request must always supersede the two other lists. Filling the
        // inactiveColorSpaceNamesAPI_ list highlights the API request precedence.
        m_inactiveColorSpaceNamesAPI = m_inactiveColorSpaceNamesConf;

        AutoMutex lock(m_cacheidMutex);
        resetCacheIDs();
        refreshActiveColorSpaces();
    }

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

ConstConfigRcPtr Config::CreateRaw()
{
    std::istringstream istream;
    istream.str(INTERNAL_RAW_PROFILE);

    return CreateFromStream(istream);
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

    return CreateRaw();
}

ConstConfigRcPtr Config::CreateFromFile(const char * filename)
{
    std::ifstream istream(filename);
    if (istream.fail())
    {
        std::ostringstream os;
        os << "Error could not read '" << filename;
        os << "' OCIO profile.";
        throw Exception (os.str().c_str());
    }

    return Config::Impl::Read(istream, filename);
}

ConstConfigRcPtr Config::CreateFromStream(std::istream & istream)
{
    return Config::Impl::Read(istream, nullptr);
}

///////////////////////////////////////////////////////////////////////////

Config::Config()
: m_impl(new Config::Impl())
{
}

Config::~Config()
{
    delete m_impl;
    m_impl = NULL;
}

unsigned Config::getMajorVersion() const
{
    return m_impl->m_majorVersion;
}

void Config::setMajorVersion(unsigned int version)
{
    if (version <  FirstSupportedMajorVersion
        || version >  LastSupportedMajorVersion)
    {
        std::ostringstream os;
            os << "The version is " << version
            << " where supported versions start at "
            << FirstSupportedMajorVersion
            << " and end at "
            << LastSupportedMajorVersion
            << ".";
            throw Exception(os.str().c_str());
    }
    m_impl->m_majorVersion = version;

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

unsigned Config::getMinorVersion() const
{
    return m_impl->m_minorVersion;
}

void Config::setMinorVersion(unsigned int version)
{
     m_impl->m_minorVersion = version;
}

void Config::upgradeToLatestVersion()
{
    const auto wasVersion = m_impl->m_majorVersion;
    if (wasVersion != LastSupportedMajorVersion)
    {
        if (wasVersion == 1)
        {
            m_impl->upgradeFromVersion1ToVersion2();
        }
        static_assert(LastSupportedMajorVersion == 2, "Config: Handle newer versions");
        setMajorVersion(LastSupportedMajorVersion);
        setMinorVersion(LastSupportedMinorVersion);
    }
}


ConfigRcPtr Config::createEditableCopy() const
{
    ConfigRcPtr config = Config::Create();
    *config->m_impl = *m_impl;
    return config;
}

void Config::sanityCheck() const
{
    if(getImpl()->m_sanity == Impl::SANITY_SANE) return;
    if(getImpl()->m_sanity == Impl::SANITY_INSANE)
    {
        throw Exception(getImpl()->m_sanitytext.c_str());
    }

    getImpl()->m_sanity = Impl::SANITY_INSANE;
    getImpl()->m_sanitytext = "";

    ///// COLORSPACES
    StringSet existingColorSpaces;

    // Confirm all ColorSpaces are valid
    for(int i=0; i<getImpl()->m_allColorSpaces->getNumColorSpaces(); ++i)
    {
        if(!getImpl()->m_allColorSpaces->getColorSpaceByIndex(i))
        {
            std::ostringstream os;
            os << "Config failed sanitycheck. ";
            os << "The color space at index " << i << " is null.";
            getImpl()->m_sanitytext = os.str();
            throw Exception(getImpl()->m_sanitytext.c_str());
        }

        const char * name = getImpl()->m_allColorSpaces->getColorSpaceByIndex(i)->getName();
        if(!name || strlen(name) == 0)
        {
            std::ostringstream os;
            os << "Config failed sanitycheck. ";
            os << "The color space at index " << i << " is not named.";
            getImpl()->m_sanitytext = os.str();
            throw Exception(getImpl()->m_sanitytext.c_str());
        }

        std::string namelower = pystring::lower(name);
        StringSet::const_iterator it = existingColorSpaces.find(namelower);
        if(it != existingColorSpaces.end())
        {
            std::ostringstream os;
            os << "Config failed sanitycheck. ";
            os << "Two colorspaces are defined with the same name, '";
            os << namelower << "'.";
            getImpl()->m_sanitytext = os.str();
            throw Exception(getImpl()->m_sanitytext.c_str());
        }

        const auto cs = getImpl()->m_allColorSpaces->getColorSpaceByIndex(i);
        ConstTransformRcPtr toTrans = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
        if (toTrans)
        {
            toTrans->validate();
        }

        ConstTransformRcPtr fromTrans = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if (fromTrans)
        {
            fromTrans->validate();
        }

        existingColorSpaces.insert(namelower);
    }

    // Confirm all roles are valid
    {
        for(StringMap::const_iterator iter = getImpl()->m_roles.begin(),
            end = getImpl()->m_roles.end(); iter!=end; ++iter)
        {
            int csindex = -1;
            if(!getImpl()->findColorSpaceIndex(&csindex, iter->second))
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The role '" << iter->first << "' ";
                os << "refers to a color space, '" << iter->second << "', ";
                os << "which is not defined.";
                getImpl()->m_sanitytext = os.str();
                throw Exception(getImpl()->m_sanitytext.c_str());
            }

            // Confirm no name conflicts between colorspaces and roles
            if(getImpl()->findColorSpaceIndex(&csindex, iter->first))
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The role '" << iter->first << "' ";
                os << " is in conflict with a color space of the same name.";
                getImpl()->m_sanitytext = os.str();
                throw Exception(getImpl()->m_sanitytext.c_str());
            }
        }
    }

    // Confirm all inactive color spaces exist.
    const std::vector<std::string> inactiveColorSpaceNames
        = getImpl()->buildInactiveColorSpaceList();

    for (const auto & name : inactiveColorSpaceNames)
    {
        ConstColorSpaceRcPtr cs = getImpl()->m_allColorSpaces->getColorSpace(name.c_str());
        if (!cs)
        {
            std::stringstream ss;
            ss << "Inactive color space '" << name << "' does not exist.";
            LogWarning(ss.str());
        }
    }


    ///// DISPLAYS / VIEWS

    int numviews = 0;

    // Confirm all Display transforms refer to colorspaces that exit.
    for(DisplayMap::const_iterator iter = getImpl()->m_displays.begin();
        iter != getImpl()->m_displays.end();
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
            getImpl()->m_sanitytext = os.str();
            throw Exception(getImpl()->m_sanitytext.c_str());
        }

        // Confirm view references exist.
        for(unsigned int i=0; i<views.size(); ++i)
        {
            if(views[i].name.empty() || views[i].colorspace.empty())
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The display '" << display << "' ";
                os << "defines a view with an empty name and/or color space.";
                getImpl()->m_sanitytext = os.str();
                throw Exception(getImpl()->m_sanitytext.c_str());
            }

            int csindex = -1;
            if(!getImpl()->findColorSpaceIndex(&csindex, views[i].colorspace))
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The display '" << display << "' ";
                os << "refers to a color space, '" << views[i].colorspace << "', ";
                os << "which is not defined.";
                getImpl()->m_sanitytext = os.str();
                throw Exception(getImpl()->m_sanitytext.c_str());
            }

            // Confirm looks references exist.
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
                        getImpl()->m_sanitytext = os.str();
                        throw Exception(getImpl()->m_sanitytext.c_str());
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
        getImpl()->m_sanitytext = os.str();
        throw Exception(getImpl()->m_sanitytext.c_str());
    }


    ///// ACTIVE DISPLAYS & VIEWS

    StringVec displays;
    for (DisplayMap::const_iterator iter = getImpl()->m_displays.begin();
         iter != getImpl()->m_displays.end();
         ++iter)
    {
        displays.push_back(iter->first);
    }


    if (!getImpl()->m_activeDisplaysEnvOverride.empty())
    {
        const bool useAllDisplays
            = getImpl()->m_activeDisplaysEnvOverride.size()==1
                && getImpl()->m_activeDisplaysEnvOverride[0] == "";

        if (!useAllDisplays)
        {
            const StringVec orderedDisplays
                = IntersectStringVecsCaseIgnore(getImpl()->m_activeDisplaysEnvOverride, displays);
            if (orderedDisplays.empty())
            {
                std::stringstream ss;
                ss << "The content of the env. variable for the list of active displays ["
                   << JoinStringEnvStyle(getImpl()->m_activeDisplaysEnvOverride)
                   << "] is invalid.";
                throw Exception(ss.str().c_str());
            }
            if (orderedDisplays.size()!=getImpl()->m_activeDisplaysEnvOverride.size())
            {
                std::stringstream ss;
                ss << "The content of the env. variable for the list of active displays ["
                   << JoinStringEnvStyle(getImpl()->m_activeDisplaysEnvOverride)
                   << "] contains invalid display name(s).";
                throw Exception(ss.str().c_str());
            }
        }
    }
    else if(!getImpl()->m_activeDisplays.empty())
    {
        const bool useAllDisplays
            = getImpl()->m_activeDisplays.size()==1
                && getImpl()->m_activeDisplays[0] == "";

        if (!useAllDisplays)
        {
            const StringVec orderedDisplays
                = IntersectStringVecsCaseIgnore(getImpl()->m_activeDisplays, displays);
            if (orderedDisplays.empty())
            {
                std::stringstream ss;
                ss << "The list of active displays ["
                   << JoinStringEnvStyle(getImpl()->m_activeDisplays)
                   << "] from the config file is invalid.";
                throw Exception(ss.str().c_str());
            }
            if (orderedDisplays.size()!=getImpl()->m_activeDisplays.size())
            {
                std::stringstream ss;
                ss << "The list of active displays ["
                   << JoinStringEnvStyle(getImpl()->m_activeDisplays)
                   << "] from the config file contains invalid display name(s).";
                throw Exception(ss.str().c_str());
            }
        }
    }

    // TODO: Add validation for active views.


    ///// TRANSFORMS


    // Confirm for all Transforms that reference internal colorspaces,
    // the named space exists and that all Transforms are valid.
    {
        ConstTransformVec allTransforms;
        getImpl()->getAllInternalTransforms(allTransforms);

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
            if(!getImpl()->findColorSpaceIndex(&csindex, *iter))
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "This config references a color space, '" << *iter << "', ";
                os << "which is not defined.";
                getImpl()->m_sanitytext = os.str();
                throw Exception(getImpl()->m_sanitytext.c_str());
            }
        }
    }

    ///// LOOKS

    // For all looks, confirm the process space exists and the look is named
    for(unsigned int i=0; i<getImpl()->m_looksList.size(); ++i)
    {
        std::string name = getImpl()->m_looksList[i]->getName();
        if(name.empty())
        {
            std::ostringstream os;
            os << "Config failed sanitycheck. ";
            os << "The look at index '" << i << "' ";
            os << "does not specify a name.";
            getImpl()->m_sanitytext = os.str();
            throw Exception(getImpl()->m_sanitytext.c_str());
        }

        std::string processSpace = getImpl()->m_looksList[i]->getProcessSpace();
        if(processSpace.empty())
        {
            std::ostringstream os;
            os << "Config failed sanitycheck. ";
            os << "The look '" << name << "' ";
            os << "does not specify a process space.";
            getImpl()->m_sanitytext = os.str();
            throw Exception(getImpl()->m_sanitytext.c_str());
        }

        int csindex=0;
        if(!getImpl()->findColorSpaceIndex(&csindex, processSpace))
        {
            std::ostringstream os;
            os << "Config failed sanitycheck. ";
            os << "The look '" << name << "' ";
            os << "specifies a process color space, '";
            os << processSpace << "', which is not defined.";
            getImpl()->m_sanitytext = os.str();
            throw Exception(getImpl()->m_sanitytext.c_str());
        }
    }

    if (getMajorVersion() >= 2)
    {
        getImpl()->m_fileRules->validate(*this);
    }

    // Everything is groovy.
    getImpl()->m_sanity = Impl::SANITY_SANE;
}

///////////////////////////////////////////////////////////////////////////

const char * Config::getDescription() const
{
    return getImpl()->m_description.c_str();
}

void Config::setDescription(const char * description)
{
    getImpl()->m_description = description;

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

// RESOURCES //////////////////////////////////////////////////////////////

ConstContextRcPtr Config::getCurrentContext() const
{
    return getImpl()->m_context;
}

void Config::addEnvironmentVar(const char * name, const char * defaultValue)
{
    if(defaultValue)
    {
        getImpl()->m_env[std::string(name)] = std::string(defaultValue);
        getImpl()->m_context->setStringVar(name, defaultValue);
    }
    else
    {
        StringMap::iterator iter = getImpl()->m_env.find(std::string(name));
        if(iter != getImpl()->m_env.end()) getImpl()->m_env.erase(iter);
    }

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

int Config::getNumEnvironmentVars() const
{
    return static_cast<int>(getImpl()->m_env.size());
}

const char * Config::getEnvironmentVarNameByIndex(int index) const
{
    if(index < 0 || index >= (int)getImpl()->m_env.size()) return "";
    StringMap::const_iterator iter = getImpl()->m_env.begin();
    for(int i = 0; i < index; ++i) ++iter;
    return iter->first.c_str();
}

const char * Config::getEnvironmentVarDefault(const char * name) const
{
    return LookupEnvironment(getImpl()->m_env, name);
}

void Config::clearEnvironmentVars()
{
    getImpl()->m_env.clear();
    getImpl()->m_context->clearStringVars();

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

void Config::setEnvironmentMode(EnvironmentMode mode)
{
    getImpl()->m_context->setEnvironmentMode(mode);

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

EnvironmentMode Config::getEnvironmentMode() const
{
    return getImpl()->m_context->getEnvironmentMode();
}

void Config::loadEnvironment()
{
    getImpl()->m_context->loadEnvironment();

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

const char * Config::getSearchPath() const
{
    return getImpl()->m_context->getSearchPath();
}

void Config::setSearchPath(const char * path)
{
    getImpl()->m_context->setSearchPath(path);

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

int Config::getNumSearchPaths() const
{
    return getImpl()->m_context->getNumSearchPaths();
}

const char * Config::getSearchPath(int index) const
{
    return getImpl()->m_context->getSearchPath(index);
}

void Config::clearSearchPaths()
{
    getImpl()->m_context->clearSearchPaths();

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

void Config::addSearchPath(const char * path)
{
    getImpl()->m_context->addSearchPath(path);

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

const char * Config::getWorkingDir() const
{
    return getImpl()->m_context->getWorkingDir();
}

void Config::setWorkingDir(const char * dirname)
{
    getImpl()->m_context->setWorkingDir(dirname);

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}


///////////////////////////////////////////////////////////////////////////

ColorSpaceSetRcPtr Config::getColorSpaces(const char * category) const
{
    ColorSpaceSetRcPtr res = ColorSpaceSet::Create();

    // Loop on the list of active color spaces.

    for(int idx=0; idx<getNumColorSpaces(); ++idx)
    {
        const char * csName = getColorSpaceNameByIndex(idx);
        ConstColorSpaceRcPtr cs = getImpl()->m_allColorSpaces->getColorSpace(csName);
        if(!category || !*category || cs->hasCategory(category))
        {
            res->addColorSpace(cs);
        }
    }

    return res;
}

int Config::getNumColorSpaces(ColorSpaceVisibility visibility) const
{
    switch(visibility)
    {
        case COLORSPACE_ALL:
            return getImpl()->m_allColorSpaces->getNumColorSpaces();
        case COLORSPACE_ACTIVE:
            return (int)getImpl()->m_activeColorSpaceNames.size();
        case COLORSPACE_INACTIVE:
        {
            const std::vector<std::string> inactiveColorSpaceNames
                = getImpl()->buildInactiveColorSpaceList();
            return (int)inactiveColorSpaceNames.size();
        }
    }

    return 0;
}

const char * Config::getColorSpaceNameByIndex(ColorSpaceVisibility visibility, int index) const
{
    if (index < 0 || index >= getNumColorSpaces(visibility))
    {
        return "";
    }

    switch(visibility)
    {
        case COLORSPACE_ALL:
            return getImpl()->m_allColorSpaces->getColorSpaceNameByIndex(index);
        case COLORSPACE_ACTIVE:
            return getImpl()->m_activeColorSpaceNames[index].c_str();
        case COLORSPACE_INACTIVE:
        {
            const std::vector<std::string> inactiveColorSpaceNames
                = getImpl()->buildInactiveColorSpaceList();
            return inactiveColorSpaceNames[index].c_str();
        }
    }

    return "";
}

// Note: works from the list of all color spaces.
ConstColorSpaceRcPtr Config::getColorSpace(const char * name) const
{
    // Check to see if the name is a color space.
    ConstColorSpaceRcPtr cs = getImpl()->m_allColorSpaces->getColorSpace(name);
    if (!cs)
    {
        // Check to see if the name is a role.
        const char * csname = LookupRole(getImpl()->m_roles, name);
        cs = getImpl()->m_allColorSpaces->getColorSpace(csname);
    }

    return cs;
}

int Config::getNumColorSpaces() const
{
    return getNumColorSpaces(COLORSPACE_ACTIVE);
}

const char * Config::getColorSpaceNameByIndex(int index) const
{
    return getColorSpaceNameByIndex(COLORSPACE_ACTIVE, index);
}

int Config::getIndexForColorSpace(const char * name) const
{
    ConstColorSpaceRcPtr cs = getColorSpace(name);
    if (!cs)
    {
        return -1;
    }

    // Check to see if the name is an active color space.
    for (int idx = 0; idx < getNumColorSpaces(COLORSPACE_ACTIVE); ++idx)
    {
        if (std::string(getColorSpaceNameByIndex(COLORSPACE_ACTIVE, idx))
                == std::string(cs->getName()))
        {
            return idx;
        }
    }

    // Requests for an inactive color space or a role mapping 
    // to an inactive color space will both fail.
    return -1;
}

void Config::setInactiveColorSpaces(const char * inactiveColorSpaces)
{
    getImpl()->setInactiveColorSpaces(inactiveColorSpaces);
}

const char * Config::getInactiveColorSpaces() const
{
    return getImpl()->m_inactiveColorSpaceNamesConf.c_str();
}

void Config::addColorSpace(const ConstColorSpaceRcPtr & original)
{
    getImpl()->m_allColorSpaces->addColorSpace(original);
    
    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
    getImpl()->refreshActiveColorSpaces();
}

void Config::removeColorSpace(const char * name)
{
    getImpl()->m_allColorSpaces->removeColorSpace(name);
    
    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
    getImpl()->refreshActiveColorSpaces();
}

void Config::clearColorSpaces()
{
    getImpl()->m_allColorSpaces->clearColorSpaces();
    
    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
    getImpl()->refreshActiveColorSpaces();
}


///////////////////////////////////////////////////////////////////////////

const char * Config::parseColorSpaceFromString(const char * str) const
{
    int rightMostColorSpaceIndex = ParseColorSpaceFromString(*this, str);

    if(rightMostColorSpaceIndex>=0)
    {
        return getImpl()->m_allColorSpaces->getColorSpaceNameByIndex(rightMostColorSpaceIndex);
    }

    if(!getImpl()->m_strictParsing)
    {
        // Is a default role defined?
        const char* csname = LookupRole(getImpl()->m_roles, ROLE_DEFAULT);
        if(csname && *csname)
        {
            int csindex = -1;
            if (getImpl()->findColorSpaceIndex(&csindex, csname))
            {
                // This is necessary to not return a reference to
                // a local variable.
                return getImpl()->m_allColorSpaces->getColorSpaceNameByIndex(csindex);
            }
        }
    }

    return "";
}

bool Config::isStrictParsingEnabled() const
{
    return getImpl()->m_strictParsing;
}

void Config::setStrictParsingEnabled(bool enabled)
{
    getImpl()->m_strictParsing = enabled;

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

// Roles
void Config::setRole(const char * role, const char * colorSpaceName)
{
    // Set the role
    if(colorSpaceName)
    {
        getImpl()->m_roles[pystring::lower(role)] = std::string(colorSpaceName);
    }
    // Unset the role
    else
    {
        StringMap::iterator iter = getImpl()->m_roles.find(pystring::lower(role));
        if(iter != getImpl()->m_roles.end())
        {
            getImpl()->m_roles.erase(iter);
        }
    }

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

int Config::getNumRoles() const
{
    return static_cast<int>(getImpl()->m_roles.size());
}

bool Config::hasRole(const char * role) const
{
    const char* rname = LookupRole(getImpl()->m_roles, role);
    return  rname && *rname;
}

const char * Config::getRoleName(int index) const
{
    if(index < 0 || index >= (int)getImpl()->m_roles.size()) return "";
    StringMap::const_iterator iter = getImpl()->m_roles.begin();
    for(int i = 0; i < index; ++i) ++iter;
    return iter->first.c_str();
}

///////////////////////////////////////////////////////////////////////////
//
// Display/View Registration

const char * Config::getDefaultDisplay() const
{
    return getDisplay(0);
}


int Config::getNumDisplays() const
{
    if(getImpl()->m_displayCache.empty())
    {
        ComputeDisplays(getImpl()->m_displayCache,
                        getImpl()->m_displays,
                        getImpl()->m_activeDisplays,
                        getImpl()->m_activeDisplaysEnvOverride);
    }

    return static_cast<int>(getImpl()->m_displayCache.size());
}

const char * Config::getDisplay(int index) const
{
    if(getImpl()->m_displayCache.empty())
    {
        ComputeDisplays(getImpl()->m_displayCache,
                        getImpl()->m_displays,
                        getImpl()->m_activeDisplays,
                        getImpl()->m_activeDisplaysEnvOverride);
    }

    if(index>=0 || index < static_cast<int>(getImpl()->m_displayCache.size()))
    {
        return getImpl()->m_displayCache[index].c_str();
    }

    return "";
}

const char * Config::getDefaultView(const char * display) const
{
    return getView(display, 0);
}

int Config::getNumViews(const char * display) const
{
    if(getImpl()->m_displayCache.empty())
    {
        ComputeDisplays(getImpl()->m_displayCache,
                        getImpl()->m_displays,
                        getImpl()->m_activeDisplays,
                        getImpl()->m_activeDisplaysEnvOverride);
    }

    if(!display) return 0;

    DisplayMap::const_iterator iter = find_display_const(getImpl()->m_displays, display);
    if(iter == getImpl()->m_displays.end()) return 0;

    const ViewVec & views = iter->second;

    StringVec masterViews;
    for(unsigned int i=0; i<views.size(); ++i)
    {
        masterViews.push_back(views[i].name);
    }

    if(!getImpl()->m_activeViewsEnvOverride.empty())
    {
        const StringVec orderedViews
            = IntersectStringVecsCaseIgnore(getImpl()->m_activeViewsEnvOverride, masterViews);

        if(!orderedViews.empty())
        {
            return static_cast<int>(orderedViews.size());
        }
    }
    else if(!getImpl()->m_activeViews.empty())
    {
        const StringVec orderedViews
            = IntersectStringVecsCaseIgnore(getImpl()->m_activeViews, masterViews);

        if(!orderedViews.empty())
        {
            return static_cast<int>(orderedViews.size());
        }
    }

    return static_cast<int>(masterViews.size());
}

const char * Config::getView(const char * display, int index) const
{
    if(getImpl()->m_displayCache.empty())
    {
        ComputeDisplays(getImpl()->m_displayCache,
                        getImpl()->m_displays,
                        getImpl()->m_activeDisplays,
                        getImpl()->m_activeDisplaysEnvOverride);
    }

    if(!display) return "";

    DisplayMap::const_iterator iter = find_display_const(getImpl()->m_displays, display);
    if(iter == getImpl()->m_displays.end()) return "";

    const ViewVec & views = iter->second;

    StringVec masterViews;
    for(unsigned int i = 0; i < views.size(); ++i)
    {
        masterViews.push_back(views[i].name);
    }

    int idx = index;

    if(!getImpl()->m_activeViewsEnvOverride.empty())
    {
        const StringVec orderedViews
            = IntersectStringVecsCaseIgnore(getImpl()->m_activeViewsEnvOverride, masterViews);

        if(!orderedViews.empty())
        {
            idx = FindInStringVecCaseIgnore(masterViews, orderedViews[index]);
        }
    }
    else if(!getImpl()->m_activeViews.empty())
    {
        const StringVec orderedViews
            = IntersectStringVecsCaseIgnore(getImpl()->m_activeViews, masterViews);

        if(!orderedViews.empty())
        {
            idx = FindInStringVecCaseIgnore(masterViews, orderedViews[index]);
        }
    }

    if(idx >= 0)
    {
        return views[idx].name.c_str();
    }

    if(!views.empty())
    {
        return views[0].name.c_str();
    }

    return "";
}

const char * Config::getDisplayColorSpaceName(const char * display, const char * view) const
{
    if(!display || !view) return "";

    DisplayMap::const_iterator iter = find_display_const(getImpl()->m_displays, display);
    if(iter == getImpl()->m_displays.end()) return "";

    const ViewVec & views = iter->second;
    int index = find_view(views, view);
    if(index<0) return "";

    return views[index].colorspace.c_str();
}

const char * Config::getDisplayLooks(const char * display, const char * view) const
{
    if(!display || !view) return "";

    DisplayMap::const_iterator iter = find_display_const(getImpl()->m_displays, display);
    if(iter == getImpl()->m_displays.end()) return "";

    const ViewVec & views = iter->second;
    int index = find_view(views, view);
    if(index<0) return "";

    return views[index].looks.c_str();
}

void Config::addDisplay(const char * display, const char * view,
                        const char * colorSpaceName, const char * lookName)
{

    if(!display || !view || !colorSpaceName || !lookName) return;

    AddDisplay(getImpl()->m_displays,
                display, view, colorSpaceName, lookName);
    getImpl()->m_displayCache.clear();

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

void Config::clearDisplays()
{
    getImpl()->m_displays.clear();
    getImpl()->m_displayCache.clear();

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

void Config::setActiveDisplays(const char * displays)
{
    getImpl()->m_activeDisplays.clear();
    SplitStringEnvStyle(getImpl()->m_activeDisplays, displays);

    getImpl()->m_displayCache.clear();

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

const char * Config::getActiveDisplays() const
{
    getImpl()->m_activeDisplaysStr = JoinStringEnvStyle(getImpl()->m_activeDisplays);
    return getImpl()->m_activeDisplaysStr.c_str();
}

void Config::setActiveViews(const char * views)
{
    getImpl()->m_activeViews.clear();
    SplitStringEnvStyle(getImpl()->m_activeViews, views);

    getImpl()->m_displayCache.clear();

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

const char * Config::getActiveViews() const
{
    getImpl()->m_activeViewsStr = JoinStringEnvStyle(getImpl()->m_activeViews);
    return getImpl()->m_activeViewsStr.c_str();
}

///////////////////////////////////////////////////////////////////////////

void Config::getDefaultLumaCoefs(double * c3) const
{
    memcpy(c3, &getImpl()->m_defaultLumaCoefs[0], 3*sizeof(double));
}

void Config::setDefaultLumaCoefs(const double * c3)
{
    memcpy(&getImpl()->m_defaultLumaCoefs[0], c3, 3*sizeof(double));

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

///////////////////////////////////////////////////////////////////////////

ConstLookRcPtr Config::getLook(const char * name) const
{
    std::string namelower = pystring::lower(name);

    for(unsigned int i=0; i<getImpl()->m_looksList.size(); ++i)
    {
        if(pystring::lower(getImpl()->m_looksList[i]->getName()) == namelower)
        {
            return getImpl()->m_looksList[i];
        }
    }

    return ConstLookRcPtr();
}

int Config::getNumLooks() const
{
    return static_cast<int>(getImpl()->m_looksList.size());
}

const char * Config::getLookNameByIndex(int index) const
{
    if(index<0 || index>=static_cast<int>(getImpl()->m_looksList.size()))
    {
        return "";
    }

    return getImpl()->m_looksList[index]->getName();
}

void Config::addLook(const ConstLookRcPtr & look)
{
    std::string name = look->getName();
    if(name.empty())
        throw Exception("Cannot addLook with an empty name.");

    std::string namelower = pystring::lower(name);

    // If the look exists, replace it
    for(unsigned int i=0; i<getImpl()->m_looksList.size(); ++i)
    {
        if(pystring::lower(getImpl()->m_looksList[i]->getName()) == namelower)
        {
            getImpl()->m_looksList[i] = look->createEditableCopy();
            return;
        }
    }

    // Otherwise, add it
    getImpl()->m_looksList.push_back(look->createEditableCopy());

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

void Config::clearLooks()
{
    getImpl()->m_looksList.clear();

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

///////////////////////////////////////////////////////////////////////////

ConstFileRulesRcPtr Config::getFileRules() const noexcept
{
    return getImpl()->m_fileRules;
}

void Config::setFileRules(ConstFileRulesRcPtr fileRules)
{
    if (getMajorVersion() < 2)
    {
        throw Exception("Only config version 2 or higher can accept file rules.");
    }

    getImpl()->m_fileRules = fileRules->createEditableCopy();

    AutoMutex lock(getImpl()->m_cacheidMutex);
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
        throw Exception("Config::GetProcessor failed. Source color space is null.");
    }
    if(!dst)
    {
        throw Exception("Config::GetProcessor failed. Destination color space is null.");
    }

    ProcessorRcPtr processor = Processor::Create();
    processor->getImpl()->setColorSpaceConversion(*this, context, src, dst);
    processor->getImpl()->computeMetadata();
    return processor;
}

ConstProcessorRcPtr Config::getProcessor(const char * srcName,
                                            const char * dstName) const
{
    ConstContextRcPtr context = getCurrentContext();
    return getProcessor(context, srcName, dstName);
}

// Names can be color space name or role name
ConstProcessorRcPtr Config::getProcessor(const ConstContextRcPtr & context,
                                            const char * srcName,
                                            const char * dstName) const
{
    ConstColorSpaceRcPtr src = getColorSpace(srcName);
    if(!src)
    {
        std::ostringstream os;
        os << "Could not find color space '" << srcName << "'.";
        throw Exception(os.str().c_str());
    }

    ConstColorSpaceRcPtr dst = getColorSpace(dstName);
    if(!dst)
    {
        std::ostringstream os;
        os << "Could not find color space '" << dstName << "'.";
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
    processor->getImpl()->setTransform(*this, context, transform, direction);
    processor->getImpl()->computeMetadata();
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
    AutoMutex lock(getImpl()->m_cacheidMutex);

    // A null context will use the empty cacheid
    std::string contextcacheid = "";
    if(context) contextcacheid = context->getCacheID();

    StringMap::const_iterator cacheiditer = getImpl()->m_cacheids.find(contextcacheid);
    if(cacheiditer != getImpl()->m_cacheids.end())
    {
        return cacheiditer->second.c_str();
    }

    // Include the hash of the yaml config serialization
    if(getImpl()->m_cacheidnocontext.empty())
    {
        std::stringstream cacheid;
        serialize(cacheid);
        std::string fullstr = cacheid.str();
        getImpl()->m_cacheidnocontext = CacheIDHash(fullstr.c_str(), (int)fullstr.size());
    }

    // Also include all file references, using the context (if specified)
    std::string fileReferencesFashHash = "";
    if(context)
    {
        std::ostringstream filehash;

        ConstTransformVec allTransforms;
        getImpl()->getAllInternalTransforms(allTransforms);

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

    getImpl()->m_cacheids[contextcacheid] = getImpl()->m_cacheidnocontext + ":" + fileReferencesFashHash;
    return getImpl()->m_cacheids[contextcacheid].c_str();
}

///////////////////////////////////////////////////////////////////////////
//  Serialization

void Config::serialize(std::ostream& os) const
{
    try
    {
        OCIOYaml::Write(os, this);
    }
    catch( const std::exception & e)
    {
        std::ostringstream error;
        error << "Error building YAML: " << e.what();
        throw Exception(error.str().c_str());
    }
}


///////////////////////////////////////////////////////////////////////////
//  Config::Impl

std::vector<std::string> Config::Impl::buildInactiveColorSpaceList() const
{
    std::vector<std::string> inactiveColorSpaces;

    // An API request always supersedes the other lists.
    if (!m_inactiveColorSpaceNamesAPI.empty())
    {
        pystring::split(m_inactiveColorSpaceNamesAPI, inactiveColorSpaces, ",");
    }
    // The env. variable only supersedes the config list.
    else if (!m_inactiveColorSpaceNamesEnv.empty())
    {
        pystring::split(m_inactiveColorSpaceNamesEnv, inactiveColorSpaces, ",");
    }
    else if (!m_inactiveColorSpaceNamesConf.empty())
    {
        pystring::split(m_inactiveColorSpaceNamesConf, inactiveColorSpaces, ",");
    }

    for (auto & v : inactiveColorSpaces)
    {
        v = pystring::strip(v);
    }

    return inactiveColorSpaces;
}

void Config::Impl::refreshActiveColorSpaces()
{
    m_activeColorSpaceNames.clear();

    const std::vector<std::string> inactiveColorSpaces = buildInactiveColorSpaceList();

    for (int i = 0; i < m_allColorSpaces->getNumColorSpaces(); ++i)
    {
        ConstColorSpaceRcPtr cs = m_allColorSpaces->getColorSpaceByIndex(i);
        const std::string name(cs->getName());

        bool isActive = true;

        for (const auto & csName : inactiveColorSpaces)
        {
            if (csName==name)
            {
                isActive = false;
                break;
            }
        }

        if (isActive)
        {
            m_activeColorSpaceNames.push_back(cs->getName());
        }
    }
}

void Config::Impl::resetCacheIDs()
{
    m_cacheids.clear();
    m_cacheidnocontext = "";
    m_sanity = SANITY_UNKNOWN;
    m_sanitytext = "";
}

void Config::Impl::getAllInternalTransforms(ConstTransformVec & transformVec) const
{
    // Grab all transforms from the ColorSpaces
    for(int i=0; i<m_allColorSpaces->getNumColorSpaces(); ++i)
    {
        if(m_allColorSpaces->getColorSpaceByIndex(i)->getTransform(COLORSPACE_DIR_TO_REFERENCE))
        {
            transformVec.push_back(
                m_allColorSpaces->getColorSpaceByIndex(i)->getTransform(COLORSPACE_DIR_TO_REFERENCE));
        }
        if(m_allColorSpaces->getColorSpaceByIndex(i)->getTransform(COLORSPACE_DIR_FROM_REFERENCE))
        {
            transformVec.push_back(
                m_allColorSpaces->getColorSpaceByIndex(i)->getTransform(COLORSPACE_DIR_FROM_REFERENCE));
        }
    }

    // Grab all transforms from the Looks
    for(unsigned int i=0; i<m_looksList.size(); ++i)
    {
        if(m_looksList[i]->getTransform())
            transformVec.push_back(m_looksList[i]->getTransform());
        if(m_looksList[i]->getInverseTransform())
            transformVec.push_back(m_looksList[i]->getInverseTransform());
    }

}

ConstConfigRcPtr Config::Impl::Read(std::istream & istream, const char * filename)
{
    ConfigRcPtr config = Config::Create();
    OCIOYaml::Read(istream, config, filename);

    // An API request always supersedes the env. variable. As the OCIOYaml helper methods
    // use the Config public API, the variable reset highlights that only the
    // env. variable and the config contents are valid after a config file read.
    config->getImpl()->m_inactiveColorSpaceNamesAPI.clear();
    config->getImpl()->refreshActiveColorSpaces();

    return config;
}

} // namespace OCIO_NAMESPACE

