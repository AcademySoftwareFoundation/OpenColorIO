// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "CategoryHelpers.h"
#include "ColorSpaceHelpers.h"


namespace OCIO_NAMESPACE
{

namespace
{

// Using pointers directly because all color spaces are from a single config and thus pointers
// can be used to check if a given color space is already present.
// TODO: Enhance ColorSpaceSet to allow its use here.
typedef std::vector<const ColorSpace *> ColorSpaceVec;

template<class T>
void AddElement(std::vector<T> & vec, T elt)
{
    for (auto & entry : vec)
    {
        if (entry == elt)
        {
            // Already there.
            return;
        }
    }
    vec.push_back(elt);
}

template<class T>
bool HasCategory(const T & elt, const std::string & category)
{
    return elt->hasCategory(category.c_str());
}

template<class T>
bool HasEncoding(const T & elt, const std::string & encoding)
{
    return StringUtils::Compare(encoding, elt->getEncoding());
}

ColorSpaceVec GetColorSpaces(ConstConfigRcPtr config,
                             bool includeColorSpaces,
                             SearchReferenceSpaceType colorSpaceType,
                             const Categories & categories,
                             const Encodings & encodings)
{
    ColorSpaceVec css;
    if (includeColorSpaces && !categories.empty() && !encodings.empty())
    {
        const auto numCS = config->getNumColorSpaces(colorSpaceType, COLORSPACE_ACTIVE);
        for (int idx = 0; idx < numCS; ++idx)
        {
            auto cs = config->getColorSpace(config->getColorSpaceNameByIndex(colorSpaceType,
                                                                             COLORSPACE_ACTIVE,
                                                                             idx));
            for (const auto & cat : categories)
            {
                for (const auto & enc : encodings)
                {
                    if (HasCategory(cs, cat) && HasEncoding(cs, enc))
                    {
                        AddElement(css, cs.get());
                    }
                }
            }
        }
    }
    return css;
}

ColorSpaceVec GetColorSpaces(ConstConfigRcPtr config,
                             bool includeColorSpaces,
                             SearchReferenceSpaceType colorSpaceType,
                             const Categories & categories)
{
    ColorSpaceVec css;
    if (includeColorSpaces && !categories.empty())
    {
        const auto numCS = config->getNumColorSpaces(colorSpaceType, COLORSPACE_ACTIVE);
        for (int idx = 0; idx < numCS; ++idx)
        {
            auto cs = config->getColorSpace(config->getColorSpaceNameByIndex(colorSpaceType,
                                                                             COLORSPACE_ACTIVE,
                                                                             idx));
            for (const auto & cat : categories)
            {
                if (HasCategory(cs, cat))
                {
                    AddElement(css, cs.get());
                }
            }
        }
        }
    return css;
}

ColorSpaceVec GetColorSpacesFromEncodings(ConstConfigRcPtr config,
                                          bool includeColorSpaces,
                                          SearchReferenceSpaceType colorSpaceType,
                                          const Encodings & encodings)
{
    ColorSpaceVec css;
    if (includeColorSpaces && !encodings.empty())
    {
        const auto numCS = config->getNumColorSpaces(colorSpaceType, COLORSPACE_ACTIVE);
        for (int idx = 0; idx < numCS; ++idx)
        {
            auto cs = config->getColorSpace(config->getColorSpaceNameByIndex(colorSpaceType,
                                                                             COLORSPACE_ACTIVE,
                                                                             idx));
            for (const auto & enc : encodings)
            {
                if (HasEncoding(cs, enc))
                {
                    AddElement(css, cs.get());
                }
            }
        }
    }
    return css;
}

typedef std::vector<const NamedTransform *> NamedTransformVec;

NamedTransformVec GetNamedTransforms(ConstConfigRcPtr config,
                                     bool includeNamedTransforms,
                                     const Categories & categories,
                                     const Encodings & encodings)
{
    NamedTransformVec nts;
    if (includeNamedTransforms && !categories.empty() && !encodings.empty())
    {
        for (int idx = 0; idx < config->getNumNamedTransforms(); ++idx)
        {
            auto nt = config->getNamedTransform(config->getNamedTransformNameByIndex(idx));
            for (const auto & cat : categories)
            {
                for (const auto & enc : encodings)
                {
                    if (HasCategory(nt, cat) && HasEncoding(nt, enc))
                    {
                        AddElement(nts, nt.get());
                    }
                }
            }
        }
    }
    return nts;
}

NamedTransformVec GetNamedTransforms(ConstConfigRcPtr config,
                                     bool includeNamedTransforms,
                                     const Categories & categories)
{
    NamedTransformVec nts;
    if (includeNamedTransforms && !categories.empty())
    {
        for (int idx = 0; idx < config->getNumNamedTransforms(); ++idx)
        {
            auto nt = config->getNamedTransform(config->getNamedTransformNameByIndex(idx));
            for (const auto & cat : categories)
            {
                if (HasCategory(nt, cat))
                {
                    AddElement(nts, nt.get());
                }
            }
        }
    }
    return nts;
}

NamedTransformVec GetNamedTransformsFromEncodings(ConstConfigRcPtr config,
                                                  bool includeNamedTransforms,
                                                  const Encodings & encodings)
{
    NamedTransformVec nts;
    if (includeNamedTransforms && !encodings.empty())
    {
        for (int idx = 0; idx < config->getNumNamedTransforms(); ++idx)
        {
            auto nt = config->getNamedTransform(config->getNamedTransformNameByIndex(idx));
            for (const auto & enc : encodings)
            {
                if (HasEncoding(nt, enc))
                {
                    AddElement(nts, nt.get());
                }
            }
        }
    }
    return nts;
}

Infos GetInfos(ConstConfigRcPtr & config,
               const ColorSpaceVec & css,
               const NamedTransformVec & nts)
{
    Infos allInfos;
    for (const auto & cs : css)
    {
        allInfos.push_back(ColorSpaceInfo::Create(config, *cs));
    }
    for (const auto & nt : nts)
    {
        allInfos.push_back(ColorSpaceInfo::Create(config, *nt));
    }
    return allInfos;
}

template<typename T>
ColorSpaceNames GetNames(const T & list)
{
    ColorSpaceNames allNames;

    for (const auto & item : list)
    {
        allNames.push_back(item->getName());
    }

    return allNames;
}

template<typename T>
T Intersection(const T & list0, const T & list1)
{
    T result;
    const auto begin1 = list1.begin();
    const auto end1 = list1.end();
    for (const auto & i0 : list0)
    {
        if (std::find(begin1, end1, i0) != end1)
        {
            result.push_back(i0);
        }
    }
    return result;
}

} // anon.

StringUtils::StringVec ExtractItems(const char * strings)
{
    StringUtils::StringVec tmp = StringUtils::Split(StringUtils::Lower(strings), ',');
    StringUtils::StringVec all;

    for (const auto & val : tmp)
    {
        auto v = StringUtils::Trim(val);
        if (!v.empty())
        {
            all.push_back(v);
        }
    }

    return all;
}

ColorSpaceNames FindColorSpaceNames(ConstConfigRcPtr config, const Categories & categories)
{
    ColorSpaceVec allCS = GetColorSpaces(config, true, SEARCH_REFERENCE_SPACE_ALL, categories);
    return GetNames(allCS);
}

namespace
{
// Used by FindColorSpaceInfos to identify and log if a fall-back was required.
enum CategoryUsage
{
    NOT_USED,
    SHOULD_BE_USED,
    IGNORED,
    NONE_FOUND
};
struct LogMessageHelper
{
    bool m_ignoreEncodings = false;
    bool m_ignoreCategories = false;
    bool m_emptyIntersection = false;
    CategoryUsage m_appCats = NOT_USED;
    CategoryUsage m_userCats = NOT_USED;
    ~LogMessageHelper()
    {
        if (GetLoggingLevel() >= LOGGING_LEVEL_INFO && (m_emptyIntersection ||
            m_ignoreEncodings || m_ignoreCategories ||
            m_appCats == NONE_FOUND || m_userCats == NONE_FOUND ||
            m_userCats == IGNORED))
        {
            std::stringstream os;
            os << "All parameters could not be used to create the menu:";
            if (m_emptyIntersection)
            {
                os << " Intersection of color spaces with app categories and color spaces with "
                      "user categories is empty.";
            }
            if (m_appCats == NONE_FOUND)
            {
                os << " Found no color space using app categories.";
                if (m_userCats == IGNORED || m_userCats == NONE_FOUND)
                {
                    m_ignoreCategories = true;
                }
            }
            if (m_userCats == NONE_FOUND)
            {
                os << " Found no color space using user categories.";
            }
            else if (m_userCats == IGNORED)
            {
                os << " User categories have been ignored.";
            }
            if (m_ignoreEncodings)
            {
                os << " Encodings have been ignored since they matched no color spaces.";
            }
            if (m_ignoreCategories)
            {
                os << " Categories have been ignored since they matched no color spaces.";
            }

            LogMessage(LOGGING_LEVEL_INFO, os.str().c_str());
        }
    }
};
}

Infos FindColorSpaceInfos(ConstConfigRcPtr config,
                          const Categories & appCategories,
                          const Categories & userCategories,
                          bool includeColorSpaces,
                          bool includeNamedTransforms,
                          const Encodings & encodings,
                          SearchReferenceSpaceType colorSpaceType)
{
    // At least one of include flags is true.

    LogMessageHelper log;

    // V1 does not have categories and encodings, skip them.
    if (config->getMajorVersion() >= 2)
    {
        ColorSpaceVec appCS;
        NamedTransformVec appNT;
        ColorSpaceVec appCSNoEncodings;
        NamedTransformVec appNTNoEncodings;
        bool appNoEncodingsComputed{ false };

        size_t appSize{ 0 };

        bool encsIgnored = encodings.empty();

        if (!appCategories.empty())
        {
            // 3a) Use categories and encodings, fallback to only categories, fallback to only
            //     encodings.

            log.m_appCats = SHOULD_BE_USED;

            // Use categories and encodings.

            if (!encsIgnored)
            {
                appCS = GetColorSpaces(config, includeColorSpaces, colorSpaceType,
                                       appCategories, encodings);
                appNT = GetNamedTransforms(config, includeNamedTransforms, appCategories,
                                           encodings);
                appSize = appCS.size() + appNT.size();
            }

            // Do not use encodings if empty or drop them if no result is found with them.
            if (appSize == 0)
            {
                encsIgnored = true;
                log.m_ignoreEncodings = !encodings.empty();
                appCS = GetColorSpaces(config, includeColorSpaces, colorSpaceType, appCategories);
                appNT = GetNamedTransforms(config, includeNamedTransforms, appCategories);
                appSize = appCS.size() + appNT.size();

                // Keep these results in case we need them later.
                appNoEncodingsComputed = true;
                appCSNoEncodings = appCS;
                appNTNoEncodings = appNT;
            }

            // Drop app categories and use encoding if no results.
            if (appSize == 0 && !encodings.empty())
            {
                encsIgnored = false;
                log.m_ignoreEncodings = false;
                log.m_appCats = NONE_FOUND;
                appCS = GetColorSpacesFromEncodings(config, includeColorSpaces, colorSpaceType,
                                                    encodings);
                appNT = GetNamedTransformsFromEncodings(config, includeNamedTransforms, encodings);
                appSize = appCS.size() + appNT.size();
            }

            if (appSize == 0)
            {
                log.m_appCats = NONE_FOUND;
            }
        }
        else if (!encsIgnored)
        {
            appCS = GetColorSpacesFromEncodings(config, includeColorSpaces, colorSpaceType,
                                                encodings);
            appNT = GetNamedTransformsFromEncodings(config, includeNamedTransforms, encodings);
            appSize = appCS.size() + appNT.size();
        }

        ColorSpaceVec userCS;
        NamedTransformVec userNT;
        size_t userSize{ 0 };

        if (!userCategories.empty())
        {
            // 3b) Items using user categories.

            userCS = GetColorSpaces(config, includeColorSpaces, colorSpaceType, userCategories);
            userNT = GetNamedTransforms(config, includeNamedTransforms, userCategories);
            userSize = userCS.size() + userNT.size();
            if (userSize == 0)
            {
                log.m_userCats = NONE_FOUND;
            }
        }

        if (appSize != 0 && userSize != 0)
        {
            // 3c) and 3d) Use intersection of app and user categories.

            ColorSpaceVec * appCSTest = &appCS;
            NamedTransformVec * appNTTest = &appNT;
            const auto encsIgnoredBack = encsIgnored;
            const auto ignoreEncodingsBack = log.m_ignoreEncodings;

            // Allow to run twice, with and without encodings.
            while (1)
            {
                const auto css = Intersection(*appCSTest, userCS);
                const auto nts = Intersection(*appNTTest, userNT);

                if (!css.empty() || !nts.empty())
                {
                    // 3c) or 3d) Intersection is not empty.
                    return GetInfos(config, css, nts);
                }

                if (!encsIgnored && !encodings.empty())
                {
                    // Intersection is empty, but encodings can be dropped if they were not dropped
                    // already.
                    encsIgnored = true;
                    log.m_ignoreEncodings = true;
                    if (!appNoEncodingsComputed)
                    {
                        // If not already computed, compute list with app categories and no
                        // encodings.
                        appCSNoEncodings = GetColorSpaces(config, includeColorSpaces,
                                                          colorSpaceType, appCategories);
                        appNTNoEncodings = GetNamedTransforms(config, includeNamedTransforms,
                                                              appCategories);
                    }
                    appCSTest = &appCSNoEncodings;
                    appNTTest = &appNTNoEncodings;
                }
                else
                {
                    break;
                }
            }
            log.m_emptyIntersection = true;
            encsIgnored = encsIgnoredBack;
            log.m_ignoreEncodings = ignoreEncodingsBack;
        }

        if (appSize)
        {
            // 3e) Only use app categories. Use the result of 3a).
            if (!userCategories.empty() && log.m_userCats != NONE_FOUND)
            {
                log.m_userCats = IGNORED;
            }
            return GetInfos(config, appCS, appNT);
        }

        if (userSize)
        {
            // 3f) Only use user categories.
            return GetInfos(config, userCS, userNT);
        }

        // Fallback to ignoring categories and encodings.
        log.m_ignoreCategories = !appCategories.empty() || !userCategories.empty();
    }

    // 3g) Ignore all categories and encodings and return all items.

    Infos allInfos;
    const auto numCS = config->getNumColorSpaces(colorSpaceType, COLORSPACE_ACTIVE);
    for (int idx = 0; idx < numCS; ++idx)
    {
        const char * csName = config->getColorSpaceNameByIndex(colorSpaceType,
                                                               COLORSPACE_ACTIVE, idx);
        ConstColorSpaceRcPtr cs = config->getColorSpace(csName);
        allInfos.push_back(ColorSpaceInfo::Create(config, *cs));
    }

    if (includeNamedTransforms)
    {
        for (int idx = 0; idx < config->getNumNamedTransforms(); ++idx)
        {
            auto nt = config->getNamedTransform(config->getNamedTransformNameByIndex(idx));
            allInfos.push_back(ColorSpaceInfo::Create(config, *nt));
        }
    }

    // Nothing is found, no need to log anything.
    if (allInfos.size() == 0)
    {
        log.m_appCats = NOT_USED;
        log.m_userCats = NOT_USED;
        log.m_emptyIntersection = false;
        log.m_ignoreCategories = false;
        log.m_ignoreEncodings = false;
    }
    return allInfos;
}


} // namespace OCIO_NAMESPACE
