// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CUSTOMKEYS_H
#define INCLUDED_OCIO_CUSTOMKEYS_H


#include <map>
#include <sstream>
#include <string>

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

class CustomKeysContainer
{
public:
    CustomKeysContainer() = default;
    CustomKeysContainer(const CustomKeysContainer &) = default;
    CustomKeysContainer & operator=(const CustomKeysContainer &) = default;

    using CustomKeys = std::map<std::string, std::string>;

    size_t getSize() const noexcept
    {
        return m_customKeys.size();
    }

    const char * getName(size_t key) const
    {
        validateIndex(key);
        auto cust = std::next(m_customKeys.begin(), key);
        return (*cust).first.c_str();
    }

    const char * getValue(size_t key) const
    {
        validateIndex(key);
        auto cust = std::next(m_customKeys.begin(), key);
        return (*cust).second.c_str();
    }

    void set(const char * key, const char * value)
    {
        if (!key || !*key)
        {
            throw Exception("Key has to be a non-empty string.");
        }
        if (value && *value)
        {
            m_customKeys[key] = value;
        }
        else
        {
            m_customKeys.erase(key);
        }
    }

private:
    void validateIndex(size_t key) const
    {
        const auto numKeys = getSize();
        if (key >= numKeys)
        {
            std::ostringstream oss;
            oss << "Key index '" << key << "' is invalid, there are '" << numKeys
                << "' custom keys.";
            throw Exception(oss.str().c_str());
        }
    }

    CustomKeys m_customKeys;
};

}  // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_CUSTOMKEYS_H
