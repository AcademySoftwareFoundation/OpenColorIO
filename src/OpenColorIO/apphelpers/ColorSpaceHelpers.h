// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_COLORSPACE_HELPERS_H
#define INCLUDED_OCIO_COLORSPACE_HELPERS_H

#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "CategoryHelpers.h"
#include "utils/StringUtils.h"


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
                                           const ColorSpace & cs);

    static ConstColorSpaceInfoRcPtr Create(const ConstConfigRcPtr & config,
                                           const NamedTransform & nt);

    static ConstColorSpaceInfoRcPtr CreateFromRole(const ConstConfigRcPtr & config,
                                                   const char * role,
                                                   const char * family);

    static ConstColorSpaceInfoRcPtr CreateFromSingleRole(const ConstConfigRcPtr & config,
                                                         const char * role);

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
    StringUtils::StringVec m_hierarchyLevels;
};


class ColorSpaceMenuParametersImpl : public ColorSpaceMenuParameters
{
public:
    ColorSpaceMenuParametersImpl(ConstConfigRcPtr config);

    ColorSpaceMenuParametersImpl() = delete;
    ColorSpaceMenuParametersImpl & operator=(const ColorSpaceMenuParametersImpl &) = delete;

    void setParameters(ConstColorSpaceMenuParametersRcPtr parameters);

    void setConfig(ConstConfigRcPtr config) noexcept override;
    ConstConfigRcPtr getConfig() const noexcept override;
    void setRole(const char * role) noexcept override;
    const char * getRole() const noexcept override;
    void setAppCategories(const char * appCategories) noexcept override;
    const char * getAppCategories() const noexcept override;
    void setUserCategories(const char * userCategories) noexcept override;
    const char * getUserCategories() const noexcept override;
    void setEncodings(const char * encoding) noexcept override;
    const char * getEncodings() const noexcept override;
    void setIncludeColorSpaces(bool include) noexcept override;
    bool getIncludeColorSpaces() const noexcept override;
    void setIncludeRoles(bool include) noexcept override;
    bool getIncludeRoles() const noexcept override;
    void setIncludeNamedTransforms(bool include) noexcept override;
    bool getIncludeNamedTransforms() const noexcept override;
    SearchReferenceSpaceType getSearchReferenceSpaceType() const noexcept override;
    void setSearchReferenceSpaceType(SearchReferenceSpaceType colorspaceType) noexcept override;

    void addColorSpace(const char * name) noexcept override;
    size_t getNumAddedColorSpaces() const noexcept override;
    const char * getAddedColorSpace(size_t index) const noexcept override;
    void clearAddedColorSpaces() noexcept override;

    ColorSpaceMenuParametersImpl(const ColorSpaceMenuParametersImpl &) = delete;
    virtual ~ColorSpaceMenuParametersImpl() = default;

    static void Deleter(ColorSpaceMenuParameters * p);

    // Creation data.
    ConstConfigRcPtr m_config;
    std::string m_role;
    std::string m_appCategories;
    std::string m_userCategories;
    std::string m_encodings;
    bool m_includeColorSpaces = true;
    bool m_includeRoles = false;
    bool m_includeNamedTransforms = false;
    SearchReferenceSpaceType m_colorSpaceType = SEARCH_REFERENCE_SPACE_ALL;

    StringUtils::StringVec m_additionalColorSpaces;
};


class ColorSpaceMenuHelperImpl : public ColorSpaceMenuHelper
{
public:

    ColorSpaceMenuHelperImpl() = delete;
    ColorSpaceMenuHelperImpl & operator=(const ColorSpaceMenuHelperImpl &) = delete;

    ColorSpaceMenuHelperImpl(ConstColorSpaceMenuParametersRcPtr parameters);

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

    static void Deleter(ColorSpaceMenuHelper * hlp);

    std::ostream & serialize(std::ostream & os) const;

protected:
    void refresh();

private:
    // Creation data.
    ColorSpaceMenuParametersImpl m_parameters;

    // Contains all the color space names (m_colorSpaces and m_additionalColorSpaces).
    Infos m_entries;
};


}  // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_COLORSPACE_HELPERS_H
