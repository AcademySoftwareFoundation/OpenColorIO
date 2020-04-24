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

#include "Display.h"
#include "FileRules.h"
#include "HashUtils.h"
#include "Logging.h"
#include "LookParse.h"
#include "MathUtils.h"
#include "Mutex.h"
#include "OCIOYaml.h"
#include "OpBuilders.h"
#include "ParseUtils.h"
#include "PathUtils.h"
#include "Platform.h"
#include "PrivateTypes.h"
#include "Processor.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

const char * OCIO_CONFIG_ENVVAR               = "OCIO";
const char * OCIO_ACTIVE_DISPLAYS_ENVVAR      = "OCIO_ACTIVE_DISPLAYS";
const char * OCIO_ACTIVE_VIEWS_ENVVAR         = "OCIO_ACTIVE_VIEWS";
const char * OCIO_INACTIVE_COLORSPACES_ENVVAR = "OCIO_INACTIVE_COLORSPACES";
const char * OCIO_OPTIMIZATION_FLAGS_ENVVAR   = "OCIO_OPTIMIZATION_FLAGS";

namespace
{

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
    StringMap::const_iterator iter = roles.find(StringUtils::Lower(rolename));
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
        colorSpaceNames.insert(context->resolveStringVar(lookTransform->getSrc()));
        colorSpaceNames.insert(context->resolveStringVar(lookTransform->getDst()));
    }
}

static constexpr char AddedDefault[]{ "added_default_rule_colorspace" };

void FindAvailableName(const ColorSpaceSetRcPtr & colorspaces, std::string & csname)
{
    int i = 0;
    csname = AddedDefault;
    while (true)
    {
        if (!colorspaces->hasColorSpace(csname.c_str()))
        {
            break;
        }

        csname = AddedDefault + std::to_string(i);
        ++i;
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
    char m_familySeparator = 0;
    std::string m_description;

    // The final list of inactive color spaces is built from several inputs.
    // An API request (using Config::setInactiveColorSpaces()) always supersedes
    // the list from the env. variable (i.e. OCIO_INACTIVE_COLORSPACES_ENVVAR) which
    // supersedes the list from the config. file (i.e. inactive_colorspaces).
    //
    // Refer to Config::Impl::refreshActiveColorSpaces() to have the implementation details.

    ColorSpaceSetRcPtr m_allColorSpaces; // All the color spaces (i.e. no filtering).
    StringUtils::StringVec m_activeColorSpaceNames; // A built list of active color space names.

    std::string m_inactiveColorSpaceNamesAPI;  // Inactive color space filter from API request. 
    std::string m_inactiveColorSpaceNamesEnv;  // Inactive color space filter from env. variable.
    std::string m_inactiveColorSpaceNamesConf; // Inactive color space filter from config. file. 

    StringMap m_roles;
    LookVec m_looksList;

    DisplayMap m_displays;
    StringUtils::StringVec m_activeDisplays;
    StringUtils::StringVec m_activeDisplaysEnvOverride;
    StringUtils::StringVec m_activeViews;
    StringUtils::StringVec m_activeViewsEnvOverride;

    std::vector<ViewTransformRcPtr> m_viewTransforms;

    mutable std::string m_activeDisplaysStr;
    mutable std::string m_activeViewsStr;
    mutable StringUtils::StringVec m_displayCache;

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
        activeDisplays = StringUtils::Trim(activeDisplays);
        if (!activeDisplays.empty())
        {
            SplitStringEnvStyle(m_activeDisplaysEnvOverride, activeDisplays.c_str());
        }

        std::string activeViews;
        Platform::Getenv(OCIO_ACTIVE_VIEWS_ENVVAR, activeViews);
        activeViews = StringUtils::Trim(activeViews);
        if (!activeViews.empty())
        {
            SplitStringEnvStyle(m_activeViewsEnvOverride, activeViews.c_str());
        }

        m_defaultLumaCoefs.resize(3);
        m_defaultLumaCoefs[0] = DEFAULT_LUMA_COEFF_R;
        m_defaultLumaCoefs[1] = DEFAULT_LUMA_COEFF_G;
        m_defaultLumaCoefs[2] = DEFAULT_LUMA_COEFF_B;

        Platform::Getenv(OCIO_INACTIVE_COLORSPACES_ENVVAR, m_inactiveColorSpaceNamesEnv);
        m_inactiveColorSpaceNamesEnv = StringUtils::Trim(m_inactiveColorSpaceNamesEnv);
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
            m_familySeparator = rhs.m_familySeparator;
            m_description = rhs.m_description;

            m_allColorSpaces = rhs.m_allColorSpaces; // Deep copy the colorspaces
            m_activeColorSpaceNames       = rhs.m_activeColorSpaceNames;
            m_inactiveColorSpaceNamesConf = rhs.m_inactiveColorSpaceNamesConf;
            m_inactiveColorSpaceNamesEnv  = rhs.m_inactiveColorSpaceNamesEnv;
            m_inactiveColorSpaceNamesAPI  = rhs.m_inactiveColorSpaceNamesAPI;

            // Deep copy the looks.
            m_looksList.clear();
            m_looksList.reserve(rhs.m_looksList.size());
            for(unsigned int i=0; i<rhs.m_looksList.size(); ++i)
            {
                m_looksList.push_back(rhs.m_looksList[i]->createEditableCopy());
            }

            // Assignment operator will suffice for these.
            m_roles = rhs.m_roles;

            m_displays = rhs.m_displays;
            m_activeDisplays = rhs.m_activeDisplays;
            m_activeViews = rhs.m_activeViews;
            m_activeViewsEnvOverride = rhs.m_activeViewsEnvOverride;
            m_activeDisplaysEnvOverride = rhs.m_activeDisplaysEnvOverride;
            m_activeDisplaysStr = rhs.m_activeDisplaysStr;
            m_displayCache = rhs.m_displayCache;

            // Deep copy view transforms.
            m_viewTransforms.clear();
            m_viewTransforms.reserve(rhs.m_viewTransforms.size());
            for (const auto & vt : rhs.m_viewTransforms)
            {
                m_viewTransforms.push_back(vt->createEditableCopy());
            }

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

    // Only search for a color space name (i.e. not for a role name).
    int getColorSpaceIndex(const char * csname) const
    {
        return m_allColorSpaces->getColorSpaceIndex(csname);
    }

    // Only search for a color space name (i.e. not for a role name).
    bool hasColorSpace(const char * csname) const
    {
        return m_allColorSpaces->hasColorSpace(csname);
    }

    StringUtils::StringVec buildInactiveColorSpaceList() const;
    void refreshActiveColorSpaces();

    // Any time you modify the state of the config, you must call this
    // to reset internal cache states.  You also should do this in a
    // thread safe manner by acquiring the m_cacheidMutex.
    void resetCacheIDs();

    // Get all internal transforms (to generate cacheIDs, validation, etc).
    // This currently crawls colorspaces + looks + view transforms.
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

        const char * rname = LookupRole(m_roles, ROLE_DEFAULT);
        if (!hasColorSpace(rname))
        {
            std::string defaultCS;
            bool addNewDefault = true;
            // The default role doesn't exist so look for a color space named "raw"
            // (not case-sensitive) with isdata true.
            const int csindex = getColorSpaceIndex("raw");
            if (-1 != csindex)
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
            = StringUtils::Trim(std::string(inactiveColorSpaces ? inactiveColorSpaces : ""));

        // An API request must always supersede the two other lists. Filling the
        // inactiveColorSpaceNamesAPI_ list highlights the API request precedence.
        m_inactiveColorSpaceNamesAPI = m_inactiveColorSpaceNamesConf;

        AutoMutex lock(m_cacheidMutex);
        resetCacheIDs();
        refreshActiveColorSpaces();
    }

    void checkVersionConsistency(ConstTransformRcPtr & transform) const;
    void checkVersionConsistency() const;

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
    m_impl = nullptr;
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

    bool hasDisplayReferredColorspace = false;

    // Confirm all ColorSpaces are valid.
    for(int i=0; i<getImpl()->m_allColorSpaces->getNumColorSpaces(); ++i)
    {
        const auto cs = getImpl()->m_allColorSpaces->getColorSpaceByIndex(i);
        if (!cs)
        {
            std::ostringstream os;
            os << "Config failed sanitycheck. ";
            os << "The color space at index " << i << " is null.";
            getImpl()->m_sanitytext = os.str();
            throw Exception(getImpl()->m_sanitytext.c_str());
        }

        const char * name = cs->getName();
        if (!name || !*name)
        {
            std::ostringstream os;
            os << "Config failed sanitycheck. ";
            os << "The color space at index " << i << " is not named.";
            getImpl()->m_sanitytext = os.str();
            throw Exception(getImpl()->m_sanitytext.c_str());
        }

        const std::string namelower = StringUtils::Lower(name);
        StringSet::const_iterator it = existingColorSpaces.find(namelower);
        if (it != existingColorSpaces.end())
        {
            std::ostringstream os;
            os << "Config failed sanitycheck. ";
            os << "Two colorspaces are defined with the same name, '";
            os << namelower << "'.";
            getImpl()->m_sanitytext = os.str();
            throw Exception(getImpl()->m_sanitytext.c_str());
        }

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
        if (cs->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY)
        {
            hasDisplayReferredColorspace = true;
        }
    }

    // Confirm all roles are valid.
    {
        for(StringMap::const_iterator iter = getImpl()->m_roles.begin(),
            end = getImpl()->m_roles.end(); iter!=end; ++iter)
        {
            if(!getImpl()->hasColorSpace(iter->second.c_str()))
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
            if(getImpl()->hasColorSpace(iter->first.c_str()))
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
    const StringUtils::StringVec inactiveColorSpaceNames
        = getImpl()->buildInactiveColorSpaceList();

    for (const auto & name : inactiveColorSpaceNames)
    {
        ConstColorSpaceRcPtr cs = getImpl()->m_allColorSpaces->getColorSpace(name.c_str());
        if (!cs)
        {
            std::ostringstream os;
            os << "Inactive color space '" << name << "' does not exist.";
            LogWarning(os.str());
        }
    }


    ///// DISPLAYS / VIEWS

    int numviews = 0;

    // Confirm all Display transforms refer to colorspaces that exit.
    for(DisplayMap::const_iterator iter = getImpl()->m_displays.begin();
        iter != getImpl()->m_displays.end();
        ++iter)
    {
        const std::string display = iter->first;
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
            if(views[i].m_name.empty() || views[i].m_colorspace.empty())
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The display '" << display << "' ";
                os << "defines a view with an empty name and/or color space.";
                getImpl()->m_sanitytext = os.str();
                throw Exception(getImpl()->m_sanitytext.c_str());
            }

            if(!getImpl()->hasColorSpace(views[i].m_colorspace.c_str()))
            {
                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "The display '" << display << "' ";
                os << " view '" << views[i].m_name << "' ";
                os << "refers to a color space, '" << views[i].m_colorspace << "', ";
                os << "which is not defined.";
                getImpl()->m_sanitytext = os.str();
                throw Exception(getImpl()->m_sanitytext.c_str());
            }

            // If there is a view transform, it must exist and its color space must be a
            // display-referred color space.
            if (!views[i].m_viewTransform.empty())
            {
                if (!getViewTransform(views[i].m_viewTransform.c_str()))
                {
                    std::ostringstream os;
                    os << "Config failed sanitycheck. ";
                    os << "The display '" << display << "' ";
                    os << " view '" << views[i].m_name << "' ";
                    os << "refers to a view transform, '" << views[i].m_viewTransform << "', ";
                    os << "which is not defined.";
                    getImpl()->m_sanitytext = os.str();
                    throw Exception(getImpl()->m_sanitytext.c_str());
                }

                auto cs = getImpl()->m_allColorSpaces->getColorSpace(views[i].m_colorspace.c_str());
                if (cs->getReferenceSpaceType() != REFERENCE_SPACE_DISPLAY)
                {
                    std::ostringstream os;
                    os << "Config failed sanitycheck. ";
                    os << "The display '" << display << "' ";
                    os << " view '" << views[i].m_name << "' ";
                    os << "refers to a color space, '" << views[i].m_colorspace << "', ";
                    os << "that is not a display-referred color space.";
                    getImpl()->m_sanitytext = os.str();
                    throw Exception(getImpl()->m_sanitytext.c_str());
                }
            }

            // Confirm looks references exist.
            LookParseResult looks;
            const LookParseResult::Options & options = looks.parse(views[i].m_looks);

            for(unsigned int optionindex=0;
                optionindex < options.size();
                ++optionindex)
            {
                for(unsigned int tokenindex=0;
                    tokenindex < options[optionindex].size();
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
    if (numviews == 0)
    {
        std::ostringstream os;
        os << "Config failed sanitycheck. ";
        os << "No displays are specified.";
        getImpl()->m_sanitytext = os.str();
        throw Exception(getImpl()->m_sanitytext.c_str());
    }


    ///// ACTIVE DISPLAYS & VIEWS

    StringUtils::StringVec displays;
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
            const StringUtils::StringVec orderedDisplays
                = IntersectStringVecsCaseIgnore(getImpl()->m_activeDisplaysEnvOverride, displays);
            if (orderedDisplays.empty())
            {
                std::ostringstream os;
                os << "The content of the env. variable for the list of active displays ["
                   << JoinStringEnvStyle(getImpl()->m_activeDisplaysEnvOverride)
                   << "] is invalid.";
                getImpl()->m_sanitytext = os.str();
                throw Exception(getImpl()->m_sanitytext.c_str());
            }
            if (orderedDisplays.size()!=getImpl()->m_activeDisplaysEnvOverride.size())
            {
                std::ostringstream os;
                os << "The content of the env. variable for the list of active displays ["
                   << JoinStringEnvStyle(getImpl()->m_activeDisplaysEnvOverride)
                   << "] contains invalid display name(s).";
                getImpl()->m_sanitytext = os.str();
                throw Exception(getImpl()->m_sanitytext.c_str());
            }
        }
    }
    else if (!getImpl()->m_activeDisplays.empty())
    {
        const bool useAllDisplays = getImpl()->m_activeDisplays.size() == 1 &&
                                    getImpl()->m_activeDisplays[0] == "";

        if (!useAllDisplays)
        {
            const StringUtils::StringVec orderedDisplays
                = IntersectStringVecsCaseIgnore(getImpl()->m_activeDisplays, displays);
            if (orderedDisplays.empty())
            {
                std::ostringstream os;
                os << "The list of active displays ["
                   << JoinStringEnvStyle(getImpl()->m_activeDisplays)
                   << "] from the config file is invalid.";
                getImpl()->m_sanitytext = os.str();
                throw Exception(getImpl()->m_sanitytext.c_str());
            }
            if (orderedDisplays.size()!=getImpl()->m_activeDisplays.size())
            {
                std::ostringstream os;
                os << "The list of active displays ["
                   << JoinStringEnvStyle(getImpl()->m_activeDisplays)
                   << "] from the config file contains invalid display name(s).";
                getImpl()->m_sanitytext = os.str();
                throw Exception(getImpl()->m_sanitytext.c_str());
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
        for (unsigned int i = 0; i < allTransforms.size(); ++i)
        {
            allTransforms[i]->validate();

            ConstContextRcPtr context = getCurrentContext();
            GetColorSpaceReferences(colorSpaceNames, allTransforms[i], context);
        }

        for (const auto & name : colorSpaceNames)
        {
            if (!getImpl()->hasColorSpace(name.c_str()))
            {
                // Check to see if the name is a role.
                const char * csname = LookupRole(getImpl()->m_roles, name);

                std::ostringstream os;
                os << "Config failed sanitycheck. ";
                os << "This config references a color space, '";

                if (!csname || !*csname)
                {
                    os << name << "', which is not defined.";
                    getImpl()->m_sanitytext = os.str();
                    throw Exception(getImpl()->m_sanitytext.c_str());
                }
                else if(!getImpl()->hasColorSpace(csname))
                {
                    os << csname << "' (for role '" << name << "'), which is not defined.";
                    getImpl()->m_sanitytext = os.str();
                    throw Exception(getImpl()->m_sanitytext.c_str());
                }
            }
        }
    }

    ///// LOOKS

    // For all looks, confirm the process space exists and the look is named.
    for(unsigned int i=0; i<getImpl()->m_looksList.size(); ++i)
    {
        const char * lookName = getImpl()->m_looksList[i]->getName();
        if (!lookName || !*lookName)
        {
            std::ostringstream os;
            os << "Config failed sanitycheck. ";
            os << "The look at index '" << i << "' ";
            os << "does not specify a name.";
            getImpl()->m_sanitytext = os.str();
            throw Exception(getImpl()->m_sanitytext.c_str());
        }

        const char * processSpace = getImpl()->m_looksList[i]->getProcessSpace();
        if (!processSpace || !*processSpace)
        {
            std::ostringstream os;
            os << "Config failed sanitycheck. ";
            os << "The look '" << lookName << "' ";
            os << "does not specify a process space.";
            getImpl()->m_sanitytext = os.str();
            throw Exception(getImpl()->m_sanitytext.c_str());
        }

        if (!getImpl()->hasColorSpace(processSpace))
        {
            // Check to see if the processSpace is a role.
            const char * csname = LookupRole(getImpl()->m_roles, processSpace);

            std::ostringstream os;
            os << "Config failed sanitycheck. ";
            os << "The look '" << lookName << "' ";
            os << "specifies a process color space, '";

            if (!csname || !*csname)
            {
                os << processSpace << "', which is not defined.";
                getImpl()->m_sanitytext = os.str();
                throw Exception(getImpl()->m_sanitytext.c_str());
            }
            else if (!getImpl()->hasColorSpace(csname))
            {
                os << csname << "' (for role '" << processSpace << "'), which is not defined.";
                getImpl()->m_sanitytext = os.str();
                throw Exception(getImpl()->m_sanitytext.c_str());
            }
        }
    }

    ///// ViewTransforms

    if (!getImpl()->m_viewTransforms.empty())
    {
        // Note: Config::addViewTransform validates that view_transforms have a unique, non-empty
        // name and define a transform.

        auto fromScene = getDefaultSceneToDisplayViewTransform();
        // If there are view transforms, there must be one from the scene reference space.
        if (!fromScene)
        {
            getImpl()->m_sanitytext = "Config failed sanitycheck. If there are view_transforms, "
                                      "at least one must use the scene reference space.";
            throw Exception(getImpl()->m_sanitytext.c_str());
        }
    }
    else if (hasDisplayReferredColorspace)
    {
        getImpl()->m_sanitytext = "Config failed sanitycheck. If there are display-referred "
                                  "color spaces, there must be view_transforms.";
        throw Exception(getImpl()->m_sanitytext.c_str());
    }

    ///// FileRules

    if (getMajorVersion() >= 2)
    {
        auto colorSpaceAccessor = std::bind(&Config::getColorSpace, this, std::placeholders::_1);
        getImpl()->m_fileRules->getImpl()->sanityCheck(colorSpaceAccessor);
    }

    // Everything is groovy.
    getImpl()->m_sanity = Impl::SANITY_SANE;
}

///////////////////////////////////////////////////////////////////////////

char Config::getFamilySeparator() const
{
    return getImpl()->m_familySeparator;
}

void Config::setFamilySeparator(char separator)
{
    const int val = (int)separator;
    if (val!=0 && (val<32 || val>126))
    {
        std::string err("Invalid family separator '");
        err += separator;
        err += "'.";

        throw Exception(err.c_str());
    }

    getImpl()->m_familySeparator = separator;
    
    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
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

namespace
{
bool MatchReferenceType(SearchReferenceSpaceType st, ReferenceSpaceType t)
{
    switch (st)
    {
    case SEARCH_REFERENCE_SPACE_SCENE:
        return t == REFERENCE_SPACE_SCENE;
    case SEARCH_REFERENCE_SPACE_DISPLAY:
        return t == REFERENCE_SPACE_DISPLAY;
    case SEARCH_REFERENCE_SPACE_ALL:
        return true;
    }
    return false;
}
}

int Config::getNumColorSpaces(SearchReferenceSpaceType searchReferenceType,
                              ColorSpaceVisibility visibility) const
{
    int res = 0;
    switch (visibility)
    {
    case COLORSPACE_ALL:
    {
        const int nbCS = getImpl()->m_allColorSpaces->getNumColorSpaces();
        if (searchReferenceType == SEARCH_REFERENCE_SPACE_ALL)
        {
            return nbCS;
        }
        for (int i = 0; i < nbCS; ++i)
        {
            auto cs = getImpl()->m_allColorSpaces->getColorSpaceByIndex(i);
            if (MatchReferenceType(searchReferenceType, cs->getReferenceSpaceType()))
            {
                ++res;
            }
        }
        break;
    }
    case COLORSPACE_ACTIVE:
    {
        const size_t nbCS = getImpl()->m_activeColorSpaceNames.size();
        if (searchReferenceType == SEARCH_REFERENCE_SPACE_ALL)
        {
            return (int)nbCS;
        }
        for (auto csname : getImpl()->m_activeColorSpaceNames)
        {
            auto cs = getColorSpace(csname.c_str());
            if (MatchReferenceType(searchReferenceType, cs->getReferenceSpaceType()))
            {
                ++res;
            }
        }
        break;
    }
    case COLORSPACE_INACTIVE:
    {
        const StringUtils::StringVec inactiveColorSpaceNames
            = getImpl()->buildInactiveColorSpaceList();
        const auto ics = inactiveColorSpaceNames.size();
        if (searchReferenceType == SEARCH_REFERENCE_SPACE_ALL)
        {
            return (int)ics;
        }
        for (auto csname : inactiveColorSpaceNames)
        {
            auto cs = getColorSpace(csname.c_str());
            if (MatchReferenceType(searchReferenceType, cs->getReferenceSpaceType()))
            {
                ++res;
            }
        }
        break;
    }
    }

    return res;
}

const char * Config::getColorSpaceNameByIndex(SearchReferenceSpaceType searchReferenceType,
                                              ColorSpaceVisibility visibility, int index) const
{
    if (index < 0)
    {
        return "";
    }

    int current = 0;
    switch (visibility)
    {
    case COLORSPACE_ALL:
    {
        if (searchReferenceType == SEARCH_REFERENCE_SPACE_ALL)
        {
            if (index < getImpl()->m_allColorSpaces->getNumColorSpaces())
            {
                return getImpl()->m_allColorSpaces->getColorSpaceNameByIndex(index);
            }
            else
            {
                return "";
            }
        }
        const int nbCS = getImpl()->m_allColorSpaces->getNumColorSpaces();
        for (int i = 0; i < nbCS; ++i)
        {
            auto cs = getImpl()->m_allColorSpaces->getColorSpaceByIndex(i);
            if (MatchReferenceType(searchReferenceType, cs->getReferenceSpaceType()))
            {
                if (current == index)
                {
                    return cs->getName();
                }
                ++current;
            }
        }
        break;
    }
    case COLORSPACE_ACTIVE:
    {
        if (searchReferenceType == SEARCH_REFERENCE_SPACE_ALL)
        {
            if (index < (int)getImpl()->m_activeColorSpaceNames.size())
            {
                return getImpl()->m_activeColorSpaceNames[index].c_str();
            }
            else
            {
                return "";
            }
        }
        const int nbCS = (int)getImpl()->m_activeColorSpaceNames.size();
        for (int i = 0; i < nbCS; ++i)
        {
            auto csname = getImpl()->m_activeColorSpaceNames[i];
            auto cs = getColorSpace(csname.c_str());
            if (MatchReferenceType(searchReferenceType, cs->getReferenceSpaceType()))
            {
                if (current == index)
                {
                    return cs->getName();
                }
                ++current;
            }
        }
        break;
    }
    case COLORSPACE_INACTIVE:
    {
        // TODO: Building the list at each call could be an issue when called in loops.
        const StringUtils::StringVec inactiveColorSpaceNames =
            getImpl()->buildInactiveColorSpaceList();
        if (searchReferenceType == SEARCH_REFERENCE_SPACE_ALL)
        {
            if (index < (int)inactiveColorSpaceNames.size())
            {
                return inactiveColorSpaceNames[index].c_str();
            }
            else
            {
                return "";
            }
        }
        const int nbCS = (int)inactiveColorSpaceNames.size();
        for (int i = 0; i < nbCS; ++i)
        {
            auto csname = inactiveColorSpaceNames[i];
            auto cs = getColorSpace(csname.c_str());
            if (MatchReferenceType(searchReferenceType, cs->getReferenceSpaceType()))
            {
                if (current == index)
                {
                    return cs->getName();
                }
                ++current;
            }
        }
        break;
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
    return getNumColorSpaces(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ACTIVE);
}

const char * Config::getColorSpaceNameByIndex(int index) const
{
    return getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ACTIVE, index);
}

int Config::getIndexForColorSpace(const char * name) const
{
    ConstColorSpaceRcPtr cs = getColorSpace(name);
    if (!cs)
    {
        return -1;
    }

    // Check to see if the name is an active color space.
    for (int idx = 0; idx < getNumColorSpaces(SEARCH_REFERENCE_SPACE_ALL,
                                              COLORSPACE_ACTIVE); ++idx)
    {
        if (std::string(getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL,
                                                 COLORSPACE_ACTIVE, idx))
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

bool Config::isColorSpaceUsed(const char * name) const noexcept
{
    // Check if a color space is used somewhere in the config other than where it is defined,
    // for example, in a display/view, look, or ColorSpaceTransform.  If the color space is
    // defined in the config, but not used elsewhere, this function returns false.

    if (!name || !*name) return false;

    // Check for all color spaces, looks and view transforms.

    ConstTransformVec allTransforms;
    getImpl()->getAllInternalTransforms(allTransforms);

    std::set<std::string> colorSpaceNames;
    for (const auto & transform : allTransforms)
    {
        ConstContextRcPtr context = getCurrentContext();
        GetColorSpaceReferences(colorSpaceNames, transform, context);
    }

    for (const auto & csName : colorSpaceNames)
    {
        if (0 == Platform::Strcasecmp(name, csName.c_str()))
        {
            return true;
        }
    }

    // Check for roles.

    const int numRoles = getNumRoles();
    for (int idx = 0; idx < numRoles; ++idx)
    {
        const char * roleName = getRoleName(idx);
        const char * csName = LookupRole(getImpl()->m_roles, roleName);
        if (0 == Platform::Strcasecmp(csName, name))
        {
            return true;
        }
    }

    // Check for all (display, view) pairs (i.e. active and inactive ones).

    for (const auto & display : getImpl()->m_displays)
    {
        const char * dispName = display.first.c_str();
        for (const auto & view : display.second)
        {
            const char * viewName = view.m_name.c_str();
            const char * csName = getDisplayColorSpaceName(dispName, viewName);
            if (0 == Platform::Strcasecmp(csName, name))
            {
                return true;
            }
        }
    }

    // Check for 'process_space' from look.

    const int numLooks = getNumLooks();
    for (int idx = 0; idx < numLooks; ++idx)
    {
        const char * lookName = getLookNameByIndex(idx);

        ConstLookRcPtr l = getLook(lookName);
        if (0 == Platform::Strcasecmp(l->getProcessSpace(), name))
        {
            return true;
        }
    }      

    // Check the file rules.

    ConstFileRulesRcPtr rules = getFileRules();

    const size_t numRules = rules->getNumEntries();
    for (size_t idx = 0; idx < numRules; ++idx)
    {
        const char * csName = rules->getColorSpace(idx);
        if (0 == Platform::Strcasecmp(csName, name))
        {
            return true;
        }
    }

    return false;
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
            const int csindex = getImpl()->getColorSpaceIndex(csname);
            if (-1 != csindex)
            {
                // This is necessary to not return a reference to a local variable.
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
        getImpl()->m_roles[StringUtils::Lower(role)] = std::string(colorSpaceName);
    }
    // Unset the role
    else
    {
        StringMap::iterator iter = getImpl()->m_roles.find(StringUtils::Lower(role));
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

const char * Config::getRoleColorSpace(int index) const
{
    return LookupRole(getImpl()->m_roles, getRoleName(index));
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

    if(index>=0 && index < static_cast<int>(getImpl()->m_displayCache.size()))
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

    StringUtils::StringVec masterViews;
    for(unsigned int i=0; i<views.size(); ++i)
    {
        masterViews.push_back(views[i].m_name);
    }

    if(!getImpl()->m_activeViewsEnvOverride.empty())
    {
        const StringUtils::StringVec orderedViews
            = IntersectStringVecsCaseIgnore(getImpl()->m_activeViewsEnvOverride, masterViews);

        if(!orderedViews.empty())
        {
            return static_cast<int>(orderedViews.size());
        }
    }
    else if(!getImpl()->m_activeViews.empty())
    {
        const StringUtils::StringVec orderedViews
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

    StringUtils::StringVec masterViews;
    for(unsigned int i = 0; i < views.size(); ++i)
    {
        masterViews.push_back(views[i].m_name);
    }

    int idx = index;

    if(!getImpl()->m_activeViewsEnvOverride.empty())
    {
        const StringUtils::StringVec orderedViews
            = IntersectStringVecsCaseIgnore(getImpl()->m_activeViewsEnvOverride, masterViews);

        if(!orderedViews.empty())
        {
            idx = FindInStringVecCaseIgnore(masterViews, orderedViews[index]);
        }
    }
    else if(!getImpl()->m_activeViews.empty())
    {
        const StringUtils::StringVec orderedViews
            = IntersectStringVecsCaseIgnore(getImpl()->m_activeViews, masterViews);

        if(!orderedViews.empty())
        {
            idx = FindInStringVecCaseIgnore(masterViews, orderedViews[index]);
        }
    }

    if(idx >= 0)
    {
        return views[idx].m_name.c_str();
    }

    if(!views.empty())
    {
        return views[0].m_name.c_str();
    }

    return "";
}

const char * Config::getDisplayViewTransformName(const char * display, const char * view) const
{
    if (!display || !view) return "";

    DisplayMap::const_iterator iter = find_display_const(getImpl()->m_displays, display);
    if (iter == getImpl()->m_displays.end()) return "";

    const ViewVec & views = iter->second;
    const int index = find_view(views, view);
    if (index<0) return "";

    return views[index].m_viewTransform.c_str();
}

const char * Config::getDisplayColorSpaceName(const char * display, const char * view) const
{
    if(!display || !view) return "";

    DisplayMap::const_iterator iter = find_display_const(getImpl()->m_displays, display);
    if(iter == getImpl()->m_displays.end()) return "";

    const ViewVec & views = iter->second;
    const int index = find_view(views, view);
    if(index<0) return "";

    return views[index].m_colorspace.c_str();
}

const char * Config::getDisplayLooks(const char * display, const char * view) const
{
    if(!display || !view) return "";

    DisplayMap::const_iterator iter = find_display_const(getImpl()->m_displays, display);
    if(iter == getImpl()->m_displays.end()) return "";

    const ViewVec & views = iter->second;
    const int index = find_view(views, view);
    if(index<0) return "";

    return views[index].m_looks.c_str();
}


void Config::addDisplay(const char * display, const char * view,
                        const char * colorSpaceName, const char * looks)
{
    addDisplay(display, view, nullptr, colorSpaceName, looks);
}

void Config::addDisplay(const char * display, const char * view, const char * viewTransform,
                        const char * displayColorSpaceName, const char * looks)
{
    if (!display || !view || !displayColorSpaceName) return;

    AddDisplay(getImpl()->m_displays, display, view, viewTransform, displayColorSpaceName, looks);
    getImpl()->m_displayCache.clear();

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

void Config::removeDisplay(const char * display, const char * view)
{
    if(!display || !view) return;

    const std::string displayNameRef(display);
    const std::string viewNameRef(view);

    // Check if the display exists.

    DisplayMap::iterator iter = find_display(getImpl()->m_displays, display);
    if (iter==getImpl()->m_displays.end())
    {
        return;
    }

    // Check if the display needs to be removed also.

    const bool removeDisplay = iter->second.size()<=1;
    if (removeDisplay)
    {
        if (iter->second.size()==1 && !StrEqualsCaseIgnore(viewNameRef, iter->second[0].m_name))
        {
            // The display does not have the view.
            return;
        }
    }

    // Remove the display/view pair.

    if (removeDisplay)
    {
        getImpl()->m_displays.erase(iter);
    }
    else
    {
        ViewVec & views = iter->second;

        views.erase(std::remove_if(views.begin(),
                                   views.end(),
                                   [viewNameRef](const View & view)
                                   {
                                        return StrEqualsCaseIgnore(view.m_name, viewNameRef);
                                   } ),
                    views.end());
    }

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
    const std::string namelower = StringUtils::Lower(name);

    for(unsigned int i=0; i<getImpl()->m_looksList.size(); ++i)
    {
        if(StringUtils::Lower(getImpl()->m_looksList[i]->getName()) == namelower)
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
    const std::string name = look->getName();
    if(name.empty())
        throw Exception("Cannot addLook with an empty name.");

    const std::string namelower = StringUtils::Lower(name);

    // If the look exists, replace it
    for(unsigned int i=0; i<getImpl()->m_looksList.size(); ++i)
    {
        if(StringUtils::Lower(getImpl()->m_looksList[i]->getName()) == namelower)
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

int Config::getNumViewTransforms() const noexcept
{
    return static_cast<int>(getImpl()->m_viewTransforms.size());
}

ConstViewTransformRcPtr Config::getViewTransform(const char * name) const noexcept
{
    const std::string namelower = StringUtils::Lower(name);

    for (auto vt : getImpl()->m_viewTransforms)
    {
        if (StringUtils::Lower(vt->getName()) == namelower)
        {
            return vt;
        }
    }

    return ConstViewTransformRcPtr();
}

const char * Config::getViewTransformNameByIndex(int index) const noexcept
{
    if (index<0 || index >= static_cast<int>(getImpl()->m_viewTransforms.size()))
    {
        return "";
    }

    return getImpl()->m_viewTransforms[index]->getName();
}

ConstViewTransformRcPtr Config::getDefaultSceneToDisplayViewTransform() const
{
    // The default view transform between the main reference space (scene-referred) and the
    // display-referred space is the first one in the list that uses a scene-referred
    // reference space.
    for (const auto & viewTransform : getImpl()->m_viewTransforms)
    {
        if (viewTransform->getReferenceSpaceType() == REFERENCE_SPACE_SCENE)
        {
            return viewTransform;
        }
    }
    return ConstViewTransformRcPtr();
}

void Config::addViewTransform(const ConstViewTransformRcPtr & viewTransform)
{
    const std::string name = viewTransform->getName();
    if (name.empty())
    {
        throw Exception("Cannot add view transform with an empty name.");
    }

    if (!viewTransform->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE) &&
        !viewTransform->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE))
    {
        throw Exception("Cannot add view transform with no transform.");
    }

    const std::string namelower = StringUtils::Lower(name);

    bool addIt = true;

    // If the view transform exists, replace it.
    for (auto && vt : getImpl()->m_viewTransforms)
    {
        if (StringUtils::Lower(vt->getName()) == namelower)
        {
            vt = viewTransform->createEditableCopy();
            addIt = false;
            break;
        }
    }

    // Otherwise, add it.
    if (addIt)
    {
        getImpl()->m_viewTransforms.push_back(viewTransform->createEditableCopy());
    }

    AutoMutex lock(getImpl()->m_cacheidMutex);
    getImpl()->resetCacheIDs();
}

void Config::clearViewTransforms()
{
    getImpl()->m_viewTransforms.clear();

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

const char * Config::getColorSpaceFromFilepath(const char * filePath) const
{
    return getImpl()->m_fileRules->getImpl()->getColorSpaceFromFilepath(*this, filePath);
}

const char * Config::getColorSpaceFromFilepath(const char * filePath, size_t & ruleIndex) const
{
    return getImpl()->m_fileRules->getImpl()->getColorSpaceFromFilepath(*this, filePath, ruleIndex);
}

bool Config::filepathOnlyMatchesDefaultRule(const char * filePath) const
{
    return getImpl()->m_fileRules->getImpl()->filepathOnlyMatchesDefaultRule(*this, filePath);
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
        os << "Could not find source color space '" << srcName << "'.";
        throw Exception(os.str().c_str());
    }

    ConstColorSpaceRcPtr dst = getColorSpace(dstName);
    if(!dst)
    {
        std::ostringstream os;
        os << "Could not find destination color space '" << dstName << "'.";
        throw Exception(os.str().c_str());
    }

    return getProcessor(context, src, dst);
}


ConstProcessorRcPtr Config::getProcessor(const char * inputColorSpaceName,
                                         const char * display, const char * view) const
{
    ConstContextRcPtr context = getCurrentContext();
    return getProcessor(context, inputColorSpaceName, display, view);
}

ConstProcessorRcPtr Config::getProcessor(const ConstContextRcPtr & context,
                                         const char * inputColorSpaceName,
                                         const char * display, const char * view) const
{
    auto dt = DisplayTransform::Create();
    dt->setInputColorSpaceName(inputColorSpaceName);
    dt->setDisplay(display);
    dt->setView(view);
    dt->validate();
    return getProcessor(context, dt, TRANSFORM_DIR_FORWARD);
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

ConstProcessorRcPtr Config::GetProcessor(const ConstConfigRcPtr & srcConfig,
                                         const char * srcName,
                                         const ConstConfigRcPtr & dstConfig,
                                         const char * dstName)
{
    return GetProcessor(srcConfig->getCurrentContext(), srcConfig, srcName,
                        dstConfig->getCurrentContext(), dstConfig, dstName);
}

ConstProcessorRcPtr Config::GetProcessor(const ConstContextRcPtr & srcContext,
                                         const ConstConfigRcPtr & srcConfig,
                                         const char * srcName,
                                         const ConstContextRcPtr & dstContext,
                                         const ConstConfigRcPtr & dstConfig,
                                         const char * dstName)
{
    ConstColorSpaceRcPtr srcColorSpace = srcConfig->getColorSpace(srcName);
    if (!srcColorSpace)
    {
        std::ostringstream os;
        os << "Could not find source color space '" << srcName << "'.";
        throw Exception(os.str().c_str());
    }

    constexpr char exchangeSceneName[]{ "aces_interchange" };
    constexpr char exchangeDisplayName[]{ "cie_xyz_d65_interchange" };
    const bool sceneReferred = (srcColorSpace->getReferenceSpaceType() == REFERENCE_SPACE_SCENE);
    const char * exchangeRoleName = sceneReferred ? exchangeSceneName : exchangeDisplayName;
    const char * srcExName = LookupRole(srcConfig->getImpl()->m_roles, exchangeRoleName);
    if (!srcExName || !*srcExName)
    {
        std::ostringstream os;
        os << "The role '" << exchangeRoleName << "' is missing in the source config.";
        throw Exception(os.str().c_str());
    }
    ConstColorSpaceRcPtr srcExCs = srcConfig->getImpl()->m_allColorSpaces->getColorSpace(srcExName);
    if (!srcExCs)
    {
        std::ostringstream os;
        os << "The role '" << exchangeRoleName << "' refers to color space '" << srcExName;
        os << "' that is missing in the source config.";
        throw Exception(os.str().c_str());
    }

    const char * dstExName = LookupRole(dstConfig->getImpl()->m_roles, exchangeRoleName);
    if (!dstExName || !*dstExName)
    {
        std::ostringstream os;
        os << "The role '" << exchangeRoleName << "' is missing in the destination config.";
        throw Exception(os.str().c_str());
    }
    ConstColorSpaceRcPtr dstExCs = dstConfig->getImpl()->m_allColorSpaces->getColorSpace(dstExName);
    if (!dstExCs)
    {
        std::ostringstream os;
        os << "The role '" << exchangeRoleName << "' refers to color space '" << dstExName;
        os << "' that is missing in the destination config.";
        throw Exception(os.str().c_str());
    }

    return GetProcessor(srcContext, srcConfig, srcName, srcExName,
                        dstContext, dstConfig, dstName, dstExName);
}

ConstProcessorRcPtr Config::GetProcessor(const ConstConfigRcPtr & srcConfig,
                                         const char * srcName,
                                         const char * srcInterchangeName,
                                         const ConstConfigRcPtr & dstConfig,
                                         const char * dstName,
                                         const char * dstInterchangeName)
{
    return GetProcessor(srcConfig->getCurrentContext(), srcConfig, srcName, srcInterchangeName,
                        dstConfig->getCurrentContext(), dstConfig, dstName, dstInterchangeName);
}

ConstProcessorRcPtr Config::GetProcessor(const ConstContextRcPtr & srcContext,
                                         const ConstConfigRcPtr & srcConfig,
                                         const char * srcName,
                                         const char * srcInterchangeName,
                                         const ConstContextRcPtr & dstContext,
                                         const ConstConfigRcPtr & dstConfig,
                                         const char * dstName,
                                         const char * dstInterchangeName)
{
    ConstColorSpaceRcPtr srcColorSpace = srcConfig->getColorSpace(srcName);
    if (!srcColorSpace)
    {
        std::ostringstream os;
        os << "Could not find source color space '" << srcName << "'.";
        throw Exception(os.str().c_str());
    }

    ConstColorSpaceRcPtr srcExCs = srcConfig->getColorSpace(srcInterchangeName);
    if (!srcExCs)
    {
        std::ostringstream os;
        os << "Could not find source interchange color space '" << srcInterchangeName << "'.";
        throw Exception(os.str().c_str());
    }

    ConstColorSpaceRcPtr dstColorSpace = dstConfig->getColorSpace(dstName);
    if (!dstColorSpace)
    {
        std::ostringstream os;
        os << "Could not find destination color space '" << dstName << "'.";
        throw Exception(os.str().c_str());
    }

    ConstColorSpaceRcPtr dstExCs = dstConfig->getColorSpace(dstInterchangeName);
    if (!dstExCs)
    {
        std::ostringstream os;
        os << "Could not find destination interchange color space '" << dstInterchangeName << "'.";
        throw Exception(os.str().c_str());
    }

    auto p1 = srcConfig->getProcessor(srcContext, srcColorSpace, srcExCs);
    if (!p1)
    {
        throw Exception("Can't create the processor for the source config and "
            "the source color space.");
    }

    auto p2 = dstConfig->getProcessor(dstContext, dstExCs, dstColorSpace);
    if (!p1)
    {
        throw Exception("Can't create the processor for the destination config "
            "and the destination color space.");
    }

    ProcessorRcPtr processor = Processor::Create();
    processor->getImpl()->concatenate(p1, p2);
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
    std::string contextcacheid;
    if(context) contextcacheid = context->getCacheID();

    StringMap::const_iterator cacheiditer = getImpl()->m_cacheids.find(contextcacheid);
    if(cacheiditer != getImpl()->m_cacheids.end())
    {
        return cacheiditer->second.c_str();
    }

    // Include the hash of the yaml config serialization
    if(getImpl()->m_cacheidnocontext.empty())
    {
        std::ostringstream cacheid;
        serialize(cacheid);
        const std::string fullstr = cacheid.str();
        getImpl()->m_cacheidnocontext = CacheIDHash(fullstr.c_str(), (int)fullstr.size());
    }

    // Also include all file references, using the context (if specified)
    std::string fileReferencesFashHash;
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
                const std::string resolvedLocation = context->resolveFileLocation(iter->c_str());
                filehash << GetFastFileHash(resolvedLocation) << " ";
            }
            catch(...)
            {
                filehash << "? ";
                continue;
            }
        }

        const std::string fullstr = filehash.str();
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
        getImpl()->checkVersionConsistency();

        OCIOYaml::Write(os, *this);
    }
    catch (const std::exception & e)
    {
        std::ostringstream error;
        error << "Error building YAML: " << e.what();
        throw Exception(error.str().c_str());
    }
}


///////////////////////////////////////////////////////////////////////////
//  Config::Impl

StringUtils::StringVec Config::Impl::buildInactiveColorSpaceList() const
{
    StringUtils::StringVec inactiveColorSpaces;

    // An API request always supersedes the other lists.
    if (!m_inactiveColorSpaceNamesAPI.empty())
    {
        inactiveColorSpaces = StringUtils::Split(m_inactiveColorSpaceNamesAPI, ',');
    }
    // The env. variable only supersedes the config list.
    else if (!m_inactiveColorSpaceNamesEnv.empty())
    {
        inactiveColorSpaces = StringUtils::Split(m_inactiveColorSpaceNamesEnv, ',');
    }
    else if (!m_inactiveColorSpaceNamesConf.empty())
    {
        inactiveColorSpaces = StringUtils::Split(m_inactiveColorSpaceNamesConf, ',');
    }

    for (auto & v : inactiveColorSpaces)
    {
        v = StringUtils::Trim(v);
    }

    return inactiveColorSpaces;
}

void Config::Impl::refreshActiveColorSpaces()
{
    m_activeColorSpaceNames.clear();

    const StringUtils::StringVec inactiveColorSpaces = buildInactiveColorSpaceList();

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
    // Grab all transforms from the ColorSpaces.

    for (int i=0; i<m_allColorSpaces->getNumColorSpaces(); ++i)
    {
        ConstTransformRcPtr tr
            = m_allColorSpaces->getColorSpaceByIndex(i)->getTransform(COLORSPACE_DIR_TO_REFERENCE);
        if (tr)
        {
            transformVec.push_back(tr);
        }

        tr = m_allColorSpaces->getColorSpaceByIndex(i)->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if (tr)
        {
            transformVec.push_back(tr);
        }
    }

    // Grab all transforms from the Looks.

    for (const auto & look : m_looksList)
    {
        ConstTransformRcPtr tr = look->getTransform();
        if (tr)
        {
            transformVec.push_back(tr);
        }

        tr = look->getInverseTransform();
        if (tr)
        {
            transformVec.push_back(tr);
        }
    }

    // Grab all transforms from the view transforms.

    for (const auto & vt : m_viewTransforms)
    {
        ConstTransformRcPtr tr = vt->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE);
        if (tr)
        {
            transformVec.push_back(tr);
        }

        tr = vt->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE);
        if (tr)
        {
            transformVec.push_back(tr);
        }
    }
}

ConstConfigRcPtr Config::Impl::Read(std::istream & istream, const char * filename)
{
    ConfigRcPtr config = Config::Create();
    OCIOYaml::Read(istream, config, filename);

    config->getImpl()->checkVersionConsistency();

    // An API request always supersedes the env. variable. As the OCIOYaml helper methods
    // use the Config public API, the variable reset highlights that only the
    // env. variable and the config contents are valid after a config file read.
    config->getImpl()->m_inactiveColorSpaceNamesAPI.clear();
    config->getImpl()->refreshActiveColorSpaces();

    return config;
}

void Config::Impl::checkVersionConsistency(ConstTransformRcPtr & transform) const
{
    if (transform)
    {
        if (ConstExponentTransformRcPtr ex = DynamicPtrCast<const ExponentTransform>(transform))
        {
            if (m_majorVersion < 2 && ex->getNegativeStyle() != NEGATIVE_CLAMP)
            {
                throw Exception("Config version 1 only supports ExponentTransform clamping negative values.");
            }
        }
        else if (DynamicPtrCast<const ExponentWithLinearTransform>(transform))
        {
            if (m_majorVersion < 2)
            {
                throw Exception("Only config version 2 (or higher) can have ExponentWithLinearTransform.");
            }
        }
        else if (DynamicPtrCast<const ExposureContrastTransform>(transform))
        {
            if (m_majorVersion < 2)
            {
                throw Exception("Only config version 2 (or higher) can have ExposureContrastTransform.");
            }
        }
        else if (DynamicPtrCast<const FixedFunctionTransform>(transform))
        {
            if (m_majorVersion < 2)
            {
                throw Exception("Only config version 2 (or higher) can have FixedFunctionTransform.");
            }
        }
        else if (DynamicPtrCast<const LogAffineTransform>(transform))
        {
            if (m_majorVersion < 2)
            {
                throw Exception("Only config version 2 (or higher) can have LogAffineTransform.");
            }
        }
        else if (DynamicPtrCast<const LogCameraTransform>(transform))
        {
            if (m_majorVersion < 2)
            {
                throw Exception("Only config version 2 (or higher) can have LogCameraTransform.");
            }
        }
        else if (DynamicPtrCast<const RangeTransform>(transform))
        {
            if (m_majorVersion < 2)
            {
                throw Exception("Only config version 2 (or higher) can have RangeTransform.");
            }
        }
        else if (ConstGroupTransformRcPtr grp = DynamicPtrCast<const GroupTransform>(transform))
        {
            const int numTransforms = grp->getNumTransforms();
            for (int idx = 0; idx < numTransforms; ++idx)
            {
                ConstTransformRcPtr tr = grp->getTransform(idx);
                checkVersionConsistency(tr);
            }
        }
    }
}

void Config::Impl::checkVersionConsistency() const
{
    // Check for the Transforms.

    ConstTransformVec transforms;
    getAllInternalTransforms(transforms);

    for (auto & transform : transforms)
    {
        checkVersionConsistency(transform);
    }

    // Check for the FileRules.

    if (m_majorVersion < 2 && m_fileRules->getNumEntries() > 1)
    {
        throw Exception("Only version 2 (or higher) can have FileRules.");
    }

    // Check for the DisplayColorSpaces.

    if (m_majorVersion < 2)
    {
        const int nbCS = m_allColorSpaces->getNumColorSpaces();
        for (int i = 0; i < nbCS; ++i)
        {
            const auto & cs = m_allColorSpaces->getColorSpaceByIndex(i);
            if (MatchReferenceType(SEARCH_REFERENCE_SPACE_DISPLAY, cs->getReferenceSpaceType()))
            {
                throw Exception("Only version 2 (or higher) can have DisplayColorSpaces.");
            }
        }
    }

    // Check for the ViewTransforms.

    if (m_majorVersion < 2 && m_viewTransforms.size() != 0)
    {
        throw Exception("Only version 2 (or higher) can have ViewTransforms.");
    }

}

} // namespace OCIO_NAMESPACE

