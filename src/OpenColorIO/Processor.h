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


#ifndef INCLUDED_OCIO_PROCESSOR_H
#define INCLUDED_OCIO_PROCESSOR_H

#include <OpenColorIO/OpenColorIO.h>

#include "Mutex.h"
#include "Op.h"
#include "PrivateTypes.h"

OCIO_NAMESPACE_ENTER
{
    class Processor::Impl
    {
    private:
        ProcessorMetadataRcPtr m_metadata;

        // Vector of ops for the processor.
        OpRcPtrVec m_ops;

        mutable std::string m_cpuCacheID;
        
        mutable Mutex m_resultsCacheMutex;
        
    public:
        Impl();
        ~Impl();
        
        bool isNoOp() const;
        bool hasChannelCrosstalk() const;
        
        ConstProcessorMetadataRcPtr getMetadata() const;

        DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;

        void apply(ImageDesc& img) const;
        
        void applyRGB(float * pixel) const;
        void applyRGBA(float * pixel) const;
        const char * getCpuCacheID() const;
        
        // Extract all the information to fully implement the processor shader program.
        void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;

        // Get a CPU processor instance for arbitrary input and output pixel formats.
        ConstCPUProcessorRcPtr getCPUProcessor(PixelFormat in, PixelFormat out) const;

        ////////////////////////////////////////////
        //
        // Builder functions, Not exposed
        
        void addColorSpaceConversion(const Config & config,
                                     const ConstContextRcPtr & context,
                                     const ConstColorSpaceRcPtr & srcColorSpace,
                                     const ConstColorSpaceRcPtr & dstColorSpace);
        
        void addTransform(const Config & config,
                          const ConstContextRcPtr & context,
                          const ConstTransformRcPtr& transform,
                          TransformDirection direction);
        
        void finalize();
    };
    
}
OCIO_NAMESPACE_EXIT

#endif
