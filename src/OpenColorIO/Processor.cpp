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
#include "transforms/FileTransform.h"

OCIO_NAMESPACE_ENTER
{



    //////////////////////////////////////////////////////////////////////////
    
    class ProcessorMetadata::Impl
    {
    public:
        StringSet files;
        StringVec looks;
        
        Impl()
        { }
        
        ~Impl()
        { }
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

    void Processor::write(const char * formatName, std::ostream & os) const
    {
        getImpl()->write(formatName, os);
    }

    int Processor::getNumWriteFormats()
    {
        return FormatRegistry::GetInstance().getNumFormats(FORMAT_CAPABILITY_WRITE);
    }

    const char * Processor::getFormatNameByIndex(int index)
    {
        return FormatRegistry::GetInstance().getFormatNameByIndex(FORMAT_CAPABILITY_WRITE, index);
    }

    const char * Processor::getFormatExtensionByIndex(int index)
    {
        return FormatRegistry::GetInstance().getFormatExtensionByIndex(FORMAT_CAPABILITY_WRITE, index);
	}
	
    bool Processor::hasDynamicProperty(DynamicPropertyType type) const
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
    
    ConstGPUProcessorRcPtr Processor::getDefaultGPUProcessor() const
    {
        return getImpl()->getDefaultGPUProcessor();
    }

    ConstGPUProcessorRcPtr Processor::getOptimizedGPUProcessor(OptimizationFlags oFlags, 
                                                               FinalizationFlags fFlags) const
    {
        return getImpl()->getOptimizedGPUProcessor(oFlags, fFlags);
    }

    ConstCPUProcessorRcPtr Processor::getDefaultCPUProcessor() const
    {
        return getImpl()->getDefaultCPUProcessor();
    }

    ConstCPUProcessorRcPtr Processor::getOptimizedCPUProcessor(OptimizationFlags oFlags,
                                                               FinalizationFlags fFlags) const
    {
        return getImpl()->getOptimizedCPUProcessor(oFlags, fFlags);
    }

    ConstCPUProcessorRcPtr Processor::getOptimizedCPUProcessor(BitDepth inBitDepth, 
                                                               BitDepth outBitDepth,
                                                               OptimizationFlags oFlags,
                                                               FinalizationFlags fFlags) const
    {
        return getImpl()->getOptimizedCPUProcessor(inBitDepth, outBitDepth, oFlags, fFlags);
    }

    

    Processor::Impl::Impl():
        m_metadata(ProcessorMetadata::Create())
    {
    }
    
    Processor::Impl::~Impl()
    { }
    
    bool Processor::Impl::isNoOp() const
    {
        return IsOpVecNoOp(m_ops);
    }
    
    bool Processor::Impl::hasChannelCrosstalk() const
    {
        for(const auto & op : m_ops)
        {
            if(op->hasChannelCrosstalk()) return true;
        }
        
        return false;
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

    void Processor::Impl::write(const char * formatName, std::ostream & os) const
    {
        FileFormat* fmt = FormatRegistry::GetInstance().getFileFormatByName(formatName);

        if (!fmt)
        {
            std::ostringstream err;
            err << "The format named '" << formatName;
            err << "' could not be found. ";
            throw Exception(err.str().c_str());
        }

        try
        {
            std::string fName{ formatName };
            fmt->write(m_ops, getFormatMetadata(), fName, os);
        }
        catch (std::exception & e)
        {
            std::ostringstream err;
            err << "Error writing " << formatName << ":";
            err << e.what();
            throw Exception(err.str().c_str());
        }
    }

    bool Processor::Impl::hasDynamicProperty(DynamicPropertyType type) const
    {
        for (const auto & op : m_ops)
        {
            if (op->hasDynamicProperty(type))
            {
                return true;
            }
        }
        return false;
    }

    DynamicPropertyRcPtr Processor::Impl::getDynamicProperty(DynamicPropertyType type) const
    {
        for(const auto & op : m_ops)
        {
            if(op->hasDynamicProperty(type))
            {
                return op->getDynamicProperty(type);
            }
        }

        throw Exception("Cannot find dynamic property; not used by processor.");
    }

    const char * Processor::Impl::getCacheID() const
    {
        AutoMutex lock(m_resultsCacheMutex);
        
        if(!m_cpuCacheID.empty()) return m_cpuCacheID.c_str();
        
        if(m_ops.empty())
        {
            m_cpuCacheID = "<NOOP>";
        }
        else
        {
            std::ostringstream cacheid;
            for(const auto & op : m_ops)
            {
                cacheid << op->getCacheID() << " ";
            }
            std::string fullstr = cacheid.str();
            
            m_cpuCacheID = CacheIDHash(fullstr.c_str(), (int)fullstr.size());
        }
        
        return m_cpuCacheID.c_str();
    }
    
    ///////////////////////////////////////////////////////////////////////////

    ConstGPUProcessorRcPtr Processor::Impl::getDefaultGPUProcessor() const
    {
        GPUProcessorRcPtr gpu = GPUProcessorRcPtr(new GPUProcessor(), &GPUProcessor::deleter);

        gpu->getImpl()->finalize(m_ops, 
                                 OPTIMIZATION_DEFAULT, FINALIZATION_DEFAULT);

        return gpu;
    }

    ConstGPUProcessorRcPtr Processor::Impl::getOptimizedGPUProcessor(OptimizationFlags oFlags,
                                                                     FinalizationFlags fFlags) const
    {
        GPUProcessorRcPtr gpu = GPUProcessorRcPtr(new GPUProcessor(), &GPUProcessor::deleter);

        gpu->getImpl()->finalize(m_ops, oFlags, fFlags);

        return gpu;
    }

    ///////////////////////////////////////////////////////////////////////////

    ConstCPUProcessorRcPtr Processor::Impl::getDefaultCPUProcessor() const
    {
        CPUProcessorRcPtr cpu = CPUProcessorRcPtr(new CPUProcessor(), &CPUProcessor::deleter);

        cpu->getImpl()->finalize(m_ops, 
                                 BIT_DEPTH_F32, BIT_DEPTH_F32, 
                                 OPTIMIZATION_DEFAULT, FINALIZATION_DEFAULT);

        return cpu;
    }

    ConstCPUProcessorRcPtr Processor::Impl::getOptimizedCPUProcessor(OptimizationFlags oFlags,
                                                                     FinalizationFlags fFlags) const
    {
        CPUProcessorRcPtr cpu = CPUProcessorRcPtr(new CPUProcessor(), &CPUProcessor::deleter);

        cpu->getImpl()->finalize(m_ops,
                                 BIT_DEPTH_F32, BIT_DEPTH_F32,
                                 oFlags, fFlags);

        return cpu;
    }

    ConstCPUProcessorRcPtr Processor::Impl::getOptimizedCPUProcessor(BitDepth inBitDepth, 
                                                                     BitDepth outBitDepth,
                                                                     OptimizationFlags oFlags,
                                                                     FinalizationFlags fFlags) const
    {
        CPUProcessorRcPtr cpu = CPUProcessorRcPtr(new CPUProcessor(), &CPUProcessor::deleter);

        cpu->getImpl()->finalize(m_ops, inBitDepth, outBitDepth, oFlags, fFlags);

        return cpu;
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
        BuildColorSpaceOps(m_ops, config, context, srcColorSpace, dstColorSpace);
        FinalizeOpVec(m_ops, FINALIZATION_EXACT);
        UnifyDynamicProperties(m_ops);
    }
    
    void Processor::Impl::setTransform(const Config & config,
                                       const ConstContextRcPtr & context,
                                       const ConstTransformRcPtr& transform,
                                       TransformDirection direction)
    {
        if (!m_ops.empty())
        {
            throw Exception("Internal error: Processor should be empty");
        }
        transform->validate();
        BuildOps(m_ops, config, context, transform, direction);
        FinalizeOpVec(m_ops, FINALIZATION_EXACT);
        UnifyDynamicProperties(m_ops);
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

}
OCIO_NAMESPACE_EXIT
