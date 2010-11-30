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

#include "AllocationOp.h"
#include "LogOps.h"
#include "MatrixOps.h"
#include "Op.h"

#include <OpenColorIO/OpenColorIO.h>

OCIO_NAMESPACE_ENTER
{

    void CreateAllocationOps(OpRcPtrVec & ops,
                             const AllocationData & data,
                             TransformDirection dir)
    {
        if(data.allocation == ALLOCATION_UNIFORM)
        {
            float oldmin[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            float oldmax[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            float newmin[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            float newmax[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            
            if(data.vars.size() >= 2)
            {
                for(int i=0; i<3; ++i)
                {
                    oldmin[i] = data.vars[0];
                    oldmax[i] = data.vars[1];
                }
            }
            
            CreateFitOp(ops,
                        oldmin, oldmax,
                        newmin, newmax,
                        dir);
        }
        else if(data.allocation == ALLOCATION_LG2)
        {
            float oldmin[4] = { -10.0f, -10.0f, -10.0f, 0.0f };
            float oldmax[4] = { 6.0f, 6.0f, 6.0f, 1.0f };
            float newmin[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            float newmax[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            
            if(data.vars.size() >= 2)
            {
                for(int i=0; i<3; ++i)
                {
                    oldmin[i] = data.vars[0];
                    oldmax[i] = data.vars[1];
                }
            }
            
            
            // Log Settings
            // output = k * log(mx+b, base) + kb
            
            float k[3] = { 1.0f, 1.0f, 1.0f };
            float m[3] = { 1.0f, 1.0f, 1.0f };
            float b[3] = { 0.0f, 0.0f, 0.0f };
            float base[3] = { 2.0f, 2.0f, 2.0f };
            float kb[3] = { 0.0f, 0.0f, 0.0f };
            
            if(data.vars.size() >= 3)
            {
                for(int i=0; i<3; ++i)
                {
                    b[i] = data.vars[2];
                }
            }
            
            if(dir == TRANSFORM_DIR_FORWARD)
            {
                CreateLogOp(ops, k, m, b, base, kb, dir);
                
                CreateFitOp(ops,
                            oldmin, oldmax,
                            newmin, newmax,
                            dir);
            }
            else if(dir == TRANSFORM_DIR_INVERSE)
            {
                CreateFitOp(ops,
                            oldmin, oldmax,
                            newmin, newmax,
                            dir);
                
                CreateLogOp(ops, k, m, b, base, kb, dir);
            }
            else
            {
                throw Exception("Cannot BuildAllocationOps, unspecified transform direction.");
            }
        }
        else
        {
            throw Exception("Unsupported Allocation Type.");
        }
    }
    
}
OCIO_NAMESPACE_EXIT
