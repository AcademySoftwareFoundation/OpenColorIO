/*
Copyright (c) 2019 Autodesk Inc., et al.
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

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GPUProcessor.h"
#include "GpuShader.h"
#include "GpuShaderUtils.h"
#include "Logging.h"
#include "ops/Allocation/AllocationOp.h"
#include "ops/Lut3D/Lut3DOp.h"
#include "ops/NoOp/NoOps.h"


OCIO_NAMESPACE_ENTER
{

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


OpRcPtrVec Create3DLut(const OpRcPtrVec & ops, unsigned edgelen)
{
    if(ops.size()==0) return OpRcPtrVec();

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
    for(const auto & op : ops)
    {
        op->apply(&lut3D[0], &lut3D[0], lut3DNumPixels);
    }
    
    // Convert the RGBA image to an RGB image, in place.           
    for(unsigned i=0; i<lut3DNumPixels; ++i)
    {
        lut->lut[3*i+0] = lut3D[4*i+0];
        lut->lut[3*i+1] = lut3D[4*i+1];
        lut->lut[3*i+2] = lut3D[4*i+2];
    }

    OpRcPtrVec newOps;
    CreateLut3DOp(newOps, lut, INTERP_LINEAR, TRANSFORM_DIR_FORWARD);
    return newOps;
}

}


DynamicPropertyRcPtr GPUProcessor::Impl::getDynamicProperty(DynamicPropertyType type) const
{
    for(const auto & op : m_ops)
    {
        if(op->hasDynamicProperty(type))
        {
            return op->getDynamicProperty(type);
        }
    }

    throw Exception("Cannot find dynamic property; not used by GPU processor.");
}

void GPUProcessor::Impl::finalize(const OpRcPtrVec & rawOps,
                                  OptimizationFlags oFlags,
                                  FinalizationFlags fFlags)
{
    AutoMutex lock(m_mutex);

    // Prepare the list of ops.

    m_ops = rawOps.clone();

    // GPU Processor only supports F32.
    for(auto & op : m_ops)
    {
        op->setInputBitDepth(BIT_DEPTH_F32);
        op->setOutputBitDepth(BIT_DEPTH_F32);
    }

    OptimizeOpVec(m_ops, oFlags);
    FinalizeOpVec(m_ops, fFlags);
    UnifyDynamicProperties(m_ops);

    // Does the color processing introduce crosstalk between the pixel channels?

    m_hasChannelCrosstalk = false;
    for(const auto & op : m_ops)
    {
        if(op->hasChannelCrosstalk())
        {
            m_hasChannelCrosstalk = true;
            break;
        }
    }

    // Compute the cache id.

    std::stringstream ss;
    ss << "GPU Processor: oFlags " << oFlags
       << " fFlags " << fFlags
       << " ops :";
    for(const auto & op : m_ops)
    {
        ss << " " << op->getCacheID();
    }

    m_cacheID = ss.str();
}

void GPUProcessor::Impl::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
{
    AutoMutex lock(m_mutex);

    OpRcPtrVec gpuOps;

    LegacyGpuShaderDesc * legacy = dynamic_cast<LegacyGpuShaderDesc*>(shaderDesc.get());
    if(legacy)
    {
        gpuOps = m_ops.clone();

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
                        gpuOps);

        LogDebug("GPU Ops: 3DLUT");
        FinalizeOpVec(gpuOpsCpuLatticeProcess, FINALIZATION_DEFAULT);
        OpRcPtrVec gpuLut = Create3DLut(gpuOpsCpuLatticeProcess, legacy->getEdgelen());

        gpuOps.clear();
        gpuOps += gpuOpsHwPreProcess;
        gpuOps += gpuLut;
        gpuOps += gpuOpsHwPostProcess;

        OptimizeOpVec(gpuOps, OPTIMIZATION_DEFAULT);
        FinalizeOpVec(gpuOps, FINALIZATION_DEFAULT);
    }
    else
    {
        gpuOps = m_ops;
    }

    // Create the shader program information
    for(const auto & op : gpuOps)
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


//////////////////////////////////////////////////////////////////////////


void GPUProcessor::deleter(GPUProcessor * c)
{
    delete c;
}

GPUProcessor::GPUProcessor()
    :   m_impl(new Impl)
{
}

GPUProcessor::~GPUProcessor()
{
    delete m_impl;
    m_impl = nullptr;
}

bool GPUProcessor::isNoOp() const
{
    return getImpl()->isNoOp();
}

bool GPUProcessor::hasChannelCrosstalk() const
{
    return getImpl()->hasChannelCrosstalk();
}

const char * GPUProcessor::getCacheID() const
{
    return getImpl()->getCacheID();
}

DynamicPropertyRcPtr GPUProcessor::getDynamicProperty(DynamicPropertyType type) const
{
    return getImpl()->getDynamicProperty(type);
}

void GPUProcessor::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
{
    return getImpl()->extractGpuShaderInfo(shaderDesc);
}


}
OCIO_NAMESPACE_EXIT

