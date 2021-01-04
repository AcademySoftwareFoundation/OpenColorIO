// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <map>
#include <mutex>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ColorSpaceHelpers.h"
#include "Platform.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{

ConstColorSpaceInfoRcPtr ColorSpaceInfo::Create(const ConstConfigRcPtr & config,
                                                const ColorSpace & cs)
{
    return ConstColorSpaceInfoRcPtr(new ColorSpaceInfo(config, cs.getName(), nullptr,
                                                       cs.getFamily(),
                                                       cs.getDescription()),
                                    &ColorSpaceInfo::Deleter);
}

ConstColorSpaceInfoRcPtr ColorSpaceInfo::Create(const ConstConfigRcPtr & config,
                                                const NamedTransform & nt)
{
    return ConstColorSpaceInfoRcPtr(new ColorSpaceInfo(config, nt.getName(), nullptr,
                                                       nt.getFamily(),
                                                       nt.getDescription()),
                                    &ColorSpaceInfo::Deleter);
}

ConstColorSpaceInfoRcPtr ColorSpaceInfo::Create(const ConstConfigRcPtr & config,
                                                const char * name,
                                                const char * family,
                                                const char * description)
{
    return ColorSpaceInfo::Create(config, name, nullptr, family, description);
}

ConstColorSpaceInfoRcPtr ColorSpaceInfo::Create(const ConstConfigRcPtr & config,
                                                const char * name,
                                                const char * uiName,
                                                const char * family,
                                                const char * description)
{
    return ConstColorSpaceInfoRcPtr(new ColorSpaceInfo(config, name, uiName, family,
                                                       description),
                                    &ColorSpaceInfo::Deleter);
}

ConstColorSpaceInfoRcPtr ColorSpaceInfo::CreateFromRole(const ConstConfigRcPtr & config,
                                                        const char * role,
                                                        const char * family)
{
    if (config->hasRole(role))
    {
        ConstColorSpaceRcPtr cs = config->getColorSpace(role);
        std::ostringstream uiName;
        uiName << role << " (" << cs->getName() << ")";

        return ColorSpaceInfo::Create(config,
                                      role, // use role name
                                      uiName.str().c_str(),
                                      family,
                                      nullptr);
    }
    return ConstColorSpaceInfoRcPtr();
}

ConstColorSpaceInfoRcPtr ColorSpaceInfo::CreateFromSingleRole(const ConstConfigRcPtr & config,
                                                              const char * role)
{
    if (config->hasRole(role))
    {
        ConstColorSpaceRcPtr cs = config->getColorSpace(role);
        std::ostringstream uiName;
        uiName << role << " (" << cs->getName() << ")";

        return ColorSpaceInfo::Create(config,
                                      cs->getName(), // use color space name
                                      uiName.str().c_str(),
                                      nullptr,
                                      nullptr);
    }
    return ConstColorSpaceInfoRcPtr();
}

void ColorSpaceInfo::Deleter(ColorSpaceInfo * cs)
{
    delete (ColorSpaceInfo*)cs;
}

ColorSpaceInfo::ColorSpaceInfo(const ConstConfigRcPtr & config,
                               const char * name,
                               const char * uiName,
                               const char * family,
                               const char * description)
    :   m_name(name ? name : "")
    ,   m_uiName((uiName && *uiName) ? uiName : m_name)
    ,   m_family(family ? family : "")
    ,   m_description(description ? description : "")
{
    StringUtils::StringVec vals;
    if (config->getFamilySeparator() && !m_family.empty())
    {
        vals = StringUtils::Split(m_family, config->getFamilySeparator());
    }
    else
    {
        vals.push_back(m_family);
    }

    for (const auto & val : vals)
    {
        auto v = StringUtils::Trim(val);
        if (!v.empty())
        {
            m_hierarchyLevels.push_back(v);
        }
    }
}

const char * ColorSpaceInfo::getName() const noexcept
{
    return m_name.c_str();
}

const char * ColorSpaceInfo::getUIName() const noexcept
{
    return m_uiName.c_str();
}

const char * ColorSpaceInfo::getFamily() const noexcept
{
    return m_family.c_str();
}

const char * ColorSpaceInfo::getDescription() const noexcept
{
    return m_description.c_str();
}

size_t ColorSpaceInfo::getNumHierarchyLevels() const noexcept
{
    return m_hierarchyLevels.size();
}

const char * ColorSpaceInfo::getHierarchyLevel(size_t i) const noexcept
{
    if (i < m_hierarchyLevels.size())
    {
        return m_hierarchyLevels[i].c_str();
    }
    return "";
}

ColorSpaceMenuParametersRcPtr ColorSpaceMenuParameters::Create(ConstConfigRcPtr config)
{
    return std::shared_ptr<ColorSpaceMenuParameters>(new ColorSpaceMenuParametersImpl(config),
                                                     &ColorSpaceMenuParametersImpl::Deleter);
}

ColorSpaceMenuParametersImpl::ColorSpaceMenuParametersImpl(ConstConfigRcPtr config)
    : m_config(config)
{
}

void ColorSpaceMenuParametersImpl::setParameters(ConstColorSpaceMenuParametersRcPtr parameters)
{
    const auto & impl = dynamic_cast<const ColorSpaceMenuParametersImpl &>(*parameters);
    m_config                 = impl.m_config;
    m_role                   = impl.m_role;
    m_appCategories          = impl.m_appCategories;
    m_userCategories         = impl.m_userCategories;
    m_encodings              = impl.m_encodings;
    m_includeColorSpaces     = impl.m_includeColorSpaces;
    m_includeRoles           = impl.m_includeRoles;
    m_includeNamedTransforms = impl.m_includeNamedTransforms;
    m_colorSpaceType         = impl.m_colorSpaceType;
    m_additionalColorSpaces  = impl.m_additionalColorSpaces;
}

void ColorSpaceMenuParametersImpl::setConfig(ConstConfigRcPtr config) noexcept
{
    m_config = config;
}

ConstConfigRcPtr ColorSpaceMenuParametersImpl::getConfig() const noexcept
{
    return m_config;
}

void ColorSpaceMenuParametersImpl::setRole(const char * role) noexcept
{
    m_role = role ? role : "";
}

const char * ColorSpaceMenuParametersImpl::getRole() const noexcept
{
    return m_role.c_str();
}

void ColorSpaceMenuParametersImpl::setAppCategories(const char * appCategories) noexcept
{
    m_appCategories = appCategories ? appCategories : "";
}

const char * ColorSpaceMenuParametersImpl::getAppCategories() const noexcept
{
    return m_appCategories.c_str();
}

void ColorSpaceMenuParametersImpl::setUserCategories(const char * userCategories) noexcept
{
    m_userCategories = userCategories ? userCategories : "";
}

const char * ColorSpaceMenuParametersImpl::getUserCategories() const noexcept
{
    return m_userCategories.c_str();
}

void ColorSpaceMenuParametersImpl::setEncodings(const char * encodings) noexcept
{
    m_encodings = encodings ? encodings : "";
}

const char * ColorSpaceMenuParametersImpl::getEncodings() const noexcept
{
    return m_encodings.c_str();
}

void ColorSpaceMenuParametersImpl::setIncludeColorSpaces(bool include) noexcept
{
    m_includeColorSpaces = include;
}

bool ColorSpaceMenuParametersImpl::getIncludeColorSpaces() const noexcept
{
    return m_includeColorSpaces;
}

void ColorSpaceMenuParametersImpl::setIncludeRoles(bool include) noexcept
{
    m_includeRoles = include;
}

bool ColorSpaceMenuParametersImpl::getIncludeRoles() const noexcept
{
    return m_includeRoles;
}

void ColorSpaceMenuParametersImpl::setIncludeNamedTransforms(bool include) noexcept
{
    m_includeNamedTransforms = include;
}

bool ColorSpaceMenuParametersImpl::getIncludeNamedTransforms() const noexcept
{
    return m_includeNamedTransforms;
}

void ColorSpaceMenuParametersImpl::addColorSpace(const char * name) noexcept
{
    if (name && *name && !StringUtils::Contain(m_additionalColorSpaces, name))
    {
        m_additionalColorSpaces.push_back(name);
    }
}

size_t ColorSpaceMenuParametersImpl::getNumAddedColorSpaces() const noexcept
{
    return m_additionalColorSpaces.size();
}

const char * ColorSpaceMenuParametersImpl::getAddedColorSpace(size_t index) const noexcept
{
    if (index < m_additionalColorSpaces.size())
    {
        return m_additionalColorSpaces[index].c_str();
    }
    return "";
}

void ColorSpaceMenuParametersImpl::clearAddedColorSpaces() noexcept
{
    m_additionalColorSpaces.clear();
}

SearchReferenceSpaceType ColorSpaceMenuParametersImpl::getSearchReferenceSpaceType() const noexcept
{
    return m_colorSpaceType;
}

void ColorSpaceMenuParametersImpl::setSearchReferenceSpaceType(SearchReferenceSpaceType colorSpaceType) noexcept
{
    m_colorSpaceType = colorSpaceType;
}

void ColorSpaceMenuParametersImpl::Deleter(ColorSpaceMenuParameters * p)
{
    delete (ColorSpaceMenuParametersImpl*)p;
}

std::ostream & operator<<(std::ostream & os, const ColorSpaceMenuParameters & p)
{
    const auto & impl = dynamic_cast<const ColorSpaceMenuParametersImpl &>(p);

    os << "config: " << (impl.m_config ? impl.m_config->getCacheID() : "missing");
    if (!impl.m_role.empty())
    {
        os << ", role: " << impl.m_role;
    }
    if (!impl.m_appCategories.empty())
    {
        os << ", appCategories: " << impl.m_appCategories;
    }
    if (!impl.m_userCategories.empty())
    {
        os << ", userCategories: " << impl.m_userCategories;
    }
    if (!impl.m_encodings.empty())
    {
        os << ", encodings: " << impl.m_encodings;
    }
    os << ", includeColorSpaces: " << (p.getIncludeColorSpaces() ? "true" : "false");
    os << ", includeRoles: " << (p.getIncludeRoles() ? "true" : "false");
    os << ", includeNamedTransforms: " << (p.getIncludeNamedTransforms() ? "true" : "false");
    if (impl.m_colorSpaceType == SEARCH_REFERENCE_SPACE_SCENE)
    {
        os << ", colorSpaceType: scene";
    }
    else if (impl.m_colorSpaceType == SEARCH_REFERENCE_SPACE_DISPLAY)
    {
        os << ", colorSpaceType: display";
    }
    const auto numcs = impl.m_additionalColorSpaces.size();
    if (numcs)
    {
        os << ", addedSpaces: ";
        if (numcs == 1)
        {
            os << impl.m_additionalColorSpaces[0];
        }
        else
        {
            os << "[" << impl.m_additionalColorSpaces[0];
            for (size_t cs = 1; cs < numcs; ++cs)
            {
                os << ", " << impl.m_additionalColorSpaces[cs];
            }
            os << "]";
        }
    }
    return os;
}

ColorSpaceMenuHelperRcPtr ColorSpaceMenuHelper::Create(ConstColorSpaceMenuParametersRcPtr p)
{
    if (!p || !p->getConfig())
    {
        throw Exception("ColorSpaceMenuHelper needs parameters with a config.");
    }

    try
    {
        p->getConfig()->validate();
    }
    catch (Exception & e)
    {
        std::ostringstream oss;
        oss << "ColorSpaceMenuHelper needs a valid config. Validation failed with: " << e.what();
        throw Exception(oss.str().c_str());
    }

    if (!p->getIncludeColorSpaces() && p->getIncludeRoles())
    {
        throw Exception("ColorSpaceMenuHelper needs to include color spaces if roles are "
                        "included.");
    }

    ConstColorSpaceMenuParametersRcPtr parameters = p;
    // User categories from the environment variable override what is specified by the application.
    std::string envUserCategories;
    if (Platform::Getenv(OCIO_USER_CATEGORIES_ENVVAR, envUserCategories))
    {
        if (!envUserCategories.empty())
        {
            auto userCategories = StringUtils::Trim(envUserCategories);
            if (!userCategories.empty())
            {
                auto newP = ColorSpaceMenuParameters::Create(parameters->getConfig());
                auto * impl = dynamic_cast<ColorSpaceMenuParametersImpl *>(newP.get());
                impl->setParameters(parameters);
                newP->setUserCategories(userCategories.c_str());
                parameters = newP;
            }
        }
    }

    // Note: Add a cache to not recreate a new menu helper for the same parameters.

    using Entries = std::map<std::size_t, ColorSpaceMenuHelperRcPtr>;

    static std::mutex g_mutex;
    static Entries g_entries;

    static const bool isCacheEnabled = !IsEnvVariablePresent("OCIO_DISABLE_ALL_CACHES");
    if (isCacheEnabled)
    {
        std::lock_guard<std::mutex> lock(g_mutex);

        std::ostringstream oss;
        oss << *parameters;

        const std::size_t key = std::hash<std::string>{}(oss.str());

        ColorSpaceMenuHelperRcPtr & entry = g_entries[key];
        if (!entry)
        {
            entry = std::shared_ptr<ColorSpaceMenuHelper>(new ColorSpaceMenuHelperImpl(parameters),
                                                          &ColorSpaceMenuHelperImpl::Deleter);
        }

        return entry;
    }

    return std::shared_ptr<ColorSpaceMenuHelper>(new ColorSpaceMenuHelperImpl(parameters),
                                                 &ColorSpaceMenuHelperImpl::Deleter);
}

void ColorSpaceMenuHelperImpl::Deleter(ColorSpaceMenuHelper * incs)
{
    delete (ColorSpaceMenuHelperImpl*)incs;
}

ColorSpaceMenuHelperImpl::ColorSpaceMenuHelperImpl(ConstColorSpaceMenuParametersRcPtr parameters)
    : ColorSpaceMenuHelper()
    , m_parameters(ConfigRcPtr())
{
    m_parameters.setParameters(parameters);
    refresh();
}

void ColorSpaceMenuHelperImpl::refresh()
{
    m_entries.clear();

    // 1) If the role exists, use only that space.

    if (!m_parameters.m_role.empty())
    {
        if (m_parameters.m_config->hasRole(m_parameters.m_role.c_str()))
        {
            m_entries.push_back(ColorSpaceInfo::CreateFromSingleRole(m_parameters.m_config,
                                                                     m_parameters.m_role.c_str()));
            return;
        }
    }

    // 2) & 3) Identify potential menu items and then filter them by category and encoding.

    const Categories allAppCategories = ExtractItems(m_parameters.m_appCategories.c_str());
    const Categories allUserCategories = ExtractItems(m_parameters.m_userCategories.c_str());
    const Encodings allEncodings = ExtractItems(m_parameters.m_encodings.c_str());
    const auto numNT = m_parameters.m_config->getNumNamedTransforms();
    const auto numCS = m_parameters.m_config->getNumColorSpaces(m_parameters.m_colorSpaceType,
                                                                COLORSPACE_ACTIVE);
    if ((m_parameters.m_includeColorSpaces && numCS != 0) ||
        (m_parameters.m_includeNamedTransforms && numNT != 0))
    {
        m_entries = FindColorSpaceInfos(m_parameters.m_config,
                                        allAppCategories, allUserCategories,
                                        m_parameters.m_includeColorSpaces,
                                        m_parameters.m_includeNamedTransforms,
                                        allEncodings, m_parameters.m_colorSpaceType);
    }

    // 4) Include roles if requested.

    if (m_parameters.m_includeRoles)
    {
        for (int idx = 0; idx < m_parameters.m_config->getNumRoles(); ++idx)
        {
            auto info = ColorSpaceInfo::CreateFromRole(m_parameters.m_config,
                                                       m_parameters.m_config->getRoleName(idx),
                                                       "Roles");
            m_entries.push_back(info);
        }
    }

    // 5) Add additional color spaces.

    Infos additionalColorSpaces;
    for (const auto & name : m_parameters.m_additionalColorSpaces)
    {
        auto cs = m_parameters.m_config->getColorSpace(name.c_str());
        if (cs)
        {
            bool addit = true;
            for (const auto & entry : m_entries)
            {
                if (StringUtils::Compare(cs->getName(), entry->getName()))
                {
                    // Already there.
                    addit = false;
                    break;
                }
            }
            for (const auto & addedname : additionalColorSpaces)
            {
                if (StringUtils::Compare(cs->getName(), addedname->getName()))
                {
                    // Already there.
                    addit = false;
                    break;
                }
            }
            if (addit)
            {
                auto csInfo = ColorSpaceInfo::Create(m_parameters.m_config, *cs);
                additionalColorSpaces.push_back(csInfo);
            }
        }
        else
        {
            auto nt = m_parameters.m_config->getNamedTransform(name.c_str());
            if (nt)
            {
                bool addit = true;
                for (const auto & entry : m_entries)
                {
                    if (StringUtils::Compare(nt->getName(), entry->getName()))
                    {
                        // Already there.
                        addit = false;
                        break;
                    }
                }
                for (const auto & addedname : additionalColorSpaces)
                {
                    if (StringUtils::Compare(nt->getName(), addedname->getName()))
                    {
                        // Already there.
                        addit = false;
                        break;
                    }
                }
                if (addit)
                {
                    auto csInfo = ColorSpaceInfo::Create(m_parameters.m_config, *nt);
                    additionalColorSpaces.push_back(csInfo);
                }
            }
            else
            {
                std::ostringstream oss;
                oss << "Element '" << name << "' is neither a color space not a named transform.";
                throw Exception(oss.str().c_str());
            }
        }
    }

    m_entries.insert(m_entries.end(),
                     additionalColorSpaces.begin(),
                     additionalColorSpaces.end());
}

size_t ColorSpaceMenuHelperImpl::getNumColorSpaces() const noexcept
{
    return m_entries.size();
}

const char * ColorSpaceMenuHelperImpl::getName(size_t idx) const noexcept
{
    if (idx < m_entries.size())
    {
        return m_entries[idx]->getName();
    }
    return "";
}

const char * ColorSpaceMenuHelperImpl::getUIName(size_t idx) const noexcept
{
    if (idx < m_entries.size())
    {
        return m_entries[idx]->getUIName();
    }
    return "";
}

size_t ColorSpaceMenuHelperImpl::getIndexFromName(const char * name) const noexcept
{
    if (name && *name)
    {
        const std::string csName(name);
        for (size_t idx = 0; idx < m_entries.size(); ++idx)
        {
            if (StringUtils::Compare(m_entries[idx]->getName(), csName))
            {
                return idx;
            }
        }
    }
    return (size_t)(-1);
}

size_t ColorSpaceMenuHelperImpl::getIndexFromUIName(const char * name) const noexcept
{
    if (name && *name)
    {
        const std::string csName(name);
        for (size_t idx = 0; idx < m_entries.size(); ++idx)
        {
            if (StringUtils::Compare(m_entries[idx]->getUIName(), csName))
            {
                return idx;
            }
        }
    }
    return (size_t)(-1);
}

const char * ColorSpaceMenuHelperImpl::getFamily(size_t idx) const noexcept
{
    if (idx < m_entries.size())
    {
        return m_entries[idx]->getFamily();
    }
    return "";
}

const char * ColorSpaceMenuHelperImpl::getDescription(size_t idx) const noexcept
{
    if (idx < m_entries.size())
    {
        return m_entries[idx]->getDescription();
    }
    return "";
}

size_t ColorSpaceMenuHelperImpl::getNumHierarchyLevels(size_t idx) const noexcept
{
    if (idx < m_entries.size())
    {
        return m_entries[idx]->getNumHierarchyLevels();
    }
    return 0;
}

const char * ColorSpaceMenuHelperImpl::getHierarchyLevel(size_t idx, size_t i) const noexcept
{
    if (idx < m_entries.size())
    {
        return m_entries[idx]->getHierarchyLevel(i);
    }
    return "";
}

const char * ColorSpaceMenuHelperImpl::getNameFromUIName(const char * uiName) const noexcept
{
    if (uiName && *uiName)
    {
        for (const auto & entry : m_entries)
        {
            if (StringUtils::Compare(uiName, entry->getUIName()))
            {
                return entry->getName();
            }
        }
    }
    return "";
}

const char * ColorSpaceMenuHelperImpl::getUINameFromName(const char * name) const noexcept
{
    if (name && *name)
    {
        for (const auto & entry : m_entries)
        {
            if (StringUtils::Compare(name, entry->getName()))
            {
                return entry->getUIName();
            }
        }
    }
    return "";
}

std::ostream & ColorSpaceMenuHelperImpl::serialize(std::ostream & os) const
{
    os << m_parameters;
    os << ", color spaces = [";
    bool first = true;
    for (const auto & cs : m_entries)
    {
        if (!first)
        {
            os << ", ";
        }
        os << cs->getName();
        first = false;
    }
    os << "]";
    return os;
}

std::ostream & operator<<(std::ostream & os, const ColorSpaceMenuHelper & menu)
{
    const auto & impl = dynamic_cast<const ColorSpaceMenuHelperImpl &>(menu);
    impl.serialize(os);
    return os;
}

namespace ColorSpaceHelpers
{

namespace
{
void AddColorSpace(ConfigRcPtr & config,
                   ColorSpaceRcPtr & colorSpace,
                   FileTransformRcPtr & userTransform,
                   const char * connectionColorSpaceName)
{
    if (!connectionColorSpaceName || !*connectionColorSpaceName)
    {
        throw Exception("Invalid connection color space name.");
    }

    // Check for a role and an active or inactive color space.
    if (config->getColorSpace(colorSpace->getName()))
    {
        std::string errMsg;
        errMsg += "Color space name '";
        errMsg += colorSpace->getName();
        errMsg += "' already exists.";

        throw Exception(errMsg.c_str());
    }

    // Step 1 - Create the color transformation.

    GroupTransformRcPtr grp =  GroupTransform::Create();
    grp->appendTransform(userTransform);

    // Check for an active or inactive color space.
    ConstColorSpaceRcPtr connectionCS = config->getColorSpace(connectionColorSpaceName);
    if (!connectionCS)
    {
        std::string errMsg;
        errMsg += "Connection color space name '";
        errMsg += connectionColorSpaceName;
        errMsg += "' does not exist.";

        throw Exception(errMsg.c_str());
    }

    ConstTransformRcPtr tr = connectionCS->getTransform(COLORSPACE_DIR_TO_REFERENCE);
    if (tr)
    {
        grp->appendTransform(tr->createEditableCopy());
    }
    else
    {
        tr = connectionCS->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if (tr)
        {
            TransformRcPtr t = tr->createEditableCopy();
            t->setDirection(CombineTransformDirections(tr->getDirection(), TRANSFORM_DIR_INVERSE));
            grp->appendTransform(t);
        }
    }

    grp->validate();

    // Step 2 - Add the color space to the config.

    colorSpace->setTransform(DynamicPtrCast<const Transform>(grp), COLORSPACE_DIR_TO_REFERENCE);
    config->addColorSpace(colorSpace);
}
}

// TODO: This function only adds a color space that uses a to_reference transform.
// May want to add support for userTransforms that go in the opposite direction.
void AddColorSpace(ConfigRcPtr & config,
                   const ColorSpaceInfo & colorSpaceInfo,
                   FileTransformRcPtr & userTransform,
                   const char * categories,
                   const char * connectionColorSpaceName)
{
    ColorSpaceRcPtr colorSpace = ColorSpace::Create();

    colorSpace->setName(colorSpaceInfo.getName());
    colorSpace->setFamily(colorSpaceInfo.getFamily());
    colorSpace->setDescription(colorSpaceInfo.getDescription());

    if (categories && *categories)
    {
        const Categories cats = ExtractItems(categories);

        // Only add the categories if already used.
        ColorSpaceNames names = FindColorSpaceNames(config, cats);
        if (!names.empty())
        {
            for (const auto & cat : cats)
            {
                colorSpace->addCategory(cat.c_str());
            }
        }
    }

    AddColorSpace(config, colorSpace, userTransform, connectionColorSpaceName);
}

void AddColorSpace(ConfigRcPtr & config,
                   const char * name,
                   const char * transformFilePath,
                   const char * categories,
                   const char * connectionColorSpaceName)
{
    ConstColorSpaceInfoRcPtr info = ColorSpaceInfo::Create(config, name, nullptr, nullptr);

    FileTransformRcPtr file = FileTransform::Create();
    file->setSrc(transformFilePath);

    AddColorSpace(config, *info, file, categories, connectionColorSpaceName);
}

} // ColorSpaceHelpers

} // namespace OCIO_NAMESPACE
