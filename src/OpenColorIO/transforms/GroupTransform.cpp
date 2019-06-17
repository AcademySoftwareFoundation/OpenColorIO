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
#include "OpBuilders.h"

#include <sstream>

OCIO_NAMESPACE_ENTER
{
    GroupTransformRcPtr GroupTransform::Create()
    {
        return GroupTransformRcPtr(new GroupTransform(), &deleter);
    }
    
    void GroupTransform::deleter(GroupTransform* t)
    {
        delete t;
    }
    
    namespace
    {
        typedef std::vector<TransformRcPtr> TransformRcPtrVec;
    }
    
    class GroupTransform::Impl
    {
    public:
        TransformDirection dir_;
        TransformRcPtrVec vec_;
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD)
        { }

        Impl(const Impl &) = delete;

        ~Impl()
        {
            vec_.clear();
        }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                dir_ = rhs.dir_;

                vec_.clear();

                for (unsigned int i = 0; i < rhs.vec_.size(); ++i)
                {
                    vec_.push_back(rhs.vec_[i]->createEditableCopy());
                }
            }
            return *this;
        }
    };
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    GroupTransform::GroupTransform()
        : m_impl(new GroupTransform::Impl)
    {
    }
    
    TransformRcPtr GroupTransform::createEditableCopy() const
    {
        GroupTransformRcPtr transform = GroupTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    GroupTransform::~GroupTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    GroupTransform& GroupTransform::operator= (const GroupTransform & rhs)
    {
        if(this!=&rhs)
        {
            *m_impl = *rhs.m_impl;
        }
        return *this;
    }
    
    TransformDirection GroupTransform::getDirection() const
    {
        return getImpl()->dir_;
    }
    
    void GroupTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }
    
    void GroupTransform::validate() const
    {
        Transform::validate();

        for (int i = 0; i<size(); ++i)
        {
            getTransform(i)->validate();
        }
    }

    int GroupTransform::size() const
    {
        return static_cast<int>(getImpl()->vec_.size());
    }
    
    ConstTransformRcPtr GroupTransform::getTransform(int index) const
    {
        if(index < 0 || index >= (int)getImpl()->vec_.size())
        {
            std::ostringstream os;
            os << "Invalid transform index " << index << ".";
            throw Exception(os.str().c_str());
        }
        
        return getImpl()->vec_[index];
    }
    
    void GroupTransform::push_back(const ConstTransformRcPtr& transform)
    {
        getImpl()->vec_.push_back(transform->createEditableCopy());
    }
    
    void GroupTransform::clear()
    {
        getImpl()->vec_.clear();
    }
    
    bool GroupTransform::empty() const
    {
        return getImpl()->vec_.empty();
    }
    
    std::ostream& operator<< (std::ostream& os, const GroupTransform& groupTransform)
    {
        os << "<GroupTransform ";
        os << "direction=" << TransformDirectionToString(groupTransform.getDirection()) << ", ";
        os << "transforms=";
        for(int i=0; i<groupTransform.size(); ++i)
        {
            ConstTransformRcPtr transform = groupTransform.getTransform(i);
            os << "\n\t" << *transform;
        }
        os << ">";
        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    void BuildGroupOps(OpRcPtrVec & ops,
                       const Config& config,
                       const ConstContextRcPtr & context,
                       const GroupTransform& groupTransform,
                       TransformDirection dir)
    {
        TransformDirection combinedDir = CombineTransformDirections(dir,
                                                  groupTransform.getDirection());
        
        if(combinedDir == TRANSFORM_DIR_FORWARD)
        {
            for(int i=0; i<groupTransform.size(); ++i)
            {
                ConstTransformRcPtr childTransform = groupTransform.getTransform(i);
                BuildOps(ops, config, context, childTransform, TRANSFORM_DIR_FORWARD);
            }
        }
        else if(combinedDir == TRANSFORM_DIR_INVERSE)
        {
            for(int i=groupTransform.size()-1; i>=0; --i)
            {
                ConstTransformRcPtr childTransform = groupTransform.getTransform(i);
                BuildOps(ops, config, context, childTransform, TRANSFORM_DIR_INVERSE);
            }
        }
    }

}
OCIO_NAMESPACE_EXIT
