// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_PARSED_LOOK_H
#define INCLUDED_OCIO_PARSED_LOOK_H

#include <OpenColorIO/OpenColorIO.h>

#include <vector>

namespace OCIO_NAMESPACE
{
// Looks parse structures
// This is contains a list, where each option entry corresponds to
// an "or" separated looks token list.
// I.e, " +cc,-onset | +cc " parses to TWO options: (+cc,-onset), (+cc)

class LookParseResult
{
    public:
    struct Token
    {
        std::string name;
        TransformDirection dir;

        Token():
            dir(TRANSFORM_DIR_FORWARD) {}

        void parse(const std::string & str);
        void serialize(std::ostream & os) const;
    };

    typedef std::vector<Token> Tokens;

    static void serialize(std::ostream & os, const Tokens & tokens);

    typedef std::vector<Tokens> Options;

    const Options & parse(const std::string & looksstr);

    const Options & getOptions() const;
    bool empty() const;

    void reverse();

    private:
    Options m_options;
};

} // namespace OCIO_NAMESPACE

#endif
