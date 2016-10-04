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

#include "LogOps.h"
#include "OpBuilders.h"

OCIO_NAMESPACE_ENTER
{
    LogTransformRcPtr LogTransform::Create()
    {
        return LogTransformRcPtr(new LogTransform(), &deleter);
    }
    
    void LogTransform::deleter(LogTransform* t)
    {
        delete t;
    }
    
    
    class LogTransform::Impl
    {
    public:
        TransformDirection dir_;
        float base_;
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD),
            base_(2.0)
        { }
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            dir_ = rhs.dir_;
            base_ = rhs.base_;
            return *this;
        }
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    LogTransform::LogTransform()
        : m_impl(new LogTransform::Impl)
    {
    }
    
    TransformRcPtr LogTransform::createEditableCopy() const
    {
        LogTransformRcPtr transform = LogTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    LogTransform::~LogTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    LogTransform& LogTransform::operator= (const LogTransform & rhs)
    {
        *m_impl = *rhs.m_impl;
        return *this;
    }
    
    TransformDirection LogTransform::getDirection() const
    {
        return getImpl()->dir_;
    }
    
    void LogTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }
    
    
    float LogTransform::getBase() const
    {
        return getImpl()->base_;
    }
    
    void LogTransform::setBase(float val)
    {
        getImpl()->base_ = val;
    }
    
    std::ostream& operator<< (std::ostream& os, const LogTransform& t)
    {
        os << "<LogTransform ";
        os << "base=" << t.getBase() << ", ";
        os << "direction=" << TransformDirectionToString(t.getDirection());
        os << ">";
        
        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    void BuildLogOps(OpRcPtrVec & ops,
                     const Config& /*config*/,
                     const LogTransform& transform,
                     TransformDirection dir)
    {
        TransformDirection combinedDir = CombineTransformDirections(dir,
                                                  transform.getDirection());
        
        float basescalar = transform.getBase();
        float base[3] = { basescalar, basescalar, basescalar };
        
        float k[3] = { 1.0f, 1.0f, 1.0f };
        float m[3] = { 1.0f, 1.0f, 1.0f };
        float b[3] = { 0.0f, 0.0f, 0.0f };
        float kb[3] = { 0.0f, 0.0f, 0.0f };
        
        // output = k * log(mx+b, base) + kb
        CreateLogOp(ops,
                    k, m, b, base, kb,
                    combinedDir);
    }
}
OCIO_NAMESPACE_EXIT
