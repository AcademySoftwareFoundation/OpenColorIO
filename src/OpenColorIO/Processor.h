// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_PROCESSOR_H
#define INCLUDED_OCIO_PROCESSOR_H

#include <OpenColorIO/OpenColorIO.h>

#include "Mutex.h"
#include "Op.h"
#include "PrivateTypes.h"

OCIO_NAMESPACE_ENTER
{
    class Processor::Impl
    {
    private:
        ProcessorMetadataRcPtr m_metadata;

        // Vector of ops for the processor.
        OpRcPtrVec m_ops;

        mutable std::string m_cpuCacheID;
        
        mutable Mutex m_resultsCacheMutex;

    public:
        Impl();
        ~Impl();
        
        bool isNoOp() const;
        bool hasChannelCrosstalk() const;
        
        ConstProcessorMetadataRcPtr getProcessorMetadata() const;

        const FormatMetadata & getFormatMetadata() const;

        int getNumTransforms() const;
        const FormatMetadata & getTransformFormatMetadata(int index) const;

        bool hasDynamicProperty(DynamicPropertyType type) const;
        DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;

        const char * getCacheID() const;

        GroupTransformRcPtr createGroupTransform() const;

        void write(const char * formatName, std::ostream & os) const;

        void apply(ImageDesc& img) const;
        
        // Get an optimized GPU processor instance for F32 images with default optimizations.
        ConstGPUProcessorRcPtr getDefaultGPUProcessor() const;

        // Get an optimized GPU processor instance for F32 images.
        ConstGPUProcessorRcPtr getOptimizedGPUProcessor(OptimizationFlags oFlags, 
                                                        FinalizationFlags fFlags) const;

        // Get an optimized CPU processor instance for F32 images with default optimizations.
        ConstCPUProcessorRcPtr getDefaultCPUProcessor() const;

        // Get an optimized CPU processor instance for F32 images.
        ConstCPUProcessorRcPtr getOptimizedCPUProcessor(OptimizationFlags oFlags,
                                                        FinalizationFlags fFlags) const;

        // Get a optimized CPU processor instance for arbitrary input and output bit-depths.
        ConstCPUProcessorRcPtr getOptimizedCPUProcessor(BitDepth inBitDepth,
                                                        BitDepth outBitDepth,
                                                        OptimizationFlags oFlags,
                                                        FinalizationFlags fFlags) const;

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

        void computeMetadata();
    };
    
}
OCIO_NAMESPACE_EXIT

#endif
