// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "ContextVariableUtils.h"
#include "utils/StringUtils.h"


#if defined(__APPLE__) && !defined(__IPHONE__)
#include <crt_externs.h> // _NSGetEnviron()
#elif !defined(_WIN32)
#include<stdio.h>
extern char ** environ;
#endif


namespace
{

inline char ** GetEnviron()
{
#if __IPHONE__
    // TODO: fix this
    return NULL;
#elif __APPLE__
    return (*_NSGetEnviron());
#else
    return environ;
#endif
}

} // anon.

namespace OCIO_NAMESPACE
{

bool ContainsContextVariableToken(const std::string & str)
{
    if (StringUtils::Find(str, "$") != std::string::npos) return true;
    if (StringUtils::Find(str, "%") != std::string::npos) return true;

    return false;
}

// The method only searches for at least one context variable without checking its existence. 
bool ContainsContextVariables(const std::string & str)
{
    // As soon as there is the '$' reserved token, a context variable is present. It does not matter
    // to check for the exact syntax (i.e. "$FOO" or "${FOO}").  Note that the ambiguous case
    // "${FOO" is then resolved when calling Config::getProcessor() which will throw if "{FOO" ends
    // up to not be a context variable i.e. that was a typo.
    std::string::size_type begin = StringUtils::Find(str, "$");
    if (begin != std::string::npos)
    {
        return true;
    }

    begin = StringUtils::Find(str, "%");
    if (begin != std::string::npos)
    {
        const std::string::size_type end = StringUtils::ReverseFind(str, "%");
        if (end != std::string::npos && begin != end) return true;
    }

    return false;
}

void LoadEnvironment(EnvMap & map, bool update)
{
    // First, add or update the context variables with existing env. variables.

    for (char **env = GetEnviron(); *env != NULL; ++env)
    {
        // Split environment up into std::map[name] = value.

        const std::string env_str = (char*)*env;
        const int pos = static_cast<int>(env_str.find_first_of('='));

        const std::string name  = env_str.substr(0, pos);
        const std::string value = env_str.substr(pos+1, env_str.length());

        if (update)
        {
            // Update existing key:values that match.
            EnvMap::iterator iter = map.find(name);
            if (iter != map.end())
            {
                iter->second = value;
            }
        }
        else
        {
            map.insert(EnvMap::value_type(name, value));
        }
    }
}

std::string ResolveContextVariables(const std::string & str, const EnvMap & map, UsedEnvs & used)
{
    // Early exit if no reserved tokens are found.
    if (!ContainsContextVariables(str))
    {
        return str;
    }

    std::string orig = str;
    std::string newstr = str;

    // This walks through the envmap in key order,
    // from longest to shortest to handle envvars which are
    // substrings.
    // ie. '$TEST_$TESTING_$TE' will expand in this order '2 1 3'

    for (const auto & entry : map)
    {
        if (StringUtils::ReplaceInPlace(newstr, ("${"+ entry.first + "}"), entry.second))
        {
            used[entry.first] = entry.second;
        }

        if (StringUtils::ReplaceInPlace(newstr, ("$" + entry.first),       entry.second))
        {
            used[entry.first] = entry.second;
        }

        if (StringUtils::ReplaceInPlace(newstr, ("%" + entry.first + "%"), entry.second))
        {
            used[entry.first] = entry.second;
        }
    }

    // recursively call till string doesn't expand anymore
    if(newstr != orig)
    {
        return ResolveContextVariables(newstr, map, used);
    }

    return orig;
}

bool CollectContextVariables(const Config & config, 
                             const Context & context,
                             ConstTransformRcPtr transform,
                             ContextRcPtr & usedContextVars)
{
    if(ConstColorSpaceTransformRcPtr tr = DynamicPtrCast<const ColorSpaceTransform>(transform))
    {
        if (CollectContextVariables(config, context, *tr, usedContextVars)) return true;
    }
    else if(ConstDisplayViewTransformRcPtr tr = DynamicPtrCast<const DisplayViewTransform>(transform))
    {
        if (CollectContextVariables(config, context, *tr, usedContextVars)) return true;
    }
    else if(ConstFileTransformRcPtr tr = DynamicPtrCast<const FileTransform>(transform))
    {
        if (CollectContextVariables(config, context, *tr, usedContextVars)) return true;
    }
    else if(ConstGroupTransformRcPtr tr = DynamicPtrCast<const GroupTransform>(transform))
    {
        if (CollectContextVariables(config, context, *tr, usedContextVars)) return true;
    }
    else if(ConstLookTransformRcPtr tr = DynamicPtrCast<const LookTransform>(transform))
    {
        if (CollectContextVariables(config, context, *tr, usedContextVars)) return true;
    }

    return false;
}

} // namespace OCIO_NAMESPACE

