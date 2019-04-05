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

class CPUProcessor::Impl
{
public:
    Impl() = default;
    ~Impl() = default;

    bool isNoOp() const { return m_ops.empty(); }

    bool hasChannelCrosstalk() const { return m_hasChannelCrosstalk; }

    const char * getCacheID() const;

    void finalize(const OpRcPtrVec & ops, PixelFormat in, PixelFormat out);

    PixelFormat getInputPixelFormat() const { return m_inPixelFormat; }
    PixelFormat getOutputPixelFormat() const { return m_outPixelFormat; }

    void apply(const void * inImg, void * outImg, long numPixels) const;

private:
    ConstOpCPURcPtrVec m_ops;
    PixelFormat        m_inPixelFormat = PIXEL_FORMAT_RGBA_F32;
    PixelFormat        m_outPixelFormat = PIXEL_FORMAT_RGBA_F32;
    bool               m_hasChannelCrosstalk = true;
    std::string        m_cacheID;
    size_t             m_numOps = 0;
    bool               m_reuseOutBuffer = false;
};


}
OCIO_NAMESPACE_EXIT


#endif
