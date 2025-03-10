// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_CONFIG_UTILS_H
#define INCLUDED_OCIO_CONFIG_UTILS_H

#include <map>

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
    const char * csName;
    ReferenceSpaceType type;
    std::vector<float> vals;
};

struct ColorSpaceFingerprints
{
    std::vector<Fingerprint> vec;
    std::vector<float> sceneRefTestVals;
    std::vector<float> displayRefTestVals;
};

bool calcColorSpaceFingerprint(std::vector<float> & fingerprintVals, 
                               const ColorSpaceFingerprints & fingerprints, 
                               const ConstConfigRcPtr & config, 
                               const ConstColorSpaceRcPtr & cs);

void initializeColorSpaceFingerprints(ColorSpaceFingerprints & fingerprints,
                                      const ConstConfigRcPtr & config);

const char * findEquivalentColorspace(const ColorSpaceFingerprints & fingerprints,
                                      const ConstConfigRcPtr & inputConfig, 
                                      const ConstColorSpaceRcPtr & inputCS);

// Temporarily deactivate the Processor cache on a Config object.
// Currently, this also clears the cache.
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
