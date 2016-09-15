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

#include <OpenColorIO/OpenColorIO.h>

#include "ClampOp.h"
#include "OpBuilders.h"


OCIO_NAMESPACE_ENTER
{
    ClampTransformRcPtr ClampTransform::Create()
    {
        return ClampTransformRcPtr(new ClampTransform(), &deleter);
    }
    
    void ClampTransform::deleter(ClampTransform* t)
    {
        delete t;
    }
    
    
    class ClampTransform::Impl
    {
    public:
        TransformDirection dir_;
        float min_[4], max_[4];
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD)
        {
            for(int i=0; i<4; ++i)
            {
                min_[i] = 1.0f;
                max_[i] = 1.0f;
            }
        }
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            dir_ = rhs.dir_;
            memcpy(min_, rhs.min_, 4*sizeof(float));
            memcpy(max_, rhs.max_, 4*sizeof(float));
            return *this;
        }
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    ClampTransform::ClampTransform()
        : m_impl(new ClampTransform::Impl)
    {
    }
    
    TransformRcPtr ClampTransform::createEditableCopy() const
    {
        ClampTransformRcPtr transform = ClampTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    ClampTransform::~ClampTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    ClampTransform& ClampTransform::operator= (const ClampTransform & rhs)
    {
        *m_impl = *rhs.m_impl;
        return *this;
    }
    
    TransformDirection ClampTransform::getDirection() const
    {
        return getImpl()->dir_;
    }
    
    void ClampTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }
    
    void ClampTransform::setMin(const float * vec4)
    {
        if(vec4) memcpy(getImpl()->min_, vec4, 4*sizeof(float));
    }
    
    void ClampTransform::getMin(float * vec4) const
    {
        if(vec4) memcpy(vec4, getImpl()->min_, 4*sizeof(float));
    }
    
    void ClampTransform::setMax(const float * vec4)
    {
        if(vec4) memcpy(getImpl()->max_, vec4, 4*sizeof(float));
    }
    
    void ClampTransform::getMax(float * vec4) const
    {
        if(vec4) memcpy(vec4, getImpl()->max_, 4*sizeof(float));
    }
    
    std::ostream& operator<< (std::ostream& os, const ClampTransform& t)
    {
        float min[4], max[4];

        t.getMin(min);
        t.getMax(max);

        os << "<ClampTransform";
        os << " direction=" << TransformDirectionToString(t.getDirection());
        os << ", min=" << min[0];
        for (int i = 1; i < 4; ++i)
        {
            os << " " << min[i];
        }
        os << ", max=" << max[0];
        for (int i = 1; i < 4; ++i)
        {
            os << " " << max[i];
        }

        os << ">";
        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    
    void BuildClampOps(OpRcPtrVec & ops,
                       const Config& /*config*/,
                       const ClampTransform & transform,
                       TransformDirection dir)
    {
        TransformDirection combinedDir = CombineTransformDirections(dir,
            transform.getDirection());
        
        float min[4], max[4];
        transform.getMin(min);
        transform.getMax(max);
        
        CreateClampOps(ops, min, max, combinedDir);
    }
}
OCIO_NAMESPACE_EXIT
