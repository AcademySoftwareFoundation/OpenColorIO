/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <OpenColorIO/OpenColorIO.h>

#include <algorithm>

#include "LookParse.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"
#include <iostream>

OCIO_NAMESPACE_ENTER
{
    void LookParseResult::Token::parse(const std::string & str)
    {
        // Assert no commas, colons, or | in str.
        
        if(pystring::startswith(str, "+"))
        {
            name = pystring::lstrip(str, "+");
            dir = TRANSFORM_DIR_FORWARD;
        }
        // TODO: Handle --
        else if(pystring::startswith(str, "-"))
        {
            name = pystring::lstrip(str, "-");
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
        if(dir==TRANSFORM_DIR_FORWARD)
        {
            os << name;
        }
        else if(dir==TRANSFORM_DIR_INVERSE)
        {
            os << "-" << name;
        }
        else
        {
            os << "?" << name;
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
        
        std::string strippedlooks = pystring::strip(looksstr);
        if(strippedlooks.empty())
        {
            return m_options;
        }
        
        std::vector<std::string> options;
        pystring::split(strippedlooks, options, "|");
        
        std::vector<std::string> vec;
        
        for(unsigned int optionsindex=0;
            optionsindex<options.size();
            ++optionsindex)
        {
            LookParseResult::Tokens tokens;
            
            vec.clear();
            SplitStringEnvStyle(vec, options[optionsindex].c_str());
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
        
        for(unsigned int optionindex=0;
            optionindex<m_options.size();
            ++optionindex)
        {
            std::reverse(m_options[optionindex].begin(), m_options[optionindex].end());
            
            for(unsigned int tokenindex=0;
                tokenindex<m_options[optionindex].size();
                ++tokenindex)
            {
                m_options[optionindex][tokenindex].dir = 
                    GetInverseTransformDirection(
                        m_options[optionindex][tokenindex].dir);
            }
        }
    }
}
OCIO_NAMESPACE_EXIT



///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

OCIO_NAMESPACE_USING

#include "UnitTest.h"

OIIO_ADD_TEST(LookParse, Parse)
{
    LookParseResult r;
    
    {
    const LookParseResult::Options & options = r.parse("");
    OIIO_CHECK_EQUAL(options.size(), 0);
    OIIO_CHECK_EQUAL(options.empty(), true);
    }
    
    {
    const LookParseResult::Options & options = r.parse("  ");
    OIIO_CHECK_EQUAL(options.size(), 0);
    OIIO_CHECK_EQUAL(options.empty(), true);
    }
    
    {
    const LookParseResult::Options & options = r.parse("cc");
    OIIO_CHECK_EQUAL(options.size(), 1);
    OIIO_CHECK_EQUAL(options[0][0].name, "cc");
    OIIO_CHECK_EQUAL(options[0][0].dir, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(options.empty(), false);
    }
    
    {
    const LookParseResult::Options & options = r.parse("+cc");
    OIIO_CHECK_EQUAL(options.size(), 1);
    OIIO_CHECK_EQUAL(options[0][0].name, "cc");
    OIIO_CHECK_EQUAL(options[0][0].dir, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(options.empty(), false);
    }
    
    {
    const LookParseResult::Options & options = r.parse("  +cc");
    OIIO_CHECK_EQUAL(options.size(), 1);
    OIIO_CHECK_EQUAL(options[0][0].name, "cc");
    OIIO_CHECK_EQUAL(options[0][0].dir, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(options.empty(), false);
    }
    
    {
    const LookParseResult::Options & options = r.parse("  +cc   ");
    OIIO_CHECK_EQUAL(options.size(), 1);
    OIIO_CHECK_EQUAL(options[0][0].name, "cc");
    OIIO_CHECK_EQUAL(options[0][0].dir, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(options.empty(), false);
    }
    
    {
    const LookParseResult::Options & options = r.parse("+cc,-di");
    OIIO_CHECK_EQUAL(options.size(), 1);
    OIIO_CHECK_EQUAL(options[0].size(), 2);
    OIIO_CHECK_EQUAL(options[0][0].name, "cc");
    OIIO_CHECK_EQUAL(options[0][0].dir, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(options[0][1].name, "di");
    OIIO_CHECK_EQUAL(options[0][1].dir, TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(options.empty(), false);
    }
    
    {
    const LookParseResult::Options & options = r.parse("  +cc ,  -di");
    OIIO_CHECK_EQUAL(options.size(), 1);
    OIIO_CHECK_EQUAL(options[0].size(), 2);
    OIIO_CHECK_EQUAL(options[0][0].name, "cc");
    OIIO_CHECK_EQUAL(options[0][0].dir, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(options[0][1].name, "di");
    OIIO_CHECK_EQUAL(options[0][1].dir, TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(options.empty(), false);
    }
    
    {
    const LookParseResult::Options & options = r.parse("  +cc :  -di");
    OIIO_CHECK_EQUAL(options.size(), 1);
    OIIO_CHECK_EQUAL(options[0].size(), 2);
    OIIO_CHECK_EQUAL(options[0][0].name, "cc");
    OIIO_CHECK_EQUAL(options[0][0].dir, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(options[0][1].name, "di");
    OIIO_CHECK_EQUAL(options[0][1].dir, TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(options.empty(), false);
    }
    
    {
    const LookParseResult::Options & options = r.parse("+cc, -di |-cc");
    OIIO_CHECK_EQUAL(options.size(), 2);
    OIIO_CHECK_EQUAL(options[0].size(), 2);
    OIIO_CHECK_EQUAL(options[0][0].name, "cc");
    OIIO_CHECK_EQUAL(options[0][0].dir, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(options[0][1].name, "di");
    OIIO_CHECK_EQUAL(options[0][1].dir, TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(options[1].size(), 1);
    OIIO_CHECK_EQUAL(options.empty(), false);
    OIIO_CHECK_EQUAL(options[1][0].name, "cc");
    OIIO_CHECK_EQUAL(options[1][0].dir, TRANSFORM_DIR_INVERSE);
    }
    
    {
    const LookParseResult::Options & options = r.parse("+cc, -di |-cc|   ");
    OIIO_CHECK_EQUAL(options.size(), 3);
    OIIO_CHECK_EQUAL(options[0].size(), 2);
    OIIO_CHECK_EQUAL(options[0][0].name, "cc");
    OIIO_CHECK_EQUAL(options[0][0].dir, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(options[0][1].name, "di");
    OIIO_CHECK_EQUAL(options[0][1].dir, TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(options[1].size(), 1);
    OIIO_CHECK_EQUAL(options.empty(), false);
    OIIO_CHECK_EQUAL(options[1][0].name, "cc");
    OIIO_CHECK_EQUAL(options[1][0].dir, TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(options[2].size(), 1);
    OIIO_CHECK_EQUAL(options[2][0].name, "");
    OIIO_CHECK_EQUAL(options[2][0].dir, TRANSFORM_DIR_FORWARD);
    }
}

OIIO_ADD_TEST(LookParse, Reverse)
{
    LookParseResult r;
    
    {
    r.parse("+cc, -di |-cc|   ");
    r.reverse();
    const LookParseResult::Options & options = r.getOptions();
    
    OIIO_CHECK_EQUAL(options.size(), 3);
    OIIO_CHECK_EQUAL(options[0].size(), 2);
    OIIO_CHECK_EQUAL(options[0][1].name, "cc");
    OIIO_CHECK_EQUAL(options[0][1].dir, TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(options[0][0].name, "di");
    OIIO_CHECK_EQUAL(options[0][0].dir, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(options[1].size(), 1);
    OIIO_CHECK_EQUAL(options.empty(), false);
    OIIO_CHECK_EQUAL(options[1][0].name, "cc");
    OIIO_CHECK_EQUAL(options[1][0].dir, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(options[2].size(), 1);
    OIIO_CHECK_EQUAL(options[2][0].name, "");
    OIIO_CHECK_EQUAL(options[2][0].dir, TRANSFORM_DIR_INVERSE);
    }

    
}

#endif // OCIO_UNIT_TEST
