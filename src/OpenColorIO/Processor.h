// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_PROCESSOR_H
#define INCLUDED_OCIO_PROCESSOR_H

#include <OpenColorIO/OpenColorIO.h>

#include "Caching.h"
#include "Mutex.h"
#include "Op.h"
#include "PrivateTypes.h"


namespace OCIO_NAMESPACE
{
class Processor::Impl
{
private:
    ProcessorMetadataRcPtr m_metadata;

    // Vector of ops for the processor.
    OpRcPtrVec m_ops;

    mutable std::string m_cacheID;

    mutable Mutex m_resultsCacheMutex;

    ProcessorCacheFlags m_cacheFlags { PROCESSOR_CACHE_DEFAULT };

    // Speedup GPU & CPU Processor accesses by using a cache.
    mutable ProcessorCache<std::size_t, ProcessorRcPtr>    m_optProcessorCache;
    mutable ProcessorCache<std::size_t, GPUProcessorRcPtr> m_gpuProcessorCache;
    mutable ProcessorCache<std::size_t, CPUProcessorRcPtr> m_cpuProcessorCache;

public:
    Impl();
    Impl(Impl &) = delete;
    Impl & operator=(const Impl & rhs);
    ~Impl();

    bool isNoOp() const;
    bool hasChannelCrosstalk() const;

    ConstProcessorMetadataRcPtr getProcessorMetadata() const;

    const FormatMetadata & getFormatMetadata() const;

    int getNumTransforms() const;
    const FormatMetadata & getTransformFormatMetadata(int index) const;

    bool isDynamic() const noexcept;
    bool hasDynamicProperty(DynamicPropertyType type) const noexcept;
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;

    const char * getCacheID() const;

    GroupTransformRcPtr createGroupTransform() const;

    ConstProcessorRcPtr getOptimizedProcessor(OptimizationFlags oFlags) const;

    ConstProcessorRcPtr getOptimizedProcessor(BitDepth inBD,
                                              BitDepth outBD,
                                              OptimizationFlags oFlags) const;

    // Get an optimized GPU processor instance for F32 images with default optimizations.
    ConstGPUProcessorRcPtr getDefaultGPUProcessor() const;

    // Get an optimized GPU processor instance for F32 images.
    ConstGPUProcessorRcPtr getOptimizedGPUProcessor(OptimizationFlags oFlags) const;

    // Get an optimized CPU processor instance for F32 images with default optimizations.
    ConstCPUProcessorRcPtr getDefaultCPUProcessor() const;

    // Get an optimized CPU processor instance for F32 images.
    ConstCPUProcessorRcPtr getOptimizedCPUProcessor(OptimizationFlags oFlags) const;

    // Get a optimized CPU processor instance for arbitrary input and output bit-depths.
    ConstCPUProcessorRcPtr getOptimizedCPUProcessor(BitDepth inBitDepth,
                                                    BitDepth outBitDepth,
                                                    OptimizationFlags oFlags) const;

    // Enable or disable the internal caches.
    void setProcessorCacheFlags(ProcessorCacheFlags flags) noexcept;

    ////////////////////////////////////////////
    //
    // Builder functions, Not exposed

    void setColorSpaceConversion(const Config & config,
                                 const ConstContextRcPtr & context,
                                 const ConstColorSpaceRcPtr & srcColorSpace,
                                 const ConstColorSpaceRcPtr & dstColorSpace);

    void setTransform(const Config & config,
                      const ConstContextRcPtr & context,
                      const ConstTransformRcPtr& transform,
                      TransformDirection direction);

    void concatenate(ConstProcessorRcPtr & p1, ConstProcessorRcPtr & p2);

    void computeMetadata();
};

} // namespace OCIO_NAMESPACE

#endif
