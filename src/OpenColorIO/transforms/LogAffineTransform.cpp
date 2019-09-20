// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Log/LogOpData.h"

OCIO_NAMESPACE_ENTER
{
    LogAffineTransformRcPtr LogAffineTransform::Create()
    {
        return LogAffineTransformRcPtr(new LogAffineTransform(), &deleter);
    }
    
    void LogAffineTransform::deleter(LogAffineTransform* t)
    {
        delete t;
    }
    
    
    class LogAffineTransform::Impl : public LogOpData
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
    
    
    LogAffineTransform::LogAffineTransform()
        : m_impl(new LogAffineTransform::Impl)
    {
    }
    
    TransformRcPtr LogAffineTransform::createEditableCopy() const
    {
        LogAffineTransformRcPtr transform = LogAffineTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    LogAffineTransform::~LogAffineTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    LogAffineTransform& LogAffineTransform::operator= (const LogAffineTransform & rhs)
    {
        if (this != &rhs)
        {
            *m_impl = *rhs.m_impl;
        }
        return *this;
    }
    
    TransformDirection LogAffineTransform::getDirection() const
    {
        return getImpl()->getDirection();
    }
    
    void LogAffineTransform::setDirection(TransformDirection dir)
    {
        getImpl()->setDirection(dir);
    }
    
    void LogAffineTransform::validate() const
    {
        try
        {
            Transform::validate();
            getImpl()->validate();
        }
        catch (Exception & ex)
        {
            std::string errMsg("LogAffineTransform validation failed: ");
            errMsg += ex.what();
            throw Exception(errMsg.c_str());
        }
    }

    FormatMetadata & LogAffineTransform::getFormatMetadata()
    {
        return m_impl->getFormatMetadata();
    }

    const FormatMetadata & LogAffineTransform::getFormatMetadata() const
    {
        return m_impl->getFormatMetadata();
    }

    void LogAffineTransform::setBase(double base)
    {
        getImpl()->setBase(base);
    }

    double LogAffineTransform::getBase() const
    {
        return getImpl()->getBase();
    }

    void LogAffineTransform::setLogSideSlopeValue(const double(&values)[3])
    {
        getImpl()->setValue(LOG_SIDE_SLOPE, values);
    }
    void LogAffineTransform::setLogSideOffsetValue(const double(&values)[3])
    {
        getImpl()->setValue(LOG_SIDE_OFFSET, values);
    }
    void LogAffineTransform::setLinSideSlopeValue(const double(&values)[3])
    {
        getImpl()->setValue(LIN_SIDE_SLOPE, values);
    }
    void LogAffineTransform::setLinSideOffsetValue(const double(&values)[3])
    {
        getImpl()->setValue(LIN_SIDE_OFFSET, values);
    }

    void LogAffineTransform::getLogSideSlopeValue(double(&values)[3]) const
    {
        getImpl()->getValue(LOG_SIDE_SLOPE, values);
    }
    void LogAffineTransform::getLogSideOffsetValue(double(&values)[3]) const
    {
        getImpl()->getValue(LOG_SIDE_OFFSET, values);
    }
    void LogAffineTransform::getLinSideSlopeValue(double(&values)[3]) const
    {
        getImpl()->getValue(LIN_SIDE_SLOPE, values);
    }
    void LogAffineTransform::getLinSideOffsetValue(double(&values)[3]) const
    {
        getImpl()->getValue(LIN_SIDE_OFFSET, values);
    }

    std::ostream& operator<< (std::ostream& os, const LogAffineTransform& t)
    {
        os << "<LogAffineTransform ";
        os << "base=" << t.getBase() << ", ";
        double values[3];
        t.getLogSideSlopeValue(values);
        os << "logSideSlope=" << values[0] << " " << values[1] << " " << values[2] << ", ";
        t.getLogSideOffsetValue(values);
        os << "logSideOffset=" << values[0] << " " << values[1] << " " << values[2] << ", ";
        t.getLinSideSlopeValue(values);
        os << "linSideSlope=" << values[0] << " " << values[1] << " " << values[2] << ", ";
        t.getLinSideOffsetValue(values);
        os << "linSideOffset=" << values[0] << " " << values[1] << " " << values[2] << ", ";
        os << "direction=" << TransformDirectionToString(t.getDirection());
        os << ">";

        return os;
    }
    
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

bool AllEqual(double (&values)[3])
{
    return values[0] == values[1] && values[0] == values[2];
}

OCIO_ADD_TEST(LogAffineTransform, basic)
{
    const OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();

    const double base = log->getBase();
    OCIO_CHECK_EQUAL(base, 2.0);
    double values[3];
    log->getLinSideOffsetValue(values);
    OCIO_CHECK_ASSERT(AllEqual(values));
    OCIO_CHECK_EQUAL(values[0], 0.0);
    log->getLinSideSlopeValue(values);
    OCIO_CHECK_ASSERT(AllEqual(values));
    OCIO_CHECK_EQUAL(values[0], 1.0);
    log->getLogSideOffsetValue(values);
    OCIO_CHECK_ASSERT(AllEqual(values));
    OCIO_CHECK_EQUAL(values[0], 0.0);
    log->getLogSideSlopeValue(values);
    OCIO_CHECK_ASSERT(AllEqual(values));
    OCIO_CHECK_EQUAL(values[0], 1.0);
    OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    log->setBase(3.0);
    log->getBase();
    OCIO_CHECK_EQUAL(log->getBase(), 3.0);

    log->setLinSideOffsetValue({ 0.1, 0.2, 0.3 });
    log->getLinSideOffsetValue(values);
    OCIO_CHECK_EQUAL(values[0], 0.1);
    OCIO_CHECK_EQUAL(values[1], 0.2);
    OCIO_CHECK_EQUAL(values[2], 0.3);

    log->setLinSideSlopeValue({ 1.1, 1.2, 1.3 });
    log->getLinSideSlopeValue(values);
    OCIO_CHECK_EQUAL(values[0], 1.1);
    OCIO_CHECK_EQUAL(values[1], 1.2);
    OCIO_CHECK_EQUAL(values[2], 1.3);

    log->setLogSideOffsetValue({ 0.1, 0.2, 0.3 });
    log->getLogSideOffsetValue(values);
    OCIO_CHECK_EQUAL(values[0], 0.1);
    OCIO_CHECK_EQUAL(values[1], 0.2);
    OCIO_CHECK_EQUAL(values[2], 0.3);

    log->setLogSideSlopeValue({ 1.1, 1.2, 1.3 });
    log->getLogSideSlopeValue(values);
    OCIO_CHECK_EQUAL(values[0], 1.1);
    OCIO_CHECK_EQUAL(values[1], 1.2);
    OCIO_CHECK_EQUAL(values[2], 1.3);
}

#endif