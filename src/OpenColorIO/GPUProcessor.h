// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GPUPROCESSOR_H
#define INCLUDED_OCIO_GPUPROCESSOR_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


namespace OCIO_NAMESPACE
{

class GPUProcessor::Impl
{
public:
    Impl() = default;
    ~Impl() = default;

    bool isNoOp() const noexcept { return m_isNoOp; }

    bool hasChannelCrosstalk() const noexcept { return m_hasChannelCrosstalk; }

    const char * getCacheID() const noexcept { return m_cacheID.c_str(); }

    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;

    void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;
    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const;

    ////////////////////////////////////////////
    //
    // Builder functions, Not exposed

    void finalize(const OpRcPtrVec & rawOps, OptimizationFlags oFlags);

private:
    OpRcPtrVec    m_ops;
    bool          m_isNoOp = false;
    bool          m_hasChannelCrosstalk = true;
    std::string   m_cacheID;
    mutable Mutex m_mutex;
};


} // namespace OCIO_NAMESPACE


#endif
