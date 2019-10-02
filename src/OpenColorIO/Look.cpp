// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
