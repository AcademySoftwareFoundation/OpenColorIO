// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "CategoryHelpers.h"
#include "CategoryNames.h"
#include "ColorSpaceHelpers.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

class StringsImpl : public Strings
{
public:
    StringsImpl() = default;
    StringsImpl(const StringsImpl &) = delete;
    StringsImpl & operator=(const StringsImpl &) = delete;
    ~StringsImpl() override = default;

    size_t getNumString() const noexcept override;
    bool empty() const noexcept override;

    const char * getString(size_t idx) const override;

    void appendString(const char * value) override;

    void clear();

    static void Deleter(Strings * cs);

private:
    std::vector<std::string> m_values;
};

StringsRcPtr Strings::Create()
{
    return StringsRcPtr(new StringsImpl(), &StringsImpl::Deleter);
}

void StringsImpl::Deleter(Strings * cs)
{
    delete (StringsImpl*)cs;
}

size_t StringsImpl::getNumString() const noexcept
{
    return m_values.size();
}

bool StringsImpl::empty() const noexcept
{
    return m_values.empty();
}

const char * StringsImpl::getString(size_t idx) const
{
    return m_values.at(idx).c_str();
}

void StringsImpl::appendString(const char * value)
{
    return m_values.push_back(value);
}

void StringsImpl::clear()
{
    m_values.clear();
}


class ColorSpaceInfoImpl : public ColorSpaceInfo
{
public:
    ColorSpaceInfoImpl() = delete;
    ColorSpaceInfoImpl(const ColorSpaceInfoImpl &) = delete;
    ColorSpaceInfoImpl & operator=(const ColorSpaceInfoImpl &) = delete;
    ~ColorSpaceInfoImpl() override = default;

    ColorSpaceInfoImpl(ConstConfigRcPtr config,
                       const char * name,
                       const char * uiName,
                       const char * family,
                       const char * descriptione);

    const char * getName() const noexcept override;
    const char * getUIName() const noexcept override;
    const char * getFamily() const noexcept override;
    const char * getDescription() const noexcept override;

    ConstStringsRcPtr getHierarchyLevels() const noexcept override;
    ConstStringsRcPtr getDescriptions() const noexcept override;

    ColorSpaceRcPtr createColorSpace() const noexcept;

    static void Deleter(ColorSpaceInfo * cs);

private:
    const std::string m_name;
    const std::string m_uiName;
    const std::string m_family;
    const std::string m_description;

    // Extracted from the color space's family attribute to be used for a hierarchical menu.
    StringsRcPtr m_hierarchyLevels;
    // The description attribute of the color space, separated into lines.
    StringsRcPtr m_descriptionLineByLine;
};


ConstColorSpaceInfoRcPtr ColorSpaceInfo::Create(ConstConfigRcPtr config, ConstColorSpaceRcPtr cs)
{
    return ConstColorSpaceInfoRcPtr(
        new ColorSpaceInfoImpl(config, cs->getName(), nullptr, 
                               cs->getFamily(), cs->getDescription()),
        &ColorSpaceInfoImpl::Deleter);
}

ConstColorSpaceInfoRcPtr ColorSpaceInfo::Create(ConstConfigRcPtr config,
                                                const char * name,
                                                const char * family,
                                                const char * description)
{
    return ConstColorSpaceInfoRcPtr(
        new ColorSpaceInfoImpl(config, name, nullptr, family, description),
        &ColorSpaceInfoImpl::Deleter);
}

ConstColorSpaceInfoRcPtr ColorSpaceInfo::Create(ConstConfigRcPtr config,
                                                const char * name,
                                                const char * uiName,
                                                const char * family,
                                                const char * description)
{
    return ConstColorSpaceInfoRcPtr(
        new ColorSpaceInfoImpl(config, name, uiName, family, description),
        &ColorSpaceInfoImpl::Deleter);
}

void ColorSpaceInfoImpl::Deleter(ColorSpaceInfo * cs)
{
    delete (ColorSpaceInfoImpl*)cs;
}

ColorSpaceInfoImpl::ColorSpaceInfoImpl(ConstConfigRcPtr config,
                                       const char * name,
                                       const char * uiName,
                                       const char * family,
                                       const char * description)
    :   ColorSpaceInfo()
    ,   m_name(name ? name : "")
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

    m_hierarchyLevels = Strings::Create();
    for (const auto & val : vals)
    {
        auto v = StringUtils::Trim(val);
        if (!v.empty())
        {
            m_hierarchyLevels->appendString(v.c_str());
        }
    }

    m_descriptionLineByLine = Strings::Create();
    if (!m_description.empty())
    {
        vals = StringUtils::SplitByLines(m_description);

        for (const auto & val : vals)
        {
            auto v = StringUtils::Trim(val);
            // Preserve empty lines.
            m_descriptionLineByLine->appendString(v.c_str());
        }
    }
}

const char * ColorSpaceInfoImpl::getName() const noexcept
{
    return m_name.c_str();
}

const char * ColorSpaceInfoImpl::getUIName() const noexcept
{
    return m_uiName.c_str();
}

const char * ColorSpaceInfoImpl::getFamily() const noexcept
{
    return m_family.c_str();
}

const char * ColorSpaceInfoImpl::getDescription() const noexcept
{
    return m_description.c_str();
}

ConstStringsRcPtr ColorSpaceInfoImpl::getHierarchyLevels() const noexcept
{
    return m_hierarchyLevels;
}

ConstStringsRcPtr ColorSpaceInfoImpl::getDescriptions() const noexcept
{
    return m_descriptionLineByLine;
}


class MenuHelperImpl : public ColorSpaceMenuHelper
{
public:

    MenuHelperImpl() = delete;
    MenuHelperImpl & operator=(const MenuHelperImpl &) = delete;

    MenuHelperImpl(const ConstConfigRcPtr & config,
                   const char * role,          // Role name to override categories.
                   const char * categories);   // Comma-separated list of categories.

    MenuHelperImpl(const MenuHelperImpl &) = delete;
    ~MenuHelperImpl() override = default;

    size_t getNumColorSpaces() const noexcept override;
    const char * getColorSpaceName(size_t idx) const override;
    const char * getColorSpaceUIName(size_t idx) const override;
    ConstColorSpaceInfoRcPtr getColorSpace(size_t idx) const override;

    const char * getNameFromUIName(const char * uiName) const override;
    const char * getUINameFromName(const char * name) const override;

    void addColorSpaceToMenu(ConstColorSpaceInfoRcPtr & cs) override;

    void refresh(ConstConfigRcPtr config) override;
    void refreshEntries();

    static void Deleter(ColorSpaceMenuHelper * hlp);

private:
    ConstConfigRcPtr m_config;
    const std::string m_roleName;
    const std::string m_categories;

    Infos m_entries; // Contains all the color space names.

    Infos m_colorSpaces;
    Infos m_additionalColorSpaces;
};

ColorSpaceMenuHelperRcPtr ColorSpaceMenuHelper::Create(const ConstConfigRcPtr & config,
                                                       const char * role,
                                                       const char * categories)
{
    return std::shared_ptr<ColorSpaceMenuHelper>(
        new MenuHelperImpl(config, role, categories),
        &MenuHelperImpl::Deleter);
}

void MenuHelperImpl::Deleter(ColorSpaceMenuHelper * incs)
{
    delete (MenuHelperImpl*)incs;
}

MenuHelperImpl::MenuHelperImpl(const ConstConfigRcPtr & config,
                               const char * role,
                               const char * categories)
    :   ColorSpaceMenuHelper()
    ,   m_config(config)
    ,   m_roleName(StringUtils::Lower(role ? role : ""))
    ,   m_categories(StringUtils::Lower(categories ? categories : ""))
{
    refreshEntries();
}

void MenuHelperImpl::refresh(ConstConfigRcPtr config)
{
    m_config = config;
    refreshEntries();
}

void MenuHelperImpl::refreshEntries()
{
    // Find all color spaces.

    m_colorSpaces 
        = getColorSpaceInfosFromCategories(m_config, m_roleName.c_str(), m_categories.c_str());
    if (m_colorSpaces.empty())
    {
        std::stringstream oss;
        oss << "With role '" << m_roleName << "' and categories [" << m_categories <<"]"
            << " no color spaces were found.";

        throw Exception(oss.str().c_str());
    }

    // Rebuild the complete list.

    m_entries = m_colorSpaces;
    m_entries.insert(m_entries.end(), m_additionalColorSpaces.begin(), m_additionalColorSpaces.end());
}

size_t MenuHelperImpl::getNumColorSpaces() const noexcept
{
    return m_entries.size();
}

const char * MenuHelperImpl::getColorSpaceName(size_t idx) const
{
    return getColorSpace(idx)->getName();
}

const char * MenuHelperImpl::getColorSpaceUIName(size_t idx) const
{
    return getColorSpace(idx)->getUIName();
}

ConstColorSpaceInfoRcPtr MenuHelperImpl::getColorSpace(size_t idx) const
{
    if (idx < m_entries.size())
    {
        return m_entries[idx];
    }

    std::ostringstream oss;
    oss << "Invalid color space index " << idx
        << " where size is " << m_entries.size() << ".";

    throw Exception(oss.str().c_str());
}

void MenuHelperImpl::addColorSpaceToMenu(ConstColorSpaceInfoRcPtr & cs)
{
    if (!m_config->getColorSpace(cs->getName()))
    {
        std::ostringstream oss;
        oss << "Color space '" << cs->getName() << "' does not exist.";
        throw Exception(oss.str().c_str());
    }

    for (const auto & entry : m_entries)
    {
        if (StringUtils::Compare(cs->getName(), entry->getName()))
        {
            std::ostringstream oss;
            oss << "Color space '" << cs->getName() << "' already present.";
            throw Exception(oss.str().c_str());
        }
    }

    for (const auto & entry : m_additionalColorSpaces)
    {
        if (StringUtils::Compare(cs->getName(), entry->getName()))
        {
            std::ostringstream oss;
            oss << "Color space '" << cs->getName()
                << "' already present as additional color space.";
            throw Exception(oss.str().c_str());
        }
    }

    m_additionalColorSpaces.push_back(cs);

    refreshEntries();
}

const char * MenuHelperImpl::getNameFromUIName(const char * uiName) const
{
    if (!uiName || !*uiName)
    {
        throw Exception("Invalid color space name.");
    }

    for (const auto & entry : m_entries)
    {
        if (StringUtils::Compare(uiName, entry->getUIName()))
        {
            return entry->getName();
        }
    }

    return uiName;
}

const char * MenuHelperImpl::getUINameFromName(const char * name) const
{
    if (!name || !*name)
    {
        throw Exception("Invalid color space name.");
    }

    for (const auto & entry : m_entries)
    {
        if (StringUtils::Compare(name, entry->getName()))
        {
            return entry->getUIName();
        }
    }

    return name;
}


namespace ColorSpaceHelpers
{

ConstProcessorRcPtr GetProcessor(const ConstConfigRcPtr & config,
                                 const char * inputColorSpaceName,
                                 const char * outputColorSpaceName)
{
    ColorSpaceMenuHelperRcPtr menuHelper
        = ColorSpaceMenuHelper::Create(config, nullptr, nullptr);

    return config->getProcessor(menuHelper->getNameFromUIName(inputColorSpaceName), 
                                menuHelper->getNameFromUIName(outputColorSpaceName));
}


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


} // ColorSpaceHelpers


} // namespace OCIO_NAMESPACE
