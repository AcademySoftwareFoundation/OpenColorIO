/*
Copyright (c) 2018 Autodesk Inc., et al.
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

#include "OpBuilders.h"
#include "ops/Gamma/GammaOpData.h"
#include "ops/Gamma/GammaOps.h"


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


///////////////////////////////////////////////////////////////////////////////////////////////////////

void BuildExponentWithLinearOps(OpRcPtrVec & ops,
                                const Config & /*config*/,
                                const ExponentWithLinearTransform & transform,
                                TransformDirection dir)
{
    TransformDirection combinedDir 
        = CombineTransformDirections(dir, transform.getDirection());
    
    double gamma4[4] = { 1., 1., 1., 1. };
    transform.getGamma(gamma4);

    double offset4[4] = { 0., 0., 0., 0. };
    transform.getOffset(offset4);

    CreateGammaOp(ops, "", OpData::Descriptions(),
                  combinedDir==TRANSFORM_DIR_FORWARD ? GammaOpData::MONCURVE_FWD
                                                     : GammaOpData::MONCURVE_REV,
                  gamma4, offset4);
}

}
OCIO_NAMESPACE_EXIT


////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"


namespace
{

void CheckValues(const double(&v1)[4], const double(&v2)[4])
{
    static const float errThreshold = 1e-8f;

    OIIO_CHECK_CLOSE(v1[0], v2[0], errThreshold);
    OIIO_CHECK_CLOSE(v1[1], v2[1], errThreshold);
    OIIO_CHECK_CLOSE(v1[2], v2[2], errThreshold);
    OIIO_CHECK_CLOSE(v1[3], v2[3], errThreshold);
}

};

OIIO_ADD_TEST(ExponentWithLinearTransform, basic)
{
    OCIO::ExponentWithLinearTransformRcPtr exp = OCIO::ExponentWithLinearTransform::Create();
    OIIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    exp->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    double val4[4] = { -1., -1. -1. -1. };

    OIIO_CHECK_NO_THROW(exp->getGamma(val4));
    CheckValues(val4, { 1., 1., 1., 1. });

    val4[1] = 2.1234567;
    OIIO_CHECK_NO_THROW(exp->setGamma(val4));
    val4[1] = -1.;
    OIIO_CHECK_NO_THROW(exp->getGamma(val4));
    CheckValues(val4, {1., 2.1234567, 1., 1.});

    OIIO_CHECK_NO_THROW(exp->getOffset(val4));
    CheckValues(val4, { 0., 0., 0., 0. });

    val4[1] = 0.1234567;
    OIIO_CHECK_NO_THROW(exp->setOffset(val4));
    val4[1] = -1.;
    OIIO_CHECK_NO_THROW(exp->getOffset(val4));
    CheckValues(val4, { 0., 0.1234567, 0., 0. });
}

#endif
