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

#include "Op.h"

OCIO_NAMESPACE_ENTER
{
    class LocalProcessor;
    typedef SharedPtr<const LocalProcessor> ConstLocalProcessorRcPtr;
    typedef SharedPtr<LocalProcessor> LocalProcessorRcPtr;
    
    class LocalProcessor : public Processor
    {
    public:
        static LocalProcessorRcPtr Create();
        
        virtual ~LocalProcessor();
        
        // Public interface
        
        virtual bool isNoOp() const;
        
        virtual void apply(ImageDesc& img) const;
        
        virtual void applyRGB(float * pixel) const;
        virtual void applyRGBA(float * pixel) const;
        
        virtual const char * getGpuShaderText(const GpuShaderDesc & gpuDesc) const;
        virtual const char * getGpuLut3DCacheID(const GpuShaderDesc & shaderDesc) const;
        
        virtual void getGpuLut3D(float* lut3d, const GpuShaderDesc & shaderDesc) const;
        
        
        ////////////////////////////////////////////
        //
        //  Not exposed
        
        
        std::string getInfo() const;
        
        ////////////////////////////////////////////
        //
        // Builder functions
        
        void registerOp(OpRcPtr op);
        
        void finalizeOps();
        
    private:
        LocalProcessor();
        LocalProcessor(const LocalProcessor &);
        LocalProcessor& operator= (const LocalProcessor &);
        
        static void deleter(LocalProcessor* p);
        
        OpRcPtrVec m_opVec;
        
        mutable std::string m_shaderText;
        mutable std::string m_lut3DHash;
    };
    
    
    // TODO: Move these!
    // TODO: Its not ideal that buildops requires a config to be passed around
    // but the only alternative is to make build ops a function on it?
    // and even if it were, what about the build calls it dispatches to...
    
    // TODO: all of the build op functions shouldnt take a LocalProcessor class
    // Instead, they should take an abstract interface class that defines
    // registerOp(OpRcPtr op), annotateColorSpace, finalizeOps, etc.
    // of which LocalProcessor happens to be one example.
    // Then the only location in the codebase that knows of LocalProcessor is
    // in Config.cpp, which creates one.
    
    void BuildOps(LocalProcessor & processor,
                  const Config & config,
                  const ConstTransformRcPtr & transform,
                  TransformDirection dir);
}
OCIO_NAMESPACE_EXIT

#endif
