/*
Copyright (c) 2019 Autodesk Inc., et al.
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

    private:        
        Impl(const Impl & rhs);
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

    void LogAffineTransform::setBase(double base)
    {
        getImpl()->setBase(base);
    }

    double LogAffineTransform::getBase() const
    {
        return getImpl()->getBase();
    }

    void LogAffineTransform::setValue(LogAffineParameter val, const double(&values)[3])
    {
        getImpl()->setValue(val, values);
    }

    void LogAffineTransform::getValue(LogAffineParameter val, double(&values)[3]) const
    {
        getImpl()->getValue(val, values);
    }

    std::ostream& operator<< (std::ostream& os, const LogAffineTransform& t)
    {
        os << "<LogAffineTransform ";
        os << "base=" << t.getBase() << ", ";
        double values[3];
        t.getValue(LOG_SIDE_SLOPE, values);
        os << "logSideSlope=" << values[0] << " " << values[1] << " " << values[2] << ", ";
        t.getValue(LOG_SIDE_OFFSET, values);
        os << "logSideOffset=" << values[0] << " " << values[1] << " " << values[2] << ", ";
        t.getValue(LIN_SIDE_SLOPE, values);
        os << "linSideSlope=" << values[0] << " " << values[1] << " " << values[2] << ", ";
        t.getValue(LIN_SIDE_OFFSET, values);
        os << "linSideOffset=" << values[0] << " " << values[1] << " " << values[2] << ", ";
        os << "direction=" << TransformDirectionToString(t.getDirection());
        os << ">";

        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    void BuildLogOps(OpRcPtrVec & ops,
                     const Config& /*config*/,
                     const LogAffineTransform& transform,
                     TransformDirection dir)
    {
        TransformDirection combinedDir =
            CombineTransformDirections(dir,
                                       transform.getDirection());

        double base = transform.getBase();
        double logSlope[3] = { 1.0, 1.0, 1.0 };
        double linSlope[3] = { 1.0, 1.0, 1.0 };
        double linOffset[3] = { 0.0, 0.0, 0.0 };
        double logOffset[3] = { 0.0, 0.0, 0.0 };
        
        transform.getValue(LOG_SIDE_SLOPE, logSlope);
        transform.getValue(LOG_SIDE_OFFSET, logOffset);
        transform.getValue(LIN_SIDE_SLOPE, linSlope);
        transform.getValue(LIN_SIDE_OFFSET, linOffset);
        

        CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset, combinedDir);
    }
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

bool AllEqual(double (&values)[3])
{
    return values[0] == values[1] && values[0] == values[2];
}

OIIO_ADD_TEST(LogAffineTransform, basic)
{
    const OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();

    const double base = log->getBase();
    OIIO_CHECK_EQUAL(base, 2.0);
    double values[3];
    log->getValue(OCIO::LIN_SIDE_OFFSET, values);
    OIIO_CHECK_ASSERT(AllEqual(values));
    OIIO_CHECK_EQUAL(values[0], 0.0);
    log->getValue(OCIO::LIN_SIDE_SLOPE, values);
    OIIO_CHECK_ASSERT(AllEqual(values));
    OIIO_CHECK_EQUAL(values[0], 1.0);
    log->getValue(OCIO::LOG_SIDE_OFFSET, values);
    OIIO_CHECK_ASSERT(AllEqual(values));
    OIIO_CHECK_EQUAL(values[0], 0.0);
    log->getValue(OCIO::LOG_SIDE_SLOPE, values);
    OIIO_CHECK_ASSERT(AllEqual(values));
    OIIO_CHECK_EQUAL(values[0], 1.0);
    OIIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    log->setBase(3.0);
    log->getBase();
    OIIO_CHECK_EQUAL(log->getBase(), 3.0);

    log->setValue(OCIO::LIN_SIDE_OFFSET, { 0.1, 0.2, 0.3 });
    log->getValue(OCIO::LIN_SIDE_OFFSET, values);
    OIIO_CHECK_EQUAL(values[0], 0.1);
    OIIO_CHECK_EQUAL(values[1], 0.2);
    OIIO_CHECK_EQUAL(values[2], 0.3);

    log->setValue(OCIO::LIN_SIDE_SLOPE, { 1.1, 1.2, 1.3 });
    log->getValue(OCIO::LIN_SIDE_SLOPE, values);
    OIIO_CHECK_EQUAL(values[0], 1.1);
    OIIO_CHECK_EQUAL(values[1], 1.2);
    OIIO_CHECK_EQUAL(values[2], 1.3);

    log->setValue(OCIO::LOG_SIDE_OFFSET, { 0.1, 0.2, 0.3 });
    log->getValue(OCIO::LOG_SIDE_OFFSET, values);
    OIIO_CHECK_EQUAL(values[0], 0.1);
    OIIO_CHECK_EQUAL(values[1], 0.2);
    OIIO_CHECK_EQUAL(values[2], 0.3);

    log->setValue(OCIO::LOG_SIDE_SLOPE, { 1.1, 1.2, 1.3 });
    log->getValue(OCIO::LOG_SIDE_SLOPE, values);
    OIIO_CHECK_EQUAL(values[0], 1.1);
    OIIO_CHECK_EQUAL(values[1], 1.2);
    OIIO_CHECK_EQUAL(values[2], 1.3);
}

#endif