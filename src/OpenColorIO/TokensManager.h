// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_TOKENS_MANAGER_H
#define INCLUDED_OCIO_TOKENS_MANAGER_H

#include <cstring>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "PrivateTypes.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{

class TokensManager
{
public:
    TokensManager() = default;
    TokensManager(const TokensManager &) = delete;
    virtual ~TokensManager() = default;
    TokensManager & operator=(const TokensManager &) = default;

    typedef StringUtils::StringVec Tokens;

    Tokens::const_iterator findToken(const char * token) const noexcept
    {
        if (!token || !*token) return m_tokens.end();

        // NB: Categories are not case-sensitive and whitespace is stripped.
        // Stored tokens are already trimmed by addToken().
        const std::string ref(StringUtils::Trim(token));

        for (auto itr = m_tokens.begin(); itr != m_tokens.end(); ++itr)
        {
            if (StringUtils::Compare(*itr, ref))
            {
                return itr;
            }
        }

        return m_tokens.end();
    }

    bool hasToken(const char * token) const noexcept
    {
        return findToken(token) != m_tokens.end();
    }

    void addToken(const char * token)
    {
        if (!token || !*token) return;
        if (findToken(token) == m_tokens.end())
        {
            m_tokens.push_back(StringUtils::Trim(token));
        }
    }

    void removeToken(const char * token) noexcept
    {
        const auto itr = findToken(token);
        if (itr != m_tokens.end())
        {
            m_tokens.erase(itr);
        }
    }

    int getNumTokens() const noexcept
    {
        return static_cast<int>(m_tokens.size());
    }

    const char * getToken(int index) const noexcept
    {
        if (index<0 || index >= (int)m_tokens.size()) return nullptr;

        return m_tokens[index].c_str();
    }

    void clearTokens() noexcept
    {
        m_tokens.clear();
    }

private:
    Tokens m_tokens;
};

} // namespace OCIO_NAMESPACE

#endif
