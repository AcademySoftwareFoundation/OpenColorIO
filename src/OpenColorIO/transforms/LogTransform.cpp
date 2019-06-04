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

#include "ops/Log/LogOpData.h"
#include "ops/Log/LogOps.h"
#include "ops/Log/LogUtils.h"
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
    
    
    class LogTransform::Impl : public LogOpData
    {
    public:
        
        Impl()
            : LogOpData(2.0f, TRANSFORM_DIR_FORWARD)
        { }

        Impl(const Impl &) = delete;

        ~Impl()
        { }

        Impl& operator = (const Impl & rhs)
        {
            if (this != &rhs)
            {
                LogOpData::operator=(rhs);
            }
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
        if (this != &rhs)
        {
            *m_impl = *rhs.m_impl;
        }
        return *this;
    }
    
    TransformDirection LogTransform::getDirection() const
    {
        return getImpl()->getDirection();
    }
    
    void LogTransform::setDirection(TransformDirection dir)
    {
        getImpl()->setDirection(dir);
    }
    
    void LogTransform::validate() const
    {
        try
        {
            Transform::validate();
            getImpl()->validate();
        }
        catch (Exception & ex)
        {
            std::string errMsg("LogTransform validation failed: ");
            errMsg += ex.what();
            throw Exception(errMsg.c_str());
        }
    }

    double LogTransform::getBase() const
    {
        return getImpl()->getBase();
    }
    
    void LogTransform::setBase(double val)
    {
        getImpl()->setBase(val);
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
        TransformDirection combinedDir =
            CombineTransformDirections(dir,
                                       transform.getDirection());
        CreateLogOp(ops, transform.getBase(), combinedDir);
    }
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(LogTransform, basic)
{
    const OCIO::LogTransformRcPtr log = OCIO::LogTransform::Create();

    OIIO_CHECK_EQUAL(log->getBase(), 2.0f);
    OIIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    log->setBase(10.0f);
    OIIO_CHECK_EQUAL(log->getBase(), 10.0f);
}

#endif