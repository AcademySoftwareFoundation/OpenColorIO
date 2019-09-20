// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GPUPROCESSOR_H
#define INCLUDED_OCIO_GPUPROCESSOR_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


OCIO_NAMESPACE_ENTER
{

class GPUProcessor::Impl
{
public:
    Impl() = default;
    ~Impl() = default;

    bool isNoOp() const noexcept { return m_ops.empty(); }

    bool hasChannelCrosstalk() const noexcept { return m_hasChannelCrosstalk; }

    const char * getCacheID() const noexcept { return m_cacheID.c_str(); }

    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;

    void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;

    ////////////////////////////////////////////
    //
    // Builder functions, Not exposed
        
    void finalize(const OpRcPtrVec & rawOps,
                  OptimizationFlags oFlags, FinalizationFlags fFlags);

private:
    OpRcPtrVec    m_ops;
    bool          m_hasChannelCrosstalk = true;
    std::string   m_cacheID;
    mutable Mutex m_mutex;
};


}
OCIO_NAMESPACE_EXIT


#endif
