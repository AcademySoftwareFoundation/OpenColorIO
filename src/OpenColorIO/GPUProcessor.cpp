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

    if (shaderCreator->getLanguage() == LANGUAGE_OSL_1)
    {
        ss.newLine() << "color4 " << fcnName << "(color4 inPixel)";
        ss.newLine() << "{";
        ss.indent();
        ss.newLine() << "color4 " << shaderCreator->getPixelName() << " = inPixel;";
    }
    else
    {
        ss.newLine() << ss.float4Keyword() << " " << fcnName 
                     << "(" << ss.float4Keyword() << " inPixel)";
        ss.newLine() << "{";
        ss.indent();
        ss.newLine() << ss.float4Decl(shaderCreator->getPixelName()) << " = inPixel;";
    }

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


}

void GPUProcessor::Impl::finalize(const OpRcPtrVec & rawOps, OptimizationFlags oFlags)
{
    AutoMutex lock(m_mutex);

    // Prepare the list of ops.

    m_ops = rawOps;

    m_ops.finalize();
    m_ops.optimize(oFlags);
    m_ops.validateDynamicProperties();

    // Is NoOp ?
    m_isNoOp  = m_ops.isNoOp();

    // Does the color processing introduce crosstalk between the pixel channels?
    m_hasChannelCrosstalk = m_ops.hasChannelCrosstalk();

    // Calculate and assemble the GPU cache ID from the ops.

    std::stringstream ss;
    ss << "GPU Processor: oFlags " << oFlags
       << " ops : " << m_ops.getCacheID();

    m_cacheID = ss.str();
}

void GPUProcessor::Impl::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    AutoMutex lock(m_mutex);

    // Create the shader program information.
    for(const auto & op : m_ops)
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
    std::string key(CacheIDHash(tmpKey.c_str(), tmpKey.size()));

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
