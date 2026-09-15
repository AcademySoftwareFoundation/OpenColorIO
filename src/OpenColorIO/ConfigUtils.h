// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_CONFIG_UTILS_H
#define INCLUDED_OCIO_CONFIG_UTILS_H

#include <memory>
#include <string_view>

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

namespace ConfigUtils
{

bool GetInterchangeRolesForColorSpaceConversion(const char ** srcInterchangeCSName,
                                                const char ** dstInterchangeCSName,
                                                ReferenceSpaceType & interchangeType,
                                                const ConstConfigRcPtr & srcConfig,
                                                const char * srcName,
                                                const ConstConfigRcPtr & dstConfig,
                                                const char * dstName);

void IdentifyInterchangeSpace(const char ** srcInterchange, 
                              const char ** builtinInterchange,
                              const ConstConfigRcPtr & srcConfig,
                              const char * srcColorSpaceName, 
                              const ConstConfigRcPtr & builtinConfig, 
                              const char * builtinColorSpaceName);

const char * IdentifyBuiltinColorSpace(const ConstConfigRcPtr & srcConfig,
                                       const ConstConfigRcPtr & builtinConfig, 
                                       const char * builtinColorSpaceName);

ConstTransformRcPtr simplifyTransform(const ConstGroupTransformRcPtr & gt);
ConstTransformRcPtr invertTransform(const ConstTransformRcPtr & t);
ConstTransformRcPtr getTransformDir(const ConstColorSpaceRcPtr & cs, ColorSpaceDirection dir);

ConstTransformRcPtr getRefSpaceConverter(const ConstConfigRcPtr & srcConfig, 
                                         const ConstConfigRcPtr & dstConfig, 
                                         ReferenceSpaceType refSpaceType);

void initializeRefSpaceConverters(ConstTransformRcPtr & inputToBaseGtScene,
                                  ConstTransformRcPtr & inputToBaseGtDisplay,
                                  const ConstConfigRcPtr & baseConfig,
                                  const ConstConfigRcPtr & inputConfig);

void updateReferenceColorspace(ColorSpaceRcPtr & cs, 
                               const ConstTransformRcPtr & toNewReferenceTransform);
void updateReferenceView(ViewTransformRcPtr & vt, 
                         const ConstTransformRcPtr & toNewSceneReferenceTransform,
                         const ConstTransformRcPtr & toNewDisplayReferenceTransform);

struct Fingerprint
{
    // NB: The csName is a string rather than a pointer to the name owned by the ColorSpace
    // object since the fingerprints may be cached for longer than the lifetime of that object.
    std::string csName;
    ReferenceSpaceType type;
    std::vector<float> vals;
};

// The test values of a config, expressed in the reference spaces of that config.  These
// are all that is needed to calculate the fingerprint of a color space from that config.
//
struct TestVals
{
    std::vector<float> sceneRefTestVals;
    std::vector<float> displayRefTestVals;

    // True if the test values were successfully converted into the reference space of the
    // config they were initialized from (i.e., an interchange space was identified).  If
    // false, the values are the default ACES2065-1 or CIE-XYZ-D65 values, which are only
    // meaningful if the config happens to use that space as its reference space.
    //
    // Fingerprints calculated from two different configs may only be compared with each
    // other if the corresponding flag is true for both configs, otherwise the test values
    // do not represent the same colors in both configs.
    bool sceneRefTestValsConverted = false;
    bool displayRefTestValsConverted = false;
};

// A fingerprint for each color space of a config, along with the test values they were
// calculated from.  This is what is needed to search that config for a color space that is
// equivalent to one from another config.
//
struct ColorSpaceFingerprints
{
    // Keep a copy of the testVals because it is a useful record of what was used to
    // compute the fingerprints.
    TestVals testVals;
    std::vector<Fingerprint> vec;
};

bool calcColorSpaceFingerprint(std::vector<float> & fingerprintVals,
                               const TestVals & testVals,
                               const ConstConfigRcPtr & config,
                               const ConstColorSpaceRcPtr & cs);

// Initialize the test values needed to calculate fingerprints for a config.
void initializeTestVals(TestVals & testVals,
                        const ConstConfigRcPtr & config);

// Initialize the vector of color space fingerprints.
void initializeFingerprintVec(ColorSpaceFingerprints & fingerprints,
                              const ConstConfigRcPtr & config);

// Initialize the test values, then the fingerprints.
void initializeColorSpaceFingerprints(ColorSpaceFingerprints & fingerprints,
                                      const ConstConfigRcPtr & config);

const char * findEquivalentColorspace(const ColorSpaceFingerprints & fingerprints,
                                      const ConstConfigRcPtr & inputConfig,
                                      const ConstColorSpaceRcPtr & inputCS);

// Same as above, but the fingerprint of inputCS is calculated using the test values from
// inputTestVals (which must come from inputConfig) rather than those from fingerprints.
// This allows searching for an equivalent color space without needing to first adjust the
// reference space of inputCS to match the config the fingerprints were built from.
const char * findEquivalentColorspace(const ColorSpaceFingerprints & fingerprints,
                                      const TestVals & inputTestVals,
                                      const ConstConfigRcPtr & inputConfig,
                                      const ConstColorSpaceRcPtr & inputCS);

// Try to find the name of a color space in the built-in config that is equivalent to
// srcColorSpace.  Only active color spaces of the built-in config are searched.
//
// srcConfig/srcColorSpace -- The color space to search for and the config that owns it.
// builtinConfig -- The built-in config object to search.
// srcTestVals -- The (already calculated) test values of srcConfig.
// fingerprints -- The (already calculated) fingerprints of builtinConfig.
// Returns the name of the color space in the built-in config.
//
// \throw Exception if an interchange space cannot be found in either config.
//
const char * LocateBuiltinColorSpace(const ConstConfigRcPtr & srcConfig,
                                     const ConstColorSpaceRcPtr & srcColorSpace,
                                     const ConstConfigRcPtr & builtinConfig,
                                     const std::shared_ptr<const TestVals> & srcTestVals,
                                     const std::shared_ptr<const ColorSpaceFingerprints> & fingerprints);

// Sanitize a single token (e.g. a config name or a color space base name) for use in a Color
// Interop ID, per Annex C of the ASWF Color Interop Forum ColorInteropID recommendation.  Not
// meant to be applied to an already-namespaced ID string as a whole.
std::string SanitizeIDToken(std::string_view token);

// Implements Config::generateLocalIDForColorSpace.  See that method's doc comment for the
// algorithm.
//
// \throw Exception if srcColorSpaceName is null/empty, the config's name is empty or disallowed,
//        or the config does not contain the requested color space.
std::string GenerateLocalIDForColorSpace(const Config & config, const char * srcColorSpaceName);

// Implements Config::findColorSpaceForID.  See that method's doc comment for the algorithm.
// The allColorSpaces argument must be the complete (unfiltered) set of color spaces of the
// given config.  It is needed because the "local" mode fall-back must compare against the
// sanitized form of the color space names and aliases, which requires iterating over the
// color space objects themselves.
ConstColorSpaceRcPtr FindColorSpaceForID(const Config & config,
                                         const ConstColorSpaceSetRcPtr & allColorSpaces,
                                         const char * idString);

// Temporarily deactivate the Processor cache on a Config object.
//
class SuspendCacheGuard
{
public:
    SuspendCacheGuard();
    SuspendCacheGuard(const SuspendCacheGuard &) = delete;
    SuspendCacheGuard & operator=(const SuspendCacheGuard &) = delete;

    SuspendCacheGuard(const ConstConfigRcPtr & config)
        : m_config(config), m_origCacheFlags(config->getProcessorCacheFlags())
    {
        m_config->setProcessorCacheFlags(PROCESSOR_CACHE_OFF);
    }

    ~SuspendCacheGuard()
    {
        m_config->setProcessorCacheFlags(m_origCacheFlags);
    }

private:
    ConstConfigRcPtr m_config = nullptr;
    ProcessorCacheFlags m_origCacheFlags;
};

} // namespace ConfigUtils

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_CONFIG_UTILS_H
