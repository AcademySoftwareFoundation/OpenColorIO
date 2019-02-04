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
#include <sstream>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShader.h"
#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "Logging.h"
#include "OpBuilders.h"
#include "ops/Allocation/AllocationOp.h"
#include "ops/Lut3D/Lut3DOp.h"
#include "ops/NoOp/NoOps.h"
#include "Processor.h"
#include "ScanlineHelper.h"

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
        m_impl = NULL;
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
        m_impl = NULL;
    }
    
    bool Processor::isNoOp() const
    {
        return getImpl()->isNoOp();
    }
    
    bool Processor::hasChannelCrosstalk() const
    {
        return getImpl()->hasChannelCrosstalk();
    }
    
    ConstProcessorMetadataRcPtr Processor::getMetadata() const
    {
        return getImpl()->getMetadata();
    }
    
    void Processor::apply(ImageDesc& img) const
    {
        getImpl()->apply(img);
    }
    void Processor::applyRGB(float * pixel) const
    {
        getImpl()->applyRGB(pixel);
    }
    
    void Processor::applyRGBA(float * pixel) const
    {
        getImpl()->applyRGBA(pixel);
    }
    
    const char * Processor::getCpuCacheID() const
    {
        return getImpl()->getCpuCacheID();
    }
    
    void Processor::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
    {
        getImpl()->extractGpuShaderInfo(shaderDesc);
    }
    
    //////////////////////////////////////////////////////////////////////////
    
    
    
    namespace
    {
        void WriteShaderHeader(GpuShaderDescRcPtr & shaderDesc)
        {
            const std::string fcnName(shaderDesc->getFunctionName());

            GpuShaderText ss(shaderDesc->getLanguage());

            ss.newLine();
            ss.newLine() << "// Declaration of the OCIO shader function";
            ss.newLine();

            ss.newLine() << ss.vec4fKeyword() << " " << fcnName 
                         << "(in "  << ss.vec4fKeyword() << " inPixel)";
            ss.newLine() << "{";
            ss.indent();
            ss.newLine() << ss.vec4fKeyword() << " " 
                         << shaderDesc->getPixelName() << " = inPixel;";

            shaderDesc->addToFunctionHeaderShaderCode(ss.string().c_str());
        }
        
        
        void WriteShaderFooter(GpuShaderDescRcPtr & shaderDesc)
        {
            GpuShaderText ss(shaderDesc->getLanguage());

            ss.newLine();
            ss.indent();
            ss.newLine() << "return " << shaderDesc->getPixelName() << ";";
            ss.dedent();
            ss.newLine() << "}";

            shaderDesc->addToFunctionFooterShaderCode(ss.string().c_str());
        }


        void deepCopy(OpRcPtrVec & dst, const OpRcPtrVec & src)
        {
            for(auto & op : src)
            {
                dst.push_back(op->clone());
            }
        }


        void shallowCopy(OpRcPtrVec & dst, const OpRcPtrVec & src)
        {
            std::copy(src.begin(), src.end(), back_inserter(dst));   
        }


        OpRcPtrVec create3DLut(const OpRcPtrVec & src, unsigned edgelen)
        {
            if(src.size()==0) return OpRcPtrVec();

            const unsigned lut3DEdgeLen   = edgelen;
            const unsigned lut3DNumPixels = lut3DEdgeLen*lut3DEdgeLen*lut3DEdgeLen;

            Lut3DRcPtr lut = Lut3D::Create();
            lut->size[0] = lut3DEdgeLen;
            lut->size[1] = lut3DEdgeLen;
            lut->size[2] = lut3DEdgeLen;

            lut->lut.resize(lut3DNumPixels*3);
            
            // Allocate 3D LUT image, RGBA
            std::vector<float> lut3D(lut3DNumPixels*4);
            GenerateIdentityLut3D(&lut3D[0], lut3DEdgeLen, 4, LUT3DORDER_FAST_RED);
            
            // Apply the lattice ops to it
            const OpRcPtrVec::size_type max = src.size();
            for(OpRcPtrVec::size_type i=0; i<max; ++i)
            {
                src[i]->apply(&lut3D[0], lut3DNumPixels);
            }
            
            // Convert the RGBA image to an RGB image, in place.           
            for(unsigned i=0; i<lut3DNumPixels; ++i)
            {
                lut->lut[3*i+0] = lut3D[4*i+0];
                lut->lut[3*i+1] = lut3D[4*i+1];
                lut->lut[3*i+2] = lut3D[4*i+2];
            }

            OpRcPtrVec ops;
            CreateLut3DOp(ops, lut, INTERP_LINEAR, TRANSFORM_DIR_FORWARD);
            return ops;
        }
    }
    
    
    //////////////////////////////////////////////////////////////////////////
    
    
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
        for(auto & op : m_ops)
        {
            if(op->hasChannelCrosstalk()) return true;
        }
        
        return false;
    }
    
    ConstProcessorMetadataRcPtr Processor::Impl::getMetadata() const
    {
        return m_metadata;
    }
    
    void Processor::Impl::apply(ImageDesc& img) const
    {
        if(m_ops.empty()) return;
        
        ScanlineHelper scanlineHelper(img);
        float * rgbaBuffer = 0;
        long numPixels = 0;
        
        while(true)
        {
            scanlineHelper.prepRGBAScanline(&rgbaBuffer, &numPixels);
            if(numPixels == 0) break;
            if(!rgbaBuffer)
                throw Exception("Cannot apply transform; null image.");
            
            for(auto & op : m_ops)
            {
                op->apply(rgbaBuffer, numPixels);
            }
            
            scanlineHelper.finishRGBAScanline();
        }
    }
    
    void Processor::Impl::applyRGB(float * pixel) const
    {
        if(m_ops.empty()) return;
        
        // We need to allocate a temp array as the pixel must be 4 floats in size
        // (otherwise, sse loads will potentially fail)
        
        float rgbaBuffer[4] = { pixel[0], pixel[1], pixel[2], 0.0f };
        
        for(auto & op : m_ops)
        {
            op->apply(rgbaBuffer, 1);
        }
        
        pixel[0] = rgbaBuffer[0];
        pixel[1] = rgbaBuffer[1];
        pixel[2] = rgbaBuffer[2];
    }
    
    void Processor::Impl::applyRGBA(float * pixel) const
    {
        for(auto & op : m_ops)
        {
            op->apply(pixel, 1);
        }
    }
    
    const char * Processor::Impl::getCpuCacheID() const
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
            for(auto & op : m_ops)
            {
                cacheid << op->getCacheID() << " ";
            }
            std::string fullstr = cacheid.str();
            
            m_cpuCacheID = CacheIDHash(fullstr.c_str(), (int)fullstr.size());
        }
        
        return m_cpuCacheID.c_str();
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    void Processor::Impl::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
    {
        AutoMutex lock(m_resultsCacheMutex);

        LegacyGpuShaderDesc * legacy = dynamic_cast<LegacyGpuShaderDesc*>(shaderDesc.get());

        OpRcPtrVec gpuOps;

        if(legacy)
        {
            // GPU Process setup
            //
            // Partition the original, raw opvec into 3 segments for GPU Processing
            //
            // Interior index range does not support the gpu shader.
            // This is used to bound our analytical shader text generation
            // start index and end index are inclusive.
            
            // These 3 op vecs represent the 3 stages in our gpu pipe.
            // 1) preprocess shader text
            // 2) 3D LUT process lookup
            // 3) postprocess shader text
            
            OpRcPtrVec gpuOpsHwPreProcess;
            OpRcPtrVec gpuOpsCpuLatticeProcess;
            OpRcPtrVec gpuOpsHwPostProcess;

            PartitionGPUOps(gpuOpsHwPreProcess,
                            gpuOpsCpuLatticeProcess,
                            gpuOpsHwPostProcess,
                            m_ops);

            LogDebug("GPU Ops: Pre-3DLUT");
            FinalizeOpVec(gpuOpsHwPreProcess);

            LogDebug("GPU Ops: 3DLUT");
            FinalizeOpVec(gpuOpsCpuLatticeProcess);
            OpRcPtrVec gpuLut = create3DLut(gpuOpsCpuLatticeProcess, legacy->getEdgelen());
            FinalizeOpVec(gpuLut);

            LogDebug("GPU Ops: Post-3DLUT");
            FinalizeOpVec(gpuOpsHwPostProcess);

            shallowCopy(gpuOps, gpuOpsHwPreProcess);
            shallowCopy(gpuOps, gpuLut);
            shallowCopy(gpuOps, gpuOpsHwPostProcess);
        }
        else
        {
            // Note: finalize() already finalized & optimized the Op list.

            LogDebug("GPU Ops");
            shallowCopy(gpuOps, m_ops);
        }

        // Create the shader program information
        for(auto & op : gpuOps)
        {
            op->extractGpuShaderInfo(shaderDesc);
        }

        WriteShaderHeader(shaderDesc);
        WriteShaderFooter(shaderDesc);

        shaderDesc->finalize();
            
        if(IsDebugLoggingEnabled())
        {
            LogDebug("GPU Shader");
            LogDebug(shaderDesc->getShaderText());
        }
    }


    ///////////////////////////////////////////////////////////////////////////
    
    
    
    void Processor::Impl::addColorSpaceConversion(const Config & config,
                                 const ConstContextRcPtr & context,
                                 const ConstColorSpaceRcPtr & srcColorSpace,
                                 const ConstColorSpaceRcPtr & dstColorSpace)
    {
        BuildColorSpaceOps(m_ops, config, context, srcColorSpace, dstColorSpace);
    }
    
    void Processor::Impl::addTransform(const Config & config,
                      const ConstContextRcPtr & context,
                      const ConstTransformRcPtr& transform,
                      TransformDirection direction)
    {
        BuildOps(m_ops, config, context, transform, direction);
    }

    void Processor::Impl::finalize()
    {
        AutoMutex lock(m_resultsCacheMutex);

        // Pull out metadata, before the no-ops are removed.
        for(auto & op : m_ops)
        {
            op->dumpMetadata(m_metadata);
        }
        
        LogDebug("CPU Ops");
        FinalizeOpVec(m_ops);
    }

}
OCIO_NAMESPACE_EXIT
