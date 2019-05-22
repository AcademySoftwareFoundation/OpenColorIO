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

#include <OpenColorIO/OpenColorIO.h>

#include "Logging.h"
#include "Op.h"

#include <iterator>
#include <sstream>
#include <algorithm>

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        const int MAX_OPTIMIZATION_PASSES = 8;
        
        int RemoveNoOps(OpRcPtrVec & opVec)
        {
            int count = 0;
            
            OpRcPtrVec::iterator iter = opVec.begin();
            while(iter != opVec.end())
            {
                if((*iter)->isNoOp())
                {
                    iter = opVec.erase(iter);
                    ++count;
                }
                else
                {
                    ++iter;
                }
            }
            
            return count;
        }
        
        int RemoveInverseOps(OpRcPtrVec & opVec)
        {
            int count = 0;
            int firstindex = 0; // this must be a signed int
            
            while(firstindex < static_cast<int>(opVec.size()-1))
            {
                ConstOpRcPtr first = opVec[firstindex];
                ConstOpRcPtr second = opVec[firstindex+1];
                
                // The common case of inverse ops is to have a deep nesting:
                // ..., A, B, B', A', ...
                //
                // Consider the above, when firstindex reaches B:
                //
                //         |
                // ..., A, B, B', A', ...
                //
                // We will remove B and B'.
                // Firstindex remains pointing at the original location:
                // 
                //         |
                // ..., A, A', ...
                //
                // We then decrement firstindex by 1,
                // to backstep and reconsider the A, A' case:
                //
                //      |            <-- firstindex decremented
                // ..., A, A', ...
                //
                
                if(first->isSameType(second) && first->isInverse(second))
                {
                    opVec.erase(opVec.begin() + firstindex,
                        opVec.begin() + firstindex + 2);
                    ++count;
                    
                    firstindex = std::max(0, firstindex-1);
                }
                else
                {
                    ++firstindex;
                }
            }
            
            return count;
        }
        
        int CombineOps(OpRcPtrVec & opVec)
        {
            int count = 0;
            int firstindex = 0; // this must be a signed int
            
            OpRcPtrVec tmpops;
            
            while(firstindex < static_cast<int>(opVec.size()-1))
            {
                ConstOpRcPtr first = opVec[firstindex];
                ConstOpRcPtr second = opVec[firstindex+1];
                
                if(first->canCombineWith(second))
                {
                    tmpops.clear();
                    first->combineWith(tmpops, second);
                    
                    // tmpops may have any number of ops in it. (0, 1, 2, ...)
                    // (size 0 would occur potentially iff the combination 
                    // results in a no-op)
                    //
                    // No matter the number, we need to swap them in for the
                    // original ops
                    
                    // Erase the initial two ops we've combined
                    opVec.erase(opVec.begin() + firstindex,
                        opVec.begin() + firstindex + 2);
                    
                    // Insert the new ops (which may be empty) at
                    // this location
                    opVec.insert(opVec.begin() + firstindex, tmpops.begin(), tmpops.end());
                    
                    // Decrement firstindex by 1,
                    // to backstep and reconsider the A, A' case.
                    // See RemoveInverseOps for the full discussion of
                    // why this is appropriate
                    firstindex = std::max(0, firstindex-1);
                    
                    // We've done something so increment the count!
                    ++count;
                }
                else
                {
                    ++firstindex;
                }
            }
            
            return count;
        }
    }
    
    void OptimizeOpVec(OpRcPtrVec & ops)
    {
        if(ops.empty()) return;
        
        
        if(IsDebugLoggingEnabled())
        {
            LogDebug("Optimizing Op Vec...");
            LogDebug(SerializeOpVec(ops, 4));
        }
        
        OpRcPtrVec::size_type originalSize = ops.size();
        int total_noops = 0;
        int total_inverseops = 0;
        int total_combines = 0;
        int passes = 0;
        
        while(passes<=MAX_OPTIMIZATION_PASSES)
        {
            int noops = RemoveNoOps(ops);
            int inverseops = RemoveInverseOps(ops);
            int combines = CombineOps(ops);
            
            if(noops == 0 && inverseops==0 && combines==0)
            {
                // No optimization progress was made, so stop trying.
                break;
            }
            
            total_noops += noops;
            total_inverseops += inverseops;
            total_combines += combines;
            
            ++passes;
        }
        
        OpRcPtrVec::size_type finalSize = ops.size();
        
        if(passes == MAX_OPTIMIZATION_PASSES)
        {
            std::ostringstream os;
            os << "The max number of passes, " << passes << ", ";
            os << "was reached during optimization. This is likely a sign ";
            os << "that either the complexity of the color transform is ";
            os << "very high, or that some internal optimizers are in conflict ";
            os << "(undo-ing / redo-ing the other's results).";
            LogDebug(os.str().c_str());
        }
        
        if(IsDebugLoggingEnabled())
        {
            std::ostringstream os;
            os << "Optimized ";
            os << originalSize << "->" << finalSize << ", ";
            os << passes << " passes, ";
            os << total_noops << " noops removed, ";
            os << total_inverseops << " inverse ops removed\n";
            os << total_combines << " ops combines\n";
            os << SerializeOpVec(ops, 4);
            LogDebug(os.str());
        }
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

#include "ops/Exponent/ExponentOps.h"
#include "ops/Log/LogOps.h"
#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Lut3D/Lut3DOp.h"
#include "ops/Matrix/MatrixOps.h"

OIIO_ADD_TEST(OpOptimizers, RemoveInverseOps)
{
    OCIO::OpRcPtrVec ops;
    
    const double exp[4] = { 1.2, 1.3, 1.4, 1.5 };
    
    double logSlope[3] = { 0.18, 0.18, 0.18 };
    double linSlope[3] = { 2.0, 2.0, 2.0 };
    double linOffset[3] = { 0.1, 0.1, 0.1 };
    double base = 10.0;
    double logOffset[3] = { 1.0, 1.0, 1.0 };
    
    OCIO::CreateExponentOp(ops, exp, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_INVERSE);
    OCIO::CreateExponentOp(ops, exp, OCIO::TRANSFORM_DIR_INVERSE);
    
    OIIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveInverseOps(ops);
    OIIO_CHECK_EQUAL(ops.size(), 0);
    
    
    ops.clear();
    OCIO::CreateExponentOp(ops, exp, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateExponentOp(ops, exp, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_INVERSE);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateExponentOp(ops, exp, OCIO::TRANSFORM_DIR_FORWARD);
    
    OIIO_CHECK_EQUAL(ops.size(), 5);
    OCIO::RemoveInverseOps(ops);
    OIIO_CHECK_EQUAL(ops.size(), 1);
}


OIIO_ADD_TEST(OpOptimizers, CombineOps)
{
    float m1[4] = { 2.0f, 2.0f, 2.0f, 1.0f };
    float m2[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
    float m3[4] = { 0.6f, 0.6f, 0.6f, 1.0f };
    float m4[4] = { 0.7f, 0.7f, 0.7f, 1.0f };
    
    const double exp[4] = { 1.2, 1.3, 1.4, 1.5 };
    
    {
    OCIO::OpRcPtrVec ops;
    OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
    
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OCIO::CombineOps(ops);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    }
    
    {
    OCIO::OpRcPtrVec ops;
    OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateScaleOp(ops, m3, OCIO::TRANSFORM_DIR_FORWARD);
    
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OCIO::CombineOps(ops);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    }
    
    {
    OCIO::OpRcPtrVec ops;
    OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateScaleOp(ops, m3, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateScaleOp(ops, m4, OCIO::TRANSFORM_DIR_FORWARD);
    
    OIIO_CHECK_EQUAL(ops.size(), 3);
    OCIO::CombineOps(ops);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    }
    
    {
    OCIO::OpRcPtrVec ops;
    OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateScaleOp(ops, m2, OCIO::TRANSFORM_DIR_FORWARD);
    
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OCIO::CombineOps(ops);
    OIIO_CHECK_EQUAL(ops.size(), 0);
    }
    
    {
    OCIO::OpRcPtrVec ops;
    OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_INVERSE);
    
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OCIO::CombineOps(ops);
    OIIO_CHECK_EQUAL(ops.size(), 0);
    }
    
    {
    OCIO::OpRcPtrVec ops;
    OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
    
    OIIO_CHECK_EQUAL(ops.size(), 5);
    OCIO::CombineOps(ops);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    }
    
    {
    OCIO::OpRcPtrVec ops;
    OCIO::CreateExponentOp(ops, exp, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateScaleOp(ops, m2, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateExponentOp(ops, exp, OCIO::TRANSFORM_DIR_INVERSE);
    
    OIIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::CombineOps(ops);
    OIIO_CHECK_EQUAL(ops.size(), 0);
    }
}

#endif // OCIO_UNIT_TEST
