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

#include "Op.h"
#include "pystring/pystring.h"

#include <sstream>

OCIO_NAMESPACE_ENTER
{
    Op::~Op()
    { }
    
    bool Op::canCombineWith(const OpRcPtr & /*op*/) const
    {
        return false;
    }
    
    void Op::combineWith(OpRcPtrVec & /*ops*/,
                         const OpRcPtr & /*secondOp*/) const
    {
        std::ostringstream os;
        os << "Op: " << getInfo() << " cannot be combined. ";
        os << "A type-specific combining function is not defined.";
        throw Exception(os.str().c_str());
    }
    
    std::ostream& operator<< (std::ostream & os, const Op & op)
    {
        os << op.getInfo();
        return os;
    }
    
    namespace
    {
        const int FLOAT_DECIMALS = 7;
    }
    
    std::string AllocationData::getCacheID() const
    {
        std::ostringstream os;
        os.precision(FLOAT_DECIMALS);
        os << AllocationToString(allocation) << " ";
        
        for(unsigned int i=0; i<vars.size(); ++i)
        {
            os << vars[i] << " ";
        }
        
        return os.str();
    }
    
    std::ostream& operator<< (std::ostream & os, const AllocationData & allocation)
    {
        os << allocation.getCacheID();
        return os;
    }
    
    std::string SerializeOpVec(const OpRcPtrVec & ops, int indent)
    {
        std::ostringstream os;
        
        for(OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
        {
            os << pystring::mul(" ", indent);
            os << "Op " << i << ": " << *ops[i] << " ";
            os << ops[i]->getCacheID() << " supports_gpu:" << ops[i]->supportsGpuShader();
            os << "\n";
        }
        
        return os.str();
    }
    
    bool IsOpVecNoOp(const OpRcPtrVec & ops)
    {
        for(OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
        {
            if(!ops[i]->isNoOp()) return false;
        }
        
        return true;
    }
    
    void FinalizeOpVec(OpRcPtrVec & ops, bool optimize)
    {
        // TODO: Add envvar to force disable optimizations
        
        if(optimize)
        {
            OptimizeOpVec(ops);
        }
        
        for(OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
        {
            ops[i]->finalize();
        }
    }
}
OCIO_NAMESPACE_EXIT
