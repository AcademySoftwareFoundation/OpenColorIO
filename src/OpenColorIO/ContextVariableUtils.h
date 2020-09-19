// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CONTEXT_VARIABLE_UTILS_H
#define INCLUDED_OCIO_CONTEXT_VARIABLE_UTILS_H


#include <functional>
#include <map>
#include <string>

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

// True if the string contains at least one context variable reserved token (that is, '$' or '%').
bool ContainsContextVariableToken(const std::string & str);

// True if the string contains a context variable.
bool ContainsContextVariables(const std::string & str);

// The EnvMap is ordered by the length of the keys (long -> short). This is so that recursive string
// expansion will deal with similar prefixed keys as expected.
// ie. '$TEST_$TESTING_$TE' will expand in this order '2 1 3'
template <class T>
struct EnvMapKey
{
    bool operator() (const T & x, const T & y) const
    {
        // If the lengths are unequal, sort by length.
        if(x.length() != y.length())
        {
            return (x.length() > y.length());
        }
        // Otherwise, use the standard string sort comparison.
        else
        {
            return (x<y);
        }
    }
};
typedef std::map< std::string, std::string, EnvMapKey< std::string > > EnvMap;

// Get map of current env key = value, or update the existing entries
void LoadEnvironment(EnvMap & map, bool update = false);

// Resolve a string with $VAR, ${VAR}, or %VAR% using the keys passed in EnvMap.
// Any entries in EnvMap that are used are added to UsedEnvs.
typedef std::map<std::string, std::string> UsedEnvs;
// TODO: Keep the resolution order?
std::string ResolveContextVariables(const std::string & str, const EnvMap & map, UsedEnvs & envs);


// Return true if an instance of a transform uses a context variable, either directly or indirectly. 
// Add any context variables that are used to usedContextVars.
bool CollectContextVariables(const Config & config, 
                             const Context & context,
                             ConstTransformRcPtr tr,
                             ContextRcPtr & usedContextVars);

bool CollectContextVariables(const Config & config,
                             const Context & context,
                             const ColorSpaceTransform & tr,
                             ContextRcPtr & usedContextVars);

bool CollectContextVariables(const Config & config,
                             const Context & context,
                             const DisplayViewTransform & tr,
                             ContextRcPtr & usedContextVars);

bool CollectContextVariables(const Config & config, 
                             const Context & context,
                             const FileTransform & tr,
                             ContextRcPtr & usedContextVars);

bool CollectContextVariables(const Config & config, 
                             const Context & context,
                             const GroupTransform & tr,
                             ContextRcPtr & usedContextVars);

bool CollectContextVariables(const Config & config, 
                             const Context & context,
                             const LookTransform & tr,
                             ContextRcPtr & usedContextVars);


// Helper methods to search for context variables.

bool CollectContextVariables(const Config & config,
                             const Context & context,
                             TransformDirection direction,
                             const Look & look,
                             ContextRcPtr & usedContextVars);

bool CollectContextVariables(const Config & config, 
                             const Context & context,
                             ConstColorSpaceRcPtr & tr,
                             ContextRcPtr & usedContextVars);

} // namespace OCIO_NAMESPACE

#endif
