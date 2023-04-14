// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "ConfigUtils.h"
#include "MathUtils.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{

namespace ConfigUtils
{

//////////////////////////////////////////////////////////////////////////////////////

// The following code needs to know the names of some of the color spaces in the
// built-in default config.  If the color space names of that config are ever
// modified, the following strings should be kept in sync.

// Name of the sRGB space in the built-in config.
//
const char * getSRGBColorSpaceName()
{
    constexpr const char * srgbColorSpaceName = "sRGB - Texture";
    return srgbColorSpaceName;
}

// Define the set of candidate built-in default config reference linear color spaces that 
// will be used when searching through the source config. If the source config scene-referred 
// reference space is the equivalent of one of these spaces, it should be possible to identify 
// it with the following heuristics.
//
const char * getBuiltinLinearSpaceName(int index)
{
    constexpr const char * builtinLinearSpaces[] = { "ACES2065-1", 
                                                     "ACEScg", 
                                                     "Linear Rec.709 (sRGB)", 
                                                     "Linear P3-D65",
                                                     "Linear Rec.2020" };
    return builtinLinearSpaces[Clamp(index, 0, 4)];
}

// The number of items available from getBuiltinLinearSpaceName.  (Obviously, update this
// integer if that list changes.)
//
inline int getNumberOfbuiltinLinearSpaces()
{
    return 5;
}

//////////////////////////////////////////////////////////////////////////////////////

// Use the interchange roles in the pair of provided configs to return the color space
// names to be used for the conversion between the provided pair of color spaces.
// Note that the color space names returned depend on the image state of the provided
// color spaces.  The returned color space names are the names that the interchange
// roles point to and the function checks that they exist.  An exception is raised if
// there are problems with the input arguments or if the interchange roles are present
// but point to color spaces that don't exist.  If the interchange roles are simply
// not present, no exception is thrown but false is returned.  If the returned interchange
// color space names are present and exist, true is returned.
//
// This function does NOT use any heuristics.
//
// [out] srcInterchangeCSName -- Name of the interchange color space from the src config.
// [out] dstInterchangeCSName -- Name of the interchange color space from the dst config.
// [out] interchangeType -- The ReferenceSpaceType of the interchange color space.
// srcConfig -- Source config object.
// srcName -- Name of the color space to be converted from the source config.
//      May be empty if the source color space is unknown.
// dstConfig -- Destination config object.
// dstName -- Name of the color space to be converted from the destination config.
// Returns True if the necessary interchange roles were found.
//
bool GetInterchangeRolesForColorSpaceConversion(const char ** srcInterchangeCSName,
                                                const char ** dstInterchangeCSName,
                                                ReferenceSpaceType & interchangeType,
                                                const ConstConfigRcPtr & srcConfig,
                                                const char * srcName,
                                                const ConstConfigRcPtr & dstConfig,
                                                const char * dstName)
{
    ConstColorSpaceRcPtr dstColorSpace = dstConfig->getColorSpace(dstName);
    if (!dstColorSpace)
    {
        std::ostringstream os;
        os << "Could not find destination color space '" << dstName << "'.";
        throw Exception(os.str().c_str());
    }

    interchangeType = REFERENCE_SPACE_SCENE;

    if (srcName && !*srcName)
    {
        // If srcName is empty, just use the reference type of the destination side.
        // In this scenario, the source color space is unknown but the assumption is
        // that when it is found it will have the same reference space type as the
        // destination color space.
        if (dstColorSpace->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY)
        {
            interchangeType = REFERENCE_SPACE_DISPLAY;
        }
    }
    else
    {
        ConstColorSpaceRcPtr srcColorSpace = srcConfig->getColorSpace(srcName);
        if (!srcColorSpace)
        {
            std::ostringstream os;
            os << "Could not find source color space '" << srcName << "'.";
            throw Exception(os.str().c_str());
        }

        // Only use the display-referred reference space if both color spaces are 
        // display-referred.  If only one of the spaces is display-referred, it's
        // better to use the scene-referred space since the conversion to scene-
        // referred will happen within the config that has the display-referred
        // color space.  The config with the scene-referred color space may not
        // even have a default view transform to use.  In addition, it's important
        // that this function always use the same reference space even if the order
        // of src & dst is swapped, so the result is the inverse (which it might
        // not be if the view transform in the opposite config is used).
        if (srcColorSpace->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY  &&
            dstColorSpace->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY)
        {
            interchangeType = REFERENCE_SPACE_DISPLAY;
        }
    }

    const char * interchangeRoleName = (interchangeType == REFERENCE_SPACE_SCENE)
                                       ? ROLE_INTERCHANGE_SCENE : ROLE_INTERCHANGE_DISPLAY;

    if (!srcConfig->hasRole(interchangeRoleName))
    {
        return false;
    }
    // Get the color space name assigned to the interchange role.
    ConstColorSpaceRcPtr srcInterchangeCS = srcConfig->getColorSpace(interchangeRoleName);
    if (!srcInterchangeCS)
    {
        std::ostringstream os;
        os << "The role '" << interchangeRoleName << "' refers to a color space ";
        os << "that is missing in the source config.";
        throw Exception(os.str().c_str());
    }
    *srcInterchangeCSName = srcInterchangeCS->getName();

    if (!dstConfig->hasRole(interchangeRoleName))
    {
        return false;
    }
    // Get the color space name assigned to the interchange role.
    ConstColorSpaceRcPtr dstInterchangeCS = dstConfig->getColorSpace(interchangeRoleName);
    if (!dstInterchangeCS)
    {
        std::ostringstream os;
        os << "The role '" << interchangeRoleName << "' refers to a color space ";
        os << "that is missing in the destination config.";
        throw Exception(os.str().c_str());
    }
    *dstInterchangeCSName = dstInterchangeCS->getName();

    return true;
}

// Return true if the color space name or its aliases contains sRGB (case-insensitive).
//
bool containsSRGB(ConstColorSpaceRcPtr & cs)
{
    std::string name = StringUtils::Lower(cs->getName());
    if (StringUtils::Find(name, "srgb") != std::string::npos)
    {
        return true;
    }

    size_t nbOfAliases = cs->getNumAliases();
    for (size_t i = 0; i < nbOfAliases; i++)
    {
        if (StringUtils::Find(cs->getAlias(i), "srgb") != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

// Find a color space where isData is false and it has neither a to_ref or from_ref 
// transform.  Currently only selecting scene-referred spaces.  Note: this returns
// the first reference space found, even if it is inactive.  Returns empty if none
// are found.
//
const char * getRefSpaceName(const ConstConfigRcPtr & cfg)
{
    // It's important to support inactive spaces since sometimes the only reference space
    // may be inactive, e.g. the display-referred reference in the built-in configs.
    int nbCs = cfg->getNumColorSpaces(SEARCH_REFERENCE_SPACE_SCENE, COLORSPACE_ALL);

    for (int i = 0; i < nbCs; i++)
    {
        const char * csname = cfg->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_SCENE, 
                                                            COLORSPACE_ALL, 
                                                            i);
        ConstColorSpaceRcPtr cs = cfg->getColorSpace(csname);

        if (cs->isData())
        {
            continue;
        }
        ConstTransformRcPtr t = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
        if (t != nullptr)
        {
            continue;
        }
        t = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if (t != nullptr) 
        {
            continue;
        }

        return csname;
    }
    return "";
}

// Return false if the supplied Processor modifies any of the supplied float values
// by more than the supplied absolute tolerance amount.
//
bool isIdentityTransform(ConstProcessorRcPtr proc, 
                         std::vector<float> & RGBAvals, 
                         float absTolerance)
{
    std::vector<float> out(RGBAvals.size(), 0.f);

    PackedImageDesc desc( &RGBAvals[0], (long) RGBAvals.size() / 4, 1, CHANNEL_ORDERING_RGBA );
    PackedImageDesc descDst( &out[0], (long) RGBAvals.size() / 4, 1, CHANNEL_ORDERING_RGBA );

    ConstCPUProcessorRcPtr cpu  = proc->getOptimizedCPUProcessor(OPTIMIZATION_NONE);
    cpu->apply(desc, descDst);

    for (size_t i = 0; i < out.size(); i++)
    {
        if (!EqualWithAbsError(RGBAvals[i], out[i], absTolerance))
        {
            return false;
        }
    }

    return true;
}

// Helper to avoid color spaces that are without transforms or are data spaces.
//
bool hasNoTransform(const ConstColorSpaceRcPtr & cs)
{
    if (cs->isData())
    {
        return true;
    }
    ConstTransformRcPtr srcTransform = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
    if (!srcTransform)
    {
        srcTransform = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if (srcTransform)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    return false;
}

// Test the supplied color space against a set of color spaces in the built-in config.
// If a match is found, it indicates what reference space is used by the config.
// Return the index into the list of built-in linear spaces, or -1 if not found.
//
// srcConfig -- Source config object.
// srcRefName -- Name of a scene-referred reference color space in the src config.
// cs -- Color space from the source config to test.
// builtinConfig -- The built-in config object.
// Returns the index into the list of built-in linear spaces.
//
int getReferenceSpaceFromLinearSpace(const ConstConfigRcPtr & srcConfig, 
                                     const char * srcRefName,
                                     const ConstColorSpaceRcPtr & cs,
                                     const ConstConfigRcPtr & builtinConfig)
{
    // Currently only handling scene-referred spaces in the heuristics.
    if (cs->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY)
    {
        return -1;
    }
    // Don't check spaces without transforms / data spaces.
    if (hasNoTransform(cs))
    {
        return -1;
    }

    // Define a set of (somewhat arbitrary) RGB values to test whether the combined transform is 
    // enough of an identity.
    std::vector<float> vals = { 0.7f,  0.4f,  0.02f, 0.f,
                                0.02f, 0.6f, -0.2f,  0.f,
                                0.3f,  0.02f, 1.5f,  0.f,
                                0.f,   0.f,   0.f,   0.f,
                                1.f,   1.f,   1.f,   0.f };

    // Test the transform from the test color space to its reference space against all combinations
    // of the built-in linear color spaces.  If one of them results in an identity, that identifies
    // what the source color space and reference space are.

    for (int i = 0; i < getNumberOfbuiltinLinearSpaces(); i++)
    {
        for (int j = 0; j < getNumberOfbuiltinLinearSpaces(); j++)
        {
            // Ensure the built-in side of the conversion is never an identity, since if
            // both the src side and built-in side are an identity, it would seem as though
            // the reference space has been identified, but in fact it would not be.
            if (i != j)
            {
                ConstProcessorRcPtr proc = Config::GetProcessorFromConfigs(
                    srcConfig,
                    cs->getName(),
                    srcRefName,
                    builtinConfig,
                    getBuiltinLinearSpaceName(i),
                    getBuiltinLinearSpaceName(j));

                if (isIdentityTransform(proc, vals, 1e-3f))
                {
                    return j;
                }
            }
        }
    }

    return -1;
}

// Test the supplied color space against a set of color spaces in the built-in config
// to see if it matches an sRGB texture color space with one of a set of known primaries
// used as its reference space.  If a match is found, it indicates what reference space 
// is used by the config.  Return the index into the list of built-in linear spaces, 
// or -1 if not found.
//
// srcConfig -- Source config object.
// srcRefName -- Name of a scene-referred reference color space in the src config.
// cs -- Color space from the source config to test.
// builtinConfig -- The built-in config object.
// Returns the index into the list of built-in linear spaces.
//
int getReferenceSpaceFromSRGBSpace(const ConstConfigRcPtr & srcConfig, 
                                   const char * srcRefName,
                                   const ConstColorSpaceRcPtr cs,
                                   const ConstConfigRcPtr & builtinConfig)
{
    // Currently only handling scene-referred spaces in the heuristics.
    if (cs->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY)
    {
        return -1;
    }
    // Get a transform in the to-reference direction.
    ConstTransformRcPtr toRefTransform;
    ConstTransformRcPtr ctransform = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
    if (ctransform) 
    {
        toRefTransform = ctransform;
    }
    else
    {
        ctransform = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if (ctransform)
        {
            TransformRcPtr transform = ctransform->createEditableCopy();
            transform->setDirection(TRANSFORM_DIR_INVERSE);
            toRefTransform = transform;
        }
        else
        {
            // Don't check spaces without transforms / data spaces.
            return -1;
        }
    }

    // First check if it has the right non-linearity. The objective is to fail quickly on color 
    // spaces that are definitely not sRGB before proceeding to the longer test of guessing the 
    // reference space primaries.

    // Break point is at 0.039286, so include at least one value below this.
    std::vector<float> vals =
    {
        0.5f,  0.5f,  0.5f, 
        0.03f, 0.03f, 0.03f, 
        0.25f, 0.25f, 0.25f, 
        0.75f, 0.75f, 0.75f, 
        0.f,   0.f,   0.f, 
        1.f,   1.f ,  1.f
    };
    std::vector<float> out(vals.size(), 0.f);

    PackedImageDesc desc( &vals[0], (long) vals.size() / 3, 1, CHANNEL_ORDERING_RGB );
    PackedImageDesc descDst( &out[0], (long) vals.size() / 3, 1, CHANNEL_ORDERING_RGB );

    ConstProcessorRcPtr proc = srcConfig->getProcessor(toRefTransform, TRANSFORM_DIR_FORWARD);

    ConstCPUProcessorRcPtr cpu  = proc->getOptimizedCPUProcessor(OPTIMIZATION_NONE);
    // Convert the non-linear values to linear.
    cpu->apply(desc, descDst);

    for (size_t i = 0; i < out.size(); i++)
    {
        // Apply the sRGB function to convert the processed linear values back to non-linear.
        // Please see GammaOpUtils.cpp. This value provides continuity at the breakpoint.
        if (out[i] <= 0.0030399346397784323f)
        {
            out[i] *= 12.923210180787857f;
        }
        else
        {
            out[i] = 1.055f * std::pow(out[i], 1.f / 2.4f) - 0.055f;
        }
        // Compare against the original source values.
        if (!EqualWithAbsError(vals[i], out[i], 1e-3f))
        {
            return -1;
        }
    } 

    // Define a (somewhat arbitrary) set of RGB values to test whether the transform is in fact 
    // converting sRGB texture values to the candidate reference space. It includes 0.02 which is 
    // on the sRGB linear segment, color values, and neutral values.
    vals = { 0.7f,  0.4f,  0.02f, 0.f,
             0.02f, 0.6f,  0.2f,  0.f,
             0.3f,  0.02f, 0.5f,  0.f,
             0.f,   0.f,   0.f,   0.f,
             1.f,   1.f,   1.f,   0.f, };

    // The color space has the sRGB non-linearity. Now try combining the transform with a 
    // transform from the Built-in config that goes from a variety of reference spaces to an 
    // sRGB texture space. If the result is an identity, then that tells what the source config
    // reference space is.

    for (int i = 0; i < getNumberOfbuiltinLinearSpaces(); i++)
    {
        ConstProcessorRcPtr proc = Config::GetProcessorFromConfigs(srcConfig,
                                                                   cs->getName(),
                                                                   srcRefName,
                                                                   builtinConfig,
                                                                   getSRGBColorSpaceName(),
                                                                   getBuiltinLinearSpaceName(i));
        if (isIdentityTransform(proc, vals, 1e-3f))
        {
            return i;
        }
    }

    return -1;
}

// Identify the interchange spaces of the source config and the built-in default config
// that should be used to convert from the src color space to the built-in color space,
// or vice-versa.  Throws if no suitable spaces are found.
//
// [out] srcInterchange -- Name of the interchange color space from the source config.
// [out] builtinInterchange -- Name of the interchange color space from the built-in config.
// srcConfig -- Source config object.
// srcColorSpaceName -- Name of the color space to be converted from the source config.
// builtinConfig -- Built-in config object.
// builtinColorSpaceName -- Name of the color space to be converted from the built-in config.
//
// Throws an exception if an interchange space cannot be found.
//
void IdentifyInterchangeSpace(const char ** srcInterchange, 
                              const char ** builtinInterchange,
                              const ConstConfigRcPtr & srcConfig,
                              const char * srcColorSpaceName, 
                              const ConstConfigRcPtr & builtinConfig,
                              const char * builtinColorSpaceName)
{
    // Before resorting to heuristics, check if the configs already have the interchange
    // roles defined. 
    //
    // Note that this is the only place that srcColorSpaceName and builtinColorSpaceName 
    // are used, in order to determine whether the scene- or display-referred interchange 
    // role is most appropriate.  These color spaces are not used below for the heuristics.

    ReferenceSpaceType interchangeType;
    if ( GetInterchangeRolesForColorSpaceConversion(srcInterchange, builtinInterchange, 
                                                    interchangeType,
                                                    srcConfig, srcColorSpaceName, 
                                                    builtinConfig, builtinColorSpaceName) )
    {
        // No need for the heuristics.
        return;
    }

    // Use heuristics to try and find a color space in the source config that matches 
    // a color space in the Built-in config.

    // Currently only handling scene-referred spaces in the heuristics.
    if (builtinConfig->getColorSpace(builtinColorSpaceName)->getReferenceSpaceType() 
        == REFERENCE_SPACE_DISPLAY)
    {
        std::ostringstream os;
        os  << "The heuristics currently only support scene-referred color spaces. "
            << "Please set the interchange roles.";
        throw Exception(os.str().c_str());
    }

    // Identify the name of a reference space in the source config.
    *srcInterchange = getRefSpaceName(srcConfig);
    if (!*srcInterchange)
    {
        std::ostringstream os;
        os  << "The supplied config does not have a color space for the reference.";
        throw Exception(os.str().c_str());
    }

    // The heuristics need to create a lot of Processors and send RGB values through
    // them to try and identify a known color space.  Turn off the Processor cache in
    // the configs to avoid polluting the cache with transforms that won't be reused
    // and avoid the overhead of maintaining the cache.
    SuspendCacheGuard srcGuard(srcConfig);
    SuspendCacheGuard builtinGuard(builtinConfig);

    // Check for an sRGB texture space.
    int refColorSpacePrimsIndex = -1;
    int nbCs = srcConfig->getNumColorSpaces();
    for (int i = 0; i < nbCs; i++)
    {
        ConstColorSpaceRcPtr cs = srcConfig->getColorSpace(srcConfig->getColorSpaceNameByIndex(i));
        if (containsSRGB(cs))
        {
            refColorSpacePrimsIndex = getReferenceSpaceFromSRGBSpace(srcConfig, 
                                                                     *srcInterchange,
                                                                     cs, 
                                                                     builtinConfig);
            // Break out when a match is found.
            if (refColorSpacePrimsIndex > -1) break; 
        }
    }

    if (refColorSpacePrimsIndex < 0)
    {
        // Check for a scene-linear space with known primaries.
        nbCs = srcConfig->getNumColorSpaces();
        for (int i = 0; i < nbCs; i++)
        {
            ConstColorSpaceRcPtr cs = srcConfig->getColorSpace(srcConfig->getColorSpaceNameByIndex(i));
            if (srcConfig->isColorSpaceLinear(cs->getName(), REFERENCE_SPACE_SCENE))
            {
                refColorSpacePrimsIndex = getReferenceSpaceFromLinearSpace(srcConfig,
                                                                           *srcInterchange,
                                                                           cs, 
                                                                           builtinConfig);
                // Break out when a match is found.
                if (refColorSpacePrimsIndex > -1) break; 
            }
        }
    }

    if (refColorSpacePrimsIndex > -1)
    {
        *builtinInterchange = getBuiltinLinearSpaceName(refColorSpacePrimsIndex);
    }
    else
    {
        std::ostringstream os;
        os  << "Heuristics were not able to find a known color space in the provided config. "
            << "Please set the interchange roles.";
        throw Exception(os.str().c_str());
    }
}

// Try to find the name of a color space in the source config that is equivalent to the
// specified color space from the provided built-in config.  Only active color spaces 
// are searched.
//
// srcConfig -- The source config object to search.
// builtinConfig -- The built-in config object containing the desired color space.
// builtinColorSpaceName -- Name of the desired color space from the built-in config.
// Returns the name of the color space in the source config.
//
// \throw Exception if an interchange space cannot be found or the equivalent space cannot be found.
//
const char * IdentifyBuiltinColorSpace(const ConstConfigRcPtr & srcConfig,
                                       const ConstConfigRcPtr & builtinConfig,
                                       const char * builtinColorSpaceName)
{
    // Note: Technically, the built-in config could be any config, if the interchange
    // roles are set in both configs, and the supplied built-in config supports the list
    // of color spaces returned by getBuiltinLinearSpaceName.

    ConstColorSpaceRcPtr builtinColorSpace = builtinConfig->getColorSpace(builtinColorSpaceName);
    if (!builtinColorSpace)
    {
        std::ostringstream os;
        os  << "Built-in config does not contain the requested color space: " 
            << builtinColorSpaceName << ".";
        throw Exception(os.str().c_str());
    }

    ReferenceSpaceType builtinRefSpaceType = builtinColorSpace->getReferenceSpaceType();

    // Identify interchange spaces.  Passing an empty string for the source color space
    // means that only the builtinColorSpace will be used to determine the reference
    // space type of the interchange role.  Will throw if the space cannot be found.
    // Only color spaces in the srcConfig that have the same reference type as the
    // builtinColorSpace will be searched by the heuristics below.
    const char * srcInterchangeName = nullptr;
    const char * builtinInterchangeName = nullptr;
    IdentifyInterchangeSpace(&srcInterchangeName, 
                             &builtinInterchangeName, 
                             srcConfig, 
                             "", 
                             builtinConfig,
                             builtinColorSpaceName);

    // The heuristics need to create a lot of Processors and send RGB values through
    // them to try and identify a known color space.  Turn off the Processor cache in
    // the configs to avoid polluting the cache with transforms that won't be reused
    // and avoid the overhead of maintaining the cache.
    SuspendCacheGuard srcGuard(srcConfig);
    SuspendCacheGuard builtinGuard(builtinConfig);

    if (*builtinInterchangeName)
    {
        std::vector<float> vals = { 0.7f,  0.4f,  0.02f, 0.f,
                                    0.02f, 0.6f,  0.2f,  0.f,
                                    0.3f,  0.02f, 0.5f,  0.f,
                                    0.f,   0.f,   0.f,   0.f,
                                    1.f,   1.f,   1.f,   0.f };

        // Loop over each non-data color space in the source config and test if the conversion to
        // the specified space in the built-in config is an identity.
        //
        //    Note that there is a possibility that both the source and built-in sides of the
        //    transform could be an identity (e.g., if the user asks for ACES2065-1 and that is
        //    also the reference space in both configs).  However, this would not prevent the
        //    algorithm from returning the correct result, as long as the interchange spaces
        //    were correctly identified.

        int nbCs = srcConfig->getNumColorSpaces();
        for (int i = 0; i < nbCs; i++)
        {
            ConstColorSpaceRcPtr cs = srcConfig->getColorSpace(srcConfig->getColorSpaceNameByIndex(i));
            if (cs->isData() || (cs->getReferenceSpaceType() != builtinRefSpaceType))
            {
                continue;
            }

            ConstProcessorRcPtr proc = Config::GetProcessorFromConfigs(srcConfig,
                                                                       cs->getName(),
                                                                       srcInterchangeName,
                                                                       builtinConfig,
                                                                       builtinColorSpaceName,
                                                                       builtinInterchangeName);
            if (isIdentityTransform(proc, vals, 1e-3f))
            {
                return cs->getName();
            }
        }
    }

    std::ostringstream os;
    os  << "Heuristics were not able to find an equivalent to the requested color space: "
        << builtinColorSpaceName << ".";
    throw Exception(os.str().c_str());
}

}  // namespace ConfigUtils

}  // namespace OCIO_NAMESPACE
