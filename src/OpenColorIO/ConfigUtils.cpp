// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <pystring.h>

#include "ConfigUtils.h"
#include "MathUtils.h"
#include "utils/StringUtils.h"
#include "Logging.h"

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
bool containsSRGB(const ConstColorSpaceRcPtr & cs)
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

// Find the first scene-referred color space with isdata: true.
//
const char * getDataSpaceName(const ConstConfigRcPtr & cfg)
{
    int nbCs = cfg->getNumColorSpaces(SEARCH_REFERENCE_SPACE_SCENE, COLORSPACE_ALL);

    for (int i = 0; i < nbCs; i++)
    {
        const char * csname = cfg->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_SCENE, 
                                                            COLORSPACE_ALL, 
                                                            i);
        ConstColorSpaceRcPtr cs = cfg->getColorSpace(csname);

        if (cs->isData())
        {
            return csname;
        }
    }
    return "";
}

// Return false if the supplied Processor modifies any of the supplied float values
// by more than the supplied absolute tolerance amount.
//
bool isIdentityTransform(const ConstProcessorRcPtr & proc, 
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

// Determine whether a Processor contains a MatrixTransform with off-diagonal coefficients.
//
bool hasNonTrivialMatrixTransform(const ConstProcessorRcPtr & proc)
{
    GroupTransformRcPtr gt = proc->createGroupTransform();
    // The result of createGroupTransform only contains transforms that correspond to ops,
    // in other words, there are no complex transforms such as File, Builtin, or ColorSpace,
    // and the only GroupTransform is the enclosing one.
    for (int i = 0; i < gt->getNumTransforms(); ++i)
    {
        ConstTransformRcPtr transform = gt->getTransform(i);
        if (transform->getTransformType() == TRANSFORM_TYPE_MATRIX)
        {
            // Check that there is a significant off-diagonal coefficient in the matrix.
            // This is to avoid matrices that are not actual color primary conversions,
            // for example, the scale and offset that are sometimes prepended to a Lut1D.
            ConstMatrixTransformRcPtr mtx = DynamicPtrCast<const MatrixTransform>(transform);
            double values[16];
            mtx->getMatrix(values);
            for (int j = 0; j < 3 ; ++j)      // only checking rgb, not alpha
            {
                for (int k = 0; k < 3 ; ++k)  // only checking rgb, not alpha
                {
                    if (j != k)
                    {
                        if (std::abs(values[j * 4 + k]) > 0.1)
                        {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

// Determine if the transform contains a type that is inappropriate for the heuristics.
//
bool containsBlockedTransform(const ConstTransformRcPtr & transform)
{
    // If it's a GroupTransform, need to recurse into it to check the contents.
    if (transform->getTransformType() == TRANSFORM_TYPE_GROUP)
    {
        ConstGroupTransformRcPtr gt = DynamicPtrCast<const GroupTransform>(transform);
        for (int i = 0; i < gt->getNumTransforms(); ++i)
        {
            ConstTransformRcPtr tr = gt->getTransform(i);
            if (containsBlockedTransform(tr))
            {
                return true;
            }
        }
    }

    // Prevent FileTransforms from being used, except for spi1d and spimtx since these
    // may be used with OCIO v1 configs to implement the type of color spaces the heuristics
    // are designed to look for.  (E.g. The sRGB Texture space in the legacy ACES configs.)
    else if (transform->getTransformType() == TRANSFORM_TYPE_FILE)
    {
        ConstFileTransformRcPtr ft = DynamicPtrCast<const FileTransform>(transform);
        std::string filepath = ft->getSrc();
        std::string root, extension;
        pystring::os::path::splitext(root, extension, filepath);
        extension = StringUtils::Lower(extension);
        if (extension != ".spi1d" && extension != ".spimtx")
        {
            return true;
        }
    }

    // Prevent transforms that may be hiding a FileTransform.
    else if (transform->getTransformType() == TRANSFORM_TYPE_COLORSPACE
     || transform->getTransformType() == TRANSFORM_TYPE_DISPLAY_VIEW
     || transform->getTransformType() == TRANSFORM_TYPE_LOOK)
    {
        return true;
    }

    // Putting this here since Lut3D is the main type of transform to avoid,
    // however given that the input transform comes directly from a color space
    // and has not been converted to a processor yet, it should never actually
    // have a transform of this type (it would still be a FileTransform).
    else if (transform->getTransformType() == TRANSFORM_TYPE_LUT3D)
    {
        return true;
    }

    return false;
}

// Look at the to_ref/from_ref transforms in the color space and exclude color spaces
// that are probably not what the heuristics are looking for and could be prohibitively
// expensive to fully check.
//
// Because this check is done before a processor is built, it is inexpensive but it may
// be inaccurate.  In other words, it's possible that this check will exclude some 
// reasonable color spaces, but that's better than trying to invert 3d-LUTs, etc.
//
// cs -- Color space object to check.
// refSpaceType -- Exclude if the color space is not of the same reference space type.
// blockRefSpaces -- Exclude the color space if it does not have any transforms.
//
bool excludeColorSpaceFromHeuristics(const ConstColorSpaceRcPtr & cs, 
                                     ReferenceSpaceType refSpaceType,
                                     bool blockRefSpaces)
{
    if (cs->isData())
    {
        return true;
    }

    if (cs->getReferenceSpaceType() != refSpaceType)
    {
        return true;
    }

    ConstTransformRcPtr transform = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
    if (transform)
    {
        // The to_ref transform is what the heuristics try to use first, so if that
        // does not contain problematic transforms, it's ok to proceed without checking
        // the from_ref transform.

        return containsBlockedTransform(transform);
    }

    // There is no to_ref transform, check if the from_ref is present.
    transform = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
    if (transform)
    {
        return containsBlockedTransform(transform);
    }
    // Color space contains no transforms (it's a reference space).
    else
    {
        return blockRefSpaces;
    }
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
                                   const ConstColorSpaceRcPtr & cs,
                                   const ConstConfigRcPtr & builtinConfig)
{
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

    // Ensure that the color space is not only an sRGB curve, it needs to have a color matrix
    // too or else the last step below could succeed by pairing the built-in sRGB space with
    // a linear space that cancels out the matrix in the built-in sRGB space.
        // NB: This is being done after the getProcessor call rather than simply looking at
        // the raw color space transform contents since once it becomes a processor, the complex
        // transforms (e.g. File, ColorSpace, Builtins) that could be hiding a matrix are 
        // converted into ops.
    if (!hasNonTrivialMatrixTransform(proc))
    {
        return -1;
    }

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
        // (This assumes equal channel sRGB values remain so in the reference space of src config.)
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

    // At this point, cs has the expected non-linearity and a non-trivial matrix to its reference space.
    // Now try to identify the matrix.
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
    if (!**srcInterchange)
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
            // Exclude color spaces that may be too expensive to test or otherwise inappropriate.
            // Currently only handling scene-referred spaces in the heuristics.
            if (excludeColorSpaceFromHeuristics(cs, REFERENCE_SPACE_SCENE, true))
            {
                continue;
            }

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

            // Exclude color spaces that may be too expensive to test or otherwise inappropriate.
            // Currently only handling scene-referred spaces in the heuristics.
            if (excludeColorSpaceFromHeuristics(cs, REFERENCE_SPACE_SCENE, true))
            {
                continue;
            }

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

    if (builtinColorSpace->isData())
    {
        const char * dataName = getDataSpaceName(srcConfig);
        if (!*dataName)
        {
            std::ostringstream os;
            os  << "The requested space is a data space but the supplied config does not have a data space.";
            throw Exception(os.str().c_str());
        }
        return dataName;
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

        // Loop over the active, non-excluded, color spaces in the source config and test if the
        // conversion to the specified space in the built-in config is an identity.
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

            if (excludeColorSpaceFromHeuristics(cs, builtinRefSpaceType, false))
            {
                continue;
            }

            ConstProcessorRcPtr proc = Config::GetProcessorFromConfigs(srcConfig,
                                                                       cs->getName(),
                                                                       srcInterchangeName,
                                                                       builtinConfig,
                                                                       builtinColorSpaceName,
                                                                       builtinInterchangeName);
//            if (isIdentityTransform(proc, vals, 1e-3f))
            // Tolerance is just loose enough to accept both bfd and cat02 adaptation.
            if (isIdentityTransform(proc, vals, 5e-3f))
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


// Simplify a transform by removing nested group transforms and identities.
//
ConstTransformRcPtr simplifyTransform(const ConstGroupTransformRcPtr & gt)
{
    ConstConfigRcPtr config = Config::CreateRaw();
    ConstProcessorRcPtr p = config->getProcessor(gt);
    ConstProcessorRcPtr opt = p->getOptimizedProcessor(OPTIMIZATION_DEFAULT);
    GroupTransformRcPtr finalGt = opt->createGroupTransform();
    if (finalGt->getNumTransforms() == 1)
    {
        return finalGt->getTransform(0);
    }
    return finalGt;
}

ConstTransformRcPtr invertTransform(const ConstTransformRcPtr & t)
{
    TransformRcPtr eT = t->createEditableCopy();
    eT->setDirection(TRANSFORM_DIR_INVERSE);
    return eT;
}

// Return a transform in either the to_ref or from_ref direction for this color space.
// Return an identity matrix, if the color space has no transforms.
//
ConstTransformRcPtr getTransformForDir(const ConstColorSpaceRcPtr & cs, ColorSpaceDirection dir)
{
    ColorSpaceDirection otherDir = COLORSPACE_DIR_TO_REFERENCE;
    if (dir == COLORSPACE_DIR_TO_REFERENCE)
    {
        otherDir = COLORSPACE_DIR_FROM_REFERENCE;
    }

    ConstTransformRcPtr t = cs->getTransform(dir);
    if (t) 
    {
        return t;
    }

    ConstTransformRcPtr tOther = cs->getTransform(otherDir);
    if (tOther) 
    {
        return invertTransform(tOther);
    }

    // If it's the reference space, it won't have a transform, so return an identity matrix.
    double m44[16];
    double offset4[4];
    MatrixTransform::Identity(m44, offset4);
    MatrixTransformRcPtr p = MatrixTransform::Create();
    p->setMatrix(m44);
    p->setOffset(offset4);

    return p;
}

// Get a transform to convert from the source config reference space to the
// destination config reference space.  The ref_space_type specifies whether
// to work with the scene-referred or display-referred reference space.
//
ConstTransformRcPtr getRefSpaceConverter(const ConstConfigRcPtr & srcConfig, 
                                         const ConstConfigRcPtr & dstConfig, 
                                         ReferenceSpaceType refSpaceType)
{
    ConstConfigRcPtr builtinConfig = Config::CreateFromBuiltinConfig("ocio://cg-config-latest");

    auto getColorspaceOfRefType = [](const ConstConfigRcPtr & config, 
                                     ReferenceSpaceType refType) -> const char *
    {
        // Just return the first one, doesn't matter if it's inactive or a data space.
        // All that matters is the reference space type.
        // Casting to SearchReferenceSpaceType since they have the same values.
        SearchReferenceSpaceType searchRefType = static_cast<SearchReferenceSpaceType>(refType);
        for (int i = 0; i < config->getNumColorSpaces(searchRefType, COLORSPACE_ALL); i++)
        {
            const char * name = config->getColorSpaceNameByIndex(searchRefType, COLORSPACE_ALL, i);
            ConstColorSpaceRcPtr cs = config->getColorSpace(name);
            return cs->getName();
        }

        throw Exception("Config is lacking any color spaces of the requested reference space type.");
    };

    // Identify an interchange space for the src config.
    // Note that the interchange space will always be a linear color space.
    // TODO: IdentifyInterchangeSpace currently fails if the config does not have
    //  a color space for the reference space.
    // TODO: IdentifyInterchangeSpace will fail in the display-referred case if the
    //  config does not have the cie_xyz_d65_interchange role.
    const char * srcInterchange = nullptr;
    const char * srcBuiltinInterchange = nullptr;
    Config::IdentifyInterchangeSpace(&srcInterchange,
                                     &srcBuiltinInterchange,
                                     srcConfig,
                                     getColorspaceOfRefType(srcConfig, refSpaceType),
                                     builtinConfig,
                                     getColorspaceOfRefType(builtinConfig, refSpaceType));
// std::cout << "srcInterchange: " << srcInterchange << "\n";

    // Identify an interchange space for the dst config.
    // Note that the interchange space will always be a linear color space.
    const char * dstInterchange = nullptr;
    const char * dstBuiltinInterchange = nullptr;
    Config::IdentifyInterchangeSpace(&dstInterchange,
                                     &dstBuiltinInterchange,
                                     dstConfig,
                                     getColorspaceOfRefType(dstConfig, refSpaceType),
                                     builtinConfig,
                                     getColorspaceOfRefType(builtinConfig, refSpaceType));
// std::cout << "dstInterchange: " << dstInterchange << "\n";

    // Get the from_ref transform from the srcInterchange space.
    ConstTransformRcPtr srcFromRef = getTransformForDir(srcConfig->getColorSpace(srcInterchange), 
                                                        COLORSPACE_DIR_FROM_REFERENCE);

    // Get a conversion from one builtin interchange to another
    ColorSpaceTransformRcPtr cst = ColorSpaceTransform::Create();
    GroupTransformRcPtr srcBuiltinToDstBuiltin;
    if (srcBuiltinInterchange && *srcBuiltinInterchange &&
        dstBuiltinInterchange && *dstBuiltinInterchange)
    {
        // srcBuiltinInterchange and dstBuiltinInterchange are not empty.
        cst->setSrc(srcBuiltinInterchange);
        cst->setDst(dstBuiltinInterchange);

        srcBuiltinToDstBuiltin = builtinConfig->getProcessor(cst)->createGroupTransform();
    }

    // Append to_ref transform from the dstInterchange space.
    ConstTransformRcPtr dstToRef = getTransformForDir(dstConfig->getColorSpace(dstInterchange), 
                                                      COLORSPACE_DIR_TO_REFERENCE);

    // Combine into a group transform.
    // Note: Some of these pieces may be identities but the whole thing needs to get
    // simplified/optimized after being combined with the existing transform anyway
    // since one of these pieces may be the inverse of a color space's existing transform.
    GroupTransformRcPtr gt = GroupTransform::Create();
//     gt->appendTransform(srcFromRef->createEditableCopy());
//     gt->appendTransform(srcBuiltinToDstBuiltin);
//     gt->appendTransform(dstToRef->createEditableCopy());
// std::cout << "srcFromRef: " << *srcFromRef << "\n";
// std::cout << "srcToDstBuiltin: " << *srcBuiltinToDstBuiltin << "\n";
// std::cout << "dstToRef: " << *dstToRef << "\n";
// std::cout << "simplified: " << *simplifyTransform(gt) << "\n\n";

    // FIXME: If the src or dst contain FileTransforms, resolve them so there is no dependence
    // on the search_path of the original configs. This is necessary for simplifyTransform
    // below but would fail if if the conversion involved a transform that may not appear
    // in a config, such as a LUT.
    gt->appendTransform(srcConfig->getProcessor(srcFromRef)->createGroupTransform());
    gt->appendTransform(srcBuiltinToDstBuiltin);
    gt->appendTransform(dstConfig->getProcessor(dstToRef)->createGroupTransform());

    // TODO: Need to ensure that gt would never contain a file transform.
    // for (size_t t = 0; t < gt->getNumTransforms(); t++)
    // {
    //     if (gt->getTransform(t)->getTransformType() == TRANSFORM_TYPE_FILE)
    //     {
    //         throw Exception("Cannot contain FileTransform.");
    //     }
    // }
    return simplifyTransform(gt);
}

bool transformIsEmpty(const ConstTransformRcPtr & tr)
{
    if (tr->getTransformType() == TRANSFORM_TYPE_GROUP)
    {
        ConstGroupTransformRcPtr gt = DynamicPtrCast<const GroupTransform>(tr);
        if (gt->getNumTransforms() == 0)
        {
            return true;
        }
    }
    return false;
}

// Update the reference space used by a color space's transforms.
// The argument is a group transform that converts from the current to the new ref. space.
//
void updateReferenceColorspace(ColorSpaceRcPtr & cs, 
                               const ConstTransformRcPtr & toNewReferenceTransform)
{
    if (!toNewReferenceTransform || !cs)
    {
        std::ostringstream os;
        os << "Could not update reference space, converter transform was not initialized.";
        throw Exception(os.str().c_str());
    }

    if (transformIsEmpty(toNewReferenceTransform))
    {
        return;
    }

    ConstTransformRcPtr transformTo = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
    if (transformTo)
    {
        GroupTransformRcPtr gt = GroupTransform::Create();
        gt->appendTransform(transformTo->createEditableCopy());
        gt->appendTransform(toNewReferenceTransform->createEditableCopy());

        // NB: Don't want to call simplify_transforms on gt since it would do things like
        // expand built-in or file transforms. But as a result, there could be transforms that
        // appear more complex than necessary. In some cases there could be color spaces
        // with transforms present that would actually simplify into an identity.  In other
        // words there could be color spaces that are effectively the reference space that
        // have from_ref or to_ref transforms.
        cs->setTransform(gt, COLORSPACE_DIR_TO_REFERENCE);
    }

    ConstTransformRcPtr transformFrom = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
    if (transformFrom)
    {
        ConstTransformRcPtr inv = invertTransform(toNewReferenceTransform);
        GroupTransformRcPtr gt = GroupTransform::Create();
        gt->appendTransform(inv->createEditableCopy());
        gt->appendTransform(transformFrom->createEditableCopy());
        cs->setTransform(gt, COLORSPACE_DIR_FROM_REFERENCE);
    }

    if (transformTo == nullptr && transformFrom == nullptr && !cs->isData())
    {
        GroupTransformRcPtr gt = GroupTransform::Create();
        gt->appendTransform(toNewReferenceTransform->createEditableCopy());
        cs->setTransform(gt, COLORSPACE_DIR_TO_REFERENCE);
    }
}

// Update the transforms in a view transform to adapt the reference spaces.
// Note that the from_ref transform converts from the scene-referred reference space to
// the display-referred reference space.
//
void updateReferenceView(ViewTransformRcPtr & vt, 
                         const ConstTransformRcPtr & toNewSceneReferenceTransform,
                         const ConstTransformRcPtr & toNewDisplayReferenceTransform)
{
    if (!toNewSceneReferenceTransform || !toNewDisplayReferenceTransform || !vt)
    {
        std::ostringstream os;
        os << "Could not update view transform reference spaces, converter transforms were not initialized.";
        throw Exception(os.str().c_str());
    }

    // FIXME: Is any more error checking needed?

    const bool emptySceneSide = transformIsEmpty(toNewSceneReferenceTransform);
    const bool emptyDisplaySide = transformIsEmpty(toNewDisplayReferenceTransform);

    if (emptySceneSide && emptyDisplaySide) return;

    ConstTransformRcPtr transformTo = vt->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE);
    if (transformTo)
    {
        GroupTransformRcPtr gt = GroupTransform::Create();
        if (!emptyDisplaySide)
        {
            ConstTransformRcPtr inv = invertTransform(toNewDisplayReferenceTransform);
            gt->appendTransform(inv->createEditableCopy());
        }

        gt->appendTransform(transformTo->createEditableCopy());

        if (vt->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY)
        {
            // Use the converter to display reference on both sides.
            if (!emptyDisplaySide)
            {
                gt->appendTransform(toNewDisplayReferenceTransform->createEditableCopy());
            }
        }
        else
        {
            if (!emptySceneSide)
            {
                gt->appendTransform(toNewSceneReferenceTransform->createEditableCopy());
            }
        }
        vt->setTransform(gt, VIEWTRANSFORM_DIR_TO_REFERENCE);
    }

    ConstTransformRcPtr transformFrom = vt->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE);
    if (transformFrom)
    {
        GroupTransformRcPtr gt = GroupTransform::Create();

        if (vt->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY)
        {
            // Use the converter to display reference on both sides.
            if (!emptyDisplaySide)
            {
                ConstTransformRcPtr inv = invertTransform(toNewDisplayReferenceTransform);
                gt->appendTransform(inv->createEditableCopy());
            }
        }
        else
        {
            if (!emptySceneSide)
            {
                ConstTransformRcPtr inv = invertTransform(toNewSceneReferenceTransform); 
                gt->appendTransform(inv->createEditableCopy());
            }
        }

        gt->appendTransform(transformFrom->createEditableCopy());

        if (!emptyDisplaySide)
        {
            gt->appendTransform(toNewDisplayReferenceTransform->createEditableCopy());
        }
        vt->setTransform(gt, VIEWTRANSFORM_DIR_FROM_REFERENCE);
    }

    // Note that Config::addViewTransform prevents creating a view transform that 
    // has no transforms, so at least one direction will be present.
}

// FIXME: Make this a functional.
bool hasColorSpaceRefType(const ConstConfigRcPtr & config, ReferenceSpaceType refType)
{
    SearchReferenceSpaceType searchRefType = static_cast<SearchReferenceSpaceType>(refType);
    int n = config->getNumColorSpaces(searchRefType, COLORSPACE_ALL);
    return n > 0;
}

void initializeRefSpaceConverters(ConstTransformRcPtr & inputToBaseGtScene,
                                  ConstTransformRcPtr & inputToBaseGtDisplay,
                                  const ConstConfigRcPtr & baseConfig,
                                  const ConstConfigRcPtr & inputConfig)
{
    // Note: The base config reference space is always used, regardless of strategy.
//std::cout << "initializing ref converters\n";

    if (hasColorSpaceRefType(baseConfig, REFERENCE_SPACE_SCENE) &&
        hasColorSpaceRefType(inputConfig, REFERENCE_SPACE_SCENE))
    {
//         try
//         {
            // This may throw.
            inputToBaseGtScene = getRefSpaceConverter(
                inputConfig,
                baseConfig,
                REFERENCE_SPACE_SCENE
            );
//         }
//         catch(const Exception & e)
//         {
//             LogError(e.what());
//         }
    }
    else
    {
        // Always need to initialize both transforms, even if they're empty.
        inputToBaseGtScene = GroupTransform::Create();
    }

    // Only attempt to build the converter if the input config has this type of
    // reference space. Using the input config for this determination since it is
    // only input config color spaces whose reference space is converted.
    if (hasColorSpaceRefType(baseConfig, REFERENCE_SPACE_DISPLAY) &&
        hasColorSpaceRefType(inputConfig, REFERENCE_SPACE_DISPLAY))
    {
//         try
//         {
            inputToBaseGtDisplay = getRefSpaceConverter(
                inputConfig,
                baseConfig,
                REFERENCE_SPACE_DISPLAY
            );
//         }
//         catch(const Exception & e)
//         {
//             LogError(e.what());
//         }
    }
    else
    {
        // Always need to initialize both transforms, even if they're empty.
        inputToBaseGtDisplay = GroupTransform::Create();
    }
}

bool calcColorSpaceFingerprint(std::vector<float> & fingerprint, const ColorSpaceFingerprints & fingerprints, const ConstConfigRcPtr & config, const ConstColorSpaceRcPtr & cs)
{
    bool skipColorSpace = false;
    // Define a set of (somewhat arbitrary) RGB values to test whether the combined transform is 
    // enough of an identity.
    // TODO: Should we check values outside [0,1]?

//     std::vector<float> RGBAvals = { 0.4f,  0.23f,  0.07f, 0.f,
//                                     0.35f, 0.66f,  0.16f,  0.f,
//                                     0.2f,  0.11f, 0.86f,  0.f,
//                                     0.f,   0.f,   0.f,   0.f,
//                                     0.04f,   0.03f,   0.02f,   0.f,
//                                     1.f,   1.f,   1.f,   0.f };

//     std::vector<float> RGBAvals = { 0.7f,  0.4f,  0.02f, 0.f,
//                                     0.02f, 0.6f,  0.2f,  0.f,
//                                     0.3f,  0.02f, 0.5f,  0.f,
//                                     0.f,   0.f,   0.f,   0.f,
//                                     1.f,   1.f,   1.f,   0.f };

// FIXME: THIS BREAKS THE TESTS

//             std::vector<float> vals = { 0.7f,  0.4f,  0.02f, 0.f,
//                                         0.02f, 0.6f, -0.2f,  0.f,
//                                         0.3f,  0.02f, 1.5f,  0.f,
//                                         0.f,   0.f,   0.f,   0.f,
//                                         1.f,   1.f,   1.f,   0.f };

    ConstTransformRcPtr fromRef = getTransformForDir(cs, COLORSPACE_DIR_FROM_REFERENCE);

    ConstProcessorRcPtr p;
    try
    {
        p = config->getProcessor(fromRef);
    }
    catch (...) 
    { 
        // If the transform doesn't validate (singular matrix, etc.), don't consider it.
        return true;
    }

    // TODO: Get a group transform and don't bother if it contains a 3d-lut.
    // With optimization-none, it should not invert a 3d-lut, regardless of direction?

    if (cs->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY)
    {
        fingerprint = fingerprints.displayRefTestVals;
    }
    else
    {
        fingerprint = fingerprints.sceneRefTestVals;
    }
    const size_t n = fingerprint.size();
    PackedImageDesc desc( &fingerprint[0], (long) n / 4, 1, CHANNEL_ORDERING_RGBA );

    ConstCPUProcessorRcPtr cpu  = p->getOptimizedCPUProcessor(OPTIMIZATION_NONE);
    cpu->apply(desc);

    // TODO: add a cacheID to GroupTransform using the opData IDs, use that to do an easier comparison.
    // TODO: compare to_refs to to_refs.
    return skipColorSpace;
}

void initializeTestVals(ColorSpaceFingerprints & fingerprints, const ConstConfigRcPtr & config)
{
    // Define a set of test values that are slightly inside the Rec.709 gamut
    // for the most common scene-referred and display-referred reference spaces.

    std::vector<float> ACESvals = { 
        0.401273353908f, 0.089901034233f, 0.025611641641f, 0.f, // lin_rec709 {0.9, 0.01, 0.01}
        0.350859941355f, 0.733961091587f, 0.109276432439f, 0.f, // lin_rec709 {0.1, 0.09, 0.01}
        0.171696591718f, 0.104272268468f, 0.786227391453f, 0.f, // lin_rec709 {0.1, 0.02, 0.09}
        0.f            , 0.f            , 0.f            , 0.5f,
        0.037018876439f, 0.030827687576f, 0.021641700645f, 0.f, // lin_rec709 {0.05, 0.03, 0.02}
        1.f            , 1.f            , 1.f            , 1.f };

    std::vector<float> XYZvals = { 
        0.376532370617f, 0.199248715226f, 0.028095006164f, 0.f, // lin_rec709 {0.9, 0.01, 0.01}
        0.327754621322f, 0.646500124103f, 0.116973931525f, 0.f, // lin_rec709 {0.1, 0.09, 0.01}
        0.173708304342f, 0.081402847459f, 0.858056140808f, 0.f, // lin_rec709 {0.1, 0.02, 0.09}
        0.f            , 0.f            , 0.f            , 0.5f,
        0.034956685913f, 0.033530856964f, 0.023553027375f, 0.f, // lin_rec709 {0.05, 0.03, 0.02}
        0.950455927052f, 1.f            , 1.089057750760f, 1.f };

    // Try to convert to the actual reference spaces of the config.

    fingerprints.sceneRefTestVals = ACESvals;
    fingerprints.displayRefTestVals = XYZvals;

    ConstProcessorRcPtr p;
    try
    {
        // First check if the config recognizes one of the common names.
        ConstColorSpaceRcPtr cs;
        cs = config->getColorSpace("aces_interchange");
        if (!cs)
        {
            cs = config->getColorSpace("ACES2065-1");
            if (!cs)
            {
                cs = config->getColorSpace("lin_ap0_scene");
                if (!cs)
                {
                    // Otherwise, see if it's present using a different name.
// std::cout << "--> need lin_rec709\n";
                    ConstConfigRcPtr builtinConfig = Config::CreateFromBuiltinConfig("ocio://cg-config-latest");
                    // This throws if it cannot find the requested space.
                    const char * cs_name = 
                        Config::IdentifyBuiltinColorSpace(config, builtinConfig, "aces_interchange");
                    cs = config->getColorSpace(cs_name);
// std::cout << "--> identified lin_rec709\n";
                }
            }
        }
//        else
// std::cout << "--> found lin_rec709\n";

        ConstTransformRcPtr toRef = getTransformForDir(cs, COLORSPACE_DIR_TO_REFERENCE);
        p = config->getProcessor(toRef);

        const size_t n = ACESvals.size();
        std::vector<float> out(n, 0.f);

        PackedImageDesc descSrc( &ACESvals[0], (long) n / 4, 1, CHANNEL_ORDERING_RGBA );
        PackedImageDesc descDst( &out[0], (long) n / 4, 1, CHANNEL_ORDERING_RGBA );

        ConstCPUProcessorRcPtr cpu  = p->getOptimizedCPUProcessor(OPTIMIZATION_NONE);
        cpu->apply(descSrc, descDst);

        fingerprints.sceneRefTestVals = out;
// std::cout << "--> computed lin_rec709\n";
    }
    catch (...) 
    { 
// std::cout << "--> exception lin_rec709\n";
        fingerprints.sceneRefTestVals = ACESvals;
    }

    const int m = config->getNumColorSpaces(SEARCH_REFERENCE_SPACE_DISPLAY, COLORSPACE_ALL);
    if (m == 0)
    {
        return;
    }

    try
    {
        // First check if the config recognizes one of the common names.
        ConstColorSpaceRcPtr cs;
        cs = config->getColorSpace("cie_xyz_d65_interchange");
        if (!cs)
        {
            cs = config->getColorSpace("CIE-XYZ-D65");
            if (!cs)
            {
                cs = config->getColorSpace("CIE XYZ-D65");
                if (!cs)
                {
                    // Otherwise, see if it's present using a different name.
// std::cout << "--> need xyz\n";
                    ConstConfigRcPtr builtinConfig = Config::CreateFromBuiltinConfig("ocio://cg-config-latest");
                    // TODO: verify this will convert to the same ref space type.
                    const char * cs_name = 
                        Config::IdentifyBuiltinColorSpace(config, builtinConfig, "cie_xyz_d65_interchange");
                    cs = config->getColorSpace(cs_name);
// std::cout << "--> identified xyz\n";
                }
            }
        }
//        else
// std::cout << "--> found xyz\n";

        ConstTransformRcPtr toRef = getTransformForDir(cs, COLORSPACE_DIR_TO_REFERENCE);
        p = config->getProcessor(toRef);

        const size_t n = XYZvals.size();
        std::vector<float> out(n, 0.f);

        PackedImageDesc descSrc( &XYZvals[0], (long) n / 4, 1, CHANNEL_ORDERING_RGBA );
        PackedImageDesc descDst( &out[0], (long) n / 4, 1, CHANNEL_ORDERING_RGBA );

        ConstCPUProcessorRcPtr cpu  = p->getOptimizedCPUProcessor(OPTIMIZATION_NONE);
        cpu->apply(descSrc, descDst);

        fingerprints.displayRefTestVals = out;
// std::cout << "--> computed xtyz\n";
    }
    catch (...) 
    { 
// std::cout << "--> exception xyz\n";
        fingerprints.displayRefTestVals = XYZvals;
    }
}

void initializeColorSpaceFingerprints(ColorSpaceFingerprints & fingerprints, const ConstConfigRcPtr & config)
{
    SuspendCacheGuard srcGuard(config);

    initializeTestVals(fingerprints, config);

    const int n = config->getNumColorSpaces(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL);
    fingerprints.vec.clear();
    fingerprints.vec.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        const char * name = config->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL,
                                                             COLORSPACE_ALL,
                                                             i);
        ConstColorSpaceRcPtr cs = config->getColorSpace(name);
        if (!cs || cs->isData())
        {
            // Don't put data color spaces in the collection.
            continue;
        }

        ConstTransformRcPtr tFrom = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        ConstTransformRcPtr tTo = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
        if (tFrom && tTo)
        {
            // Don't bother with color spaces that have both directions defined,
            // these are more complicated and less likely to be duplicates.
            continue;
        }

        std::vector<float> fp;
        const bool skipColorSpace = calcColorSpaceFingerprint(fp, fingerprints, config, cs);
        if (!skipColorSpace)
        {
            Fingerprint fprint;
            fprint.csName = cs->getName();
            fprint.type = cs->getReferenceSpaceType();
            fprint.vals = fp;
            fingerprints.vec.push_back(fprint);
        }
    }
}

// If the base config contains a color space equivalent to inputCS, return its name.
// Return an empty string if no equivalent color space is found (within the tolerance).
// The ref_space_type specifies the type of inputCS and determines which part of the
// config is searched. 
const char * findEquivalentColorspace(const ColorSpaceFingerprints & fingerprints,
                                      const ConstConfigRcPtr & inputConfig, 
                                      const ConstColorSpaceRcPtr & inputCS)
//                                      ReferenceSpaceType refType)
{
    // The fingerprints must first be initialized from the base config.
    // NB: The inputConfig/inputCS must use the same reference space as the base config.
    // In general, this means that updateReferenceColorspace must be called on inputCS
    // before calling this function.

    // TODO: Should data spaces ever be replaced?
    if (inputCS->isData())
    {
        return "";
    }

    // Calculate the fingerprint of inputCS from inputConfig.
    std::vector<float> inputVals;
    const bool skipColorSpace = calcColorSpaceFingerprint(inputVals, fingerprints, inputConfig, inputCS);
    if (skipColorSpace)
    {
        return "";
    }

// FIXME:  Before looping over all color spaces, check to see if there's one with the same name.

    // Increase from 1e-3 to 5e-3 to allow for use of either Bradford or CAT02 adaptation.
    const float absTolerance = 5e-3f;

    // Compare to the fingerprints from the base config.
    const size_t n = inputVals.size();
    for (const auto & fp : fingerprints.vec)
    {
        // Only compare color spaces that are using the same reference space type.
        if (fp.type != inputCS->getReferenceSpaceType())
            continue;

        bool matchFound = true;
        for (size_t i = 0; i < n; i++)
        {
            // Comparison is done in the color space, not the reference space.
            // Could be linear, gamma-corrected, or log encoding.
            // TODO: Could adjust the comparison based on the color space encoding.
            if (!EqualWithAbsError(inputVals[i], fp.vals[i], absTolerance))
            {
                matchFound = false;
                continue;
            }
        }
        if (matchFound)
            return fp.csName;
    }


// TODO: Need to add isdata check to other heuristics too.

//         if (!cs->isData())
//         {
// //             ConstTransformRcPtr toRef = getTransformForDir(cs, COLORSPACE_DIR_TO_REFERENCE);
// //             GroupTransformRcPtr gt = GroupTransform::Create();
// //             gt->appendTransform(toRef->createEditableCopy());
// //             gt->appendTransform(fromRef->createEditableCopy());
// // 
// //             auto p = baseConfig->getProcessor(gt);
// 
// 
// 
//             if (isIdentityTransform(p, vals, 1e-3f))
//             {
//                 return cs->getName();
//             }
//         }
//     }

    return "";
}

}  // namespace ConfigUtils

}  // namespace OCIO_NAMESPACE
