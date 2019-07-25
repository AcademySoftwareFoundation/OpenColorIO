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


#ifndef INCLUDED_OCIO_CPUPROCESSOR_H
#define INCLUDED_OCIO_CPUPROCESSOR_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


OCIO_NAMESPACE_ENTER
{

class ScanlineHelper;

class CPUProcessor::Impl
{
public:
    Impl() = default;
    Impl(const Impl &) = delete;
    Impl& operator=(const Impl &) = delete;

    ~Impl() = default;

    bool hasChannelCrosstalk() const noexcept { return m_hasChannelCrosstalk; }

    const char * getCacheID() const noexcept { return m_cacheID.c_str(); }

    BitDepth getInputBitDepth() const noexcept { return m_inBitDepth; }
    BitDepth getOutputBitDepth() const noexcept { return m_outBitDepth; }

    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;

    void apply(ImageDesc & imgDesc) const;
    void apply(const ImageDesc & srcImgDesc, ImageDesc & dstImgDesc) const;
    void applyRGB(void * pixel) const;
    void applyRGBA(void * pixel) const;

    ////////////////////////////////////////////
    //
    // Functions not exposed to the OCIO public API.
        
    void finalize(const OpRcPtrVec & rawOps,
                  BitDepth in, BitDepth out,
                  OptimizationFlags oFlags, FinalizationFlags fFlags);

private:
    ConstOpCPURcPtr    m_inBitDepthOp; // Converts from in to F32. It could be done by the first op.
    ConstOpCPURcPtrVec m_cpuOps;       // It could be empty if the OpVec only contains a 1D LUT op
                                       // (e.g. the 1D LUT CPUOp instance would be in the m_inBitDepthOp).
    ConstOpCPURcPtr    m_outBitDepthOp;// Converts from F32 to out. It could be done by the last op.

    BitDepth           m_inBitDepth = BIT_DEPTH_F32;
    BitDepth           m_outBitDepth = BIT_DEPTH_F32;
    bool               m_hasChannelCrosstalk = true;
    std::string        m_cacheID;
    Mutex              m_mutex;

    std::unique_ptr<ScanlineHelper> m_scanlineBuilder;
};


}
OCIO_NAMESPACE_EXIT


#endif
