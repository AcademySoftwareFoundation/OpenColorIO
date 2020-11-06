// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/allocation/AllocationOp.h"
#include "OpBuilders.h"

namespace OCIO_NAMESPACE
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
    TransformDirection m_dir;
    Allocation m_allocation;
    std::vector<float> m_vars;

    Impl() :
        m_dir(TRANSFORM_DIR_FORWARD),
        m_allocation(ALLOCATION_UNIFORM)
    { }

    Impl(const Impl &) = delete;

    ~Impl()
    { }

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_dir = rhs.m_dir;
            m_allocation = rhs.m_allocation;
            m_vars = rhs.m_vars;
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
    m_impl = nullptr;
}

TransformDirection AllocationTransform::getDirection() const  noexcept
{
    return getImpl()->m_dir;
}

void AllocationTransform::setDirection(TransformDirection dir) noexcept
{
    getImpl()->m_dir = dir;
}

void AllocationTransform::validate() const
{
    try
    {
        Transform::validate();
    }
    catch (Exception & ex)
    {
        std::string errMsg("AllocationTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }

    if (getImpl()->m_allocation == ALLOCATION_UNIFORM)
    {
        if(getImpl()->m_vars.size()!=2 && getImpl()->m_vars.size()!=0)
        {
            throw Exception("AllocationTransform: wrong number of values for the uniform allocation");
        }
    }
    else if (getImpl()->m_allocation == ALLOCATION_LG2)
    {
        if(getImpl()->m_vars.size()!=3 && getImpl()->m_vars.size()!=2 && getImpl()->m_vars.size()!=0)
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
    return getImpl()->m_allocation;
}

void AllocationTransform::setAllocation(Allocation allocation)
{
    getImpl()->m_allocation = allocation;
}

int AllocationTransform::getNumVars() const
{
    return static_cast<int>(getImpl()->m_vars.size());
}

void AllocationTransform::getVars(float * vars) const
{
    if(!getImpl()->m_vars.empty())
    {
        memcpy(vars,
            &getImpl()->m_vars[0],
            getImpl()->m_vars.size()*sizeof(float));
    }
}

void AllocationTransform::setVars(int numvars, const float * vars)
{
    getImpl()->m_vars.resize(numvars);

    if(!getImpl()->m_vars.empty())
    {
        memcpy(&getImpl()->m_vars[0],
            vars,
            numvars*sizeof(float));
    }
}

std::ostream& operator<< (std::ostream& os, const AllocationTransform& t)
{
    Allocation allocation(t.getAllocation());
    const int numVars(t.getNumVars());
    std::vector<float> vars(numVars);
    if (numVars > 0)
    {
        t.getVars(&vars[0]);
    }

    os << "<AllocationTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection());
    if (numVars > 0)
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


void BuildAllocationOp(OpRcPtrVec & ops,
                       const AllocationTransform & allocationTransform,
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
} // namespace OCIO_NAMESPACE

