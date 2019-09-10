/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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

////////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "ops/exposurecontrast/ExposureContrastOps.h"
#include "UnitTest.h"

OCIO_ADD_TEST(Processor, basic)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    auto processorEmptyGroup = config->getProcessor(group);
    OCIO_CHECK_EQUAL(processorEmptyGroup->getNumTransforms(), 0);
    OCIO_CHECK_EQUAL(std::string(processorEmptyGroup->getCacheID()), "<NOOP>");

    auto mat = OCIO::MatrixTransform::Create();
    double offset[4]{ 0.1, 0.2, 0.3, 0.4 };
    mat->setOffset(offset);

    auto processorMat = config->getProcessor(mat);
    OCIO_CHECK_EQUAL(processorMat->getNumTransforms(), 1);

    OCIO_CHECK_EQUAL(std::string(processorMat->getCacheID()), "$c15dfc9b251ee075f33c4ccb3eb1e4b8");
}

OCIO_ADD_TEST(Processor, shared_dynamic_properties)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::ExposureContrastOpDataRcPtr data =
        std::make_shared<OCIO::ExposureContrastOpData>();

    data->setExposure(1.2);
    data->setPivot(0.5);
    data->getExposureProperty()->makeDynamic();

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    data = data->clone();
    data->setExposure(2.2);

    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_REQUIRE_ASSERT(ops[1]);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    auto data0 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::ExposureContrastOpData>(op0->data());
    auto data1 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::ExposureContrastOpData>(op1->data());

    OCIO::DynamicPropertyImplRcPtr dp0 = data0->getExposureProperty();
    OCIO::DynamicPropertyImplRcPtr dp1 = data1->getExposureProperty();

    OCIO_CHECK_NE(dp0->getDoubleValue(), dp1->getDoubleValue());

    OCIO::UnifyDynamicProperties(ops);

    OCIO::DynamicPropertyImplRcPtr dp0_post = data0->getExposureProperty();
    OCIO::DynamicPropertyImplRcPtr dp1_post = data1->getExposureProperty();

    OCIO_CHECK_EQUAL(dp0_post->getDoubleValue(), dp1_post->getDoubleValue());

    // Both share the same pointer.
    OCIO_CHECK_EQUAL(dp0_post.get(), dp1_post.get());

    // The first pointer is the one that gets shared.
    OCIO_CHECK_EQUAL(dp0.get(), dp0_post.get());
}

#endif