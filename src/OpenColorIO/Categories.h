// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CATEGORIES_H
#define INCLUDED_OCIO_CATEGORIES_H

#include <cstring>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "PrivateTypes.h"
#include "pystring/pystring.h"

namespace OCIO_NAMESPACE
{

class CategoriesManager
{
public:
    CategoriesManager() = default;
    CategoriesManager(const CategoriesManager &) = delete;
    virtual ~CategoriesManager() = default;
    CategoriesManager & operator=(const CategoriesManager &) = default;

    typedef StringVec Categories;

    Categories::const_iterator findCategory(const char * category) const
    {
        if (!category || !*category) return m_categories.end();

        // NB: Categories are not case-sensitive and whitespace is stripped.
        const std::string ref(pystring::strip(pystring::lower(category)));

        for (auto itr = m_categories.begin(); itr != m_categories.end(); ++itr)
        {
            if (pystring::strip(pystring::lower(*itr)) == ref)
            {
                return itr;
            }
        }

        return m_categories.end();
    }

    bool hasCategory(const char * category) const
    {
        return findCategory(category) != m_categories.end();
    }

    void addCategory(const char * category)
    {
        if (findCategory(category) == m_categories.end())
        {
            m_categories.push_back(pystring::strip(category));
        }
    }

    void removeCategory(const char * category)
    {
        if (!category || !*category) return;

        // NB: Categories are not case-sensitive and whitespace is stripped.
        const std::string ref(pystring::strip(pystring::lower(category)));

        for (auto itr = m_categories.begin(); itr != m_categories.end(); ++itr)
        {
            if (pystring::strip(pystring::lower(*itr)) == ref)
            {
                m_categories.erase(itr);
                return;
            }
        }

        return;
    }

    int getNumCategories() const
    {
        return static_cast<int>(m_categories.size());
    }

    const char * getCategory(int index) const
    {
        if (index<0 || index >= (int)m_categories.size()) return nullptr;

        return m_categories[index].c_str();
    }

    void clearCategories()
    {
        m_categories.clear();
    }

protected:
    Categories m_categories;
};

} // namespace OCIO_NAMESPACE

#endif
