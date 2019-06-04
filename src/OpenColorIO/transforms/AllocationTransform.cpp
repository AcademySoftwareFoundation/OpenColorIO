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

#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Allocation/AllocationOp.h"
#include "OpBuilders.h"

OCIO_NAMESPACE_ENTER
{
    AllocationTransformRcPtr AllocationTransform::Create()
    {
        return AllocationTransformRcPtr(new AllocationTransform(), &deleter);
    }
    
    void AllocationTransform::deleter(AllocationTransform* t)
    {
        delete t;
    }
    
    
    class AllocationTransform::Impl
    {
    public:
        TransformDirection dir_;
        Allocation allocation_;
        std::vector<float> vars_;
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD),
            allocation_(ALLOCATION_UNIFORM)
        { }

        Impl(const Impl &) = delete;

        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                dir_ = rhs.dir_;
                allocation_ = rhs.allocation_;
                vars_ = rhs.vars_;
            }
            return *this;
        }
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    AllocationTransform::AllocationTransform()
        : m_impl(new AllocationTransform::Impl)
    {
    }
    
    TransformRcPtr AllocationTransform::createEditableCopy() const
    {
        AllocationTransformRcPtr transform = AllocationTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    AllocationTransform::~AllocationTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    AllocationTransform& AllocationTransform::operator= (const AllocationTransform & rhs)
    {
        if (this != &rhs)
        {
            *m_impl = *rhs.m_impl;
        }
        return *this;
    }
    
    TransformDirection AllocationTransform::getDirection() const
    {
        return getImpl()->dir_;
    }
    
    void AllocationTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }

    void AllocationTransform::validate() const
    {
        Transform::validate();

        if (getImpl()->allocation_ == ALLOCATION_UNIFORM)
        {
            if(getImpl()->vars_.size()!=2 && getImpl()->vars_.size()!=0)
            {
                throw Exception("AllocationTransform: wrong number of values for the uniform allocation");
            }
        }
        else if (getImpl()->allocation_ == ALLOCATION_LG2)
        {
            if(getImpl()->vars_.size()!=3 && getImpl()->vars_.size()!=2 && getImpl()->vars_.size()!=0)
            {
                throw Exception("AllocationTransform: wrong number of values for the logarithmic allocation");
            }
        }
        else
        {
            throw Exception("AllocationTransform: invalid allocation type");
        }
    }

    Allocation AllocationTransform::getAllocation() const
    {
        return getImpl()->allocation_;
    }
    
    void AllocationTransform::setAllocation(Allocation allocation)
    {
        getImpl()->allocation_ = allocation;
    }
    
    int AllocationTransform::getNumVars() const
    {
        return static_cast<int>(getImpl()->vars_.size());
    }
    
    void AllocationTransform::getVars(float * vars) const
    {
        if(!getImpl()->vars_.empty())
        {
            memcpy(vars,
                &getImpl()->vars_[0],
                getImpl()->vars_.size()*sizeof(float));
        }
    }
    
    void AllocationTransform::setVars(int numvars, const float * vars)
    {
        getImpl()->vars_.resize(numvars);
        
        if(!getImpl()->vars_.empty())
        {
            memcpy(&getImpl()->vars_[0],
                vars,
                numvars*sizeof(float));
        }
    }
    
    std::ostream& operator<< (std::ostream& os, const AllocationTransform& t)
    {
        Allocation allocation(t.getAllocation());
        int numVars(t.getNumVars());
        std::vector<float> vars(numVars);
        t.getVars(&vars[0]);

        os << "<AllocationTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection());
        if (numVars)
        {
            os << ", allocation=" << AllocationToString(allocation) << ", ";
            os << "vars=" << vars[0];
            for (int i = 1; i < numVars; ++i)
            {
                os << " " << vars[i];
            }
        }
        os << ">";
        
        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    void BuildAllocationOps(OpRcPtrVec & ops,
                      const Config& /*config*/,
                      const AllocationTransform& allocationTransform,
                      TransformDirection dir)
    {
        TransformDirection combinedDir = CombineTransformDirections(dir,
                                                  allocationTransform.getDirection());
        
        AllocationData data;
        data.allocation = allocationTransform.getAllocation();
        data.vars.resize(allocationTransform.getNumVars());
        if(!data.vars.empty())
        {
            allocationTransform.getVars(&data.vars[0]);
        }
        
        CreateAllocationOps(ops, data, combinedDir);
    }
}
OCIO_NAMESPACE_EXIT


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(AllocationTransform, allocation)
{
    OCIO::AllocationTransformRcPtr al = OCIO::AllocationTransform::Create();

    al->setAllocation(OCIO::ALLOCATION_UNIFORM);
    OIIO_CHECK_NO_THROW(al->validate());

    std::vector<float> envs(2, 0.0f);
    al->setVars(static_cast<int>(envs.size()), &envs[0]);
    OIIO_CHECK_NO_THROW(al->validate());

    envs.push_back(0.01f);
    al->setVars(static_cast<int>(envs.size()), &envs[0]);
    OIIO_CHECK_THROW(al->validate(), OCIO::Exception);

    al->setAllocation(OCIO::ALLOCATION_LG2);
    OIIO_CHECK_NO_THROW(al->validate());

    envs.push_back(0.1f);
    al->setVars(static_cast<int>(envs.size()), &envs[0]);
    OIIO_CHECK_THROW(al->validate(), OCIO::Exception);

    al->setVars(0, 0x0);
    OIIO_CHECK_NO_THROW(al->validate());
}

#endif
