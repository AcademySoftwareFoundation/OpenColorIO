// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"
#include "ops/Gamma/GammaOpData.h"


OCIO_NAMESPACE_ENTER
{
ExponentTransformRcPtr ExponentTransform::Create()
{
    return ExponentTransformRcPtr(new ExponentTransform(), &deleter);
}

void ExponentTransform::deleter(ExponentTransform* t)
{
    delete t;
}


class ExponentTransform::Impl : public GammaOpData
{
public:
    TransformDirection m_dir;
    
    Impl()
        :   GammaOpData()
        ,   m_dir(TRANSFORM_DIR_FORWARD)
    {
        setRedParams  ( {1.} );
        setGreenParams( {1.} );
        setBlueParams ( {1.} );
        setAlphaParams( {1.} );
    }

    Impl(const Impl &) = delete;

    ~Impl()
    { }
    
    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            GammaOpData::operator=(rhs);
            m_dir = rhs.m_dir;
        }
        return *this;
    }
};

///////////////////////////////////////////////////////////////////////////



ExponentTransform::ExponentTransform()
    : m_impl(new ExponentTransform::Impl)
{
}

TransformRcPtr ExponentTransform::createEditableCopy() const
{
    ExponentTransformRcPtr transform = ExponentTransform::Create();
    *(transform->m_impl) = *m_impl;
    return transform;
}

ExponentTransform::~ExponentTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

ExponentTransform& ExponentTransform::operator= (const ExponentTransform & rhs)
{
    if (this != &rhs)
    {
        *m_impl = *rhs.m_impl;
    }
    return *this;
}

TransformDirection ExponentTransform::getDirection() const
{
    return getImpl()->m_dir;
}

void ExponentTransform::setDirection(TransformDirection dir)
{
    getImpl()->m_dir = dir;
}

void ExponentTransform::validate() const
{
    try
    {
        Transform::validate();
        getImpl()->validate();
    }
    catch(Exception & ex)
    {
        std::string errMsg("ExponentTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & ExponentTransform::getFormatMetadata()
{
    return m_impl->getFormatMetadata();
}

const FormatMetadata & ExponentTransform::getFormatMetadata() const
{
    return m_impl->getFormatMetadata();
}

void ExponentTransform::setValue(const float * vec4)
{
    if(vec4)
    {
        getImpl()->getRedParams()  [0] = vec4[0];
        getImpl()->getGreenParams()[0] = vec4[1];
        getImpl()->getBlueParams() [0] = vec4[2];
        getImpl()->getAlphaParams()[0] = vec4[3];
    }
}

void ExponentTransform::setValue(const double(&vec4)[4])
{
    getImpl()->getRedParams()  [0] = vec4[0];
    getImpl()->getGreenParams()[0] = vec4[1];
    getImpl()->getBlueParams() [0] = vec4[2];
    getImpl()->getAlphaParams()[0] = vec4[3];
}

void ExponentTransform::getValue(float * vec4) const
{
    if(vec4)
    {
        vec4[0] = (float)getImpl()->getRedParams()  [0];
        vec4[1] = (float)getImpl()->getGreenParams()[0];
        vec4[2] = (float)getImpl()->getBlueParams() [0];
        vec4[3] = (float)getImpl()->getAlphaParams()[0];
    }
}

void ExponentTransform::getValue(double(&vec4)[4]) const
{
    vec4[0] = getImpl()->getRedParams()  [0];
    vec4[1] = getImpl()->getGreenParams()[0];
    vec4[2] = getImpl()->getBlueParams() [0];
    vec4[3] = getImpl()->getAlphaParams()[0];
}

std::ostream& operator<< (std::ostream& os, const ExponentTransform & t)
{
    float value[4];
    t.getValue(value);

    os << "<ExponentTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
    
    os << "value=" << value[0];
    for (int i = 1; i < 4; ++i)
    {
      os << " " << value[i];
    }
    
    os << ">";
    return os;
}


}
OCIO_NAMESPACE_EXIT


////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"


OCIO_ADD_TEST(ExponentTransform, basic)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();
    OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    exp->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    std::vector<float> val4(4, 1.), identity_val4(4, 1.);
    OCIO_CHECK_NO_THROW(exp->getValue(&val4[0]));
    OCIO_CHECK_ASSERT(val4 == identity_val4);

    val4[1] = 2.;
    OCIO_CHECK_NO_THROW(exp->setValue(&val4[0]));
    OCIO_CHECK_NO_THROW(exp->getValue(&val4[0]));
    OCIO_CHECK_ASSERT(val4 != identity_val4);
}

namespace
{

void CheckValues(const double(&v1)[4], const double(&v2)[4])
{
    static const float errThreshold = 1e-8f;

    OCIO_CHECK_CLOSE(v1[0], v2[0], errThreshold);
    OCIO_CHECK_CLOSE(v1[1], v2[1], errThreshold);
    OCIO_CHECK_CLOSE(v1[2], v2[2], errThreshold);
    OCIO_CHECK_CLOSE(v1[3], v2[3], errThreshold);
}

};

OCIO_ADD_TEST(ExponentTransform, double)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();
    OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    double val4[4] = { -1., -2., -3., -4. };
    OCIO_CHECK_NO_THROW(exp->getValue(val4));
    CheckValues(val4, { 1., 1., 1., 1. });

    val4[1] = 2.1234567;
    OCIO_CHECK_NO_THROW(exp->setValue(val4));
    val4[1] = -2.;
    OCIO_CHECK_NO_THROW(exp->getValue(val4));
    CheckValues(val4, {1., 2.1234567, 1., 1.});
}

#endif
