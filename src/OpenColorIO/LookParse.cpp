// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <iostream>

#include <OpenColorIO/OpenColorIO.h>

#include "LookParse.h"
#include "ParseUtils.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{
void LookParseResult::Token::parse(const std::string & str)
{
    // Assert no commas, colons, or | in str.

    if(StringUtils::StartsWith(str, "+"))
    {
        name = StringUtils::LeftTrim(str, '+');
        dir = TRANSFORM_DIR_FORWARD;
    }
    // TODO: Handle --
    else if(StringUtils::StartsWith(str, "-"))
    {
        name = StringUtils::LeftTrim(str, '-');
        dir = TRANSFORM_DIR_INVERSE;
    }
    else
    {
        name = str;
        dir = TRANSFORM_DIR_FORWARD;
    }
}

void LookParseResult::Token::serialize(std::ostream & os) const
{
    switch (dir)
    {
    case TRANSFORM_DIR_FORWARD:
        os << name;
        break;
    case TRANSFORM_DIR_INVERSE:
        os << "-" << name;
        break;
    }
}

void LookParseResult::serialize(std::ostream & os, const Tokens & tokens)
{
    for(unsigned int i=0; i<tokens.size(); ++i)
    {
        if(i!=0) os << ", ";
        tokens[i].serialize(os);
    }
}

const LookParseResult::Options & LookParseResult::parse(const std::string & looksstr)
{
    m_options.clear();

    std::string strippedlooks = StringUtils::Trim(looksstr);
    if(strippedlooks.empty())
    {
        return m_options;
    }

    const StringUtils::StringVec options = StringUtils::Split(strippedlooks, '|');

    StringUtils::StringVec vec;

    for(unsigned int optionsindex=0; optionsindex<options.size(); ++optionsindex)
    {
        LookParseResult::Tokens tokens;

        vec.clear();
        vec = SplitStringEnvStyle(options[optionsindex]);
        for(unsigned int i=0; i<vec.size(); ++i)
        {
            LookParseResult::Token t;
            t.parse(vec[i]);
            tokens.push_back(t);
        }

        m_options.push_back(tokens);
    }

    return m_options;
}

const LookParseResult::Options & LookParseResult::getOptions() const
{
    return m_options;
}

bool LookParseResult::empty() const
{
    return m_options.empty();
}

void LookParseResult::reverse()
{
    // m_options itself should NOT be reversed.
    // The individual looks
    // need to be applied in the inverse direction. But, the precedence
    // for which option to apply is to be maintained!

    for (unsigned int optionindex=0; optionindex<m_options.size(); ++optionindex)
    {
        std::reverse(m_options[optionindex].begin(), m_options[optionindex].end());

        for (unsigned int tokenindex=0; tokenindex<m_options[optionindex].size(); ++tokenindex)
        {
            m_options[optionindex][tokenindex].dir = 
                GetInverseTransformDirection(m_options[optionindex][tokenindex].dir);
        }
    }
}
} // namespace OCIO_NAMESPACE

