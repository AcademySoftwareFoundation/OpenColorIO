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

OCIO_NAMESPACE_ENTER
{
    LookRcPtr Look::Create()
    {
        return LookRcPtr(new Look(), &deleter);
    }
    
    void Look::deleter(Look* c)
    {
        delete c;
    }
    
    class Look::Impl
    {
    public:
        std::string name_;
        std::string processSpace_;
        std::string description_;
        TransformRcPtr transform_;
        TransformRcPtr inverseTransform_;
        
        Impl()
        { }

        Impl(const Impl &) = delete;
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                name_ = rhs.name_;
                processSpace_ = rhs.processSpace_;
                description_ = rhs.description_;

                transform_ = rhs.transform_?
                    rhs.transform_->createEditableCopy() : rhs.transform_;

                inverseTransform_ = rhs.inverseTransform_?
                    rhs.inverseTransform_->createEditableCopy()
                    : rhs.inverseTransform_;
            }
            return *this;
        }
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    Look::Look()
    : m_impl(new Look::Impl)
    {
    }
    
    Look::~Look()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    LookRcPtr Look::createEditableCopy() const
    {
        LookRcPtr cs = Look::Create();
        *cs->m_impl = *m_impl;
        return cs;
    }
    
    const char * Look::getName() const
    {
        return getImpl()->name_.c_str();
    }
    
    void Look::setName(const char * name)
    {
        getImpl()->name_ = name;
    }
    
    const char * Look::getProcessSpace() const
    {
        return getImpl()->processSpace_.c_str();
    }
    
    void Look::setProcessSpace(const char * processSpace)
    {
        getImpl()->processSpace_ = processSpace;
    }
    
    ConstTransformRcPtr Look::getTransform() const
    {
        return getImpl()->transform_;
    }
    
    void Look::setTransform(const ConstTransformRcPtr & transform)
    {
        getImpl()->transform_ = transform->createEditableCopy();
    }
    
    ConstTransformRcPtr Look::getInverseTransform() const
    {
        return getImpl()->inverseTransform_;
    }
    
    void Look::setInverseTransform(const ConstTransformRcPtr & transform)
    {
        getImpl()->inverseTransform_ = transform->createEditableCopy();
    }
    
    const char * Look::getDescription() const
    {
        return getImpl()->description_.c_str();
    }

    void Look::setDescription(const char * description)
    {
        getImpl()->description_ = description;
    }

    std::ostream& operator<< (std::ostream& os, const Look& look)
    {
        os << "<Look";
        os << " name=" << look.getName();
        os << ", processSpace=" << look.getProcessSpace();
        
        if(look.getTransform())
        {
            os << ",\n    transform=";
            os << "\n\t" << *look.getTransform();
        }
        
        if(look.getInverseTransform())
        {
            os << ",\n    inverseTransform=";
            os << "\n\t" << *look.getInverseTransform();
        }
        
        os << ">";
        
        return os;
    }
}
OCIO_NAMESPACE_EXIT
