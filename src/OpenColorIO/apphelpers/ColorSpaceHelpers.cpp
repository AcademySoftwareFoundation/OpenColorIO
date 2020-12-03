// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <map>
#include <mutex>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ColorSpaceHelpers.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{
namespace Category
{
const char * INPUT = "input";
const char * SCENE_LINEAR_WORKING_SPACE = "scene_linear_working_space";
const char * LOG_WORKING_SPACE = "log_working_space";
const char * VIDEO_WORKING_SPACE = "video_working_space";
const char * LUT_INPUT_SPACE = "lut_input_space";

} // Category

ConstColorSpaceInfoRcPtr ColorSpaceInfo::Create(const ConstConfigRcPtr & config,
                                                const ConstColorSpaceRcPtr & cs)
{
    return ConstColorSpaceInfoRcPtr(new ColorSpaceInfo(config, cs->getName(), nullptr,
                                                       cs->getFamily(),
                                                       cs->getDescription()),
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
                                      role,
                                      uiName.str().c_str(),
                                      family,
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

namespace
{
ColorSpaceMenuHelperRcPtr CreateColorSpaceMenuHelper(const ConstConfigRcPtr & config,
                                                     const char * role,
                                                     const char * categories,
                                                     ColorSpaceMenuHelper::IncludeTypeFlag includeFlag)
{
    // Note: Add a cache to not recreate a new menu helper for the same config.   

    using Entries = std::map<std::size_t, ColorSpaceMenuHelperRcPtr>;

    static std::mutex g_mutex;
    static Entries g_entries;

    static const bool isCacheEnabled = !IsEnvVariablePresent("OCIO_DISABLE_ALL_CACHES");
    if (isCacheEnabled)
    {
        std::lock_guard<std::mutex> lock(g_mutex);

        std::ostringstream oss;
        oss << std::string(config->getCacheID())
            << std::string(role ? role : "")
            << std::string(categories ? categories : "")
            << includeFlag;

        const std::size_t key = std::hash<std::string>{}(oss.str());

        ColorSpaceMenuHelperRcPtr & entry = g_entries[key];
        if (!entry)
        {
            entry = std::shared_ptr<ColorSpaceMenuHelper>(
                new ColorSpaceMenuHelperImpl(config, role, categories, includeFlag),
                &ColorSpaceMenuHelperImpl::Deleter);
        }

        return entry;
    }
    else
    {
        return std::shared_ptr<ColorSpaceMenuHelper>(
            new ColorSpaceMenuHelperImpl(config, role, categories, includeFlag),
            &ColorSpaceMenuHelperImpl::Deleter);
    }
}
}

ColorSpaceMenuHelperRcPtr ColorSpaceMenuHelper::Create(const ConstConfigRcPtr & config,
                                                       const char * role,
                                                       const char * categories,
                                                       ColorSpaceMenuHelper::IncludeTypeFlag includeFlag)

{
    return CreateColorSpaceMenuHelper(config, role, categories, includeFlag);
}

void ColorSpaceMenuHelperImpl::Deleter(ColorSpaceMenuHelper * incs)
{
    delete (ColorSpaceMenuHelperImpl*)incs;
}

ColorSpaceMenuHelperImpl::ColorSpaceMenuHelperImpl(const ConstConfigRcPtr & config,
                                                   const char * role,
                                                   const char * categories,
                                                   IncludeTypeFlag includeFlag)
    :   ColorSpaceMenuHelper()
    ,   m_config(config)
    ,   m_roleName(StringUtils::Lower(role))
    ,   m_categories(StringUtils::Lower(categories))
    ,   m_includeFlag(includeFlag)
{
    refresh();
}

void ColorSpaceMenuHelperImpl::refresh(const ConstConfigRcPtr & config)
{
    m_config = config;
    refresh();
}

void ColorSpaceMenuHelperImpl::refresh()
{
    // Find all color spaces.

    m_colorSpaces = getColorSpaceInfosFromCategories(m_config, m_roleName.c_str(),
                                                     m_categories.c_str(), m_includeFlag);
    if (m_colorSpaces.empty())
    {
        std::stringstream oss;
        oss << "With role '" << m_roleName << "' and categories [" << m_categories <<"]"
            << " no color spaces were found.";

        throw Exception(oss.str().c_str());
    }

    auto previouslyAdded = m_additionalColorSpaces;
    m_additionalColorSpaces.clear();

    for (const auto & cs : previouslyAdded)
    {
        if (m_config->getColorSpace(cs->getName()))
        {
            bool addit = true;
            for (const auto & entry : m_colorSpaces)
            {
                if (StringUtils::Compare(cs->getName(), entry->getName()))
                {
                    // Already there.
                    addit = false;
                    break;
                }
            }
            if (addit)
            {
                m_additionalColorSpaces.push_back(cs);
            }
        }
    }

    // Rebuild the complete list.

    m_entries = m_colorSpaces;
    m_entries.insert(m_entries.end(),
                     m_additionalColorSpaces.begin(),
                     m_additionalColorSpaces.end());
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

void ColorSpaceMenuHelperImpl::addColorSpaceToMenu(const char * name)
{
    ConstColorSpaceRcPtr cs = m_config->getColorSpace(name);
    if (!m_config->getColorSpace(name))
    {
        std::ostringstream oss;
        oss << "Color space '" << std::string(name ? name : "") << "' does not exist.";
        throw Exception(oss.str().c_str());
    }

    for (const auto & entry : m_entries)
    {
        if (StringUtils::Compare(name, entry->getName()))
        {
            // Already there.
            return;
        }
    }

    auto csInfo = ColorSpaceInfo::Create(m_config, cs);
    m_additionalColorSpaces.push_back(csInfo);

    refresh();
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
    os << "Config: " << m_config->getCacheID();
    if (!m_roleName.empty()) os << ", role: " << m_roleName;
    if (!m_categories.empty()) os << ", categories: " << m_categories;
    os << ", includeFlag: [color spaces";
    if (HasFlag(m_includeFlag, INCLUDE_ROLES))
    {
        os << ", roles";
    }
    if (HasFlag(m_includeFlag, INCLUDE_NAMEDTRANSFORMS))
    {
        os << ", named transforms";
    }
    os << "], ";
    os << "color spaces = [";
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
        const Categories cats = ExtractCategories(categories);

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
