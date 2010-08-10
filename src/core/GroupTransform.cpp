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
#include "GroupTransform.h"

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
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    GroupTransform::GroupTransform()
        : m_impl(new GroupTransform::Impl)
    {
    }
    
    /*
    GroupTransform::GroupTransform(const GroupTransform & rhs)
        : 
        Transform(),
        m_impl(new GroupTransform::Impl)
    {
        *m_impl = *rhs.m_impl;
    }
    */
    
    TransformRcPtr GroupTransform::createEditableCopy() const
    {
        GroupTransformRcPtr transform = GroupTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    GroupTransform::~GroupTransform()
    {
    }
    
    GroupTransform& GroupTransform::operator= (const GroupTransform & rhs)
    {
        *m_impl = *rhs.m_impl;
        return *this;
    }
    
    TransformDirection GroupTransform::getDirection() const
    {
        return m_impl->getDirection();
    }
    
    void GroupTransform::setDirection(TransformDirection dir)
    {
        m_impl->setDirection(dir);
    }
    
    int GroupTransform::size() const
    {
        return m_impl->size();
    }
    
    ConstTransformRcPtr GroupTransform::getTransform(int index) const
    {
        return m_impl->getTransform(index);
    }
    
    TransformRcPtr GroupTransform::getEditableTransform(int index)
    {
        return m_impl->getEditableTransform(index);
    }
    
    void GroupTransform::push_back(const ConstTransformRcPtr& transform)
    {
        m_impl->push_back(transform);
    }
    /*
    void GroupTransform::push_back(TransformRcPtr transform)
    {
        m_impl->push_back(transform);
    }
    */
    
    void GroupTransform::clear()
    {
        m_impl->clear();
    }
    
    bool GroupTransform::empty() const
    {
        return m_impl->empty();
    }
        
    
    std::ostream& operator<< (std::ostream& os, const GroupTransform& groupTransform)
    {
        for(int i=0; i<groupTransform.size(); ++i)
        {
            if(i!=groupTransform.size()-1) os << "\n";
            ConstTransformRcPtr transform = groupTransform.getTransform(i);
            os << "\t" << *transform;
        }
        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    GroupTransform::Impl::Impl() :
        m_direction(TRANSFORM_DIR_FORWARD)
    { }
    
    /*
    GroupTransform::Impl::Impl(const Impl & rhs) :
        m_direction(rhs.m_direction)
    {
        for(unsigned int i=0; i<rhs.m_vec.size(); ++i)
        {
            m_vec.push_back(rhs.m_vec[i]->clone());
        }
    }
    */
    
    GroupTransform::Impl::~Impl()
    {
        clear();
    }
    
    GroupTransform::Impl& GroupTransform::Impl::operator= (const Impl & rhs)
    {
        m_direction = rhs.m_direction;
        
        clear();
        
        for(unsigned int i=0; i<rhs.m_vec.size(); ++i)
        {
            m_vec.push_back(rhs.m_vec[i]->createEditableCopy());
        }
        
        return *this;
    }
    
    TransformDirection GroupTransform::Impl::getDirection() const
    {
        return m_direction;
    }
    
    void GroupTransform::Impl::setDirection(TransformDirection dir)
    {
        m_direction = dir;
    }
    
    int GroupTransform::Impl::size() const
    {
        return static_cast<int>(m_vec.size());
    }

    ConstTransformRcPtr GroupTransform::Impl::getTransform(int index) const
    {
        if(index < 0 || index >= (int)m_vec.size())
        {
            std::ostringstream os;
            os << "Invalid transform index " << index << ".";
            throw Exception(os.str().c_str());
        }
        
        return m_vec[index];
    }
    
    TransformRcPtr GroupTransform::Impl::getEditableTransform(int index)
    {
        if(index < 0 || index >= (int)m_vec.size())
        {
            std::ostringstream os;
            os << "Invalid transform index " << index << ".";
            throw Exception(os.str().c_str());
        }
        
        return m_vec[index];
    }

    void GroupTransform::Impl::push_back(const ConstTransformRcPtr& transform)
    {
        m_vec.push_back(transform->createEditableCopy());
    }
    /*
    void GroupTransform::Impl::push_back(TransformRcPtr transform)
    {
        m_vec.push_back(transform);
    }
    */
    void GroupTransform::Impl::clear()
    {
        m_vec.clear();
    }
    
    bool GroupTransform::Impl::empty() const
    {
        return m_vec.empty();
    }
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    void BuildGroupOps(LocalProcessor & processor,
                       const Config& config,
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
                BuildOps(processor, config, childTransform, TRANSFORM_DIR_FORWARD);
            }
        }
        else if(combinedDir == TRANSFORM_DIR_INVERSE)
        {
            for(int i=groupTransform.size()-1; i>=0; --i)
            {
                ConstTransformRcPtr childTransform = groupTransform.getTransform(i);
                BuildOps(processor, config, childTransform, TRANSFORM_DIR_INVERSE);
            }
        }
    }

}
OCIO_NAMESPACE_EXIT
