// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_COLORSPACE_HELPERS_H
#define INCLUDED_OCIO_COLORSPACE_HELPERS_H

#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "CategoryHelpers.h"


namespace OCIO_NAMESPACE
{

// Helper class used for color spaces, roles, and named transforms. The family string is processed
// using the family separator and exposed as a set of named hierarchy levels. The uiName is an
// alternative name, when not provided, it is the same as the name.
class ColorSpaceInfo
{
public:
    static ConstColorSpaceInfoRcPtr Create(const ConstConfigRcPtr & config,
                                           const char * name,
                                           const char * family,
                                           const char * description);

    static ConstColorSpaceInfoRcPtr Create(const ConstConfigRcPtr & config,
                                           const char * name,
                                           const char * uiName,
                                           const char * family,
                                           const char * description);

    static ConstColorSpaceInfoRcPtr Create(const ConstConfigRcPtr & config,
                                           const ConstColorSpaceRcPtr & cs);

    static ConstColorSpaceInfoRcPtr Create(const ConstConfigRcPtr & config,
                                           const ConstNamedTransformRcPtr & nt);

    static ConstColorSpaceInfoRcPtr CreateFromRole(const ConstConfigRcPtr & config,
                                                   const char * role,
                                                   const char * family);

    ColorSpaceInfo(const ConstConfigRcPtr & config,
                   const char * name,
                   const char * uiName,
                   const char * family,
                   const char * description);

    const char * getName() const noexcept;
    const char * getUIName() const noexcept;
    const char * getDescription() const noexcept;
    const char * getFamily() const noexcept;

    // The family is split into levels using the 'family separator'.
    size_t getNumHierarchyLevels() const noexcept;
    const char * getHierarchyLevel(size_t i) const noexcept;

    static void Deleter(ColorSpaceInfo * cs);

    ColorSpaceInfo() = default;
    virtual ~ColorSpaceInfo() = default;
    ColorSpaceInfo(const ColorSpaceInfo &) = delete;
    ColorSpaceInfo & operator=(const ColorSpaceInfo &) = delete;

private:
    const std::string m_name;
    const std::string m_uiName;
    const std::string m_family;
    const std::string m_description;

    // Extracted from the color space's family attribute to be used for a hierarchical menu.
    std::vector<std::string> m_hierarchyLevels;
};


class ColorSpaceMenuHelperImpl : public ColorSpaceMenuHelper
{
public:

    ColorSpaceMenuHelperImpl() = delete;
    ColorSpaceMenuHelperImpl & operator=(const ColorSpaceMenuHelperImpl &) = delete;

    ColorSpaceMenuHelperImpl(const ConstConfigRcPtr & config,
                             const char * role,             // Role name to override categories.
                             const char * categories,       // Comma-separated list of categories.
                             IncludeTypeFlag includeFlag);

    ColorSpaceMenuHelperImpl(const ColorSpaceMenuHelperImpl &) = delete;
    virtual ~ColorSpaceMenuHelperImpl() = default;

    size_t getNumColorSpaces() const noexcept override;
    const char * getName(size_t idx) const noexcept override;
    const char * getUIName(size_t idx) const noexcept override;

    size_t getIndexFromName(const char * name) const noexcept override;
    size_t getIndexFromUIName(const char * name) const noexcept override;

    const char * getFamily(size_t idx) const noexcept override;
    const char * getDescription(size_t idx) const noexcept override;
    size_t getNumHierarchyLevels(size_t idx) const noexcept override;
    const char * getHierarchyLevel(size_t idx, size_t i) const noexcept override;

    const char * getNameFromUIName(const char * uiName) const noexcept override;
    const char * getUINameFromName(const char * name) const noexcept override;

    void addColorSpaceToMenu(const char * name) override;

    void refresh(const ConstConfigRcPtr & config) override;

    static void Deleter(ColorSpaceMenuHelper * hlp);

    std::ostream & serialize(std::ostream & os) const;

protected:
    void refresh();

private:
    ConstConfigRcPtr m_config;
    const std::string m_roleName;
    const std::string m_categories;
    const IncludeTypeFlag m_includeFlag;

    // Contains all the color space names (m_colorSpaces and m_additionalColorSpaces).
    Infos m_entries;

    Infos m_colorSpaces;
    Infos m_additionalColorSpaces;
};


}  // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_COLORSPACE_HELPERS_H
