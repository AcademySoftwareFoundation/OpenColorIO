// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cstring>
#include <iterator>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "CPUProcessor.h"
#include "GPUProcessor.h"
#include "HashUtils.h"
#include "OpBuilders.h"
#include "Processor.h"
#include "TransformBuilder.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{

class ProcessorMetadata::Impl
{
public:
    StringSet files;
    StringUtils::StringVec looks;

    Impl()  = default;
    ~Impl() = default;
};

ProcessorMetadataRcPtr ProcessorMetadata::Create()
{
    return ProcessorMetadataRcPtr(new ProcessorMetadata(), &deleter);
}

ProcessorMetadata::ProcessorMetadata()
    : m_impl(new ProcessorMetadata::Impl)
{ }

ProcessorMetadata::~ProcessorMetadata()
{
    delete m_impl;
    m_impl = nullptr;
}

void ProcessorMetadata::deleter(ProcessorMetadata* c)
{
    delete c;
}

int ProcessorMetadata::getNumFiles() const
{
    return static_cast<int>(getImpl()->files.size());
}

const char * ProcessorMetadata::getFile(int index) const
{
    if(index < 0 ||
        index >= (static_cast<int>(getImpl()->files.size())))
    {
        return "";
    }

    StringSet::const_iterator iter = getImpl()->files.begin();
    std::advance( iter, index );

    return iter->c_str();
}

void ProcessorMetadata::addFile(const char * fname)
{
    getImpl()->files.insert(fname);
}

int ProcessorMetadata::getNumLooks() const
{
    return static_cast<int>(getImpl()->looks.size());
}

const char * ProcessorMetadata::getLook(int index) const
{
    if(index < 0 ||
        index >= (static_cast<int>(getImpl()->looks.size())))
    {
        return "";
    }

    return getImpl()->looks[index].c_str();
}

void ProcessorMetadata::addLook(const char * look)
{
    getImpl()->looks.push_back(look);
}

//////////////////////////////////////////////////////////////////////////

ProcessorRcPtr Processor::Create()
{
    return ProcessorRcPtr(new Processor(), &deleter);
}

void Processor::deleter(Processor* c)
{
    delete c;
}

Processor::Processor()
    : m_impl(new Processor::Impl)
{
}

Processor::~Processor()
{
    delete m_impl;
    m_impl = nullptr;
}

bool Processor::isNoOp() const
{
    return getImpl()->isNoOp();
}

bool Processor::hasChannelCrosstalk() const
{
    return getImpl()->hasChannelCrosstalk();
}

ConstProcessorMetadataRcPtr Processor::getProcessorMetadata() const
{
    return getImpl()->getProcessorMetadata();
}

const FormatMetadata & Processor::getFormatMetadata() const
{
    return getImpl()->getFormatMetadata();
}

int Processor::getNumTransforms() const
{
    return getImpl()->getNumTransforms();
}

const FormatMetadata & Processor::getTransformFormatMetadata(int index) const
{
    return getImpl()->getTransformFormatMetadata(index);
}

GroupTransformRcPtr Processor::createGroupTransform() const
{
    return getImpl()->createGroupTransform();
}

bool Processor::isDynamic() const noexcept
{
    return getImpl()->isDynamic();
}

bool Processor::hasDynamicProperty(DynamicPropertyType type) const noexcept
{
    return getImpl()->hasDynamicProperty(type);
}

DynamicPropertyRcPtr Processor::getDynamicProperty(DynamicPropertyType type) const
{
    return getImpl()->getDynamicProperty(type);
}

const char * Processor::getCacheID() const
{
    return getImpl()->getCacheID();
}

ConstProcessorRcPtr Processor::getOptimizedProcessor(OptimizationFlags oFlags) const
{
    return getImpl()->getOptimizedProcessor(oFlags);
}

ConstProcessorRcPtr Processor::getOptimizedProcessor(BitDepth inBD, BitDepth outBD, 
                                                     OptimizationFlags oFlags) const
{
    return getImpl()->getOptimizedProcessor(inBD, outBD, oFlags);
}

ConstGPUProcessorRcPtr Processor::getDefaultGPUProcessor() const
{
    return getImpl()->getDefaultGPUProcessor();
}

ConstGPUProcessorRcPtr Processor::getOptimizedGPUProcessor(OptimizationFlags oFlags) const
{
    return getImpl()->getOptimizedGPUProcessor(oFlags);
}

ConstCPUProcessorRcPtr Processor::getDefaultCPUProcessor() const
{
    return getImpl()->getDefaultCPUProcessor();
}

ConstCPUProcessorRcPtr Processor::getOptimizedCPUProcessor(OptimizationFlags oFlags) const
{
    return getImpl()->getOptimizedCPUProcessor(oFlags);
}

ConstCPUProcessorRcPtr Processor::getOptimizedCPUProcessor(BitDepth inBitDepth,
                                                            BitDepth outBitDepth,
                                                            OptimizationFlags oFlags) const
{
    return getImpl()->getOptimizedCPUProcessor(inBitDepth, outBitDepth, oFlags);
}


// Instantiate the cache with the right types.
template class ProcessorCache<std::size_t, ProcessorRcPtr>;
template class ProcessorCache<std::size_t, GPUProcessorRcPtr>;
template class ProcessorCache<std::size_t, CPUProcessorRcPtr>;


Processor::Impl::Impl():
    m_metadata(ProcessorMetadata::Create())
{
}

Processor::Impl::~Impl()
{
}

Processor::Impl & Processor::Impl::operator=(const Impl & rhs)
{
    if (this != &rhs)
    {
        AutoMutex lock(m_resultsCacheMutex);

        m_metadata = rhs.m_metadata;
        m_ops      = rhs.m_ops;

        m_cacheID.clear();

        m_cacheFlags = rhs.m_cacheFlags;

        const bool enableCaches
            = (m_cacheFlags & PROCESSOR_CACHE_ENABLED) == PROCESSOR_CACHE_ENABLED;

        m_optProcessorCache.clear();
        m_optProcessorCache.enable(enableCaches);

        m_gpuProcessorCache.clear();
        m_gpuProcessorCache.enable(enableCaches);

        m_cpuProcessorCache.clear();
        m_cpuProcessorCache.enable(enableCaches);
    }
    return *this;
}

bool Processor::Impl::isNoOp() const
{
    return m_ops.isNoOp();
}

bool Processor::Impl::hasChannelCrosstalk() const
{
    return m_ops.hasChannelCrosstalk();
}

ConstProcessorMetadataRcPtr Processor::Impl::getProcessorMetadata() const
{
    return m_metadata;
}

const FormatMetadata & Processor::Impl::getFormatMetadata() const
{
    return m_ops.getFormatMetadata();
}

int Processor::Impl::getNumTransforms() const
{
    return (int)m_ops.size();
}

const FormatMetadata & Processor::Impl::getTransformFormatMetadata(int index) const
{
    auto op = OCIO_DYNAMIC_POINTER_CAST<const Op>(m_ops[index]);
    return op->data()->getFormatMetadata();
}

GroupTransformRcPtr Processor::Impl::createGroupTransform() const
{
    GroupTransformRcPtr group = GroupTransform::Create();

    // Copy format metadata.
    group->getFormatMetadata() = getFormatMetadata();

    // Build transforms from ops.
    for (ConstOpRcPtr op : m_ops)
    {
        CreateTransform(group, op);
    }

    return group;
}

bool Processor::Impl::isDynamic() const noexcept
{
    return m_ops.isDynamic();
}

bool Processor::Impl::hasDynamicProperty(DynamicPropertyType type) const noexcept
{
    return m_ops.hasDynamicProperty(type);
}

DynamicPropertyRcPtr Processor::Impl::getDynamicProperty(DynamicPropertyType type) const
{
    return m_ops.getDynamicProperty(type);
}

const char * Processor::Impl::getCacheID() const
{
    AutoMutex lock(m_resultsCacheMutex);

    if(!m_cacheID.empty()) return m_cacheID.c_str();

    if(m_ops.empty())
    {
        m_cacheID = "<NOOP>";
    }
    else
    {
        std::ostringstream cacheidStream;
        for(const auto & op : m_ops)
        {
            cacheidStream << op->getCacheID() << " ";
        }

        const std::string fullstr = cacheidStream.str();
        m_cacheID = CacheIDHash(fullstr.c_str(), (int)fullstr.size());
    }

    return m_cacheID.c_str();
}

///////////////////////////////////////////////////////////////////////////

namespace
{
OptimizationFlags EnvironmentOverride(OptimizationFlags oFlags)
{
    const std::string envFlag = GetEnvVariable(OCIO_OPTIMIZATION_FLAGS_ENVVAR);
    if (!envFlag.empty())
    {
        // Use 0 to allow base to be determined by the format.
        oFlags = static_cast<OptimizationFlags>(std::stoul(envFlag, nullptr, 0));
    }
    return oFlags;
}
}

ConstProcessorRcPtr Processor::Impl::getOptimizedProcessor(OptimizationFlags oFlags) const
{
    return getOptimizedProcessor(BIT_DEPTH_F32, BIT_DEPTH_F32, oFlags);
}

ConstProcessorRcPtr Processor::Impl::getOptimizedProcessor(BitDepth inBitDepth, 
                                                           BitDepth outBitDepth,
                                                           OptimizationFlags oFlags) const
{
    // Helper method.
    auto CreateProcessor = [](const Processor::Impl & procImpl,
                              BitDepth inBitDepth,
                              BitDepth outBitDepth,
                              OptimizationFlags oFlags) -> ProcessorRcPtr
    {
        ProcessorRcPtr proc = Create();
        *proc->getImpl() = procImpl;

        proc->getImpl()->m_ops.finalize(oFlags);
        proc->getImpl()->m_ops.optimizeForBitdepth(inBitDepth, outBitDepth, oFlags);
        proc->getImpl()->m_ops.validateDynamicProperties();

        return proc;
    };


    oFlags = EnvironmentOverride(oFlags);

    if (m_optProcessorCache.isEnabled())
    {
        AutoMutex guard(m_optProcessorCache.lock());

        std::ostringstream oss;
        oss << inBitDepth << outBitDepth << oFlags;

        const std::size_t key = std::hash<std::string>{}(oss.str());

        // As the entry is a shared pointer instance, having an empty one means that the entry does
        // not exist in the cache. So, it provides a fast existence check & access in one call.
        ProcessorRcPtr & processor = m_optProcessorCache[key];
        if (!processor)
        {
            // Note: Some combinations of bit-depth and opt flags will produce identical Processors.
            // Duplicates could be identified by computing the Processor cacheID, but that is too
            // slow to attempt here.

            processor = CreateProcessor(*this, inBitDepth, outBitDepth, oFlags);
        }

        return processor;
    }
    else
    {
        return CreateProcessor(*this, inBitDepth, outBitDepth, oFlags);
    }
}

///////////////////////////////////////////////////////////////////////////

ConstGPUProcessorRcPtr Processor::Impl::getDefaultGPUProcessor() const
{
    return getOptimizedGPUProcessor(OPTIMIZATION_DEFAULT);
}

ConstGPUProcessorRcPtr Processor::Impl::getOptimizedGPUProcessor(OptimizationFlags oFlags) const
{
    // Helper method.
    auto CreateProcessor = [](const OpRcPtrVec & ops,
                              OptimizationFlags oFlags) -> GPUProcessorRcPtr
    {
        GPUProcessorRcPtr gpu = GPUProcessorRcPtr(new GPUProcessor(), &GPUProcessor::deleter);
        gpu->getImpl()->finalize(ops, oFlags);
        return gpu;
    };

    oFlags = EnvironmentOverride(oFlags);

    if (m_gpuProcessorCache.isEnabled())
    {
        AutoMutex guard(m_gpuProcessorCache.lock());

        // As the entry is a shared pointer instance, having an empty one means that the entry does
        // not exist in the cache. So, it provides a fast existence check & access in one call.
        GPUProcessorRcPtr & processor = m_gpuProcessorCache[oFlags];
        if (!processor)
        {
            processor = CreateProcessor(m_ops, oFlags);
        }
        
        return processor;
    }
    else
    {
        return CreateProcessor(m_ops, oFlags);
    }
}

///////////////////////////////////////////////////////////////////////////

ConstCPUProcessorRcPtr Processor::Impl::getDefaultCPUProcessor() const
{
    return getOptimizedCPUProcessor(OPTIMIZATION_DEFAULT);
}

ConstCPUProcessorRcPtr Processor::Impl::getOptimizedCPUProcessor(OptimizationFlags oFlags) const
{
    return getOptimizedCPUProcessor(BIT_DEPTH_F32, BIT_DEPTH_F32, oFlags);
}

ConstCPUProcessorRcPtr Processor::Impl::getOptimizedCPUProcessor(BitDepth inBitDepth,
                                                                 BitDepth outBitDepth,
                                                                 OptimizationFlags oFlags) const
{
    // Helper method.
    auto CreateProcessor = [](const OpRcPtrVec & ops,
                              BitDepth inBitDepth,
                              BitDepth outBitDepth,
                              OptimizationFlags oFlags) -> CPUProcessorRcPtr
    {
        CPUProcessorRcPtr cpu = CPUProcessorRcPtr(new CPUProcessor(), &CPUProcessor::deleter);
        cpu->getImpl()->finalize(ops, inBitDepth, outBitDepth, oFlags);
        return cpu;
    };

    oFlags = EnvironmentOverride(oFlags);

    const bool shareDynamicProperties 
        = (m_cacheFlags & PROCESSOR_CACHE_SHARE_DYN_PROPERTIES) == PROCESSOR_CACHE_SHARE_DYN_PROPERTIES;

    const bool useCache = m_ops.isDynamic() ? shareDynamicProperties : true;

    if (m_cpuProcessorCache.isEnabled() && useCache)
    {
        AutoMutex guard(m_cpuProcessorCache.lock());

        std::ostringstream oss;
        oss << inBitDepth << outBitDepth << oFlags;

        const std::size_t key = std::hash<std::string>{}(oss.str());

        // As the entry is a shared pointer instance, having an empty one means that the entry does
        // not exist in the cache. So, it provides a fast existence check & access in one call.
        CPUProcessorRcPtr & processor = m_cpuProcessorCache[key];
        if (!processor)
        {
            processor = CreateProcessor(m_ops, inBitDepth, outBitDepth, oFlags);
        }
        
        return processor;
    }
    else
    {
        return CreateProcessor(m_ops, inBitDepth, outBitDepth, oFlags);
    }
}

void Processor::Impl::setProcessorCacheFlags(ProcessorCacheFlags flags) noexcept
{
    m_cacheFlags = flags;

    const bool cacheEnabled = (m_cacheFlags & PROCESSOR_CACHE_ENABLED) == PROCESSOR_CACHE_ENABLED;

    m_optProcessorCache.enable(cacheEnabled);
    m_gpuProcessorCache.enable(cacheEnabled);
    m_cpuProcessorCache.enable(cacheEnabled);
}

///////////////////////////////////////////////////////////////////////////


void Processor::Impl::setColorSpaceConversion(const Config & config,
                                              const ConstContextRcPtr & context,
                                              const ConstColorSpaceRcPtr & srcColorSpace,
                                              const ConstColorSpaceRcPtr & dstColorSpace)
{
    if (!m_ops.empty())
    {
        throw Exception("Internal error: Processor should be empty");
    }

    // Default behavior is to bypass data color space. ColorSpaceTransform can be used to not bypass
    // data color spaces.
    BuildColorSpaceOps(m_ops, config, context, srcColorSpace, dstColorSpace, true);

    std::ostringstream desc;
    desc << "Color space conversion from " << srcColorSpace->getName()
         << " to " << dstColorSpace->getName();
    m_ops.getFormatMetadata().addAttribute(METADATA_DESCRIPTION, desc.str().c_str());
    m_ops.finalize(OPTIMIZATION_NONE);
    m_ops.validateDynamicProperties();
}

void Processor::Impl::setTransform(const Config & config,
                                   const ConstContextRcPtr & context,
                                   const ConstTransformRcPtr & transform,
                                   TransformDirection direction)
{
    if (!m_ops.empty())
    {
        throw Exception("Internal error: Processor should be empty");
    }

    transform->validate();

    BuildOps(m_ops, config, context, transform, direction);

    m_ops.finalize(OPTIMIZATION_NONE);
    m_ops.validateDynamicProperties();
}

void Processor::Impl::concatenate(ConstProcessorRcPtr & p1, ConstProcessorRcPtr & p2)
{
    m_ops = p1->getImpl()->m_ops;
    m_ops += p2->getImpl()->m_ops;

    computeMetadata();

    // Ops have been validated by p1 & p2.
    m_ops.validateDynamicProperties();
}


void Processor::Impl::computeMetadata()
{
    AutoMutex lock(m_resultsCacheMutex);

    // Pull out metadata, before the no-ops are removed.
    for(auto & op : m_ops)
    {
        op->dumpMetadata(m_metadata);
    }
}

} // namespace OCIO_NAMESPACE
