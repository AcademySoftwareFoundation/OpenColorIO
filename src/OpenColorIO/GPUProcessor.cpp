// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GPUProcessor.h"
#include "GpuShader.h"
#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "Logging.h"
#include "ops/allocation/AllocationOp.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ops/noop/NoOps.h"


namespace OCIO_NAMESPACE
{

namespace
{

void WriteShaderHeader(GpuShaderCreatorRcPtr & shaderCreator)
{
    const std::string fcnName(shaderCreator->getFunctionName());

    GpuShaderText ss(shaderCreator->getLanguage());

    ss.newLine();
    ss.newLine() << "// Declaration of the OCIO shader function";
    ss.newLine();

    ss.newLine() << ss.float4Keyword() << " " << fcnName
                 << "(in "  << ss.float4Keyword() << " inPixel)";
    ss.newLine() << "{";
    ss.indent();
    ss.newLine() << ss.float4Keyword() << " "
                 << shaderCreator->getPixelName() << " = inPixel;";

    shaderCreator->addToFunctionHeaderShaderCode(ss.string().c_str());
}


void WriteShaderFooter(GpuShaderCreatorRcPtr & shaderCreator)
{
    GpuShaderText ss(shaderCreator->getLanguage());

    ss.newLine();
    ss.indent();
    ss.newLine() << "return " << shaderCreator->getPixelName() << ";";
    ss.dedent();
    ss.newLine() << "}";

    shaderCreator->addToFunctionFooterShaderCode(ss.string().c_str());
}


OpRcPtrVec Create3DLut(const OpRcPtrVec & ops, unsigned edgelen)
{
    if(ops.size()==0) return OpRcPtrVec();

    const unsigned lut3DEdgeLen   = edgelen;
    const unsigned lut3DNumPixels = lut3DEdgeLen*lut3DEdgeLen*lut3DEdgeLen;

    Lut3DOpDataRcPtr lut = std::make_shared<Lut3DOpData>(lut3DEdgeLen);

    // Allocate 3D LUT image, RGBA
    std::vector<float> lut3D(lut3DNumPixels*4);
    GenerateIdentityLut3D(&lut3D[0], lut3DEdgeLen, 4, LUT3DORDER_FAST_BLUE);

    // Apply the lattice ops to it
    for(const auto & op : ops)
    {
        op->apply(&lut3D[0], &lut3D[0], lut3DNumPixels);
    }

    // Convert the RGBA image to an RGB image, in place.
    auto & lutArray = lut->getArray();
    for(unsigned i=0; i<lut3DNumPixels; ++i)
    {
        lutArray[3*i+0] = lut3D[4*i+0];
        lutArray[3*i+1] = lut3D[4*i+1];
        lutArray[3*i+2] = lut3D[4*i+2];
    }

    OpRcPtrVec newOps;
    CreateLut3DOp(newOps, lut, TRANSFORM_DIR_FORWARD);
    return newOps;
}

}

void GPUProcessor::Impl::finalize(const OpRcPtrVec & rawOps,
                                  OptimizationFlags oFlags)
{
    AutoMutex lock(m_mutex);

    // Prepare the list of ops.

    m_ops = rawOps;

    m_ops.finalize(oFlags);
    m_ops.validateDynamicProperties();

    // Is NoOp ?
    m_isNoOp  = m_ops.isNoOp();

    // Does the color processing introduce crosstalk between the pixel channels?
    m_hasChannelCrosstalk = m_ops.hasChannelCrosstalk();

    // Calculate and assemble the GPU cache ID from the ops.

    std::stringstream ss;
    ss << "GPU Processor: oFlags " << oFlags
       << " ops :";
    for(const auto & op : m_ops)
    {
        ss << " " << op->getCacheID();
    }

    m_cacheID = ss.str();
}

void GPUProcessor::Impl::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    AutoMutex lock(m_mutex);

    OpRcPtrVec gpuOps;

    LegacyGpuShaderDesc * legacy = dynamic_cast<LegacyGpuShaderDesc*>(shaderCreator.get());
    if(legacy)
    {
        gpuOps = m_ops;

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
        OpRcPtrVec gpuLut = Create3DLut(gpuOpsCpuLatticeProcess, legacy->getEdgelen());

        gpuOps.clear();
        gpuOps += gpuOpsHwPreProcess;
        gpuOps += gpuLut;
        gpuOps += gpuOpsHwPostProcess;

        gpuOps.finalize(OPTIMIZATION_DEFAULT);
    }
    else
    {
        gpuOps = m_ops;
    }

    // Create the shader program information.
    for(const auto & op : gpuOps)
    {
        op->extractGpuShaderInfo(shaderCreator);
    }

    WriteShaderHeader(shaderCreator);
    WriteShaderFooter(shaderCreator);

    shaderCreator->finalize();
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

void GPUProcessor::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
{
    GpuShaderCreatorRcPtr shaderCreator = DynamicPtrCast<GpuShaderCreator>(shaderDesc);
    getImpl()->extractGpuShaderInfo(shaderCreator);
}

void GPUProcessor::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    // Note that several generated fragment shader programs could be in the same
    // global fragment shader program (i.e. being embedded in another one). To avoid
    // any resource name conflict the processor instance provides a unique identifier
    // to uniquely name the resources (when the color transformations are simlar
    // i.e. same ops with different values) or as a key for a cache mechanism
    // (color transforms are identical so a shader program could be reused).

    // Build a unique key usable by the fragment shader program.

    std::string tmpKey(shaderCreator->getCacheID());
    tmpKey += getImpl()->getCacheID();

    // Way too long uid for a resource name so shorten it.
    std::string key(CacheIDHash(tmpKey.c_str(), (int)tmpKey.size()));

    // Prepend a user defined uid if any.
    if (std::strlen(shaderCreator->getUniqueID())!=0)
    {
        key = shaderCreator->getUniqueID() + key;
    }

    if (!std::isalpha(key[0]))
    {
        // A resource name must start with a letter.
        key = "k_" + key;
    }

    // A resource name only accepts alphanumeric characters.
    key.erase(std::remove_if(key.begin(), key.end(),
                             [](char const & c) -> bool { return !std::isalnum(c) && c!='_'; } ),
              key.end());

    // Extract the information to fully build the fragment shader program.

    shaderCreator->begin(key.c_str());

    try
    {
        getImpl()->extractGpuShaderInfo(shaderCreator);
    }
    catch(const Exception &)
    {
        shaderCreator->end();
        throw;
    }

    shaderCreator->end();
}


} // namespace OCIO_NAMESPACE

