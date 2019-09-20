// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"
#include "ops/Gamma/GammaOpData.h"


OCIO_NAMESPACE_ENTER
{

ExponentWithLinearTransformRcPtr ExponentWithLinearTransform::Create()
{
    return ExponentWithLinearTransformRcPtr(new ExponentWithLinearTransform(), &deleter);
}

void ExponentWithLinearTransform::deleter(ExponentWithLinearTransform* t)
{
    delete t;
}


class ExponentWithLinearTransform::Impl : public GammaOpData
{
public:
    TransformDirection m_dir;
    
    Impl()
        :   GammaOpData()
        ,   m_dir(TRANSFORM_DIR_FORWARD)
    {
        setRedParams  ( {1., 0.} );
        setGreenParams( {1., 0.} );
        setBlueParams ( {1., 0.} );
        setAlphaParams( {1., 0.} );

        setStyle(GammaOpData::MONCURVE_FWD);
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



ExponentWithLinearTransform::ExponentWithLinearTransform()
    : m_impl(new ExponentWithLinearTransform::Impl)
{
}

TransformRcPtr ExponentWithLinearTransform::createEditableCopy() const
{
    ExponentWithLinearTransformRcPtr transform = ExponentWithLinearTransform::Create();
    *(transform->m_impl) = *m_impl;
    return transform;
}

ExponentWithLinearTransform::~ExponentWithLinearTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

ExponentWithLinearTransform& ExponentWithLinearTransform::operator= (const ExponentWithLinearTransform & rhs)
{
    if (this != &rhs)
    {
        *m_impl = *rhs.m_impl;
    }
    return *this;
}

TransformDirection ExponentWithLinearTransform::getDirection() const
{
    return getImpl()->m_dir;
}

void ExponentWithLinearTransform::setDirection(TransformDirection dir)
{
    getImpl()->m_dir = dir;
}

void ExponentWithLinearTransform::validate() const
{
    try
    {
        Transform::validate();
        getImpl()->validate();
    }
    catch(Exception & ex)
    {
        std::string errMsg("ExponentWithLinearTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & ExponentWithLinearTransform::getFormatMetadata()
{
    return m_impl->getFormatMetadata();
}

const FormatMetadata & ExponentWithLinearTransform::getFormatMetadata() const
{
    return m_impl->getFormatMetadata();
}

void ExponentWithLinearTransform::setGamma(const double(&values)[4])
{
    getImpl()->getRedParams()  [0] = values[0];
    getImpl()->getGreenParams()[0] = values[1];
    getImpl()->getBlueParams() [0] = values[2];
    getImpl()->getAlphaParams()[0] = values[3];
}

void ExponentWithLinearTransform::getGamma(double(&values)[4]) const
{
    values[0] = getImpl()->getRedParams()  [0];
    values[1] = getImpl()->getGreenParams()[0];
    values[2] = getImpl()->getBlueParams() [0];
    values[3] = getImpl()->getAlphaParams()[0];
}

void ExponentWithLinearTransform::setOffset(const double(&values)[4])
{
    const GammaOpData::Params red = { getImpl()->getRedParams()  [0], values[0] };
    const GammaOpData::Params grn = { getImpl()->getGreenParams()[0], values[1] };
    const GammaOpData::Params blu = { getImpl()->getBlueParams() [0], values[2] };
    const GammaOpData::Params alp = { getImpl()->getAlphaParams()[0], values[3] };

    getImpl()->setRedParams(red);
    getImpl()->setGreenParams(grn);
    getImpl()->setBlueParams(blu);
    getImpl()->setAlphaParams(alp);
}

void ExponentWithLinearTransform::getOffset(double(&values)[4]) const
{
    values[0] = getImpl()->getRedParams().size()  == 2 ? getImpl()->getRedParams()  [1] : 0.;
    values[1] = getImpl()->getGreenParams().size()== 2 ? getImpl()->getGreenParams()[1] : 0.;
    values[2] = getImpl()->getBlueParams().size() == 2 ? getImpl()->getBlueParams() [1] : 0.;
    values[3] = getImpl()->getAlphaParams().size()== 2 ? getImpl()->getAlphaParams()[1] : 0.;
}

std::ostream& operator<< (std::ostream& os, const ExponentWithLinearTransform & t)
{
    os << "<ExponentWithLinearTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
    
    double gamma[4];
    t.getGamma(gamma);

    os << "gamma=" << gamma[0];
    for (int i = 1; i < 4; ++i)
    {
      os << " " << gamma[i];
    }
    
    double offset[4];
    t.getOffset(offset);

    os << ", offset=" << offset[0];
    for (int i = 1; i < 4; ++i)
    {
      os << " " << offset[i];
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

OCIO_ADD_TEST(ExponentWithLinearTransform, basic)
{
    OCIO::ExponentWithLinearTransformRcPtr exp = OCIO::ExponentWithLinearTransform::Create();
    OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    exp->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    double val4[4] = { -1., -1. -1. -1. };

    OCIO_CHECK_NO_THROW(exp->getGamma(val4));
    CheckValues(val4, { 1., 1., 1., 1. });

    val4[1] = 2.1234567;
    OCIO_CHECK_NO_THROW(exp->setGamma(val4));
    val4[1] = -1.;
    OCIO_CHECK_NO_THROW(exp->getGamma(val4));
    CheckValues(val4, {1., 2.1234567, 1., 1.});

    OCIO_CHECK_NO_THROW(exp->getOffset(val4));
    CheckValues(val4, { 0., 0., 0., 0. });

    val4[1] = 0.1234567;
    OCIO_CHECK_NO_THROW(exp->setOffset(val4));
    val4[1] = -1.;
    OCIO_CHECK_NO_THROW(exp->getOffset(val4));
    CheckValues(val4, { 0., 0.1234567, 0., 0. });
}

#endif
