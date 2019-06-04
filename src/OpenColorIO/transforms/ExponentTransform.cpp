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

#include "OpBuilders.h"
#include "ops/Exponent/ExponentOps.h"
#include "ops/Gamma/GammaOpData.h"
#include "ops/Gamma/GammaOps.h"


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


    
///////////////////////////////////////////////////////////////////////////////////////////////////////

void BuildExponentOps(OpRcPtrVec & ops,
                      const Config & config,
                      const ExponentTransform & transform,
                      TransformDirection dir)
{
    TransformDirection combinedDir = CombineTransformDirections(dir,
        transform.getDirection());
    
    double vec4[4] = { 1., 1., 1., 1. };
    transform.getValue(vec4);

    if(config.getMajorVersion()==1)
    {
        CreateExponentOp(ops,
                         vec4,
                         combinedDir);
    }
    else
    {
        CreateGammaOp(ops, "", OpData::Descriptions(),
                      combinedDir==TRANSFORM_DIR_FORWARD ? GammaOpData::BASIC_FWD
                                                         : GammaOpData::BASIC_REV,
                      vec4, nullptr);
    }
}

    
}
OCIO_NAMESPACE_EXIT


////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"


OIIO_ADD_TEST(ExponentTransform, basic)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();
    OIIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    exp->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    std::vector<float> val4(4, 1.), identity_val4(4, 1.);
    OIIO_CHECK_NO_THROW(exp->getValue(&val4[0]));
    OIIO_CHECK_ASSERT(val4 == identity_val4);

    val4[1] = 2.;
    OIIO_CHECK_NO_THROW(exp->setValue(&val4[0]));
    OIIO_CHECK_NO_THROW(exp->getValue(&val4[0]));
    OIIO_CHECK_ASSERT(val4 != identity_val4);
}

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

OIIO_ADD_TEST(ExponentTransform, double)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();
    OIIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    double val4[4] = { -1., -2., -3., -4. };
    OIIO_CHECK_NO_THROW(exp->getValue(val4));
    CheckValues(val4, { 1., 1., 1., 1. });

    val4[1] = 2.1234567;
    OIIO_CHECK_NO_THROW(exp->setValue(val4));
    val4[1] = -2.;
    OIIO_CHECK_NO_THROW(exp->getValue(val4));
    CheckValues(val4, {1., 2.1234567, 1., 1.});
}

#endif
