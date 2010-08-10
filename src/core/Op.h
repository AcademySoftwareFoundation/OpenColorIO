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


#ifndef INCLUDED_OCIO_OP_H
#define INCLUDED_OCIO_OP_H

#include <OpenColorIO/OpenColorIO.h>

#include "Config.h"

#include <vector>

OCIO_NAMESPACE_ENTER
{
    class Op
    {
        public:
            virtual ~Op();
            
            // This is called a single time after construction.
            // All pre-processing and safety checks should happen here,
            // rather than in the constructor.
            
            virtual void setup() = 0;
            
            // Render the specified pixels.
            //
            // This must be safe to call in a multi-threaded context.
            // Ops that have mutable data internally, or rely on external
            // caching, must thus be appropriately mutexed.
            
            virtual void apply(float* rgbaBuffer, long numPixels) const = 0;
            
            //! Does this op support gpu shader text generation
            virtual bool supportsGPUShader() const = 0;
            
        private:
            Op& operator= (const Op &);
    };
    
    typedef SharedPtr<Op> OpRcPtr;
    typedef std::vector<OpRcPtr> OpRcPtrVec;
}
OCIO_NAMESPACE_EXIT

#endif
