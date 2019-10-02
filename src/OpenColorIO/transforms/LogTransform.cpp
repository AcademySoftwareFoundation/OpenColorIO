// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Log/LogOpData.h"

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

    FormatMetadata & LogTransform::getFormatMetadata()
    {
        return m_impl->getFormatMetadata();
    }

    const FormatMetadata & LogTransform::getFormatMetadata() const
    {
        return m_impl->getFormatMetadata();
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
    
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OCIO_ADD_TEST(LogTransform, basic)
{
    const OCIO::LogTransformRcPtr log = OCIO::LogTransform::Create();

    OCIO_CHECK_EQUAL(log->getBase(), 2.0f);
    OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    log->setBase(10.0f);
    OCIO_CHECK_EQUAL(log->getBase(), 10.0f);
}

#endif